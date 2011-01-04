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

#include <gcutter.h>
#define shutdown inet_shutdown
#include "milter-test-utils.h"
#undef shutdown

#include <unistd.h>

#define BUFFER_SIZE 4096

#define MILTER_TEST_SERVER_GET_PRIVATE(obj)                     \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj),                         \
                                 MILTER_TEST_TYPE_SERVER,       \
                                 MilterTestServerPrivate))

typedef struct _MilterTestServerPrivate	MilterTestServerPrivate;
struct _MilterTestServerPrivate
{
    MilterDecoder *decoder;
    MilterEventLoop *loop;

    GIOChannel *channel;
    gint fd;
    guint watch_id;
};

G_DEFINE_TYPE(MilterTestServer, milter_test_server, G_TYPE_OBJECT);

static void dispose        (GObject         *object);

static void
milter_test_server_class_init (MilterTestServerClass *klass)
{
    GObjectClass *gobject_class;

    gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->dispose      = dispose;

    g_type_class_add_private(gobject_class, sizeof(MilterTestServerPrivate));
}

static void
milter_test_server_init (MilterTestServer *server)
{
    MilterTestServerPrivate *priv;

    priv = MILTER_TEST_SERVER_GET_PRIVATE(server);

    priv->decoder = NULL;

    priv->fd = -1;
    priv->channel = NULL;
    priv->watch_id = -1;
}

static void
dispose (GObject *object)
{
    MilterTestServerPrivate *priv;

    priv = MILTER_TEST_SERVER_GET_PRIVATE(object);

    if (priv->watch_id > 0) {
        milter_event_loop_remove(priv->loop, priv->watch_id);
    }

    if (priv->channel) {
        g_io_channel_unref(priv->channel);
        priv->channel = NULL;
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

    G_OBJECT_CLASS(milter_test_server_parent_class)->dispose(object);
}

MilterTestServer *
milter_test_server_new (const gchar *spec, MilterDecoder *decoder,
                        MilterEventLoop *loop)
{
    MilterTestServerPrivate *priv;
    gint domain;
    struct sockaddr *address;
    socklen_t address_size;
    MilterTestServer *server;
    gint fd;
    GError *error = NULL;

    milter_connection_parse_spec(spec, &domain, &address, &address_size, &error);
    gcut_assert_error(error);

    fd = socket(domain, SOCK_STREAM, 0);

    if (connect(fd, address, address_size) == -1) {
        close(fd);
        cut_assert_errno();
    }

    server = g_object_new(MILTER_TEST_TYPE_SERVER, NULL);

    priv = MILTER_TEST_SERVER_GET_PRIVATE(server);

    priv->decoder = g_object_ref(decoder);
    priv->loop = g_object_ref(loop);

    priv->fd = fd;
    priv->channel = g_io_channel_unix_new(priv->fd);
    priv->watch_id = -1;

    g_io_channel_set_close_on_unref(priv->channel, TRUE);
    if (!g_io_channel_set_encoding(priv->channel, NULL, &error) ||
        !g_io_channel_set_flags(priv->channel,
                                G_IO_FLAG_NONBLOCK |
                                G_IO_FLAG_IS_READABLE |
                                G_IO_FLAG_IS_WRITEABLE,
                                &error)) {
        g_object_unref(server);
        gcut_assert_error(error);
    }
    priv->watch_id = milter_test_io_add_decode_watch(priv->loop,
                                                     priv->channel,
                                                     priv->decoder);

    return server;
}

void
milter_test_server_write_with_error_check (MilterTestServer *server,
                                           const gchar *data, gsize data_size)
{
    MilterTestServerPrivate *priv;

    priv = MILTER_TEST_SERVER_GET_PRIVATE(server);
    milter_test_write_data_to_io_channel(priv->loop, priv->channel,
                                         data, data_size);
}

/*
vi:nowrap:ai:expandtab:sw=4
*/
