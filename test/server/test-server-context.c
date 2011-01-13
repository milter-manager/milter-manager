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

#include <errno.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <milter/server.h>
#include <milter/core.h>

#include <milter-test-utils.h>

#include <gcutter.h>

void test_establish_connection (void);
void test_establish_connection_failure (void);
void test_negotiate (void);
void test_connect (void);
void test_helo (void);
void test_envelope_from (void);
void test_envelope_recipient (void);
void test_envelope_recipient_again (void);
void test_envelope_recipient_temporary_failure (void);
void test_envelope_recipient_reject (void);
void test_envelope_recipient_discard (void);
void test_envelope_recipient_accept (void);
void test_status (void);
void test_state (void);
void test_last_state (void);
void test_macro (void);
void test_macros_hash_table (void);
void data_has_accepted_recipient (void);
void test_has_accepted_recipient (gconstpointer data);

static MilterEventLoop *loop;

static MilterServerContext *context;
static MilterTestClient *client;

static MilterReplyEncoder *encoder;
static MilterDecoder *decoder;

static MilterOption *option;

static MilterStatus reply_status;
static gboolean command_received;
static gboolean reply_received;
static gboolean ready_received;
static gboolean connection_timeout_received;

static MilterMessageResult *message_result;

static GError *actual_error;
static GError *expected_error;

static void
write_data (const gchar *data, gsize data_size)
{
    reply_received = FALSE;
    milter_test_client_write(client, data, data_size);
}

static void
send_continue (void)
{
    const gchar *packet;
    gsize packet_size;

    milter_reply_encoder_encode_continue(encoder, &packet, &packet_size);
    write_data(packet, packet_size);
}

static void
send_temporary_failure (void)
{
    const gchar *packet;
    gsize packet_size;

    milter_reply_encoder_encode_temporary_failure(encoder, &packet, &packet_size);
    write_data(packet, packet_size);
}

static void
send_reject (void)
{
    const gchar *packet;
    gsize packet_size;

    milter_reply_encoder_encode_reject(encoder, &packet, &packet_size);
    write_data(packet, packet_size);
}

static void
send_discard (void)
{
    const gchar *packet;
    gsize packet_size;

    milter_reply_encoder_encode_discard(encoder, &packet, &packet_size);
    write_data(packet, packet_size);
}

static void
send_accept (void)
{
    const gchar *packet;
    gsize packet_size;

    milter_reply_encoder_encode_accept(encoder, &packet, &packet_size);
    write_data(packet, packet_size);
}

static void
cb_negotiate_received (MilterDecoder *decoder, gpointer user_data)
{
    const gchar *packet;
    gsize packet_size;
    MilterMacrosRequests *macros_requests;

    command_received = TRUE;

    macros_requests = milter_macros_requests_new();
    milter_reply_encoder_encode_negotiate(encoder, &packet, &packet_size,
                                          option, macros_requests);
    g_object_unref(macros_requests);
    write_data(packet, packet_size);
}

static void
cb_command_received (MilterDecoder *decoder, gpointer user_data)
{
    command_received = TRUE;
    switch (reply_status) {
    case MILTER_STATUS_TEMPORARY_FAILURE:
        send_temporary_failure();
        break;
    case MILTER_STATUS_REJECT:
        send_reject();
        break;
    case MILTER_STATUS_DISCARD:
        send_discard();
        break;
    case MILTER_STATUS_ACCEPT:
        send_accept();
        break;
    default:
        send_continue();
        break;
    }
}

static void
cb_reply_received (MilterServerContext *context, gpointer user_data)
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
cb_message_processed (MilterServerContext *context,
                      MilterMessageResult *result,
                      gpointer user_data)
{
    if (message_result)
        g_object_unref(message_result);
    message_result = g_object_ref(result);
}

static void
setup_server_context_signals (MilterServerContext *context)
{
#define CONNECT(name)                                                   \
    g_signal_connect(context, #name, G_CALLBACK(cb_reply_received), NULL)

    CONNECT(negotiate_reply);
    CONNECT(continue);
    CONNECT(temporary_failure);
    CONNECT(reject);
    CONNECT(discard);
    CONNECT(accept);

#undef CONNECT

#define CONNECT(name)                                                   \
    g_signal_connect(context, #name, G_CALLBACK(cb_ ## name ## _received), NULL)

    CONNECT(ready);
    CONNECT(error);

#undef CONNECT

    g_signal_connect(context, "message-processed",
                     G_CALLBACK(cb_message_processed), NULL);
}

static void
setup_command_signals (MilterDecoder *decoder)
{
    g_signal_connect(decoder, "negotiate", G_CALLBACK(cb_negotiate_received),
                     NULL);

#define CONNECT(name)                                                   \
    g_signal_connect(decoder, #name, G_CALLBACK(cb_command_received), NULL)

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
cut_setup (void)
{
    loop = milter_test_event_loop_new();

    context = milter_server_context_new();
    milter_server_context_set_name(context, "test-server-context");
    milter_agent_set_event_loop(MILTER_AGENT(context), loop);
    setup_server_context_signals(context);

    option = milter_option_new(6,
                               MILTER_ACTION_ADD_HEADERS |
                               MILTER_ACTION_CHANGE_BODY |
                               MILTER_ACTION_ADD_ENVELOPE_RECIPIENT |
                               MILTER_ACTION_DELETE_ENVELOPE_RECIPIENT |
                               MILTER_ACTION_CHANGE_HEADERS |
                               MILTER_ACTION_QUARANTINE |
                               MILTER_ACTION_CHANGE_ENVELOPE_FROM |
                               MILTER_ACTION_ADD_ENVELOPE_RECIPIENT_WITH_PARAMETERS |
                               MILTER_ACTION_SET_SYMBOL_LIST,
                               MILTER_STEP_SKIP);
    milter_server_context_set_option(context, option);

    client = NULL;

    encoder = MILTER_REPLY_ENCODER(milter_reply_encoder_new());
    decoder = milter_command_decoder_new();
    setup_command_signals(decoder);

    actual_error = NULL;
    expected_error = NULL;

    reply_status = MILTER_STATUS_CONTINUE;
    ready_received = FALSE;
    connection_timeout_received = FALSE;

    message_result = NULL;
}

void
cut_teardown (void)
{
    if (option)
        g_object_unref(option);

    if (client)
        g_object_unref(client);

    if (encoder)
        g_object_unref(encoder);
    if (decoder)
        g_object_unref(decoder);

    if (message_result)
        g_object_unref(message_result);

    if (context)
        g_object_unref(context);
    if (actual_error)
        g_error_free(actual_error);
    if (expected_error)
        g_error_free(expected_error);

    if (loop)
        g_object_unref(loop);
}

static void
setup_test_client (const gchar *spec)
{
    cut_trace(client = milter_test_client_new(spec, decoder, loop));
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

    timeout_waiting_id = milter_event_loop_add_timeout(loop, 1,
                                                       cb_timeout_waiting,
                                                       &timeout_waiting);
    while (timeout_waiting && !ready_received) {
        milter_event_loop_iterate(loop, TRUE);
    }
    milter_event_loop_remove(loop, timeout_waiting_id);
    cut_assert_true(timeout_waiting, cut_message("timeout"));
}

static void
wait_error (void)
{
    gboolean timeout_waiting = TRUE;
    guint timeout_waiting_id;

    timeout_waiting_id = milter_event_loop_add_timeout(loop, 2,
                                                       cb_timeout_waiting,
                                                       &timeout_waiting);
    while (timeout_waiting && !actual_error) {
        milter_event_loop_iterate(loop, TRUE);
    }
    milter_event_loop_remove(loop, timeout_waiting_id);
    cut_assert_true(timeout_waiting, cut_message("timeout"));
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

    gcut_assert_equal_enum(MILTER_TYPE_STATUS,
                           MILTER_STATUS_NOT_CHANGE,
                           milter_server_context_get_status(context));
}

void
test_establish_connection_failure (void)
{
    const gchar spec[] = "inet:9999@127.0.0.1";
    GError *error = NULL;

    milter_server_context_set_connection_spec(context, spec, &error);
    gcut_assert_error(error);

    milter_server_context_establish_connection(context, &error);
    gcut_assert_error(error);

    wait_error();
    expected_error = g_error_new(MILTER_SERVER_CONTEXT_ERROR,
                                 MILTER_SERVER_CONTEXT_ERROR_CONNECTION_FAILURE,
                                 "Failed to connect to %s: %s",
                                 spec, g_strerror(ECONNREFUSED));
    gcut_assert_equal_error(expected_error, actual_error);

    gcut_assert_equal_enum(MILTER_TYPE_STATUS,
                           MILTER_STATUS_NOT_CHANGE,
                           milter_server_context_get_status(context));
}

static void
wait_for_receiving_command (void)
{
    command_received = FALSE;

    while (!command_received) {
        milter_event_loop_iterate(loop, TRUE);
    }
}

static void
wait_for_receiving_reply (void)
{
    while (!reply_received) {
        milter_event_loop_iterate(loop, TRUE);
    }
}

void
test_negotiate (void)
{
    cut_trace(test_establish_connection());

    milter_server_context_negotiate(context, option);
    cut_assert_false(milter_server_context_is_negotiated(context));

    wait_for_receiving_command();
    cut_assert_false(milter_server_context_is_negotiated(context));

    wait_for_receiving_reply();
    cut_assert_true(milter_server_context_is_negotiated(context));

    gcut_assert_equal_enum(MILTER_TYPE_STATUS,
                           MILTER_STATUS_NOT_CHANGE,
                           milter_server_context_get_status(context));
}

void
test_connect (void)
{
    struct sockaddr_in address;
    const gchar host_name[] = "mx.example.com";
    const gchar ip_address[] = "192.168.123.123";
    guint16 port;

    cut_trace(test_negotiate());

    port = g_htons(50443);
    address.sin_family = AF_INET;
    address.sin_port = port;
    inet_pton(AF_INET, ip_address, &(address.sin_addr));

    milter_server_context_connect(context, host_name,
                                  (struct sockaddr *)&address,
                                  sizeof(address));
    cut_assert_true(milter_server_context_is_processing(context));

    wait_for_receiving_command();
    cut_assert_true(milter_server_context_is_processing(context));

    wait_for_receiving_reply();
    cut_assert_false(milter_server_context_is_processing(context));

    gcut_assert_equal_enum(MILTER_TYPE_STATUS,
                           MILTER_STATUS_NOT_CHANGE,
                           milter_server_context_get_status(context));
}

void
test_helo (void)
{
    const gchar fqdn[] = "delian";

    cut_trace(test_connect());

    milter_server_context_helo(context, fqdn);
    cut_assert_true(milter_server_context_is_processing(context));

    wait_for_receiving_command();
    cut_assert_true(milter_server_context_is_processing(context));

    wait_for_receiving_reply();
    cut_assert_false(milter_server_context_is_processing(context));

    gcut_assert_equal_enum(MILTER_TYPE_STATUS,
                           MILTER_STATUS_NOT_CHANGE,
                           milter_server_context_get_status(context));
}

void
test_envelope_from (void)
{
    const gchar from[] = "example@example.com";
    MilterMessageResult *result;

    cut_trace(test_helo());

    milter_server_context_envelope_from(context, from);
    wait_for_receiving_command();

    wait_for_receiving_reply();

    gcut_assert_equal_enum(MILTER_TYPE_STATUS,
                           MILTER_STATUS_NOT_CHANGE,
                           milter_server_context_get_status(context));

    result = milter_server_context_get_message_result(context);
    cut_assert_equal_string(from,
                            milter_message_result_get_from(result));
}

void
test_envelope_recipient (void)
{
    const gchar recipient[] = "receiver1@example.com";
    MilterMessageResult *result;

    cut_trace(test_envelope_from());

    milter_server_context_envelope_recipient(context, recipient);
    wait_for_receiving_command();

    wait_for_receiving_reply();

    gcut_assert_equal_enum(MILTER_TYPE_STATUS,
                           MILTER_STATUS_NOT_CHANGE,
                           milter_server_context_get_status(context));

    result = milter_server_context_get_message_result(context);
    gcut_assert_equal_list_string(gcut_take_new_list_string(recipient,
                                                            NULL),
                                  milter_message_result_get_recipients(result));
    gcut_assert_equal_enum(MILTER_TYPE_STATUS,
                           MILTER_STATUS_DEFAULT,
                           milter_message_result_get_status(result));
}

void
test_envelope_recipient_again (void)
{
    const gchar recipient[] = "receiver2@example.com";
    MilterMessageResult *result;

    cut_trace(test_envelope_recipient());

    milter_server_context_envelope_recipient(context, recipient);
    wait_for_receiving_command();

    wait_for_receiving_reply();

    gcut_assert_equal_enum(MILTER_TYPE_STATUS,
                           MILTER_STATUS_NOT_CHANGE,
                           milter_server_context_get_status(context));

    result = milter_server_context_get_message_result(context);
    gcut_assert_equal_list_string(
        gcut_take_new_list_string("receiver1@example.com",
                                  recipient,
                                  NULL),
        milter_message_result_get_recipients(result));
    gcut_assert_equal_enum(MILTER_TYPE_STATUS,
                           MILTER_STATUS_DEFAULT,
                           milter_message_result_get_status(result));
}

void
test_envelope_recipient_temporary_failure (void)
{
    const gchar recipient[] = "receiver-temporary-failure@example.com";
    MilterMessageResult *result;

    cut_trace(test_envelope_recipient());

    milter_server_context_envelope_recipient(context, recipient);
    reply_status = MILTER_STATUS_TEMPORARY_FAILURE;
    wait_for_receiving_command();

    wait_for_receiving_reply();

    gcut_assert_equal_enum(MILTER_TYPE_STATUS,
                           MILTER_STATUS_NOT_CHANGE,
                           milter_server_context_get_status(context));

    result = milter_server_context_get_message_result(context);
    gcut_assert_equal_list_string(
        gcut_take_new_list_string("receiver1@example.com",
                                  NULL),
        milter_message_result_get_recipients(result));
    gcut_assert_equal_list_string(
        gcut_take_new_list_string(recipient,
                                  NULL),
        milter_message_result_get_temporary_failed_recipients(result));
    gcut_assert_equal_enum(MILTER_TYPE_STATUS,
                           MILTER_STATUS_DEFAULT,
                           milter_message_result_get_status(result));
}

void
test_envelope_recipient_reject (void)
{
    const gchar recipient[] = "receiver-reject@example.com";
    MilterMessageResult *result;

    cut_trace(test_envelope_recipient());

    milter_server_context_envelope_recipient(context, recipient);
    reply_status = MILTER_STATUS_REJECT;
    wait_for_receiving_command();

    wait_for_receiving_reply();

    gcut_assert_equal_enum(MILTER_TYPE_STATUS,
                           MILTER_STATUS_NOT_CHANGE,
                           milter_server_context_get_status(context));

    result = milter_server_context_get_message_result(context);
    gcut_assert_equal_list_string(
        gcut_take_new_list_string("receiver1@example.com",
                                  NULL),
        milter_message_result_get_recipients(result));
    gcut_assert_equal_list_string(
        gcut_take_new_list_string(recipient,
                                  NULL),
        milter_message_result_get_rejected_recipients(result));
    gcut_assert_equal_enum(MILTER_TYPE_STATUS,
                           MILTER_STATUS_DEFAULT,
                           milter_message_result_get_status(result));
}

void
test_envelope_recipient_discard (void)
{
    const gchar recipient[] = "receiver-discard@example.com";

    cut_trace(test_envelope_recipient());

    milter_server_context_envelope_recipient(context, recipient);
    reply_status = MILTER_STATUS_DISCARD;
    wait_for_receiving_command();

    wait_for_receiving_reply();

    gcut_assert_equal_enum(MILTER_TYPE_STATUS,
                           MILTER_STATUS_DISCARD,
                           milter_server_context_get_status(context));

    gcut_assert_equal_object(NULL,
                             milter_server_context_get_message_result(context));

    gcut_assert_equal_list_string(
        gcut_take_new_list_string("receiver1@example.com",
                                  recipient,
                                  NULL),
        milter_message_result_get_recipients(message_result));
    gcut_assert_equal_enum(MILTER_TYPE_STATUS,
                           MILTER_STATUS_DISCARD,
                           milter_message_result_get_status(message_result));
}

void
test_envelope_recipient_accept (void)
{
    const gchar recipient[] = "receiver-accept@example.com";

    cut_trace(test_envelope_recipient());

    milter_server_context_envelope_recipient(context, recipient);
    reply_status = MILTER_STATUS_ACCEPT;
    wait_for_receiving_command();

    wait_for_receiving_reply();

    gcut_assert_equal_enum(MILTER_TYPE_STATUS,
                           MILTER_STATUS_ACCEPT,
                           milter_server_context_get_status(context));

    gcut_assert_equal_object(NULL,
                             milter_server_context_get_message_result(context));

    gcut_assert_equal_list_string(
        gcut_take_new_list_string("receiver1@example.com",
                                  recipient,
                                  NULL),
        milter_message_result_get_recipients(message_result));
    gcut_assert_equal_enum(MILTER_TYPE_STATUS,
                           MILTER_STATUS_ACCEPT,
                           milter_message_result_get_status(message_result));
}

void
test_status (void)
{
    gcut_assert_equal_enum(MILTER_TYPE_STATUS,
                           MILTER_STATUS_NOT_CHANGE,
                           milter_server_context_get_status(context));

    milter_server_context_set_status(context, MILTER_STATUS_ACCEPT);
    gcut_assert_equal_enum(MILTER_TYPE_STATUS,
                           MILTER_STATUS_ACCEPT,
                           milter_server_context_get_status(context));
}

void
test_state (void)
{
    gcut_assert_equal_enum(MILTER_TYPE_SERVER_CONTEXT_STATE,
                           MILTER_SERVER_CONTEXT_STATE_START,
                           milter_server_context_get_state(context));

    milter_server_context_set_state(context, MILTER_SERVER_CONTEXT_STATE_QUIT);
    gcut_assert_equal_enum(MILTER_TYPE_SERVER_CONTEXT_STATE,
                           MILTER_SERVER_CONTEXT_STATE_QUIT,
                           milter_server_context_get_state(context));
}

void
test_last_state (void)
{
    gcut_assert_equal_enum(MILTER_TYPE_SERVER_CONTEXT_STATE,
                           MILTER_SERVER_CONTEXT_STATE_START,
                           milter_server_context_get_last_state(context));

    milter_server_context_set_state(context, MILTER_SERVER_CONTEXT_STATE_DATA);
    gcut_assert_equal_enum(MILTER_TYPE_SERVER_CONTEXT_STATE,
                           MILTER_SERVER_CONTEXT_STATE_DATA,
                           milter_server_context_get_last_state(context));

    milter_server_context_set_state(context, MILTER_SERVER_CONTEXT_STATE_QUIT);
    gcut_assert_equal_enum(MILTER_TYPE_SERVER_CONTEXT_STATE,
                           MILTER_SERVER_CONTEXT_STATE_DATA,
                           milter_server_context_get_last_state(context));
}

void
test_macro (void)
{
    MilterProtocolAgent *agent;

    agent = MILTER_PROTOCOL_AGENT(context);
    milter_protocol_agent_set_macro_context(agent, MILTER_COMMAND_CONNECT);

    gcut_assert_equal_hash_table_string_string(
        gcut_hash_table_string_string_new(NULL, NULL,
                                          NULL),
        milter_protocol_agent_get_available_macros(agent));

    milter_protocol_agent_set_macro(agent, MILTER_COMMAND_CONNECT,
                                    "if_name", "localhost");
    gcut_assert_equal_hash_table_string_string(
        gcut_hash_table_string_string_new("if_name", "localhost",
                                          NULL),
        milter_protocol_agent_get_available_macros(agent));

    milter_protocol_agent_set_macro(agent, MILTER_COMMAND_CONNECT,
                                    "if_name", NULL);
    gcut_assert_equal_hash_table_string_string(
        gcut_hash_table_string_string_new(NULL, NULL,
                                          NULL),
        milter_protocol_agent_get_available_macros(agent));
}

void
test_macros_hash_table (void)
{
    MilterProtocolAgent *agent;

    agent = MILTER_PROTOCOL_AGENT(context);
    milter_protocol_agent_set_macro_context(agent, MILTER_COMMAND_CONNECT);

    gcut_assert_equal_hash_table_string_string(
        gcut_hash_table_string_string_new(NULL, NULL,
                                          NULL),
        milter_protocol_agent_get_available_macros(agent));

    milter_protocol_agent_set_macros_hash_table(
        agent, MILTER_COMMAND_CONNECT,
        gcut_hash_table_string_string_new("if_name", "localhost",
                                          "daemon_name", NULL,
                                          "if_addr", "IPv6:::1",
                                          NULL));
    gcut_assert_equal_hash_table_string_string(
        gcut_hash_table_string_string_new("if_name", "localhost",
                                          "if_addr", "IPv6:::1",
                                          NULL),
        milter_protocol_agent_get_available_macros(agent));
}

void
data_has_accepted_recipient (void)
{
#define ADD(label, expected, recipients)                         \
    gcut_add_datum(label " - [" #expected "] <" recipients ">",  \
                   "expected", G_TYPE_BOOLEAN, expected,         \
                   "recipients", G_TYPE_STRING, recipients,      \
                   NULL)

    ADD("no message result", TRUE, "");
    ADD("all accepted", TRUE,
        "alice@example.com,"
        "bob@example.com");
    ADD("one accepted - no unaccepted", TRUE,
        "alice@example.com");
    ADD("one accepted - one rejected", TRUE,
        "alice@example.com,"
        "bob@example.com:reject");
    ADD("one accepted - one temporary failed", TRUE,
        "alice@example.com,"
        "bob@example.com:temporary-failure");
    ADD("no accepted - one rejected", FALSE,
        "alice@example.com:reject");
    ADD("no accepted - one temporary failed", FALSE,
        "alice@example.com:temporary-failure");
    ADD("all unaccepted", FALSE,
        "alice@example.com:temporary-failure,"
        "bob@example.com:reject");

#undef ADD
}

void
test_has_accepted_recipient (gconstpointer data)
{
    const gchar *recipients_text;
    const gchar **recipients;

    cut_trace(test_envelope_from());

    recipients_text = gcut_data_get_string(data, "recipients");
    recipients = cut_take_string_array(g_strsplit(recipients_text, ",", 0));
    for (; *recipients; recipients++) {
        gchar **recipient_and_action;
        const gchar *recipient, *action = NULL;

        recipient_and_action = g_strsplit(*recipients, ":", 2);
        recipient = cut_take_strdup(recipient_and_action[0]);
        if (recipient_and_action[1])
            action = cut_take_strdup(recipient_and_action[1]);
        g_strfreev(recipient_and_action);

        if (cut_equal_string(action, "reject")) {
            reply_status = MILTER_STATUS_REJECT;
        } else if (cut_equal_string(action, "temporary-failure")) {
            reply_status = MILTER_STATUS_TEMPORARY_FAILURE;
        } else {
            reply_status = MILTER_STATUS_CONTINUE;
        }
        milter_server_context_envelope_recipient(context, recipient);
        cut_assert_true(milter_server_context_is_processing(context));

        wait_for_receiving_command();
        cut_assert_true(milter_server_context_is_processing(context));

        wait_for_receiving_reply();
        cut_assert_false(milter_server_context_is_processing(context));
    }

    if (gcut_data_get_boolean(data, "expected")) {
        cut_assert_true(milter_server_context_has_accepted_recipient(context));
    } else {
        cut_assert_false(milter_server_context_has_accepted_recipient(context));
    }
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
