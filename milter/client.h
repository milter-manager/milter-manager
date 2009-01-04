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

#ifndef __MILTER_CLIENT_H__
#define __MILTER_CLIENT_H__

#include <milter/core.h>
#include <milter/client/milter-client-context.h>
#include <milter/client/milter-client-enum-types.h>

G_BEGIN_DECLS

/**
 * SECTION: client
 * @title: Client library
 * @short_description: The client library to implement a milter.
 *
 * The milter-client library provides full milter protocol
 * processing features on client-side. "client-side" means
 * milter-side not MTA-side.
 *
 * %MilterClient and %MilterClientContext is the main
 * classes in the milter-client library. %MilterClient
 * accepts n-connections from MTA and each connection is
 * processed by %MilterClientContext.
 *
 * <rd>
 * == Usage overview
 *
 * You need to set connection spec and connect
 * #MilterClient::connection-established signal before
 * entering mail loop.
 *
 * Connection spec is a entry point of %MilterClient for
 * MTA.  It has 'PROTOCOL:INFORMATION' format. For IPv4
 * socket, 'inet:PORT' or 'inet:PORT&commat;HOST' are valid
 * formats. For IPv6 socket, 'inet6:PORT' or
 * 'inet6:PORT&commat;HOST' are valid formats. For UNIX domain
 * socket, 'unix:PATH' is a valid format.
 *
 * %MilterClient emits #MilterClient::connection-established
 * signal. %MilterClientContext setup should be done in the
 * signal. In many cases, you just connect callbacks to
 * the passed %MilterClientContext. See %MilterClientContext
 * for available signals.
 *
 * milter_client_main() is mail loop function. You can enter
 * into mail loop after you finish to prepare your
 * %MilterClient.
 *
 * Here is an example codes that uses the milter-client
 * library. See also tool/milter-test-client.c. It is an
 * example milter implementation using the milter-client
 * library.
 *
 * |[
 * #include <stdlib.h>
 * #include <milter/client.h>
 *
 * static void
 * cb_connection_established (MilterClient *client,
 *                            MilterClientContext *context,
 *                            gpointer user_data)
 * {
 *    connect_to_your_interesting_signals(client, context, user_data);
 * }
 *
 * int
 * main (int argc, char **argv)
 * {
 *     gboolean success;
 *     const gchar spec[] = "inet:10025@localhost";
 *     MilterClient *client;
 *     GError *error = NULL;
 *
 *     milter_init();
 *
 *     client = milter_client_new();
 *     if (!milter_client_set_connection_spec(client, spec, &error)) {
 *         g_print("%s\n", error->message);
 *         g_error_free(error);
 *         return EXIT_FAILURE;
 *     }
 *     g_signal_connect(client, "connection-established",
 *                      G_CALLBACK(cb_connection_established), NULL);
 *     milter_client_main(client);
 *
 *     milter_quit();
 *
 *     return success ? EXIT_SUCCESS : EXIT_FAILURE;
 * }
 * ]|
 *
 * == Processing model
 *
 * The libmilter provided by Sendmail uses a thread for each
 * connection model. But the milter-client library doesn't
 * use it. The milter-client library uses two threads. One
 * thread is for %MilterClient and other thread is for
 * %MilterClientContext<!-- -->s. %MilterClient just
 * accepting connections from MTA and dispatches it in the
 * first thread. Each connection is associated with
 * %MilterClientContext and they are processed in the second
 * thread. The milter-client library's model is the same
 * model of memcached. (memcached uses libevent as its event
 * loop backend but the milter-client library uses GLib's
 * %GMainLoop for it.)
 *
 * FIXME (should be confirmed): The libmilter's model will
 * have more cost to accepts rather than the milter-client
 * library's model. Because the libmilter's model creates a
 * thread but the milter-client library just allocate
 * %MilterClientContext.
 * </rd>
 */

#define MILTER_CLIENT_ERROR           (milter_client_error_quark())

#define MILTER_TYPE_CLIENT            (milter_client_get_type())
#define MILTER_CLIENT(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), MILTER_TYPE_CLIENT, MilterClient))
#define MILTER_CLIENT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), MILTER_TYPE_CLIENT, MilterClientClass))
#define MILTER_IS_CLIENT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), MILTER_TYPE_CLIENT))
#define MILTER_IS_CLIENT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), MILTER_TYPE_CLIENT))
#define MILTER_CLIENT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), MILTER_TYPE_CLIENT, MilterClientClass))

typedef enum
{
    MILTER_CLIENT_ERROR_RUNNING,
    MILTER_CLIENT_ERROR_UNIX_SOCKET,
    MILTER_CLIENT_ERROR_IO_ERROR
} MilterClientError;

typedef struct _MilterClient         MilterClient;
typedef struct _MilterClientClass    MilterClientClass;

/**
 * MilterClient:
 *
 * %MilterClient is a front object. It accepts connections
 * from MTA and dispatches it.
 **/
struct _MilterClient
{
    GObject object;
};

struct _MilterClientClass
{
    GObjectClass parent_class;

    void   (*connection_established)      (MilterClient *client,
                                           MilterClientContext *context);
    void   (*listen_started)              (MilterClient *client,
                                           struct sockaddr *address,
                                           socklen_t        address_size);
    gchar *(*get_default_connection_spec) (MilterClient *client);
    guint  (*get_unix_socket_mode)        (MilterClient *client);
    gboolean (*is_remove_unix_socket_on_close)
                                          (MilterClient *client);
};

GQuark               milter_client_error_quark       (void);

GType                milter_client_get_type          (void) G_GNUC_CONST;

MilterClient        *milter_client_new               (void);

gchar               *milter_client_get_default_connection_spec
                                                     (MilterClient  *client);
gboolean             milter_client_set_connection_spec
                                                     (MilterClient  *client,
                                                      const gchar   *spec,
                                                      GError       **error);
GIOChannel          *milter_client_get_listen_channel(MilterClient  *client);
void                 milter_client_set_listen_channel(MilterClient  *client,
                                                      GIOChannel    *channel);
void                 milter_client_set_listen_backlog(MilterClient  *client,
                                                      gint           backlog);
gboolean             milter_client_is_remove_unix_socket_on_create
                                                     (MilterClient  *client);
void                 milter_client_set_remove_unix_socket_on_create
                                                     (MilterClient  *client,
                                                      gboolean       remove);
void                 milter_client_set_timeout       (MilterClient  *client,
                                                      guint          timeout);
guint                milter_client_get_unix_socket_mode
                                                     (MilterClient  *client);
guint                milter_client_get_default_unix_socket_mode
                                                     (MilterClient  *client);
void                 milter_client_set_default_unix_socket_mode
                                                     (MilterClient  *client,
                                                      guint          mode);
gboolean             milter_client_is_remove_unix_socket_on_close
                                                     (MilterClient  *client);
gboolean             milter_client_get_default_remove_unix_socket_on_close
                                                     (MilterClient  *client);
void                 milter_client_set_default_remove_unix_socket_on_close
                                                     (MilterClient  *client,
                                                      gboolean       remove);
gboolean             milter_client_main              (MilterClient  *client);
void                 milter_client_shutdown          (MilterClient  *client);


G_END_DECLS

#endif /* __MILTER_CLIENT_H__ */

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
