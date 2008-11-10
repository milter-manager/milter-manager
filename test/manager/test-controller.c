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

static GKeyFile *scenario;
static GList *imported_scenarios;

static MilterManagerConfiguration *config;
static MilterClientContext *client_context;
static MilterManagerController *controller;
static MilterManagerTestServer *server;
static MilterOption *option;

static GError *actual_error;
static GError *expected_error;
static gboolean finished;

static GList *test_clients;

static MilterStatus response_status;

void
startup (void)
{
    MILTER_TYPE_CLIENT_CONTEXT_ERROR;
}

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
    if (actual_error)
        g_error_free(actual_error);
    actual_error = g_error_copy(error);
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
    GIOChannel *channel;
    MilterWriter *writer;
    MilterReader *reader;

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
    g_io_channel_unref(channel);

    client_context = milter_client_context_new();
    milter_handler_set_writer(MILTER_HANDLER(client_context), writer);
    g_object_unref(writer);

    server = milter_manager_test_server_new();
    milter_handler_set_reader(MILTER_HANDLER(server), reader);
    g_object_unref(reader);

    controller = milter_manager_controller_new(config, client_context);
    actual_error = NULL;
    setup_controller_signals(controller);

    option = NULL;

    test_clients = NULL;

    response_status = MILTER_STATUS_DEFAULT;

    finished = FALSE;

    expected_error = NULL;
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

    if (test_clients) {
        g_list_foreach(test_clients, (GFunc)g_object_unref, NULL);
        g_list_free(test_clients);
    }

    if (actual_error)
        g_error_free(actual_error);
    if (expected_error)
        g_error_free(expected_error);
}

#define collect_n_received(event)                                       \
    milter_manager_test_clients_collect_n_received(                     \
        test_clients,                                                   \
        milter_manager_test_client_get_n_ ## event ## _received)

#define get_received_data(name)                         \
    milter_manager_test_client_get_ ## name(client)

#define collect_received_strings(name)                  \
    milter_manager_test_clients_collect_strings(        \
        test_clients,                                   \
        milter_manager_test_client_get_ ## name)

#define collect_received_pairs(name, value)             \
    milter_manager_test_clients_collect_pairs(          \
        test_clients,                                   \
        milter_manager_test_client_get_ ## name,        \
        milter_manager_test_client_get_ ## value)

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

#define assert_have_response(name)                              \
    cut_trace_with_info_expression(                             \
        assert_have_response_helper(name "-response", FALSE),   \
        assert_have_response(name))

#define assert_not_have_response(name)                          \
    cut_trace_with_info_expression(                             \
        assert_have_response_helper(name "-response", TRUE),    \
        assert_not_have_response(name))

static void
assert_have_response_helper (const gchar *name, gboolean should_timeout)
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
    timeout_waiting_id = g_timeout_add(50, cb_timeout_waiting,
                                       &timeout_waiting);
    while (timeout_waiting && response_waiting) {
        g_main_context_iteration(NULL, TRUE);
    }
    g_source_remove(timeout_waiting_id);
    g_signal_handler_disconnect(client_context, response_waiting_id);

    if (should_timeout)
        cut_assert_false(timeout_waiting, "<%s>", name);
    else
        cut_assert_true(timeout_waiting, "<%s>", name);
    gcut_assert_error(actual_error);
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
start_client_with_string_list (guint port, const gchar **argument_strings)
{
    GArray *arguments;
    const gchar **argument;

    arguments = g_array_new(TRUE, TRUE, sizeof(gchar *));
    if (argument_strings) {
        for (argument = argument_strings; *argument; argument++) {
            gchar *dupped_argument;

            dupped_argument = g_strdup(*argument);
            g_array_append_val(arguments, dupped_argument);
        }
    }
    cut_take(arguments, (CutDestroyFunction)arguments_free);
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

    timeout_waiting_id = g_timeout_add(50, cb_timeout_waiting,
                                       &timeout_waiting);
    while (timeout_waiting && !finished) {
        g_main_context_iteration(NULL, TRUE);
    }
    g_source_remove(timeout_waiting_id);

    cut_assert_true(timeout_waiting, "timeout");
    cut_assert_true(finished);
}

#define wait_controller_error()                    \
    cut_trace_with_info_expression(                \
        wait_controller_error_helper(),            \
        wait_controller_error())

static void
wait_controller_error_helper (void)
{
    gboolean timeout_waiting = TRUE;
    guint timeout_waiting_id;

    timeout_waiting_id = g_timeout_add(50, cb_timeout_waiting,
                                       &timeout_waiting);
    while (timeout_waiting && !actual_error) {
        g_main_context_iteration(NULL, TRUE);
    }
    g_source_remove(timeout_waiting_id);

    cut_assert_true(timeout_waiting, "timeout");
    cut_assert_not_null(actual_error);
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

static gint
get_integer (GKeyFile *scenario, const gchar *group, const gchar *key)
{
    GError *error = NULL;
    gint value;

    value = g_key_file_get_integer(scenario, group, key, &error);
    gcut_assert_error(error, "[%s]", group);
    return value;
}

static const gchar *
get_string (GKeyFile *scenario, const gchar *group, const gchar *key)
{
    GError *error = NULL;
    gchar *value;

    value = g_key_file_get_string(scenario, group, key, &error);
    gcut_assert_error(error, "[%s]", group);
    return cut_take_string(value);
}

static const gchar **
get_string_list (GKeyFile *scenario, const gchar *group, const gchar *key,
                 gsize *length)
{
    GError *error = NULL;
    gchar **value;

    value = g_key_file_get_string_list(scenario, group, key, length, &error);
    gcut_assert_error(error, "[%s]", group);
    return cut_take_string_array(value);
}

static const GList *
get_string_g_list (GKeyFile *scenario, const gchar *group, const gchar *key)
{
    GError *error = NULL;
    GList *list = NULL;
    const gchar **strings;
    gsize length, i;

    if (g_key_file_has_key(scenario, group, key, &error)) {
        strings = get_string_list(scenario, group, key, &length);
        for (i = 0; i < length; i++) {
            if (strings[i] && strings[i][0] == '\0')
                list = g_list_append(list, NULL);
            else
                list = g_list_append(list, g_strdup(strings[i]));
        }
    }
    gcut_assert_error(error);

    return gcut_take_list(list, g_free);
}

static gint
get_enum (GKeyFile *scenario, const gchar *group, const gchar *key,
          GType enum_type)
{
    GError *error = NULL;
    gint value;

    value = gcut_key_file_get_enum(scenario, group, key, enum_type, &error);
    gcut_assert_error(error, "[%s]", group);
    return value;
}

static const GList *
get_pair_list (GKeyFile *scenario, const gchar *group, const gchar *key)
{
    const GList *sorted_pairs = NULL;
    GError *error = NULL;

    if (g_key_file_has_key(scenario, group, key, &error)) {
        const gchar **pair_strings;
        GList *pairs = NULL;
        gsize length, i;

        pair_strings = get_string_list(scenario, group, key, &length);
        for (i = 0; i < length; i += 2) {
            MilterManagerTestPair *pair;
            const gchar *name, *value;

            name = pair_strings[i];
            value = pair_strings[i + 1];
            if (value && value[0] == '\0')
                value = NULL;
            pair = milter_manager_test_pair_new(name, value);
            pairs = g_list_prepend(pairs, pair);
        }
        sorted_pairs =
            gcut_take_list(
                g_list_sort(pairs, milter_manager_test_pair_compare),
                (CutDestroyFunction)milter_manager_test_pair_free);
    }
    gcut_assert_error(error);

    return sorted_pairs;
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
    const gchar **scenario_names;
    gsize length, i;

    has_key = g_key_file_has_key(scenario, SCENARIO_GROUP, key, &error);
    gcut_assert_error(error);
    if (!has_key)
        return;

    scenario_names = get_string_list(scenario, SCENARIO_GROUP, key, &length);
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
    cut_take_string(scenario_path);
    gcut_assert_error(error, "<%s>", scenario_path);

    cut_trace(import_scenarios(*scenario));
}

static void
setup_client (GKeyFile *scenario, const gchar *group)
{
    GError *error = NULL;
    guint port;
    gsize length;
    const gchar **arguments = NULL;

    port = g_key_file_get_integer(scenario, group, "port", &error);
    gcut_assert_error(error);
    if (g_key_file_has_key(scenario, group, "arguments", &error)) {
        arguments = get_string_list(scenario, group, "arguments", &length);
    }
    gcut_assert_error(error);

    start_client_with_string_list(port, arguments);
}

static void
setup_clients (GKeyFile *scenario)
{
    const gchar **clients;
    gsize length, i;

    clients = get_string_list(scenario, SCENARIO_GROUP, "clients", &length);
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

static void
assert_response_error (GKeyFile *scenario, const gchar *group)
{
    GError *error = NULL;

    if (g_key_file_has_key(scenario, group, "error_domain", &error)) {
        GQuark domain;
        const gchar *domain_name, *type_name, *message;
        GType type;
        gint code;

        domain_name = get_string(scenario, group, "error_domain");
        domain = g_quark_from_string(domain_name);

        type_name = get_string(scenario, group, "error_type");
        type = g_type_from_name(type_name);
        cut_assert_operator_uint(0, <, type, "name: <%s>", type_name);
        code = get_enum(scenario, group, "error_code", type);

        message = get_string(scenario, group, "error_message");
        expected_error = g_error_new(domain, code, "%s", message);
        wait_controller_error();
    }
    gcut_assert_error(error);

    gcut_assert_equal_error(expected_error, actual_error);
    if (expected_error && actual_error)
        cut_return();
}

static void
assert_response_common (GKeyFile *scenario, const gchar *group)
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
        cut_trace(assert_have_response_helper(response_signal_name,
                                              n_received == 0));
    }

    get_n_received = response_to_get_n_received(response);
    cut_assert_not_null(get_n_received, "response: <%s>(%s)", response, group);
    actual_n_received =
        milter_manager_test_clients_collect_n_received(test_clients,
                                                       get_n_received);
    cut_assert_equal_uint(n_received, actual_n_received, "[%s]", group);
    gcut_assert_equal_enum(MILTER_TYPE_STATUS, status, response_status,
                           "[%s]", group);

}

static void
assert_response_reply_code (GKeyFile *scenario, const gchar *group)
{
    const GList *replied_codes;

    replied_codes = get_string_g_list(scenario, group, "reply_codes");
    cut_trace(milter_manager_test_server_wait_signal(server));
    gcut_assert_equal_list_string(
        replied_codes,
        milter_manager_test_server_get_received_reply_codes(server));
}

static void
assert_response (GKeyFile *scenario, const gchar *group)
{
    cut_trace(assert_response_error(scenario, group));
    cut_trace(assert_response_common(scenario, group));
    cut_trace(assert_response_reply_code(scenario, group));
}

static void
do_negotiate (GKeyFile *scenario, const gchar *group)
{
    option = milter_option_new_from_key_file(scenario, group);

    milter_server_context_negotiate(MILTER_SERVER_CONTEXT(server), option);
    milter_manager_controller_negotiate(controller, option);
    cut_trace(assert_response(scenario, group));

    /* FIXME: should check received option */
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
    const GList *expected_connect_infos;
    const GList *actual_connect_infos;

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

    expected_connect_infos = get_pair_list(scenario, group, "infos");
    actual_connect_infos = collect_received_pairs(connect_host, connect_address);
    gcut_assert_equal_list(
        expected_connect_infos, actual_connect_infos,
        milter_manager_test_pair_equal,
        (GCutInspectFunc)milter_manager_test_pair_inspect,
        NULL);
}

static void
do_helo (GKeyFile *scenario, const gchar *group)
{
    MilterManagerTestClient *client;
    const gchar *fqdn;
    const GList *expected_fqdns;

    fqdn = get_string(scenario, group, "fqdn");
    milter_manager_controller_helo(controller, fqdn);

    cut_trace(assert_response(scenario, group));

    client = test_clients->data;
    cut_assert_equal_string(fqdn, get_received_data(helo_fqdn));

    expected_fqdns = get_string_g_list(scenario, group, "fqdns");
    gcut_assert_equal_list_string(expected_fqdns,
                                  collect_received_strings(helo_fqdn),
                                  "[%s]", group);
}

static void
do_envelope_from (GKeyFile *scenario, const gchar *group)
{
    const gchar *from;
    const GList *expected_froms;

    from = get_string(scenario, group, "from");
    milter_manager_controller_envelope_from(controller, from);

    cut_trace(assert_response(scenario, group));

    expected_froms = get_string_g_list(scenario, group, "froms");
    gcut_assert_equal_list_string(expected_froms,
                                  collect_received_strings(envelope_from),
                                  "[%s]", group);
}

static void
do_envelope_recipient (GKeyFile *scenario, const gchar *group)
{
    const gchar *recipient;
    const GList *expected_recipients;

    recipient = get_string(scenario, group, "recipient");
    milter_manager_controller_envelope_recipient(controller, recipient);

    cut_trace(assert_response(scenario, group));

    expected_recipients = get_string_g_list(scenario, group, "recipients");
    gcut_assert_equal_list_string(expected_recipients,
                                  collect_received_strings(envelope_recipient),
                                  "[%s]", group);
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
    const GList *expected_headers;
    const GList *actual_headers;
    const gchar *name, *value;

    name = get_string(scenario, group, "name");
    value = get_string(scenario, group, "value");
    milter_manager_controller_header(controller, name, value);

    cut_trace(assert_response(scenario, group));

    expected_headers = get_pair_list(scenario, group, "headers");
    actual_headers = collect_received_pairs(header_name, header_value);
    gcut_assert_equal_list(
        expected_headers, actual_headers,
        milter_manager_test_pair_equal,
        (GCutInspectFunc)milter_manager_test_pair_inspect,
        NULL);
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
    const gchar *chunk;

    chunk = get_string(scenario, group, "chunk");
    milter_manager_controller_body(controller, chunk, strlen(chunk));

    cut_trace(assert_response(scenario, group));

    gcut_assert_equal_list_string(get_string_g_list(scenario, group, "chunks"),
                                  collect_received_strings(body_chunk),
                                  "<%s>", group);
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

    milter_server_context_set_state(MILTER_SERVER_CONTEXT(server),
                                    MILTER_SERVER_CONTEXT_STATE_END_OF_MESSAGE);
    milter_manager_controller_end_of_message(controller,
                                             chunk,
                                             chunk ? strlen(chunk) : 0);

    cut_trace(assert_response(scenario, group));
    client = test_clients->data;
    cut_assert_equal_string(chunk, get_received_data(end_of_message_chunk));
}

static const GList *
sort_const_pair_list (const GList *pairs)
{
    return gcut_take_list(g_list_sort(g_list_copy((GList *)pairs),
                                      milter_manager_test_pair_compare),
                          NULL);
}

static const GList *
sort_const_string_list (const GList *strings)
{
    return gcut_take_list(g_list_sort(g_list_copy((GList *)strings),
                                      (GCompareFunc)g_utf8_collate),
                          NULL);
}

static void
do_end_of_message_quarantine (GKeyFile *scenario, const gchar *group)
{
    const GList *expected_reasons;
    const GList *actual_reasons;

    expected_reasons = get_string_g_list(scenario, group, "quarantine_reasons");
    actual_reasons =
        milter_manager_test_server_get_received_quarantine_reasons(server);

    gcut_assert_equal_list_string(sort_const_string_list(expected_reasons),
                                  sort_const_string_list(actual_reasons));
}

static const GList *
get_expected_add_headers (GKeyFile *scenario, const gchar *group)
{
    const gchar key[] = "add_headers";
    GError *error = NULL;
    const GList *expected_added_headers = NULL;

    if (g_key_file_has_key(scenario, group, key, &error)) {
        const gchar **added_header_strings;
        GList *added_headers = NULL;
        gsize length;

        added_header_strings = get_string_list(scenario, group, key, &length);
        for (; *added_header_strings; added_header_strings++) {
            MilterManagerTestHeader *header;
            gchar **components;

            components = g_strsplit(*added_header_strings, ":", 3);
            header = milter_manager_test_header_new(atoi(components[0]),
                                                    components[1],
                                                    components[2]);
            g_strfreev(components);
            added_headers = g_list_prepend(added_headers, header);
        }
        expected_added_headers =
            gcut_take_list(
                g_list_sort(added_headers,
                            (GCompareFunc)milter_manager_test_header_compare),
                (CutDestroyFunction)milter_manager_test_header_free);
    }
    gcut_assert_error(error);

    return expected_added_headers;
}

static void
do_end_of_message_add_header (GKeyFile *scenario, const gchar *group)
{
    const GList *expected_added_headers;
    const GList *actual_added_headers;

    expected_added_headers = get_expected_add_headers(scenario, group);

    actual_added_headers =
        milter_manager_test_server_get_received_add_headers(server);
    actual_added_headers =
        gcut_take_list(
            g_list_sort(g_list_copy((GList *)actual_added_headers),
                        (GCompareFunc)milter_manager_test_header_compare),
            NULL);

    gcut_assert_equal_list(
        expected_added_headers, actual_added_headers,
        (GEqualFunc)milter_manager_test_header_equal,
        (GCutInspectFunc)milter_manager_test_header_inspect_without_index,
        NULL);
}

static void
do_end_of_message_change_from (GKeyFile *scenario, const gchar *group)
{
    const GList *expected_froms;
    const GList *actual_froms;

    expected_froms = get_pair_list(scenario, group, "change_froms");
    actual_froms = milter_manager_test_server_get_received_change_froms(server);
    gcut_assert_equal_list(expected_froms,
                           sort_const_pair_list(actual_froms),
                           milter_manager_test_pair_equal,
                           milter_manager_test_pair_inspect,
                           NULL);
}

static void
do_end_of_message_add_recipient (GKeyFile *scenario, const gchar *group)
{
    const GList *expected_recipients;
    const GList *actual_recipients;

    expected_recipients = get_pair_list(scenario, group, "add_recipients");
    actual_recipients =
        milter_manager_test_server_get_received_add_recipients(server);
    gcut_assert_equal_list(expected_recipients,
                           sort_const_pair_list(actual_recipients),
                           milter_manager_test_pair_equal,
                           milter_manager_test_pair_inspect,
                           NULL);
}

static void
do_end_of_message_delete_recipient (GKeyFile *scenario, const gchar *group)
{
    const GList *expected_recipients;
    const GList *actual_recipients;

    expected_recipients =
        get_string_g_list(scenario, group, "delete_recipients");
    actual_recipients =
        milter_manager_test_server_get_received_delete_recipients(server);
    gcut_assert_equal_list_string(sort_const_string_list(expected_recipients),
                                  sort_const_string_list(actual_recipients));
}

static void
do_end_of_message_replace_body (GKeyFile *scenario, const gchar *group)
{
    const GList *expected_bodies;
    const GList *actual_bodies;

    expected_bodies = get_string_g_list(scenario, group, "replace_bodies");
    actual_bodies =
        milter_manager_test_server_get_received_replace_bodies(server);
    gcut_assert_equal_list_string(expected_bodies,
                                  actual_bodies);
}

static void
do_end_of_message_progress (GKeyFile *scenario, const gchar *group)
{
    const gchar key[] = "n_progresses";
    GError *error = NULL;
    guint expected_n_progresses = 0;

    if (g_key_file_has_key(scenario, group, key, &error)) {
        expected_n_progresses = get_integer(scenario, group, key);
    }
    gcut_assert_error(error);
    cut_assert_equal_uint(expected_n_progresses,
                          milter_manager_test_server_get_n_progresses(server));
}

static void
do_end_of_message_full (GKeyFile *scenario, const gchar *group)
{
    cut_trace(do_end_of_message(scenario, group));
    cut_trace(milter_manager_test_server_wait_signal(server));
    cut_trace(do_end_of_message_quarantine(scenario, group));
    cut_trace(do_end_of_message_add_header(scenario, group));
    cut_trace(do_end_of_message_change_from(scenario, group));
    cut_trace(do_end_of_message_add_recipient(scenario, group));
    cut_trace(do_end_of_message_delete_recipient(scenario, group));
    cut_trace(do_end_of_message_replace_body(scenario, group));
    cut_trace(do_end_of_message_progress(scenario, group));
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
    gcut_assert_error(error, "[%s]", group);

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
        cut_trace(do_end_of_message_full(scenario, group));
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

static void
data_scenario_basic (void)
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
        "end-of-message - chunk", g_strdup("end-of-message-chunk.txt"), g_free,
        "quit", g_strdup("quit.txt"), g_free,
        "abort", g_strdup("abort.txt"), g_free,
        "unknown", g_strdup("unknown.txt"), g_free,
        "reply-code", g_strdup("reply-code.txt"), g_free);
}

static void
data_scenario_end_of_message_action (void)
{
    cut_add_data("quarantine", g_strdup("quarantine.txt"), g_free,
                 "add-header", g_strdup("add-header.txt"), g_free,
                 "change-from", g_strdup("change-from.txt"), g_free,
                 "add-recipient", g_strdup("add-recipient.txt"), g_free,
                 "delete-recipient", g_strdup("delete-recipient.txt"), g_free,
                 "replace-body", g_strdup("replace-body.txt"), g_free,
                 "progress", g_strdup("progress.txt"), g_free);

    cut_add_data("replace-body - separate",
                 g_strdup("replace-body-separate.txt"), g_free);
}

static void
data_scenario_envelope_from (void)
{
    cut_add_data("envelope-from - reject",
                 g_strdup("envelope-from-reject.txt"), g_free,
                 "envelope-from - discard",
                 g_strdup("envelope-from-discard.txt"), g_free,
                 "envelope-from - reject & discard",
                 g_strdup("envelope-from-reject-and-discard.txt"), g_free,
                 "envelope-from - reject & accept",
                 g_strdup("envelope-from-reject-and-accept.txt"), g_free,
                 "envelope-from - accept",
                 g_strdup("envelope-from-accept.txt"), g_free,
                 "envelope-from - accept all",
                 g_strdup("envelope-from-accept-all.txt"), g_free);
}

static void
data_scenario_envelope_recipient (void)
{
    cut_add_data(
        "envelope-recipient - accept",
        g_strdup("envelope-recipient-accept.txt"), g_free,
        "envelope-recipient - accept - all",
        g_strdup("envelope-recipient-accept-all.txt"), g_free,
        "envelope-recipient - reject",
        g_strdup("envelope-recipient-reject.txt"), g_free,
        "envelope-recipient - reject & temporary-failure & discard",
        g_strdup("envelope-recipient-reject-and-temporary-failure-and-discard.txt"),
        g_free,
        "envelope-recipient - reject & discard - simultaneously",
        g_strdup("envelope-recipient-reject-and-discard-simultaneously.txt"),
        g_free,
        "envelope-recipient - discard",
        g_strdup("envelope-recipient-discard.txt"), g_free,
        "envelope-recipient - temporary-failure",
        g_strdup("envelope-recipient-temporary-failure.txt"), g_free,
        "envelope-recipient - temporary-failure - all",
        g_strdup("envelope-recipient-temporary-failure-all.txt"), g_free);
}

static void
data_scenario_body (void)
{
    cut_add_data("body - skip", g_strdup("body-skip.txt"), g_free,
                 "body - skip all", g_strdup("body-skip-all.txt"), g_free,
                 "body - accept", g_strdup("body-accept.txt"), g_free);
}

static void
data_scenario_reply_code (void)
{
    cut_add_data("reply-code - invalid",
                 g_strdup("reply-code-invalid.txt"), g_free);
}

void
data_scenario (void)
{
    data_scenario_basic();
    data_scenario_end_of_message_action();
    data_scenario_envelope_from();
    data_scenario_envelope_recipient();
    data_scenario_body();
    data_scenario_reply_code();
}

void
test_scenario (gconstpointer data)
{
    const gchar *scenario_name = data;
    const gchar omit_key[] = "omit";
    GError *error = NULL;

    cut_trace(load_scenario(scenario_name, &scenario));

    if (g_key_file_has_key(scenario, SCENARIO_GROUP, omit_key, &error))
        cut_omit("%s", get_string(scenario, SCENARIO_GROUP, omit_key));
    gcut_assert_error(error);

    cut_trace(setup_clients(scenario));
    cut_trace(g_list_foreach(imported_scenarios, (GFunc)do_actions, NULL));
    cut_trace(do_actions(scenario));
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
