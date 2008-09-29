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

#include <milter-core.h>
#include "milter-client.h"
#include "milter-client-marshalers.h"

static gboolean need_more = TRUE;

#define MILTER_CLIENT_GET_PRIVATE(obj)                  \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj),                 \
                                 MILTER_CLIENT_TYPE,    \
                                 MilterClientPrivate))

typedef struct _MilterClientPrivate	MilterClientPrivate;
struct _MilterClientPrivate
{
    gint server_fd;
    MilterClientContextSetupFunc context_setup_func;
    gpointer context_setup_user_data;
    MilterClientContextTeardownFunc context_teardown_func;
    gpointer context_teardown_user_data;
};

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
    priv->server_fd = -1;
    priv->context_setup_func = NULL;
    priv->context_setup_user_data = NULL;
    priv->context_teardown_func = NULL;
    priv->context_teardown_user_data = NULL;
}

static void
dispose (GObject *object)
{
    MilterClientPrivate *priv;

    priv = MILTER_CLIENT_GET_PRIVATE(object);

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
            need_more = FALSE;
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
watch_func (GIOChannel *channel, GIOCondition condition, gpointer data)
{
    MilterClientContext *context = data;
    gboolean keep_callback = TRUE;

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

    if (!keep_callback)
        need_more = FALSE;
    return keep_callback;
}

static void
process_client_channel (MilterClient *client, GIOChannel *channel)
{
    MilterClientPrivate *priv;
    MilterClientContext *context;
    MilterWriter *writer;

    priv = MILTER_CLIENT_GET_PRIVATE(client);

    writer = milter_writer_io_channel_new(channel);
    context = milter_client_context_new();
    milter_client_context_set_writer(context, writer);
    if (priv->context_setup_func)
        priv->context_setup_func(context, priv->context_setup_user_data);
    g_io_add_watch(channel,
                   G_IO_IN | G_IO_PRI |
                   G_IO_ERR | G_IO_HUP | G_IO_NVAL,
                   watch_func, context);
    while (need_more) {
        g_main_context_iteration(NULL, TRUE);
    }
    if (priv->context_teardown_func)
        priv->context_teardown_func(context, priv->context_teardown_user_data);
    g_object_unref(context);
    g_object_unref(writer);
}

gboolean
milter_client_main (MilterClient *client)
{
    MilterClientPrivate *priv;
    struct sockaddr_in server_address;
    struct sockaddr_in client_address;
    socklen_t client_address_size;
    gboolean reuse_address = TRUE;
    gint client_fd;

    priv = MILTER_CLIENT_GET_PRIVATE(client);
    priv->server_fd = socket(PF_INET, SOCK_STREAM, 0);
    setsockopt(priv->server_fd, SOL_SOCKET, SO_REUSEADDR,
               &reuse_address, sizeof(reuse_address));

    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(10025);
    bind(priv->server_fd,
         (struct sockaddr *)&server_address,
         sizeof(server_address));

    listen(priv->server_fd, 5);
    client_address_size = sizeof(client_address);
    client_fd = accept(priv->server_fd,
                       (struct sockaddr *)&client_address,
                       &client_address_size);
    {
        GIOChannel *client_channel;

        client_channel = g_io_channel_unix_new(client_fd);
        g_io_channel_set_encoding(client_channel, NULL, NULL);
        g_io_channel_set_flags(client_channel,
                               G_IO_FLAG_NONBLOCK |
                               G_IO_FLAG_IS_READABLE |
                               G_IO_FLAG_IS_WRITEABLE,
                               NULL);
        g_io_channel_set_close_on_unref(client_channel, TRUE);
        process_client_channel(client, client_channel);
        g_io_channel_unref(client_channel);
        g_print("done\n");
    }

    close(priv->server_fd);

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
