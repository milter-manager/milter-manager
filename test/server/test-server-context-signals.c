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
#include <errno.h>

#include <milter/server.h>
#include <milter/core.h>

#include <gcutter.h>

#include "milter-test-utils.h"

void test_negotiate_reply (void);
void test_continue (void);
void test_reply_code_temporary_failure (void);
void test_reply_code_temporary_failure_on_invalid_state (void);
void test_reply_code_reject (void);
void test_reply_code_reject_on_invalid_state (void);
void test_temporary_failure (void);
void test_reject (void);
void test_accept (void);
void test_discard (void);
void test_add_header (void);
void test_add_header_on_invalid_state (void);
void test_insert_header (void);
void test_insert_header_on_invalid_state (void);
void test_change_header (void);
void test_change_header_on_invalid_state (void);
void test_delete_header (void);
void test_delete_header_on_invalid_state (void);
void test_change_from (void);
void test_change_from_on_invalid_state (void);
void test_add_recipient (void);
void test_add_recipient_on_invalid_state (void);
void test_delete_recipient (void);
void test_delete_recipient_on_invalid_state (void);
void test_replace_body (void);
void test_replace_body_on_invalid_state (void);
void test_progress (void);
void test_progress_on_invalid_state (void);
void test_quarantine (void);
void test_quarantine_on_invalid_state (void);
void test_connection_failure (void);
void test_shutdown (void);
void test_skip (void);
void test_skip_on_invalid_state (void);
void test_reading_timeout (void);
void test_writing_timeout (void);
void test_invalid_state_error (void);
void test_busy_error (void);
void test_no_busy_on_abort (void);
void test_decode_error (void);
void test_write_error (void);

static MilterEventLoop *loop;
static MilterServerContext *context;

static MilterReader *reader;
static MilterWriter *writer;

static MilterReplyEncoder *encoder;

static GError *actual_error;
static GError *expected_error;

static GIOChannel *read_channel;
static GIOChannel *write_channel;
static MilterOption *option;
static MilterMacrosRequests *requests;

static gboolean data_written;
static gint n_negotiate_replys;
static gint n_continues;
static gint n_reply_codes;
static gint n_temporary_failures;
static gint n_rejects;
static gint n_accepts;
static gint n_discards;
static gint n_add_headers;
static gint n_insert_headers;
static gint n_change_headers;
static gint n_delete_headers;
static gint n_change_froms;
static gint n_add_recipients;
static gint n_delete_recipients;
static gint n_replace_bodys;
static gint n_progresss;
static gint n_quarantines;
static gint n_connection_failures;
static gint n_shutdowns;
static gint n_skips;
static gint n_errors;

static guint actual_reply_code;
static gchar *actual_reply_extended_code;
static gchar *actual_reply_message;
static gchar *actual_header_name;
static gchar *actual_header_value;
static guint32 actual_header_index;
static gchar *actual_envelope_from;
static gchar *actual_envelope_recipient;
static gchar *actual_envelope_parameters;
static gchar *actual_recipient;
static gchar *actual_recipient_parameters;
static gchar *actual_quarantine_reason;
static MilterOption *actual_option;
static MilterMacrosRequests *actual_requests;

static gboolean timed_out_before;
static gboolean timed_out_after;
static guint timeout_id;
static GString *buffer;

static MilterStatus
cb_reply_code (MilterServerContext *context,
               guint code,
               const gchar *extended_code,
               const gchar *message,
               gpointer user_data)
{
    n_reply_codes++;

    actual_reply_code = code;
    actual_reply_extended_code = g_strdup(extended_code);
    actual_reply_message = g_strdup(message);

    return MILTER_STATUS_CONTINUE;
}

static MilterStatus
cb_negotiate_reply (MilterServerContext *context,
                    MilterOption *option,
                    MilterMacrosRequests *requests,
                    gpointer user_data)
{
    n_negotiate_replys++;

    if (option)
        actual_option = g_object_ref(option);

    if (requests)
        actual_requests = g_object_ref(requests);

    return MILTER_STATUS_CONTINUE;
}

static MilterStatus
cb_continue (MilterServerContext *context, gpointer user_data)
{
    n_continues++;

    return MILTER_STATUS_CONTINUE;
}

static MilterStatus
cb_temporary_failure (MilterServerContext *context, gpointer user_data)
{
    n_temporary_failures++;

    return MILTER_STATUS_CONTINUE;
}

static MilterStatus
cb_reject (MilterServerContext *context, gpointer user_data)
{
    n_rejects++;

    return MILTER_STATUS_CONTINUE;
}

static MilterStatus
cb_accept (MilterServerContext *context, gpointer user_data)
{
    n_accepts++;

    return MILTER_STATUS_CONTINUE;
}

static MilterStatus
cb_discard (MilterServerContext *context, gpointer user_data)
{
    n_discards++;

    return MILTER_STATUS_CONTINUE;
}

static MilterStatus
cb_add_header (MilterServerContext *context,
               const gchar *name, const gchar *value,
               gpointer user_data)
{
    n_add_headers++;

    actual_header_name = g_strdup(name);
    actual_header_value = g_strdup(value);

    return MILTER_STATUS_CONTINUE;
}

static MilterStatus
cb_insert_header (MilterServerContext *context,
                  gint index,
                  const gchar *name,
                  const gchar *value,
                  gpointer user_data)
{
    n_insert_headers++;

    actual_header_index = index;
    actual_header_name = g_strdup(name);
    actual_header_value = g_strdup(value);

    return MILTER_STATUS_CONTINUE;
}

static MilterStatus
cb_change_header (MilterServerContext *context,
                  const gchar *name,
                  gint index,
                  const gchar *value,
                  gpointer user_data)
{
    n_change_headers++;

    actual_header_name = g_strdup(name);
    actual_header_index = index;
    actual_header_value = g_strdup(value);

    return MILTER_STATUS_CONTINUE;
}

static MilterStatus
cb_delete_header (MilterServerContext *context,
                  const gchar *name,
                  gint index,
                  gpointer user_data)
{
    n_delete_headers++;

    actual_header_name = g_strdup(name);
    actual_header_index = index;

    return MILTER_STATUS_CONTINUE;
}

static MilterStatus
cb_change_from (MilterServerContext *context,
                const gchar *from,
                const gchar *parameters,
                gpointer user_data)
{
    n_change_froms++;

    actual_envelope_from = g_strdup(from);
    actual_envelope_parameters = g_strdup(parameters);

    return MILTER_STATUS_CONTINUE;
}

static MilterStatus
cb_add_recipient (MilterServerContext *context,
                  const gchar *recipient,
                  const gchar *parameters,
                  gpointer user_data)
{
    n_add_recipients++;

    actual_recipient = g_strdup(recipient);
    actual_recipient_parameters = g_strdup(parameters);

    return MILTER_STATUS_CONTINUE;
}

static MilterStatus
cb_delete_recipient (MilterServerContext *context,
                     const gchar *recipient,
                     gpointer user_data)
{
    n_delete_recipients++;

    actual_recipient = g_strdup(recipient);

    return MILTER_STATUS_CONTINUE;
}

static MilterStatus
cb_replace_body (MilterServerContext *context,
                 const gchar *body,
                 gsize body_sze,
                 gsize *packged_size,
                 gpointer user_data)
{
    n_replace_bodys++;

    return MILTER_STATUS_CONTINUE;
}


static MilterStatus
cb_progress (MilterServerContext *context, gpointer user_data)
{
    n_progresss++;

    return MILTER_STATUS_CONTINUE;
}

static MilterStatus
cb_quarantine (MilterServerContext *context,
               const gchar *reason,
               gpointer user_data)
{
    n_quarantines++;

    actual_quarantine_reason = g_strdup(reason);

    return MILTER_STATUS_CONTINUE;
}

static MilterStatus
cb_connection_failure (MilterServerContext *context, gpointer user_data)
{
    n_connection_failures++;

    return MILTER_STATUS_CONTINUE;
}

static MilterStatus
cb_shutdown (MilterServerContext *context, gpointer user_data)
{
    n_shutdowns++;

    return MILTER_STATUS_CONTINUE;
}

static MilterStatus
cb_skip (MilterServerContext *context, gpointer user_data)
{
    n_skips++;

    return MILTER_STATUS_CONTINUE;
}

static void
cb_error (MilterErrorEmittable *emittable, GError *error)
{
    GError *local_error;

    n_errors++;

    local_error = actual_error;
    actual_error = NULL;
    gcut_assert_error(local_error);
    actual_error = g_error_copy(error);
}

static void
setup_signals (MilterServerContext *context)
{
#define CONNECT(name) do                                                \
{                                                                       \
    g_signal_connect(context, #name, G_CALLBACK(cb_ ## name), NULL);    \
} while (0)

    CONNECT(negotiate_reply);
    CONNECT(continue);
    CONNECT(reply_code);
    CONNECT(temporary_failure);
    CONNECT(reject);
    CONNECT(accept);
    CONNECT(discard);
    CONNECT(add_header);
    CONNECT(insert_header);
    CONNECT(change_header);
    CONNECT(delete_header);
    CONNECT(change_from);
    CONNECT(add_recipient);
    CONNECT(delete_recipient);
    CONNECT(replace_body);
    CONNECT(progress);
    CONNECT(quarantine);
    CONNECT(connection_failure);
    CONNECT(shutdown);
    CONNECT(skip);
    CONNECT(error);

#undef CONNECT
}

void
cut_setup (void)
{
    GError *error = NULL;

    loop = milter_test_event_loop_new();

    context = milter_server_context_new();
    milter_server_context_set_name(context, "test-server-context-signals");
    milter_agent_set_event_loop(MILTER_AGENT(context), loop);
    setup_signals(context);

    {
        MilterOption *option;
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
        g_object_unref(option);
    }

    read_channel = gcut_string_io_channel_new(NULL);
    gcut_string_io_channel_set_pipe_mode(read_channel, TRUE);
    g_io_channel_set_encoding(read_channel, NULL, NULL);
    g_io_channel_set_buffered(read_channel, FALSE);
    reader = milter_reader_io_channel_new(read_channel);
    milter_agent_set_reader(MILTER_AGENT(context), reader);

    write_channel = gcut_string_io_channel_new(NULL);
    g_io_channel_set_encoding(write_channel, NULL, NULL);
    writer = milter_writer_io_channel_new(write_channel);
    milter_agent_set_writer(MILTER_AGENT(context), writer);

    milter_agent_start(MILTER_AGENT(context), &error);
    gcut_assert_error(error);

    encoder = MILTER_REPLY_ENCODER(milter_reply_encoder_new());

    option = NULL;
    requests = NULL;

    actual_error = NULL;
    expected_error = NULL;

    data_written = FALSE;

    n_negotiate_replys = 0;
    n_continues = 0;
    n_reply_codes = 0;
    n_temporary_failures = 0;
    n_rejects = 0;
    n_accepts = 0;
    n_discards = 0;
    n_add_headers = 0;
    n_insert_headers = 0;
    n_change_headers = 0;
    n_delete_headers = 0;
    n_change_froms = 0;
    n_add_recipients = 0;
    n_delete_recipients = 0;
    n_progresss = 0;
    n_quarantines = 0;
    n_connection_failures = 0;
    n_shutdowns = 0;
    n_skips = 0;
    n_errors = 0;

    actual_header_index = -1;
    actual_reply_code = 0;
    actual_reply_extended_code = NULL;
    actual_reply_message = NULL;
    actual_header_name = NULL;
    actual_header_value = NULL;
    actual_envelope_from = NULL;
    actual_envelope_recipient = NULL;
    actual_envelope_parameters = NULL;
    actual_recipient = NULL;
    actual_recipient_parameters = NULL;
    actual_quarantine_reason = NULL;
    actual_option = NULL;
    actual_requests = NULL;

    timed_out_before = FALSE;
    timed_out_after = FALSE;
    timeout_id = 0;

    buffer = g_string_new(NULL);
}

void
cut_teardown (void)
{
    if (timeout_id > 0)
        milter_event_loop_remove(loop, timeout_id);

    if (context)
        g_object_unref(context);

    if (reader)
        g_object_unref(reader);

    if (writer)
        g_object_unref(writer);

    if (encoder)
        g_object_unref(encoder);

    if (read_channel) {
        gcut_string_io_channel_set_limit(read_channel, 0);
        g_io_channel_unref(read_channel);
    }

    if (write_channel) {
        gcut_string_io_channel_set_limit(write_channel, 0);
        g_io_channel_unref(write_channel);
    }

    if (option)
        g_object_unref(option);
    if (requests)
        g_object_unref(requests);

    if (actual_error)
        g_error_free(actual_error);
    if (expected_error)
        g_error_free(expected_error);

    if (actual_reply_extended_code)
        g_free(actual_reply_extended_code);
    if (actual_reply_message)
        g_free(actual_reply_message);
    if (actual_header_name)
        g_free(actual_header_name);
    if (actual_header_value)
        g_free(actual_header_value);
    if (actual_envelope_from)
        g_free(actual_envelope_from);
    if (actual_envelope_recipient)
        g_free(actual_envelope_recipient);
    if (actual_envelope_parameters)
        g_free(actual_envelope_parameters);
    if (actual_recipient)
        g_free(actual_recipient);
    if (actual_recipient_parameters)
        g_free(actual_recipient_parameters);
    if (actual_quarantine_reason)
        g_free(actual_quarantine_reason);
    if (actual_option)
        g_object_unref(actual_option);
    if (actual_requests)
        g_object_unref(actual_requests);

    if (buffer)
        g_string_free(buffer, TRUE);

    if (loop)
        g_object_unref(loop);
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

static void
write_data_without_error_handling (const gchar *data, gsize data_size)
{
    gsize bytes_written;
    GError *error = NULL;

    g_io_channel_write_chars(read_channel, data, data_size,
                             &bytes_written, &error);
    gcut_assert_error(error);
    cut_assert_equal_int(data_size, bytes_written);

    milter_test_pump_all_events(loop);
}

static void
write_data (const gchar *data, gsize data_size)
{
    GError *error;

    write_data_without_error_handling(data, data_size);
    error = actual_error;
    actual_error = NULL;
    gcut_assert_error(error);
}

#define milter_assert_status(expected)                                  \
    gcut_assert_equal_enum(MILTER_TYPE_STATUS,                          \
                           expected,                                    \
                           milter_server_context_get_status(context))

void
test_negotiate_reply (void)
{
    const gchar *packet;
    gsize packet_size;

    option = milter_option_new(2,
                               MILTER_ACTION_ADD_HEADERS,
                               MILTER_STEP_NO_HEADERS);
    requests = milter_macros_requests_new();
    milter_macros_requests_set_symbols(requests,
                                       MILTER_COMMAND_HELO,
                                       "G", "N", "U", NULL);

    milter_server_context_negotiate(context, option);
    pump_all_events();

    milter_reply_encoder_encode_negotiate(encoder,
                                          &packet, &packet_size,
                                          option, requests);
    write_data(packet, packet_size);
    cut_assert_equal_int(1, n_negotiate_replys);
    milter_assert_equal_option(option, actual_option);
    milter_assert_equal_macros_requests(requests, actual_requests);
    milter_assert_status(MILTER_STATUS_NOT_CHANGE);
}

void
test_continue (void)
{
    const gchar *packet;
    gsize packet_size;

    milter_server_context_helo(context, "delian");
    pump_all_events();

    milter_reply_encoder_encode_continue(encoder, &packet, &packet_size);
    write_data(packet, packet_size);
    cut_assert_equal_int(1, n_continues);
    milter_assert_status(MILTER_STATUS_NOT_CHANGE);
}

void
test_reply_code_temporary_failure (void)
{
    const gchar *packet;
    gsize packet_size;
    const gchar code[] = "451 4.7.1 Greylisting in action, please come back";

    milter_server_context_set_state(context,
                                    MILTER_SERVER_CONTEXT_STATE_DATA);

    milter_reply_encoder_encode_reply_code(encoder, &packet, &packet_size, code);
    write_data(packet, packet_size);
    cut_assert_equal_int(1, n_reply_codes);
    cut_assert_equal_uint(451, actual_reply_code);
    cut_assert_equal_string("4.7.1", actual_reply_extended_code);
    cut_assert_equal_string("Greylisting in action, please come back",
                            actual_reply_message);
    milter_assert_status(MILTER_STATUS_TEMPORARY_FAILURE);
}

void
test_reply_code_temporary_failure_on_invalid_state (void)
{
    const gchar *packet;
    gsize packet_size;
    const gchar code[] = "451 4.7.1 Greylisting in action, please come back";

    milter_reply_encoder_encode_reply_code(encoder, &packet, &packet_size, code);
    write_data_without_error_handling(packet, packet_size);
    cut_assert_equal_int(0, n_reply_codes);
    cut_assert_equal_int(1, n_errors);

    expected_error = g_error_new(MILTER_SERVER_CONTEXT_ERROR,
                                 MILTER_SERVER_CONTEXT_ERROR_INVALID_STATE,
                                 "<reply-code> should be caused on "
                                 "<connect, helo, envelope-from, "
                                 "envelope-recipient, data, header, "
                                 "end-of-header, body, end-of-message, "
                                 "unknown> but was <start>");
    gcut_assert_equal_error(expected_error, actual_error);
}

void
test_reply_code_reject (void)
{
    const gchar *packet;
    gsize packet_size;
    const gchar code[] = "554 5.7.1 1% 2%% 3%%%";

    milter_server_context_set_state(context,
                                    MILTER_SERVER_CONTEXT_STATE_DATA);

    milter_reply_encoder_encode_reply_code(encoder, &packet, &packet_size, code);
    write_data(packet, packet_size);
    cut_assert_equal_int(1, n_reply_codes);
    cut_assert_equal_uint(554, actual_reply_code);
    cut_assert_equal_string("5.7.1", actual_reply_extended_code);
    cut_assert_equal_string("1% 2%% 3%%%",
                            actual_reply_message);
    milter_assert_status(MILTER_STATUS_REJECT);
}

void
test_reply_code_reject_on_invalid_state (void)
{
    const gchar *packet;
    gsize packet_size;
    const gchar code[] = "554 5.7.1 1% 2%% 3%%%";

    milter_reply_encoder_encode_reply_code(encoder, &packet, &packet_size, code);
    write_data_without_error_handling(packet, packet_size);
    cut_assert_equal_int(0, n_reply_codes);
    cut_assert_equal_int(1, n_errors);

    expected_error = g_error_new(MILTER_SERVER_CONTEXT_ERROR,
                                 MILTER_SERVER_CONTEXT_ERROR_INVALID_STATE,
                                 "<reply-code> should be caused on "
                                 "<connect, helo, envelope-from, "
                                 "envelope-recipient, data, header, "
                                 "end-of-header, body, end-of-message, "
                                 "unknown> but was <start>");
    gcut_assert_equal_error(expected_error, actual_error);
}

void
test_temporary_failure (void)
{
    const gchar *packet;
    gsize packet_size;

    milter_reply_encoder_encode_temporary_failure(encoder, &packet, &packet_size);
    write_data(packet, packet_size);
    cut_assert_equal_int(1, n_temporary_failures);
    milter_assert_status(MILTER_STATUS_TEMPORARY_FAILURE);
}

void
test_reject (void)
{
    const gchar *packet;
    gsize packet_size;

    milter_reply_encoder_encode_reject(encoder, &packet, &packet_size);
    write_data(packet, packet_size);
    cut_assert_equal_int(1, n_rejects);
    milter_assert_status(MILTER_STATUS_REJECT);
}

void
test_accept (void)
{
    const gchar *packet;
    gsize packet_size;

    milter_reply_encoder_encode_accept(encoder, &packet, &packet_size);
    write_data(packet, packet_size);
    cut_assert_equal_int(1, n_accepts);
    milter_assert_status(MILTER_STATUS_ACCEPT);
}

void
test_discard (void)
{
    const gchar *packet;
    gsize packet_size;

    milter_reply_encoder_encode_discard(encoder, &packet, &packet_size);
    write_data(packet, packet_size);
    cut_assert_equal_int(1, n_discards);
    milter_assert_status(MILTER_STATUS_DISCARD);
}

void
test_add_header (void)
{
    const gchar *packet;
    gsize packet_size;
    const gchar name[] = "X-HEADER-NAME";
    const gchar value[] = "MilterServerContext test";

    milter_server_context_end_of_message(context, NULL, 0);
    pump_all_events();

    cut_assert_true(milter_server_context_is_processing(context));
    milter_reply_encoder_encode_add_header(encoder,
                                           &packet, &packet_size,
                                           name, value);
    write_data(packet, packet_size);
    cut_assert_equal_int(1, n_add_headers);
    cut_assert_equal_string(name, actual_header_name);
    cut_assert_equal_string(value, actual_header_value);
    milter_assert_status(MILTER_STATUS_NOT_CHANGE);
    cut_assert_true(milter_server_context_is_processing(context));
}

void
test_add_header_on_invalid_state (void)
{
    const gchar *packet;
    gsize packet_size;
    const gchar name[] = "X-HEADER-NAME";
    const gchar value[] = "MilterServerContext test";

    milter_reply_encoder_encode_add_header(encoder,
                                           &packet, &packet_size,
                                           name, value);
    write_data_without_error_handling(packet, packet_size);
    cut_assert_equal_int(0, n_add_headers);
    cut_assert_equal_int(1, n_errors);

    expected_error = g_error_new(MILTER_SERVER_CONTEXT_ERROR,
                                 MILTER_SERVER_CONTEXT_ERROR_INVALID_STATE,
                                 "<add-header> should be caused on "
                                 "<end-of-message> but was <start>");
    gcut_assert_equal_error(expected_error, actual_error);
}

void
test_insert_header (void)
{
    const gchar *packet;
    gsize packet_size;
    const gchar name[] = "X-HEADER-NAME";
    const gchar value[] = "MilterServerContext test";

    milter_server_context_end_of_message(context, NULL, 0);
    pump_all_events();

    cut_assert_true(milter_server_context_is_processing(context));
    milter_reply_encoder_encode_insert_header(encoder,
                                              &packet, &packet_size,
                                              G_MAXUINT32, name, value);
    write_data(packet, packet_size);
    cut_assert_equal_int(1, n_insert_headers);
    cut_assert_equal_int(G_MAXUINT32, actual_header_index);
    cut_assert_equal_string(name, actual_header_name);
    cut_assert_equal_string(value, actual_header_value);
    milter_assert_status(MILTER_STATUS_NOT_CHANGE);
    cut_assert_true(milter_server_context_is_processing(context));
}

void
test_insert_header_on_invalid_state (void)
{
    const gchar *packet;
    gsize packet_size;
    const gchar name[] = "X-HEADER-NAME";
    const gchar value[] = "MilterServerContext test";

    milter_reply_encoder_encode_insert_header(encoder,
                                              &packet, &packet_size,
                                              G_MAXUINT32, name, value);
    write_data_without_error_handling(packet, packet_size);
    cut_assert_equal_int(0, n_insert_headers);
    cut_assert_equal_int(1, n_errors);

    expected_error = g_error_new(MILTER_SERVER_CONTEXT_ERROR,
                                 MILTER_SERVER_CONTEXT_ERROR_INVALID_STATE,
                                 "<insert-header> should be caused on "
                                 "<end-of-message> but was <start>");
    gcut_assert_equal_error(expected_error, actual_error);
}

void
test_change_header (void)
{
    const gchar *packet;
    gsize packet_size;
    const gchar name[] = "X-HEADER-NAME";
    const gchar value[] = "MilterServerContext test";

    milter_server_context_end_of_message(context, NULL, 0);
    pump_all_events();

    cut_assert_true(milter_server_context_is_processing(context));
    milter_reply_encoder_encode_change_header(encoder, &packet, &packet_size,
                                              name, 0, value);
    write_data(packet, packet_size);
    cut_assert_equal_int(1, n_change_headers);
    cut_assert_equal_int(0, actual_header_index);
    cut_assert_equal_string(name, actual_header_name);
    cut_assert_equal_string(value, actual_header_value);
    milter_assert_status(MILTER_STATUS_NOT_CHANGE);
    cut_assert_true(milter_server_context_is_processing(context));
}

void
test_change_header_on_invalid_state (void)
{
    const gchar *packet;
    gsize packet_size;
    const gchar name[] = "X-HEADER-NAME";
    const gchar value[] = "MilterServerContext test";

    milter_reply_encoder_encode_change_header(encoder, &packet, &packet_size,
                                              name, 0, value);
    write_data_without_error_handling(packet, packet_size);
    cut_assert_equal_int(0, n_change_headers);
    cut_assert_equal_int(1, n_errors);

    expected_error = g_error_new(MILTER_SERVER_CONTEXT_ERROR,
                                 MILTER_SERVER_CONTEXT_ERROR_INVALID_STATE,
                                 "<change-header> should be caused on "
                                 "<end-of-message> but was <start>");
    gcut_assert_equal_error(expected_error, actual_error);
}

void
test_delete_header (void)
{
    const gchar *packet;
    gsize packet_size;
    const gchar name[] = "X-HEADER-NAME";

    milter_server_context_end_of_message(context, NULL, 0);
    pump_all_events();

    cut_assert_true(milter_server_context_is_processing(context));
    milter_reply_encoder_encode_delete_header(encoder, &packet, &packet_size,
                                              name, 0);
    write_data(packet, packet_size);
    cut_assert_equal_int(1, n_delete_headers);
    cut_assert_equal_int(0, actual_header_index);
    cut_assert_equal_string(name, actual_header_name);
    milter_assert_status(MILTER_STATUS_NOT_CHANGE);
    cut_assert_true(milter_server_context_is_processing(context));
}

void
test_delete_header_on_invalid_state (void)
{
    const gchar *packet;
    gsize packet_size;
    const gchar name[] = "X-HEADER-NAME";

    milter_reply_encoder_encode_delete_header(encoder, &packet, &packet_size,
                                              name, 0);
    write_data_without_error_handling(packet, packet_size);
    cut_assert_equal_int(0, n_delete_headers);
    cut_assert_equal_int(1, n_errors);

    expected_error = g_error_new(MILTER_SERVER_CONTEXT_ERROR,
                                 MILTER_SERVER_CONTEXT_ERROR_INVALID_STATE,
                                 "<delete-header> should be caused on "
                                 "<end-of-message> but was <start>");
    gcut_assert_equal_error(expected_error, actual_error);
}

void
test_change_from (void)
{
    const gchar *packet;
    gsize packet_size;
    const gchar from[] = "example@example.com";

    milter_server_context_end_of_message(context, NULL, 0);
    pump_all_events();

    cut_assert_true(milter_server_context_is_processing(context));
    milter_reply_encoder_encode_change_from(encoder,
                                            &packet, &packet_size,
                                            from, NULL);
    write_data(packet, packet_size);
    cut_assert_equal_int(1, n_change_froms);
    cut_assert_equal_string(from, actual_envelope_from);
    milter_assert_status(MILTER_STATUS_NOT_CHANGE);
    cut_assert_true(milter_server_context_is_processing(context));
}

void
test_change_from_on_invalid_state (void)
{
    const gchar *packet;
    gsize packet_size;
    const gchar from[] = "example@example.com";

    milter_reply_encoder_encode_change_from(encoder,
                                            &packet, &packet_size,
                                            from, NULL);
    write_data_without_error_handling(packet, packet_size);
    cut_assert_equal_int(0, n_change_froms);
    cut_assert_equal_int(1, n_errors);

    expected_error = g_error_new(MILTER_SERVER_CONTEXT_ERROR,
                                 MILTER_SERVER_CONTEXT_ERROR_INVALID_STATE,
                                 "<change-from> should be caused on "
                                 "<end-of-message> but was <start>");
    gcut_assert_equal_error(expected_error, actual_error);
}

void
test_add_recipient (void)
{
    const gchar *packet;
    gsize packet_size;
    const gchar recipient[] = "example@example.com";

    milter_server_context_end_of_message(context, NULL, 0);
    pump_all_events();

    cut_assert_true(milter_server_context_is_processing(context));
    milter_reply_encoder_encode_add_recipient(encoder,
                                              &packet, &packet_size,
                                              recipient, NULL);
    write_data(packet, packet_size);
    cut_assert_equal_int(1, n_add_recipients);
    cut_assert_equal_string(recipient, actual_recipient);
    cut_assert_null(actual_recipient_parameters);
    milter_assert_status(MILTER_STATUS_NOT_CHANGE);
    cut_assert_true(milter_server_context_is_processing(context));
}

void
test_add_recipient_on_invalid_state (void)
{
    const gchar *packet;
    gsize packet_size;
    const gchar recipient[] = "example@example.com";

    milter_reply_encoder_encode_add_recipient(encoder,
                                              &packet, &packet_size,
                                              recipient, NULL);
    write_data_without_error_handling(packet, packet_size);
    cut_assert_equal_int(0, n_add_recipients);
    cut_assert_equal_int(1, n_errors);

    expected_error = g_error_new(MILTER_SERVER_CONTEXT_ERROR,
                                 MILTER_SERVER_CONTEXT_ERROR_INVALID_STATE,
                                 "<add-recipient> should be caused on "
                                 "<end-of-message> but was <start>");
    gcut_assert_equal_error(expected_error, actual_error);
}

void
test_delete_recipient (void)
{
    const gchar *packet;
    gsize packet_size;
    const gchar recipient[] = "example@example.com";

    milter_server_context_end_of_message(context, NULL, 0);
    pump_all_events();

    cut_assert_true(milter_server_context_is_processing(context));
    milter_reply_encoder_encode_delete_recipient(encoder,
                                                 &packet, &packet_size,
                                                 recipient);
    write_data(packet, packet_size);
    cut_assert_equal_int(1, n_delete_recipients);
    cut_assert_equal_string(recipient, actual_recipient);
    milter_assert_status(MILTER_STATUS_NOT_CHANGE);
    cut_assert_true(milter_server_context_is_processing(context));
}

void
test_delete_recipient_on_invalid_state (void)
{
    const gchar *packet;
    gsize packet_size;
    const gchar recipient[] = "example@example.com";

    milter_reply_encoder_encode_delete_recipient(encoder,
                                                 &packet, &packet_size,
                                                 recipient);
    write_data_without_error_handling(packet, packet_size);
    cut_assert_equal_int(0, n_delete_recipients);
    cut_assert_equal_int(1, n_errors);

    expected_error = g_error_new(MILTER_SERVER_CONTEXT_ERROR,
                                 MILTER_SERVER_CONTEXT_ERROR_INVALID_STATE,
                                 "<delete-recipient> should be caused on "
                                 "<end-of-message> but was <start>");
    gcut_assert_equal_error(expected_error, actual_error);
}

void
test_replace_body (void)
{
    const gchar *packet;
    gsize packet_size;
    const gchar chunk[] = "This is a body text.";
    gsize packed_size;

    milter_server_context_end_of_message(context, NULL, 0);
    pump_all_events();

    cut_assert_true(milter_server_context_is_processing(context));
    milter_reply_encoder_encode_replace_body(encoder, &packet, &packet_size,
                                             chunk, strlen(chunk), &packed_size);
    write_data(packet, packet_size);
    cut_assert_equal_int(1, n_replace_bodys);
    milter_assert_status(MILTER_STATUS_NOT_CHANGE);
    cut_assert_true(milter_server_context_is_processing(context));
}

void
test_replace_body_on_invalid_state (void)
{
    const gchar *packet;
    gsize packet_size;
    const gchar chunk[] = "This is a body text.";
    gsize packed_size;

    milter_reply_encoder_encode_replace_body(encoder, &packet, &packet_size,
                                             chunk, strlen(chunk), &packed_size);
    write_data_without_error_handling(packet, packet_size);
    cut_assert_equal_int(0, n_replace_bodys);
    cut_assert_equal_int(1, n_errors);

    expected_error = g_error_new(MILTER_SERVER_CONTEXT_ERROR,
                                 MILTER_SERVER_CONTEXT_ERROR_INVALID_STATE,
                                 "<replace-body> should be caused on "
                                 "<end-of-message> but was <start>");
    gcut_assert_equal_error(expected_error, actual_error);
}

void
test_progress (void)
{
    const gchar *packet;
    gsize packet_size;

    milter_server_context_end_of_message(context, NULL, 0);
    pump_all_events();

    cut_assert_true(milter_server_context_is_processing(context));
    milter_reply_encoder_encode_progress(encoder, &packet, &packet_size);
    write_data(packet, packet_size);
    cut_assert_equal_int(1, n_progresss);
    milter_assert_status(MILTER_STATUS_NOT_CHANGE);
    cut_assert_true(milter_server_context_is_processing(context));
}

void
test_progress_on_invalid_state (void)
{
    const gchar *packet;
    gsize packet_size;

    milter_reply_encoder_encode_progress(encoder, &packet, &packet_size);
    write_data_without_error_handling(packet, packet_size);
    cut_assert_equal_int(0, n_progresss);
    cut_assert_equal_int(1, n_errors);

    expected_error = g_error_new(MILTER_SERVER_CONTEXT_ERROR,
                                 MILTER_SERVER_CONTEXT_ERROR_INVALID_STATE,
                                 "<progress> should be caused on "
                                 "<end-of-message> but was <start>");
    gcut_assert_equal_error(expected_error, actual_error);
}

void
test_quarantine (void)
{
    const gchar *packet;
    gsize packet_size;
    const gchar reason[] = "infection";

    milter_server_context_end_of_message(context, NULL, 0);
    pump_all_events();

    cut_assert_true(milter_server_context_is_processing(context));
    milter_reply_encoder_encode_quarantine(encoder, &packet, &packet_size, reason);
    write_data_without_error_handling(packet, packet_size);
    cut_assert_equal_int(1, n_quarantines);
    cut_assert_equal_string(reason, actual_quarantine_reason);
    milter_assert_status(MILTER_STATUS_QUARANTINE);
    cut_assert_true(milter_server_context_is_processing(context));
}

void
test_quarantine_on_invalid_state (void)
{
    const gchar *packet;
    gsize packet_size;
    const gchar reason[] = "infection";

    milter_reply_encoder_encode_quarantine(encoder, &packet, &packet_size, reason);
    write_data_without_error_handling(packet, packet_size);
    cut_assert_equal_int(0, n_quarantines);
    cut_assert_equal_int(1, n_errors);

    expected_error = g_error_new(MILTER_SERVER_CONTEXT_ERROR,
                                 MILTER_SERVER_CONTEXT_ERROR_INVALID_STATE,
                                 "<quarantine> should be caused on "
                                 "<end-of-message> but was <start>");
    gcut_assert_equal_error(expected_error, actual_error);
}

void
test_connection_failure (void)
{
    const gchar *packet;
    gsize packet_size;

    milter_reply_encoder_encode_connection_failure(encoder, &packet, &packet_size);
    write_data(packet, packet_size);
    cut_assert_equal_int(1, n_connection_failures);
    milter_assert_status(MILTER_STATUS_ERROR);
}

void
test_shutdown (void)
{
    const gchar *packet;
    gsize packet_size;

    milter_reply_encoder_encode_shutdown(encoder, &packet, &packet_size);
    write_data(packet, packet_size);
    cut_assert_equal_int(1, n_shutdowns);
    milter_assert_status(MILTER_STATUS_NOT_CHANGE);
}

void
test_skip (void)
{
    const gchar *packet;
    gsize packet_size;

    milter_server_context_set_state(context,
                                    MILTER_SERVER_CONTEXT_STATE_BODY);

    milter_reply_encoder_encode_skip(encoder, &packet, &packet_size);
    write_data(packet, packet_size);
    cut_assert_equal_int(1, n_skips);
    cut_assert_true(milter_server_context_get_skip_body(context));
    milter_assert_status(MILTER_STATUS_NOT_CHANGE);
}

void
test_skip_on_invalid_state (void)
{
    const gchar *packet;
    gsize packet_size;

    milter_reply_encoder_encode_skip(encoder, &packet, &packet_size);
    write_data_without_error_handling(packet, packet_size);
    cut_assert_equal_int(0, n_skips);
    cut_assert_equal_int(1, n_errors);

    expected_error = g_error_new(MILTER_SERVER_CONTEXT_ERROR,
                                 MILTER_SERVER_CONTEXT_ERROR_INVALID_STATE,
                                 "<skip> should be caused on "
                                 "<body> but was <start>");
    cut_assert_false(milter_server_context_get_skip_body(context));
    gcut_assert_equal_error(expected_error, actual_error);
}

static gboolean
cb_timeout (MilterServerContext *context, gpointer data)
{
    gboolean *timeout_flag = data;

    *timeout_flag = TRUE;
    gcut_string_io_channel_set_buffer_limit(read_channel, 0);
    return FALSE;
}

static gboolean
cb_timeout_detect (gpointer data)
{
    gboolean *timeout_flag = data;

    *timeout_flag = TRUE;
    return FALSE;
}

static gboolean
cb_io_timeout_detect (gpointer data)
{
    gboolean *timeout_flag = data;

    *timeout_flag = TRUE;
    gcut_string_io_channel_set_buffer_limit(write_channel, 0);
    return FALSE;
}

void
test_reading_timeout (void)
{
    gboolean timed_out_reading = FALSE;

    return;
    milter_server_context_set_reading_timeout(context, 1);
    milter_server_context_helo(context, "delian");

    g_signal_connect(context, "reading-timeout",
                     G_CALLBACK(cb_timeout), &timed_out_reading);
    milter_event_loop_add_timeout(loop, 0.5,
                                  cb_timeout_detect, &timed_out_before);
    timeout_id = milter_event_loop_add_timeout(loop,
                                               2,
                                               cb_timeout_detect,
                                               &timed_out_after);
    while (!timed_out_reading && !timed_out_after)
        milter_event_loop_iterate(loop, TRUE);
    cut_assert_true(timed_out_before);
    cut_assert_true(timed_out_reading);
    cut_assert_false(timed_out_after);
}

void
test_writing_timeout (void)
{
    gboolean timed_out_writing = FALSE;

    return;
    milter_server_context_set_writing_timeout(context, 1);
    g_signal_connect(context, "writing-timeout",
                     G_CALLBACK(cb_timeout), &timed_out_writing);
    milter_event_loop_add_timeout(loop, 0.5,
                                  cb_timeout_detect, &timed_out_before);
    timeout_id = milter_event_loop_add_timeout(loop,
                                               2,
                                               cb_io_timeout_detect,
                                               &timed_out_after);

    g_io_channel_set_buffered(write_channel, FALSE);
    gcut_string_io_channel_set_buffer_limit(write_channel, 1);
    gcut_string_io_channel_set_pipe_mode(write_channel, TRUE);
    g_io_channel_set_close_on_unref(write_channel, TRUE);

    milter_server_context_helo(context, "delian");
    while (!timed_out_writing && !timed_out_after)
        milter_event_loop_iterate(loop, TRUE);
    cut_assert_true(timed_out_before);
    cut_assert_true(timed_out_writing);
    cut_assert_false(timed_out_after);
}

void
test_busy_error (void)
{
    milter_server_context_helo(context, "delian");
    pump_all_events();
    milter_server_context_data(context);
    milter_test_pump_all_events(loop);

    expected_error = g_error_new(MILTER_SERVER_CONTEXT_ERROR,
                                 MILTER_SERVER_CONTEXT_ERROR_BUSY,
                                 "previous command has been processing: "
                                 "helo -> data");
    gcut_assert_equal_error(expected_error, actual_error);
}

void
test_no_busy_on_abort (void)
{
    milter_server_context_helo(context, "delian");
    pump_all_events();
    milter_server_context_abort(context);
    pump_all_events();
    milter_server_context_quit(context);
    pump_all_events();
}

static void
pack (GString *buffer)
{
    guint32 content_size;
    gchar content_string[sizeof(guint32)];

    content_size = g_htonl(buffer->len);
    memcpy(content_string, &content_size, sizeof(content_size));
    g_string_prepend_len(buffer, content_string, sizeof(content_size));
}

void
test_decode_error (void)
{
    g_string_append(buffer, "XXXX");
    pack(buffer);
    write_data_without_error_handling(buffer->str, buffer->len);

    expected_error = g_error_new(MILTER_AGENT_ERROR,
                                 MILTER_AGENT_ERROR_DECODE_ERROR,
                                 "Decode error: "
                                 "milter-decoder-error-quark:%u: "
                                 "unexpected reply was received: X",
                                 MILTER_DECODER_ERROR_UNEXPECTED_COMMAND);
    gcut_assert_equal_error(expected_error, actual_error);
}

void
test_write_error (void)
{
    GError *error = NULL;

    if (MILTER_IS_LIBEV_EVENT_LOOP(loop))
        cut_omit("MilterLibevEventLoop doesn't support GCutStringIOChannel.");

    g_io_channel_set_buffered(write_channel, FALSE);
    g_io_channel_set_flags(write_channel, G_IO_FLAG_NONBLOCK, &error);
    gcut_assert_error(error);
    gcut_string_io_channel_set_limit(write_channel, 1);

    milter_server_context_helo(context, "delian");
    milter_test_pump_all_events(loop);

    expected_error = g_error_new(MILTER_AGENT_ERROR,
                                 MILTER_AGENT_ERROR_IO_ERROR,
                                 "Output error: "
                                 "%s:%d: failed to write: "
                                 "%s:%d: %s",
                                 g_quark_to_string(MILTER_WRITER_ERROR),
                                 MILTER_WRITER_ERROR_IO_ERROR,
                                 g_quark_to_string(G_IO_CHANNEL_ERROR),
                                 G_IO_CHANNEL_ERROR_NOSPC,
                                 g_strerror(ENOSPC));
    gcut_assert_equal_error(expected_error, actual_error);
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
