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
#  include "../config.h"
#endif /* HAVE_CONFIG_H */

#include <string.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h>

#include <unistd.h>
#include <errno.h>

#include <milter-core.h>
#include "milter-client.h"
#include "milter-client-marshalers.h"

#define MILTER_CLIENT_GET_PRIVATE(obj)                  \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj),                 \
                                 MILTER_CLIENT_TYPE,    \
                                 MilterClientPrivate))

typedef struct _MilterClientPrivate	MilterClientPrivate;
struct _MilterClientPrivate
{
    gboolean running;
    struct sockaddr *address;
    socklen_t address_size;
    MilterClientContextSetupFunc context_setup_func;
    gpointer context_setup_user_data;
    MilterClientContextTeardownFunc context_teardown_func;
    gpointer context_teardown_user_data;
    GList *process_data;
};

typedef struct _MilterClientProcessData
{
    gboolean done;
    MilterClient *client;
    MilterClientContext *context;
    MilterWriter *writer;
    GIOChannel *channel;
    guint watch_id;
} MilterClientProcessData;

G_DEFINE_TYPE(MilterClient, milter_client, G_TYPE_OBJECT);

static void dispose        (GObject         *object);
static void set_property   (GObject         *object,
                            guint            prop_id,
                            const GValue    *value,
                            GParamSpec      *pspec);
static void get_property   (GObject         *object,
                            guint            prop_id,
                            GValue          *value,
                            GParamSpec      *pspec);

static void
milter_client_class_init (MilterClientClass *klass)
{
    GObjectClass *gobject_class;

    gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->dispose      = dispose;
    gobject_class->set_property = set_property;
    gobject_class->get_property = get_property;

    g_type_class_add_private(gobject_class, sizeof(MilterClientPrivate));
}

static void
milter_client_init (MilterClient *client)
{
    MilterClientPrivate *priv;

    priv = MILTER_CLIENT_GET_PRIVATE(client);
    priv->running = FALSE;
    priv->address = NULL;
    priv->address_size = 0;
    priv->context_setup_func = NULL;
    priv->context_setup_user_data = NULL;
    priv->context_teardown_func = NULL;
    priv->context_teardown_user_data = NULL;
    priv->process_data = NULL;
}

static void
process_data_free (MilterClientProcessData *data)
{
    MilterClientPrivate *priv;

    priv = MILTER_CLIENT_GET_PRIVATE(data->client);
    if (priv->context_teardown_func)
        priv->context_teardown_func(data->context,
                                    priv->context_teardown_user_data);
    g_object_unref(data->context);
    g_object_unref(data->writer);
    g_io_channel_unref(data->channel);
    g_free(data);
    g_print("free\n");
}

static void
dispose (GObject *object)
{
    MilterClientPrivate *priv;

    priv = MILTER_CLIENT_GET_PRIVATE(object);

    if (priv->address) {
        g_free(priv->address);
        priv->address = NULL;
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

GQuark
milter_client_error_quark (void)
{
    return g_quark_from_static_string("milter-client-error-quark");
}

MilterClient *
milter_client_new (void)
{
    return g_object_new(MILTER_CLIENT_TYPE,
                        NULL);
}

gboolean
milter_client_set_connection_spec (MilterClient *client, const gchar *spec,
                                   GError **error)
{
    MilterClientPrivate *priv;

    priv = MILTER_CLIENT_GET_PRIVATE(client);
    if (priv->address) {
        g_free(priv->address);
        priv->address = NULL;
    }
    return milter_utils_parse_connection_spec(spec,
                                              &(priv->address),
                                              &(priv->address_size),
                                              error);
}

#define BUFFER_SIZE 4096
static gboolean
feed_to_context (MilterClientContext *context, GIOChannel *channel)
{
    gboolean error_occurred = FALSE;

    while (!error_occurred) {
        GIOStatus status;
        gboolean eof = FALSE;
        gchar stream[BUFFER_SIZE + 1];
        gsize length = 0;
        GError *error = NULL;

        status = g_io_channel_read_chars(channel, stream, BUFFER_SIZE,
                                         &length, &error);
        if (status == G_IO_STATUS_EOF) {
            eof = TRUE;
        }

        if (error) {
            g_print("error: %s\n", error->message);
            error_occurred = TRUE;
            g_error_free(error);
            break;
        }

        if (length <= 0)
            break;

        milter_client_context_feed(context, stream, length, &error);
        if (error) {
            g_print("error: %s\n", error->message);
            error_occurred = TRUE;
            g_error_free(error);
            break;
        }

        if (eof)
            break;
    }

    return !error_occurred;
}

static gboolean
client_watch_func (GIOChannel *channel, GIOCondition condition, gpointer _data)
{
    MilterClientProcessData *data = _data;
    MilterClientContext *context;
    gboolean keep_callback = TRUE;

    g_print("watch!\n");
    context = data->context;
    if (condition & G_IO_IN ||
        condition & G_IO_PRI) {
        keep_callback = feed_to_context(context, channel);
    }

    if (condition & G_IO_ERR ||
        condition & G_IO_HUP ||
        condition & G_IO_NVAL) {
        GArray *messages;
        gchar *message;

        g_print("error!\n");
        messages = g_array_new(TRUE, TRUE, sizeof(gchar *));
        if (condition & G_IO_ERR) {
            message = "Error";
            g_array_append_val(messages, message);
        }
        if (condition & G_IO_HUP) {
            message = "Hung up";
            g_array_append_val(messages, message);
        }
        if (condition & G_IO_NVAL) {
            message = "Invalid request";
            g_array_append_val(messages, message);
        }
        message = g_strjoinv(" | ", (gchar **)(messages->data));
        g_print("%s\n", message);
        g_free(message);
        g_array_free(messages, TRUE);
        keep_callback = FALSE;
    }

    if (!keep_callback) {
        data->done = TRUE;
        g_print("done\n");
    }

    return keep_callback;
}

static MilterStatus
cb_close (MilterClientContext *context, gpointer _data)
{
    MilterClientProcessData *data = _data;

    data->done = TRUE;
    g_print("ID: %d\n", data->watch_id);
    if (data->watch_id > 0)
        g_source_remove(data->watch_id);
    data->watch_id = -1;

    return MILTER_STATUS_CONTINUE;
}

static void
process_client_channel (MilterClient *client, GIOChannel *channel)
{
    MilterClientPrivate *priv;
    MilterClientContext *context;
    MilterWriter *writer;
    MilterClientProcessData *data;

    priv = MILTER_CLIENT_GET_PRIVATE(client);

    writer = milter_writer_io_channel_new(channel);
    context = milter_client_context_new();
    milter_handler_set_writer(MILTER_HANDLER(context), writer);
    if (priv->context_setup_func)
        priv->context_setup_func(context, priv->context_setup_user_data);

    data = g_new(MilterClientProcessData, 1);
    data->done = FALSE;
    data->client = client;
    data->context = context;
    data->writer = writer;
    data->channel = channel;

    data->watch_id = g_io_add_watch(channel,
                                    G_IO_IN | G_IO_PRI |
                                    G_IO_ERR | G_IO_HUP | G_IO_NVAL,
                                    client_watch_func, data);

    g_signal_connect(context, "close", G_CALLBACK(cb_close), data);

    priv->process_data = g_list_prepend(priv->process_data, data);
}

static gboolean
accept_child (gint server_fd, MilterClient *client)
{
    gint client_fd;
    GIOChannel *client_channel;

    g_print("start!\n");
    client_fd = accept(server_fd, NULL, NULL);
    g_print("accept!\n");

    client_channel = g_io_channel_unix_new(client_fd);
    g_io_channel_set_encoding(client_channel, NULL, NULL);
    g_io_channel_set_flags(client_channel,
                           G_IO_FLAG_NONBLOCK |
                           G_IO_FLAG_IS_READABLE |
                           G_IO_FLAG_IS_WRITEABLE,
                           NULL);
    g_io_channel_set_close_on_unref(client_channel, TRUE);
    process_client_channel(client, client_channel);

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
        keep_callback = accept_child(g_io_channel_unix_get_fd(channel),
                                     client);
    }

    if (condition & G_IO_ERR ||
        condition & G_IO_HUP ||
        condition & G_IO_NVAL) {
        GArray *messages;
        gchar *message;

        g_print("server error!\n");
        messages = g_array_new(TRUE, TRUE, sizeof(gchar *));
        if (condition & G_IO_ERR) {
            message = "Error";
            g_array_append_val(messages, message);
        }
        if (condition & G_IO_HUP) {
            message = "Hung up";
            g_array_append_val(messages, message);
        }
        if (condition & G_IO_NVAL) {
            message = "Invalid request";
            g_array_append_val(messages, message);
        }
        message = g_strjoinv(" | ", (gchar **)(messages->data));
        g_print("%s\n", message);
        g_free(message);
        g_array_free(messages, TRUE);
        keep_callback = FALSE;
    }

    if (!keep_callback)
        priv->running = FALSE;
    return keep_callback;
}

gboolean
milter_client_main (MilterClient *client)
{
    MilterClientPrivate *priv;
    gboolean reuse_address = TRUE;
    GIOChannel *server_channel;
    gint server_fd;

    priv = MILTER_CLIENT_GET_PRIVATE(client);
    server_fd = socket(PF_INET, SOCK_STREAM, 0);
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR,
               &reuse_address, sizeof(reuse_address));

    if (!priv->address)
        milter_client_set_connection_spec(client, "inet:11111", NULL);

    if (bind(server_fd, priv->address, priv->address_size) == -1) {
        g_print("failed to bind(): %s\n", strerror(errno));
        close(server_fd);
        return FALSE;
    }

    if (listen(server_fd, 5) == -1) {
        g_print("failed to listen(): %s\n", strerror(errno));
        close(server_fd);
        return FALSE;
    };

    server_channel = g_io_channel_unix_new(server_fd);
    g_io_channel_set_flags(server_channel, G_IO_FLAG_IS_READABLE, NULL);
    g_io_channel_set_close_on_unref(server_channel, TRUE);
    g_io_add_watch(server_channel,
                   G_IO_IN | G_IO_PRI |
                   G_IO_ERR | G_IO_HUP | G_IO_NVAL,
                   server_watch_func, client);
    priv->running = TRUE;
    while (priv->running) {
        GList *node;

        g_print("loop\n");
        g_main_context_iteration(NULL, TRUE);
        g_print("loop done\n");

        for (node = priv->process_data; node; node = g_list_next(node)) {
            MilterClientProcessData *data = node->data;

            if (data->done) {
                g_print("delete\n");
                process_data_free(data);
                node = g_list_delete_link(priv->process_data, node);
                g_print("%p\n", node);
                priv->process_data = node;
            }
        }
    }
    g_print("finish\n");
    g_io_channel_unref(server_channel);

    return TRUE;
}

void
milter_client_set_context_setup_func (MilterClient *client,
                                      MilterClientContextSetupFunc setup_func,
                                      gpointer user_data)
{
    MilterClientPrivate *priv;

    priv = MILTER_CLIENT_GET_PRIVATE(client);
    priv->context_setup_func = setup_func;
    priv->context_setup_user_data = user_data;
}

void
milter_client_set_context_teardown_func (MilterClient *client,
                                         MilterClientContextTeardownFunc teardown_func,
                                         gpointer user_data)
{
    MilterClientPrivate *priv;

    priv = MILTER_CLIENT_GET_PRIVATE(client);
    priv->context_teardown_func = teardown_func;
    priv->context_teardown_user_data = user_data;
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
