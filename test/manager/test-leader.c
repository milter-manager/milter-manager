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

#include <string.h>

#include <milter/manager/milter-manager-leader.h>
#include <milter/manager/milter-manager-enum-types.h>

#include <milter-test-utils.h>
#include <milter-manager-test-utils.h>
#include <milter-manager-test-client.h>
#include <milter-manager-test-server.h>
#include <milter-manager-test-scenario.h>

#include <gcutter.h>


void data_scenario (void);
void test_scenario (gconstpointer data);

void data_large_body (void);
void test_large_body (gconstpointer data);

void test_configuration (void);

static gchar *scenario_dir;
static MilterManagerTestScenario *main_scenario;

static MilterEventLoop *loop;

static MilterManagerConfiguration *config;
static MilterClientContext *client_context;
static MilterManagerLeader *leader;
static MilterManagerTestServer *server;

static GError *actual_error;
static GError *expected_error;
static gboolean finished;

static gint n_negotiate_responses;
static gint n_connect_responses;
static gint n_helo_responses;
static gint n_envelope_from_responses;
static gint n_envelope_recipient_responses;
static gint n_data_responses;
static gint n_header_responses;
static gint n_end_of_header_responses;
static gint n_body_responses;
static gint n_end_of_message_responses;
static gint n_abort_responses;
static gint n_unknown_responses;

static MilterStatus response_status;

void
cut_startup (void)
{
    MILTER_TYPE_CLIENT_CONTEXT_ERROR;
    MILTER_TYPE_MANAGER_CHILDREN_ERROR;
}

static void
cb_error (MilterManagerLeader *leader, GError *error, gpointer user_data)
{
    if (actual_error)
        g_error_free(actual_error);
    actual_error = g_error_copy(error);
}

static void
cb_finished (MilterManagerLeader *leader, gpointer user_data)
{
    finished = TRUE;
}

static void
setup_leader_signals (MilterManagerLeader *leader)
{
#define CONNECT(name)                                                   \
    g_signal_connect(leader, #name, G_CALLBACK(cb_ ## name), NULL)

    CONNECT(error);
    CONNECT(finished);

#undef CONNECT
}

static MilterStatus
cb_negotiate_response (MilterClientContext *context, MilterOption *option,
                       MilterMacrosRequests *macros_requests,
                       MilterStatus status, gpointer user_data)
{
    n_negotiate_responses++;

    response_status = status;
    return status;
}

static MilterStatus
cb_connect_response (MilterClientContext *context,
                     MilterStatus status, gpointer user_data)
{
    n_connect_responses++;

    response_status = status;
    return status;
}

static MilterStatus
cb_helo_response (MilterClientContext *context,
                  MilterStatus status, gpointer user_data)
{
    n_helo_responses++;

    response_status = status;
    return status;
}

static MilterStatus
cb_envelope_from_response (MilterClientContext *context,
                           MilterStatus status, gpointer user_data)
{
    n_envelope_from_responses++;

    response_status = status;
    return status;
}

static MilterStatus
cb_envelope_recipient_response (MilterClientContext *context,
                                MilterStatus status, gpointer user_data)
{
    n_envelope_recipient_responses++;

    response_status = status;
    return status;
}

static MilterStatus
cb_data_response (MilterClientContext *context,
                  MilterStatus status, gpointer user_data)
{
    n_data_responses++;

    response_status = status;
    return status;
}

static MilterStatus
cb_header_response (MilterClientContext *context,
                    MilterStatus status, gpointer user_data)
{
    n_header_responses++;

    response_status = status;
    return status;
}

static MilterStatus
cb_end_of_header_response (MilterClientContext *context,
                           MilterStatus status, gpointer user_data)
{
    n_end_of_header_responses++;

    response_status = status;
    return status;
}

static MilterStatus
cb_body_response (MilterClientContext *context,
                  MilterStatus status, gpointer user_data)
{
    n_body_responses++;

    response_status = status;
    return status;
}

static MilterStatus
cb_end_of_message_response (MilterClientContext *context,
                            MilterStatus status, gpointer user_data)
{
    n_end_of_message_responses++;

    response_status = status;
    return status;
}

static MilterStatus
cb_abort_response (MilterClientContext *context,
                   MilterStatus status, gpointer user_data)
{
    n_abort_responses++;

    response_status = status;
    return status;
}

static MilterStatus
cb_unknown_response (MilterClientContext *context,
                     MilterStatus status, gpointer user_data)
{
    n_unknown_responses++;

    response_status = status;
    return status;
}

static void
setup_client_context_signals (MilterClientContext *context)
{
#define CONNECT(name) do                                                \
{                                                                       \
    g_signal_connect(context, #name "_response",                        \
                         G_CALLBACK(cb_ ## name ## _response), NULL);   \
} while (0)

    CONNECT(negotiate);
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
    CONNECT(unknown);

#undef CONNECT
}

void
cut_setup (void)
{
    GIOChannel *channel, *null_channel;
    MilterWriter *writer, *null_writer;
    MilterReader *reader;
    GError *error = NULL;

    scenario_dir = g_build_filename(milter_test_get_source_dir(),
                                    "test",
                                    "fixtures",
                                    "leader",
                                    NULL);

    main_scenario = NULL;

    loop = milter_test_event_loop_new();

    config = milter_manager_configuration_new(NULL);
    milter_manager_configuration_load(config, "milter-manager.conf", &error);
    gcut_assert_error(error);

    channel = gcut_string_io_channel_new(NULL);
    g_io_channel_set_encoding(channel, NULL, NULL);
    g_io_channel_set_flags(channel, G_IO_FLAG_NONBLOCK, NULL);
    gcut_string_io_channel_set_pipe_mode(channel, TRUE);
    writer = milter_writer_io_channel_new(channel);
    reader = milter_reader_io_channel_new(channel);
    g_io_channel_unref(channel);

    client_context = milter_client_context_new(NULL);
    milter_agent_set_writer(MILTER_AGENT(client_context), writer);
    g_object_unref(writer);
    milter_agent_set_event_loop(MILTER_AGENT(client_context), loop);
    milter_agent_start(MILTER_AGENT(client_context), &error);
    gcut_assert_error(error);

    null_channel = gcut_string_io_channel_new(NULL);
    g_io_channel_set_encoding(null_channel, NULL, NULL);
    g_io_channel_set_flags(null_channel, G_IO_FLAG_NONBLOCK, NULL);
    null_writer = milter_writer_io_channel_new(null_channel);
    g_io_channel_unref(null_channel);

    server = milter_manager_test_server_new();
    milter_agent_set_reader(MILTER_AGENT(server), reader);
    g_object_unref(reader);
    milter_agent_set_writer(MILTER_AGENT(server), null_writer);
    g_object_unref(null_writer);
    milter_agent_set_event_loop(MILTER_AGENT(server), loop);
    milter_agent_start(MILTER_AGENT(server), &error);
    gcut_assert_error(error);

    leader = milter_manager_leader_new(config, client_context);
    actual_error = NULL;
    setup_leader_signals(leader);
    setup_client_context_signals(client_context);

    response_status = MILTER_STATUS_DEFAULT;

    finished = FALSE;

    n_negotiate_responses = 0;
    n_connect_responses = 0;
    n_helo_responses = 0;
    n_envelope_from_responses = 0;
    n_envelope_recipient_responses = 0;
    n_header_responses = 0;
    n_end_of_header_responses = 0;
    n_data_responses = 0;
    n_body_responses = 0;
    n_end_of_message_responses = 0;
    n_abort_responses = 0;
    n_unknown_responses = 0;

    expected_error = NULL;
}

static void
teardown_error (void)
{
    if (actual_error) {
        g_error_free(actual_error);
        actual_error = NULL;
    }

    if (expected_error) {
        g_error_free(expected_error);
        expected_error = NULL;
    }
}

void
cut_teardown (void)
{
    if (config)
        g_object_unref(config);
    if (client_context)
        g_object_unref(client_context);
    if (leader)
        g_object_unref(leader);

    if (server)
        g_object_unref(server);

    if (loop)
        g_object_unref(loop);

    if (scenario_dir)
        g_free(scenario_dir);
    if (main_scenario)
        g_object_unref(main_scenario);

    teardown_error();
}

#define pump_all_events_without_error_check()           \
    cut_trace_with_info_expression(                     \
        pump_all_events_without_error_check_helper(),   \
        pump_all_events_without_error_check())

#define pump_all_events()                       \
    cut_trace_with_info_expression(             \
        pump_all_events_helper(),               \
        pump_all_events())

static void
pump_all_events_without_error_check_helper (void)
{
    if (MILTER_IS_LIBEV_EVENT_LOOP(loop))
        cut_omit("MilterLibevEventLoop doesn't support GCutStringIOChannel.");

    milter_test_pump_all_events(loop);
}

static void
pump_all_events_helper (void)
{
    GError *error;

    pump_all_events_without_error_check_helper();
    error = actual_error;
    actual_error = NULL;
    gcut_assert_error(error);
}

#define started_clients                                                 \
    milter_manager_test_scenario_get_started_clients(main_scenario)

#define collect_n_received(event)                                       \
    milter_manager_test_clients_collect_n_received(                     \
        started_clients,                                                \
        milter_manager_test_client_get_n_ ## event ## _received)

#define get_received_data(name)                         \
    milter_manager_test_client_get_ ## name(client)

#define collect_received_strings(name)                  \
    milter_manager_test_clients_collect_strings(        \
        started_clients,                                \
        milter_manager_test_client_get_ ## name)

#define collect_received_pairs(name, value)             \
    milter_manager_test_clients_collect_pairs(          \
        started_clients,                                \
        milter_manager_test_client_get_ ## name,        \
        milter_manager_test_client_get_ ## value)

#define collect_received_macros(command)                \
    milter_manager_test_clients_collect_macros(         \
        started_clients, command)

static gboolean
cb_timeout_emitted (gpointer data)
{
    gboolean *emitted = data;

    *emitted = TRUE;
    return FALSE;
}

static guint
get_response_count (const gchar *response)
{
    if (g_str_equal(response, "negotiate-response")) {
        return n_negotiate_responses;
    } else if (g_str_equal(response, "connect-response")) {
        return n_connect_responses;
    } else if (g_str_equal(response, "helo-response")) {
        return n_helo_responses;
    } else if (g_str_equal(response, "envelope-from-response")) {
        return n_envelope_from_responses;
    } else if (g_str_equal(response, "envelope-recipient-response")) {
        return n_envelope_recipient_responses;
    } else if (g_str_equal(response, "data-response")) {
        return n_data_responses;
    } else if (g_str_equal(response, "header-response")) {
        return n_header_responses;
    } else if (g_str_equal(response, "end-of-header-response")) {
        return n_end_of_header_responses;
    } else if (g_str_equal(response, "body-response")) {
        return n_body_responses;
    } else if (g_str_equal(response, "end-of-message-response")) {
        return n_end_of_message_responses;
    } else if (g_str_equal(response, "abort-response")) {
        return n_abort_responses;
    } else if (g_str_equal(response, "unknown-response")) {
        return n_unknown_responses;
    }

    return 0;
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
assert_have_response_helper (const gchar *name)
{
    GError *error;
    gboolean timeout_emitted = FALSE;
    guint timeout_emitted_id;

    timeout_emitted_id =
        milter_event_loop_add_timeout(loop, 3,
                                      cb_timeout_emitted, &timeout_emitted);
    while (!timeout_emitted &&
           get_response_count(name) == 0) {
        milter_event_loop_iterate(loop, TRUE);
    }
    milter_event_loop_remove(loop, timeout_emitted_id);

    cut_set_message("<%s>", name);
    cut_assert_false(timeout_emitted);
    error = actual_error;
    actual_error = NULL;
    gcut_assert_error(error);
}

#define wait_reply_from_client(client, actual_getter)   \
    cut_trace_with_info_expression(                     \
        milter_manager_test_client_wait_reply(          \
            client, actual_getter),                     \
        wait_reply_from_client(client, actual_getter))

#define assert_nothing_output(client)                                   \
    cut_trace_with_info_expression(                                     \
        milter_manager_test_client_assert_nothing_output(client),       \
        assert_nothing_output(client))


#define wait_finished()                    \
    cut_trace_with_info_expression(        \
        wait_finished_helper(),            \
        wait_finished())

static void
wait_finished_helper (void)
{
    gboolean timeout_emitted = FALSE;
    guint timeout_emitted_id;

    timeout_emitted_id =
        milter_event_loop_add_timeout(loop, 0.1,
                                      cb_timeout_emitted, &timeout_emitted);
    while (!timeout_emitted && !finished) {
        milter_event_loop_iterate(loop, TRUE);
    }
    milter_event_loop_remove(loop, timeout_emitted_id);

    cut_assert_false(timeout_emitted);
    cut_assert_true(finished);
}

#define wait_leader_error()                    \
    cut_trace_with_info_expression(            \
        wait_leader_error_helper(),            \
        wait_leader_error())

static void
wait_leader_error_helper (void)
{
    gboolean timeout_emitted = FALSE;
    guint timeout_emitted_id;

    pump_all_events_without_error_check();

    timeout_emitted_id =
        milter_event_loop_add_timeout(loop, 0.1,
                                      cb_timeout_emitted, &timeout_emitted);
    while (!timeout_emitted && !actual_error) {
        milter_event_loop_iterate(loop, TRUE);
    }
    milter_event_loop_remove(loop, timeout_emitted_id);

    cut_assert_false(timeout_emitted);
    cut_assert_not_null(actual_error);
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
    } else if (g_str_equal(response, "abort")) {
        return milter_manager_test_client_get_n_abort_received;
    } else if (g_str_equal(response, "unknown")) {
        return milter_manager_test_client_get_n_unknown_received;
    }

    return NULL;
}

#define has_key(scenario, group, key)                                     \
    milter_manager_test_scenario_has_key(scenario, group, key)

#define get_integer(scenario, group, key)                               \
    milter_manager_test_scenario_get_integer(scenario, group, key)

#define get_boolean(scenario, group, key)                               \
    milter_manager_test_scenario_get_boolean(scenario, group, key)

#define get_string(scenario, group, key)                                \
    milter_manager_test_scenario_get_string(scenario, group, key)

#define get_string_with_sub_key(scenario, group, key, sub_key)          \
    milter_manager_test_scenario_get_string_with_sub_key(scenario, group, key, sub_key)

#define get_string_list(scenario, group, key, length)                   \
    milter_manager_test_scenario_get_string_list(scenario, group, key, length)

#define get_string_g_list(scenario, group, key)                         \
    milter_manager_test_scenario_get_string_g_list(scenario, group, key)

#define get_enum(scenario, group, key, enum_type)                       \
    milter_manager_test_scenario_get_enum(scenario, group, key, enum_type)

#define get_option_list(scenario, group, key)                           \
    milter_manager_test_scenario_get_option_list(scenario, group, key)

#define get_header_list(scenario, group, key)                           \
    milter_manager_test_scenario_get_header_list(scenario, group, key)

#define get_pair_list(scenario, group, key)                             \
    milter_manager_test_scenario_get_pair_list(scenario, group, key)

#define get_pair_list_without_sort(scenario, group, key)                         \
    milter_manager_test_scenario_get_pair_list_without_sort(scenario, group, key)

static const GHashTable *
get_hash_table (MilterManagerTestScenario *scenario,
                const gchar *group,
                const gchar *key)
{
    const GList *pair_list, *node;
    GHashTable *hash_table;

    hash_table = g_hash_table_new_full(g_str_hash, g_str_equal,
                                       g_free, g_free);

    pair_list = get_pair_list(scenario, group, key);

    for (node = pair_list; node; node = g_list_next(node)) {
        MilterManagerTestPair *pair = node->data;

        g_hash_table_insert(hash_table,
                            g_strdup(pair->name),
                            g_strdup(pair->value));
    }

    return gcut_take_hash_table(hash_table);
}

#define return_on_error() do {                  \
    if (expected_error && actual_error) {       \
        return;                                 \
    }                                           \
} while (0)

static void
assert_response_error (MilterManagerTestScenario *scenario, const gchar *group)
{
    if (has_key(scenario, group, "error_domain")) {
        GQuark domain;
        const gchar *domain_name, *type_name, *message;
        GType type;
        gint code;

        domain_name = get_string(scenario, group, "error_domain");
        domain = g_quark_from_string(domain_name);

        type_name = get_string(scenario, group, "error_type");
        type = g_type_from_name(type_name);
        cut_assert_operator_uint(0, <, type,
                                 cut_message("name: <%s>", type_name));
        code = get_enum(scenario, group, "error_code", type);

        message = get_string(scenario, group, "error_message");
        if (expected_error)
            g_error_free(expected_error);
        expected_error = g_error_new(domain, code, "%s", message);
        wait_leader_error();
    } else {
        pump_all_events();
    }

    gcut_assert_equal_error(expected_error, actual_error);
}

#define assert_n_received(scenario, group, n_received_key, response_name) \
    cut_trace_with_info_expression(                                     \
        assert_n_received_helper(scenario, group,                       \
                                 n_received_key, response_name),        \
        assert_n_received(scenario, group, n_received_key, response_name))

static void
assert_n_received_helper (MilterManagerTestScenario *scenario,
                          const gchar *group,
                          const gchar *n_received_key,
                          const gchar *response_name)
{
    MilterManagerTestClientGetNReceived get_n_received;
    guint n_received, actual_n_received;

    get_n_received = response_to_get_n_received(response_name);
    cut_set_message("response: <%s>(%s)", response_name, group);
    cut_assert_not_null(get_n_received);

    n_received = get_integer(scenario, group, n_received_key);
    if (n_received > g_list_length((GList *)started_clients)) {
        milter_manager_test_clients_wait_reply(started_clients,
                                               get_n_received);
    } else if (n_received > 0) {
        milter_manager_test_clients_wait_n_replies(started_clients,
                                                   get_n_received,
                                                   n_received);
    }

    actual_n_received =
        milter_manager_test_clients_collect_n_received(started_clients,
                                                       get_n_received);
    cut_set_message("[%s]<%s>: <%s>=<%u>",
                    group, response_name, n_received_key, n_received);
    cut_assert_equal_uint(n_received, actual_n_received);
}

static void
assert_response_common (MilterManagerTestScenario *scenario, const gchar *group)
{
    const gchar *response = NULL;
    MilterStatus status;

    if (g_str_equal(group, "abort")) {
        const GList *node;
        for (node = started_clients; node; node = g_list_next(node)) {
            const gchar *name;

            MilterManagerTestClient *client = node->data;
            name = milter_manager_test_client_get_name(client);

            response = get_string_with_sub_key(scenario, group, "response", name);
            if (g_str_equal(response, "none")) {
                assert_nothing_output(client);
            } else {
                MilterManagerTestClientGetNReceived get_n_received;
                get_n_received = response_to_get_n_received(response);
                wait_reply_from_client(client, get_n_received);
            }
        }
    } else {
        response = get_string(scenario, group, "response");
    }
    status = get_enum(scenario, group, "status", MILTER_TYPE_STATUS);

    if (g_str_equal(response, "quit")) {
        wait_finished();
    } else if (g_str_equal(response, "abort") ||
               g_str_equal(response, "none")) {
        /* Do nothing */
    } else {
        const gchar *response_signal_name;

        response_signal_name = cut_take_printf("%s-response", response);
        cut_trace(assert_have_response_helper(response_signal_name));
    }

    if (!g_str_equal(response, "quit") && !g_str_equal(response, "none")) {
        assert_n_received(scenario, group, "n_received", response);
    }

    if (has_key(scenario, group, "n_abort")) {
        assert_n_received(scenario, group, "n_abort", "abort");
    }

    milter_event_loop_iterate(loop, FALSE);
    cut_set_message("[%s]", group);
    gcut_assert_equal_enum(MILTER_TYPE_STATUS, status, response_status);

}

static void
assert_response_reply_code (MilterManagerTestScenario *scenario, const gchar *group)
{
    const GList *replied_codes;

    replied_codes = get_string_g_list(scenario, group, "reply_codes");
    pump_all_events();
    gcut_assert_equal_list_string(
        replied_codes,
        milter_manager_test_server_get_received_reply_codes(server));
}

#define assert_response(scenario, group) do {           \
    cut_trace_with_info_expression(                     \
        assert_response_helper(scenario, group),        \
        assert_response(scenario, group));              \
    return_on_error();                                  \
} while (0)

static void
assert_response_helper (MilterManagerTestScenario *scenario, const gchar *group)
{
    cut_trace(assert_response_error(scenario, group));
    return_on_error();
    cut_trace(assert_response_common(scenario, group));
    cut_trace(assert_response_reply_code(scenario, group));
}

#define assert_alive(scenario, group)           \
    cut_trace_with_info_expression(             \
        assert_alive_helper(scenario, group),   \
        assert_alive(scenario, group))

static void
assert_alive_helper (MilterManagerTestScenario *scenario, const gchar *group)
{
    guint expected_n_alive, actual_n_alive;

    if (!has_key(scenario, group, "n_alive"))
        return;

    expected_n_alive = get_integer(scenario, group, "n_alive");
    milter_manager_test_clients_wait_n_alive(started_clients, expected_n_alive);
    actual_n_alive =
        milter_manager_test_clients_collect_n_alive(started_clients);
    cut_assert_equal_uint(expected_n_alive, actual_n_alive);
}

#define assert_macros(scenario, group, command)         \
    cut_trace_with_info_expression(                     \
        assert_macros_helper(scenario, group, command), \
        assert_macros(scenario, group, command))

static void
assert_macros_helper (MilterManagerTestScenario *scenario, const gchar *group,
                      MilterCommand command)
{
    const GList *expected_macros;
    const GList *actual_macros;

    expected_macros = get_pair_list_without_sort(scenario, group, "macros");
    if (expected_macros) {
        actual_macros = collect_received_macros(command);
        gcut_assert_equal_list(
            expected_macros, actual_macros,
            milter_manager_test_pair_equal,
            (GCutInspectFunction)milter_manager_test_pair_inspect,
            NULL);
    }
}

static gboolean
option_equal (gconstpointer value1, gconstpointer value2)
{
    MilterOption *option1, *option2;

    option1 = MILTER_OPTION(value1);
    option2 = MILTER_OPTION(value2);

    if (option1 == option2) {
        return TRUE;
    } else if (option1 == NULL || option2 == NULL) {
        return FALSE;
    } else {
        return milter_option_equal(option1, option2);
    }
}

static void
do_negotiate (MilterManagerTestScenario *scenario, const gchar *group)
{
    MilterOption *option;
    const GList *options;

    option = milter_manager_test_scenario_get_option(scenario, group,
                                                     main_scenario);
    gcut_take_object(G_OBJECT(option));
    milter_server_context_negotiate(MILTER_SERVER_CONTEXT(server), option);
    milter_manager_leader_negotiate(leader, option, NULL);
    assert_response(scenario, group);
    assert_alive(scenario, group);

    if (milter_manager_test_scenario_has_group(main_scenario, group) &&
        milter_manager_test_scenario_has_key(main_scenario, group, "options")) {
        options = get_option_list(main_scenario, group, "options");
    } else {
        options = get_option_list(scenario, group, "options");
    }

    gcut_assert_equal_list_object_custom(
        options,
        milter_manager_test_clients_collect_negotiate_options(started_clients),
        option_equal);

    if (milter_manager_test_scenario_has_group(main_scenario, group) &&
        milter_manager_test_scenario_has_key(main_scenario, group, "option")) {
        MilterManagerChildren *children;
        options = get_option_list(main_scenario, group, "option");
        children = milter_manager_leader_get_children(leader);
        gcut_assert_equal_object_custom(
            options->data,
            milter_manager_children_get_option(children),
            option_equal);
    }
}

static void
define_macro (MilterManagerTestScenario *scenario, const gchar *group,
              MilterCommand command)
{
    const GHashTable *macros;

    macros = get_hash_table(scenario, group, "macro");
    if (macros)
        milter_manager_leader_define_macro(leader, command, (GHashTable *)macros);
}

static void
do_connect (MilterManagerTestScenario *scenario, const gchar *group)
{
    GError *error = NULL;
    const gchar *host;
    const gchar *address_spec;
    gint domain;
    struct sockaddr *address;
    socklen_t address_size;
    const GList *expected_connect_infos;
    const GList *actual_connect_infos;
    MilterStatus status;

    host = get_string(scenario, group, "host");
    address_spec = get_string(scenario, group, "address");

    milter_connection_parse_spec(address_spec,
                                 &domain, &address, &address_size,
                                 &error);
    gcut_assert_error(error);

    milter_server_context_connect(MILTER_SERVER_CONTEXT(server),
                                  host, address, address_size);
    define_macro(scenario, group, MILTER_COMMAND_CONNECT);
    status = milter_manager_leader_connect(leader, host, address, address_size);
    g_free(address);
    if (status != MILTER_STATUS_PROGRESS) {
        response_status = status;
    }

    assert_response(scenario, group);
    assert_alive(scenario, group);

    expected_connect_infos =
        get_pair_list_without_sort(scenario, group, "infos");
    actual_connect_infos = collect_received_pairs(connect_host, connect_address);
    gcut_assert_equal_list(
        expected_connect_infos, actual_connect_infos,
        milter_manager_test_pair_equal,
        (GCutInspectFunction)milter_manager_test_pair_inspect,
        NULL);

    assert_macros(scenario, group, MILTER_COMMAND_CONNECT);
}

static void
do_helo (MilterManagerTestScenario *scenario, const gchar *group)
{
    const gchar *fqdn;
    const GList *expected_fqdns;
    MilterStatus status;

    fqdn = get_string(scenario, group, "fqdn");
    milter_server_context_helo(MILTER_SERVER_CONTEXT(server), fqdn);
    status = milter_manager_leader_helo(leader, fqdn);
    if (status != MILTER_STATUS_PROGRESS) {
        response_status = status;
    }

    assert_response(scenario, group);
    assert_alive(scenario, group);

    expected_fqdns = get_string_g_list(scenario, group, "fqdns");
    gcut_assert_equal_list_string(expected_fqdns,
                                  collect_received_strings(helo_fqdn),
                                  cut_message("[%s]", group));
}

static void
do_envelope_from (MilterManagerTestScenario *scenario, const gchar *group)
{
    const gchar *from;
    const GList *expected_froms;
    MilterStatus status;

    from = get_string(scenario, group, "from");
    milter_server_context_envelope_from(MILTER_SERVER_CONTEXT(server), from);
    define_macro(scenario, group, MILTER_COMMAND_ENVELOPE_FROM);
    status = milter_manager_leader_envelope_from(leader, from);
    if (status != MILTER_STATUS_PROGRESS) {
        response_status = status;
    }

    assert_response(scenario, group);
    assert_alive(scenario, group);

    expected_froms = get_string_g_list(scenario, group, "froms");
    gcut_assert_equal_list_string(expected_froms,
                                  collect_received_strings(envelope_from),
                                  cut_message("[%s]", group));

    assert_macros(scenario, group, MILTER_COMMAND_ENVELOPE_FROM);
}

static void
do_envelope_recipient (MilterManagerTestScenario *scenario, const gchar *group)
{
    const gchar *recipient;
    const GList *expected_recipients;
    MilterStatus status;

    recipient = get_string(scenario, group, "recipient");
    n_envelope_recipient_responses = 0;
    milter_server_context_envelope_recipient(MILTER_SERVER_CONTEXT(server),
                                             recipient);
    status = milter_manager_leader_envelope_recipient(leader, recipient);
    if (status != MILTER_STATUS_PROGRESS) {
        response_status = status;
    }

    assert_response(scenario, group);
    assert_alive(scenario, group);

    expected_recipients = get_string_g_list(scenario, group, "recipients");
    gcut_assert_equal_list_string(expected_recipients,
                                  collect_received_strings(envelope_recipient),
                                  cut_message("[%s]", group));
}

static void
do_data (MilterManagerTestScenario *scenario, const gchar *group)
{
    MilterStatus status;

    milter_server_context_data(MILTER_SERVER_CONTEXT(server));
    status = milter_manager_leader_data(leader);
    if (status != MILTER_STATUS_PROGRESS) {
        response_status = status;
    }

    assert_response(scenario, group);
}

static void
do_header (MilterManagerTestScenario *scenario, const gchar *group)
{
    const GList *expected_headers;
    const GList *actual_headers;
    const gchar *name, *value;
    MilterStatus status;

    name = get_string(scenario, group, "name");
    value = get_string(scenario, group, "value");
    n_header_responses = 0;
    milter_server_context_header(MILTER_SERVER_CONTEXT(server), name, value);
    status = milter_manager_leader_header(leader, name, value);
    if (status != MILTER_STATUS_PROGRESS) {
        response_status = status;
    }

    milter_manager_test_server_append_header(server, name, value);

    assert_response(scenario, group);
    assert_alive(scenario, group);

    expected_headers = get_pair_list(scenario, group, "headers");
    actual_headers =
        g_list_sort((GList*)collect_received_pairs(header_name, header_value),
                     milter_manager_test_pair_compare);
    gcut_assert_equal_list(
        expected_headers, actual_headers,
        milter_manager_test_pair_equal,
        (GCutInspectFunction)milter_manager_test_pair_inspect,
        NULL,
        cut_message("[%s] <%s>:<%s>", group, name, value));
}

static void
do_end_of_header (MilterManagerTestScenario *scenario, const gchar *group)
{
    MilterStatus status;

    milter_server_context_end_of_header(MILTER_SERVER_CONTEXT(server));
    status = milter_manager_leader_end_of_header(leader);
    if (status != MILTER_STATUS_PROGRESS) {
        response_status = status;
    }

    assert_response(scenario, group);
    assert_alive(scenario, group);
}

static void
get_chunk (MilterManagerTestScenario *scenario, const gchar *group,
           const gchar **chunk, gsize *chunk_size)
{
    *chunk = NULL;
    *chunk_size = 0;

    if (has_key(scenario, group, "chunk_file")) {
        const gchar *file_name;
        gchar *file_path;
        gchar *content;
        GError *error = NULL;

        file_name = get_string(scenario, group, "chunk_file");
        file_path = g_build_filename(scenario_dir, file_name, NULL);
        g_file_get_contents(file_path, &content, chunk_size, &error);
        if (content && content[*chunk_size - 1] == '\n')
            content[*chunk_size - 1] = '\0';
        g_free(file_path);
        gcut_assert_error(error);
        *chunk = cut_take_string(content);
    } else if (has_key(scenario, group, "chunk")) {
        *chunk = get_string(scenario, group, "chunk");
        *chunk_size = strlen(*chunk);
    }
}

static const GList *
get_chunks (MilterManagerTestScenario *scenario, const gchar *group)
{
    if (has_key(scenario, group, "chunk_files")) {
        const GList *file_names, *node, *taken_chunks;
        GList *chunks = NULL;
        GError *error = NULL;

        file_names = get_string_g_list(scenario, group, "chunk_files");
        for (node = file_names; node; node = g_list_next(node)) {
            const gchar *file_name = node->data;
            gchar *content = NULL;
            gsize content_size = 0;

            if (file_name) {
                gchar *file_path;

                file_path = g_build_filename(scenario_dir, file_name, NULL);
                g_file_get_contents(file_path, &content, &content_size, &error);
                g_free(file_path);
                if (error)
                    break;
            }
            if (content && content[content_size - 1] == '\n')
                content[content_size - 1] = '\0';
            chunks = g_list_append(chunks, content);
        }
        taken_chunks = gcut_take_list(chunks, g_free);
        gcut_assert_error(error);
        return taken_chunks;
    } else {
        return get_string_g_list(scenario, group, "chunks");
    }
}

static void
do_body (MilterManagerTestScenario *scenario, const gchar *group)
{
    const gchar *chunk = NULL;
    gsize chunk_size = 0;
    const GList *expected_chunks, *actual_chunks;
    MilterStatus status;

    get_chunk(scenario, group, &chunk, &chunk_size);
    milter_server_context_body(MILTER_SERVER_CONTEXT(server),
                               chunk, chunk_size);
    n_body_responses = 0;
    status = milter_manager_leader_body(leader, chunk, chunk_size);
    if (status != MILTER_STATUS_PROGRESS) {
        response_status = status;
    }

    assert_response(scenario, group);
    assert_alive(scenario, group);

    expected_chunks = get_chunks(scenario, group);
    actual_chunks = collect_received_strings(body_chunk);
    cut_set_message("[%s]", group);
    gcut_assert_equal_list_string(expected_chunks, actual_chunks);
}

static void
do_end_of_message (MilterManagerTestScenario *scenario, const gchar *group)
{
    const gchar *chunk = NULL;
    gsize chunk_size = 0;
    const GList *expected_chunks, *actual_chunks;
    MilterStatus status;

    get_chunk(scenario, group, &chunk, &chunk_size);

    milter_server_context_end_of_message(MILTER_SERVER_CONTEXT(server),
                                         chunk, chunk_size);
    milter_client_context_set_state(client_context,
                                    MILTER_CLIENT_CONTEXT_STATE_END_OF_MESSAGE);
    status = milter_manager_leader_end_of_message(leader, chunk, chunk_size);
    if (status != MILTER_STATUS_PROGRESS) {
        response_status = status;
    }

    assert_response(scenario, group);
    assert_alive(scenario, group);

    expected_chunks = get_chunks(scenario, group);
    actual_chunks = collect_received_strings(body_chunk);
    cut_set_message("[%s]", group);
    gcut_assert_equal_list_string(expected_chunks, actual_chunks);

    expected_chunks = get_string_g_list(scenario, group,
                                        "end_of_message_chunks");
    actual_chunks = collect_received_strings(end_of_message_chunk);
    cut_set_message("[%s]", group);
    gcut_assert_equal_list_string(expected_chunks, actual_chunks);
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
do_end_of_message_quarantine (MilterManagerTestScenario *scenario, const gchar *group)
{
    const GList *expected_reasons;
    const GList *actual_reasons;

    expected_reasons = get_string_g_list(scenario, group, "quarantine_reasons");
    actual_reasons =
        milter_manager_test_server_get_received_quarantine_reasons(server);

    gcut_assert_equal_list_string(sort_const_string_list(expected_reasons),
                                  sort_const_string_list(actual_reasons));
}

static void
do_end_of_message_header (MilterManagerTestScenario *scenario,
                          const gchar *group)
{
    const GList *expected_headers;
    const GList *actual_header_list;
    MilterHeaders *actual_headers;

    expected_headers = get_header_list(scenario, group, "headers");

    actual_headers = milter_manager_test_server_get_headers(server);
    cut_assert(actual_headers);
    actual_header_list = milter_headers_get_list(actual_headers);

    cut_set_message("[%s]", group);
    gcut_assert_equal_list(
        expected_headers, actual_header_list,
        (GEqualFunc)milter_header_equal,
        (GCutInspectFunction)milter_header_inspect,
        NULL);
}

static void
do_end_of_message_change_from (MilterManagerTestScenario *scenario, const gchar *group)
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
do_end_of_message_add_recipient (MilterManagerTestScenario *scenario, const gchar *group)
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
do_end_of_message_delete_recipient (MilterManagerTestScenario *scenario, const gchar *group)
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
do_end_of_message_replace_body (MilterManagerTestScenario *scenario, const gchar *group)
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
do_end_of_message_progress (MilterManagerTestScenario *scenario, const gchar *group)
{
    const gchar key[] = "n_progresses";
    GError *error = NULL;
    guint expected_n_progresses = 0;

    if (has_key(scenario, group, key)) {
        expected_n_progresses = get_integer(scenario, group, key);
    }
    gcut_assert_error(error);
    cut_assert_equal_uint(expected_n_progresses,
                          milter_manager_test_server_get_n_progresses(server));
}

static void
do_end_of_message_full (MilterManagerTestScenario *scenario, const gchar *group)
{
    cut_trace(do_end_of_message(scenario, group));
    cut_trace(milter_manager_test_wait_signal(loop, FALSE));
    cut_trace(do_end_of_message_quarantine(scenario, group));
    cut_trace(do_end_of_message_header(scenario, group));
    cut_trace(do_end_of_message_change_from(scenario, group));
    cut_trace(do_end_of_message_add_recipient(scenario, group));
    cut_trace(do_end_of_message_delete_recipient(scenario, group));
    cut_trace(do_end_of_message_replace_body(scenario, group));
    cut_trace(do_end_of_message_progress(scenario, group));
}

static void
do_quit (MilterManagerTestScenario *scenario, const gchar *group)
{
    milter_manager_leader_quit(leader);
    assert_response(scenario, group);
}

static void
do_abort (MilterManagerTestScenario *scenario, const gchar *group)
{
    milter_manager_leader_abort(leader);
    assert_response(scenario, group);
}

static void
do_unknown (MilterManagerTestScenario *scenario, const gchar *group)
{
    const gchar *command;
    MilterStatus status;

    command = get_string(scenario, group, "unknown_command");

    status = milter_manager_leader_unknown(leader, command);
    if (status != MILTER_STATUS_PROGRESS) {
        response_status = status;
    }
    assert_response(scenario, group);

    gcut_assert_equal_list_string(get_string_g_list(scenario, group,
                                                    "unknown_commands"),
                                  collect_received_strings(unknown_command));
}

static void
do_action (MilterManagerTestScenario *scenario, const gchar *group)
{
    MilterCommand command;

    command = get_enum(scenario, group, "command", MILTER_TYPE_COMMAND);
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
    case MILTER_COMMAND_ENVELOPE_FROM:
        cut_trace(do_envelope_from(scenario, group));
        break;
    case MILTER_COMMAND_ENVELOPE_RECIPIENT:
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

    g_list_foreach((GList *)started_clients,
                   (GFunc)milter_manager_test_client_clear_data,
                   NULL);
    teardown_error();
}

static void
do_actions (MilterManagerTestScenario *scenario)
{
    const GList *imported_scenarios;
    gsize length, i;
    const gchar **actions;

    imported_scenarios =
        milter_manager_test_scenario_get_imported_scenarios(scenario);
    g_list_foreach((GList *)imported_scenarios, (GFunc)do_actions, NULL);

    actions = get_string_list(scenario, MILTER_MANAGER_TEST_SCENARIO_GROUP_NAME,
                              "actions", &length);
    for (i = 0; i < length; i++) {
        cut_trace(do_action(scenario, actions[i]));
        cut_assert_equal_string(
            NULL, milter_manager_test_server_get_last_error(server));
    }
}

static void
data_scenario_basic (void)
{
    cut_add_data("negotiate",
                 g_strdup("negotiate.txt"), g_free,
                 "connect",
                 g_strdup("connect.txt"), g_free,
                 "connect - macro",
                 g_strdup("connect-with-macro.txt"), g_free,
                 "helo",
                 g_strdup("helo.txt"), g_free,
                 "envelope-from",
                 g_strdup("envelope-from.txt"), g_free,
                 "envelope-from - macro",
                 g_strdup("envelope-from-with-macro.txt"), g_free,
                 "envelope-recipient",
                 g_strdup("envelope-recipient.txt"), g_free,
                 "data",
                 g_strdup("data.txt"), g_free,
                 "header - from",
                 g_strdup("header-from.txt"), g_free,
                 "header - from-and-mailer",
                 g_strdup("header-from-and-mailer.txt"), g_free,
                 "end-of-header",
                 g_strdup("end-of-header.txt"), g_free,
                 "body",
                 g_strdup("body.txt"), g_free,
                 "end-of-message",
                 g_strdup("end-of-message.txt"), g_free,
                 "quit",
                 g_strdup("quit.txt"), g_free,
                 "abort - on envelope-from",
                 g_strdup("abort-on-envelope-from.txt"), g_free,
                 "abort - on end-of-message",
                 g_strdup("abort-on-end-of-message.txt"), g_free,
                 "unknown",
                 g_strdup("unknown.txt"), g_free,
                 "reply-code",
                 g_strdup("reply-code.txt"), g_free);
}

static void
data_scenario_end_of_message_action (void)
{
    cut_add_data("quarantine",
                 g_strdup("quarantine.txt"), g_free,
                 "quarantine - header-change",
                 g_strdup("quarantine-header-change.txt"), g_free,
                 "quarantine - evaluation",
                 g_strdup("quarantine-evaluation.txt"), g_free,
                 NULL);

    cut_add_data("add-header",
                 g_strdup("add-header.txt"), g_free,
                 "add-header - with-leading-space",
                 g_strdup("add-header-with-leading-space.txt"), g_free,
                 "add-header - evaluation",
                 g_strdup("add-header-evaluation.txt"), g_free,
                 "add-header - evaluation - temporary failure",
                 g_strdup("add-header-evaluation-temporary-failure.txt"), g_free,
                 "add-header - evaluation - reject",
                 g_strdup("add-header-evaluation-reject.txt"), g_free,
                 "add-header - evaluation - discard",
                 g_strdup("add-header-evaluation-discard.txt"), g_free,
                 NULL);

    cut_add_data("insert-header",
                 g_strdup("insert-header.txt"), g_free,
                 "insert-header - with-leading-space",
                 g_strdup("insert-header-with-leading-space.txt"), g_free,
                 "insert-header - evaluation",
                 g_strdup("insert-header-evaluation.txt"), g_free,
                 NULL);

    cut_add_data("change-header",
                 g_strdup("change-header.txt"), g_free,
                 "change-header - with-leading-space",
                 g_strdup("change-header-with-leading-space.txt"), g_free,
                 "change-header - evaluation",
                 g_strdup("change-header-evaluation.txt"), g_free,
                 NULL);

    cut_add_data("delete-header",
                 g_strdup("delete-header.txt"), g_free,
                 "delete-header - evaluation",
                 g_strdup("delete-header-evaluation.txt"), g_free,
                 "delete-header - many",
                 g_strdup("delete-header-many.txt"), g_free,
                 NULL);

    cut_add_data("change-from",
                 g_strdup("change-from.txt"), g_free,
                 "change-from - evaluation",
                 g_strdup("change-from-evaluation.txt"), g_free,
                 NULL);

    cut_add_data("add-recipient",
                 g_strdup("add-recipient.txt"), g_free,
                 "add-recipient - evaluation",
                 g_strdup("add-recipient-evaluation.txt"), g_free,
                 NULL);

    cut_add_data("delete-recipient",
                 g_strdup("delete-recipient.txt"), g_free,
                 "delete-recipient - evaluation",
                 g_strdup("delete-recipient-evaluation.txt"), g_free,
                 NULL);

    cut_add_data("replace-body",
                 g_strdup("replace-body.txt"), g_free,
                 "replace-body - overwrite",
                 g_strdup("replace-body-overwrite.txt"), g_free,
                 "replace-body - evaluation",
                 g_strdup("replace-body-evaluation.txt"), g_free,
                 NULL);

    cut_add_data("progress", g_strdup("progress.txt"), g_free,
                 NULL);
}

static void
data_scenario_negotiate (void)
{
    cut_add_data("negotiate - no child",
                 g_strdup("negotiate-no-child.txt"), g_free,
                 "negotiate - no enabled child",
                 g_strdup("negotiate-no-enabled-child.txt"), g_free,
                 "negotiate - no available child",
                 g_strdup("negotiate-no-available-child.txt"), g_free,
                 "negotiate - no bind",
                 g_strdup("negotiate-no-bind.txt"), g_free,
                 "negotiate - fallback - discard",
                 g_strdup("negotiate-fallback-discard.txt"), g_free,
                 "negotiate - fallback - reject",
                 g_strdup("negotiate-fallback-reject.txt"), g_free,
                 "negotiate - fallback - temporary failure",
                 g_strdup("negotiate-fallback-temporary-failure.txt"), g_free,
                 NULL);
}

static void
data_scenario_connect (void)
{
    cut_add_data("connect - no child",
                 g_strdup("connect-no-child.txt"), g_free,
                 "connect - no enabled child",
                 g_strdup("connect-no-enabled-child.txt"), g_free,
                 "connect - no available child",
                 g_strdup("connect-no-available-child.txt"), g_free,
                 "connect - policy",
                 g_strdup("connect-policy.txt"), g_free,
                 NULL);
}

static void
data_scenario_helo (void)
{
    cut_add_data("helo - reject - evaluation",
                 g_strdup("helo-reject-evaluation.txt"), g_free,
                 "helo - temporary-failure - evaluation",
                 g_strdup("helo-temporary-failure-evaluation.txt"), g_free,
                 NULL);
}

static void
data_scenario_envelope_from (void)
{
    cut_add_data("envelope-from - reject",
                 g_strdup("envelope-from-reject.txt"), g_free,
                 "envelope-from - reject",
                 g_strdup("envelope-from-reject.txt"), g_free,
                 "envelope-from - discard",
                 g_strdup("envelope-from-discard.txt"), g_free,
                 "envelope-from - discard - evaluation",
                 g_strdup("envelope-from-discard-evaluation.txt"), g_free,
                 "envelope-from - reject & discard",
                 g_strdup("envelope-from-reject-and-discard.txt"), g_free,
                 "envelope-from - reject & accept",
                 g_strdup("envelope-from-reject-and-accept.txt"), g_free,
                 "envelope-from - reject - again",
                 g_strdup("envelope-from-reject-again.txt"), g_free,
                 "envelope-from - accept",
                 g_strdup("envelope-from-accept.txt"), g_free,
                 "envelope-from - accept all",
                 g_strdup("envelope-from-accept-all.txt"), g_free,
                 "envelope-from - stop",
                 g_strdup("envelope-from-stop.txt"), g_free,
                 "envelope-from - again",
                 g_strdup("envelope-from-again.txt"), g_free,
                 "envelope-from - discarded - again",
                 g_strdup("envelope-from-discarded-again.txt"), g_free);
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
        g_strdup("envelope-recipient--reject--temp-fail--discard.txt"),
        /* tar says 'file name is too long' :< */
        g_free,
        "envelope-recipient - reject & discard - simultaneously",
        g_strdup("envelope-recipient-reject-and-discard-simultaneously.txt"),
        g_free,
        "envelope-recipient - discard",
        g_strdup("envelope-recipient-discard.txt"), g_free,
        "envelope-recipient - temporary-failure",
        g_strdup("envelope-recipient-temporary-failure.txt"), g_free,
        "envelope-recipient - temporary-failure - all",
        g_strdup("envelope-recipient-temporary-failure-all.txt"), g_free,
        "envelope-recipient - stop",
        g_strdup("envelope-recipient-stop.txt"), g_free,
        "envelope-recipient - again",
        g_strdup("envelope-recipient-again.txt"), g_free,
        "envelope-recipient - temporary-failure - evaluation",
        g_strdup("envelope-recipient-temporary-failure-evaluation.txt"),
        g_free,
        "envelope-recipient - reject - evaluation",
        g_strdup("envelope-recipient-reject-evaluation.txt"),
        g_free,
        "envelope-recipient - reply-code 4xx - evaluation",
        g_strdup("envelope-recipient-reply-code-4xx-evaluation.txt"),
        g_free,
        "envelope-recipient - reply-code 5xx - evaluation",
        g_strdup("envelope-recipient-reply-code-5xx-evaluation.txt"),
        g_free,
        NULL);
}

static void
data_scenario_data (void)
{
    cut_add_data("data - no data flag on second client",
                 g_strdup("no-data-second-client.txt"), g_free,
                 "data - close without reply",
                 g_strdup("data-close-without-reply.txt"), g_free,
                 "data - reply code reject",
                 g_strdup("data-reply-code-reject.txt"), g_free,
                 NULL);
}

static void
data_scenario_header (void)
{
    cut_add_data("header - with-leading-space",
                 g_strdup("header-with-leading-space.txt"), g_free,
                 "header - with-leading-space - no space",
                 g_strdup("header-with-leading-space-no-space.txt"), g_free,
                 "header - temporary failure",
                 g_strdup("header-temporary-failure.txt"), g_free,
                 "header - reject",
                 g_strdup("header-reject.txt"), g_free,
                 "header - stop",
                 g_strdup("header-stop.txt"), g_free,
                 "header - stop - all",
                 g_strdup("header-stop-all.txt"), g_free,
                 "header - again",
                 g_strdup("header-again.txt"), g_free,
                 NULL);
}

static void
data_scenario_end_of_header (void)
{
    cut_add_data("end-of-header - again",
                 g_strdup("end-of-header-again.txt"), g_free,
                 NULL);
}

static void
data_scenario_body (void)
{
    cut_add_data("body - larger than milter chunk size",
                 g_strdup("body-larger-than-milter-chunk-size.txt"), g_free,
                 "body - no body flag on both client",
                 g_strdup("no-body-flag-on-both-client.txt"), g_free,
                 "body - skip",
                 g_strdup("body-skip.txt"), g_free,
                 "body - skip all",
                 g_strdup("body-skip-all.txt"), g_free,
                 "body - accept",
                 g_strdup("body-accept.txt"), g_free,
                 "body - again",
                 g_strdup("body-again.txt"), g_free,
                 NULL);
}

static void
data_scenario_end_of_message (void)
{
    cut_add_data("end-of-message - no body flag",
                 g_strdup("no-body-flag.txt"), g_free,
                 "end-of-message - no body flag on second client",
                 g_strdup("no-body-flag-on-second-client.txt"), g_free,
                 "end-of-message - chunk",
                 g_strdup("end-of-message-chunk.txt"), g_free,
                 "end-of-message - reject",
                 g_strdup("end-of-message-reject.txt"), g_free,
                 "end-of-message - discard",
                 g_strdup("end-of-message-discard.txt"), g_free,
                 "end-of-message - discard - again",
                 g_strdup("end-of-message-discard-again.txt"), g_free,
                 "end-of-message - temporary-failure",
                 g_strdup("end-of-message-temporary-failure.txt"), g_free,
                 "end-of-message - accept",
                 g_strdup("end-of-message-accept.txt"), g_free,
                 "end-of-message - replaced",
                 g_strdup("end-of-message-replaced.txt"), g_free,
                 "end-of-message - reply code - reject",
                 g_strdup("end-of-message-reply-code-reject.txt"), g_free,
                 "end-of-message - reply code - temporary failure",
                 g_strdup("end-of-message-reply-code-temporary-failure.txt"),
                 g_free,
                 "end-of-message - again",
                 g_strdup("end-of-message-again.txt"), g_free,
                 "end-of-message - without header",
                 g_strdup("end-of-message-without-header.txt"), g_free,
                 NULL);
}

static void
data_scenario_reply_code (void)
{
    cut_add_data("reply-code - invalid",
                 g_strdup("reply-code-invalid.txt"), g_free);
}

static void
data_scenario_applicable_condition (void)
{
    cut_add_data("applicable condition - Remote Network",
                 g_strdup("applicable-condition-remote-network.txt"), g_free);
}

void
data_scenario (void)
{
    data_scenario_basic();
    data_scenario_end_of_message_action();
    data_scenario_negotiate();
    data_scenario_connect();
    data_scenario_helo();
    data_scenario_envelope_from();
    data_scenario_envelope_recipient();
    data_scenario_data();
    data_scenario_header();
    data_scenario_end_of_header();
    data_scenario_body();
    data_scenario_end_of_message();
    data_scenario_reply_code();
    data_scenario_applicable_condition();
}

void
test_scenario (gconstpointer data)
{
    const gchar *scenario_name = data;
    const gchar omit_key[] = "omit";
    const gchar configuration_key[] = "configuration";

    main_scenario = milter_manager_test_scenario_new();
    cut_trace(milter_manager_test_scenario_load(main_scenario,
                                                scenario_dir,
                                                scenario_name));

    if (has_key(main_scenario,
                MILTER_MANAGER_TEST_SCENARIO_GROUP_NAME, omit_key))
        cut_omit("%s", get_string(main_scenario,
                                  MILTER_MANAGER_TEST_SCENARIO_GROUP_NAME,
                                  omit_key));

    if (has_key(main_scenario,
                MILTER_MANAGER_TEST_SCENARIO_GROUP_NAME, configuration_key)) {
        GError *error = NULL;
        const gchar *config_file;

        config_file =
            cut_build_path(scenario_dir,
                           get_string(main_scenario,
                                      MILTER_MANAGER_TEST_SCENARIO_GROUP_NAME,
                                      configuration_key),
                           NULL);
        milter_manager_configuration_load(config, config_file, &error);
        gcut_assert_error(error);
    }

    cut_trace(milter_manager_test_scenario_start_clients(main_scenario, loop));
    cut_trace(do_actions(main_scenario));
}

/* over 512 kbytes data causes hang up..? */
#define LARGE_DATA_SIZE 112 * 1024
void
data_large_body (void)
{
    gchar *chunk = g_new(gchar, LARGE_DATA_SIZE);
    memset(chunk, 'B', LARGE_DATA_SIZE);
    cut_add_data("large body", chunk, g_free);
}

void
test_large_body (gconstpointer data)
{
    const gchar *chunk = data;
    MilterStatus status;

    cut_trace(test_scenario("end-of-header.txt"));

    milter_server_context_set_state(MILTER_SERVER_CONTEXT(server),
                                    MILTER_SERVER_CONTEXT_STATE_BODY);
    status = milter_manager_leader_body(leader, chunk, LARGE_DATA_SIZE);
    if (status != MILTER_STATUS_PROGRESS) {
        response_status = status;
    }

    /*
    cut_assert_equal_memory(chunk, LARGE_DATA_SIZE,
                            collect_received_strings(body_chunk), LARGE_DATA_SIZE);
    */
}

void
test_configuration (void)
{
    gcut_assert_equal_object(config,
                             milter_manager_leader_get_configuration(leader));
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
