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

#include <gcutter.h>

#include <errno.h>

#define shutdown inet_shutdown
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <milter/server.h>
#include <milter/core.h>
#undef shutdown
#include "milter-test-utils.h"

void test_establish_connection (void);
void test_establish_connection_failure (void);
void test_helo (void);
void test_envelope_from (void);
void test_status (void);

static MilterServerContext *context;
static MilterTestClient *client;

static MilterReplyEncoder *encoder;
static MilterDecoder *decoder;

static gboolean command_received;
static gboolean reply_received;
static gboolean ready_received;
static gboolean connection_timeout_received;

static GError *actual_error;
static GError *expected_error;
static gchar *packet;

static void
write_data (const gchar *data, gsize data_size)
{
    reply_received = FALSE;
    milter_test_client_write(client, data, data_size);
}

static void
send_continue (void)
{
    gsize packet_size;

    if (packet)
        g_free(packet);
    milter_reply_encoder_encode_continue(encoder, &packet, &packet_size);
    write_data(packet, packet_size);
}

static void
cb_command_received (MilterDecoder *decoder, gpointer user_data)
{
    command_received = TRUE;
    send_continue();
}

static void
cb_continue_received (MilterServerContext *context, gpointer user_data)
{
    reply_received = TRUE;
}

static void
cb_ready_received (MilterServerContext *context, gpointer user_data)
{
    ready_received = TRUE;
}

static void
cb_error_received (MilterServerContext *context, GError *error,
                   gpointer user_data)
{
    if (actual_error)
        g_error_free(actual_error);
    actual_error = g_error_copy(error);
}

static void
setup_server_context_signals (MilterServerContext *context)
{
#define CONNECT(name)                                                   \
    g_signal_connect(context, #name, G_CALLBACK(cb_ ## name ## _received), NULL)

    CONNECT(continue);
    CONNECT(ready);
    CONNECT(error);

#undef CONNECT
}

static void
setup_command_signals (MilterDecoder *decoder)
{
#define CONNECT(name)                                                   \
    g_signal_connect(decoder, #name, G_CALLBACK(cb_command_received), NULL)

    CONNECT(negotiate);
    CONNECT(define_macro);
    CONNECT(connect);
    CONNECT(helo);
    CONNECT(envelope_from);
    CONNECT(envelope_recipient);
    CONNECT(header);
    CONNECT(end_of_header);
    CONNECT(body);
    CONNECT(end_of_message);
    CONNECT(abort);
    CONNECT(quit);
    CONNECT(unknown);

#undef CONNECT
}

void
setup (void)
{
    context = milter_server_context_new();
    setup_server_context_signals(context);

    client = NULL;

    encoder = MILTER_REPLY_ENCODER(milter_reply_encoder_new());
    decoder = milter_command_decoder_new();
    setup_command_signals(decoder);

    packet = NULL;
    actual_error = NULL;
    expected_error = NULL;

    ready_received = FALSE;
    connection_timeout_received = FALSE;
}

void
teardown (void)
{
    if (packet)
        g_free(packet);

    if (client)
        g_object_unref(client);

    if (encoder)
        g_object_unref(encoder);
    if (decoder)
        g_object_unref(decoder);

    if (context)
        g_object_unref(context);
    if (actual_error)
        g_error_free(actual_error);
    if (expected_error)
        g_error_free(expected_error);
}

static void
setup_test_client (const gchar *spec)
{
    cut_trace(client = milter_test_client_new(spec, decoder));
}

static gboolean
cb_timeout_waiting (gpointer data)
{
    gboolean *waiting = data;

    *waiting = FALSE;
    return FALSE;
}

static void
wait_ready (void)
{
    gboolean timeout_waiting = TRUE;
    guint timeout_waiting_id;

    timeout_waiting_id = g_timeout_add_seconds(1, cb_timeout_waiting,
                                               &timeout_waiting);
    while (timeout_waiting && !ready_received) {
        g_main_context_iteration(NULL, TRUE);
    }
    g_source_remove(timeout_waiting_id);
    cut_assert_true(timeout_waiting, "timeout");
}

static void
wait_error (void)
{
    gboolean timeout_waiting = TRUE;
    guint timeout_waiting_id;

    timeout_waiting_id = g_timeout_add_seconds(2, cb_timeout_waiting,
                                               &timeout_waiting);
    while (timeout_waiting && !actual_error) {
        g_main_context_iteration(NULL, TRUE);
    }
    g_source_remove(timeout_waiting_id);
    cut_assert_true(timeout_waiting, "timeout");
}

void
test_establish_connection (void)
{
    const gchar spec[] = "inet:9999@127.0.0.1";
    GError *error = NULL;

    cut_trace(setup_test_client(spec));

    milter_server_context_set_connection_spec(context, spec, &error);
    gcut_assert_error(error);

    milter_server_context_establish_connection(context, &error);
    gcut_assert_error(error);

    wait_ready();
}

void
test_establish_connection_failure (void)
{
    const gchar spec[] = "inet:9999@127.0.0.1";
    GError *error = NULL;

    milter_server_context_set_connection_spec(context, spec, &error);
    gcut_assert_error(error);

    expected_error = g_error_new(MILTER_SERVER_CONTEXT_ERROR,
                                 MILTER_SERVER_CONTEXT_ERROR_CONNECTION_FAILURE,
                                 "Connection error on %s: Error | Hung up",
                                 spec);
    milter_server_context_establish_connection(context, &error);
    gcut_assert_error(error);

    wait_error();
    gcut_assert_equal_error(expected_error, actual_error);
}

static void
wait_for_receiving_command (void)
{
    command_received = FALSE;

    while (!command_received) {
        g_main_context_iteration(NULL, TRUE);
    }
}

static void
wait_for_receiving_reply (void)
{
    while (!reply_received) {
        g_main_context_iteration(NULL, TRUE);
    }
}

void
test_helo (void)
{
    const gchar fqdn[] = "delian";

    cut_trace(test_establish_connection());

    milter_server_context_helo(context, fqdn);
    wait_for_receiving_command();

    wait_for_receiving_reply();
}

void
test_envelope_from (void)
{
    const gchar from[] = "example@example.com";

    cut_trace(test_helo());

    milter_server_context_envelope_from(context, from);
    wait_for_receiving_command();

    wait_for_receiving_reply();
}

void
test_status (void)
{
    gcut_assert_equal_enum(MILTER_TYPE_SERVER_CONTEXT_STATE,
                           MILTER_SERVER_CONTEXT_STATE_START,
                           milter_server_context_get_state(context));

    milter_server_context_set_state(context, MILTER_SERVER_CONTEXT_STATE_QUIT);
    gcut_assert_equal_enum(MILTER_TYPE_SERVER_CONTEXT_STATE,
                           MILTER_SERVER_CONTEXT_STATE_QUIT,
                           milter_server_context_get_state(context));
}


/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
