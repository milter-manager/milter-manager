/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Copyright (C) 2008  Kouhei Sutou <kou@cozmixng.org>
 *
 *  This library is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this library.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifdef HAVE_CONFIG_H
#  include "../../config.h"
#endif /* HAVE_CONFIG_H */

#include <string.h>
#include <stdlib.h>

#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>

#include <glib/gstdio.h>

#include <milter/core/milter-marshalers.h>
#include "../client.h"

typedef struct _GenericSocketAddress
{
    union {
        struct sockaddr address;
        struct sockaddr_un address_un;
        struct sockaddr_in address_inet;
        struct sockaddr_in6 address_inet6;
    } addresses;
} GenericSocketAddress;

enum
{
    CONNECTION_ESTABLISHED,
    LISTEN_STARTED,
    LAST_SIGNAL
};

static gint signals[LAST_SIGNAL] = {0};

#define MILTER_CLIENT_GET_PRIVATE(obj)                  \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj),                 \
                                 MILTER_TYPE_CLIENT,    \
                                 MilterClientPrivate))

typedef struct _MilterClientPrivate	MilterClientPrivate;
struct _MilterClientPrivate
{
    GIOChannel *listening_channel;
    GMainLoop *accept_loop;
    GMainLoop *main_loop;
    guint server_watch_id;
    gchar *connection_spec;
    GList *processing_data;
    guint n_processing_data;
    guint timeout;
    GIOChannel *listen_channel;
    gint listen_backlog;
    GMutex *quit_mutex;
    gboolean quitting;
    guint default_unix_socket_mode;
    gboolean default_remove_unix_socket_on_close;
    gboolean remove_unix_socket_on_create;
};

typedef struct _MilterClientProcessData
{
    MilterClientPrivate *priv;
    MilterClient *client;
    MilterClientContext *context;
} MilterClientProcessData;

MILTER_DEFINE_ERROR_EMITTABLE_TYPE(MilterClient, milter_client, G_TYPE_OBJECT)

static void dispose        (GObject         *object);
static void set_property   (GObject         *object,
                            guint            prop_id,
                            const GValue    *value,
                            GParamSpec      *pspec);
static void get_property   (GObject         *object,
                            guint            prop_id,
                            GValue          *value,
                            GParamSpec      *pspec);

static gchar *get_default_connection_spec
                           (MilterClient    *client);
static void   listen_started
                           (MilterClient    *client,
                            struct sockaddr *address,
                            socklen_t        address_size);
static void   cb_finished  (MilterClientContext *context,
                            gpointer _data);

static void
milter_client_class_init (MilterClientClass *klass)
{
    GObjectClass *gobject_class;
    MilterClientClass *client_class = NULL;

    gobject_class = G_OBJECT_CLASS(klass);
    client_class = MILTER_CLIENT_CLASS(klass);

    gobject_class->dispose      = dispose;
    gobject_class->set_property = set_property;
    gobject_class->get_property = get_property;

    client_class->get_default_connection_spec = get_default_connection_spec;
    client_class->listen_started = listen_started;

    signals[CONNECTION_ESTABLISHED] =
        g_signal_new("connection-established",
                     MILTER_TYPE_CLIENT,
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterClientClass, connection_established),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__OBJECT,
                     G_TYPE_NONE, 1, MILTER_TYPE_CLIENT_CONTEXT);

    signals[LISTEN_STARTED] =
        g_signal_new("listen-started",
                     MILTER_TYPE_CLIENT,
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterClientClass, listen_started),
                     NULL, NULL,
                     _milter_marshal_VOID__POINTER_UINT,
                     G_TYPE_NONE, 2, G_TYPE_POINTER, G_TYPE_UINT);

    g_type_class_add_private(gobject_class, sizeof(MilterClientPrivate));
}

static void
milter_client_init (MilterClient *client)
{
    MilterClientPrivate *priv;
    GMainContext *main_context;

    priv = MILTER_CLIENT_GET_PRIVATE(client);

    priv->listening_channel = NULL;

    main_context = g_main_context_new();
    priv->accept_loop = g_main_loop_new(main_context, FALSE);
    g_main_context_unref(main_context);

    priv->main_loop = g_main_loop_new(NULL, FALSE);

    priv->server_watch_id = 0;
    priv->connection_spec = NULL;
    priv->processing_data = NULL;
    priv->n_processing_data = 0;
    priv->timeout = 7210;
    priv->listen_channel = NULL;
    priv->listen_backlog = -1;
    priv->quit_mutex = g_mutex_new();
    priv->quitting = FALSE;
    priv->default_unix_socket_mode = 0660;
    priv->default_remove_unix_socket_on_close = TRUE;
    priv->remove_unix_socket_on_create = TRUE;
}

static void
process_data_free (MilterClientProcessData *data)
{
    g_signal_handlers_disconnect_by_func(data->context,
                                         G_CALLBACK(cb_finished), data);
    g_object_unref(data->context);
    g_free(data);
}

static void
dispose (GObject *object)
{
    MilterClientPrivate *priv;

    priv = MILTER_CLIENT_GET_PRIVATE(object);

    if (priv->listening_channel) {
        g_io_channel_unref(priv->listening_channel);
        priv->listening_channel = NULL;
    }

    if (priv->accept_loop) {
        g_main_loop_unref(priv->accept_loop);
        priv->accept_loop = NULL;
    }

    if (priv->main_loop) {
        g_main_loop_unref(priv->main_loop);
        priv->main_loop = NULL;
    }

    if (priv->server_watch_id > 0) {
        g_source_remove(priv->server_watch_id);
        priv->server_watch_id = 0;
    }

    if (priv->connection_spec) {
        g_free(priv->connection_spec);
        priv->connection_spec = NULL;
    }

    if (priv->processing_data) {
        g_list_foreach(priv->processing_data, (GFunc)process_data_free, NULL);
        g_list_free(priv->processing_data);
        priv->processing_data = NULL;
        priv->n_processing_data = 0;
    }

    if (priv->listen_channel) {
        g_io_channel_unref(priv->listen_channel);
        priv->listen_channel = NULL;
    }

    if (priv->quit_mutex) {
        g_mutex_free(priv->quit_mutex);
        priv->quit_mutex = NULL;
    }

    G_OBJECT_CLASS(milter_client_parent_class)->dispose(object);
}

static void
set_property (GObject      *object,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
    MilterClientPrivate *priv;

    priv = MILTER_CLIENT_GET_PRIVATE(object);
    switch (prop_id) {
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static void
get_property (GObject    *object,
              guint       prop_id,
              GValue     *value,
              GParamSpec *pspec)
{
    MilterClientPrivate *priv;

    priv = MILTER_CLIENT_GET_PRIVATE(object);
    switch (prop_id) {
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

GQuark
milter_client_error_quark (void)
{
    return g_quark_from_static_string("milter-client-error-quark");
}

MilterClient *
milter_client_new (void)
{
    return g_object_new(MILTER_TYPE_CLIENT,
                        NULL);
}

static void
listen_started (MilterClient *client,
                struct sockaddr *address, socklen_t address_size)
{
    struct sockaddr_un *address_un;
    guint mode;

    if (address->sa_family != AF_UNIX)
        return;

    address_un = (struct sockaddr_un *)address;
    mode = milter_client_get_unix_socket_mode(client);
    if (g_chmod(address_un->sun_path, mode) == -1) {
        GError *error;

        error = g_error_new(MILTER_CLIENT_ERROR,
                            MILTER_CLIENT_ERROR_UNIX_SOCKET,
                            "failed to change the mode of UNIX socket: "
                            "%s(%o): %s",
                            address_un->sun_path, mode, g_strerror(errno));
        milter_error("%s", error->message);
        milter_error_emittable_emit(MILTER_ERROR_EMITTABLE(client), error);
        g_error_free(error);
    }
}

static gchar *
get_default_connection_spec (MilterClient *client)
{
    return g_strdup("inet:10025@[127.0.0.1]");
}

gchar *
milter_client_get_default_connection_spec (MilterClient *client)
{
    MilterClientClass *klass;

    klass = MILTER_CLIENT_GET_CLASS(client);
    if (klass->get_default_connection_spec)
        return klass->get_default_connection_spec(client);
    else
        return NULL;
}

gboolean
milter_client_set_connection_spec (MilterClient *client, const gchar *spec,
                                   GError **error)
{
    MilterClientPrivate *priv;
    gboolean success;

    priv = MILTER_CLIENT_GET_PRIVATE(client);
    if (priv->connection_spec) {
        g_free(priv->connection_spec);
        priv->connection_spec = NULL;
    }

    if (!spec)
        return TRUE;

    success = milter_connection_parse_spec(spec, NULL, NULL, NULL, error);
    if (success)
        priv->connection_spec = g_strdup(spec);

    return success;
}

GIOChannel *
milter_client_get_listen_channel (MilterClient *client)
{
    return MILTER_CLIENT_GET_PRIVATE(client)->listen_channel;
}

void
milter_client_set_listen_channel (MilterClient *client, GIOChannel *channel)
{
    MilterClientPrivate *priv;

    priv = MILTER_CLIENT_GET_PRIVATE(client);
    if (priv->listen_channel)
        g_io_channel_unref(priv->listen_channel);
    priv->listen_channel = channel;
    if (priv->listen_channel)
        g_io_channel_ref(priv->listen_channel);
}

static gboolean
cb_idle_free_data (gpointer _data)
{
    MilterClientProcessData *data = _data;
    guint n_processing_data;

    milter_debug("removing a MilterClientContext");
    data->priv->processing_data =
        g_list_remove(data->priv->processing_data, data);
    data->priv->n_processing_data--;

    n_processing_data = data->priv->n_processing_data;
    g_mutex_lock(data->priv->quit_mutex);
    if (data->priv->quitting && n_processing_data == 0) {
        milter_debug("quit main loop");
        g_main_loop_quit(data->priv->main_loop);
    }
    g_mutex_unlock(data->priv->quit_mutex);

    process_data_free(data);
    milter_debug("removed a MilterClientContext: rest: <%u>", n_processing_data);

    return FALSE;
}

static void
cb_finished (MilterClientContext *context, gpointer _data)
{
    MilterClientProcessData *data = _data;

    g_idle_add(cb_idle_free_data, data);
}

typedef struct _ClientChannelSetupData
{
    MilterClient *client;
    GIOChannel *channel;
} ClientChannelSetupData;

static gboolean
cb_idle_client_channel_setup (gpointer user_data)
{
    ClientChannelSetupData *setup_data = user_data;
    MilterClient *client;
    MilterClientPrivate *priv;
    MilterClientContext *context;
    MilterWriter *writer;
    MilterReader *reader;
    MilterClientProcessData *data;

    client = setup_data->client;
    priv = MILTER_CLIENT_GET_PRIVATE(client);

    writer = milter_writer_io_channel_new(setup_data->channel);
    context = milter_client_context_new();
    milter_agent_set_writer(MILTER_AGENT(context), writer);
    g_object_unref(writer);

    reader = milter_reader_io_channel_new(setup_data->channel);
    milter_agent_set_reader(MILTER_AGENT(context), reader);
    g_object_unref(reader);

    milter_client_context_set_timeout(context, priv->timeout);

    data = g_new(MilterClientProcessData, 1);
    data->priv = priv;
    data->client = client;
    data->context = context;

    g_signal_connect(context, "finished", G_CALLBACK(cb_finished), data);

    priv->processing_data = g_list_prepend(priv->processing_data, data);
    priv->n_processing_data++;

    g_signal_emit(client, signals[CONNECTION_ESTABLISHED], 0, context);
    milter_agent_start(MILTER_AGENT(context));

    g_io_channel_unref(setup_data->channel);
    g_free(setup_data);

    return FALSE;
}

static void
process_client_channel (MilterClient *client, GIOChannel *channel)
{
    ClientChannelSetupData *data;

    data = g_new(ClientChannelSetupData, 1);
    data->client = client;
    data->channel = channel;
    g_io_channel_ref(channel);

    g_idle_add(cb_idle_client_channel_setup, data);
}

static gboolean
accept_client (gint server_fd, MilterClient *client)
{
    gint client_fd;
    GIOChannel *client_channel;
    GenericSocketAddress address;
    gchar *spec;
    socklen_t address_size;

    address_size = sizeof(address);
    memset(&address, '\0', address_size);
    milter_debug("start accepting...");
    client_fd = accept(server_fd, (struct sockaddr *)(&address), &address_size);
    if (client_fd == -1) {
        GError *error = NULL;
        g_set_error(&error,
                    MILTER_CONNECTION_ERROR,
                    MILTER_CONNECTION_ERROR_ACCEPT_FAILURE,
                    "failed to accept(): %s", g_strerror(errno));
        milter_error("failed to accept(): %s", g_strerror(errno));
        milter_error_emittable_emit(MILTER_ERROR_EMITTABLE(client),
                                    error);
        g_error_free(error);
        return TRUE;
    }

    spec = milter_connection_address_to_spec(&(address.addresses.address));
    milter_debug("accepted from %s.", spec);
    g_free(spec);

    client_channel = g_io_channel_unix_new(client_fd);
    g_io_channel_set_encoding(client_channel, NULL, NULL);
    g_io_channel_set_flags(client_channel, G_IO_FLAG_NONBLOCK, NULL);
    g_io_channel_set_close_on_unref(client_channel, TRUE);
    process_client_channel(client, client_channel);
    g_io_channel_unref(client_channel);

    return TRUE;
}

static gboolean
server_watch_func (GIOChannel *channel, GIOCondition condition, gpointer data)
{
    MilterClient *client = data;
    MilterClientPrivate *priv;
    gboolean keep_callback = TRUE;

    priv = MILTER_CLIENT_GET_PRIVATE(client);
    if (condition & G_IO_IN ||
        condition & G_IO_PRI) {
        keep_callback = accept_client(g_io_channel_unix_get_fd(channel), client);
    }

    if (condition & G_IO_ERR ||
        condition & G_IO_HUP ||
        condition & G_IO_NVAL) {
        gchar *message;
        GError *error = NULL;

        message = milter_utils_inspect_io_condition_error(condition);

        error = g_error_new(MILTER_CLIENT_ERROR,
                            MILTER_CLIENT_ERROR_IO_ERROR,
                            "IO error on waiting MTA connection socket: %s",
                            message);
        milter_error("%s", error->message);
        milter_error_emittable_emit(MILTER_ERROR_EMITTABLE(client),
                                    error);
        g_error_free(error);

        keep_callback = FALSE;
    }

    if (!keep_callback)
        milter_client_shutdown(client);
    return keep_callback;
}

static gboolean
cb_timeout_unlock_quit_mutex (gpointer data)
{
    MilterClient *client = data;
    MilterClientPrivate *priv;

    priv = MILTER_CLIENT_GET_PRIVATE(client);

    g_mutex_unlock(priv->quit_mutex);

    return FALSE;
}

static gpointer
server_accept_thread (gpointer data)
{
    MilterClient *client = data;
    MilterClientPrivate *priv;
    GSource *timeout_source;

    priv = MILTER_CLIENT_GET_PRIVATE(client);

    timeout_source = g_timeout_source_new(0);
    g_source_set_callback(timeout_source,
                          cb_timeout_unlock_quit_mutex,
                          client, NULL);
    g_source_attach(timeout_source, g_main_loop_get_context(priv->accept_loop));
    g_source_unref(timeout_source);

    g_mutex_lock(priv->quit_mutex);
    if (priv->quitting)
        g_mutex_unlock(priv->quit_mutex);
    else
        g_main_loop_run(priv->accept_loop);

    return NULL;
}


gboolean
milter_client_main (MilterClient *client)
{
    MilterClientPrivate *priv;
    GError *error = NULL;
    GSource *watch_source;
    GThread *thread;
    struct sockaddr *address = NULL;
    socklen_t address_size = 0;

    priv = MILTER_CLIENT_GET_PRIVATE(client);

    if (priv->listening_channel || priv->n_processing_data > 0) {
        GError *error;

        error = g_error_new(MILTER_CLIENT_ERROR,
                            MILTER_CLIENT_ERROR_RUNNING,
                            "The milter client is already running: <%p>",
                            client);
        milter_error("%s", error->message);
        milter_error_emittable_emit(MILTER_ERROR_EMITTABLE(client), error);
        g_error_free(error);
        return FALSE;
    }

    priv->quitting = FALSE;
    if (!priv->connection_spec) {
        gchar *default_connection_spec;

        default_connection_spec =
            milter_client_get_default_connection_spec(client);
        milter_client_set_connection_spec(client, default_connection_spec, NULL);
        g_free(default_connection_spec);
    }

    if (priv->listen_channel) {
        g_io_channel_ref(priv->listen_channel);
        priv->listening_channel = priv->listen_channel;
    } else {
        priv->listening_channel =
            milter_connection_listen(priv->connection_spec,
                                     priv->listen_backlog,
                                     &address,
                                     &address_size,
                                     priv->remove_unix_socket_on_create,
                                     &error);
        if (address_size > 0)
            g_signal_emit(client, signals[LISTEN_STARTED], 0,
                          address, address_size);
    }

    if (!priv->listening_channel) {
        milter_error("%s", error->message);
        milter_error_emittable_emit(MILTER_ERROR_EMITTABLE(client), error);
        g_error_free(error);
        return FALSE;
    }

    watch_source = g_io_create_watch(priv->listening_channel,
                                     G_IO_IN | G_IO_PRI |
                                     G_IO_ERR | G_IO_HUP | G_IO_NVAL);
    g_source_set_callback(watch_source, (GSourceFunc)server_watch_func,
                          client, NULL);
    priv->server_watch_id =
        g_source_attach(watch_source,
                        g_main_loop_get_context(priv->accept_loop));
    g_source_unref(watch_source);

    thread = g_thread_create(server_accept_thread, client, TRUE, NULL);

    g_mutex_lock(priv->quit_mutex);
    if (priv->quitting) {
        g_mutex_unlock(priv->quit_mutex);
    } else {
        g_timeout_add(0, cb_timeout_unlock_quit_mutex, client);
        g_main_loop_run(priv->main_loop);
    }
    g_thread_join(thread);

    if (priv->listening_channel) {
        g_io_channel_unref(priv->listening_channel);
        priv->listening_channel = NULL;
    }

    if (address) {
        if (address->sa_family == AF_UNIX &&
            milter_client_is_remove_unix_socket_on_close(client)) {
            struct sockaddr_un *address_un;

            address_un = (struct sockaddr_un *)address;
            if (g_unlink(address_un->sun_path) == -1) {
                GError *error;

                error = g_error_new(MILTER_CLIENT_ERROR,
                                    MILTER_CLIENT_ERROR_UNIX_SOCKET,
                                    "failed to remove used UNIX socket: %s: %s",
                                    address_un->sun_path, g_strerror(errno));
                milter_error("%s", error->message);
                milter_error_emittable_emit(MILTER_ERROR_EMITTABLE(client),
                                            error);
                g_error_free(error);
            }
        }
        g_free(address);
    }

    return TRUE;
}

void
milter_client_shutdown (MilterClient *client)
{
    MilterClientPrivate *priv;

    priv = MILTER_CLIENT_GET_PRIVATE(client);

    g_mutex_lock(priv->quit_mutex);
    if (!priv->quitting) {
        priv->quitting = TRUE;
        g_main_loop_quit(priv->accept_loop);
        if (priv->server_watch_id > 0) {
            g_source_remove(priv->server_watch_id);
            priv->server_watch_id = 0;
        }
        g_io_channel_unref(priv->listening_channel);
        priv->listening_channel = NULL;

        if (priv->n_processing_data == 0)
            g_main_loop_quit(priv->main_loop);
    }
    g_mutex_unlock(priv->quit_mutex);
}

void
milter_client_set_listen_backlog (MilterClient *client, gint backlog)
{
    MILTER_CLIENT_GET_PRIVATE(client)->listen_backlog = backlog;
}

gboolean
milter_client_is_remove_unix_socket_on_create (MilterClient *client)
{
    return MILTER_CLIENT_GET_PRIVATE(client)->remove_unix_socket_on_create;
}

void
milter_client_set_remove_unix_socket_on_create (MilterClient *client,
                                                gboolean remove)
{
    MILTER_CLIENT_GET_PRIVATE(client)->remove_unix_socket_on_create = remove;
}

void
milter_client_set_timeout (MilterClient *client, guint timeout)
{
    MILTER_CLIENT_GET_PRIVATE(client)->timeout = timeout;
}

guint
milter_client_get_unix_socket_mode (MilterClient *client)
{
    MilterClientClass *klass;

    klass = MILTER_CLIENT_GET_CLASS(client);
    if (klass->get_unix_socket_mode)
        return klass->get_unix_socket_mode(client);
    else
        return milter_client_get_default_unix_socket_mode(client);
}

guint
milter_client_get_default_unix_socket_mode (MilterClient *client)
{
    return MILTER_CLIENT_GET_PRIVATE(client)->default_unix_socket_mode;
}

void
milter_client_set_default_unix_socket_mode (MilterClient *client, guint mode)
{
    MILTER_CLIENT_GET_PRIVATE(client)->default_unix_socket_mode = mode;
}

gboolean
milter_client_is_remove_unix_socket_on_close (MilterClient *client)
{
    MilterClientClass *klass;

    klass = MILTER_CLIENT_GET_CLASS(client);
    if (klass->is_remove_unix_socket_on_close)
        return klass->is_remove_unix_socket_on_close(client);
    else
        return milter_client_get_default_remove_unix_socket_on_close(client);
}

gboolean
milter_client_get_default_remove_unix_socket_on_close (MilterClient *client)
{
    return MILTER_CLIENT_GET_PRIVATE(client)->default_remove_unix_socket_on_close;
}

void
milter_client_set_default_remove_unix_socket_on_close (MilterClient *client,
                                                       gboolean remove)
{
    MILTER_CLIENT_GET_PRIVATE(client)->default_remove_unix_socket_on_close = remove;
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
