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

#include <gcutter.h>

#define shutdown inet_shutdown
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <arpa/inet.h>
#undef shutdown

#include <milter/core/milter-reply-decoder.h>
#include <milter/core/milter-enum-types.h>
#include <milter/core/milter-macros-requests.h>
#include "../lib/milter-test-utils.h"

void data_decode_negotiate (void);
void test_decode_negotiate (gconstpointer data);
void test_decode_negotiate_without_null (void);
void test_decode_continue (void);
void test_decode_continue_with_garbage (void);
void test_decode_temporary_failure (void);
void test_decode_temporary_failure_with_garbage (void);
void test_decode_reject (void);
void test_decode_reject_with_garbage (void);
void test_decode_accept (void);
void test_decode_accept_with_garbage (void);
void test_decode_discard (void);
void test_decode_discard_with_garbage (void);
void test_decode_progress (void);
void test_decode_progress_with_garbage (void);
void test_decode_connection_failure (void);
void test_decode_connection_failure_with_garbage (void);
void test_decode_shutdown (void);
void test_decode_shutdown_with_garbage (void);
void test_decode_skip (void);
void test_decode_skip_with_garbage (void);
void test_decode_reply_code (void);
void test_decode_reply_code_without_null (void);
void test_decode_delete_recipient (void);
void test_decode_delete_recipient_without_null (void);
void test_decode_quarantine (void);
void test_decode_quarantine_without_null (void);

void test_decode_add_header (void);
void test_decode_add_header_without_name_null (void);
void test_decode_add_header_without_value_null (void);

void test_decode_add_recipient (void);
void test_decode_add_recipient_without_null (void);

void test_decode_add_recipient_with_parameters (void);
void test_decode_add_recipient_with_parameters_without_recipient_null (void);
void test_decode_add_recipient_with_parameters_without_parameters_null (void);

void test_decode_change_from_with_parameters (void);
void test_decode_change_from_with_parameters_without_from_null (void);
void test_decode_change_from_with_parameters_without_parameters_null (void);

void test_decode_replace_body (void);

void test_decode_insert_header (void);
void test_decode_change_header (void);
void test_decode_delete_header (void);

static MilterDecoder *decoder;

static GString *buffer;

static GError *expected_error;
static GError *actual_error;

static gint n_negotiate_replys;
static gint n_continues;
static gint n_temporary_failures;
static gint n_rejects;
static gint n_accepts;
static gint n_discards;
static gint n_progresss;
static gint n_reply_codes;
static gint n_delete_recipients;
static gint n_quarantines;
static gint n_connection_failures;
static gint n_shutdowns;
static gint n_skips;
static gint n_add_headers;
static gint n_add_recipients;
static gint n_change_froms;
static gint n_bodies;
static gint n_insert_headers;
static gint n_change_headers;
static gint n_delete_headers;

static guint reply_code;
static gchar *reply_extended_code;
static gchar *reply_message;
static gchar *delete_recipient;
static gchar *quarantine;

static MilterOption *actual_negotiate_option;
static MilterMacrosRequests *actual_macros_requests;
static guint32  actual_header_index;
static gchar *actual_header_name;
static gchar *actual_header_value;
static gchar *actual_recipient;
static gchar *actual_recipient_parameters;
static gchar *actual_from;
static gchar *actual_from_parameters;
static gchar *actual_macros;
static MilterMacroStage actual_macro_stage;

static gchar *actual_body_chunk;
static gsize actual_body_chunk_length;

static guint32 negotiate_version;
static MilterActionFlags negotiate_action;
static MilterStepFlags negotiate_step;
static MilterMacrosRequests *negotiate_macros_requests;

static void
cb_negotiate_reply (MilterReplyDecoder *decoder,
                    MilterOption *option,
                    MilterMacrosRequests *macros_requests,
                    gpointer user_data)
{
    n_negotiate_replys++;
    actual_negotiate_option = g_object_ref(option);
    if (macros_requests)
        actual_macros_requests = g_object_ref(macros_requests);
}

static void
cb_continue (MilterReplyDecoder *decoder, gpointer user_data)
{
    n_continues++;
}

static void
cb_temporary_failure (MilterReplyDecoder *decoder, gpointer user_data)
{
    n_temporary_failures++;
}

static void
cb_reject (MilterReplyDecoder *decoder, gpointer user_data)
{
    n_rejects++;
}

static void
cb_accept (MilterReplyDecoder *decoder, gpointer user_data)
{
    n_accepts++;
}

static void
cb_discard (MilterReplyDecoder *decoder, gpointer user_data)
{
    n_discards++;
}

static void
cb_progress (MilterReplyDecoder *decoder, gpointer user_data)
{
    n_progresss++;
}

static void
cb_connection_failure (MilterReplyDecoder *decoder, gpointer user_data)
{
    n_connection_failures++;
}

static void
cb_shutdown (MilterReplyDecoder *decoder, gpointer user_data)
{
    n_shutdowns++;
}

static void
cb_skip (MilterReplyDecoder *decoder, gpointer user_data)
{
    n_skips++;
}

static void
cb_reply_code (MilterDecoder *decoder,
               guint code,
               const gchar *extended_code,
               const gchar *message, gpointer user_data)
{
    n_reply_codes++;

    reply_code = code;
    reply_extended_code = g_strdup(extended_code);
    reply_message = g_strdup(message);
}

static void
cb_delete_recipient (MilterDecoder *decoder, const gchar *recipient,
                     gpointer user_data)
{
    n_delete_recipients++;

    delete_recipient = g_strdup(recipient);
}

static void
cb_quarantine (MilterDecoder *decoder, const gchar *reason, gpointer user_data)
{
    n_quarantines++;

    quarantine = g_strdup(reason);
}

static void
cb_add_header (MilterDecoder *decoder,
               const gchar *name,
               const gchar *value,
               gpointer user_data)
{
    n_add_headers++;

    actual_header_name = g_strdup(name);
    actual_header_value = g_strdup(value);
}

static void
cb_insert_header (MilterDecoder *decoder,
                  guint32 index,
                  const gchar *name,
                  const gchar *value,
                  gpointer user_data)
{
    n_insert_headers++;

    actual_header_index = index;
    actual_header_name = g_strdup(name);
    actual_header_value = g_strdup(value);
}

static void
cb_change_header (MilterDecoder *decoder,
                  const gchar *name,
                  guint32 index,
                  const gchar *value,
                  gpointer user_data)
{
    n_change_headers++;

    actual_header_index = index;
    actual_header_name = g_strdup(name);
    actual_header_value = g_strdup(value);
}

static void
cb_delete_header (MilterDecoder *decoder,
                  const gchar *name,
                  guint32 index,
                  gpointer user_data)
{
    n_delete_headers++;

    actual_header_index = index;
    actual_header_name = g_strdup(name);
}

static void
cb_add_recipient (MilterDecoder *decoder,
                  const gchar *recipient,
                  const gchar *parameters,
                  gpointer user_data)
{
    n_add_recipients++;

    actual_recipient = g_strdup(recipient);
    actual_recipient_parameters = g_strdup(parameters);
}

static void
cb_change_from (MilterDecoder *decoder,
                const gchar *from,
                const gchar *parameters,
                gpointer user_data)
{
    n_change_froms++;

    actual_from = g_strdup(from);
    actual_from_parameters = g_strdup(parameters);
}

static void
cb_replace_body (MilterDecoder *decoder,
                 const gchar *chunk, gsize length,
                 gpointer user_data)
{
    n_bodies++;

    if (actual_body_chunk)
        g_free(actual_body_chunk);
    actual_body_chunk = g_strndup(chunk, length);
    actual_body_chunk_length = length;
}

static void
setup_signals (MilterDecoder *decoder)
{
#define CONNECT(name)                                                   \
    g_signal_connect(decoder, #name, G_CALLBACK(cb_ ## name), NULL)

    CONNECT(negotiate_reply);
    CONNECT(continue);
    CONNECT(temporary_failure);
    CONNECT(reject);
    CONNECT(accept);
    CONNECT(discard);
    CONNECT(progress);
    CONNECT(reply_code);
    CONNECT(delete_recipient);
    CONNECT(quarantine);
    CONNECT(connection_failure);
    CONNECT(shutdown);
    CONNECT(skip);
    CONNECT(add_header);
    CONNECT(insert_header);
    CONNECT(change_header);
    CONNECT(delete_header);
    CONNECT(add_recipient);
    CONNECT(change_from);
    CONNECT(replace_body);

#undef CONNECT
}

void
setup (void)
{
    decoder = milter_reply_decoder_new();
    setup_signals(decoder);

    expected_error = NULL;
    actual_error = NULL;

    n_negotiate_replys = 0;
    n_continues = 0;
    n_temporary_failures = 0;
    n_rejects = 0;
    n_accepts = 0;
    n_discards = 0;
    n_progresss = 0;
    n_reply_codes = 0;
    n_quarantines = 0;
    n_delete_recipients = 0;
    n_connection_failures = 0;
    n_shutdowns = 0;
    n_skips = 0;
    n_add_headers = 0;
    n_add_recipients = 0;
    n_change_froms = 0;
    n_bodies = 0;

    buffer = g_string_new(NULL);

    reply_code = 0;
    reply_extended_code = NULL;
    reply_message = NULL;
    delete_recipient = NULL;
    quarantine = NULL;

    actual_negotiate_option = NULL;
    actual_macros_requests = NULL;
    actual_header_index = 0;
    actual_header_name = NULL;
    actual_header_value = NULL;
    actual_recipient = NULL;
    actual_recipient_parameters = NULL;
    actual_from = NULL;
    actual_from_parameters = NULL;
    actual_body_chunk = NULL;
    actual_body_chunk_length = 0;
    actual_macro_stage = -1;
    actual_macros = NULL;

    negotiate_version = 0;
    negotiate_action = 0;
    negotiate_step = 0;
    negotiate_macros_requests = NULL;
}

void
teardown (void)
{
    if (decoder) {
        g_object_unref(decoder);
    }

    if (buffer)
        g_string_free(buffer, TRUE);

    if (expected_error)
        g_error_free(expected_error);
    if (actual_error)
        g_error_free(actual_error);

    if (reply_extended_code)
        g_free(reply_extended_code);
    if (reply_message)
        g_free(reply_message);
    if (delete_recipient)
        g_free(delete_recipient);
    if (quarantine)
        g_free(quarantine);

    if (actual_negotiate_option)
        g_object_unref(actual_negotiate_option);
    if (actual_macros_requests)
        g_object_unref(actual_macros_requests);

    if (actual_header_name)
        g_free(actual_header_name);
    if (actual_header_value)
        g_free(actual_header_value);
    if (actual_recipient)
        g_free(actual_recipient);
    if (actual_recipient_parameters)
        g_free(actual_recipient_parameters);
    if (actual_from)
        g_free(actual_from);
    if (actual_from_parameters)
        g_free(actual_from_parameters);
    if (actual_body_chunk)
        g_free(actual_body_chunk);
    if (actual_macros)
        g_free(actual_macros);

    if (negotiate_macros_requests)
        g_object_unref(negotiate_macros_requests);
}

static GError *
decode (void)
{
    guint32 content_size;
    gchar content_string[sizeof(guint32)];
    GError *error = NULL;

    content_size = g_htonl(buffer->len);
    memcpy(content_string, &content_size, sizeof(content_size));
    g_string_prepend_len(buffer, content_string, sizeof(content_size));

    milter_decoder_decode(decoder, buffer->str, buffer->len, &error);
    g_string_truncate(buffer, 0);

    return error;
}

static void
encode_negotiate (GString *buffer,
                  guint32 version,
                  MilterActionFlags action,
                  MilterStepFlags step)
{
    guint32 version_network_byte_order;
    guint32 action_network_byte_order;
    guint32 step_network_byte_order;
    gchar version_string[sizeof(guint32)];
    gchar action_string[sizeof(guint32)];
    gchar step_string[sizeof(guint32)];

    version_network_byte_order = g_htonl(version);
    action_network_byte_order = g_htonl(action);
    step_network_byte_order = g_htonl(step);

    memcpy(version_string, &version_network_byte_order, sizeof(guint32));
    memcpy(action_string, &action_network_byte_order, sizeof(guint32));
    memcpy(step_string, &step_network_byte_order, sizeof(guint32));

    g_string_append_c(buffer, 'O');
    g_string_append_len(buffer, version_string, sizeof(version_string));
    g_string_append_len(buffer, action_string, sizeof(action_string));
    g_string_append_len(buffer, step_string, sizeof(step_string));
}

static void
encode_negotiate_macros_request_without_null (GString *buffer,
                                              MilterMacroStage stage,
                                              const gchar *symbols)
{
    guint32 normalized_stage;
    gchar encoded_stage[sizeof(guint32)];

    normalized_stage = g_htonl(stage);
    memcpy(encoded_stage, &normalized_stage, sizeof(encoded_stage));
    g_string_append_len(buffer, encoded_stage, sizeof(encoded_stage));
    g_string_append(buffer, symbols);
}

static void
encode_negotiate_macros_request (GString *buffer, MilterMacroStage stage,
                                 const gchar *symbols)
{
    encode_negotiate_macros_request_without_null(buffer,
                                                 stage,
                                                 symbols);
    g_string_append_c(buffer, '\0');
}

static void
setup_negotiate_without_macros_request (void)
{
    negotiate_version = 2;
    negotiate_action =
        MILTER_ACTION_ADD_HEADERS |
        MILTER_ACTION_CHANGE_BODY |
        MILTER_ACTION_ADD_ENVELOPE_RECIPIENT |
        MILTER_ACTION_DELETE_ENVELOPE_RECIPIENT |
        MILTER_ACTION_CHANGE_HEADERS |
        MILTER_ACTION_QUARANTINE |
        MILTER_ACTION_SET_SYMBOL_LIST;
    negotiate_step =
        MILTER_STEP_NO_CONNECT |
        MILTER_STEP_NO_HELO |
        MILTER_STEP_NO_ENVELOPE_FROM |
        MILTER_STEP_NO_ENVELOPE_RECIPIENT |
        MILTER_STEP_NO_BODY |
        MILTER_STEP_NO_HEADERS |
        MILTER_STEP_NO_END_OF_HEADER;

    encode_negotiate(buffer,
                     negotiate_version,
                     negotiate_action,
                     negotiate_step);
}

static void
setup_negotiate_with_macros_request (void)
{
    negotiate_version = 2;
    negotiate_action =
        MILTER_ACTION_ADD_HEADERS |
        MILTER_ACTION_CHANGE_BODY |
        MILTER_ACTION_ADD_ENVELOPE_RECIPIENT |
        MILTER_ACTION_DELETE_ENVELOPE_RECIPIENT |
        MILTER_ACTION_CHANGE_HEADERS |
        MILTER_ACTION_QUARANTINE |
        MILTER_ACTION_SET_SYMBOL_LIST;
    negotiate_step =
        MILTER_STEP_NO_CONNECT |
        MILTER_STEP_NO_HELO |
        MILTER_STEP_NO_ENVELOPE_FROM |
        MILTER_STEP_NO_ENVELOPE_RECIPIENT |
        MILTER_STEP_NO_BODY |
        MILTER_STEP_NO_HEADERS |
        MILTER_STEP_NO_END_OF_HEADER;

    encode_negotiate(buffer,
                     negotiate_version,
                     negotiate_action,
                     negotiate_step);
    encode_negotiate_macros_request(buffer, MILTER_MACRO_STAGE_END_OF_HEADER,
                                    "{rcpt_mailer} {rcpt_host}");
    encode_negotiate_macros_request(buffer, MILTER_MACRO_STAGE_ENVELOPE_FROM,
                                    "G N U");

    negotiate_macros_requests = milter_macros_requests_new();
    milter_macros_requests_set_symbols(negotiate_macros_requests,
                                       MILTER_COMMAND_END_OF_HEADER,
                                        "{rcpt_mailer}",
                                        "{rcpt_host}",
                                        NULL);

    milter_macros_requests_set_symbols(negotiate_macros_requests,
                                       MILTER_COMMAND_ENVELOPE_FROM,
                                        "G",
                                        "N",
                                        "U",
                                        NULL);
}

void
data_decode_negotiate (void)
{
    cut_add_data("without macros request",
                 setup_negotiate_without_macros_request,
                 NULL,
                 "with macros request",
                 setup_negotiate_with_macros_request,
                 NULL);
}

void
test_decode_negotiate (gconstpointer data)
{
    GCallback setup_function = (GCallback)data;

    setup_function();
    gcut_assert_error(decode());

    cut_assert_equal_int(1, n_negotiate_replys);
    cut_assert_equal_int(negotiate_version,
                         milter_option_get_version(actual_negotiate_option));
    gcut_assert_equal_flags(MILTER_TYPE_ACTION_FLAGS,
                            negotiate_action,
                            milter_option_get_action(actual_negotiate_option));
    gcut_assert_equal_flags(MILTER_TYPE_STEP_FLAGS,
                            negotiate_step,
                            milter_option_get_step(actual_negotiate_option));
    milter_assert_equal_macros_requests(negotiate_macros_requests,
                                        actual_macros_requests);
}

void
test_decode_negotiate_without_null (void)
{
    negotiate_version = 2;
    negotiate_action =
        MILTER_ACTION_ADD_HEADERS |
        MILTER_ACTION_CHANGE_BODY |
        MILTER_ACTION_ADD_ENVELOPE_RECIPIENT |
        MILTER_ACTION_DELETE_ENVELOPE_RECIPIENT |
        MILTER_ACTION_CHANGE_HEADERS |
        MILTER_ACTION_QUARANTINE |
        MILTER_ACTION_SET_SYMBOL_LIST;
    negotiate_step =
        MILTER_STEP_NO_CONNECT |
        MILTER_STEP_NO_HELO |
        MILTER_STEP_NO_ENVELOPE_FROM |
        MILTER_STEP_NO_ENVELOPE_RECIPIENT |
        MILTER_STEP_NO_BODY |
        MILTER_STEP_NO_HEADERS |
        MILTER_STEP_NO_END_OF_HEADER;

    encode_negotiate(buffer,
                     negotiate_version,
                     negotiate_action,
                     negotiate_step);
    encode_negotiate_macros_request_without_null(buffer,
                                                 MILTER_MACRO_STAGE_END_OF_HEADER,
                                                 "{rcpt_mailer} {rcpt_host}");
    expected_error = g_error_new(MILTER_DECODER_ERROR,
                                 MILTER_DECODER_ERROR_LONG_COMMAND_LENGTH,
                                 " Symbol list isn't terminated by NULL"
                                 "on NEGOTIATE reply: <{rcpt_mailer} {rcpt_host}>");
    actual_error = decode();
}

void
test_decode_continue (void)
{
    g_string_append(buffer, "c");
    gcut_assert_error(decode());

    cut_assert_equal_int(1, n_continues);
}

void
test_decode_continue_with_garbage (void)
{
    g_string_append(buffer, "c");
    g_string_append(buffer, "XXX");

    expected_error = g_error_new(MILTER_DECODER_ERROR,
                                 MILTER_DECODER_ERROR_LONG_COMMAND_LENGTH,
                                 "needless 3 bytes were received "
                                 "on CONTINUE reply: 0x58 0x58 0x58 (XXX)");
    actual_error = decode();
    gcut_assert_equal_error(expected_error, actual_error);
}

void
test_decode_temporary_failure (void)
{
    g_string_append(buffer, "t");
    gcut_assert_error(decode());

    cut_assert_equal_int(1, n_temporary_failures);
}

void
test_decode_temporary_failure_with_garbage (void)
{
    g_string_append(buffer, "t");
    g_string_append(buffer, "XXX");

    expected_error = g_error_new(MILTER_DECODER_ERROR,
                                 MILTER_DECODER_ERROR_LONG_COMMAND_LENGTH,
                                 "needless 3 bytes were received "
                                 "on TEMPORARY FAILURE reply: 0x58 0x58 0x58 (XXX)");
    actual_error = decode();
    gcut_assert_equal_error(expected_error, actual_error);
}

void
test_decode_reject (void)
{
    g_string_append(buffer, "r");
    gcut_assert_error(decode());

    cut_assert_equal_int(1, n_rejects);
}

void
test_decode_reject_with_garbage (void)
{
    g_string_append(buffer, "r");
    g_string_append(buffer, "XXX");

    expected_error = g_error_new(MILTER_DECODER_ERROR,
                                 MILTER_DECODER_ERROR_LONG_COMMAND_LENGTH,
                                 "needless 3 bytes were received "
                                 "on REJECT reply: 0x58 0x58 0x58 (XXX)");
    actual_error = decode();
    gcut_assert_equal_error(expected_error, actual_error);
}

void
test_decode_accept (void)
{
    g_string_append(buffer, "a");
    gcut_assert_error(decode());

    cut_assert_equal_int(1, n_accepts);
}

void
test_decode_accept_with_garbage (void)
{
    g_string_append(buffer, "a");
    g_string_append(buffer, "XXX");

    expected_error = g_error_new(MILTER_DECODER_ERROR,
                                 MILTER_DECODER_ERROR_LONG_COMMAND_LENGTH,
                                 "needless 3 bytes were received "
                                 "on ACCEPT reply: 0x58 0x58 0x58 (XXX)");
    actual_error = decode();
    gcut_assert_equal_error(expected_error, actual_error);
}

void
test_decode_discard (void)
{
    g_string_append(buffer, "d");
    gcut_assert_error(decode());

    cut_assert_equal_int(1, n_discards);
}

void
test_decode_discard_with_garbage (void)
{
    g_string_append(buffer, "d");
    g_string_append(buffer, "XXX");

    expected_error = g_error_new(MILTER_DECODER_ERROR,
                                 MILTER_DECODER_ERROR_LONG_COMMAND_LENGTH,
                                 "needless 3 bytes were received "
                                 "on DISCARD reply: 0x58 0x58 0x58 (XXX)");
    actual_error = decode();
    gcut_assert_equal_error(expected_error, actual_error);
}

void
test_decode_progress (void)
{
    g_string_append(buffer, "p");
    gcut_assert_error(decode());

    cut_assert_equal_int(1, n_progresss);
}

void
test_decode_progress_with_garbage (void)
{
    g_string_append(buffer, "p");
    g_string_append(buffer, "XXX");

    expected_error = g_error_new(MILTER_DECODER_ERROR,
                                 MILTER_DECODER_ERROR_LONG_COMMAND_LENGTH,
                                 "needless 3 bytes were received "
                                 "on PROGRESS reply: 0x58 0x58 0x58 (XXX)");
    actual_error = decode();
    gcut_assert_equal_error(expected_error, actual_error);
}

void
test_decode_connection_failure (void)
{
    g_string_append(buffer, "f");
    gcut_assert_error(decode());

    cut_assert_equal_int(1, n_connection_failures);
}

void
test_decode_connection_failure_with_garbage (void)
{
    g_string_append(buffer, "f");
    g_string_append(buffer, "XXX");

    expected_error = g_error_new(MILTER_DECODER_ERROR,
                                 MILTER_DECODER_ERROR_LONG_COMMAND_LENGTH,
                                 "needless 3 bytes were received "
                                 "on CONNECTION FAILURE reply: 0x58 0x58 0x58 (XXX)");
    actual_error = decode();
    gcut_assert_equal_error(expected_error, actual_error);
}

void
test_decode_shutdown (void)
{
    g_string_append(buffer, "4");
    gcut_assert_error(decode());

    cut_assert_equal_int(1, n_shutdowns);
}

void
test_decode_shutdown_with_garbage (void)
{
    g_string_append(buffer, "4");
    g_string_append(buffer, "XXX");

    expected_error = g_error_new(MILTER_DECODER_ERROR,
                                 MILTER_DECODER_ERROR_LONG_COMMAND_LENGTH,
                                 "needless 3 bytes were received "
                                 "on SHUTDOWN reply: 0x58 0x58 0x58 (XXX)");
    actual_error = decode();
    gcut_assert_equal_error(expected_error, actual_error);
}

void
test_decode_skip (void)
{
    g_string_append(buffer, "s");
    gcut_assert_error(decode());

    cut_assert_equal_int(1, n_skips);
}

void
test_decode_skip_with_garbage (void)
{
    g_string_append(buffer, "s");
    g_string_append(buffer, "XXX");

    expected_error = g_error_new(MILTER_DECODER_ERROR,
                                 MILTER_DECODER_ERROR_LONG_COMMAND_LENGTH,
                                 "needless 3 bytes were received "
                                 "on SKIP reply: 0x58 0x58 0x58 (XXX)");
    actual_error = decode();
    gcut_assert_equal_error(expected_error, actual_error);
}

void
test_decode_reply_code (void)
{
    const gchar code[] = "554 5.7.1 1% 2%% 3%%%";

    g_string_append(buffer, "y");
    g_string_append(buffer, code);
    g_string_append_c(buffer, '\0');

    gcut_assert_error(decode());
    cut_assert_equal_int(1, n_reply_codes);
    cut_assert_equal_uint(554, reply_code);
    cut_assert_equal_string("5.7.1", reply_extended_code);
    cut_assert_equal_string("1% 2%% 3%%%", reply_message);
}

void
test_decode_reply_code_without_null (void)
{
    const gchar code[] = "554 5.7.1 1% 2%% 3%%%";

    g_string_append(buffer, "y");
    g_string_append(buffer, code);

    expected_error = g_error_new(MILTER_DECODER_ERROR,
                                 MILTER_DECODER_ERROR_MISSING_NULL,
                                 "Reply code isn't terminated by NULL "
                                 "on REPLY CODE reply: <%s>",
                                 code);
    actual_error = decode();
    gcut_assert_equal_error(expected_error, actual_error);
}

void
test_decode_delete_recipient (void)
{
    const gchar recipient[] = "example@example.com";

    g_string_append(buffer, "-");
    g_string_append(buffer, recipient);
    g_string_append_c(buffer, '\0');

    gcut_assert_error(decode());
    cut_assert_equal_int(1, n_delete_recipients);
    cut_assert_equal_string(recipient, delete_recipient);
}

void
test_decode_delete_recipient_without_null (void)
{
    const gchar recipient[] = "example@example.com";

    g_string_append(buffer, "-");
    g_string_append(buffer, recipient);

    expected_error = g_error_new(MILTER_DECODER_ERROR,
                                 MILTER_DECODER_ERROR_MISSING_NULL,
                                 "Recipient isn't terminated by NULL "
                                 "on DELETE RECIPIENT reply: <%s>",
                                 recipient);
    actual_error = decode();
    gcut_assert_equal_error(expected_error, actual_error);
}

void
test_decode_quarantine (void)
{
    const gchar reason[] = "infection";

    g_string_append(buffer, "q");
    g_string_append(buffer, reason);
    g_string_append_c(buffer, '\0');

    gcut_assert_error(decode());
    cut_assert_equal_int(1, n_quarantines);
    cut_assert_equal_string(reason, quarantine);
}

void
test_decode_quarantine_without_null (void)
{
    const gchar reason[] = "infection";

    g_string_append(buffer, "q");
    g_string_append(buffer, reason);

    expected_error = g_error_new(MILTER_DECODER_ERROR,
                                 MILTER_DECODER_ERROR_MISSING_NULL,
                                 "Reason isn't terminated by NULL "
                                 "on QUARANTINE reply: <infection>");
    actual_error = decode();
    gcut_assert_equal_error(expected_error, actual_error);
}

void
test_decode_add_header (void)
{
    const gchar name[] = "X-HEADER-NAME";
    const gchar value[] = "MilterReplyDecoder test";

    g_string_append(buffer, "h");
    g_string_append(buffer, name);
    g_string_append_c(buffer, '\0');
    g_string_append(buffer, value);
    g_string_append_c(buffer, '\0');

    gcut_assert_error(decode());

    cut_assert_equal_int(1, n_add_headers);
    cut_assert_equal_string(name, actual_header_name);
    cut_assert_equal_string(value, actual_header_value);
}

void
test_decode_add_header_without_name_null (void)
{
    g_string_append(buffer, "h");
    g_string_append(buffer, "X-HEADER-NAME");

    expected_error = g_error_new(MILTER_DECODER_ERROR,
                                 MILTER_DECODER_ERROR_MISSING_NULL,
                                 "name isn't terminated by NULL "
                                 "on header: <X-HEADER-NAME>");
    actual_error = decode();
    gcut_assert_equal_error(expected_error, actual_error);
}

void
test_decode_add_header_without_value_null (void)
{
    g_string_append(buffer, "h");
    g_string_append(buffer, "X-HEADER-NAME");
    g_string_append_c(buffer, '\0');
    g_string_append(buffer, "MilterReplyDecoder test");

    expected_error = g_error_new(MILTER_DECODER_ERROR,
                                 MILTER_DECODER_ERROR_MISSING_NULL,
                                 "value isn't terminated by NULL "
                                 "on header: <X-HEADER-NAME>: "
                                 "<MilterReplyDecoder test>");
    actual_error = decode();
    gcut_assert_equal_error(expected_error, actual_error);
}

void
test_decode_insert_header (void)
{
    const gchar name[] = "X-HEADER-NAME";
    const gchar value[] = "MilterReplyDecoder test";
    guint32 normalized_index;
    gchar encoded_index[sizeof(guint32)];

    g_string_append(buffer, "i");
    normalized_index = g_htonl(G_MAXUINT32);
    memcpy(encoded_index, &normalized_index, sizeof(encoded_index));
    g_string_append_len(buffer, encoded_index, sizeof(encoded_index));
    g_string_append(buffer, name);
    g_string_append_c(buffer, '\0');
    g_string_append(buffer, value);
    g_string_append_c(buffer, '\0');

    gcut_assert_error(decode());

    cut_assert_equal_int(1, n_insert_headers);
    cut_assert_equal_int(G_MAXUINT32, actual_header_index);
    cut_assert_equal_string(name, actual_header_name);
    cut_assert_equal_string(value, actual_header_value);
}

void
test_decode_change_header (void)
{
    const gchar name[] = "X-HEADER-NAME";
    const gchar value[] = "MilterReplyDecoder test";
    guint32 normalized_index;
    gchar encoded_index[sizeof(guint32)];

    g_string_append(buffer, "m");
    normalized_index = g_htonl(G_MAXUINT32);
    memcpy(encoded_index, &normalized_index, sizeof(encoded_index));
    g_string_append_len(buffer, encoded_index, sizeof(encoded_index));
    g_string_append(buffer, name);
    g_string_append_c(buffer, '\0');
    g_string_append(buffer, value);
    g_string_append_c(buffer, '\0');

    gcut_assert_error(decode());

    cut_assert_equal_int(1, n_change_headers);
    cut_assert_equal_int(G_MAXUINT32, actual_header_index);
    cut_assert_equal_string(name, actual_header_name);
    cut_assert_equal_string(value, actual_header_value);
}

void
test_decode_delete_header (void)
{
    const gchar name[] = "X-HEADER-NAME";
    guint32 normalized_index;
    gchar encoded_index[sizeof(guint32)];

    g_string_append(buffer, "m");
    normalized_index = g_htonl(G_MAXUINT32);
    memcpy(encoded_index, &normalized_index, sizeof(encoded_index));
    g_string_append_len(buffer, encoded_index, sizeof(encoded_index));
    g_string_append(buffer, name);
    g_string_append_c(buffer, '\0');
    g_string_append_c(buffer, '\0');

    gcut_assert_error(decode());

    cut_assert_equal_int(1, n_delete_headers);
    cut_assert_equal_int(G_MAXUINT32, actual_header_index);
    cut_assert_equal_string(name, actual_header_name);
    cut_assert_null(actual_header_value);
}

void
test_decode_add_recipient (void)
{
    const gchar recipient[] = "example@example.com";

    g_string_append(buffer, "+");
    g_string_append(buffer, recipient);
    g_string_append_c(buffer, '\0');

    gcut_assert_error(decode());

    cut_assert_equal_int(1, n_add_recipients);
    cut_assert_equal_string(recipient, actual_recipient);
    cut_assert_null(actual_recipient_parameters);
}

void
test_decode_add_recipient_without_null (void)
{
    const gchar recipient[] = "example@example.com";

    g_string_append(buffer, "+");
    g_string_append(buffer, recipient);

    expected_error = g_error_new(MILTER_DECODER_ERROR,
                                 MILTER_DECODER_ERROR_MISSING_NULL,
                                 "Recipient isn't terminated by NULL "
                                 "on ADD RECIPIENT reply: "
                                 "<example@example.com>");
    actual_error = decode();
    gcut_assert_equal_error(expected_error, actual_error);
}

void
test_decode_add_recipient_with_parameters (void)
{
    const gchar recipient[] = "example@example.com";
    const gchar parameters[] = "he is your boss";

    g_string_append(buffer, "2");
    g_string_append(buffer, recipient);
    g_string_append_c(buffer, '\0');
    g_string_append(buffer, parameters);
    g_string_append_c(buffer, '\0');

    gcut_assert_error(decode());

    cut_assert_equal_int(1, n_add_recipients);
    cut_assert_equal_string(recipient, actual_recipient);
    cut_assert_equal_string(parameters, actual_recipient_parameters);
}

void
test_decode_add_recipient_with_parameters_without_recipient_null (void)
{
    const gchar recipient[] = "example@example.com";

    g_string_append(buffer, "2");
    g_string_append(buffer, recipient);

    expected_error = g_error_new(MILTER_DECODER_ERROR,
                                 MILTER_DECODER_ERROR_MISSING_NULL,
                                 "Recipient isn't terminated by NULL "
                                 "on ADD RECIPIENT WITH PARAMETERS reply: "
                                 "<example@example.com>");
    actual_error = decode();
    gcut_assert_equal_error(expected_error, actual_error);
}

void
test_decode_add_recipient_with_parameters_without_parameters_null (void)
{
    const gchar recipient[] = "example@example.com";
    const gchar parameters[] = "he is your boss";

    g_string_append(buffer, "2");
    g_string_append(buffer, recipient);
    g_string_append_c(buffer, '\0');
    g_string_append(buffer, parameters);

    expected_error = g_error_new(MILTER_DECODER_ERROR,
                                 MILTER_DECODER_ERROR_MISSING_NULL,
                                 "Parameter isn't terminated by NULL "
                                 "on ADD RECIPIENT WITH PARAMETERS reply: "
                                 "<example@example.com>: "
                                 "<he is your boss>");
    actual_error = decode();
    gcut_assert_equal_error(expected_error, actual_error);
}

void
test_decode_change_from_with_parameters (void)
{
    const gchar from[] = "example@example.com";
    const gchar parameters[] = "he is your boss";

    g_string_append(buffer, "e");
    g_string_append(buffer, from);
    g_string_append_c(buffer, '\0');
    g_string_append(buffer, parameters);
    g_string_append_c(buffer, '\0');

    gcut_assert_error(decode());

    cut_assert_equal_int(1, n_change_froms);
    cut_assert_equal_string(from, actual_from);
    cut_assert_equal_string(parameters, actual_from_parameters);
}

void
test_decode_change_from_with_parameters_without_from_null (void)
{
    const gchar from[] = "example@example.com";

    g_string_append(buffer, "e");
    g_string_append(buffer, from);

    expected_error = g_error_new(MILTER_DECODER_ERROR,
                                 MILTER_DECODER_ERROR_MISSING_NULL,
                                 "From isn't terminated by NULL "
                                 "on CHANGE FROM reply: "
                                 "<example@example.com>");
    actual_error = decode();
    gcut_assert_equal_error(expected_error, actual_error);
}

void
test_decode_change_from_with_parameters_without_parameters_null (void)
{
    const gchar from[] = "example@example.com";
    const gchar parameters[] = "he is your boss";

    g_string_append(buffer, "e");
    g_string_append(buffer, from);
    g_string_append_c(buffer, '\0');
    g_string_append(buffer, parameters);

    expected_error = g_error_new(MILTER_DECODER_ERROR,
                                 MILTER_DECODER_ERROR_MISSING_NULL,
                                 "Parameter isn't terminated by NULL "
                                 "on CHANGE FROM reply: "
                                 "<example@example.com>: "
                                 "<he is your boss>");
    actual_error = decode();
    gcut_assert_equal_error(expected_error, actual_error);
}

void
test_decode_replace_body (void)
{
    const gchar body[] =
        "La de da de da 1.\n"
        "La de da de da 2.\n"
        "La de da de da 3.\n"
        "La de da de da 4.";

    g_string_append(buffer, "b");
    g_string_append(buffer, body);
    gcut_assert_error(decode());

    cut_assert_equal_int(1, n_bodies);
    cut_assert_equal_string(body, actual_body_chunk);
    cut_assert_equal_uint(strlen(body), actual_body_chunk_length);
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
