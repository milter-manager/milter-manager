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
#include <milter/manager/milter-manager-leader.h>
#include <milter-manager-test-server.h>
#include <milter-manager-test-scenario.h>
#undef shutdown

void data_scenario (void);
void test_scenario (gconstpointer data);

static gchar *scenario_dir;
static MilterManagerTestScenario *main_scenario;

static MilterManagerConfiguration *config;
static MilterClientContext *client_context;
static MilterManagerLeader *leader;
static MilterManagerTestServer *server;

static GError *actual_error;
static GError *expected_error;
static gboolean finished;

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

void
setup (void)
{
    GIOChannel *channel;
    MilterWriter *writer;
    MilterReader *reader;

    scenario_dir = g_build_filename(milter_manager_test_get_base_dir(),
                                    "fixtures",
                                    "leader",
                                    NULL);

    main_scenario = NULL;

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
    milter_agent_set_writer(MILTER_AGENT(client_context), writer);
    g_object_unref(writer);

    server = milter_manager_test_server_new();
    milter_agent_set_reader(MILTER_AGENT(server), reader);
    g_object_unref(reader);

    leader = milter_manager_leader_new(config, client_context);
    actual_error = NULL;
    setup_leader_signals(leader);

    response_status = MILTER_STATUS_DEFAULT;

    finished = FALSE;

    expected_error = NULL;
}

void
teardown (void)
{
    if (config)
        g_object_unref(config);
    if (client_context)
        g_object_unref(client_context);
    if (leader)
        g_object_unref(leader);

    if (server)
        g_object_unref(server);

    if (scenario_dir)
        g_free(scenario_dir);
    if (main_scenario)
        g_object_unref(main_scenario);

    if (actual_error)
        g_error_free(actual_error);
    if (expected_error)
        g_error_free(expected_error);
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
    timeout_waiting_id = g_timeout_add(100, cb_timeout_waiting,
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

#define wait_reply_from_client(client, actual_getter)   \
    cut_trace_with_info_expression(                     \
        milter_manager_test_client_wait_reply(          \
            client, actual_getter),                     \
        wait_reply_from_client(client, actual_getter))


#define wait_finished()                    \
    cut_trace_with_info_expression(        \
        wait_finished_helper(),            \
        wait_finished())

static void
wait_finished_helper (void)
{
    gboolean timeout_waiting = TRUE;
    guint timeout_waiting_id;

    timeout_waiting_id = g_timeout_add(100, cb_timeout_waiting,
                                       &timeout_waiting);
    while (timeout_waiting && !finished) {
        g_main_context_iteration(NULL, TRUE);
    }
    g_source_remove(timeout_waiting_id);

    cut_assert_true(timeout_waiting, "timeout");
    cut_assert_true(finished);
}

#define wait_leader_error()                    \
    cut_trace_with_info_expression(                \
        wait_leader_error_helper(),            \
        wait_leader_error())

static void
wait_leader_error_helper (void)
{
    gboolean timeout_waiting = TRUE;
    guint timeout_waiting_id;

    timeout_waiting_id = g_timeout_add(100, cb_timeout_waiting,
                                       &timeout_waiting);
    while (timeout_waiting && !actual_error) {
        g_main_context_iteration(NULL, TRUE);
    }
    g_source_remove(timeout_waiting_id);

    cut_assert_true(timeout_waiting, "timeout");
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
    } else if (g_str_equal(response, "quit")) {
        return milter_manager_test_client_get_n_quit_received;
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
        cut_assert_operator_uint(0, <, type, "name: <%s>", type_name);
        code = get_enum(scenario, group, "error_code", type);

        message = get_string(scenario, group, "error_message");
        expected_error = g_error_new(domain, code, "%s", message);
        wait_leader_error();
    }

    gcut_assert_equal_error(expected_error, actual_error);
    if (expected_error && actual_error)
        cut_return();
}

static void
assert_response_common (MilterManagerTestScenario *scenario, const gchar *group)
{
    MilterManagerTestClientGetNReceived get_n_received;
    guint n_received, actual_n_received;
    const gchar *response;
    MilterStatus status;

    n_received = get_integer(scenario, group, "n_received");
    if (g_str_equal(group, "abort")) {
        const GList *node;
        for (node = started_clients; node; node = g_list_next(node)) {
            const gchar *name;

            MilterManagerTestClient *client = node->data;
            name = milter_manager_test_client_get_name(client);

            response = get_string_with_sub_key(scenario, group, "response", name);
            if (g_str_equal(response, "none")) {
                /* FIXME check somthing! */
            } else {
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
    } else if (g_str_equal(response, "abort")) {
        /* Do nothing */
    } else {
        const gchar *response_signal_name;

        response_signal_name = cut_take_printf("%s-response", response);
        cut_trace(assert_have_response_helper(response_signal_name,
                                              n_received == 0));
    }

    get_n_received = response_to_get_n_received(response);
    cut_assert_not_null(get_n_received, "response: <%s>(%s)", response, group);

    /* workaround */
    if (g_str_equal(response, "end-of-message") &&
        status == MILTER_STATUS_CONTINUE &&
        n_received == 2) {
        milter_manager_test_clients_wait_reply(started_clients, get_n_received);
    }

    actual_n_received =
        milter_manager_test_clients_collect_n_received(started_clients,
                                                       get_n_received);
    cut_assert_equal_uint(n_received, actual_n_received, "[%s]", group);
    gcut_assert_equal_enum(MILTER_TYPE_STATUS, status, response_status,
                           "[%s]", group);

}

static void
assert_response_reply_code (MilterManagerTestScenario *scenario, const gchar *group)
{
    const GList *replied_codes;

    replied_codes = get_string_g_list(scenario, group, "reply_codes");
    cut_trace(milter_manager_test_wait_signal(FALSE));
    gcut_assert_equal_list_string(
        replied_codes,
        milter_manager_test_server_get_received_reply_codes(server));
}

static void
assert_response (MilterManagerTestScenario *scenario, const gchar *group)
{
    cut_trace(assert_response_error(scenario, group));
    cut_trace(assert_response_common(scenario, group));
    cut_trace(assert_response_reply_code(scenario, group));
}

static void
do_negotiate (MilterManagerTestScenario *scenario, const gchar *group)
{
    MilterOption *option;

    option = milter_manager_test_scenario_get_option(scenario, group);
    gcut_take_object(G_OBJECT(option));
    milter_server_context_negotiate(MILTER_SERVER_CONTEXT(server), option);
    milter_manager_leader_negotiate(leader, option);
    cut_trace(assert_response(scenario, group));

    gcut_assert_equal_list_object_custom(
        get_option_list(scenario, group, "options"),
        milter_manager_test_clients_collect_negotiate_options(started_clients),
        (GEqualFunc)milter_option_equal);
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

    host = get_string(scenario, group, "host");
    address_spec = get_string(scenario, group, "address");

    milter_connection_parse_spec(address_spec,
                                 &domain, &address, &address_size,
                                 &error);
    gcut_assert_error(error);

    milter_manager_leader_connect(leader, host, address, address_size);
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
do_helo (MilterManagerTestScenario *scenario, const gchar *group)
{
    const gchar *fqdn;
    const GList *expected_fqdns;

    fqdn = get_string(scenario, group, "fqdn");
    milter_manager_leader_helo(leader, fqdn);

    cut_trace(assert_response(scenario, group));

    expected_fqdns = get_string_g_list(scenario, group, "fqdns");
    gcut_assert_equal_list_string(expected_fqdns,
                                  collect_received_strings(helo_fqdn),
                                  "[%s]", group);
}

static void
do_envelope_from (MilterManagerTestScenario *scenario, const gchar *group)
{
    const gchar *from;
    const GList *expected_froms;

    from = get_string(scenario, group, "from");
    milter_manager_leader_envelope_from(leader, from);

    cut_trace(assert_response(scenario, group));

    expected_froms = get_string_g_list(scenario, group, "froms");
    gcut_assert_equal_list_string(expected_froms,
                                  collect_received_strings(envelope_from),
                                  "[%s]", group);
}

static void
do_envelope_recipient (MilterManagerTestScenario *scenario, const gchar *group)
{
    const gchar *recipient;
    const GList *expected_recipients;

    recipient = get_string(scenario, group, "recipient");
    milter_manager_leader_envelope_recipient(leader, recipient);

    cut_trace(assert_response(scenario, group));

    expected_recipients = get_string_g_list(scenario, group, "recipients");
    gcut_assert_equal_list_string(expected_recipients,
                                  collect_received_strings(envelope_recipient),
                                  "[%s]", group);
}

static void
do_data (MilterManagerTestScenario *scenario, const gchar *group)
{
    milter_manager_leader_data(leader);

    cut_trace(assert_response(scenario, group));
}

static void
do_header (MilterManagerTestScenario *scenario, const gchar *group)
{
    const GList *expected_headers;
    const GList *actual_headers;
    const gchar *name, *value;

    name = get_string(scenario, group, "name");
    value = get_string(scenario, group, "value");
    milter_manager_leader_header(leader, name, value);

    milter_manager_test_server_add_header(server, name, value);

    cut_trace(assert_response(scenario, group));

    expected_headers = get_pair_list(scenario, group, "headers");
    actual_headers = 
        g_list_sort((GList*)collect_received_pairs(header_name, header_value),
                     milter_manager_test_pair_compare);
    gcut_assert_equal_list(
        expected_headers, actual_headers,
        milter_manager_test_pair_equal,
        (GCutInspectFunc)milter_manager_test_pair_inspect,
        NULL);
}

static void
do_end_of_header (MilterManagerTestScenario *scenario, const gchar *group)
{
    milter_manager_leader_end_of_header(leader);

    cut_trace(assert_response(scenario, group));
}

static void
do_body (MilterManagerTestScenario *scenario, const gchar *group)
{
    const gchar *chunk;

    chunk = get_string(scenario, group, "chunk");
    milter_manager_leader_body(leader, chunk, strlen(chunk));

    cut_trace(assert_response(scenario, group));

    gcut_assert_equal_list_string(get_string_g_list(scenario, group, "chunks"),
                                  collect_received_strings(body_chunk),
                                  "<%s>", group);
}

static void
do_end_of_message (MilterManagerTestScenario *scenario, const gchar *group)
{
    const gchar *chunk = NULL;

    if (has_key(scenario, group, "chunk"))
        chunk = get_string(scenario, group, "chunk");

    milter_server_context_set_state(MILTER_SERVER_CONTEXT(server),
                                    MILTER_SERVER_CONTEXT_STATE_END_OF_MESSAGE);
    milter_manager_leader_end_of_message(leader,
                                         chunk,
                                         chunk ? strlen(chunk) : 0);

    cut_trace(assert_response(scenario, group));

    gcut_assert_equal_list_string(
        get_string_g_list(scenario, group, "chunks"),
        collect_received_strings(body_chunk));
    gcut_assert_equal_list_string(
        get_string_g_list(scenario, group, "end_of_message_chunks"),
        collect_received_strings(end_of_message_chunk));
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
do_end_of_message_header (MilterManagerTestScenario *scenario, const gchar *group)
{
    const GList *expected_headers;
    const GList *actual_header_list;
    MilterManagerHeaders *actual_headers;

    expected_headers = get_header_list(scenario, group, "headers");

    actual_headers = milter_manager_test_server_get_headers(server);
    cut_assert(actual_headers);
    actual_header_list = milter_manager_headers_get_list(actual_headers);

    gcut_assert_equal_list(
        expected_headers, actual_header_list,
        (GEqualFunc)milter_manager_header_equal,
        (GCutInspectFunc)milter_manager_header_inspect,
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
    cut_trace(milter_manager_test_wait_signal(FALSE));
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
    cut_trace(assert_response(scenario, group));
}

static void
do_abort (MilterManagerTestScenario *scenario, const gchar *group)
{
    milter_manager_leader_abort(leader);
    cut_trace(assert_response(scenario, group));
}

static void
do_unknown (MilterManagerTestScenario *scenario, const gchar *group)
{
    const gchar *command;

    command = get_string(scenario, group, "unknown_command");

    milter_manager_leader_unknown(leader, command);
    cut_trace(assert_response(scenario, group));

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

    g_list_foreach((GList *)started_clients,
                   (GFunc)milter_manager_test_client_clear_data,
                   NULL);
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
        "header - from-and-mailer", g_strdup("header-from-and-mailer.txt"), g_free,
        "end-of-header", g_strdup("end-of-header.txt"), g_free,
        "body", g_strdup("body.txt"), g_free,
        "end-of-message", g_strdup("end-of-message.txt"), g_free,
        "end-of-message - chunk", g_strdup("end-of-message-chunk.txt"), g_free,
        "end-of-message - reject", g_strdup("end-of-message-reject.txt"), g_free,
        "end-of-message - discard", g_strdup("end-of-message-discard.txt"), g_free,
        "end-of-message - temporary-failure", g_strdup("end-of-message-temporary-failure.txt"), g_free,
        "end-of-message - accept", g_strdup("end-of-message-accept.txt"), g_free,
        "end-of-message - replaced", g_strdup("end-of-message-replaced.txt"), g_free,
        "quit", g_strdup("quit.txt"), g_free,
        "abort - on envelope-from", g_strdup("abort-on-envelope-from.txt"), g_free,
        "abort - on end-of-message", g_strdup("abort-on-end-of-message.txt"), g_free,
        "unknown", g_strdup("unknown.txt"), g_free,
        "reply-code", g_strdup("reply-code.txt"), g_free);
}

static void
data_scenario_end_of_message_action (void)
{
    cut_add_data("quarantine", g_strdup("quarantine.txt"), g_free,
                 "add-header", g_strdup("add-header.txt"), g_free,
                 "insert-header", g_strdup("insert-header.txt"), g_free,
                 "change-header", g_strdup("change-header.txt"), g_free,
                 "change-from", g_strdup("change-from.txt"), g_free,
                 "add-recipient", g_strdup("add-recipient.txt"), g_free,
                 "delete-recipient", g_strdup("delete-recipient.txt"), g_free,
                 "replace-body", g_strdup("replace-body.txt"), g_free,
                 "progress", g_strdup("progress.txt"), g_free);

    cut_add_data("replace-body - overwrite",
                 g_strdup("replace-body-overwrite.txt"), g_free);
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

    main_scenario = milter_manager_test_scenario_new();
    cut_trace(milter_manager_test_scenario_load(main_scenario,
                                                scenario_dir,
                                                scenario_name));

    if (has_key(main_scenario,
                MILTER_MANAGER_TEST_SCENARIO_GROUP_NAME, omit_key))
        cut_omit("%s", get_string(main_scenario,
                                  MILTER_MANAGER_TEST_SCENARIO_GROUP_NAME,
                                  omit_key));

    cut_trace(milter_manager_test_scenario_start_clients(main_scenario));
    cut_trace(do_actions(main_scenario));
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
