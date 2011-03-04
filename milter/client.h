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

#ifndef __MILTER_CLIENT_H__
#define __MILTER_CLIENT_H__

#include <milter/client/milter-client.h>
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
 * socket, 'inet:PORT', 'inet:PORT&commat;HOST' or
 * 'inet:PORT&commat;[ADDRESS]' are valid formats. For IPv6
 * socket, 'inet6:PORT', 'inet6:PORT&commat;HOST' or
 * 'inet6:PORT&commat;[ADDRESS]' are valid formats. For UNIX
 * domain socket, 'unix:PATH' is a valid format.
 *
 * %MilterClient emits #MilterClient::connection-established
 * signal. %MilterClientContext setup should be done in the
 * signal. In many cases, you just connect callbacks to
 * the passed %MilterClientContext. See %MilterClientContext
 * for available signals.
 *
 * milter_client_run() is running main loop function. You
 * can enter into main loop after you finish to prepare your
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
 *     milter_client_init();
 *
 *     client = milter_client_new();
 *     if (!milter_client_set_connection_spec(client, spec, &error)) {
 *         g_print("%s\n", error->message);
 *         g_error_free(error);
 *         return EXIT_FAILURE;
 *     }
 *     g_signal_connect(client, "connection-established",
 *                      G_CALLBACK(cb_connection_established), NULL);
 *     milter_client_run(client);
 *
 *     milter_client_quit();
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
 * The libmilter's model will have more cost to accept a
 * connection rather than the milter-client library's
 * model. Because the libmilter's model creates a thread but
 * the milter-client library just allocate
 * %MilterClientContext. But in many case, this difference
 * is not bottleneck. :-|</rd>
 */

/**
 * milter_client_init:
 *
 * Call this function before using any other the milter-client
 * library functions.
 */
void                 milter_client_init              (void);

/**
 * milter_client_quit:
 *
 * Call this function after the milter-client library use.
 */
void                 milter_client_quit              (void);

/**
 * milter_client_get_option_group:
 * @client: a %MilterClient.
 *
 * Gets a %GOptionGroup for the @client. The option group
 * has common milter client options.
 *
 * Returns: a new %GOptionGroup for @client.
 */
GOptionGroup        *milter_client_get_option_group  (MilterClient *client);

G_END_DECLS

#endif /* __MILTER_CLIENT_H__ */

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
