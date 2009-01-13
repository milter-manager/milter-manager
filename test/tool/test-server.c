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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif /* HAVE_CONFIG_H */

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


void data_result (void);
void test_result (gconstpointer data);

void data_option (void);
void test_option (gconstpointer data);

void data_end_of_message_action (void);
void test_end_of_message_action (gconstpointer data);

void data_macro (void);
void test_macro (gconstpointer data);

void data_invalid_spec (void);
void test_invalid_spec (gconstpointer data);

static MilterTestClient *client;
static MilterDecoder *decoder;
static MilterReplyEncoder *encoder;
static GCutEgg *server;
static GArray *command;
static gchar *server_path;
static gchar *fixtures_path;

static gsize packet_size;
static gchar *packet;
static GError *error;
static gboolean reaped;
static gint exit_status;
static GString *output_string;
static GString *error_string;
static MilterStatus actual_status;

static gint n_negotiates;
static gint n_define_macros;
static gint n_connects;
static gint n_helos;
static gint n_envelope_froms;
static gint n_envelope_recipients;
static gint n_datas;
static gint n_headers;
static gint n_end_of_headers;
static gint n_bodies;
static gint n_end_of_messages;
static gint n_aborts;
static gint n_quits;
static gint n_unknowns;

static GHashTable *actual_defined_macros;
static gchar *actual_connect_host;
static gchar *actual_connect_address;
static gchar *actual_helo_fqdn;
static gchar *actual_envelope_from;
static GList *actual_recipients;
static GString *actual_body_string;
static MilterHeaders *actual_headers;

typedef void (*EndOfMessageActionFunc) (void);

typedef struct _TestData
{
    GHashTable *replies;
    GHashTable *expected_command_receives;
    MilterStatus expected_status;
    MilterActionFlags actions;
    MilterStepFlags steps;
    EndOfMessageActionFunc action_func;
    MilterMacrosRequests *macros_requests;
} TestData;

void
cut_startup (void)
{
    fixtures_path = g_build_filename(milter_test_get_base_dir(),
                                     "tool",
                                     "fixtures",
                                     NULL);
}

void
cut_shutdown (void)
{
    if (fixtures_path)
        g_free(fixtures_path);
}

static void
packet_free (void)
{
    if (packet)
        g_free(packet);
    packet = NULL;
}

static void
write_data (gchar *data, gsize data_size)
{
    milter_test_client_write(client, data, data_size);
    packet_free();
}

static void
cb_negotiate (MilterCommandDecoder *decoder, MilterOption *option,
              TestData *test_data)
{
    MilterOption *reply_option;

    reply_option = milter_option_new(2, test_data->actions, test_data->steps);
    milter_reply_encoder_encode_negotiate(encoder,
                                          &packet, &packet_size,
                                          reply_option,
                                          test_data->macros_requests);
    write_data(packet, packet_size);
    g_object_unref(reply_option);

    n_negotiates++;
}

static void
cb_define_macro (MilterDecoder *decoder, MilterCommand context,
                 GHashTable *macros, gpointer user_data)
{
    g_hash_table_replace(actual_defined_macros,
                         GINT_TO_POINTER(context),
                         g_hash_table_ref(macros));

    n_define_macros++;
}

static void
send_reply (MilterCommand command, TestData *test_data)
{
    MilterReply reply;
    gpointer hash_value;

    hash_value = g_hash_table_lookup(test_data->replies, GINT_TO_POINTER(command));
    if (!hash_value)
        reply = MILTER_REPLY_CONTINUE;
    else
        reply = GPOINTER_TO_INT(hash_value);

    switch (reply) {
      case MILTER_REPLY_QUARANTINE:
        milter_reply_encoder_encode_quarantine(encoder, &packet, &packet_size, "virus");
        break;
      case MILTER_REPLY_ACCEPT:
        milter_reply_encoder_encode_accept(encoder, &packet, &packet_size);
        break;
      case MILTER_REPLY_TEMPORARY_FAILURE:
        milter_reply_encoder_encode_temporary_failure(encoder, &packet, &packet_size);
        break;
      case MILTER_REPLY_REJECT:
        milter_reply_encoder_encode_reject(encoder, &packet, &packet_size);
        break;
      case MILTER_REPLY_DISCARD:
        milter_reply_encoder_encode_discard(encoder, &packet, &packet_size);
        break;
      case MILTER_REPLY_REPLY_CODE:
        milter_reply_encoder_encode_reply_code(encoder, &packet, &packet_size,
                                               "554 5.7.1 1% 2%% 3%%%");
        break;
      case MILTER_REPLY_SHUTDOWN:
        milter_reply_encoder_encode_shutdown(encoder, &packet, &packet_size);
        break;
      case MILTER_REPLY_CONNECTION_FAILURE:
        milter_reply_encoder_encode_connection_failure(encoder, &packet, &packet_size);
        break;
      case MILTER_REPLY_CONTINUE:
      default:
        milter_reply_encoder_encode_continue(encoder, &packet, &packet_size);
        break;
    }
    write_data(packet, packet_size);
}

static void
cb_connect (MilterCommandDecoder *decoder, const gchar *host_name,
            const struct sockaddr *address, socklen_t address_size,
            TestData *test_data)
{
    send_reply(MILTER_COMMAND_CONNECT, test_data);

    n_connects++;
    actual_connect_host = g_strdup(host_name);
    actual_connect_address = milter_connection_address_to_spec(address);
}

static void
cb_helo (MilterCommandDecoder *decoder, const gchar *fqdn, TestData *test_data)
{
    send_reply(MILTER_COMMAND_HELO, test_data);

    n_helos++;
    actual_helo_fqdn = g_strdup(fqdn);
}

static void
cb_envelope_from (MilterCommandDecoder *decoder, const gchar *from,
                  TestData *test_data)
{
    send_reply(MILTER_COMMAND_ENVELOPE_FROM, test_data);

    n_envelope_froms++;
    actual_envelope_from = g_strdup(from);
}

static void
cb_envelope_recipient (MilterCommandDecoder *decoder, const gchar *to,
                       TestData *test_data)
{
    send_reply(MILTER_COMMAND_ENVELOPE_RECIPIENT, test_data);

    n_envelope_recipients++;
    actual_recipients = g_list_append(actual_recipients, g_strdup(to));
}

static void
cb_data (MilterCommandDecoder *decoder, TestData *test_data)
{
    send_reply(MILTER_COMMAND_DATA, test_data);

    n_datas++;
}

static void
cb_header (MilterCommandDecoder *decoder, const gchar *name, const gchar *value,
           TestData *test_data)
{
    send_reply(MILTER_COMMAND_HEADER, test_data);

    n_headers++;
    milter_headers_add_header(actual_headers, name, value);
}

static void
cb_end_of_header (MilterCommandDecoder *decoder, TestData *test_data)
{
    send_reply(MILTER_COMMAND_END_OF_HEADER, test_data);

    n_end_of_headers++;
}

static void
cb_body (MilterCommandDecoder *decoder, const gchar *chunk, gsize size,
         TestData *test_data)
{
    send_reply(MILTER_COMMAND_BODY, test_data);

    n_bodies++;
    g_string_append_len(actual_body_string, chunk, size);
}

static void
cb_end_of_message (MilterCommandDecoder *decoder, const gchar *chunk, gsize size,
                   TestData *test_data)
{
    if (test_data->action_func)
        test_data->action_func();

    send_reply(MILTER_COMMAND_END_OF_MESSAGE, test_data);

    n_end_of_messages++;
}

static void
cb_abort (MilterCommandDecoder *decoder, TestData *test_data)
{
    n_aborts++;
}

static void
cb_quit (MilterCommandDecoder *decoder, TestData *test_data)
{
    n_quits++;
}

static void
cb_unknown (MilterCommandDecoder *decoder, gchar *command, TestData *test_data)
{
    send_reply(MILTER_COMMAND_UNKNOWN, test_data);

    n_unknowns++;
}

static void
setup_command_signals (MilterDecoder *decoder, TestData *test_data)
{
#define CONNECT(name)                                                   \
    g_signal_connect(decoder, #name, G_CALLBACK(cb_##name), test_data)

    CONNECT(negotiate);
    CONNECT(define_macro);
    CONNECT(connect);
    CONNECT(helo);
    CONNECT(envelope_from);
    CONNECT(envelope_recipient);
    CONNECT(data);
    CONNECT(header);
    CONNECT(end_of_header);
    CONNECT(body);
    CONNECT(end_of_message);
    CONNECT(abort);
    CONNECT(quit);
    CONNECT(unknown);

#undef CONNECT
}

static void
setup_server_command (void)
{
    command = g_array_new(TRUE, TRUE, sizeof(gchar *));
    server_path = g_build_filename(milter_test_get_base_dir(),
                                   "..",
                                   "tool",
                                   "milter-test-server",
                                   NULL);
    g_array_append_val(command, server_path);
}

void
setup (void)
{
    client = NULL;
    server = NULL;
    decoder = milter_command_decoder_new();
    encoder = MILTER_REPLY_ENCODER(milter_reply_encoder_new());

    n_negotiates = 0;
    n_connects = 0;
    n_helos = 0;
    n_envelope_froms = 0;
    n_envelope_recipients = 0;
    n_datas = 0;
    n_headers = 0;
    n_end_of_headers = 0;
    n_bodies = 0;
    n_end_of_messages = 0;
    n_aborts = 0;
    n_quits = 0;
    n_unknowns = 0;

    setup_server_command();

    packet = NULL;
    error = NULL;
    reaped = FALSE;
    output_string = g_string_new(NULL);
    error_string = g_string_new(NULL);

    actual_status = MILTER_STATUS_DEFAULT;
    actual_connect_host = NULL;
    actual_connect_address = NULL;
    actual_helo_fqdn = NULL;
    actual_envelope_from = NULL;
    actual_recipients = NULL;
    actual_headers = milter_headers_new();
    actual_body_string = g_string_new(NULL);
    actual_defined_macros = g_hash_table_new_full(g_direct_hash, g_direct_equal,
                                                  NULL,
                                                  (GDestroyNotify)g_hash_table_unref);
    exit_status = EXIT_FAILURE;
}

void
teardown (void)
{
    if (client)
        g_object_unref(client);
    if (decoder)
        g_object_unref(decoder);
    if (encoder)
        g_object_unref(encoder);
    if (command)
        g_array_free(command, TRUE);
    packet_free();

    if (output_string)
        g_string_free(output_string, TRUE);
    if (error_string)
        g_string_free(error_string, TRUE);
    if (error)
        g_error_free(error);

    if (actual_connect_host)
        g_free(actual_connect_host);
    if (actual_connect_address)
        g_free(actual_connect_address);
    if (actual_helo_fqdn)
        g_free(actual_helo_fqdn);
    if (actual_envelope_from)
        g_free(actual_envelope_from);
    if (actual_recipients) {
        g_list_foreach(actual_recipients, (GFunc)g_free, NULL);
        g_list_free(actual_recipients);
    }
    if (actual_headers)
        g_object_unref(actual_headers);
    if (actual_body_string)
        g_string_free(actual_body_string, TRUE);
    if (actual_defined_macros)
        g_hash_table_unref(actual_defined_macros);
}

static void
append_argument (GArray *array, const gchar *argument)
{
    gchar *dupped_argument;

    dupped_argument = g_strdup(argument);
    g_array_append_val(array, dupped_argument);
}

static void
cb_output_received (GCutEgg *egg, const gchar *chunk, gsize size,
                    gpointer user_data)
{
    g_string_append_len(output_string, chunk, size);
}

static void
cb_error_received (GCutEgg *egg, const gchar *chunk, gsize size,
                   gpointer user_data)
{
    g_string_append_len(error_string, chunk, size);
}

static void
cb_reaped (GCutEgg *egg, gint status, gpointer user_data)
{
    reaped = TRUE;
    exit_status = status;
}

static void
setup_server (const gchar *spec, const gchar *option_string)
{
    append_argument(command, "-s");
    g_array_append_val(command, spec);

    if (option_string) {
        gchar **options;
        gint i;

        options = g_strsplit(option_string, " ", -1);
        for (i = 0; i < g_strv_length(options); i++) {
            g_array_append_val(command, options[i]);
        }
        g_free(options);
    }

    server = gcut_egg_new_array(command);

#define CONNECT(name)                                                   \
    g_signal_connect(server, #name, G_CALLBACK(cb_ ## name), NULL)

    CONNECT(output_received);
    CONNECT(error_received);
    CONNECT(reaped);
#undef CONNECT
}

static void
setup_test_client (const gchar *spec, TestData *test_data)
{
    cut_trace(client = milter_test_client_new(spec, decoder));
    setup_command_signals(decoder, test_data);
}

static void
wait_for_reaping (gboolean check_success)
{
    GError *error = NULL;
    gchar *status_string;
    gchar *begin_pos, *end_pos;

    while (!reaped)
        g_main_context_iteration(NULL, TRUE);

    if (!check_success)
        return;

    cut_assert_match("The message was '(.*)'.", output_string->str);
    begin_pos = output_string->str + strlen("The message was '");
    end_pos = g_strrstr(output_string->str, "'.");
    status_string = g_strndup(begin_pos, end_pos - begin_pos);
    cut_take_string(status_string);

    if (g_str_equal(status_string, "pass"))
        status_string = "not-change";
    actual_status = gcut_enum_parse(MILTER_TYPE_STATUS, status_string, &error);

    gcut_assert_error(error);
}

static TestData *
result_test_data_new (MilterActionFlags actions,
                      MilterStepFlags steps,
                      MilterStatus status,
                      GHashTable *replies,
                      GHashTable *expected_command_receives,
                      MilterMacrosRequests *macros_requests,
                      EndOfMessageActionFunc action_func)
{
    TestData *test_data;

    test_data = g_new0(TestData, 1);
    test_data->actions = actions;
    test_data->steps = steps;
    test_data->expected_status = status;
    if (replies)
        test_data->replies = replies;
    else
        test_data->replies = g_hash_table_new(g_direct_hash, g_direct_equal);
    test_data->expected_command_receives = expected_command_receives;
    test_data->macros_requests = macros_requests;
    test_data->action_func = action_func;

    return test_data;
}

static void
result_test_data_free (TestData *test_data)
{
    g_hash_table_unref(test_data->replies);
    if (test_data->expected_command_receives)
        g_hash_table_unref(test_data->expected_command_receives);
    if (test_data->macros_requests)
        g_object_unref(test_data->macros_requests);
    g_free(test_data);
}

static GHashTable *
create_replies (MilterCommand first_command, ...)
{
    va_list var_args;
    MilterCommand command;
    GHashTable *replies;

    replies = g_hash_table_new(g_direct_hash, g_direct_equal);

    va_start(var_args, first_command);
    command = first_command;
    while (command) {
        MilterReply reply;
        reply = va_arg(var_args, gint);
        g_hash_table_insert(replies,
                            GINT_TO_POINTER(command), GINT_TO_POINTER(reply));
        command = va_arg(var_args, gint);
    }
    va_end(var_args);

    return replies;
}

static GHashTable *
create_expected_command_receives (MilterCommand first_command, ...)
{
    va_list var_args;
    MilterCommand command;
    GHashTable *expected_command_receives;

    expected_command_receives = g_hash_table_new(g_direct_hash, g_direct_equal);

    va_start(var_args, first_command);
    command = first_command;
    while (command) {
        gint expected_num;
        expected_num = va_arg(var_args, gint);
        g_hash_table_insert(expected_command_receives,
                            GINT_TO_POINTER(command), GINT_TO_POINTER(expected_num));
        command = va_arg(var_args, gint);
    }
    va_end(var_args);

    return expected_command_receives;
}

void
data_result (void)
{
    cut_add_data("all continue",
                 result_test_data_new(MILTER_ACTION_NONE,
                                      MILTER_STEP_NONE,
                                      MILTER_STATUS_NOT_CHANGE,
                                      NULL,
                                      NULL,
                                      NULL,
                                      NULL),
                 result_test_data_free,
                 "no helo",
                 result_test_data_new(MILTER_ACTION_NONE,
                                      MILTER_STEP_NO_HELO,
                                      MILTER_STATUS_NOT_CHANGE,
                                      NULL,
                                      create_expected_command_receives(MILTER_COMMAND_HELO, 0,
                                                                       NULL),
                                      NULL,
                                      NULL),
                 result_test_data_free,
                 "no connect",
                 result_test_data_new(MILTER_ACTION_NONE,
                                      MILTER_STEP_NO_CONNECT,
                                      MILTER_STATUS_NOT_CHANGE,
                                      NULL,
                                      create_expected_command_receives(MILTER_COMMAND_CONNECT, 0,
                                                                       NULL),
                                      NULL,
                                      NULL),
                 result_test_data_free,
                 "no envelope-from",
                 result_test_data_new(MILTER_ACTION_NONE,
                                      MILTER_STEP_NO_ENVELOPE_FROM,
                                      MILTER_STATUS_NOT_CHANGE,
                                      NULL,
                                      create_expected_command_receives(MILTER_COMMAND_ENVELOPE_FROM, 0,
                                                                       NULL),
                                      NULL,
                                      NULL),
                 result_test_data_free,
                 "no envelope-recipient",
                 result_test_data_new(MILTER_ACTION_NONE,
                                      MILTER_STEP_NO_ENVELOPE_RECIPIENT,
                                      MILTER_STATUS_NOT_CHANGE,
                                      NULL,
                                      create_expected_command_receives(MILTER_COMMAND_ENVELOPE_RECIPIENT, 0,
                                                                       NULL),
                                      NULL,
                                      NULL),
                 result_test_data_free,
                 "no unknown",
                 result_test_data_new(MILTER_ACTION_NONE,
                                      MILTER_STEP_NO_UNKNOWN,
                                      MILTER_STATUS_NOT_CHANGE,
                                      NULL,
                                      create_expected_command_receives(MILTER_COMMAND_UNKNOWN, 0,
                                                                       NULL),
                                      NULL,
                                      NULL),
                 result_test_data_free,
                 "no header",
                 result_test_data_new(MILTER_ACTION_NONE,
                                      MILTER_STEP_NO_HEADERS,
                                      MILTER_STATUS_NOT_CHANGE,
                                      NULL,
                                      create_expected_command_receives(MILTER_COMMAND_HEADER, 0,
                                                                       NULL),
                                      NULL,
                                      NULL),
                 result_test_data_free,
                 "no end-of-header",
                 result_test_data_new(MILTER_ACTION_NONE,
                                      MILTER_STEP_NO_END_OF_HEADER,
                                      MILTER_STATUS_NOT_CHANGE,
                                      NULL,
                                      create_expected_command_receives(MILTER_COMMAND_END_OF_HEADER, 0,
                                                                       NULL),
                                      NULL,
                                      NULL),
                 result_test_data_free,
                 "no data",
                 result_test_data_new(MILTER_ACTION_NONE,
                                      MILTER_STEP_NO_DATA,
                                      MILTER_STATUS_NOT_CHANGE,
                                      NULL,
                                      create_expected_command_receives(MILTER_COMMAND_DATA, 0,
                                                                       NULL),
                                      NULL,
                                      NULL),
                 result_test_data_free,
                 "no body",
                 result_test_data_new(MILTER_ACTION_NONE,
                                      MILTER_STEP_NO_BODY,
                                      MILTER_STATUS_NOT_CHANGE,
                                      NULL,
                                      create_expected_command_receives(MILTER_COMMAND_BODY, 0,
                                                                       NULL),
                                      NULL,
                                      NULL),
                 result_test_data_free,
                 "helo accept",
                 result_test_data_new(MILTER_ACTION_NONE,
                                      MILTER_STEP_NONE,
                                      MILTER_STATUS_ACCEPT,
                                      create_replies(MILTER_COMMAND_HELO,
                                                     MILTER_REPLY_ACCEPT,
                                                     NULL),
                                      create_expected_command_receives(MILTER_COMMAND_HELO, 1,
                                                                       MILTER_COMMAND_ENVELOPE_FROM, 0,
                                                                       NULL),
                                      NULL,
                                      NULL),
                 result_test_data_free,
                 "envelope-from discard",
                 result_test_data_new(MILTER_ACTION_NONE,
                                      MILTER_STEP_NONE,
                                      MILTER_STATUS_DISCARD,
                                      create_replies(MILTER_COMMAND_ENVELOPE_FROM,
                                                     MILTER_REPLY_DISCARD,
                                                     NULL),
                                      create_expected_command_receives(MILTER_COMMAND_ENVELOPE_FROM, 1,
                                                                       MILTER_COMMAND_ENVELOPE_RECIPIENT, 0,
                                                                       NULL),
                                      NULL,
                                      NULL),
                 result_test_data_free,
                 "data temporary-failure",
                 result_test_data_new(MILTER_ACTION_NONE,
                                      MILTER_STEP_NONE,
                                      MILTER_STATUS_TEMPORARY_FAILURE,
                                      create_replies(MILTER_COMMAND_DATA,
                                                     MILTER_REPLY_TEMPORARY_FAILURE,
                                                     NULL),
                                      create_expected_command_receives(MILTER_COMMAND_DATA, 1,
                                                                       MILTER_COMMAND_HEADER, 0,
                                                                       NULL),
                                      NULL,
                                      NULL),
                 result_test_data_free,
                 "body quarantine",
                 result_test_data_new(MILTER_ACTION_NONE,
                                      MILTER_STEP_NONE,
                                      MILTER_STATUS_NOT_CHANGE,
                                      create_replies(MILTER_COMMAND_BODY,
                                                     MILTER_REPLY_QUARANTINE,
                                                     NULL),
                                      create_expected_command_receives(MILTER_COMMAND_BODY, 1,
                                                                       MILTER_COMMAND_END_OF_MESSAGE, 0,
                                                                       NULL),
                                      NULL,
                                      NULL),
                 result_test_data_free,
                 "connect reject",
                 result_test_data_new(MILTER_ACTION_NONE,
                                      MILTER_STEP_NONE,
                                      MILTER_STATUS_REJECT,
                                      create_replies(MILTER_COMMAND_CONNECT,
                                                     MILTER_REPLY_REJECT,
                                                     NULL),
                                      create_expected_command_receives(MILTER_COMMAND_CONNECT, 1,
                                                                       MILTER_COMMAND_HELO, 0,
                                                                       NULL),
                                      NULL,
                                      NULL),
                 result_test_data_free,
                 "envelope-recipient reject",
                 result_test_data_new(MILTER_ACTION_NONE,
                                      MILTER_STEP_NONE,
                                      MILTER_STATUS_NOT_CHANGE,
                                      create_replies(MILTER_COMMAND_ENVELOPE_RECIPIENT,
                                                     MILTER_REPLY_REJECT,
                                                     NULL),
                                      create_expected_command_receives(MILTER_COMMAND_ENVELOPE_RECIPIENT, 1,
                                                                       MILTER_COMMAND_DATA, 1,
                                                                       NULL),
                                      NULL,
                                      NULL),
                 result_test_data_free,
                 "envelope-recipient temporary-failure",
                 result_test_data_new(MILTER_ACTION_NONE,
                                      MILTER_STEP_NONE,
                                      MILTER_STATUS_NOT_CHANGE,
                                      create_replies(MILTER_COMMAND_ENVELOPE_RECIPIENT,
                                                     MILTER_REPLY_TEMPORARY_FAILURE,
                                                     NULL),
                                      create_expected_command_receives(MILTER_COMMAND_ENVELOPE_RECIPIENT, 1,
                                                                       MILTER_COMMAND_DATA, 1,
                                                                       NULL),
                                      NULL,
                                      NULL),
                 result_test_data_free,
                 "reply-code",
                 result_test_data_new(MILTER_ACTION_NONE,
                                      MILTER_STEP_NONE,
                                      MILTER_STATUS_NOT_CHANGE,
                                      create_replies(MILTER_COMMAND_HELO,
                                                     MILTER_REPLY_REPLY_CODE,
                                                     NULL),
                                      create_expected_command_receives(MILTER_COMMAND_HELO, 1,
                                                                       MILTER_COMMAND_ENVELOPE_FROM, 1,
                                                                       NULL),
                                      NULL,
                                      NULL),
                 result_test_data_free,
                 "connection-faliure",
                 result_test_data_new(MILTER_ACTION_NONE,
                                      MILTER_STEP_NONE,
                                      MILTER_STATUS_NOT_CHANGE,
                                      create_replies(MILTER_COMMAND_HELO,
                                                     MILTER_REPLY_CONNECTION_FAILURE,
                                                     NULL),
                                      create_expected_command_receives(MILTER_COMMAND_HELO, 1,
                                                                       MILTER_COMMAND_ENVELOPE_FROM, 0,
                                                                       NULL),
                                      NULL,
                                      NULL),
                 result_test_data_free,
                 "shutdown",
                 result_test_data_new(MILTER_ACTION_NONE,
                                      MILTER_STEP_NONE,
                                      MILTER_STATUS_NOT_CHANGE,
                                      create_replies(MILTER_COMMAND_HELO,
                                                     MILTER_REPLY_SHUTDOWN,
                                                     NULL),
                                      create_expected_command_receives(MILTER_COMMAND_HELO, 1,
                                                                       MILTER_COMMAND_ENVELOPE_FROM, 0,
                                                                       NULL),
                                      NULL,
                                      NULL),
                 result_test_data_free);
}

static gint
get_n_command_received (MilterCommand command)
{
    switch (command) {
      case MILTER_COMMAND_NEGOTIATE:
        return n_negotiates;
        break;
      case MILTER_COMMAND_HELO:
        return n_helos;
        break;
      case MILTER_COMMAND_CONNECT:
        return n_connects;
        break;
      case MILTER_COMMAND_ENVELOPE_FROM:
        return n_envelope_froms;
        break;
      case MILTER_COMMAND_ENVELOPE_RECIPIENT:
        return n_envelope_recipients;
        break;
      case MILTER_COMMAND_UNKNOWN:
        return n_unknowns;
        break;
      case MILTER_COMMAND_DATA:
        return n_datas;
        break;
      case MILTER_COMMAND_HEADER:
        return n_headers;
        break;
      case MILTER_COMMAND_END_OF_HEADER:
        return n_end_of_headers;
        break;
      case MILTER_COMMAND_BODY:
        return n_bodies;
        break;
      case MILTER_COMMAND_END_OF_MESSAGE:
        return n_end_of_messages;
        break;
      default:
        return -1;
        break;
    }
}

static void
assert_each_command_received (gpointer key, gpointer value, gpointer user_data)
{
    MilterCommand command = GPOINTER_TO_INT(key);
    gint n_expected = GPOINTER_TO_INT(value);
    gint n_actual = get_n_command_received(command);

    cut_assert_equal_int(n_expected, n_actual,
                         "%s should be received %d times but %d times received",
                         gcut_enum_inspect(MILTER_TYPE_COMMAND, command),
                         n_expected, n_actual);
}

static void
milter_assert_equal_n_received (GHashTable *expected_command_receives)
{
    if (!expected_command_receives)
        return;
    cut_trace(g_hash_table_foreach(expected_command_receives,
                                   assert_each_command_received, NULL));
}

void
test_result (gconstpointer data)
{
    TestData *test_data = (TestData*)data;

    setup_test_client("inet:9999@localhost", test_data);
    setup_server("inet:9999@localhost", NULL);

    gcut_egg_hatch(server, &error);
    gcut_assert_error(error);

    wait_for_reaping(TRUE);
    cut_assert_equal_int(EXIT_SUCCESS, exit_status);

    gcut_assert_equal_enum(MILTER_TYPE_STATUS,
                           actual_status, test_data->expected_status);

    cut_trace(milter_assert_equal_n_received(test_data->expected_command_receives));
}

typedef void (*AssertOptionValueFunc) (void);

typedef struct _OptionTestData
{
    TestData *test_data;
    gchar *option_string;
    AssertOptionValueFunc assert_func;
} OptionTestData;

static OptionTestData *
option_test_data_new (const gchar *option_string,
                      AssertOptionValueFunc assert_func)
{
    OptionTestData *option_test_data;

    option_test_data = g_new0(OptionTestData, 1);
    option_test_data->test_data = result_test_data_new(MILTER_ACTION_NONE,
                                                       MILTER_STEP_NONE,
                                                       MILTER_STATUS_NOT_CHANGE,
                                                       NULL,
                                                       NULL,
                                                       NULL,
                                                       NULL);
    option_test_data->option_string = g_strdup(option_string);
    option_test_data->assert_func = assert_func;

    return option_test_data;
}

static void
option_test_data_free (OptionTestData *option_test_data)
{
    g_free(option_test_data->option_string);
    result_test_data_free(option_test_data->test_data);
    g_free(option_test_data);
}

static void
option_test_assert_connect_host (void)
{
    cut_assert_equal_string("test-host", actual_connect_host);
    cut_assert_equal_string("inet:50443@192.168.123.123",
                            actual_connect_address);
}

static void
option_test_assert_connect_address (void)
{
    cut_assert_equal_string("mx.local.net", actual_connect_host);
    cut_assert_equal_string("inet:12345@192.168.1.1", actual_connect_address);
}

static void
option_test_assert_connect (void)
{
    cut_assert_equal_string("test-host", actual_connect_host);
    cut_assert_equal_string("inet:12345@192.168.1.1", actual_connect_address);
}

static void
option_test_assert_helo_fqdn (void)
{
    cut_assert_equal_string("test-host", actual_helo_fqdn);
}

static void
option_test_assert_from (void)
{
    cut_assert_equal_string("from@example.com", actual_envelope_from);
}

static void
option_test_assert_recipients (void)
{
    cut_assert_equal_int(3, g_list_length(actual_recipients));
    cut_assert_equal_string("recipient1@example.com", actual_recipients->data);
    cut_assert_equal_string("recipient2@example.com", g_list_next(actual_recipients)->data);
    cut_assert_equal_string("recipient3@example.com", g_list_last(actual_recipients)->data);
}

static void
option_test_assert_headers (void)
{
    MilterHeader expected1 = {"X-TestHeader1", "TestHeader1Value"};
    MilterHeader expected2 = {"X-TestHeader2", "TestHeader2Value"};
    MilterHeader expected3 = {"X-TestHeader3", "TestHeader3Value"};

    /* 5 includes default "From" and "To" headers.*/
    cut_assert_equal_uint(5, milter_headers_length(actual_headers));
    milter_assert_equal_header(&expected1,
                               milter_headers_get_nth_header(actual_headers, 1));
    milter_assert_equal_header(&expected2,
                               milter_headers_get_nth_header(actual_headers, 2));
    milter_assert_equal_header(&expected3,
                               milter_headers_get_nth_header(actual_headers, 3));
}

static void
option_test_assert_body (void)
{
    cut_assert_equal_string("chunk1chunk2chunk3", actual_body_string->str);
}

static void
option_test_assert_mail_file (void)
{
    MilterHeader expected_header1 = {"Return-Path", "<xxxx@example.com>"};
    MilterHeader expected_header2 = {"Date", "Wed, 5 Nov 2008 11:41:54 -0200"};
    MilterHeader expected_header3 = {"Message-ID", "<168f979z.5435357@xxx>"};
    MilterHeader expected_header4 = {"Content-Type", "text/plain; charset=\"us-ascii\""};
    MilterHeader expected_header5 = {"X-Source", ""};
    MilterHeader expected_header6 = {"To", "Test Sender <test-to@example.com>"};
    MilterHeader expected_header7 = {"From", "test-from@example.com"};
    MilterHeader expected_header8 = {"List-Archive",
                                     "\n <http://sourceforge.net/mailarchive/forum.php?forum_name=milter-manager-commit>"};
    MilterHeader expected_header9 = {"Subject", "This is a test mail"};
    MilterHeader expected_header10 = {"List-Help",
                                      "\n <mailto:milter-manager-commit-request@lists.sourceforge.net?subject=help>"};

    cut_assert_equal_string("<test-from@example.com>", actual_envelope_from);
    cut_assert_equal_int(1, g_list_length(actual_recipients));
    cut_assert_equal_string("<test-to@example.com>", actual_recipients->data);

    cut_assert_equal_uint(10, milter_headers_length(actual_headers));
    milter_assert_equal_header(&expected_header1,
                               milter_headers_get_nth_header(actual_headers, 1));
    milter_assert_equal_header(&expected_header2,
                               milter_headers_get_nth_header(actual_headers, 2));
    milter_assert_equal_header(&expected_header3,
                               milter_headers_get_nth_header(actual_headers, 3));
    milter_assert_equal_header(&expected_header4,
                               milter_headers_get_nth_header(actual_headers, 4));
    milter_assert_equal_header(&expected_header5,
                               milter_headers_get_nth_header(actual_headers, 5));
    milter_assert_equal_header(&expected_header6,
                               milter_headers_get_nth_header(actual_headers, 6));
    milter_assert_equal_header(&expected_header7,
                               milter_headers_get_nth_header(actual_headers, 7));
    milter_assert_equal_header(&expected_header8,
                               milter_headers_get_nth_header(actual_headers, 8));
    milter_assert_equal_header(&expected_header9,
                               milter_headers_get_nth_header(actual_headers, 9));
    milter_assert_equal_header(&expected_header10,
                               milter_headers_get_nth_header(actual_headers, 10));

    cut_assert_equal_string("Message body\r\nMessage body\r\nMessage body\r\n",
                            actual_body_string->str);
}

#define NORMAL_COLOR "\033[00m"

static void
option_test_assert_output_message (void)
{
    const gchar *normalized_output;

    normalized_output = cut_take_replace(output_string->str,
                                        "Finished in [\\d.]+sec.",
                                        "Finished in XXXsec.");
    cut_assert_equal_string(
        "The message was 'pass'.\n"
        "\n"
        NORMAL_COLOR "  From: <kou+send@cozmixng.org>" NORMAL_COLOR "\n"
        NORMAL_COLOR "  To: <kou+receive@cozmixng.org>" NORMAL_COLOR "\n"
        "---------------------------------------\n"
        "La de da de da 1.\n"
        "La de da de da 2.\n"
        "La de da de da 3.\n"
        "La de da de da 4.\n"
        "\n"
        "Finished in XXXsec.\n",
        normalized_output);
}

static void
option_test_assert_large_mail (void)
{
    /* Nothing here yet. */
}

static const gchar *
get_mail_option (const gchar *file_name)
{
    gchar *mail_option, *file_path;

    file_path = g_build_filename(fixtures_path, file_name, NULL);
    cut_take_string(file_path);
    mail_option = g_strdup_printf("--mail-file=%s", file_path);
    return cut_take_string(mail_option);
}

void
data_option (void)
{
    cut_add_data("connect-host",
                 option_test_data_new("--connect-host=test-host",
                                      option_test_assert_connect_host),
                 option_test_data_free,
                 "connect-address",
                 option_test_data_new("--connect-address=inet:12345@192.168.1.1",
                                      option_test_assert_connect_address),
                 option_test_data_free,
                 "connect",
                 option_test_data_new("--connect-host=test-host "
                                      "--connect-address=inet:12345@192.168.1.1",
                                      option_test_assert_connect),
                 option_test_data_free,
                 "helo-fqdn",
                 option_test_data_new("--helo-fqdn=test-host",
                                      option_test_assert_helo_fqdn),
                 option_test_data_free,
                 "envelope-from",
                 option_test_data_new("--from=from@example.com",
                                      option_test_assert_from),
                 option_test_data_free,
                 "recipients",
                 option_test_data_new("--recipient=recipient1@example.com "
                                      "--recipient=recipient2@example.com "
                                      "--recipient=recipient3@example.com",
                                      option_test_assert_recipients),
                 option_test_data_free,
                 "headers",
                 option_test_data_new("--header=X-TestHeader1:TestHeader1Value "
                                      "--header=X-TestHeader2:TestHeader2Value "
                                      "--header=X-TestHeader3:TestHeader3Value",
                                      option_test_assert_headers),
                 option_test_data_free,
                 "body",
                 option_test_data_new("--body=chunk1 "
                                      "--body=chunk2 "
                                      "--body=chunk3",
                                      option_test_assert_body),
                 option_test_data_free,
                 "output-message",
                 option_test_data_new("--output-message",
                                      option_test_assert_output_message),
                 option_test_data_free,
                 "mail-file",
                 option_test_data_new(get_mail_option("parse-test.mail"),
                                      option_test_assert_mail_file),
                 option_test_data_free,
                 "large-file",
                 option_test_data_new(get_mail_option("large.mail"),
                                      option_test_assert_large_mail),
                 option_test_data_free);
}

void
test_option (gconstpointer data)
{
    OptionTestData *option_test_data = (OptionTestData*)data;

    setup_test_client("inet:9999@localhost", option_test_data->test_data);
    setup_server("inet:9999@localhost", option_test_data->option_string);

    gcut_egg_hatch(server, &error);
    gcut_assert_error(error);

    wait_for_reaping(TRUE);
    cut_assert_equal_int(EXIT_SUCCESS, exit_status);

    gcut_assert_equal_enum(MILTER_TYPE_STATUS,
                           option_test_data->test_data->expected_status,
                           actual_status);

    option_test_data->assert_func();
}

typedef void (*AssertFunc) (void);

typedef struct _EndOfMessageActionTestData
{
    TestData *test_data;
    AssertFunc assert_func;
} EndOfMessageActionTestData;

static EndOfMessageActionTestData *
end_of_message_action_test_data_new (EndOfMessageActionFunc action_func,
                                     AssertFunc assert_func)
{
    EndOfMessageActionTestData *test_data;

    test_data = g_new0(EndOfMessageActionTestData, 1);
    test_data->test_data = result_test_data_new(MILTER_ACTION_NONE,
                                                MILTER_STEP_NONE,
                                                MILTER_STATUS_NOT_CHANGE,
                                                NULL,
                                                NULL,
                                                NULL,
                                                action_func);
    test_data->assert_func = assert_func;

    return test_data;
}

static void
end_of_message_action_test_data_free (EndOfMessageActionTestData *data)
{
    result_test_data_free(data->test_data);
    g_free(data);
}

static void
change_from_function (void)
{
    packet_free();

    milter_reply_encoder_encode_change_from(encoder, &packet, &packet_size,
                                            "changed-from@example.com", NULL);

    write_data(packet, packet_size);
}

static void
end_of_message_action_assert_change_from (void)
{
    cut_assert_match("(.*\n)*- From: <kou\\+send@cozmixng\\.org>"
                     "(.*\n)*", output_string->str);
    cut_assert_match("(.*\n)*\\+ From: changed-from@example\\.com"
                     "(.*\n)*", output_string->str);
}

static void
add_recipient_function (void)
{
    packet_free();

    milter_reply_encoder_encode_add_recipient(encoder, &packet, &packet_size, 
                                              "added-to@example.com", NULL);

    write_data(packet, packet_size);
}

static void
end_of_message_action_assert_add_recipient (void)
{
    cut_assert_match("(.*\n)*- To: <kou\\+receive@cozmixng\\.org>"
                     "(.*\n)*", output_string->str);
    cut_assert_match("(.*\n)*\\+ To: <kou\\+receive@cozmixng\\.org>, added-to@example\\.com"
                     "(.*\n)*", output_string->str);
}

static void
delete_recipient_function (void)
{
    packet_free();

    milter_reply_encoder_encode_delete_recipient(encoder, &packet, &packet_size,
                                                 "kou+receive@cozmixng.org");

    write_data(packet, packet_size);
}

static void
end_of_message_action_assert_delete_recipient (void)
{
    cut_assert_match("(.*\n)*- To: <kou\\+receive@cozmixng\\.org>"
                     "(.*\n)*", output_string->str);
}

static void
add_header_function (void)
{
    packet_free();

    milter_reply_encoder_encode_add_header(encoder, &packet, &packet_size, 
                                           "X-Test-Header1", "TestHeader1Value");

    write_data(packet, packet_size);
}

static void
end_of_message_action_assert_add_header (void)
{
    cut_assert_match("(.*\n)*\\+ X-Test-Header1: TestHeader1Value"
                     "(.*\n)*", output_string->str);
}

static void
insert_header_function (void)
{
    packet_free();

    milter_reply_encoder_encode_insert_header(encoder, &packet, &packet_size, 
                                              1, "X-Test-Header1", "TestHeader1Value");

    write_data(packet, packet_size);
}

static void
end_of_message_action_assert_insert_header (void)
{
    cut_assert_match("(.*\n)*\\+ X-Test-Header1: TestHeader1Value"
                     "(.*\n)*", output_string->str);
}

static void
change_header_function (void)
{
    packet_free();

    milter_reply_encoder_encode_change_header(encoder, &packet, &packet_size,
                                              "To", 1, "changed-to@example.com");

    write_data(packet, packet_size);
}

static void
end_of_message_action_assert_change_header (void)
{
    cut_assert_match("(.*\n)*- To: <kou\\+receive@cozmixng\\.org>"
                     "(.*\n)*", output_string->str);
    cut_assert_match("(.*\n)*\\+ To: changed-to@example\\.com"
                     "(.*\n)*", output_string->str);
}

static void
delete_header_function (void)
{
    packet_free();

    milter_reply_encoder_encode_delete_header(encoder, &packet, &packet_size,
                                              "To", 1);

    write_data(packet, packet_size);
}

static void
end_of_message_action_assert_delete_header (void)
{
    cut_assert_match("(.*\n)*- To: <kou\\+receive@cozmixng\\.org>"
                     "(.*\n)*", output_string->str);
}

static void
replace_body_function (void)
{
    const gchar replace_string[] = "This is the replaced message.";
    gsize packed_size;

    packet_free();

    milter_reply_encoder_encode_replace_body(encoder, &packet, &packet_size,
                                             replace_string,
                                             strlen(replace_string),
                                             &packed_size);

    write_data(packet, packet_size);
}

static void
end_of_message_action_assert_replace_body (void)
{
    cut_assert_match("(.*\n)*-*"
                     "This is the replaced message.",
                     output_string->str);
}

void
data_end_of_message_action (void)
{
    cut_add_data("change-from",
                 end_of_message_action_test_data_new(change_from_function,
                                                     end_of_message_action_assert_change_from),
                 end_of_message_action_test_data_free,
                 "add-recipient",
                 end_of_message_action_test_data_new(add_recipient_function,
                                                     end_of_message_action_assert_add_recipient),
                 end_of_message_action_test_data_free,
                 "delete-recipient",
                 end_of_message_action_test_data_new(delete_recipient_function,
                                                     end_of_message_action_assert_delete_recipient),
                 end_of_message_action_test_data_free,
                 "add-header",
                 end_of_message_action_test_data_new(add_header_function,
                                                     end_of_message_action_assert_add_header),
                 end_of_message_action_test_data_free,
                 "insert-header",
                 end_of_message_action_test_data_new(insert_header_function,
                                                     end_of_message_action_assert_insert_header),
                 end_of_message_action_test_data_free,
                 "change-header",
                 end_of_message_action_test_data_new(change_header_function,
                                                     end_of_message_action_assert_change_header),
                 end_of_message_action_test_data_free,
                 "delete-header",
                 end_of_message_action_test_data_new(delete_header_function,
                                                     end_of_message_action_assert_delete_header),
                 end_of_message_action_test_data_free,
                 "replace-body",
                 end_of_message_action_test_data_new(replace_body_function,
                                                     end_of_message_action_assert_replace_body),
                 end_of_message_action_test_data_free);
}

void
test_end_of_message_action (gconstpointer data)
{
    EndOfMessageActionTestData *action_test_data;
    action_test_data = (EndOfMessageActionTestData*)data;

    setup_test_client("inet:9999@localhost", action_test_data->test_data);
    setup_server("inet:9999@localhost", "--output-message");

    gcut_egg_hatch(server, &error);
    gcut_assert_error(error);

    wait_for_reaping(TRUE);
    cut_assert_equal_int(EXIT_SUCCESS, exit_status);

    gcut_assert_equal_enum(MILTER_TYPE_STATUS,
                           actual_status, action_test_data->test_data->expected_status);

    action_test_data->assert_func();
}

typedef void (*MacroTestAssertFunc) (GHashTable *expected_macros_table);

typedef struct _MacroTestData
{
    TestData *test_data;
    GHashTable *expected_macros_table;
    MacroTestAssertFunc assert_func;
} MacroTestData;

static MacroTestData *
macro_test_data_new (GHashTable *expected_macros_table,
                     MilterMacrosRequests *macros_requests,
                     MacroTestAssertFunc assert_func)
{
    MacroTestData *test_data;

    test_data = g_new0(MacroTestData, 1);
    test_data->test_data = result_test_data_new(MILTER_ACTION_NONE,
                                                MILTER_STEP_NONE,
                                                MILTER_STATUS_NOT_CHANGE,
                                                NULL,
                                                NULL,
                                                macros_requests,
                                                NULL);
    test_data->expected_macros_table = expected_macros_table;
    test_data->assert_func = assert_func;

    return test_data;
}

static void
macro_test_data_free (MacroTestData *data)
{
    result_test_data_free(data->test_data);
    g_hash_table_unref(data->expected_macros_table);
    g_free(data);
}

static GHashTable *
create_macro_hash_table (const gchar *first_key, ...)
{
    va_list var_args;
    GHashTable *macros;
    const gchar *key;

    macros = g_hash_table_new_full(g_str_hash, g_str_equal,
                                   g_free, g_free);

    va_start(var_args, first_key);
    key = first_key;
    while (key) {
        const gchar *value;
        value = va_arg(var_args, gchar *);
        g_hash_table_insert(macros, g_strdup(key), g_strdup(value));
        key = va_arg(var_args, gchar *);
    }
    va_end(var_args);

    return macros;
}

static GHashTable *
create_expected_default_macros_table (void)
{
    GHashTable *expected_macros_table;
    expected_macros_table = g_hash_table_new_full(g_direct_hash, g_direct_equal,
                                                  NULL,
                                                  (GDestroyNotify)g_hash_table_unref);
    g_hash_table_insert(expected_macros_table,
                        GINT_TO_POINTER(MILTER_COMMAND_CONNECT),
                        create_macro_hash_table("daemon_name", "milter-test-server",
                                                "if_name", "localhost",
                                                "if_addr", "127.0.0.1",
                                                "j", "milter-test-server",
                                                NULL));
    g_hash_table_insert(expected_macros_table,
                        GINT_TO_POINTER(MILTER_COMMAND_HELO),
                        create_macro_hash_table("tls_version", "0",
                                                "cipher", "0",
                                                "cipher_bits", "0",
                                                "cert_subject", "cert_subject",
                                                "cert_issuer", "cert_issuer",
                                                NULL));
    g_hash_table_insert(expected_macros_table,
                        GINT_TO_POINTER(MILTER_COMMAND_ENVELOPE_FROM),
                        create_macro_hash_table("i", "i",
                                                "auth_type", "auth_type",
                                                "auth_authen", "auth_authen",
                                                "auto_ssf", "auto_ssf",
                                                "auto_author", "auto_author",
                                                "mail_mailer", "mail_mailer",
                                                "mail_host", "mail_host",
                                                "mail_addr", "mail_addr",
                                                NULL));
    g_hash_table_insert(expected_macros_table,
                        GINT_TO_POINTER(MILTER_COMMAND_ENVELOPE_RECIPIENT),
                        create_macro_hash_table("rcpt_mailer", "rcpt_mailer",
                                                "rcpt_host", "rcpt_host",
                                                "rcpt_addr", "<kou+receive@cozmixng.org>",
                                                NULL));
    g_hash_table_insert(expected_macros_table,
                        GINT_TO_POINTER(MILTER_COMMAND_END_OF_MESSAGE),
                        create_macro_hash_table("msg-id", "msg-id",
                                                NULL));
    return expected_macros_table;
}

static GHashTable *
create_expected_connect_macros_table (void)
{
    GHashTable *expected_macros_table;
    GHashTable *connect_macros;

    expected_macros_table = create_expected_default_macros_table();
    connect_macros = g_hash_table_lookup(expected_macros_table,
                                         GINT_TO_POINTER(MILTER_COMMAND_CONNECT));

    g_hash_table_insert(connect_macros,
                        g_strdup("macro_name"),
                        g_strdup("{macro_name}"));

    return expected_macros_table;
}

static void
assert_each_macros (gpointer key, gpointer value, gpointer user_data)
{
    GHashTable *actual_macros = value;
    GHashTable *expected_macros_table = user_data;
    GHashTable *expected_macros;

    expected_macros = g_hash_table_lookup(expected_macros_table, key);
    cut_assert_not_null(expected_macros);

    gcut_assert_equal_hash_table_string_string(expected_macros,
                                               actual_macros);
}

static void
default_macro_test_assert (GHashTable *expected_macros_table)
{
    cut_trace(g_hash_table_foreach(actual_defined_macros,
              (GHFunc)assert_each_macros, expected_macros_table));
}

static MilterMacrosRequests *
create_macros_requests (MilterCommand first_command, ...)
{
    va_list var_args;
    MilterCommand command;
    MilterMacrosRequests *macros_requests;

    macros_requests = milter_macros_requests_new();

    va_start(var_args, first_command);
    command = first_command;
    while (command) {
        gchar **symbols;
        symbols = va_arg(var_args, gchar**);
        milter_macros_requests_set_symbols_string_array(macros_requests,
                                                        command,
                                                        (const gchar**)symbols);
        command = va_arg(var_args, gint);
    }
    va_end(var_args);

    return macros_requests;
}

void
data_macro (void)
{
    const gchar *connect_macros[] = {"{macro_name}", NULL};

    cut_add_data("default",
                 macro_test_data_new(create_expected_default_macros_table(),
                                     NULL,
                                     default_macro_test_assert),
                 macro_test_data_free,
                 "connect macro",
                 macro_test_data_new(create_expected_connect_macros_table(),
                                     create_macros_requests(MILTER_COMMAND_CONNECT,
                                                            connect_macros, NULL),
                                     default_macro_test_assert),
                 macro_test_data_free);
}

void
test_macro (gconstpointer data)
{
    MacroTestData *macro_test_data;
    macro_test_data = (MacroTestData*)data;

    setup_test_client("inet:9999@localhost", macro_test_data->test_data);
    setup_server("inet:9999@localhost", NULL);

    gcut_egg_hatch(server, &error);
    gcut_assert_error(error);

    wait_for_reaping(TRUE);
    cut_assert_equal_int(EXIT_SUCCESS, exit_status);

    gcut_assert_equal_enum(MILTER_TYPE_STATUS,
                           macro_test_data->test_data->expected_status,
                           actual_status);

    macro_test_data->assert_func(macro_test_data->expected_macros_table);
}

typedef struct _InvalidSpecTestData
{
    gchar *spec;
    gchar *expected_message;
} InvalidSpecTestData;

static InvalidSpecTestData *
invalid_spec_test_data_new (const gchar *spec,
                            const gchar *expected_message)
{
    InvalidSpecTestData *test_data;

    test_data = g_new0(InvalidSpecTestData, 1);
    test_data->spec = g_strdup(spec);
    test_data->expected_message = g_strdup(expected_message);

    return test_data;
}

static void
invalid_spec_test_data_free (InvalidSpecTestData *data)
{
    g_free(data->spec);
    g_free(data->expected_message);
    g_free(data);
}

void
data_invalid_spec (void)
{
    cut_add_data("invalid protocol name",
                 invalid_spec_test_data_new("xxxx:9999@localhost",
                                            "protocol must be 'unix', 'inet' or 'inet6': "
                                            "<xxxx:9999@localhost>: <xxxx>\n"),
                 invalid_spec_test_data_free,
                 "out of range port number",
                 invalid_spec_test_data_new("inet:99999@localhost",
                                            "port number should be less than 65536: "
                                            "<inet:99999@localhost>: <99999>\n"),
                 invalid_spec_test_data_free);
}

void
test_invalid_spec (gconstpointer data)
{
    InvalidSpecTestData *test_data = (InvalidSpecTestData*)data;

    setup_server(test_data->spec, NULL);
    gcut_egg_hatch(server, &error);
    gcut_assert_error(error);

    wait_for_reaping(FALSE);
    cut_assert_equal_string(test_data->expected_message, output_string->str);
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
