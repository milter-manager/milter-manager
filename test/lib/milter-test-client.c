/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Copyright (C) 2008-2011  Kouhei Sutou <kou@clear-code.com>
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

#include <unistd.h>

#include "milter-test-utils.h"

#include <gcutter.h>

#define BUFFER_SIZE 4096

#define MILTER_TEST_CLIENT_GET_PRIVATE(obj)                     \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj),                         \
                                 MILTER_TEST_TYPE_CLIENT,       \
                                 MilterTestClientPrivate))

typedef struct _MilterTestClientPrivate	MilterTestClientPrivate;
struct _MilterTestClientPrivate
{
    MilterDecoder *decoder;
    MilterEventLoop *loop;

    GIOChannel *channel;
    GIOChannel *server_channel;

    gint fd;
    gint server_fd;

    guint watch_id;
    guint server_watch_id;
};

G_DEFINE_TYPE(MilterTestClient, milter_test_client, G_TYPE_OBJECT);

static void dispose        (GObject         *object);

static void
milter_test_client_class_init (MilterTestClientClass *klass)
{
    GObjectClass *gobject_class;

    gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->dispose      = dispose;

    g_type_class_add_private(gobject_class, sizeof(MilterTestClientPrivate));
}

static void
milter_test_client_init (MilterTestClient *client)
{
    MilterTestClientPrivate *priv;

    priv = MILTER_TEST_CLIENT_GET_PRIVATE(client);

    priv->decoder = NULL;

    priv->server_fd = -1;
    priv->server_channel = NULL;
    priv->server_watch_id = -1;

    priv->fd = -1;
    priv->channel = NULL;
    priv->watch_id = -1;
}

static void
dispose (GObject *object)
{
    MilterTestClientPrivate *priv;

    priv = MILTER_TEST_CLIENT_GET_PRIVATE(object);

    if (priv->server_watch_id > 0) {
        milter_event_loop_remove(priv->loop, priv->server_watch_id);
        priv->server_watch_id = 0;
    }

    if (priv->watch_id > 0) {
        milter_event_loop_remove(priv->loop, priv->watch_id);
        priv->watch_id = 0;
    }

    if (priv->server_channel) {
        g_io_channel_unref(priv->server_channel);
        priv->server_channel = NULL;
    }

    if (priv->channel) {
        g_io_channel_unref(priv->channel);
        priv->channel = NULL;
    }

    if (priv->server_fd > 0) {
        close(priv->server_fd);
        priv->server_fd = 0;
    }

    if (priv->fd > 0) {
        close(priv->fd);
        priv->fd = 0;
    }

    if (priv->decoder) {
        g_object_unref(priv->decoder);
        priv->decoder = NULL;
    }

    if (priv->loop) {
        g_object_unref(priv->loop);
        priv->loop = NULL;
    }

    G_OBJECT_CLASS(milter_test_client_parent_class)->dispose(object);
}

static gboolean
accept_server (MilterTestClient *client)
{
    MilterTestClientPrivate *priv;
    GError *error = NULL;

    priv = MILTER_TEST_CLIENT_GET_PRIVATE(client);

    priv->server_fd = accept(priv->fd, NULL, NULL);

    priv->server_channel = g_io_channel_unix_new(priv->server_fd);
    g_io_channel_set_encoding(priv->server_channel, NULL, &error);
    gcut_assert_error(error);

    g_io_channel_set_flags(priv->server_channel,
                           G_IO_FLAG_NONBLOCK |
                           G_IO_FLAG_IS_READABLE |
                           G_IO_FLAG_IS_WRITEABLE,
                           &error);
    gcut_assert_error(error);

    g_io_channel_set_close_on_unref(priv->server_channel, TRUE);

    priv->server_watch_id = milter_test_io_add_decode_watch(priv->loop,
                                                            priv->server_channel,
                                                            priv->decoder);

    return TRUE;
}

static gboolean
watch_func (GIOChannel *channel, GIOCondition condition, gpointer data)
{
    MilterTestClient *client = data;
    gboolean keep_callback = TRUE;

    if (condition & G_IO_IN ||
        condition & G_IO_PRI) {
        keep_callback = accept_server(client);
    }

    if (condition & G_IO_ERR ||
        condition & G_IO_HUP ||
        condition & G_IO_NVAL) {
        gchar *message;

        message = milter_utils_inspect_io_condition_error(condition);
        cut_notify("%s", message);
        g_free(message);
        return FALSE;
    }

    return keep_callback;
}


MilterTestClient *
milter_test_client_new (const gchar *spec, MilterDecoder *decoder,
                        MilterEventLoop *loop)
{
    MilterTestClientPrivate *priv;
    gint domain;
    struct sockaddr *address;
    socklen_t address_size;
    MilterTestClient *client;
    gint fd;
    gboolean reuse_address = TRUE;
    GError *error = NULL;

    milter_connection_parse_spec(spec, &domain, &address, &address_size, &error);
    gcut_assert_error(error);

    fd = socket(domain, SOCK_STREAM, 0);
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,
               &reuse_address, sizeof(reuse_address));

    if (bind(fd, address, address_size) == -1 ||
        listen(fd, 5) == -1) {
        close(fd);
        cut_assert_errno();
    }

    client = g_object_new(MILTER_TEST_TYPE_CLIENT, NULL);

    priv = MILTER_TEST_CLIENT_GET_PRIVATE(client);

    priv->decoder = g_object_ref(decoder);
    priv->loop = g_object_ref(loop);

    priv->fd = fd;
    priv->channel = g_io_channel_unix_new(priv->fd);
    priv->watch_id = -1;

    g_io_channel_set_close_on_unref(priv->channel, TRUE);
    if (!g_io_channel_set_encoding(priv->channel, NULL, &error) ||
        !g_io_channel_set_flags(priv->channel,
                                G_IO_FLAG_NONBLOCK | G_IO_FLAG_IS_READABLE,
                                &error)) {
        g_object_unref(client);
        gcut_assert_error(error);
    }
    priv->watch_id = milter_event_loop_watch_io(priv->loop,
                                                priv->channel,
                                                G_IO_IN | G_IO_PRI |
                                                G_IO_ERR | G_IO_HUP | G_IO_NVAL,
                                                watch_func, client);

    return client;
}

void
milter_test_client_write_with_error_check (MilterTestClient *client,
                                           const gchar *data, gsize data_size)
{
    MilterTestClientPrivate *priv;

    priv = MILTER_TEST_CLIENT_GET_PRIVATE(client);
    milter_test_write_data_to_io_channel(priv->loop, priv->server_channel,
                                         data, data_size);
}

/*
vi:nowrap:ai:expandtab:sw=4
*/
