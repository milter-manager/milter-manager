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

#define ADDITIONAL_FLAG_MASK 0xf000
#define REJECT_FLAG 0x1000
#define TEMPORARY_FAILURE_FLAG 0x2000

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

static MilterEventLoop *loop;

static MilterTestClient *client;
static MilterDecoder *decoder;
static MilterReplyEncoder *encoder;
static GCutProcess *server;
static GArray *command;
static gchar *server_path;
static gchar *fixtures_path;

static GError *error;
static gboolean reaped;
static gint exit_status;
static GString *output_string;
static GString *normalized_output_string;
static GString *error_string;
static MilterStatus actual_status;
static const gchar *actual_status_string;

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
static GList *actual_defined_macros_list;
static gchar *actual_connect_host;
static gchar *actual_connect_address;
static gchar *actual_helo_fqdn;
static gchar *actual_envelope_from;
static GList *actual_recipients;
static GString *actual_body_string;
static MilterHeaders *expected_headers;
static MilterHeaders *actual_headers;

typedef void (*EndOfMessageActionFunc) (void);

typedef struct _TestData
{
    gchar *option_string;
    GList *replies;
    GHashTable *expected_command_receives;
    MilterStatus expected_status;
    gchar *expected_status_string;
    MilterActionFlags actions;
    MilterStepFlags steps;
    EndOfMessageActionFunc action_func;
    MilterMacrosRequests *macros_requests;
} TestData;

void
cut_startup (void)
{
    fixtures_path = g_build_filename(milter_test_get_source_dir(),
                                     "test",
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
write_data (const gchar *data, gsize data_size)
{
    milter_test_client_write(client, data, data_size);
}

static void
cb_negotiate (MilterCommandDecoder *decoder, MilterOption *option,
              TestData *test_data)
{
    const gchar *packet;
    gsize packet_size;
    MilterOption *reply_option;
    MilterMacrosRequests *macros_requests = NULL;

    if (test_data) {
        reply_option = milter_option_new(6,
                                         test_data->actions,
                                         test_data->steps);
        macros_requests = test_data->macros_requests;
        if (macros_requests)
            g_object_ref(macros_requests);
    } else {
        reply_option = milter_option_copy(option);
        milter_option_remove_step(reply_option, MILTER_STEP_NO_MASK);
    }
    milter_reply_encoder_encode_negotiate(encoder,
                                          &packet, &packet_size,
                                          reply_option,
                                          macros_requests);
    write_data(packet, packet_size);
    g_object_unref(reply_option);
    if (macros_requests)
        g_object_unref(macros_requests);

    n_negotiates++;
}

static void
append_to_actual_defined_macros_list (MilterCommand command,
                                      GHashTable *macros)
{
    gchar *command_nick_name;
    GList *keys, *node;

    command_nick_name = milter_utils_get_enum_nick_name(MILTER_TYPE_COMMAND,
                                                        command);
    actual_defined_macros_list =
        g_list_append(actual_defined_macros_list,
                      g_strdup_printf("command:%s", command_nick_name));
    g_free(command_nick_name);

    keys = g_hash_table_get_keys(macros);
    keys = g_list_sort(keys, (GCompareFunc)strcmp);
    for (node = keys; node; node = g_list_next(node)) {
        const gchar *key = node->data;

        actual_defined_macros_list =
            g_list_append(actual_defined_macros_list, g_strdup(key));
        actual_defined_macros_list =
            g_list_append(actual_defined_macros_list,
                          g_strdup(g_hash_table_lookup(macros, key)));
    }
    g_list_free(keys);
}

static void
cb_define_macro (MilterDecoder *decoder, MilterCommand command,
                 GHashTable *macros, gpointer user_data)
{
    g_hash_table_replace(actual_defined_macros,
                         GINT_TO_POINTER(command),
                         g_hash_table_ref(macros));

    append_to_actual_defined_macros_list(command, macros);

    n_define_macros++;
}

static void
send_reply (MilterCommand command, TestData *test_data)
{
    const gchar *packet;
    gsize packet_size;
    MilterReply reply;
    gint additional_flag;
    GList *reply_pair = NULL;

    if (test_data)
        reply_pair = g_list_find(test_data->replies, GINT_TO_POINTER(command));

    if (reply_pair) {
        GList *reply_value;

        reply_value = g_list_next(reply_pair);
        reply = GPOINTER_TO_INT(reply_value->data);
        test_data->replies = g_list_remove_link(test_data->replies, reply_pair);
        test_data->replies = g_list_remove_link(test_data->replies, reply_value);
    } else {
        reply = MILTER_REPLY_CONTINUE;
    }

    additional_flag = reply & ADDITIONAL_FLAG_MASK;
    reply = reply & ~ADDITIONAL_FLAG_MASK;

    switch (reply) {
    case MILTER_REPLY_ACCEPT:
        milter_reply_encoder_encode_accept(encoder, &packet, &packet_size);
        break;
    case MILTER_REPLY_TEMPORARY_FAILURE:
        milter_reply_encoder_encode_temporary_failure(encoder,
                                                      &packet, &packet_size);
        break;
    case MILTER_REPLY_REJECT:
        milter_reply_encoder_encode_reject(encoder, &packet, &packet_size);
        break;
    case MILTER_REPLY_DISCARD:
        milter_reply_encoder_encode_discard(encoder, &packet, &packet_size);
        break;
    case MILTER_REPLY_REPLY_CODE:
        if (additional_flag == TEMPORARY_FAILURE_FLAG)
            milter_reply_encoder_encode_reply_code(encoder,
                                                   &packet, &packet_size,
                                                   "451 4.7.1 Greylisting");
        else
            milter_reply_encoder_encode_reply_code(encoder,
                                                   &packet, &packet_size,
                                                   "554 5.7.1 1% 2%% 3%%%");
        break;
    case MILTER_REPLY_SHUTDOWN:
        milter_reply_encoder_encode_shutdown(encoder, &packet, &packet_size);
        break;
    case MILTER_REPLY_CONNECTION_FAILURE:
        milter_reply_encoder_encode_connection_failure(encoder, &packet, &packet_size);
        break;
    case MILTER_REPLY_QUARANTINE:
        milter_reply_encoder_encode_quarantine(encoder, &packet, &packet_size,
                                               "virus");
        write_data(packet, packet_size);
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
    if (!test_data || !(test_data->steps & MILTER_STEP_NO_REPLY_CONNECT))
        send_reply(MILTER_COMMAND_CONNECT, test_data);

    n_connects++;
    actual_connect_host = g_strdup(host_name);
    actual_connect_address = milter_connection_address_to_spec(address);
}

static void
cb_helo (MilterCommandDecoder *decoder, const gchar *fqdn, TestData *test_data)
{
    if (!test_data || !(test_data->steps & MILTER_STEP_NO_REPLY_HELO))
        send_reply(MILTER_COMMAND_HELO, test_data);

    n_helos++;
    actual_helo_fqdn = g_strdup(fqdn);
}

static void
cb_envelope_from (MilterCommandDecoder *decoder, const gchar *from,
                  TestData *test_data)
{
    if (!test_data || !(test_data->steps & MILTER_STEP_NO_REPLY_ENVELOPE_FROM))
        send_reply(MILTER_COMMAND_ENVELOPE_FROM, test_data);

    n_envelope_froms++;
    actual_envelope_from = g_strdup(from);
}

static void
cb_envelope_recipient (MilterCommandDecoder *decoder, const gchar *to,
                       TestData *test_data)
{
    if (!test_data ||
        !(test_data->steps & MILTER_STEP_NO_REPLY_ENVELOPE_RECIPIENT))
        send_reply(MILTER_COMMAND_ENVELOPE_RECIPIENT, test_data);

    n_envelope_recipients++;
    actual_recipients = g_list_append(actual_recipients, g_strdup(to));
}

static void
cb_data (MilterCommandDecoder *decoder, TestData *test_data)
{
    if (!test_data || !(test_data->steps & MILTER_STEP_NO_REPLY_DATA))
        send_reply(MILTER_COMMAND_DATA, test_data);

    n_datas++;
}

static void
cb_header (MilterCommandDecoder *decoder, const gchar *name, const gchar *value,
           TestData *test_data)
{
    if (!test_data || !(test_data->steps & MILTER_STEP_NO_REPLY_HEADER))
        send_reply(MILTER_COMMAND_HEADER, test_data);

    n_headers++;
    milter_headers_append_header(actual_headers, name, value);
}

static void
cb_end_of_header (MilterCommandDecoder *decoder, TestData *test_data)
{
    if (!test_data || !(test_data->steps & MILTER_STEP_NO_REPLY_END_OF_HEADER))
        send_reply(MILTER_COMMAND_END_OF_HEADER, test_data);

    n_end_of_headers++;
}

static void
cb_body (MilterCommandDecoder *decoder, const gchar *chunk, gsize size,
         TestData *test_data)
{
    if (!test_data || !(test_data->steps & MILTER_STEP_NO_REPLY_BODY))
        send_reply(MILTER_COMMAND_BODY, test_data);

    n_bodies++;
    g_string_append_len(actual_body_string, chunk, size);
}

static void
cb_end_of_message (MilterCommandDecoder *decoder, const gchar *chunk, gsize size,
                   TestData *test_data)
{
    if (test_data && test_data->action_func)
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
cut_setup (void)
{
    loop = milter_glib_event_loop_new(NULL);

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

    error = NULL;
    reaped = FALSE;
    output_string = g_string_new(NULL);
    normalized_output_string = g_string_new(NULL);
    error_string = g_string_new(NULL);

    actual_status = MILTER_STATUS_DEFAULT;
    actual_status_string = NULL;
    actual_connect_host = NULL;
    actual_connect_address = NULL;
    actual_helo_fqdn = NULL;
    actual_envelope_from = NULL;
    actual_recipients = NULL;
    expected_headers = milter_headers_new();
    actual_headers = milter_headers_new();
    actual_body_string = g_string_new(NULL);
    actual_defined_macros =
        g_hash_table_new_full(g_direct_hash, g_direct_equal,
                              NULL,
                              (GDestroyNotify)g_hash_table_unref);
    actual_defined_macros_list = NULL;
    exit_status = EXIT_FAILURE;
}

void
cut_teardown (void)
{
    if (client)
        g_object_unref(client);
    if (decoder)
        g_object_unref(decoder);
    if (encoder)
        g_object_unref(encoder);
    if (command)
        g_array_free(command, TRUE);

    if (output_string)
        g_string_free(output_string, TRUE);
    if (normalized_output_string)
        g_string_free(normalized_output_string, TRUE);
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
    if (expected_headers)
        g_object_unref(expected_headers);
    if (actual_headers)
        g_object_unref(actual_headers);
    if (actual_body_string)
        g_string_free(actual_body_string, TRUE);
    if (actual_defined_macros)
        g_hash_table_unref(actual_defined_macros);
    if (actual_defined_macros_list) {
        g_list_foreach(actual_defined_macros_list, (GFunc)g_free, NULL);
        g_list_free(actual_defined_macros_list);
    }
}

static void
cb_output_received (GCutProcess *process, const gchar *chunk, gsize size,
                    gpointer user_data)
{
    g_string_append_len(output_string, chunk, size);
}

static void
cb_error_received (GCutProcess *process, const gchar *chunk, gsize size,
                   gpointer user_data)
{
    g_string_append_len(error_string, chunk, size);
}

static void
cb_reaped (GCutProcess *process, gint status, gpointer user_data)
{
    reaped = TRUE;
    exit_status = status;
}

static void
append_argument (const gchar *argument)
{
    gchar *dupped_argument;

    dupped_argument = g_strdup(argument);
    g_array_append_val(command, dupped_argument);
}

static void
setup_server (const gchar *spec, const gchar *option_string)
{
    append_argument("--name");
    append_argument("milter-test-server");
    append_argument("-s");
    g_array_append_val(command, spec);

    if (option_string) {
        gint i, argc;
        gchar **argv;
        GError *error = NULL;

        if (!g_shell_parse_argv(option_string, &argc, &argv, &error)) {
            gcut_assert_error(error);
        }
        for (i = 0; i < argc; i++) {
            gchar *arg;
            arg = g_strdup(argv[i]);
            g_array_append_val(command, arg);
        }
        g_strfreev(argv);
    }

    server = gcut_process_new_array(command);

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
    cut_trace(client = milter_test_client_new(spec, decoder, loop));
    setup_command_signals(decoder, test_data);
}

static gboolean
cb_timeout_waiting (gpointer data)
{
    gboolean *waiting = data;

    *waiting = FALSE;
    return FALSE;
}

static void
wait_for_reaping (gboolean check_success)
{
    GError *error = NULL;
    gchar *normalized_status_string;
    gchar *begin_pos, *status_end_pos, *normalized_status_end_pos;
    gboolean timeout_waiting = TRUE;
    guint timeout_waiting_id;

    timeout_waiting_id = milter_event_loop_add_timeout(loop, 1,
                                                       cb_timeout_waiting,
                                                       &timeout_waiting);
    while (timeout_waiting && !reaped) {
        milter_event_loop_iterate(loop, TRUE);
    }
    milter_event_loop_remove(loop, timeout_waiting_id);
    cut_assert_true(timeout_waiting, cut_message("timeout"));

    g_string_assign(normalized_output_string,
                    cut_take_replace(output_string->str,
                                     "elapsed-time: [\\d.]+ seconds",
                                     "elapsed-time: XXX seconds"));

    if (!check_success)
        return;

    cut_assert_match("status: (.*)", output_string->str);
    begin_pos = output_string->str + strlen("status: ");

    status_end_pos = strstr(output_string->str, "\n");
    actual_status_string = cut_take_strndup(begin_pos,
                                            status_end_pos - begin_pos);

    normalized_status_end_pos = strstr(output_string->str, " [");
    if (!normalized_status_end_pos)
        normalized_status_end_pos = status_end_pos;
    normalized_status_string = g_strndup(begin_pos,
                                         normalized_status_end_pos - begin_pos);

    if (strcmp(normalized_status_string, "pass") == 0) {
        g_free(normalized_status_string);
        normalized_status_string = g_strdup("not-change");
    }
    actual_status = gcut_enum_parse(MILTER_TYPE_STATUS,
                                    normalized_status_string,
                                    &error);
    g_free(normalized_status_string);

    gcut_assert_error(error);
}

static TestData *
result_test_data_new (const gchar *option_string,
                      MilterActionFlags actions,
                      MilterStepFlags steps,
                      MilterStatus status,
                      const gchar *status_string,
                      GList *replies,
                      GHashTable *expected_command_receives,
                      MilterMacrosRequests *macros_requests,
                      EndOfMessageActionFunc action_func)
{
    TestData *test_data;

    test_data = g_new0(TestData, 1);
    test_data->option_string = g_strdup(option_string);
    test_data->actions = actions;
    test_data->steps = steps;
    test_data->expected_status = status;
    test_data->expected_status_string = g_strdup(status_string);
    if (replies)
        test_data->replies = replies;
    else
        test_data->replies = NULL;
    test_data->expected_command_receives = expected_command_receives;
    test_data->macros_requests = macros_requests;
    test_data->action_func = action_func;

    return test_data;
}

static void
result_test_data_free (TestData *test_data)
{
    if (test_data->option_string)
        g_free(test_data->option_string);
    if (test_data->expected_status_string)
        g_free(test_data->expected_status_string);
    g_list_free(test_data->replies);
    if (test_data->expected_command_receives)
        g_hash_table_unref(test_data->expected_command_receives);
    if (test_data->macros_requests)
        g_object_unref(test_data->macros_requests);
    g_free(test_data);
}

static GList *
create_replies (MilterCommand first_command, ...)
{
    va_list var_args;
    MilterCommand command;
    GList *replies = NULL;

    va_start(var_args, first_command);
    command = first_command;
    while (command) {
        MilterReply reply;

        reply = va_arg(var_args, gint);
        replies = g_list_append(replies, GINT_TO_POINTER(command));
        replies = g_list_append(replies, GINT_TO_POINTER(reply));
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
                            GINT_TO_POINTER(command),
                            GINT_TO_POINTER(expected_num));
        command = va_arg(var_args, gint);
    }
    va_end(var_args);

    return expected_command_receives;
}

#define ADD(label, option_string, actions, steps, status,               \
            status_string, replies, expected_command_receives,          \
            macros_requests, action_func)                               \
    cut_add_data(label,                                                 \
                 result_test_data_new(option_string,                    \
                                      actions,                          \
                                      steps,                            \
                                      status,                           \
                                      status_string,                    \
                                      replies,                          \
                                      expected_command_receives,        \
                                      macros_requests,                  \
                                      action_func),                     \
                 result_test_data_free,                                 \
                 NULL)

static void
all_data_result (void)
{
    ADD("all continue",
        NULL,
        MILTER_ACTION_NONE,
        MILTER_STEP_NONE,
        MILTER_STATUS_NOT_CHANGE,
        "pass",
        NULL,
        NULL,
        NULL,
        NULL);
}

static void
no_data_result (void)
{
    ADD("no connect",
        NULL,
        MILTER_ACTION_NONE,
        MILTER_STEP_NO_CONNECT,
        MILTER_STATUS_NOT_CHANGE,
        "pass",
        NULL,
        create_expected_command_receives(MILTER_COMMAND_CONNECT, 0,
                                         NULL),
        NULL,
        NULL);
    ADD("no helo",
        NULL,
        MILTER_ACTION_NONE,
        MILTER_STEP_NO_HELO,
        MILTER_STATUS_NOT_CHANGE,
        "pass",
        NULL,
        create_expected_command_receives(MILTER_COMMAND_HELO, 0,
                                         NULL),
        NULL,
        NULL);
    ADD("no envelope-from",
        NULL,
        MILTER_ACTION_NONE,
        MILTER_STEP_NO_ENVELOPE_FROM,
        MILTER_STATUS_NOT_CHANGE,
        "pass",
        NULL,
        create_expected_command_receives(MILTER_COMMAND_ENVELOPE_FROM, 0,
                                         NULL),
        NULL,
        NULL);
    ADD("no envelope-recipient",
        NULL,
        MILTER_ACTION_NONE,
        MILTER_STEP_NO_ENVELOPE_RECIPIENT,
        MILTER_STATUS_NOT_CHANGE,
        "pass",
        NULL,
        create_expected_command_receives(MILTER_COMMAND_ENVELOPE_RECIPIENT, 0,
                                         NULL),
        NULL,
        NULL);
    ADD("no unknown",
        NULL,
        MILTER_ACTION_NONE,
        MILTER_STEP_NO_UNKNOWN,
        MILTER_STATUS_NOT_CHANGE,
        "pass",
        NULL,
        create_expected_command_receives(MILTER_COMMAND_UNKNOWN, 0,
                                         NULL),
        NULL,
        NULL);
    ADD("no header",
        NULL,
        MILTER_ACTION_NONE,
        MILTER_STEP_NO_HEADERS,
        MILTER_STATUS_NOT_CHANGE,
        "pass",
        NULL,
        create_expected_command_receives(MILTER_COMMAND_HEADER, 0,
                                         NULL),
        NULL,
        NULL);
    ADD("no end-of-header",
        NULL,
        MILTER_ACTION_NONE,
        MILTER_STEP_NO_END_OF_HEADER,
        MILTER_STATUS_NOT_CHANGE,
        "pass",
        NULL,
        create_expected_command_receives(MILTER_COMMAND_END_OF_HEADER, 0,
                                         NULL),
        NULL,
        NULL);
    ADD("no data",
        NULL,
        MILTER_ACTION_NONE,
        MILTER_STEP_NO_DATA,
        MILTER_STATUS_NOT_CHANGE,
        "pass",
        NULL,
        create_expected_command_receives(MILTER_COMMAND_DATA, 0,
                                         NULL),
        NULL,
        NULL);
    ADD("no body",
        NULL,
        MILTER_ACTION_NONE,
        MILTER_STEP_NO_BODY,
        MILTER_STATUS_NOT_CHANGE,
        "pass",
        NULL,
        create_expected_command_receives(MILTER_COMMAND_BODY, 0,
                                         NULL),
        NULL,
        NULL);
}

static void
accept_data_result (void)
{
    ADD("helo accept",
        NULL,
        MILTER_ACTION_NONE,
        MILTER_STEP_NONE,
        MILTER_STATUS_ACCEPT,
        "accept",
        create_replies(MILTER_COMMAND_HELO,
                       MILTER_REPLY_ACCEPT,
                       NULL),
        create_expected_command_receives(MILTER_COMMAND_HELO, 1,
                                         MILTER_COMMAND_ENVELOPE_FROM, 0,
                                         NULL),
        NULL,
        NULL);
}

static void
discard_data_result (void)
{
    ADD("envelope-from discard",
        NULL,
        MILTER_ACTION_NONE,
        MILTER_STEP_NONE,
        MILTER_STATUS_DISCARD,
        "discard",
        create_replies(MILTER_COMMAND_ENVELOPE_FROM,
                       MILTER_REPLY_DISCARD,
                       NULL),
        create_expected_command_receives(MILTER_COMMAND_ENVELOPE_FROM, 1,
                                         MILTER_COMMAND_ENVELOPE_RECIPIENT, 0,
                                         NULL),
        NULL,
        NULL);
}

static void
temporary_failure_data_result (void)
{
    ADD("data temporary-failure",
        NULL,
        MILTER_ACTION_NONE,
        MILTER_STEP_NONE,
        MILTER_STATUS_TEMPORARY_FAILURE,
        "temporary-failure",
        create_replies(MILTER_COMMAND_DATA,
                       MILTER_REPLY_TEMPORARY_FAILURE,
                       NULL),
        create_expected_command_receives(MILTER_COMMAND_DATA, 1,
                                         MILTER_COMMAND_HEADER, 0,
                                         NULL),
        NULL,
        NULL);
}

static void
reject_data_result (void)
{
    ADD("connect reject",
        NULL,
        MILTER_ACTION_NONE,
        MILTER_STEP_NONE,
        MILTER_STATUS_REJECT,
        "reject",
        create_replies(MILTER_COMMAND_CONNECT,
                       MILTER_REPLY_REJECT,
                       NULL),
        create_expected_command_receives(MILTER_COMMAND_CONNECT, 1,
                                         MILTER_COMMAND_HELO, 0,
                                         NULL),
        NULL,
        NULL);
    ADD("envelope-recipient reject",
        NULL,
        MILTER_ACTION_NONE,
        MILTER_STEP_NONE,
        MILTER_STATUS_REJECT,
        "reject",
        create_replies(MILTER_COMMAND_ENVELOPE_RECIPIENT, MILTER_REPLY_REJECT,
                       NULL),
        create_expected_command_receives(MILTER_COMMAND_ENVELOPE_RECIPIENT, 1,
                                         MILTER_COMMAND_DATA, 0,
                                         NULL),
        NULL,
        NULL);
}

static void
combined_data_result (void)
{
    ADD("envelope-recipient reject/continue",
        "-r recipient1@example.com -r recipient2@example.com",
        MILTER_ACTION_NONE,
        MILTER_STEP_NONE,
        MILTER_STATUS_NOT_CHANGE,
        "pass",
        create_replies(MILTER_COMMAND_ENVELOPE_RECIPIENT, MILTER_REPLY_REJECT,
                       MILTER_COMMAND_ENVELOPE_RECIPIENT, MILTER_REPLY_CONTINUE,
                       NULL),
        create_expected_command_receives(MILTER_COMMAND_ENVELOPE_RECIPIENT, 2,
                                         MILTER_COMMAND_DATA, 1,
                                         NULL),
        NULL,
        NULL);
    ADD("envelope-recipient continue/reject",
        "-r recipient1@example.com -r recipient2@example.com",
        MILTER_ACTION_NONE,
        MILTER_STEP_NONE,
        MILTER_STATUS_NOT_CHANGE,
        "pass",
        create_replies(MILTER_COMMAND_ENVELOPE_RECIPIENT, MILTER_REPLY_CONTINUE,
                       MILTER_COMMAND_ENVELOPE_RECIPIENT, MILTER_REPLY_REJECT,
                       NULL),
        create_expected_command_receives(MILTER_COMMAND_ENVELOPE_RECIPIENT, 2,
                                         MILTER_COMMAND_DATA, 1,
                                         NULL),
        NULL,
        NULL);
    ADD("envelope-recipient reject/reject",
        "-r recipient1@example.com -r recipient2@example.com",
        MILTER_ACTION_NONE,
        MILTER_STEP_NONE,
        MILTER_STATUS_REJECT,
        "reject",
        create_replies(MILTER_COMMAND_ENVELOPE_RECIPIENT, MILTER_REPLY_REJECT,
                       MILTER_COMMAND_ENVELOPE_RECIPIENT, MILTER_REPLY_REJECT,
                       NULL),
        create_expected_command_receives(MILTER_COMMAND_ENVELOPE_RECIPIENT, 2,
                                         MILTER_COMMAND_DATA, 0,
                                         NULL),
        NULL,
        NULL);
    ADD("envelope-recipient temporary-failure",
        NULL,
        MILTER_ACTION_NONE,
        MILTER_STEP_NONE,
        MILTER_STATUS_TEMPORARY_FAILURE,
        "temporary-failure",
        create_replies(MILTER_COMMAND_ENVELOPE_RECIPIENT,
                       MILTER_REPLY_TEMPORARY_FAILURE,
                       NULL),
        create_expected_command_receives(MILTER_COMMAND_ENVELOPE_RECIPIENT, 1,
                                         MILTER_COMMAND_DATA, 0,
                                         NULL),
        NULL,
        NULL);
    ADD("envelope-recipient temporary-failure/continue",
        "-r recipient1@example.com -r recipient2@example.com",
        MILTER_ACTION_NONE,
        MILTER_STEP_NONE,
        MILTER_STATUS_NOT_CHANGE,
        "pass",
        create_replies(MILTER_COMMAND_ENVELOPE_RECIPIENT,
                       MILTER_REPLY_TEMPORARY_FAILURE,
                       MILTER_COMMAND_ENVELOPE_RECIPIENT,
                       MILTER_REPLY_CONTINUE,
                       NULL),
        create_expected_command_receives(MILTER_COMMAND_ENVELOPE_RECIPIENT, 2,
                                         MILTER_COMMAND_DATA, 1,
                                         NULL),
        NULL,
        NULL);
    ADD("envelope-recipient continue/temporary-failure",
        "-r recipient1@example.com -r recipient2@example.com",
        MILTER_ACTION_NONE,
        MILTER_STEP_NONE,
        MILTER_STATUS_NOT_CHANGE,
        "pass",
        create_replies(MILTER_COMMAND_ENVELOPE_RECIPIENT,
                       MILTER_REPLY_CONTINUE,
                       MILTER_COMMAND_ENVELOPE_RECIPIENT,
                       MILTER_REPLY_TEMPORARY_FAILURE,
                       NULL),
        create_expected_command_receives(MILTER_COMMAND_ENVELOPE_RECIPIENT, 2,
                                         MILTER_COMMAND_DATA, 1,
                                         NULL),
        NULL,
        NULL);
    ADD("envelope-recipient temporary-failure/temporary-failure",
        "-r recipient1@example.com -r recipient2@example.com",
        MILTER_ACTION_NONE,
        MILTER_STEP_NONE,
        MILTER_STATUS_TEMPORARY_FAILURE,
        "temporary-failure",
        create_replies(MILTER_COMMAND_ENVELOPE_RECIPIENT,
                       MILTER_REPLY_TEMPORARY_FAILURE,
                       MILTER_COMMAND_ENVELOPE_RECIPIENT,
                       MILTER_REPLY_TEMPORARY_FAILURE,
                       NULL),
        create_expected_command_receives(MILTER_COMMAND_ENVELOPE_RECIPIENT, 2,
                                         MILTER_COMMAND_DATA, 0,
                                         NULL),
        NULL,
        NULL);
    ADD("envelope-recipient reject/temporary-failure",
        "-r recipient1@example.com -r recipient2@example.com",
        MILTER_ACTION_NONE,
        MILTER_STEP_NONE,
        MILTER_STATUS_TEMPORARY_FAILURE,
        "temporary-failure",
        create_replies(MILTER_COMMAND_ENVELOPE_RECIPIENT, MILTER_REPLY_REJECT,
                       MILTER_COMMAND_ENVELOPE_RECIPIENT,
                       MILTER_REPLY_TEMPORARY_FAILURE,
                       NULL),
        create_expected_command_receives(MILTER_COMMAND_ENVELOPE_RECIPIENT, 2,
                                         MILTER_COMMAND_DATA, 0,
                                         NULL),
        NULL,
        NULL);
    ADD("envelope-recipient temporary-failure/reject",
        "-r recipient1@example.com -r recipient2@example.com",
        MILTER_ACTION_NONE,
        MILTER_STEP_NONE,
        MILTER_STATUS_REJECT,
        "reject",
        create_replies(MILTER_COMMAND_ENVELOPE_RECIPIENT,
                       MILTER_REPLY_TEMPORARY_FAILURE,
                       MILTER_COMMAND_ENVELOPE_RECIPIENT, MILTER_REPLY_REJECT,
                       NULL),
        create_expected_command_receives(MILTER_COMMAND_ENVELOPE_RECIPIENT, 2,
                                         MILTER_COMMAND_DATA, 0,
                                         NULL),
        NULL,
        NULL);
}

static void
reply_code_data_result (void)
{
    ADD("reply-code - reject",
        NULL,
        MILTER_ACTION_NONE,
        MILTER_STEP_NONE,
        MILTER_STATUS_REJECT,
        "reject [554]<5.7.1>(1% 2%% 3%%%)",
        create_replies(MILTER_COMMAND_HELO,
                       MILTER_REPLY_REPLY_CODE | REJECT_FLAG,
                       NULL),
        create_expected_command_receives(MILTER_COMMAND_HELO, 1,
                                         MILTER_COMMAND_ENVELOPE_FROM, 0,
                                         NULL),
        NULL,
        NULL);
    ADD("reply-code - temporary failure",
        NULL,
        MILTER_ACTION_NONE,
        MILTER_STEP_NONE,
        MILTER_STATUS_TEMPORARY_FAILURE,
        "temporary-failure [451]<4.7.1>(Greylisting)",
        create_replies(MILTER_COMMAND_HELO,
                       MILTER_REPLY_REPLY_CODE | TEMPORARY_FAILURE_FLAG,
                       NULL),
        create_expected_command_receives(MILTER_COMMAND_HELO, 1,
                                         MILTER_COMMAND_ENVELOPE_FROM, 0,
                                         NULL),
        NULL,
        NULL);
}

static void
connection_failure_data_result (void)
{
    ADD("connection-failure",
        NULL,
        MILTER_ACTION_NONE,
        MILTER_STEP_NONE,
        MILTER_STATUS_ERROR,
        "error",
        create_replies(MILTER_COMMAND_HELO,
                       MILTER_REPLY_CONNECTION_FAILURE,
                       NULL),
        create_expected_command_receives(MILTER_COMMAND_HELO, 1,
                                         MILTER_COMMAND_ENVELOPE_FROM, 0,
                                         NULL),
        NULL,
        NULL);
}

static void
shutdown_data_result (void)
{
    ADD("shutdown",
        NULL,
        MILTER_ACTION_NONE,
        MILTER_STEP_NONE,
        MILTER_STATUS_ABORT,
        "abort",
        create_replies(MILTER_COMMAND_HELO,
                       MILTER_REPLY_SHUTDOWN,
                       NULL),
        create_expected_command_receives(MILTER_COMMAND_HELO, 1,
                                         MILTER_COMMAND_ENVELOPE_FROM, 0,
                                         NULL),
        NULL,
        NULL);
}

static void
no_reply_data_result (void)
{
    ADD("no reply connect",
        NULL,
        MILTER_ACTION_NONE,
        MILTER_STEP_NO_REPLY_CONNECT,
        MILTER_STATUS_NOT_CHANGE,
        "pass",
        NULL,
        NULL,
        NULL,
        NULL);
    ADD("no reply helo",
        NULL,
        MILTER_ACTION_NONE,
        MILTER_STEP_NO_REPLY_HELO,
        MILTER_STATUS_NOT_CHANGE,
        "pass",
        NULL,
        NULL,
        NULL,
        NULL);
    ADD("no reply envelope-from",
        NULL,
        MILTER_ACTION_NONE,
        MILTER_STEP_NO_REPLY_ENVELOPE_FROM,
        MILTER_STATUS_NOT_CHANGE,
        "pass",
        NULL,
        NULL,
        NULL,
        NULL);
    ADD("no reply envelope-recipient",
        NULL,
        MILTER_ACTION_NONE,
        MILTER_STEP_NO_REPLY_ENVELOPE_RECIPIENT,
        MILTER_STATUS_NOT_CHANGE,
        "pass",
        NULL,
        NULL,
        NULL,
        NULL);
    ADD("no reply unknown",
        NULL,
        MILTER_ACTION_NONE,
        MILTER_STEP_NO_REPLY_UNKNOWN,
        MILTER_STATUS_NOT_CHANGE,
        "pass",
        NULL,
        NULL,
        NULL,
        NULL);
    ADD("no reply header",
        NULL,
        MILTER_ACTION_NONE,
        MILTER_STEP_NO_REPLY_HEADER,
        MILTER_STATUS_NOT_CHANGE,
        "pass",
        NULL,
        NULL,
        NULL,
        NULL);
    ADD("no reply end-of-header",
        NULL,
        MILTER_ACTION_NONE,
        MILTER_STEP_NO_REPLY_END_OF_HEADER,
        MILTER_STATUS_NOT_CHANGE,
        "pass",
        NULL,
        NULL,
        NULL,
        NULL);
    ADD("no reply data",
        NULL,
        MILTER_ACTION_NONE,
        MILTER_STEP_NO_REPLY_DATA,
        MILTER_STATUS_NOT_CHANGE,
        "pass",
        NULL,
        NULL,
        NULL,
        NULL);
    ADD("no reply body",
        NULL,
        MILTER_ACTION_NONE,
        MILTER_STEP_NO_REPLY_BODY,
        MILTER_STATUS_NOT_CHANGE,
        "pass",
        NULL,
        NULL,
        NULL,
        NULL);
}

void
data_result (void)
{
    all_data_result();
    no_data_result();
    accept_data_result();
    discard_data_result();
    temporary_failure_data_result();
    reject_data_result();
    combined_data_result();
    reply_code_data_result();
    connection_failure_data_result();
    shutdown_data_result();
    no_reply_data_result();
}
#undef ADD

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

    cut_set_message("%s should be received %d times but %d times received",
                    gcut_enum_inspect(MILTER_TYPE_COMMAND, command),
                    n_expected, n_actual);
    cut_assert_equal_int(n_expected, n_actual);
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
    GError *error = NULL;
    TestData *test_data = (TestData *)data;

    setup_test_client("inet:9999@localhost", test_data);
    setup_server("inet:9999@localhost", test_data->option_string);

    gcut_process_run(server, &error);
    gcut_assert_error(error);

    wait_for_reaping(TRUE);
    cut_assert_equal_int(EXIT_SUCCESS, exit_status);

    gcut_assert_equal_enum(MILTER_TYPE_STATUS,
                           test_data->expected_status, actual_status);
    cut_assert_equal_string(test_data->expected_status_string,
                            actual_status_string);

    cut_trace(milter_assert_equal_n_received(test_data->expected_command_receives));
}

typedef void (*AssertOptionValueFunc) (void);

typedef struct _OptionTestData
{
    TestData *test_data;
    AssertOptionValueFunc assert_func;
} OptionTestData;

static OptionTestData *
option_test_data_new (const gchar *option_string,
                      AssertOptionValueFunc assert_func)
{
    OptionTestData *option_test_data;

    option_test_data = g_new0(OptionTestData, 1);
    option_test_data->test_data = result_test_data_new(option_string,
                                                       MILTER_ACTION_NONE,
                                                       MILTER_STEP_NONE,
                                                       MILTER_STATUS_NOT_CHANGE,
                                                       NULL,
                                                       NULL,
                                                       NULL,
                                                       NULL,
                                                       NULL);
    option_test_data->assert_func = assert_func;

    return option_test_data;
}

static void
option_test_data_free (OptionTestData *option_test_data)
{
    result_test_data_free(option_test_data->test_data);
    g_free(option_test_data);
}

static void
option_test_assert_connect_host (void)
{
    cut_assert_equal_string("test-host", actual_connect_host);
    cut_assert_equal_string("inet:50443@[192.168.123.123]",
                            actual_connect_address);
}

static void
option_test_assert_connect_address (void)
{
    cut_assert_equal_string("mx.example.net", actual_connect_host);
    cut_assert_equal_string("inet:12345@[192.168.1.1]", actual_connect_address);
}

static void
option_test_assert_connect (void)
{
    cut_assert_equal_string("test-host", actual_connect_host);
    cut_assert_equal_string("inet:12345@[192.168.1.1]", actual_connect_address);
}

static void
option_test_assert_helo_fqdn (void)
{
    cut_assert_equal_string("test-host", actual_helo_fqdn);
}

static void
option_test_assert_from (void)
{
    cut_assert_equal_string("<from@example.com>", actual_envelope_from);
}

static void
option_test_assert_recipients (void)
{
    gcut_assert_equal_list_string(
        gcut_take_new_list_string("<recipient1@example.com>",
                                  "<recipient2@example.com>",
                                  "<recipient3@example.com>",
                                  NULL),
        actual_recipients);
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
option_test_assert_mail_file_lf (void)
{
    cut_assert_equal_string("<test-from@example.com>", actual_envelope_from);
    gcut_assert_equal_list_string(
        gcut_take_new_list_string("<test-to@example.com>",
                                  NULL),
        actual_recipients);

    milter_headers_append_header(expected_headers,
                                 "Return-Path",
                                 "<xxxx@example.com>");
    milter_headers_append_header(expected_headers,
                                 "Date",
                                 "Wed, 5 Nov 2008 11:41:54 -0200");
    milter_headers_append_header(expected_headers,
                                 "Message-ID",
                                 "<168f979z.5435357@xxx>");
    milter_headers_append_header(expected_headers,
                                 "Content-Type",
                                 "text/plain; charset=\"us-ascii\"");
    milter_headers_append_header(expected_headers,
                                 "X-Source",
                                 "");
    milter_headers_append_header(expected_headers,
                                 "To",
                                 "Test Sender <test-to@example.com>");
    milter_headers_append_header(expected_headers,
                                 "From",
                                 "test-from@example.com");
    milter_headers_append_header(expected_headers,
                                 "List-Archive",
                                 "\n <http://sourceforge.net/mailarchive/forum.php?forum_name=milter-manager-commit>");
    milter_headers_append_header(expected_headers,
                                 "Subject",
                                 "This is a test mail");
    milter_headers_append_header(expected_headers,
                                 "List-Help",
                                 "\n <mailto:milter-manager-commit-request@lists.sourceforge.net?subject=help>");
    milter_headers_append_header(expected_headers,
                                 "Authentication-Results",
                                 "mail.example.com;\n"
                                 "\tspf=none smtp.mailfrom=test-from@example.com;\n"
                                 "\tsender-id=none header.From=test-from@example.com;\n"
                                 "\tdkim=none;\n"
                                 "\tdkim-adsp=none header.From=test-from@example.com");
    milter_headers_append_header(expected_headers,
                                 "Authentication-Results",
                                 "mail.example.org;\n"
                                 "\tspf=none smtp.mailfrom=test-from@example.com;\n"
                                 "\tsender-id=none header.From=test-from@example.com;\n"
                                 "\tdkim=none;\n"
                                 "\tdkim-adsp=none header.From=test-from@example.com");

    milter_assert_equal_headers(expected_headers, actual_headers);

    cut_assert_equal_string("Message body\nMessage body\nMessage body\n",
                            actual_body_string->str);
}

static void
option_test_assert_mail_file_crlf (void)
{
    cut_assert_equal_string("<test-from@example.com>", actual_envelope_from);
    gcut_assert_equal_list_string(
        gcut_take_new_list_string("<test-to@example.com>",
                                  NULL),
        actual_recipients);

    milter_headers_append_header(expected_headers,
                                 "Return-Path",
                                 "<xxxx@example.com>");
    milter_headers_append_header(expected_headers,
                                 "Date",
                                 "Wed, 5 Nov 2008 11:41:54 -0200");
    milter_headers_append_header(expected_headers,
                                 "Message-ID",
                                 "<168f979z.5435357@xxx>");
    milter_headers_append_header(expected_headers,
                                 "Content-Type",
                                 "text/plain; charset=\"us-ascii\"");
    milter_headers_append_header(expected_headers,
                                 "X-Source",
                                 "");
    milter_headers_append_header(expected_headers,
                                 "To",
                                 "Test Sender <test-to@example.com>");
    milter_headers_append_header(expected_headers,
                                 "From",
                                 "test-from@example.com");
    milter_headers_append_header(expected_headers,
                                 "List-Archive",
                                 "\r\n <http://sourceforge.net/mailarchive/forum.php?forum_name=milter-manager-commit>");
    milter_headers_append_header(expected_headers,
                                 "Subject",
                                 "This is a test mail");
    milter_headers_append_header(expected_headers,
                                 "List-Help",
                                 "\r\n <mailto:milter-manager-commit-request@lists.sourceforge.net?subject=help>");
    milter_headers_append_header(expected_headers,
                                 "Authentication-Results",
                                 "mail.example.com;\r\n"
                                 "\tspf=none smtp.mailfrom=test-from@example.com;\r\n"
                                 "\tsender-id=none header.From=test-from@example.com;\r\n"
                                 "\tdkim=none;\r\n"
                                 "\tdkim-adsp=none header.From=test-from@example.com");
    milter_headers_append_header(expected_headers,
                                 "Authentication-Results",
                                 "mail.example.org;\r\n"
                                 "\tspf=none smtp.mailfrom=test-from@example.com;\r\n"
                                 "\tsender-id=none header.From=test-from@example.com;\r\n"
                                 "\tdkim=none;\r\n"
                                 "\tdkim-adsp=none header.From=test-from@example.com");

    milter_assert_equal_headers(expected_headers, actual_headers);

    cut_assert_equal_string("Message body\r\nMessage body\r\nMessage body\r\n",
                            actual_body_string->str);
}

static void
option_test_assert_mail_file_folded_to (void)
{
    gcut_assert_equal_list_string(
        gcut_take_new_list_string("<test-to@example.com>",
                                  NULL),
        actual_recipients);
}

static void
option_test_assert_mail_file_folded_from (void)
{
    cut_assert_equal_string("<test-from@example.com>", actual_envelope_from);
}

#define RED_COLOR "\033[01;31m"
#define GREEN_COLOR "\033[01;32m"
#define NORMAL_COLOR "\033[00m"

static void
option_test_assert_output_message (void)
{
    cut_assert_equal_string(
        "status: pass\n"
        "elapsed-time: XXX seconds\n"
        "\n"
        "Envelope:------------------------------\n"
        "  MAIL FROM:<sender@example.com>" "\n"
        "  RCPT TO:<receiver@example.org>" "\n"
        "Header:--------------------------------\n"
        "  From: <sender@example.com>" "\n"
        "  To: <receiver@example.org>" "\n"
        "Body:----------------------------------\n"
        "La de da de da 1.\n"
        "La de da de da 2.\n"
        "La de da de da 3.\n"
        "La de da de da 4."
        "\n",
        normalized_output_string->str);
}

static void
option_test_assert_output_message_charset (void)
{
    cut_assert_equal_string(
        "status: pass\n"
        "elapsed-time: XXX seconds\n"
        "\n"
        "Envelope:------------------------------\n"
        "  MAIL FROM:<sender@example.com>" "\n"
        "  RCPT TO:<receiver@example.org>" "\n"
        "Header:--------------------------------\n"
        "  Content-Type: text/plain; charset=\"us-ascii\"" "\n"
        "  From: <sender@example.com>" "\n"
        "  To: <receiver@example.org>" "\n"
        "Body:----------------------------------\n"
        "La de da de da 1.\n"
        "La de da de da 2.\n"
        "La de da de da 3.\n"
        "La de da de da 4."
        "\n",
        normalized_output_string->str);
}

static void
option_test_assert_large_mail (void)
{
    const gchar expected_header[] = "\n--=-fXjrl3xPRPdvOSpDlnzU\n";
    const gchar expected_footer[] = "\n--=-fXjrl3xPRPdvOSpDlnzU--\n";

    cut_assert_equal_memory(expected_header, strlen(expected_header),
                            actual_body_string->str, strlen(expected_header));
    cut_assert_equal_memory(expected_footer, strlen(expected_footer),
                            actual_body_string->str +
                            actual_body_string->len -
                            strlen(expected_footer),
                            strlen(expected_footer));
    cut_assert_equal_int(2404259, actual_body_string->len);
}

static void
option_test_assert_authenticated_info (const gchar *key, const gchar *value)
{
    GHashTable *actual_macros;
    GHashTable *expected_macros;

    actual_macros =
        g_hash_table_lookup(actual_defined_macros,
                            GINT_TO_POINTER(MILTER_COMMAND_ENVELOPE_FROM));
    expected_macros =
        gcut_take_new_hash_table_string_string("i", "i",
                                               "mail_mailer", "mail_mailer",
                                               "mail_host", "mail_host",
                                               "mail_addr", "mail_addr",
                                               key, value,
                                               NULL);

    gcut_assert_equal_hash_table_string_string(expected_macros,
                                               actual_macros);
}

static void
option_test_assert_authenticated_name (void)
{
    cut_trace(option_test_assert_authenticated_info("auth_authen", "user"));
}

static void
option_test_assert_authenticated_type (void)
{
    cut_trace(option_test_assert_authenticated_info("auth_type", "PLAIN"));
}

static void
option_test_assert_authenticated_author (void)
{
    cut_trace(option_test_assert_authenticated_info("auth_author",
                                                    "another-user@example.com"));
}

static const gchar *
mail_file_option (const gchar *file_name)
{
    gchar *file_path;

    file_path = g_build_filename(fixtures_path, file_name, NULL);
    cut_take_string(file_path);
    return cut_take_printf("--mail-file=%s", file_path);
}

void
data_option (void)
{
#define ADD(label, option, ...)                                 \
    cut_add_data(label,                                         \
                 option_test_data_new(option, __VA_ARGS__),     \
                 option_test_data_free,                         \
                 NULL)

    ADD("connect-host",
        "--connect-host=test-host",
        option_test_assert_connect_host);
    ADD("connect-address",
        "--connect-address=inet:12345@192.168.1.1",
        option_test_assert_connect_address);
    ADD("connect",
        "--connect-host=test-host "
        "--connect-address=inet:12345@192.168.1.1",
        option_test_assert_connect);
    ADD("helo-fqdn",
        "--helo-fqdn=test-host",
        option_test_assert_helo_fqdn);
    ADD("envelope-from",
        "--envelope-from=from@example.com",
        option_test_assert_from);
    ADD("envelope-recipients",
        "--envelope-recipient=recipient1@example.com "
        "--envelope-recipient=recipient2@example.com "
        "--envelope-recipient=recipient3@example.com",
        option_test_assert_recipients);
    ADD("headers",
        "--header=X-TestHeader1:TestHeader1Value "
        "--header=X-TestHeader2:TestHeader2Value "
        "--header=X-TestHeader3:TestHeader3Value",
        option_test_assert_headers);
    ADD("body",
        "--body=chunk1 "
        "--body=chunk2 "
        "--body=chunk3",
        option_test_assert_body);
    ADD("output-message",
        "--output-message",
        option_test_assert_output_message);
    ADD("output-message - charset",
        "--header='Content-Type:text/plain; charset=\"us-ascii\"' "
        "--output-message",
        option_test_assert_output_message_charset);
    ADD("mail-file - LF",
        mail_file_option("parse-test-lf.mail"),
        option_test_assert_mail_file_lf);
    ADD("mail-file - CRLF",
        mail_file_option("parse-test-crlf.mail"),
        option_test_assert_mail_file_crlf);
    ADD("mail-file - folded headers - To",
        mail_file_option("folded-to.mail"),
        option_test_assert_mail_file_folded_to);
    ADD("mail-file - folded headers - From",
        mail_file_option("folded-from.mail"),
        option_test_assert_mail_file_folded_from);
    ADD("large-file",
        mail_file_option("large.mail"),
        option_test_assert_large_mail);
    ADD("authentication-name",
        "--authenticated-name=user",
        option_test_assert_authenticated_name);
    ADD("authentication-type",
        "--authenticated-type=PLAIN",
        option_test_assert_authenticated_type);
    ADD("authentication-author",
        "--authenticated-author=another-user@example.com",
        option_test_assert_authenticated_author);
#undef ADD
}

void
test_option (gconstpointer data)
{
    GError *error = NULL;
    OptionTestData *option_test_data = (OptionTestData*)data;

    setup_test_client("inet:9999@localhost", option_test_data->test_data);
    setup_server("inet:9999@localhost",
                 option_test_data->test_data->option_string);

    gcut_process_run(server, &error);
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
                                     AssertFunc assert_func,
                                     MilterStatus status,
                                     const gchar *status_string)
{
    EndOfMessageActionTestData *test_data;

    test_data = g_new0(EndOfMessageActionTestData, 1);
    test_data->test_data = result_test_data_new(NULL,
                                                MILTER_ACTION_NONE,
                                                MILTER_STEP_NONE,
                                                status,
                                                status_string,
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
    const gchar *packet;
    gsize packet_size;

    milter_reply_encoder_encode_change_from(encoder, &packet, &packet_size,
                                            "sender+changed@example.com", NULL);

    write_data(packet, packet_size);
}

static void
end_of_message_action_assert_change_from (void)
{
    cut_assert_equal_string(
        "status: pass\n"
        "elapsed-time: XXX seconds\n"
        "\n"
        "Envelope:------------------------------\n"
        RED_COLOR "- MAIL FROM:<sender@example.com>" NORMAL_COLOR "\n"
        GREEN_COLOR "+ MAIL FROM:<sender+changed@example.com>" NORMAL_COLOR "\n"
        "  RCPT TO:<receiver@example.org>" "\n"
        "Header:--------------------------------\n"
        "  From: <sender@example.com>" "\n"
        "  To: <receiver@example.org>" "\n"
        "Body:----------------------------------\n"
        "La de da de da 1.\n"
        "La de da de da 2.\n"
        "La de da de da 3.\n"
        "La de da de da 4."
        "\n",
        normalized_output_string->str);
}

static void
add_recipient_function (void)
{
    const gchar *packet;
    gsize packet_size;

    milter_reply_encoder_encode_add_recipient(encoder, &packet, &packet_size,
                                              "receiver+added@example.org",
                                              NULL);

    write_data(packet, packet_size);
}

static void
end_of_message_action_assert_add_recipient (void)
{
    cut_assert_equal_string(
        "status: pass\n"
        "elapsed-time: XXX seconds\n"
        "\n"
        "Envelope:------------------------------\n"
        "  MAIL FROM:<sender@example.com>" "\n"
        "  RCPT TO:<receiver@example.org>" "\n"
        GREEN_COLOR "+ RCPT TO:<receiver+added@example.org>" NORMAL_COLOR "\n"
        "Header:--------------------------------\n"
        "  From: <sender@example.com>" "\n"
        "  To: <receiver@example.org>" "\n"
        "Body:----------------------------------\n"
        "La de da de da 1.\n"
        "La de da de da 2.\n"
        "La de da de da 3.\n"
        "La de da de da 4."
        "\n",
        normalized_output_string->str);
}

static void
delete_recipient_function (void)
{
    const gchar *packet;
    gsize packet_size;

    milter_reply_encoder_encode_delete_recipient(encoder, &packet, &packet_size,
                                                 "receiver@example.org");

    write_data(packet, packet_size);
}

static void
end_of_message_action_assert_delete_recipient (void)
{
    cut_assert_equal_string(
        "status: pass\n"
        "elapsed-time: XXX seconds\n"
        "\n"
        "Envelope:------------------------------\n"
        "  MAIL FROM:<sender@example.com>" "\n"
        RED_COLOR "- RCPT TO:<receiver@example.org>" NORMAL_COLOR "\n"
        "Header:--------------------------------\n"
        "  From: <sender@example.com>" "\n"
        "  To: <receiver@example.org>" "\n"
        "Body:----------------------------------\n"
        "La de da de da 1.\n"
        "La de da de da 2.\n"
        "La de da de da 3.\n"
        "La de da de da 4."
        "\n",
        normalized_output_string->str);
}

static void
add_header_function (void)
{
    const gchar *packet;
    gsize packet_size;

    milter_reply_encoder_encode_add_header(encoder, &packet, &packet_size,
                                           "X-Test-Header1", "TestHeader1Value");

    write_data(packet, packet_size);
}

static void
end_of_message_action_assert_add_header (void)
{
    cut_assert_equal_string(
        "status: pass\n"
        "elapsed-time: XXX seconds\n"
        "\n"
        "Envelope:------------------------------\n"
        "  MAIL FROM:<sender@example.com>" "\n"
        "  RCPT TO:<receiver@example.org>" "\n"
        "Header:--------------------------------\n"
        "  From: <sender@example.com>" "\n"
        "  To: <receiver@example.org>" "\n"
        GREEN_COLOR "+ X-Test-Header1: TestHeader1Value" NORMAL_COLOR "\n"
        "Body:----------------------------------\n"
        "La de da de da 1.\n"
        "La de da de da 2.\n"
        "La de da de da 3.\n"
        "La de da de da 4."
        "\n",
        normalized_output_string->str);
}

static void
insert_header_function (void)
{
    const gchar *packet;
    gsize packet_size;

    milter_reply_encoder_encode_insert_header(encoder, &packet, &packet_size,
                                              1, "To",
                                              "<receiver+added@example.org>");

    write_data(packet, packet_size);
}

static void
end_of_message_action_assert_insert_header (void)
{
    cut_assert_equal_string(
        "status: pass\n"
        "elapsed-time: XXX seconds\n"
        "\n"
        "Envelope:------------------------------\n"
        "  MAIL FROM:<sender@example.com>" "\n"
        "  RCPT TO:<receiver@example.org>" "\n"
        "Header:--------------------------------\n"
        "  From: <sender@example.com>" "\n"
        GREEN_COLOR "+ To: <receiver+added@example.org>" NORMAL_COLOR "\n"
        "  To: <receiver@example.org>" "\n"
        "Body:----------------------------------\n"
        "La de da de da 1.\n"
        "La de da de da 2.\n"
        "La de da de da 3.\n"
        "La de da de da 4."
        "\n",
        normalized_output_string->str);
}

static void
change_header_function (void)
{
    const gchar *packet;
    gsize packet_size;

    milter_reply_encoder_encode_change_header(encoder, &packet, &packet_size,
                                              "To", 1,
                                              "<receiver+changed@example.org>");

    write_data(packet, packet_size);
}

static void
end_of_message_action_assert_change_header (void)
{
    cut_assert_equal_string(
        "status: pass\n"
        "elapsed-time: XXX seconds\n"
        "\n"
        "Envelope:------------------------------\n"
        "  MAIL FROM:<sender@example.com>" "\n"
        "  RCPT TO:<receiver@example.org>" "\n"
        "Header:--------------------------------\n"
        "  From: <sender@example.com>" "\n"
        GREEN_COLOR "+ To: <receiver+changed@example.org>" NORMAL_COLOR "\n"
        RED_COLOR "- To: <receiver@example.org>" NORMAL_COLOR "\n"
        "Body:----------------------------------\n"
        "La de da de da 1.\n"
        "La de da de da 2.\n"
        "La de da de da 3.\n"
        "La de da de da 4."
        "\n",
        normalized_output_string->str);
}

static void
delete_header_function (void)
{
    const gchar *packet;
    gsize packet_size;

    milter_reply_encoder_encode_delete_header(encoder, &packet, &packet_size,
                                              "To", 1);

    write_data(packet, packet_size);
}

static void
end_of_message_action_assert_delete_header (void)
{
    cut_assert_equal_string(
        "status: pass\n"
        "elapsed-time: XXX seconds\n"
        "\n"
        "Envelope:------------------------------\n"
        "  MAIL FROM:<sender@example.com>" "\n"
        "  RCPT TO:<receiver@example.org>" "\n"
        "Header:--------------------------------\n"
        "  From: <sender@example.com>" "\n"
        RED_COLOR "- To: <receiver@example.org>" NORMAL_COLOR "\n"
        "Body:----------------------------------\n"
        "La de da de da 1.\n"
        "La de da de da 2.\n"
        "La de da de da 3.\n"
        "La de da de da 4."
        "\n",
        normalized_output_string->str);
}

static void
replace_body_function (void)
{
    const gchar *packet;
    gsize packet_size;
    const gchar replace_string[] = "This is the replaced message.";
    gsize packed_size;

    milter_reply_encoder_encode_replace_body(encoder, &packet, &packet_size,
                                             replace_string,
                                             strlen(replace_string),
                                             &packed_size);

    write_data(packet, packet_size);
}

static void
end_of_message_action_assert_replace_body (void)
{
    cut_assert_equal_string(
        "status: pass\n"
        "elapsed-time: XXX seconds\n"
        "\n"
        "Envelope:------------------------------\n"
        "  MAIL FROM:<sender@example.com>" "\n"
        "  RCPT TO:<receiver@example.org>" "\n"
        "Header:--------------------------------\n"
        "  From: <sender@example.com>" "\n"
        "  To: <receiver@example.org>" "\n"
        "Body:----------------------------------\n"
        "This is the replaced message.\n",
        normalized_output_string->str);
}

static void
quarantine_function (void)
{
    const gchar *packet;
    gsize packet_size;
    const gchar reason[] = "Virus is found.";

    milter_reply_encoder_encode_quarantine(encoder, &packet, &packet_size,
                                           reason);
    write_data(packet, packet_size);
}

static void
end_of_message_action_assert_quarantine (void)
{
    cut_assert_equal_string(
        "status: quarantine\n"
        "elapsed-time: XXX seconds\n"
        "Quarantine reason: <Virus is found.>\n"
        "\n"
        "Envelope:------------------------------\n"
        "  MAIL FROM:<sender@example.com>" "\n"
        "  RCPT TO:<receiver@example.org>" "\n"
        "Header:--------------------------------\n"
        "  From: <sender@example.com>" "\n"
        "  To: <receiver@example.org>" "\n"
        "Body:----------------------------------\n"
        "La de da de da 1.\n"
        "La de da de da 2.\n"
        "La de da de da 3.\n"
        "La de da de da 4."
        "\n",
        normalized_output_string->str);
}

void
data_end_of_message_action (void)
{
#define ADD(label, action_func, assert_func, status, status_string)     \
    cut_add_data(label,                                                 \
                 end_of_message_action_test_data_new(action_func,       \
                                                     assert_func,       \
                                                     status,            \
                                                     status_string),    \
                 end_of_message_action_test_data_free,                  \
                 NULL)

    ADD("change-from",
        change_from_function,
        end_of_message_action_assert_change_from,
        MILTER_STATUS_NOT_CHANGE,
        "pass");
    ADD("add-recipient",
        add_recipient_function,
        end_of_message_action_assert_add_recipient,
        MILTER_STATUS_NOT_CHANGE,
        "pass");
    ADD("delete-recipient",
        delete_recipient_function,
        end_of_message_action_assert_delete_recipient,
        MILTER_STATUS_NOT_CHANGE,
        "pass");
    ADD("add-header",
        add_header_function,
        end_of_message_action_assert_add_header,
        MILTER_STATUS_NOT_CHANGE,
        "pass");
    ADD("insert-header",
        insert_header_function,
        end_of_message_action_assert_insert_header,
        MILTER_STATUS_NOT_CHANGE,
        "pass");
    ADD("change-header",
        change_header_function,
        end_of_message_action_assert_change_header,
        MILTER_STATUS_NOT_CHANGE,
        "pass");
    ADD("delete-header",
        delete_header_function,
        end_of_message_action_assert_delete_header,
        MILTER_STATUS_NOT_CHANGE,
        "pass");
    ADD("replace-body",
        replace_body_function,
        end_of_message_action_assert_replace_body,
        MILTER_STATUS_NOT_CHANGE,
        "pass");
    ADD("quarantine",
        quarantine_function,
        end_of_message_action_assert_quarantine,
        MILTER_STATUS_QUARANTINE,
        "quarantine");
#undef ADD
}

void
test_end_of_message_action (gconstpointer data)
{
    GError *error = NULL;
    EndOfMessageActionTestData *action_test_data;

    action_test_data = (EndOfMessageActionTestData*)data;

    setup_test_client("inet:9999@localhost", action_test_data->test_data);
    setup_server("inet:9999@localhost", "--output-message");

    gcut_process_run(server, &error);
    gcut_assert_error(error);

    wait_for_reaping(TRUE);
    cut_assert_equal_int(EXIT_SUCCESS, exit_status);

    gcut_assert_equal_enum(MILTER_TYPE_STATUS,
                           action_test_data->test_data->expected_status,
                           actual_status);
    cut_assert_equal_string(action_test_data->test_data->expected_status_string,
                            actual_status_string);

    action_test_data->assert_func();
}

void
data_macro (void)
{
#define ADD(label, expected, arguments)                         \
    gcut_add_datum(label,                                       \
                   "expected", G_TYPE_POINTER, expected,        \
                   gcut_list_string_free,                       \
                   "arguments", G_TYPE_STRING, arguments,       \
                   NULL)

    ADD("default",
        gcut_list_string_new("command:connect",
                             "daemon_name", "milter-test-server",
                             "if_addr", "127.0.0.1",
                             "if_name", "localhost",
                             "j", "mail.example.com",
                             "command:helo",
                             "cert_issuer", "cert_issuer",
                             "cert_subject", "cert_subject",
                             "cipher", "0",
                             "cipher_bits", "0",
                             "tls_version", "0",
                             "command:envelope-from",
                             "i", "i",
                             "mail_addr", "mail_addr",
                             "mail_host", "mail_host",
                             "mail_mailer", "mail_mailer",
                             "command:envelope-recipient",
                             "rcpt_addr", "<receiver@example.org>",
                             "rcpt_host", "rcpt_host",
                             "rcpt_mailer", "rcpt_mailer",
                             "command:end-of-message",
                             "i", "message-id",
                             NULL),
        NULL);

    ADD("connect",
        gcut_list_string_new("command:connect",
                             "client_connections", "10",
                             "daemon_name", "milter-test-server",
                             "if_addr", "127.0.0.1",
                             "if_name", "localhost",
                             "j", "mail.example.com",
                             "command:helo",
                             "cert_issuer", "cert_issuer",
                             "cert_subject", "cert_subject",
                             "cipher", "0",
                             "cipher_bits", "0",
                             "tls_version", "0",
                             "command:envelope-from",
                             "i", "i",
                             "mail_addr", "mail_addr",
                             "mail_host", "mail_host",
                             "mail_mailer", "mail_mailer",
                             "command:envelope-recipient",
                             "rcpt_addr", "<receiver@example.org>",
                             "rcpt_host", "rcpt_host",
                             "rcpt_mailer", "rcpt_mailer",
                             "command:end-of-message",
                             "i", "message-id",
                             NULL),
        "--connect-macro client_connections:10");

    ADD("helo",
        gcut_list_string_new("command:connect",
                             "daemon_name", "milter-test-server",
                             "if_addr", "127.0.0.1",
                             "if_name", "localhost",
                             "j", "mail.example.com",
                             "command:helo",
                             "cert_issuer", "cert_issuer",
                             "cert_subject", "cert_subject",
                             "cipher", "0",
                             "cipher_bits", "0",
                             "client_ptr", "unknown",
                             "tls_version", "0",
                             "command:envelope-from",
                             "i", "i",
                             "mail_addr", "mail_addr",
                             "mail_host", "mail_host",
                             "mail_mailer", "mail_mailer",
                             "command:envelope-recipient",
                             "rcpt_addr", "<receiver@example.org>",
                             "rcpt_host", "rcpt_host",
                             "rcpt_mailer", "rcpt_mailer",
                             "command:end-of-message",
                             "i", "message-id",
                             NULL),
        "--helo-macro client_ptr:unknown");

    ADD("envelope-from",
        gcut_list_string_new("command:connect",
                             "daemon_name", "milter-test-server",
                             "if_addr", "127.0.0.1",
                             "if_name", "localhost",
                             "j", "mail.example.com",
                             "command:helo",
                             "cert_issuer", "cert_issuer",
                             "cert_subject", "cert_subject",
                             "cipher", "0",
                             "cipher_bits", "0",
                             "tls_version", "0",
                             "command:envelope-from",
                             "client_addr", "192.168.0.3",
                             "client_name", "local-sender.example.net",
                             "i", "i",
                             "mail_addr", "mail_addr",
                             "mail_host", "mail_host",
                             "mail_mailer", "mail_mailer",
                             "command:envelope-recipient",
                             "rcpt_addr", "<receiver@example.org>",
                             "rcpt_host", "rcpt_host",
                             "rcpt_mailer", "rcpt_mailer",
                             "command:end-of-message",
                             "i", "message-id",
                             NULL),
        "--envelope-from-macro client_addr:192.168.0.3 "
        "--envelope-from-macro client_name:local-sender.example.net");

    ADD("envelope-recipient-macro",
        gcut_list_string_new("command:connect",
                             "daemon_name", "milter-test-server",
                             "if_addr", "127.0.0.1",
                             "if_name", "localhost",
                             "j", "mail.example.com",
                             "command:helo",
                             "cert_issuer", "cert_issuer",
                             "cert_subject", "cert_subject",
                             "cipher", "0",
                             "cipher_bits", "0",
                             "tls_version", "0",
                             "command:envelope-from",
                             "i", "i",
                             "mail_addr", "mail_addr",
                             "mail_host", "mail_host",
                             "mail_mailer", "mail_mailer",
                             "command:envelope-recipient",
                             "client_port", "2929",
                             "rcpt_addr", "<receiver@example.org>",
                             "rcpt_host", "rcpt_host",
                             "rcpt_mailer", "rcpt_mailer",
                             "command:end-of-message",
                             "i", "message-id",
                             NULL),
        "--envelope-recipient-macro client_port:2929");

    ADD("data",
        gcut_list_string_new("command:connect",
                             "daemon_name", "milter-test-server",
                             "if_addr", "127.0.0.1",
                             "if_name", "localhost",
                             "j", "mail.example.com",
                             "command:helo",
                             "cert_issuer", "cert_issuer",
                             "cert_subject", "cert_subject",
                             "cipher", "0",
                             "cipher_bits", "0",
                             "tls_version", "0",
                             "command:envelope-from",
                             "i", "i",
                             "mail_addr", "mail_addr",
                             "mail_host", "mail_host",
                             "mail_mailer", "mail_mailer",
                             "command:envelope-recipient",
                             "rcpt_addr", "<receiver@example.org>",
                             "rcpt_host", "rcpt_host",
                             "rcpt_mailer", "rcpt_mailer",
                             "command:data",
                             "i", "queue-id-2929",
                             "command:end-of-message",
                             "i", "message-id",
                             NULL),
        "--data-macro i:queue-id-2929");

    ADD("end-of-header",
        gcut_list_string_new("command:connect",
                             "daemon_name", "milter-test-server",
                             "if_addr", "127.0.0.1",
                             "if_name", "localhost",
                             "j", "mail.example.com",
                             "command:helo",
                             "cert_issuer", "cert_issuer",
                             "cert_subject", "cert_subject",
                             "cipher", "0",
                             "cipher_bits", "0",
                             "tls_version", "0",
                             "command:envelope-from",
                             "i", "i",
                             "mail_addr", "mail_addr",
                             "mail_host", "mail_host",
                             "mail_mailer", "mail_mailer",
                             "command:envelope-recipient",
                             "rcpt_addr", "<receiver@example.org>",
                             "rcpt_host", "rcpt_host",
                             "rcpt_mailer", "rcpt_mailer",
                             "command:end-of-header",
                             "n-headers", "100",
                             "command:end-of-message",
                             "i", "message-id",
                             NULL),
        "--end-of-header-macro n-headers:100");

    ADD("end-of-message",
        gcut_list_string_new("command:connect",
                             "daemon_name", "milter-test-server",
                             "if_addr", "127.0.0.1",
                             "if_name", "localhost",
                             "j", "mail.example.com",
                             "command:helo",
                             "cert_issuer", "cert_issuer",
                             "cert_subject", "cert_subject",
                             "cipher", "0",
                             "cipher_bits", "0",
                             "tls_version", "0",
                             "command:envelope-from",
                             "i", "i",
                             "mail_addr", "mail_addr",
                             "mail_host", "mail_host",
                             "mail_mailer", "mail_mailer",
                             "command:envelope-recipient",
                             "rcpt_addr", "<receiver@example.org>",
                             "rcpt_host", "rcpt_host",
                             "rcpt_mailer", "rcpt_mailer",
                             "command:end-of-message",
                             "elapsed", "0.29",
                             "i", "message-id",
                             NULL),
        "--end-of-message-macro elapsed:0.29");

#undef ADD
}

void
test_macro (gconstpointer data)
{
    GError *error = NULL;

    setup_test_client("inet:9999@localhost", NULL);
    setup_server("inet:9999@localhost",
                 gcut_data_get_pointer(data, "arguments"));

    gcut_process_run(server, &error);
    gcut_assert_error(error);

    wait_for_reaping(TRUE);
    cut_assert_equal_int(EXIT_SUCCESS, exit_status);

    gcut_assert_equal_enum(MILTER_TYPE_STATUS,
                           MILTER_STATUS_NOT_CHANGE,
                           actual_status);

    gcut_assert_equal_list_string(gcut_data_get_pointer(data, "expected"),
                                  actual_defined_macros_list);
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
#define ADD(label, spec, expected_message)                              \
    cut_add_data(label,                                                 \
                 invalid_spec_test_data_new(spec, expected_message),    \
                 invalid_spec_test_data_free,                           \
                 NULL)

    ADD("invalid protocol name",
        "xxxx:9999@localhost",
        "protocol must be 'unix', 'inet' or 'inet6': "
        "<xxxx:9999@localhost>: <xxxx>\n");
    ADD("out of range port number",
        "inet:99999@localhost",
        "port number should be less than 65536: "
        "<inet:99999@localhost>: <99999>\n");

#undef ADD
}

void
test_invalid_spec (gconstpointer data)
{
    GError *error = NULL;
    InvalidSpecTestData *test_data = (InvalidSpecTestData *)data;

    setup_server(test_data->spec, NULL);
    gcut_process_run(server, &error);
    gcut_assert_error(error);

    wait_for_reaping(FALSE);
    cut_assert_equal_string(test_data->expected_message, output_string->str);
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
