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

#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <arpa/inet.h>

#include <milter/server.h>
#include <milter/core.h>

#include <gcutter.h>

#include "milter-test-utils.h"

void test_negotiate (void);
void test_negotiated_newer_version (void);
void test_connect (void);
void test_connect_with_macro (void);
void test_helo (void);
void test_envelope_from (void);
void test_envelope_recipient (void);
void test_data (void);
void test_data_with_protocol_version2 (void);
void test_unknown (void);
void test_header (void);
void test_end_of_header (void);
void test_body (void);
void test_end_of_message (void);
void test_end_of_message_without_chunk (void);
void test_quit (void);
void test_quit_after_connect (void);
void test_abort (void);
void test_abort_and_quit (void);

static guint32 protocol_version;

static MilterServerContext *context;

static MilterWriter *writer;

static MilterDecoder *decoder;
static MilterCommandEncoder *encoder;
static MilterReplyEncoder *reply_encoder;
static GString *packet_string;
static GHashTable *macros;

static GIOChannel *channel;
static MilterEventLoop *loop;

static GError *expected_error, *actual_error;

static MilterMessageResult *expected_result, *actual_result;

static guint n_message_processed;

static void
cb_error (MilterErrorEmittable *emittable, GError *error, gpointer user_data)
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
    if (actual_result)
        g_object_unref(actual_result);
    actual_result = result;
    g_object_ref(actual_result);

    n_message_processed++;
}

static void
setup_signals (MilterServerContext *context)
{
#define CONNECT(name)                                                   \
    g_signal_connect(context, #name, G_CALLBACK(cb_ ## name), NULL)

    CONNECT(message_processed);
    CONNECT(error);
#undef CONNECT
}

void
cut_setup (void)
{
    MilterOption *option;
    GError *error = NULL;

    protocol_version = 6;

    context = milter_server_context_new();
    milter_server_context_set_name(context, "test-server");
    option = milter_option_new(protocol_version,
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
    g_object_unref(option);

    setup_signals(context);

    channel = gcut_string_io_channel_new(NULL);
    g_io_channel_set_encoding(channel, NULL, NULL);

    loop = milter_test_event_loop_new();
    milter_agent_set_event_loop(MILTER_AGENT(context), loop);
    writer = milter_writer_io_channel_new(channel);
    milter_agent_set_writer(MILTER_AGENT(context), writer);
    milter_agent_start(MILTER_AGENT(context), &error);
    gcut_assert_error(error);

    decoder = milter_agent_get_decoder(MILTER_AGENT(context));

    encoder = MILTER_COMMAND_ENCODER(milter_command_encoder_new());
    reply_encoder = MILTER_REPLY_ENCODER(milter_reply_encoder_new());
    packet_string = NULL;
    macros = NULL;

    expected_error = NULL;
    actual_error = NULL;

    n_message_processed = 0;
    actual_result = NULL;
    expected_result = NULL;
}

static void
channel_free (void)
{
    gcut_string_io_channel_clear(channel);
}

void
cut_teardown (void)
{
    if (context)
        g_object_unref(context);

    if (writer)
        g_object_unref(writer);

    if (loop)
        g_object_unref(loop);

    if (encoder)
        g_object_unref(encoder);
    if (reply_encoder)
        g_object_unref(reply_encoder);

    if (channel)
        g_io_channel_unref(channel);

    if (packet_string)
        g_string_free(packet_string, TRUE);
    if (macros)
        g_hash_table_unref(macros);

    if (expected_error)
        g_error_free(expected_error);
    if (actual_error)
        g_error_free(actual_error);

    if (actual_result)
        g_object_unref(actual_result);
    if (expected_result)
        g_object_unref(expected_result);
}

static void
pump_all_events (void)
{
    GError *error;

    if (MILTER_IS_LIBEV_EVENT_LOOP(loop))
        cut_omit("MilterLibevEventLoop doesn't support GCutStringIOChannel.");

    milter_test_pump_all_events(loop);
    error = actual_error;
    actual_error = NULL;
    gcut_assert_error(error);
}

#define milter_test_assert_packet(channel, packet, packet_size)         \
    cut_trace(milter_test_assert_packet_helper(channel, packet, packet_size))

static void
milter_test_assert_packet_helper (GIOChannel *channel,
                                  const gchar *packet, gsize packet_size)
{
    GString *actual;

    actual = gcut_string_io_channel_get_string(channel);
    cut_assert_equal_memory(packet, packet_size, actual->str, actual->len);
}

#define milter_test_assert_state(state)                                 \
    cut_trace(                                                          \
        gcut_assert_equal_enum(MILTER_TYPE_SERVER_CONTEXT_STATE,        \
                               MILTER_SERVER_CONTEXT_STATE_ ## state,   \
                               milter_server_context_get_state(context)))

#define milter_test_assert_status(status)                               \
    cut_trace(                                                          \
        gcut_assert_equal_enum(MILTER_TYPE_STATUS,                      \
                               MILTER_STATUS_ ## status,                \
                               milter_server_context_get_status(context)))

#define milter_test_assert_result(from, recipients, headers,            \
                                  body_size, state, status,             \
                                  added_headers, removed_headers,       \
                                  is_quarantine)                        \
    cut_trace(milter_test_assert_result_helper(from, recipients,        \
                                               headers, body_size,      \
                                               state, status,           \
                                               added_headers,           \
                                               removed_headers,         \
                                               is_quarantine))

static gboolean
result_equal (gconstpointer data1, gconstpointer data2)
{
    MilterMessageResult *result1, *result2;

    result1 = MILTER_MESSAGE_RESULT(data1);
    result2 = MILTER_MESSAGE_RESULT(data2);

    /* FIXME: need more check! */
    return
        g_str_equal(milter_message_result_get_from(result1),
                    milter_message_result_get_from(result2)) &&
        (gcut_list_equal_string(
            milter_message_result_get_recipients(result1),
            milter_message_result_get_recipients(result2))) &&
        (milter_message_result_get_body_size(result1) ==
         milter_message_result_get_body_size(result1));
}

static void
milter_test_assert_result_helper (const gchar *from,
                                  GList *recipients,
                                  MilterHeaders *headers,
                                  guint64 body_size,
                                  MilterState state,
                                  MilterStatus status,
                                  MilterHeaders *added_headers,
                                  MilterHeaders *removed_headers,
                                  gboolean is_quarantine)
{
    if (expected_result)
        g_object_unref(expected_result);

    expected_result = milter_message_result_new();
    milter_message_result_set_from(expected_result, from);
    milter_message_result_set_recipients(expected_result, recipients);
    milter_message_result_set_headers(expected_result, headers);
    milter_message_result_set_body_size(expected_result, body_size);
    milter_message_result_set_state(expected_result, state);
    milter_message_result_set_status(expected_result, status);
    milter_message_result_set_added_headers(expected_result, added_headers);
    milter_message_result_set_removed_headers(expected_result, removed_headers);
    milter_message_result_set_quarantine(expected_result, is_quarantine);

    gcut_assert_equal_object_custom(expected_result, actual_result,
                                    result_equal);
}

#define RECIPIENTS(...)                                 \
    (GList *)(gcut_take_new_list_string(__VA_ARGS__))

#define HEADERS(...)                            \
    milter_test_take_new_headers(__VA_ARGS__)

#define STATE(name)                             \
    MILTER_STATE_ ## name

#define STATUS(name)                            \
    MILTER_STATUS_ ## name

static MilterHeaders *
milter_test_take_new_headers (const gchar *name, ...)
{
    MilterHeaders *headers;
    va_list args;

    headers = milter_headers_new();

    va_start(args, name);
    for (; name; name = va_arg(args, const gchar *)) {
        const gchar *value;

        value = va_arg(args, const gchar *);
        milter_headers_add_header(headers, name, value);
    }
    va_end(args);

    gcut_take_object(G_OBJECT(headers));
    return headers;
}

void
test_negotiate (void)
{
    MilterOption *option;
    const gchar *packet;
    gsize packet_size;

    option = milter_option_new(protocol_version,
                               MILTER_ACTION_ADD_HEADERS,
                               MILTER_STEP_NONE);
    gcut_take_object(G_OBJECT(option));
    cut_assert_true(milter_server_context_negotiate(context, option));
    pump_all_events();
    milter_test_assert_state(NEGOTIATE);
    milter_test_assert_status(NOT_CHANGE);

    milter_command_encoder_encode_negotiate(encoder,
                                            &packet, &packet_size,
                                            option);
    milter_test_assert_packet(channel, packet, packet_size);

    cut_assert_equal_uint(0, n_message_processed);
}

void
test_negotiated_newer_version (void)
{
    MilterOption *option, *reply_option;
    MilterMacrosRequests *macros_requests;
    const gchar *packet;
    gsize packet_size;
    GError *error = NULL;

    option = milter_option_new(2, MILTER_ACTION_ADD_HEADERS, MILTER_STEP_NONE);
    gcut_take_object(G_OBJECT(option));
    cut_assert_true(milter_server_context_negotiate(context, option));
    pump_all_events();
    milter_test_assert_state(NEGOTIATE);
    milter_test_assert_status(NOT_CHANGE);

    reply_option = milter_option_new(6,
                                     MILTER_ACTION_ADD_HEADERS,
                                     MILTER_STEP_NONE);
    gcut_take_object(G_OBJECT(reply_option));
    macros_requests = milter_macros_requests_new();
    gcut_take_object(G_OBJECT(macros_requests));
    milter_reply_encoder_encode_negotiate(reply_encoder,
                                          &packet, &packet_size,
                                          reply_option, macros_requests);

    milter_decoder_decode(decoder, packet, packet_size, &error);
    gcut_assert_error(error);

    expected_error =
        g_error_new(MILTER_SERVER_CONTEXT_ERROR,
                    MILTER_SERVER_CONTEXT_ERROR_NEWER_VERSION_REQUESTED,
                    "unsupported newer version is requested: 2 < 6");
    gcut_assert_equal_error(expected_error, actual_error);

    cut_assert_equal_uint(0, n_message_processed);
}

static void
reply_negotiate (void)
{
    MilterOption *reply_option;
    MilterMacrosRequests *macros_requests;
    const gchar *packet;
    gsize packet_size;
    GError *error = NULL;

    reply_option = milter_option_new(protocol_version,
                                     MILTER_ACTION_ADD_HEADERS,
                                     MILTER_STEP_NONE);
    gcut_take_object(G_OBJECT(reply_option));
    macros_requests = milter_macros_requests_new();
    gcut_take_object(G_OBJECT(macros_requests));
    milter_reply_encoder_encode_negotiate(reply_encoder,
                                          &packet, &packet_size,
                                          reply_option, macros_requests);

    milter_decoder_decode(decoder, packet, packet_size, &error);
    gcut_assert_error(error);
}

static void
reply_continue (void)
{
    const gchar *packet;
    gsize packet_size;
    GError *error = NULL;

    milter_reply_encoder_encode_continue(reply_encoder, &packet, &packet_size);
    milter_decoder_decode(decoder, packet, packet_size, &error);
    gcut_assert_error(error);
}

void
test_connect (void)
{
    struct sockaddr_in address;
    const gchar host_name[] = "mx.local.net";
    const gchar ip_address[] = "192.168.123.123";
    guint16 port;
    const gchar *packet;
    gsize packet_size;

    test_negotiate();
    channel_free();

    reply_negotiate();

    port = g_htons(50443);
    address.sin_family = AF_INET;
    address.sin_port = port;
    inet_pton(AF_INET, ip_address, &(address.sin_addr));

    cut_assert_true(milter_server_context_connect(context, host_name,
                                                  (struct sockaddr *)&address,
                                                  sizeof(address)));
    pump_all_events();
    milter_test_assert_state(CONNECT);
    milter_test_assert_status(NOT_CHANGE);

    milter_command_encoder_encode_connect(encoder,
                                          &packet, &packet_size,
                                          host_name,
                                          (const struct sockaddr *)&address,
                                          sizeof(address));
    milter_test_assert_packet(channel, packet, packet_size);

    cut_assert_equal_uint(0, n_message_processed);
}

void
test_connect_with_macro (void)
{
    struct sockaddr_in address;
    const gchar host_name[] = "mx.local.net";
    const gchar ip_address[] = "192.168.123.123";
    guint16 port;
    const gchar *packet;
    gsize packet_size;

    test_negotiate();
    channel_free();

    reply_negotiate();

    port = g_htons(50443);
    address.sin_family = AF_INET;
    address.sin_port = port;
    inet_pton(AF_INET, ip_address, &(address.sin_addr));

    macros = gcut_hash_table_string_string_new("G", "g",
                                               "N", "n",
                                               "U", "u",
                                               NULL);
    milter_protocol_agent_set_macros_hash_table(MILTER_PROTOCOL_AGENT(context),
                                                MILTER_COMMAND_CONNECT,
                                                macros);
    milter_command_encoder_encode_define_macro(encoder,
                                               &packet, &packet_size,
                                               MILTER_COMMAND_CONNECT,
                                               macros);
    cut_assert_true(milter_server_context_connect(context, host_name,
                                                  (struct sockaddr *)&address,
                                                  sizeof(address)));
    pump_all_events();
    milter_test_assert_state(CONNECT);
    milter_test_assert_status(NOT_CHANGE);

    packet_string = g_string_new_len(packet, packet_size);
    milter_command_encoder_encode_connect(encoder,
                                          &packet, &packet_size,
                                          host_name,
                                          (const struct sockaddr *)&address,
                                          sizeof(address));
    g_string_append_len(packet_string, packet, packet_size);
    milter_test_assert_packet(channel, packet_string->str, packet_string->len);

    cut_assert_equal_uint(0, n_message_processed);
}

void
test_helo (void)
{
    const gchar fqdn[] = "delian";
    const gchar *packet;
    gsize packet_size;

    test_connect_with_macro();
    channel_free();

    reply_continue();

    cut_assert_true(milter_server_context_helo(context, fqdn));
    pump_all_events();
    milter_test_assert_state(HELO);
    milter_test_assert_status(NOT_CHANGE);

    milter_command_encoder_encode_helo(encoder, &packet, &packet_size, fqdn);
    milter_test_assert_packet(channel, packet, packet_size);

    cut_assert_equal_uint(0, n_message_processed);
}

void
test_envelope_from (void)
{
    const gchar from[] = "sender@example.com";
    const gchar *packet;
    gsize packet_size;

    test_helo();
    channel_free();

    reply_continue();

    cut_assert_true(milter_server_context_envelope_from(context, from));
    pump_all_events();
    milter_test_assert_state(ENVELOPE_FROM);
    milter_test_assert_status(NOT_CHANGE);

    milter_command_encoder_encode_envelope_from(encoder,
                                                &packet, &packet_size, from);
    milter_test_assert_packet(channel, packet, packet_size);

    cut_assert_equal_uint(0, n_message_processed);
}

void
test_envelope_recipient (void)
{
    const gchar recipient[] = "receiver@example.com";
    const gchar *packet;
    gsize packet_size;

    test_envelope_from();
    channel_free();

    reply_continue();

    cut_assert_true(milter_server_context_envelope_recipient(context, recipient));
    pump_all_events();
    milter_test_assert_state(ENVELOPE_RECIPIENT);
    milter_test_assert_status(NOT_CHANGE);

    milter_command_encoder_encode_envelope_recipient(encoder,
                                                     &packet, &packet_size,
                                                     recipient);
    milter_test_assert_packet(channel, packet, packet_size);

    cut_assert_equal_uint(0, n_message_processed);
}

void
test_data (void)
{
    const gchar *packet;
    gsize packet_size;

    test_envelope_recipient();
    channel_free();

    reply_continue();

    cut_assert_true(milter_server_context_data(context));
    pump_all_events();
    milter_test_assert_state(DATA);
    milter_test_assert_status(NOT_CHANGE);

    milter_command_encoder_encode_data(encoder, &packet, &packet_size);
    milter_test_assert_packet(channel, packet, packet_size);

    cut_assert_equal_uint(0, n_message_processed);
}

void
test_data_with_protocol_version2 (void)
{
    protocol_version = 2;

    test_envelope_recipient();
    channel_free();

    reply_continue();

    cut_assert_true(milter_server_context_data(context));
    pump_all_events();
    milter_test_assert_state(DATA);
    milter_test_assert_status(NOT_CHANGE);

    milter_test_assert_packet(channel, NULL, 0);

    cut_assert_equal_uint(0, n_message_processed);
}

void
test_unknown (void)
{
    const gchar command[] = "Who am I?";
    const gchar *packet;
    gsize packet_size;

    test_data();
    channel_free();

    reply_continue();

    cut_assert_true(milter_server_context_unknown(context, command));
    pump_all_events();
    milter_test_assert_state(UNKNOWN);
    milter_test_assert_status(NOT_CHANGE);

    milter_command_encoder_encode_unknown(encoder,
                                          &packet, &packet_size,
                                          command);
    milter_test_assert_packet(channel, packet, packet_size);
}

void
test_header (void)
{
    const gchar name[] = "X-HEADER-NAME";
    const gchar value[] = "MilterServerContext test";
    const gchar *packet;
    gsize packet_size;

    test_data();
    channel_free();

    reply_continue();

    cut_assert_true(milter_server_context_header(context, name, value));
    pump_all_events();
    milter_test_assert_state(HEADER);
    milter_test_assert_status(NOT_CHANGE);

    milter_command_encoder_encode_header(encoder,
                                         &packet, &packet_size,
                                         name, value);
    milter_test_assert_packet(channel, packet, packet_size);

    cut_assert_equal_uint(0, n_message_processed);
}

void
test_end_of_header (void)
{
    const gchar *packet;
    gsize packet_size;

    test_header();
    channel_free();

    reply_continue();

    cut_assert_true(milter_server_context_end_of_header(context));
    pump_all_events();
    milter_test_assert_state(END_OF_HEADER);
    milter_test_assert_status(NOT_CHANGE);

    milter_command_encoder_encode_end_of_header(encoder, &packet, &packet_size);
    milter_test_assert_packet(channel, packet, packet_size);

    cut_assert_equal_uint(0, n_message_processed);
}

void
test_body (void)
{
    const gchar chunk[] = "This is a body text.";
    const gchar *packet;
    gsize packet_size;
    gsize packed_size;

    test_end_of_header();
    channel_free();

    reply_continue();

    cut_assert_true(milter_server_context_body(context, chunk, strlen(chunk)));
    pump_all_events();
    milter_test_assert_state(BODY);
    milter_test_assert_status(NOT_CHANGE);

    milter_command_encoder_encode_body(encoder, &packet, &packet_size,
                                       chunk, strlen(chunk), &packed_size);
    milter_test_assert_packet(channel, packet, packet_size);
    cut_assert_equal_uint(strlen(chunk), packed_size);

    cut_assert_equal_uint(0, n_message_processed);
}

void
test_end_of_message (void)
{
    const gchar chunk[] = "This is a end_of_message text.";
    const gchar *packet;
    gsize packet_size;

    test_body();
    channel_free();

    reply_continue();

    cut_assert_true(milter_server_context_end_of_message(context,
                                                         chunk, strlen(chunk)));
    pump_all_events();
    milter_test_assert_state(END_OF_MESSAGE);
    milter_test_assert_status(NOT_CHANGE);

    milter_command_encoder_encode_end_of_message(encoder, &packet, &packet_size,
                                                 chunk, strlen(chunk));
    milter_test_assert_packet(channel, packet, packet_size);

    cut_assert_equal_uint(0, n_message_processed);
}

void
test_end_of_message_without_chunk (void)
{
    const gchar *packet;
    gsize packet_size;

    test_body();
    channel_free();

    reply_continue();

    cut_assert_true(milter_server_context_end_of_message(context, NULL, 0));
    pump_all_events();
    milter_test_assert_state(END_OF_MESSAGE);
    milter_test_assert_status(NOT_CHANGE);

    milter_command_encoder_encode_end_of_message(encoder, &packet, &packet_size,
                                                 NULL, 0);
    milter_test_assert_packet(channel, packet, packet_size);

    cut_assert_equal_uint(0, n_message_processed);
}

void
test_quit (void)
{
    const gchar *packet;
    gsize packet_size;

    test_end_of_message_without_chunk();
    channel_free();

    reply_continue();

    cut_assert_true(milter_server_context_quit(context));
    pump_all_events();
    milter_test_assert_state(QUIT);
    milter_test_assert_status(NOT_CHANGE);

    milter_command_encoder_encode_quit(encoder, &packet, &packet_size);
    milter_test_assert_packet(channel, packet, packet_size);

    cut_assert_equal_uint(1, n_message_processed);
    milter_test_assert_result("sender@example.com",
                              RECIPIENTS("receiver@example.com", NULL),
                              HEADERS("X-HEADER-NAME",
                                      "MilterServerContext test",
                                      NULL),
                              20,
                              STATE(END_OF_MESSAGE_REPLIED),
                              STATUS(NOT_CHANGE),
                              HEADERS(NULL),
                              HEADERS(NULL),
                              FALSE);
}

void
test_quit_after_connect (void)
{
    const gchar *packet;
    gsize packet_size;

    test_connect();
    channel_free();

    reply_continue();

    cut_assert_true(milter_server_context_quit(context));
    pump_all_events();
    milter_test_assert_state(QUIT);
    milter_test_assert_status(NOT_CHANGE);

    milter_command_encoder_encode_quit(encoder, &packet, &packet_size);
    milter_test_assert_packet(channel, packet, packet_size);
}

void
test_abort (void)
{
    const gchar *packet;
    gsize packet_size;

    test_body();
    channel_free();

    cut_assert_true(milter_server_context_abort(context));
    pump_all_events();
    milter_test_assert_state(ABORT);
    milter_test_assert_status(ABORT);

    milter_command_encoder_encode_abort(encoder, &packet, &packet_size);
    milter_test_assert_packet(channel, packet, packet_size);
}

void
test_abort_and_quit (void)
{
    const gchar *packet;
    gsize packet_size;

    test_envelope_recipient();
    channel_free();

    cut_assert_true(milter_server_context_abort(context));
    pump_all_events();
    milter_test_assert_state(ABORT);
    milter_test_assert_status(ABORT);

    cut_assert_true(milter_server_context_quit(context));
    pump_all_events();
    milter_test_assert_state(QUIT);
    milter_test_assert_status(ABORT);

    milter_command_encoder_encode_abort(encoder, &packet, &packet_size);
    packet_string = g_string_new_len(packet, packet_size);

    milter_command_encoder_encode_quit(encoder, &packet, &packet_size);
    g_string_append_len(packet_string, packet, packet_size);

    milter_test_assert_packet(channel, packet_string->str, packet_string->len);
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
