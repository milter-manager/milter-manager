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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
#include <netinet/ip.h>

#include "../client.h"

enum
{
    CONNECTION_ESTABLISHED,
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
    GMainLoop *accept_loop;
    GMainLoop *main_loop;
    guint server_watch_id;
    gchar *connection_spec;
    GList *process_data;
    guint timeout;
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

    signals[CONNECTION_ESTABLISHED] =
        g_signal_new("connection-established",
                     MILTER_TYPE_CLIENT,
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterClientClass, connection_established),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__OBJECT,
                     G_TYPE_NONE, 1, MILTER_TYPE_CLIENT_CONTEXT);

    g_type_class_add_private(gobject_class, sizeof(MilterClientPrivate));
}

static void
milter_client_init (MilterClient *client)
{
    MilterClientPrivate *priv;
    GMainContext *main_context;

    priv = MILTER_CLIENT_GET_PRIVATE(client);

    main_context = g_main_context_new();
    priv->accept_loop = g_main_loop_new(main_context, FALSE);
    g_main_context_unref(main_context);
    priv->main_loop = g_main_loop_new(NULL, FALSE);

    priv->server_watch_id = 0;
    priv->connection_spec = NULL;
    priv->process_data = NULL;
    priv->timeout = 7210;
}

static void
process_data_free (MilterClientProcessData *data)
{
    g_object_unref(data->context);
    g_free(data);
}

static void
dispose (GObject *object)
{
    MilterClientPrivate *priv;

    priv = MILTER_CLIENT_GET_PRIVATE(object);

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

    if (priv->process_data) {
        g_list_foreach(priv->process_data, (GFunc)process_data_free, NULL);
        g_list_free(priv->process_data);
        priv->process_data = NULL;
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

MilterClient *
milter_client_new (void)
{
    return g_object_new(MILTER_TYPE_CLIENT,
                        NULL);
}

static gchar *
get_default_connection_spec (MilterClient *client)
{
    return g_strdup("inet:10025");
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

static gboolean
cb_idle_free_data (gpointer _data)
{
    MilterClientProcessData *data = _data;

    milter_info("removing a MilterClientContext");
    /* FIXME: should use mutex */
    data->priv->process_data = g_list_remove(data->priv->process_data, data);
    process_data_free(data);
    milter_info("removed a MilterClientContext");

    return FALSE;
}

static void
cb_finished (MilterClientContext *context, gpointer _data)
{
    MilterClientProcessData *data = _data;
    GSource *source;

    source = g_idle_source_new();
    g_source_set_callback(source, cb_idle_free_data, data, NULL);
    g_source_attach(source, g_main_loop_get_context(data->priv->accept_loop));
    g_source_unref(source);
}

static void
process_client_channel (MilterClient *client, GIOChannel *channel)
{
    MilterClientPrivate *priv;
    MilterClientContext *context;
    MilterWriter *writer;
    MilterReader *reader;
    MilterClientProcessData *data;

    priv = MILTER_CLIENT_GET_PRIVATE(client);

    writer = milter_writer_io_channel_new(channel);
    context = milter_client_context_new();
    milter_agent_set_writer(MILTER_AGENT(context), writer);
    g_object_unref(writer);
    reader = milter_reader_io_channel_new(channel);
    milter_agent_set_reader(MILTER_AGENT(context), reader);
    g_object_unref(reader);
    milter_client_context_set_mta_timeout(context, priv->timeout);

    data = g_new(MilterClientProcessData, 1);
    data->priv = priv;
    data->client = client;
    data->context = context;

    g_signal_connect(context, "finished", G_CALLBACK(cb_finished), data);

    /* FIXME: should use mutex */
    priv->process_data = g_list_prepend(priv->process_data, data);

    /* FIXME: should this be done in the accept thread? or
     * main thread by using g_idle_add? */
    g_signal_emit(client, signals[CONNECTION_ESTABLISHED], 0, context);
}

static gboolean
accept_client (gint server_fd, MilterClient *client)
{
    gint client_fd;
    GIOChannel *client_channel;
    struct sockaddr address;
    gchar *spec;
    socklen_t address_size;

    milter_info("start accepting...");
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

    spec = milter_connection_address_to_spec(&address);
    milter_info("accepted from %s.", spec);
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

        g_set_error(&error,
                    MILTER_CONNECTION_ERROR,
                    MILTER_CONNECTION_ERROR_IO_ERROR,
                    "%s", message);
        milter_error_emittable_emit(MILTER_ERROR_EMITTABLE(client),
                                    error);
        g_error_free(error);

        milter_error("%s", message);
        g_free(message);
        keep_callback = FALSE;
    }

    if (!keep_callback)
        milter_client_shutdown(client);
    return keep_callback;
}

static gpointer
server_accept_thread (gpointer data)
{
    MilterClient *client = data;
    MilterClientPrivate *priv;

    priv = MILTER_CLIENT_GET_PRIVATE(client);

    g_main_loop_run(priv->accept_loop);

    return NULL;
}

gboolean
milter_client_main (MilterClient *client)
{
    MilterClientPrivate *priv;
    GError *error = NULL;
    GIOChannel *server_channel;
    GSource *watch_source;
    GThread *thread;

    priv = MILTER_CLIENT_GET_PRIVATE(client);

    if (!priv->connection_spec) {
        gchar *default_connection_spec;

        default_connection_spec =
            milter_client_get_default_connection_spec(client);
        milter_client_set_connection_spec(client, default_connection_spec, NULL);
        g_free(default_connection_spec);
    }

    server_channel = milter_connection_listen(priv->connection_spec, 5, &error);
    if (!server_channel) {
        milter_error_emittable_emit(MILTER_ERROR_EMITTABLE(client), error);
        g_error_free(error);
        return FALSE;
    }

    watch_source = g_io_create_watch(server_channel,
                                     G_IO_IN | G_IO_PRI |
                                     G_IO_ERR | G_IO_HUP | G_IO_NVAL);
    g_source_set_callback(watch_source, (GSourceFunc)server_watch_func,
                          client, NULL);
    priv->server_watch_id =
        g_source_attach(watch_source,
                        g_main_loop_get_context(priv->main_loop));
    g_source_unref(watch_source);

    thread = g_thread_create(server_accept_thread, client, TRUE, NULL);
    g_main_loop_run(priv->main_loop);
    g_thread_join(thread);

    g_io_channel_unref(server_channel);

    return TRUE;
}

void
milter_client_shutdown (MilterClient *client)
{
    MilterClientPrivate *priv;

    priv = MILTER_CLIENT_GET_PRIVATE(client);
    g_main_loop_quit(priv->accept_loop);
    g_main_loop_quit(priv->main_loop);
}

void
milter_client_set_timeout (MilterClient *client, guint timeout)
{
    MILTER_CLIENT_GET_PRIVATE(client)->timeout = timeout;
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
