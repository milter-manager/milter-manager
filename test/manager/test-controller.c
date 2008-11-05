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

#include <string.h>

#include <gcutter.h>

#define shutdown inet_shutdown
#include <milter-manager-test-utils.h>
#include <milter-manager-test-client.h>
#include <milter/manager/milter-manager-controller.h>
#include <milter-manager-test-server.h>
#undef shutdown

#define SCENARIO_GROUP "scenario"

void data_scenario (void);
void test_scenario (gconstpointer data);
void test_negotiate (void);
void test_connect (void);
void test_helo (void);
void data_envelope_from (void);
void test_envelope_from (gconstpointer data);
void test_envelope_recipient (void);
void test_envelope_recipient_accept (void);
void test_envelope_recipient_both_accept (void);
void test_envelope_recipient_reject (void);
void test_envelope_recipient_reject_and_discard_simultaneously (void);
void test_envelope_recipient_reject_and_temporary_failure_and_discard (void);
void test_envelope_recipient_discard (void);
void test_envelope_recipient_temporary_failure (void);
void test_envelope_recipient_both_temporary_failure (void);
void test_data (void);
void test_header (void);
void test_end_of_header (void);
void data_body (void);
void test_body (gconstpointer data);
void data_end_of_message (void);
void test_end_of_message (gconstpointer data);
void test_end_of_message_quarantine (void);
void test_add_header (void);
void test_change_from (void);
void test_add_recipient (void);
void test_delete_recipient (void);
void test_progress (void);

static GKeyFile *scenario;
static GList *imported_scenarios;

static MilterManagerConfiguration *config;
static MilterClientContext *client_context;
static MilterManagerController *controller;
static MilterManagerTestServer *server;
static MilterOption *option;

static GIOChannel *channel;
static MilterWriter *writer;
static MilterReader *reader;

static GArray *arguments1;
static GArray *arguments2;

static GError *controller_error;
static gboolean finished;
static gchar *quarantine_reason;

static GList *test_clients;
static GList *expected_list;

static MilterStatus response_status;

static void
add_load_path (const gchar *path)
{
    if (!path)
        return;
    milter_manager_configuration_add_load_path(config, path);
}

static void
cb_error (MilterManagerController *controller, GError *error, gpointer user_data)
{
    if (controller_error)
        g_error_free(controller_error);
    controller_error = g_error_copy(error);
}

static void
cb_finished (MilterManagerController *controller, gpointer user_data)
{
    finished = TRUE;
}

static void
setup_controller_signals (MilterManagerController *controller)
{
#define CONNECT(name)                                                   \
    g_signal_connect(controller, #name, G_CALLBACK(cb_ ## name), NULL)

    CONNECT(error);
    CONNECT(finished);

#undef CONNECT
}

void
setup (void)
{
    cut_set_fixture_data_dir(milter_manager_test_get_base_dir(),
                             "fixtures",
                             "controller",
                             NULL);

    scenario = NULL;
    imported_scenarios = NULL;

    config = milter_manager_configuration_new(NULL);
    add_load_path(g_getenv("MILTER_MANAGER_CONFIG_DIR"));
    milter_manager_configuration_load(config, "milter-manager.conf");

    channel = gcut_io_channel_string_new(NULL);
    g_io_channel_set_encoding(channel, NULL, NULL);
    g_io_channel_set_flags(channel, G_IO_FLAG_NONBLOCK, NULL);
    gcut_string_io_channel_set_pipe_mode(channel, TRUE);
    writer = milter_writer_io_channel_new(channel);
    reader = milter_reader_io_channel_new(channel);

    client_context = milter_client_context_new();
    milter_handler_set_writer(MILTER_HANDLER(client_context), writer);

    server = milter_manager_test_server_new();
    milter_handler_set_reader(MILTER_HANDLER(server), reader);

    controller = milter_manager_controller_new(config, client_context);
    controller_error = NULL;
    setup_controller_signals(controller);

    option = NULL;

    arguments1 = g_array_new(TRUE, TRUE, sizeof(gchar *));
    arguments2 = g_array_new(TRUE, TRUE, sizeof(gchar *));

    test_clients = NULL;

    response_status = MILTER_STATUS_DEFAULT;

    finished = FALSE;
    quarantine_reason = NULL;
    expected_list = NULL;
}

static void
arguments_append (GArray *arguments, const gchar *argument, ...)
{
    va_list args;

    va_start(args, argument);
    while (argument) {
        gchar *dupped_argument;

        dupped_argument = g_strdup(argument);
        g_array_append_val(arguments, dupped_argument);
        argument = va_arg(args, gchar *);
    }
    va_end(args);
}

static void
arguments_free (GArray *arguments)
{
    guint i;

    for (i = 0; i < arguments->len; i++) {
        g_free(g_array_index(arguments, gchar *, i));
    }

    g_array_free(arguments, TRUE);
}

void
teardown (void)
{
    if (scenario)
        g_key_file_free(scenario);
    if (imported_scenarios) {
        g_list_foreach(imported_scenarios, (GFunc)g_key_file_free, NULL);
        g_list_free(imported_scenarios);
    }

    if (config)
        g_object_unref(config);
    if (client_context)
        g_object_unref(client_context);
    if (controller)
        g_object_unref(controller);
    if (option)
        g_object_unref(option);

    if (server)
        g_object_unref(server);
    if (channel)
        g_io_channel_unref(channel);
    if (writer)
        g_object_unref(writer);
    if (reader)
        g_object_unref(reader);

    if (arguments1)
        arguments_free(arguments1);

    if (arguments2)
        arguments_free(arguments2);

    if (test_clients) {
        g_list_foreach(test_clients, (GFunc)g_object_unref, NULL);
        g_list_free(test_clients);
    }

    if (expected_list) {
        g_list_free(expected_list);
    }

    if (quarantine_reason)
        g_free(quarantine_reason);
}

#define collect_n_received(event)                                       \
    milter_manager_test_clients_collect_n_received(                     \
        test_clients,                                                   \
        milter_manager_test_client_get_n_ ## event ## _received)

#define get_received_data(name)                         \
    milter_manager_test_client_get_ ## name(client)

static gboolean
cb_timeout_waiting (gpointer data)
{
    gboolean *waiting = data;

    *waiting = FALSE;
    return FALSE;
}

static void
cb_negotiate_response_waiting (MilterClientContext *context,
                               MilterOption *options, MilterStatus status,
                               gpointer data)
{
    gboolean *waiting = data;

    *waiting = FALSE;
}

static void
cb_response_waiting (MilterClientContext *context, MilterStatus status,
                     gpointer data)
{
    gboolean *waiting = data;

    *waiting = FALSE;
    response_status = status;
}

#define wait_response(name)                     \
    cut_trace_with_info_expression(             \
        wait_response_helper(name "-response"), \
        wait_response(name))

static void
wait_response_helper (const gchar *name)
{
    gboolean timeout_waiting = TRUE;
    gboolean response_waiting = TRUE;
    guint timeout_waiting_id;
    guint response_waiting_id;

    if (g_str_equal(name, "negotiate-response"))
        response_waiting_id =
            g_signal_connect(client_context, name,
                             G_CALLBACK(cb_negotiate_response_waiting),
                             &response_waiting);
    else
        response_waiting_id =
            g_signal_connect(client_context, name,
                             G_CALLBACK(cb_response_waiting),
                             &response_waiting);
    timeout_waiting_id = g_timeout_add_seconds(1, cb_timeout_waiting,
                                               &timeout_waiting);
    while (timeout_waiting && response_waiting) {
        g_main_context_iteration(NULL, TRUE);
    }
    g_source_remove(timeout_waiting_id);
    g_signal_handler_disconnect(client_context, response_waiting_id);

    cut_assert_true(timeout_waiting, "timeout");
    gcut_assert_error(controller_error);
}

#define wait_reply(actual_getter)                                       \
    cut_trace_with_info_expression(                                     \
        milter_manager_test_clients_wait_reply(                         \
            test_clients,                                               \
            milter_manager_test_client_get_n_ ## actual_getter ## _received), \
        wait_reply(actual_getter))


static void
start_client (guint port, GArray *arguments)
{
    MilterManagerTestClient *client;
    GError *error = NULL;

    client = milter_manager_test_client_new(port);
    milter_manager_test_client_set_arguments(client, arguments);
    test_clients = g_list_append(test_clients, client);
    if (!milter_manager_test_client_run(client, &error)) {
        gcut_assert_error(error);
        cut_fail("couldn't run test client on port <%u>", port);
    }
}

static void
start_client_with_string_list (guint port, gchar **argument_strings)
{
    GArray *arguments;
    gchar **argument;

    arguments = g_array_new(TRUE, TRUE, sizeof(gchar *));
    if (argument_strings) {
        for (argument = argument_strings; *argument; argument++) {
            g_array_append_val(arguments, *argument);
        }
    }
    /* cut_take(arguments, g_array_free); */
    start_client(port, arguments);
}

#define wait_finished()                    \
    cut_trace_with_info_expression(        \
        wait_finished_helper(),            \
        wait_finished())

static void
wait_finished_helper (void)
{
    gboolean timeout_waiting = TRUE;
    guint timeout_waiting_id;

    timeout_waiting_id = g_timeout_add_seconds(1, cb_timeout_waiting,
                                               &timeout_waiting);
    while (timeout_waiting && !finished) {
        g_main_context_iteration(NULL, TRUE);
    }
    g_source_remove(timeout_waiting_id);

    cut_assert_true(timeout_waiting, "timeout");
    cut_assert_true(finished);
}

static MilterOption *
milter_option_new_from_key_file (GKeyFile *key_file, const gchar *group_name)
{
    GError *error = NULL;
    guint32 version;
    MilterActionFlags action;
    MilterStepFlags step;

    version = g_key_file_get_integer(key_file, group_name, "version", &error);
    gcut_assert_error(error);

    action = gcut_key_file_get_flags(key_file, group_name, "action",
                                     MILTER_TYPE_ACTION_FLAGS, &error);
    gcut_assert_error(error);

    step = gcut_key_file_get_flags(key_file, group_name, "step",
                                   MILTER_TYPE_STEP_FLAGS, &error);
    gcut_assert_error(error);

    return milter_option_new(version, action, step);
}

static void load_scenario (const gchar *name, GKeyFile **scenario);

static void
import_scenario (GKeyFile *scenario, const gchar *scenario_name)
{
    GKeyFile *imported_scenario = NULL;

    cut_trace(load_scenario(scenario_name, &imported_scenario));
    imported_scenarios = g_list_append(imported_scenarios, imported_scenario);
}

static void
import_scenarios (GKeyFile *scenario)
{
    const gchar key[] = "import";
    GError *error = NULL;
    gboolean has_key;
    gchar **scenario_names;
    gsize length, i;

    has_key = g_key_file_has_key(scenario, SCENARIO_GROUP, key, &error);
    gcut_assert_error(error);
    if (!has_key)
        return;

    scenario_names = g_key_file_get_string_list(scenario,
                                                SCENARIO_GROUP, key,
                                                &length, &error);
    gcut_assert_error(error);

    cut_take_string_array(scenario_names);
    for (i = 0; i < length; i++) {
        cut_trace(import_scenario(scenario, scenario_names[i]));
    }
}

static void
load_scenario (const gchar *name, GKeyFile **scenario)
{
    GError *error = NULL;
    gchar *scenario_path;

    *scenario = g_key_file_new();
    scenario_path = cut_build_fixture_data_path(name, NULL);
    if (!g_key_file_load_from_file(*scenario, scenario_path,
                                   G_KEY_FILE_KEEP_COMMENTS,
                                   &error)) {
        g_key_file_free(*scenario);
        *scenario = NULL;
    }
    g_free(scenario_path);
    gcut_assert_error(error);

    cut_trace(import_scenarios(*scenario));
}

static void
setup_client (GKeyFile *scenario, const gchar *group)
{
    GError *error = NULL;
    guint port;
    gsize length;
    gchar **arguments = NULL;

    port = g_key_file_get_integer(scenario, group, "port", &error);
    gcut_assert_error(error);
    if (g_key_file_has_key(scenario, group, "arguments", &error)) {
        arguments = g_key_file_get_string_list(scenario, group, "arguments",
                                               &length, &error);
    }
    gcut_assert_error(error);

    cut_take_string_array(arguments);
    start_client_with_string_list(port, arguments);
}

static void
setup_clients (GKeyFile *scenario)
{
    GError *error = NULL;
    gchar **clients;
    gsize length, i;

    clients = g_key_file_get_string_list(scenario, SCENARIO_GROUP, "clients",
                                         &length, &error);
    gcut_assert_error(error);

    cut_take_string_array(clients);
    for (i = 0; i < length; i++) {
        cut_trace(setup_client(scenario, clients[i]));
    }
}

static MilterManagerTestClientGetNReceived
response_to_get_n_received (const gchar *response)
{
    if (g_str_equal(response, "negotiate")) {
        return milter_manager_test_client_get_n_negotiate_received;
    } else if (g_str_equal(response, "connect")) {
        return milter_manager_test_client_get_n_connect_received;
    } else if (g_str_equal(response, "helo")) {
        return milter_manager_test_client_get_n_helo_received;
    } else if (g_str_equal(response, "envelope-from")) {
        return milter_manager_test_client_get_n_envelope_from_received;
    } else if (g_str_equal(response, "envelope-recipient")) {
        return milter_manager_test_client_get_n_envelope_recipient_received;
    } else if (g_str_equal(response, "data")) {
        return milter_manager_test_client_get_n_data_received;
    } else if (g_str_equal(response, "header")) {
        return milter_manager_test_client_get_n_header_received;
    } else if (g_str_equal(response, "end-of-header")) {
        return milter_manager_test_client_get_n_end_of_header_received;
    } else if (g_str_equal(response, "body")) {
        return milter_manager_test_client_get_n_body_received;
    } else if (g_str_equal(response, "end-of-message")) {
        return milter_manager_test_client_get_n_end_of_message_received;
    } else if (g_str_equal(response, "quit")) {
        return milter_manager_test_client_get_n_quit_received;
    } else if (g_str_equal(response, "abort")) {
        return milter_manager_test_client_get_n_abort_received;
    } else if (g_str_equal(response, "unknown")) {
        return milter_manager_test_client_get_n_unknown_received;
    }

    return NULL;
}

static gint
get_integer (GKeyFile *scenario, const gchar *group, const gchar *key)
{
    GError *error = NULL;
    gint value;

    value = g_key_file_get_integer(scenario, group, key, &error);
    gcut_assert_error(error, "group: <%s>", group);
    return value;
}

static const gchar *
get_string (GKeyFile *scenario, const gchar *group, const gchar *key)
{
    GError *error = NULL;
    gchar *value;

    value = g_key_file_get_string(scenario, group, key, &error);
    gcut_assert_error(error, "group: <%s>", group);
    return cut_take_string(value);
}

static gint
get_enum (GKeyFile *scenario, const gchar *group, const gchar *key,
          GType enum_type)
{
    GError *error = NULL;
    gint value;

    value = gcut_key_file_get_enum(scenario, group, key, enum_type, &error);
    gcut_assert_error(error, "group: <%s>", group);
    return value;
}


static void
assert_response (GKeyFile *scenario, const gchar *group)
{
    MilterManagerTestClientGetNReceived get_n_received;
    guint n_received, actual_n_received;
    const gchar *response;
    MilterStatus status;

    n_received = get_integer(scenario, group, "n_received");
    response = get_string(scenario, group, "response");
    status = get_enum(scenario, group, "status", MILTER_TYPE_STATUS);

    if (g_str_equal(response, "quit")) {
        wait_finished();
    } else if (g_str_equal(response, "abort")) {
        wait_reply(abort);
    } else {
        const gchar *response_signal_name;

        response_signal_name = cut_take_printf("%s-response", response);
        cut_trace(wait_response_helper(response_signal_name));
    }

    get_n_received = response_to_get_n_received(response);
    cut_assert_not_null(get_n_received, "response: <%s>(%s)", response, group);
    actual_n_received =
        milter_manager_test_clients_collect_n_received(test_clients,
                                                       get_n_received);
    cut_assert_equal_uint(n_received, actual_n_received);
    gcut_assert_equal_enum(MILTER_TYPE_STATUS, status, response_status);
}

static void
do_negotiate (GKeyFile *scenario, const gchar *group)
{
    option = milter_option_new_from_key_file(scenario, group);

    milter_server_context_negotiate(MILTER_SERVER_CONTEXT(server), option);
    milter_manager_controller_negotiate(controller, option);
    cut_trace(assert_response(scenario, group));
}

static void
do_connect (GKeyFile *scenario, const gchar *group)
{
    GError *error = NULL;
    const gchar *host;
    const gchar *address_spec;
    gint domain;
    struct sockaddr *address;
    socklen_t address_size;

    host = get_string(scenario, group, "host");
    address_spec = get_string(scenario, group, "address");

    milter_utils_parse_connection_spec(address_spec,
                                       &domain,
                                       &address,
                                       &address_size,
                                       &error);
    gcut_assert_error(error);

    milter_manager_controller_connect(controller, host, address, address_size);
    g_free(address);

    cut_trace(assert_response(scenario, group));
}

static void
do_helo (GKeyFile *scenario, const gchar *group)
{
    MilterManagerTestClient *client;
    const gchar *fqdn;

    fqdn = get_string(scenario, group, "fqdn");
    milter_manager_controller_helo(controller, fqdn);

    cut_trace(assert_response(scenario, group));

    client = test_clients->data;
    cut_assert_equal_string(fqdn, get_received_data(helo_fqdn));
}

static void
do_envelope_from (GKeyFile *scenario, const gchar *group)
{
    MilterManagerTestClient *client;
    const gchar *from;

    from = get_string(scenario, group, "from");
    milter_manager_controller_envelope_from(controller, from);

    cut_trace(assert_response(scenario, group));

    client = test_clients->data;
    cut_assert_equal_string(from, get_received_data(envelope_from));
}

static void
do_envelope_recipient (GKeyFile *scenario, const gchar *group)
{
    MilterManagerTestClient *client;
    const gchar *recipient;

    recipient = get_string(scenario, group, "recipient");
    milter_manager_controller_envelope_recipient(controller, recipient);

    cut_trace(assert_response(scenario, group));

    client = test_clients->data;
    cut_assert_equal_string(recipient, get_received_data(envelope_recipient));
}

static void
do_data (GKeyFile *scenario, const gchar *group)
{
    milter_manager_controller_data(controller);

    cut_trace(assert_response(scenario, group));
}

static void
do_header (GKeyFile *scenario, const gchar *group)
{
    MilterManagerTestClient *client;
    const gchar *name, *value;

    name = get_string(scenario, group, "name");
    value = get_string(scenario, group, "value");
    milter_manager_controller_header(controller, name, value);

    cut_trace(assert_response(scenario, group));

    client = test_clients->data;
    cut_assert_equal_string(name, get_received_data(header_name));
    cut_assert_equal_string(value, get_received_data(header_value));
}

static void
do_end_of_header (GKeyFile *scenario, const gchar *group)
{
    milter_manager_controller_end_of_header(controller);

    cut_trace(assert_response(scenario, group));
}

static void
do_body (GKeyFile *scenario, const gchar *group)
{
    MilterManagerTestClient *client;
    const gchar *chunk;

    chunk = get_string(scenario, group, "chunk");
    milter_manager_controller_body(controller, chunk, strlen(chunk));

    cut_trace(assert_response(scenario, group));

    client = test_clients->next->data;
    cut_assert_equal_string(chunk, get_received_data(body_chunk));
}

static void
do_end_of_message (GKeyFile *scenario, const gchar *group)
{
    MilterManagerTestClient *client;
    GError *error = NULL;
    const gchar *chunk = NULL;

    if (g_key_file_has_key(scenario, group, "chunk", &error))
        chunk = get_string(scenario, group, "chunk");
    gcut_assert_error(error);

    milter_manager_controller_end_of_message(controller,
                                             chunk,
                                             chunk ? strlen(chunk) : 0);

    cut_trace(assert_response(scenario, group));
    client = test_clients->data;
    cut_assert_equal_string(chunk, get_received_data(body_chunk));
}

static void
do_quit (GKeyFile *scenario, const gchar *group)
{
    milter_manager_controller_quit(controller);
    cut_trace(assert_response(scenario, group));
}

static void
do_abort (GKeyFile *scenario, const gchar *group)
{
    milter_manager_controller_abort(controller);
    cut_trace(assert_response(scenario, group));
}

static void
do_unknown (GKeyFile *scenario, const gchar *group)
{
    MilterManagerTestClient *client;
    const gchar *command;

    command = get_string(scenario, group, "unknown_command");

    milter_manager_controller_unknown(controller, command);
    cut_trace(assert_response(scenario, group));

    client = test_clients->data;
    cut_assert_equal_string(command, get_received_data(unknown_command));
}

static void
do_action (GKeyFile *scenario, const gchar *group)
{
    GError *error = NULL;
    MilterCommand command;

    command = gcut_key_file_get_enum(scenario, group, "command",
                                     MILTER_TYPE_COMMAND, &error);
    gcut_assert_error(error, "group: <%s>", group);

    switch (command) {
      case MILTER_COMMAND_NEGOTIATE:
        cut_trace(do_negotiate(scenario, group));
        break;
      case MILTER_COMMAND_CONNECT:
        cut_trace(do_connect(scenario, group));
        break;
      case MILTER_COMMAND_HELO:
        cut_trace(do_helo(scenario, group));
        break;
      case MILTER_COMMAND_MAIL:
        cut_trace(do_envelope_from(scenario, group));
        break;
      case MILTER_COMMAND_RCPT:
        cut_trace(do_envelope_recipient(scenario, group));
        break;
      case MILTER_COMMAND_DATA:
        cut_trace(do_data(scenario, group));
        break;
      case MILTER_COMMAND_HEADER:
        cut_trace(do_header(scenario, group));
        break;
      case MILTER_COMMAND_END_OF_HEADER:
        cut_trace(do_end_of_header(scenario, group));
        break;
      case MILTER_COMMAND_BODY:
        cut_trace(do_body(scenario, group));
        break;
      case MILTER_COMMAND_END_OF_MESSAGE:
        cut_trace(do_end_of_message(scenario, group));
        break;
      case MILTER_COMMAND_QUIT:
        cut_trace(do_quit(scenario, group));
        break;
      case MILTER_COMMAND_ABORT:
        cut_trace(do_abort(scenario, group));
        break;
      case MILTER_COMMAND_UNKNOWN:
        cut_trace(do_unknown(scenario, group));
        break;
      default:
        cut_error("unknown command: <%c>(<%s>)", command, group);
        break;
    }
}

static void
do_actions (GKeyFile *scenario)
{
    GError *error = NULL;
    gsize length, i;
    gchar **actions;

    actions = g_key_file_get_string_list(scenario, SCENARIO_GROUP, "actions",
                                         &length, &error);
    gcut_assert_error(error);

    cut_take_string_array(actions);
    for (i = 0; i < length; i++) {
        cut_trace(do_action(scenario, actions[i]));
        g_list_foreach(test_clients,
                       (GFunc)milter_manager_test_client_clear_data,
                       NULL);
    }
}

void
data_scenario (void)
{
    cut_add_data(
        "negotiate", g_strdup("negotiate.txt"), g_free,
        "connect", g_strdup("connect.txt"), g_free,
        "helo", g_strdup("helo.txt"), g_free,
        "envelope-from", g_strdup("envelope-from.txt"), g_free,
        "envelope-recipient", g_strdup("envelope-recipient.txt"), g_free,
        "data", g_strdup("data.txt"), g_free,
        "header - from", g_strdup("header-from.txt"), g_free,
        "end-of-header", g_strdup("end-of-header.txt"), g_free,
        "body", g_strdup("body.txt"), g_free,
        "end-of-message", g_strdup("end-of-message.txt"), g_free,
        "quit", g_strdup("quit.txt"), g_free,
        "abort", g_strdup("abort.txt"), g_free,
        "unknown", g_strdup("unknown.txt"), g_free);

    cut_add_data("body - skip", g_strdup("body-skip.txt"), g_free);
}

void
test_scenario (gconstpointer data)
{
    const gchar *scenario_name = data;

    cut_trace(load_scenario(scenario_name, &scenario));
    cut_trace(setup_clients(scenario));
    cut_trace(g_list_foreach(imported_scenarios, (GFunc)do_actions, NULL));
    cut_trace(do_actions(scenario));
}

void
test_negotiate (void)
{
    option = milter_option_new(2,
                               MILTER_ACTION_ADD_HEADERS |
                               MILTER_ACTION_CHANGE_BODY |
                               MILTER_ACTION_QUARANTINE,
                               MILTER_STEP_NONE);

    start_client(10026, arguments1);
    start_client(10027, arguments2);

    /* To set MilterOption in MilterManagerTestServer */
    milter_server_context_negotiate(MILTER_SERVER_CONTEXT(server), option);

    milter_manager_controller_negotiate(controller, option);
    wait_response("negotiate");
    cut_assert_equal_uint(g_list_length(test_clients),
                          collect_n_received(negotiate));
}

void
test_connect (void)
{
    const gchar host_name[] = "mx.local.net";
    gint domain;
    struct sockaddr *address;
    socklen_t address_size;
    GError *error = NULL;

    cut_trace(test_negotiate());

    milter_utils_parse_connection_spec("inet:2929@192.168.1.29",
                                       &domain,
                                       &address,
                                       &address_size,
                                       &error);
    gcut_assert_error(error);

    milter_manager_controller_connect(controller, host_name,
                                      address, address_size);
    g_free(address);
    wait_response("connect");
    cut_assert_equal_uint(g_list_length(test_clients),
                          collect_n_received(connect));
}

void
test_helo (void)
{
    const gchar fqdn[] = "delian";

    cut_trace(test_connect());

    milter_manager_controller_helo(controller, fqdn);
    wait_response("helo");
    cut_assert_equal_uint(g_list_length(test_clients),
                          collect_n_received(helo));
}

typedef void (*setup_from_test_func) (const gchar *from);

typedef struct _FromTestData
{
    setup_from_test_func setup_func;
    MilterStatus expected_status;
} FromTestData;

static FromTestData *
from_test_data_new (setup_from_test_func setup_func,
                    MilterStatus expected_status)
{
    FromTestData *test_data;
    test_data = g_new0(FromTestData, 1);
    test_data->setup_func = setup_func;
    test_data->expected_status = expected_status;

    return test_data;
}

static void
setup_reject_from (const gchar *from)
{
    arguments_append(arguments1,
                     "--action", "reject",
                     "--envelope-from", from,
                     NULL);
}

static void
setup_discard_from (const gchar *from)
{
    arguments_append(arguments1,
                     "--action", "discard",
                     "--envelope-from", from,
                     NULL);
}

static void
setup_accept_and_continue_from (const gchar *from)
{
    arguments_append(arguments1,
                     "--action", "accept",
                     "--envelope-from", from,
                     NULL);
}

static void
setup_both_accept_from (const gchar *from)
{
    arguments_append(arguments1,
                     "--action", "accept",
                     "--envelope-from", from,
                     NULL);
    arguments_append(arguments2,
                     "--action", "accept",
                     "--envelope-from", from,
                     NULL);
}

static void
setup_reject_and_discard_from (const gchar *from)
{
    arguments_append(arguments1,
                     "--action", "reject",
                     "--envelope-from", from,
                     NULL);
    arguments_append(arguments2,
                     "--action", "discard",
                     "--envelope-from", from,
                     NULL);
}

static void
setup_reject_and_accept_from (const gchar *from)
{
    arguments_append(arguments1,
                     "--action", "reject",
                     "--envelope-from", from,
                     NULL);
    arguments_append(arguments2,
                     "--action", "accept",
                     "--envelope-from", from,
                     NULL);
}

void
data_envelope_from (void)
{
    cut_add_data("reject",
                 from_test_data_new(setup_reject_from,
                                    MILTER_STATUS_REJECT),
                 g_free,
                 "discard",
                 from_test_data_new(setup_discard_from,
                                    MILTER_STATUS_DISCARD),
                 g_free,
                 "reject_and_discard",
                 from_test_data_new(setup_reject_and_discard_from,
                                    MILTER_STATUS_REJECT),
                 g_free,
                 "reject_and_accept",
                 from_test_data_new(setup_reject_and_accept_from,
                                    MILTER_STATUS_REJECT),
                 g_free,
                 "accept_and_continue",
                 from_test_data_new(setup_accept_and_continue_from,
                                    MILTER_STATUS_CONTINUE),
                 g_free,
                 "both_accept",
                 from_test_data_new(setup_both_accept_from,
                                    MILTER_STATUS_ACCEPT),
                 g_free,
                 "continue",
                 from_test_data_new(NULL,
                                    MILTER_STATUS_CONTINUE),
                 g_free);
}

void
test_envelope_from (gconstpointer data)
{
    const gchar from[] = "kou+sender@cozmixng.org";
    FromTestData *test_data = (FromTestData*)data;
    MilterStatus expected_status = MILTER_STATUS_CONTINUE;

    if (test_data) {
        expected_status = test_data->expected_status;
        if (test_data->setup_func)
            test_data->setup_func(from);
    }

    cut_trace(test_helo());

    milter_manager_controller_envelope_from(controller, from);
    wait_response("envelope-from");
    cut_assert_equal_uint(g_list_length(test_clients),
                          collect_n_received(envelope_from));
    gcut_assert_equal_enum(MILTER_TYPE_STATUS,
                           expected_status, response_status);
}

void
test_envelope_recipient (void)
{
    const gchar recipient[] = "kou+receiver@cozmixng.org";

    cut_trace(test_envelope_from(NULL));

    milter_manager_controller_envelope_recipient(controller, recipient);
    wait_response("envelope-recipient");
    cut_assert_equal_uint(g_list_length(test_clients),
                          collect_n_received(envelope_recipient));
}

void
test_envelope_recipient_accept (void)
{
    const gchar recipient[] = "kou+accepted@cozmixng.org";

    arguments_append(arguments1,
                     "--action", "accept",
                     "--envelope-recipient", recipient,
                     NULL);

    cut_trace(test_envelope_recipient());

    milter_manager_controller_envelope_recipient(controller, recipient);
    wait_response("envelope-recipient");
    cut_assert_equal_uint(g_list_length(test_clients) * 2,
                          collect_n_received(envelope_recipient));
    gcut_assert_equal_enum(MILTER_TYPE_STATUS,
                           MILTER_STATUS_CONTINUE, response_status);

    milter_manager_controller_data(controller);
    wait_response("data");
    cut_assert_equal_uint(g_list_length(test_clients) - 1,
                          collect_n_received(data),
                          "the child which returns 'ACCEPT' must die");
}

void
test_envelope_recipient_both_accept (void)
{
    const gchar recipient[] = "kou+accepted@cozmixng.org";

    arguments_append(arguments1,
                     "--action", "accept",
                     "--envelope-recipient", recipient,
                     NULL);
    arguments_append(arguments2,
                     "--action", "accept",
                     "--envelope-recipient", recipient,
                     NULL);

    cut_trace(test_envelope_recipient());

    milter_manager_controller_envelope_recipient(controller, recipient);
    wait_response("envelope-recipient");
    cut_assert_equal_uint(g_list_length(test_clients) * 2,
                          collect_n_received(envelope_recipient));
    gcut_assert_equal_enum(MILTER_TYPE_STATUS,
                           MILTER_STATUS_ACCEPT, response_status,
                           "Check the process is ended "
                           "if both children returns ACCEPT.");
    wait_finished();
}

void
test_envelope_recipient_reject (void)
{
    const gchar recipient[] = "kou+rejected@cozmixng.org";

    arguments_append(arguments1,
                     "--action", "reject",
                     "--envelope-recipient", recipient,
                     NULL);

    cut_trace(test_envelope_recipient());

    milter_manager_controller_envelope_recipient(controller, recipient);
    wait_response("envelope-recipient");
    cut_assert_equal_uint(g_list_length(test_clients) * 2,
                          collect_n_received(envelope_recipient));
    gcut_assert_equal_enum(MILTER_TYPE_STATUS,
                           MILTER_STATUS_REJECT, response_status);

    milter_manager_controller_data(controller);
    wait_response("data");
    cut_assert_equal_uint(g_list_length(test_clients),
                          collect_n_received(data));
}

void
test_envelope_recipient_reject_and_temporary_failure_and_discard (void)
{
    const gchar reject_recipient[] = "kou+rejected@cozmixng.org";
    const gchar temporary_failure_recipient[] =
        "kou+temporary_failure@cozmixng.org";
    const gchar discard_recipient[] = "kou+discarded@cozmixng.org";

    arguments_append(arguments1,
                     "--action", "reject",
                     "--envelope-recipient", reject_recipient,
                     "--action", "temporary_failure",
                     "--envelope-recipient", temporary_failure_recipient,
                     "--action", "discard",
                     "--envelope-recipient", discard_recipient,
                     NULL);

    cut_trace(test_envelope_recipient());

    milter_manager_controller_envelope_recipient(controller, reject_recipient);
    wait_response("envelope-recipient");
    cut_assert_equal_uint(g_list_length(test_clients) * 2,
                          collect_n_received(envelope_recipient));
    gcut_assert_equal_enum(MILTER_TYPE_STATUS,
                           MILTER_STATUS_REJECT, response_status);

    milter_manager_controller_envelope_recipient(controller,
                                                 temporary_failure_recipient);
    wait_response("envelope-recipient");
    cut_assert_equal_uint(g_list_length(test_clients) * 3,
                          collect_n_received(envelope_recipient));
    gcut_assert_equal_enum(MILTER_TYPE_STATUS,
                           MILTER_STATUS_CONTINUE, response_status);

    milter_manager_controller_envelope_recipient(controller, discard_recipient);
    wait_response("envelope-recipient");
    cut_assert_equal_uint(g_list_length(test_clients) * 4,
                          collect_n_received(envelope_recipient));
    gcut_assert_equal_enum(MILTER_TYPE_STATUS,
                           MILTER_STATUS_DISCARD, response_status);
}

void
test_envelope_recipient_reject_and_discard_simultaneously (void)
{
    const gchar recipient[] = "kou+rejected+discarded@cozmixng.org";

    arguments_append(arguments1,
                     "--action", "reject",
                     "--envelope-recipient", recipient,
                     NULL);
    arguments_append(arguments2,
                     "--action", "discard",
                     "--envelope-recipient", recipient,
                     NULL);

    cut_trace(test_envelope_recipient());

    milter_manager_controller_envelope_recipient(controller, recipient);
    wait_response("envelope-recipient");
    cut_assert_equal_uint(g_list_length(test_clients) * 2,
                          collect_n_received(envelope_recipient));
    gcut_assert_equal_enum(MILTER_TYPE_STATUS,
                           MILTER_STATUS_DISCARD, response_status);
}

void
test_envelope_recipient_discard (void)
{
    const gchar recipient[] = "kou+discarded@cozmixng.org";

    arguments_append(arguments1,
                     "--action", "discard",
                     "--envelope-recipient", recipient,
                     NULL);

    cut_trace(test_envelope_recipient());

    milter_manager_controller_envelope_recipient(controller, recipient);
    wait_response("envelope-recipient");
    cut_assert_equal_uint(g_list_length(test_clients) * 2,
                          collect_n_received(envelope_recipient));
    gcut_assert_equal_enum(MILTER_TYPE_STATUS,
                           MILTER_STATUS_DISCARD, response_status);
}

void
test_envelope_recipient_temporary_failure (void)
{
    const gchar recipient[] = "kou+temporary_failureed@cozmixng.org";

    arguments_append(arguments1,
                     "--action", "temporary_failure",
                     "--envelope-recipient", recipient,
                     NULL);

    cut_trace(test_envelope_recipient());

    milter_manager_controller_envelope_recipient(controller, recipient);
    wait_response("envelope-recipient");
    cut_assert_equal_uint(g_list_length(test_clients) * 2,
                          collect_n_received(envelope_recipient));
    gcut_assert_equal_enum(MILTER_TYPE_STATUS,
                           MILTER_STATUS_CONTINUE, response_status);
    milter_manager_controller_data(controller);
    wait_response("data");
    cut_assert_equal_uint(g_list_length(test_clients),
                          collect_n_received(data),
                          "the child which returns 'TEMPORARY_FAILURE' "
                          "must live");
}

void
test_envelope_recipient_both_temporary_failure (void)
{
    const gchar recipient[] = "kou+temporary_failureed@cozmixng.org";

    arguments_append(arguments1,
                     "--action", "temporary_failure",
                     "--envelope-recipient", recipient,
                     NULL);
    arguments_append(arguments2,
                     "--action", "temporary_failure",
                     "--envelope-recipient", recipient,
                     NULL);

    cut_trace(test_envelope_recipient());

    milter_manager_controller_envelope_recipient(controller, recipient);
    wait_response("envelope-recipient");
    cut_assert_equal_uint(g_list_length(test_clients) * 2,
                          collect_n_received(envelope_recipient));
    gcut_assert_equal_enum(MILTER_TYPE_STATUS,
                           MILTER_STATUS_TEMPORARY_FAILURE, response_status,
                           "The process goes on even if TEMPORARY_FAILURE "
                           "returns in ENVELOPE_RECIPIENT.");
    milter_manager_controller_data(controller);
    wait_response("data");
    cut_assert_equal_uint(g_list_length(test_clients),
                          collect_n_received(data));
}

void
test_data (void)
{
    cut_trace(test_envelope_recipient());

    milter_manager_controller_data(controller);
    wait_response("data");
    cut_assert_equal_uint(g_list_length(test_clients),
                          collect_n_received(data));
}

void
test_header (void)
{
    MilterManagerTestClient *client;
    const gchar name[] = "From";
    const gchar value[] = "kou+sender@cozmixng.org";

    cut_trace(test_data());

    milter_manager_controller_header(controller, name, value);
    wait_response("header");
    cut_assert_equal_uint(g_list_length(test_clients),
                          collect_n_received(header));

    client = test_clients->data;
    cut_assert_equal_string(name, get_received_data(header_name));
    cut_assert_equal_string(value, get_received_data(header_value));
}

void
test_end_of_header (void)
{
    cut_trace(test_header());

    milter_manager_controller_end_of_header(controller);
    wait_response("end-of-header");
    cut_assert_equal_uint(g_list_length(test_clients),
                          collect_n_received(end_of_header));
}

typedef void (*setup_body_test_func) (gpointer test_data);
typedef void (*assert_body_test_func) (gpointer test_data);
typedef void (*free_body_test_data_func) (gpointer test_data);

typedef struct _BodyTestData
{
    setup_body_test_func setup_func;
    assert_body_test_func assert_func;
    free_body_test_data_func free_data_func;
    gpointer test_data;
} BodyTestData;

static BodyTestData *
body_test_data_new (setup_body_test_func setup_func,
                    assert_body_test_func assert_func,
                    free_body_test_data_func free_data_func,
                    gpointer test_data)
{
    BodyTestData *data;

    data = g_new(BodyTestData, 1);

    data->setup_func = setup_func;
    data->assert_func = assert_func;
    data->free_data_func = free_data_func;
    data->test_data = test_data;

    return data;
}

static void
free_body_test_data (BodyTestData *data)
{
    if (data->free_data_func)
        data->free_data_func(data->test_data);

    free(data);
}

#define milter_manager_assert_body_test(data) \
    if (data)                                 \
        cut_trace(assert_body_test((BodyTestData*)data));

static void
assert_body_test (BodyTestData *data)
{
    if (data->assert_func)
        cut_trace(data->assert_func(data->test_data));
}

typedef struct _AllSkipBodyTestData
{
    gchar *first_skip_chunk;
    gchar *second_skip_chunk;
} AllSkipBodyTestData;

static void
setup_all_skip_body_test (gpointer test_data)
{
    AllSkipBodyTestData *data = test_data;

    arguments_append(arguments1,
                     "--action", "skip",
                     "--body", data->first_skip_chunk,
                     NULL);
    arguments_append(arguments2,
                     "--action", "skip",
                     "--body", data->second_skip_chunk,
                     NULL);
}

static void
assert_all_skip_body_test (gpointer test_data)
{
    const gchar chunk[] = "See you.";
    AllSkipBodyTestData *data = test_data;

    milter_manager_controller_body(controller,
                                   data->first_skip_chunk,
                                   strlen(data->first_skip_chunk));
    wait_response("body");
    cut_assert_equal_uint(g_list_length(test_clients) * 2,
                          collect_n_received(body));

    milter_manager_controller_body(controller, chunk, strlen(chunk));
    wait_response("body");
    /* The child which send SKIP must not be received further body command */
    cut_assert_equal_uint(g_list_length(test_clients) * 3 - 1,
                          collect_n_received(body));


    milter_manager_controller_body(controller,
                                   data->second_skip_chunk,
                                   strlen(data->second_skip_chunk));
    wait_response("body");
    /* The child which send SKIP must not be received further body command */
    cut_assert_equal_uint(g_list_length(test_clients) * 3 - 1 + 1,
                          collect_n_received(body));
    gcut_assert_equal_enum(MILTER_TYPE_STATUS,
                           MILTER_STATUS_SKIP, response_status);
}

static void
free_all_skip_body_test_data (gpointer test_data)
{
    AllSkipBodyTestData *data = test_data;

    free(data->first_skip_chunk);
    free(data->second_skip_chunk);
}

static gpointer
all_skip_body_test_data_new (const gchar *first_chunk,
                             const gchar *second_chunk)
{
    AllSkipBodyTestData *data;
    
    data = g_new0(AllSkipBodyTestData, 1);
    data->first_skip_chunk = g_strdup(first_chunk);
    data->second_skip_chunk = g_strdup(second_chunk);

    return data;
}

typedef struct _SkipBodyTestData
{
    gchar *skip_chunk;
} SkipBodyTestData;

static void
setup_skip_body_test (gpointer test_data)
{
    SkipBodyTestData *data = test_data;

    arguments_append(arguments1,
                     "--action", "skip",
                     "--body", data->skip_chunk,
                     NULL);
}

static void
assert_skip_body_test (gpointer test_data)
{
    const gchar chunk[] = "See you.";
    SkipBodyTestData *data = test_data;

    milter_manager_controller_body(controller,
                                   data->skip_chunk,
                                   strlen(data->skip_chunk));
    wait_response("body");
    cut_assert_equal_uint(g_list_length(test_clients) * 2,
                          collect_n_received(body));

    milter_manager_controller_body(controller, chunk, strlen(chunk));
    wait_response("body");
    /* The child which send SKIP must not be received further body command */
    cut_assert_equal_uint(g_list_length(test_clients) * 3 - 1,
                          collect_n_received(body));

    /* The child which sends SKIP should be received next command. */
    milter_manager_controller_end_of_message(controller, chunk, strlen(chunk));
    wait_response("end-of-message");
    cut_assert_equal_uint(g_list_length(test_clients),
                          collect_n_received(end_of_message));
}

static void
free_skip_body_test_data (gpointer test_data)
{
    SkipBodyTestData *data = test_data;

    free(data->skip_chunk);
}

static gpointer
skip_body_test_data_new (const gchar *skip_chunk)
{
    SkipBodyTestData *skip_data;
    
    skip_data = g_new0(SkipBodyTestData, 1);
    skip_data->skip_chunk = g_strdup(skip_chunk);

    return skip_data;
}

typedef struct _AcceptBodyTestData
{
    gchar *first_chunk;
    gchar *second_chunk;
} AcceptBodyTestData;

static void
setup_accept_body_test (gpointer test_data)
{
    AcceptBodyTestData *data = test_data;

    if (data->first_chunk) {
        arguments_append(arguments1,
                         "--action", "accept",
                         "--body", data->first_chunk,
                         NULL);
    }

    if (data->second_chunk) {
        arguments_append(arguments2,
                         "--action", "accept",
                         "--body", data->second_chunk,
                         NULL);
    }
}

static void
assert_accept_body_test (gpointer test_data)
{
    AcceptBodyTestData *data = test_data;

    milter_manager_controller_body(controller,
                                   data->first_chunk,
                                   strlen(data->first_chunk));
    wait_response("body");
    cut_assert_equal_uint(g_list_length(test_clients) * 2,
                          collect_n_received(body));

    milter_manager_controller_body(controller,
                                   data->second_chunk,
                                   strlen(data->second_chunk));
    wait_response("body");
    /* The first child must not be received further body command */
    cut_assert_equal_uint(g_list_length(test_clients) * 3 - 1,
                          collect_n_received(body));
    gcut_assert_equal_enum(MILTER_TYPE_STATUS,
                           MILTER_STATUS_ACCEPT, response_status);
}

static void
free_accept_body_test_data (gpointer test_data)
{
    AcceptBodyTestData *data = test_data;

    free(data->first_chunk);
    free(data->second_chunk);
}

static gpointer
accept_body_test_data_new (const gchar *first_chunk, const gchar *second_chunk)
{
    AcceptBodyTestData *accept_data;

    accept_data = g_new0(AcceptBodyTestData, 1);
    accept_data->first_chunk = g_strdup(first_chunk);
    accept_data->second_chunk = g_strdup(second_chunk);

    return accept_data;
}

void
data_body (void)
{
    cut_add_data(
                 "normal",
                 body_test_data_new(NULL, NULL, NULL, NULL),
                 free_body_test_data,
                 "skip",
                 body_test_data_new(setup_skip_body_test,
                                    assert_skip_body_test,
                                    free_skip_body_test_data,
                                    skip_body_test_data_new("Skip")),
                 free_body_test_data,
                 "all skip",
                 body_test_data_new(setup_all_skip_body_test,
                                    assert_all_skip_body_test,
                                    free_all_skip_body_test_data,
                                    all_skip_body_test_data_new("Skip first", "Skip second")),
                 free_body_test_data,
                 "accept",
                 body_test_data_new(setup_accept_body_test,
                                    assert_accept_body_test,
                                    free_accept_body_test_data,
                                    accept_body_test_data_new("First Line", "Second Line")),
                 free_body_test_data);
}

void
test_body (gconstpointer data)
{
    MilterManagerTestClient *client;
    const gchar chunk[] = "Hi\n\nThanks,";
    BodyTestData *test_data = (BodyTestData*)data;

    if (test_data && test_data->setup_func)
        test_data->setup_func(test_data->test_data);

    cut_trace(test_end_of_header());

    milter_manager_controller_body(controller, chunk, strlen(chunk));
    wait_response("body");
    cut_assert_equal_uint(g_list_length(test_clients),
                          collect_n_received(body));

    client = test_clients->data;
    cut_assert_equal_string(chunk, get_received_data(body_chunk));

    milter_manager_assert_body_test(data);
}

void
data_end_of_message (void)
{
    cut_add_data("with chunk", g_strdup("The last text"), g_free,
                 "no chunk", NULL, NULL);
}

void
test_end_of_message (gconstpointer data)
{
    MilterManagerTestClient *client;
    const gchar *chunk = data;
    gsize chunk_size;

    cut_trace(test_body(NULL));

    if (chunk)
        chunk_size = strlen(chunk);
    else
        chunk_size = 0;

    milter_server_context_set_state(MILTER_SERVER_CONTEXT(server), 
                                    MILTER_SERVER_CONTEXT_STATE_END_OF_MESSAGE);
    milter_manager_controller_end_of_message(controller, chunk, chunk_size);
    wait_response("end-of-message");
    gcut_assert_equal_enum(MILTER_TYPE_STATUS,
                           MILTER_STATUS_CONTINUE, response_status);
    cut_assert_equal_uint(g_list_length(test_clients),
                          collect_n_received(end_of_message));

    client = test_clients->data;
    cut_assert_equal_string(chunk, get_received_data(end_of_message_chunk));
}

void
test_end_of_message_quarantine (void)
{
    const gchar chunk[] = "virus!";
    const gchar reason[] = "MAYBE VIRUS";

    arguments_append(arguments1,
                     "--quarantine", reason,
                     NULL);

    cut_trace(test_body(NULL));

    milter_manager_controller_end_of_message(controller, chunk, strlen(chunk));
    wait_response("end-of-message");
    gcut_assert_equal_enum(MILTER_TYPE_STATUS,
                           MILTER_STATUS_CONTINUE, response_status);
    cut_assert_equal_uint(g_list_length(test_clients),
                          collect_n_received(end_of_message));

    cut_trace(milter_manager_test_server_wait_signal(server));

    cut_assert_equal_string(
        reason,
        milter_manager_test_server_get_quarantine_reason(server));
}

void
test_add_header (void)
{
    MilterManagerTestHeader header1 = {0, "X-Test-Header1", "Test Header1 Value"};
    MilterManagerTestHeader header2 = {0, "X-Test-Header2", "Test Header2 Value"};
    const GList *headers;
    gchar *header_string;

    expected_list =
        g_list_append(expected_list,
                      &header1); 
    expected_list =
        g_list_append(expected_list,
                      &header2); 

    header_string = g_strdup_printf("%s:%s", header1.name, header1.value);
    cut_take_string(header_string);
    arguments_append(arguments1,
                     "--add-header", header_string,
                     NULL);
    header_string = g_strdup_printf("%s:%s", header2.name, header2.value);
    cut_take_string(header_string);
    arguments_append(arguments2,
                     "--add-header", header_string,
                     NULL);

    cut_trace(test_end_of_message(NULL));

    cut_trace(milter_manager_test_server_wait_signal(server));

    cut_assert_equal_uint(
        2,
        milter_manager_test_server_get_n_add_headers(server));

    headers =
        g_list_sort((GList*)milter_manager_test_server_get_add_headers(server),
                   (GCompareFunc)milter_manager_test_header_compare);
    gcut_assert_equal_list(expected_list, headers,
                           (GEqualFunc)milter_manager_test_header_equal,
                           (GCutInspectFunc)milter_manager_test_header_inspect_without_index,
                           NULL);
}

void
test_change_from (void)
{
    const gchar from[] = "change@example.com";

    arguments_append(arguments1,
                     "--change-from", from,
                     NULL);

    cut_trace(test_end_of_message(NULL));

    cut_trace(milter_manager_test_server_wait_signal(server));

    cut_assert_equal_uint(
        1,
        milter_manager_test_server_get_n_change_froms(server));

    cut_assert_equal_string(from,
                            milter_manager_test_server_get_from(server));
}

void
test_add_recipient (void)
{
    MilterManagerTestValueWithParam recipient1 = {"add1@example.com", NULL};
    MilterManagerTestValueWithParam recipient2 = {"add2@example.com", NULL};
    MilterManagerTestValueWithParam recipient3 = {"add3@example.com", NULL};
    const GList *actual_recipients;

    expected_list =
        g_list_append(expected_list,
                      &recipient1); 
    expected_list =
        g_list_append(expected_list,
                      &recipient2); 
    expected_list =
        g_list_append(expected_list,
                      &recipient2); 
    expected_list =
        g_list_append(expected_list,
                      &recipient3); 
    
    arguments_append(arguments1,
                     "--add-recipient", recipient1.value,
                     "--add-recipient", recipient2.value,
                     NULL);
    arguments_append(arguments2,
                     "--add-recipient", recipient2.value,
                     "--add-recipient", recipient3.value,
                     NULL);

    cut_trace(test_end_of_message(NULL));

    cut_trace(milter_manager_test_server_wait_signal(server));

    cut_assert_equal_uint(
        4,
        milter_manager_test_server_get_n_add_recipients(server));

    actual_recipients = milter_manager_test_server_get_add_recipients(server);
    gcut_assert_equal_list(expected_list, actual_recipients,
                           (GEqualFunc)milter_manager_test_value_with_param_equal,
                           (GCutInspectFunc)milter_manager_test_value_with_param_inspect,
                           NULL);
}

void
test_delete_recipient (void)
{
    const gchar recipient1[] = "delete1@example.com";
    const gchar recipient2[] = "delete2@example.com";
    const gchar recipient3[] = "delete3@example.com";
    GList *actual_recipients;

    expected_list =
        g_list_append(expected_list,
                      &recipient1); 
    expected_list =
        g_list_append(expected_list,
                      &recipient2); 
    expected_list =
        g_list_append(expected_list,
                      &recipient2); 
    expected_list =
        g_list_append(expected_list,
                      &recipient3); 
    
    arguments_append(arguments1,
                     "--delete-recipient", recipient1,
                     "--delete-recipient", recipient2,
                     NULL);
    arguments_append(arguments2,
                     "--delete-recipient", recipient2,
                     "--delete-recipient", recipient3,
                     NULL);

    cut_trace(test_end_of_message(NULL));

    cut_trace(milter_manager_test_server_wait_signal(server));

    cut_assert_equal_uint(
        4,
        milter_manager_test_server_get_n_delete_recipients(server));

    actual_recipients = 
        g_list_sort((GList*)milter_manager_test_server_get_delete_recipients(server),
                    (GCompareFunc)strcmp);
    gcut_assert_equal_list_string(expected_list, actual_recipients);
}

void
test_progress (void)
{
    cut_trace(test_end_of_message(NULL));

    cut_trace(milter_manager_test_server_wait_signal(server));

    cut_assert_equal_uint(
        2,
        milter_manager_test_server_get_n_progresses(server));
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
