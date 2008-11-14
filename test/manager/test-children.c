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

#include <gcutter.h>

#include <string.h>

#define shutdown inet_shutdown
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#include <milter-manager-test-utils.h>
#include <milter-manager-test-client.h>
#include <milter-manager-test-scenario.h>
#include <milter/manager/milter-manager-children.h>
#include <milter/manager/milter-manager-enum-types.h>
#undef shutdown

void test_foreach (void);
void data_scenario (void);
void test_scenario (gconstpointer data);
void test_negotiate (void);
void test_connect (void);
void test_connect_pass (void);
void test_connect_half_pass (void);
void test_helo (void);
void test_envelope_from (void);
void test_envelope_recipient (void);
void test_data (void);
void test_header (void);
void test_end_of_header (void);
void data_important_status (void);
void test_important_status (gconstpointer data);
void data_not_important_status (void);
void test_not_important_status (gconstpointer data);
void test_reading_timeout (void);
void test_connection_timeout (void);
void test_end_of_message_timeout (void);
void test_writing_timeout (void);

static gchar *scenario_dir;
static MilterManagerTestScenario *main_scenario;

static MilterManagerConfiguration *config;
static MilterManagerChildren *children;
static MilterOption *option;

static GError *actual_error;
static GError *expected_error;
static GList *actual_names;
static GList *expected_names;
static gchar *error_message;

static guint n_negotiate_reply_emitted;
static guint n_continue_emitted;
static guint n_reply_code_emitted;
static guint n_temporary_failure_emitted;
static guint n_reject_emitted;
static guint n_accept_emitted;
static guint n_discard_emitted;
static guint n_add_header_emitted;
static guint n_insert_header_emitted;
static guint n_change_header_emitted;
static guint n_change_from_emitted;
static guint n_add_recipient_emitted;
static guint n_delete_recipient_emitted;
static guint n_replace_body_emitted;
static guint n_progress_emitted;
static guint n_quarantine_emitted;
static guint n_connection_failure_emitted;
static guint n_shutdown_emitted;
static guint n_skip_emitted;
static guint n_error_emitted;
static guint n_finished_emitted;
static guint n_reading_timeout_emitted;
static guint n_end_of_message_timeout_emitted;

static guint log_signal_id;

static GArray *arguments1;
static GArray *arguments2;

static GList *test_clients;

static MilterOption *actual_option;

void
startup (void)
{
    MILTER_TYPE_MANAGER_CHILD_ERROR;
}

static void
cb_negotiate_reply (MilterManagerChildren *children,
                    MilterOption *option, MilterMacrosRequests *macros_requests,
                    gpointer user_data)
{
    n_negotiate_reply_emitted++;

    if (actual_option)
        g_object_unref(actual_option);
    actual_option = g_object_ref(option);
    /* FIXME: collect macros request */
}

static void
cb_continue (MilterManagerChildren *children, gpointer user_data)
{
    n_continue_emitted++;
}

static void
cb_reply_code (MilterManagerChildren *children, const gchar *code,
               gpointer user_data)
{
    n_reply_code_emitted++;
}

static void
cb_temporary_failure (MilterManagerChildren *children, gpointer user_data)
{
    n_temporary_failure_emitted++;
}

static void
cb_reject (MilterManagerChildren *children, gpointer user_data)
{
    n_reject_emitted++;
}

static void
cb_accept (MilterManagerChildren *children, gpointer user_data)
{
    n_accept_emitted++;
}

static void
cb_discard (MilterManagerChildren *children, gpointer user_data)
{
    n_discard_emitted++;
}

static void
cb_add_header (MilterManagerChildren *children,
               const gchar *name, const gchar *value,
               gpointer user_data)
{
    n_add_header_emitted++;
}

static void
cb_insert_header (MilterManagerChildren *children,
                  guint32 index, const gchar *name, const gchar *value,
                  gpointer user_data)
{
    n_insert_header_emitted++;
}

static void
cb_change_header (MilterManagerChildren *children,
                  const gchar *name, guint32 index, const gchar *value,
                  gpointer user_data)
{
    n_change_header_emitted++;
}

static void
cb_change_from (MilterManagerChildren *children,
                const gchar *from, const gchar *parameters,
                gpointer user_data)
{
    n_change_from_emitted++;
}

static void
cb_add_recipient (MilterManagerChildren *children,
                  const gchar *recipient, const gchar *parameters,
                  gpointer user_data)
{
    n_add_recipient_emitted++;
}

static void
cb_delete_recipient (MilterManagerChildren *children, const gchar *recipient,
                     gpointer user_data)
{
    n_delete_recipient_emitted++;
}

static void
cb_replace_body (MilterManagerChildren *children,
                 const gchar *body, gsize body_size,
                 gpointer user_data)
{
    n_replace_body_emitted++;
}

static void
cb_progress (MilterManagerChildren *children, gpointer user_data)
{
    n_progress_emitted++;
}

static void
cb_quarantine (MilterManagerChildren *children, const gchar *reason,
               gpointer user_data)
{
    n_quarantine_emitted++;
}

static void
cb_connection_failure (MilterManagerChildren *children, gpointer user_data)
{
    n_connection_failure_emitted++;
}

static void
cb_shutdown (MilterManagerChildren *children, gpointer user_data)
{
    n_shutdown_emitted++;
}

static void
cb_skip (MilterManagerChildren *children, gpointer user_data)
{
    n_skip_emitted++;
}

static void
cb_error (MilterManagerChildren *children, GError *error, gpointer user_data)
{
    n_error_emitted++;
    if (actual_error)
        g_clear_error(&actual_error);
    actual_error = g_error_copy(error);
}

static void
cb_finished (MilterManagerChildren *children, gpointer user_data)
{
    n_finished_emitted++;
}

static void
setup_signals (MilterManagerChildren *children)
{
#define CONNECT(name)                                                   \
    g_signal_connect(children, #name, G_CALLBACK(cb_ ## name), NULL)

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
    CONNECT(finished);

#undef CONNECT
}

static void
start_client (guint port, GArray *arguments)
{
    MilterManagerTestClient *client;
    GError *error = NULL;

    client = milter_manager_test_client_new(port, NULL);
    milter_manager_test_client_set_arguments(client, arguments);
    test_clients = g_list_append(test_clients, client);
    if (!milter_manager_test_client_run(client, &error)) {
        gcut_assert_error(error);
        cut_fail("couldn't run test client on port <%u>", port);
    }
}

static void
clear_n_emitted (void)
{
    n_negotiate_reply_emitted = 0;
    n_continue_emitted = 0;
    n_reply_code_emitted = 0;
    n_temporary_failure_emitted = 0;
    n_reject_emitted = 0;
    n_accept_emitted = 0;
    n_discard_emitted = 0;
    n_add_header_emitted = 0;
    n_insert_header_emitted = 0;
    n_change_header_emitted = 0;
    n_change_from_emitted = 0;
    n_add_recipient_emitted = 0;
    n_delete_recipient_emitted = 0;
    n_replace_body_emitted = 0;
    n_progress_emitted = 0;
    n_quarantine_emitted = 0;
    n_connection_failure_emitted = 0;
    n_shutdown_emitted = 0;
    n_skip_emitted = 0;
    n_error_emitted = 0;
    n_finished_emitted = 0;

    n_reading_timeout_emitted = 0;
    n_end_of_message_timeout_emitted = 0;
}

void
setup (void)
{
    scenario_dir = g_build_filename(milter_manager_test_get_base_dir(),
                                    "fixtures",
                                    "children",
                                    NULL);
    main_scenario = NULL;

    config = milter_manager_configuration_new(NULL);
    children = milter_manager_children_new(config);
    setup_signals(children);

    option = NULL;

    actual_error = NULL;
    expected_error = NULL;

    actual_names = NULL;
    expected_names = NULL;

    error_message = NULL;

    clear_n_emitted();

    log_signal_id = 0;

    arguments1 = g_array_new(TRUE, TRUE, sizeof(gchar *));
    arguments2 = g_array_new(TRUE, TRUE, sizeof(gchar *));

    test_clients = NULL;

    actual_option = NULL;
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
    if (config)
        g_object_unref(config);
    if (children)
        g_object_unref(children);

    if (main_scenario)
        g_object_unref(main_scenario);

    if (option)
        g_object_unref(option);

    if (actual_error)
        g_error_free(actual_error);
    if (expected_error)
        g_error_free(expected_error);

    if (error_message)
        g_free(error_message);

    if (actual_names)
        gcut_list_string_free(actual_names);
    if (expected_names)
        gcut_list_string_free(expected_names);

    if (arguments1)
        arguments_free(arguments1);
    if (arguments2)
        arguments_free(arguments2);

    if (test_clients) {
        g_list_foreach(test_clients, (GFunc)g_object_unref, NULL);
        g_list_free(test_clients);
    }

    if (log_signal_id)
        g_signal_handler_disconnect(milter_logger(), log_signal_id);

    if (actual_option)
        g_object_unref(actual_option);
}

static void
collect_child_name (gpointer data, gpointer user_data)
{
    MilterManagerChild *child = data;
    gchar *name = NULL;

    g_object_get(child,
                 "name", &name,
                 NULL);

    actual_names = g_list_append(actual_names, name);
}

void
test_foreach (void)
{
    expected_names = gcut_list_string_new("1", "2", "3", NULL);

#define ADD_CHILD(name) do {                                    \
        MilterManagerChild *child;                              \
                                                                \
        child = milter_manager_child_new(name);                 \
        milter_manager_children_add_child(children, child);     \
    } while (0)

    ADD_CHILD("1");
    ADD_CHILD("2");
    ADD_CHILD("3");

    milter_manager_children_foreach(children, collect_child_name, NULL);

    gcut_assert_equal_list_string(expected_names, actual_names);
}

static gboolean
cb_timeout_waiting (gpointer data)
{
    gboolean *waiting = data;

    *waiting = FALSE;
    return FALSE;
}

#define wait_reply(expected, actual)            \
    cut_trace_with_info_expression(             \
        wait_reply_helper(expected, &actual),   \
        wait_reply(actual))

static void
wait_reply_helper (guint expected, guint *actual)
{
    gboolean timeout_waiting = TRUE;
    guint timeout_waiting_id;

    timeout_waiting_id = g_timeout_add(500, cb_timeout_waiting,
                                       &timeout_waiting);
    while (timeout_waiting && expected > *actual) {
        g_main_context_iteration(NULL, TRUE);
    }
    g_source_remove(timeout_waiting_id);

    cut_assert_true(timeout_waiting, "timeout");
    cut_assert_equal_uint(expected, *actual);
}

static MilterManagerEgg *
egg_new (const gchar *name, const gchar *connection_spec)
{
    MilterManagerEgg *egg;
    GError *error = NULL;

    egg = milter_manager_egg_new(name);
    if (!milter_manager_egg_set_connection_spec(egg,
                                                connection_spec,
                                                &error)) {
        g_object_unref(egg);
        egg = NULL;
        gcut_assert_error(error);
    }

    return egg;
}


static void
add_child (const gchar *name, const gchar *connection_spec)
{
    MilterManagerEgg *egg;
    MilterManagerChild *child;

    egg = egg_new(name, connection_spec);
    cut_assert_not_null(egg);

    child = milter_manager_egg_hatch(egg);
    milter_manager_children_add_child(children, child);
    g_object_unref(egg);
    g_object_unref(child);
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
    guint n_finished_emitted_before = n_finished_emitted;

    timeout_waiting_id = g_timeout_add(500, cb_timeout_waiting,
                                       &timeout_waiting);
    while (timeout_waiting && n_finished_emitted == n_finished_emitted_before) {
        g_main_context_iteration(NULL, TRUE);
    }
    g_source_remove(timeout_waiting_id);

    cut_assert_true(timeout_waiting, "timeout");
    cut_assert_operator_uint(n_finished_emitted, > , n_finished_emitted_before);
}

#define wait_children_error()                      \
    cut_trace_with_info_expression(                \
        wait_children_error_helper(),              \
        wait_children_error())

static void
wait_children_error_helper (void)
{
    gboolean timeout_waiting = TRUE;
    guint timeout_waiting_id;

    timeout_waiting_id = g_timeout_add(500, cb_timeout_waiting,
                                       &timeout_waiting);
    while (timeout_waiting && !actual_error) {
        g_main_context_iteration(NULL, TRUE);
    }
    g_source_remove(timeout_waiting_id);

    cut_assert_true(timeout_waiting, "timeout");
    cut_assert_not_null(actual_error);
}

#define started_clients                                                 \
    milter_manager_test_scenario_get_started_clients(main_scenario)

#define has_group(scenario, group)                                \
    milter_manager_test_scenario_has_group(scenario, group)

#define has_key(scenario, group, key)                           \
    milter_manager_test_scenario_has_key(scenario, group, key)

#define get_integer(scenario, group, key)                               \
    milter_manager_test_scenario_get_integer(scenario, group, key)

#define get_double(scenario, group, key)                                \
    milter_manager_test_scenario_get_double(scenario, group, key)

#define get_boolean(scenario, group, key)                               \
    milter_manager_test_scenario_get_boolean(scenario, group, key)

#define get_string(scenario, group, key)                                \
    milter_manager_test_scenario_get_string(scenario, group, key)

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
setup_parameters (MilterManagerTestScenario *scenario,
                  MilterManagerChildren *children,
                  MilterManagerConfiguration *config)
{
    const gchar group[] = "config";
    const gchar privilege_mode_key[] = "privilege_mode";
    const gchar retry_connect_time_key[] = "retry_connect_time";

    if (!has_group(scenario, group))
        return;

    if (has_key(scenario, group, privilege_mode_key))
        g_object_set(config,
                     "privilege-mode", get_boolean(scenario,
                                                   group,
                                                   privilege_mode_key),
                     NULL);

    if (has_key(scenario, group, retry_connect_time_key))
        milter_manager_children_set_retry_connect_time(
            children,
            get_double(scenario, group, retry_connect_time_key));
}

static guint
get_n_emitted (const gchar *response)
{
    if (g_str_equal(response, "negotiate-reply")) {
        return n_negotiate_reply_emitted;
    } else if (g_str_equal(response, "continue")) {
        return n_continue_emitted;
    } else if (g_str_equal(response, "rely-code")) {
        return n_reply_code_emitted;
    } else if (g_str_equal(response, "temporary-failure")) {
        return n_temporary_failure_emitted;
    } else if (g_str_equal(response, "reject")) {
        return n_reject_emitted;
    } else if (g_str_equal(response, "accept")) {
        return n_accept_emitted;
    } else if (g_str_equal(response, "discard")) {
        return n_discard_emitted;
    } else if (g_str_equal(response, "add-header")) {
        return n_add_header_emitted;
    } else if (g_str_equal(response, "insert-header")) {
        return n_insert_header_emitted;
    } else if (g_str_equal(response, "change-header")) {
        return n_change_header_emitted;
    } else if (g_str_equal(response, "change-from")) {
        return n_change_from_emitted;
    } else if (g_str_equal(response, "add-recipient")) {
        return n_add_recipient_emitted;
    } else if (g_str_equal(response, "delete-recipient")) {
        return n_delete_recipient_emitted;
    } else if (g_str_equal(response, "replace-body")) {
        return n_replace_body_emitted;
    } else if (g_str_equal(response, "progress")) {
        return n_progress_emitted;
    } else if (g_str_equal(response, "quarantine")) {
        return n_quarantine_emitted;
    } else if (g_str_equal(response, "connection-failure")) {
        return n_connection_failure_emitted;
    } else if (g_str_equal(response, "shutdown")) {
        return n_shutdown_emitted;
    } else if (g_str_equal(response, "skip")) {
        return n_skip_emitted;
    } else if (g_str_equal(response, "error")) {
        return n_error_emitted;
    } else if (g_str_equal(response, "finished")) {
        return n_finished_emitted;
    } else {
        cut_error("unknown response: <%s>", response);
    }

    return 0;
}

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
        wait_children_error();
    }

    gcut_assert_equal_error(expected_error, actual_error);
    if (expected_error && actual_error)
        cut_return();
}

static void
assert_response_common (MilterManagerTestScenario *scenario, const gchar *group)
{
    guint n_emitted, actual_n_emitted;
    const gchar *response;

    n_emitted = get_integer(scenario, group, "n_emitted");
    response = get_string(scenario, group, "response");

    if (g_str_equal(response, "quit")) {
        wait_finished();
/*     } else if (g_str_equal(response, "abort")) { */
/*         wait_reply(abort); */
    } else {
        g_usleep(500); /* FIXME */
        milter_manager_test_wait_signal(n_emitted == 0);
    }

    actual_n_emitted = get_n_emitted(response);
    cut_assert_equal_uint(n_emitted, actual_n_emitted, "[%s]", group);
}

static void
assert_response (MilterManagerTestScenario *scenario, const gchar *group)
{
    cut_trace(assert_response_error(scenario, group));
    cut_trace(assert_response_common(scenario, group));
}

static void
do_negotiate (MilterManagerTestScenario *scenario, const gchar *group)
{
    MilterOption *option;

    option = milter_manager_test_scenario_get_option(scenario, group);
    gcut_take_object(G_OBJECT(option));
    milter_manager_children_negotiate(children, option);
    wait_reply(1, n_negotiate_reply_emitted);
    cut_trace(assert_response(scenario, group));

    gcut_assert_equal_object_custom(option, actual_option,
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

    host = get_string(scenario, group, "host");
    address_spec = get_string(scenario, group, "address");

    milter_utils_parse_connection_spec(address_spec,
                                       &domain,
                                       &address,
                                       &address_size,
                                       &error);
    gcut_assert_error(error);

    milter_manager_children_connect(children, host, address, address_size);
    g_free(address);

    cut_trace(assert_response(scenario, group));
}

static void
do_helo (MilterManagerTestScenario *scenario, const gchar *group)
{
    const gchar *fqdn;

    fqdn = get_string(scenario, group, "fqdn");

    milter_manager_children_helo(children, fqdn);
    cut_trace(assert_response(scenario, group));
}

static void
do_envelope_from (MilterManagerTestScenario *scenario, const gchar *group)
{
    const gchar *from;

    from = get_string(scenario, group, "from");

    milter_manager_children_envelope_from(children, from);
    cut_trace(assert_response(scenario, group));
}

static void
do_envelope_recipient (MilterManagerTestScenario *scenario, const gchar *group)
{
    const gchar *recipient;

    recipient = get_string(scenario, group, "recipient");

    milter_manager_children_envelope_recipient(children, recipient);
    cut_trace(assert_response(scenario, group));
}

static void
do_data (MilterManagerTestScenario *scenario, const gchar *group)
{
    milter_manager_children_data(children);
    cut_trace(assert_response(scenario, group));
}

static void
do_header (MilterManagerTestScenario *scenario, const gchar *group)
{
    const gchar *name, *value;

    name = get_string(scenario, group, "name");
    value = get_string(scenario, group, "value");
    milter_manager_children_header(children, name, value);

    cut_trace(assert_response(scenario, group));
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
/*
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
*/
      default:
        cut_error("unknown command: <%c>(<%s>)", command, group);
        break;
    }
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
        clear_n_emitted();
    }
}

void
data_scenario (void)
{
    cut_add_data("negotiate", g_strdup("negotiate.txt"), g_free,
                 "negotiate - retry", g_strdup("negotiate-retry.txt"), g_free,
                 "negotiate - retry - fail",
                 g_strdup("negotiate-retry-fail.txt"), g_free,
                 "connect", g_strdup("connect.txt"), g_free,
                 /* WRITEME */
                 /* "connect - pass", g_strdup("connect-pass.txt"), g_free */
                 /* "connect - half pass", g_strdup("connect-half-pass.txt"),
                    g_free */
                 "helo", g_strdup("helo.txt"), g_free,
                 "envelope-from", g_strdup("envelope-from.txt"), g_free,
                 "envelope-recipient",
                 g_strdup("envelope-recipient.txt"), g_free,
                 "data", g_strdup("data.txt"), g_free,
                 "header", g_strdup("header.txt"), g_free);
}

void
test_scenario (gconstpointer data)
{
    const gchar *scenario_name = data;
    const GList *node;

    main_scenario = milter_manager_test_scenario_new();
    cut_trace(milter_manager_test_scenario_load(main_scenario,
                                                scenario_dir,
                                                scenario_name));
    cut_trace(setup_parameters(main_scenario, children, config));

    cut_trace(milter_manager_test_scenario_start_clients(main_scenario));

    for (node = milter_manager_test_scenario_get_eggs(main_scenario);
         node;
         node = g_list_next(node)) {
        MilterManagerEgg *egg = node->data;
        MilterManagerChild *child;

        child = milter_manager_egg_hatch(egg);
        milter_manager_children_add_child(children, child);
        g_object_unref(child);
    }

    cut_trace(do_actions(main_scenario));
}

void
test_negotiate (void)
{
    option = milter_option_new(2,
                               MILTER_ACTION_ADD_HEADERS |
                               MILTER_ACTION_CHANGE_BODY,
                               MILTER_STEP_NONE);

    start_client(10026, arguments1);
    start_client(10027, arguments2);

    add_child("milter@10026", "inet:10026@localhost");
    add_child("milter@10027", "inet:10027@localhost");

    milter_manager_children_negotiate(children, option);
    wait_reply(1, n_negotiate_reply_emitted);
}

void
test_connect (void)
{
    struct sockaddr_in address;
    const gchar host_name[] = "mx.local.net";
    const gchar ip_address[] = "192.168.123.123";

    cut_trace(test_negotiate());

    address.sin_family = AF_INET;
    address.sin_port = g_htons(50443);
    inet_aton(ip_address, &(address.sin_addr));

    milter_manager_children_connect(children,
                                    host_name,
                                    (struct sockaddr *)(&address),
                                    sizeof(address));
    wait_reply(1, n_continue_emitted);
}

static gboolean
cb_check_connect_pass (MilterServerContext *context,
                       const gchar *host_name,
                       const struct sockaddr *address,
                       socklen_t address_length,
                       gpointer user_data)
{
    return TRUE;
}

static void
connect_pass_check_signals (gpointer data, gpointer user_data)
{
    MilterManagerChild *child = data;

#define CONNECT(name)                                                   \
    g_signal_connect(child, #name, G_CALLBACK(cb_ ## name ## _pass), NULL)

    CONNECT(check_connect);

#undef CONNECT
}

void
test_connect_pass (void)
{
    struct sockaddr_in address;
    const gchar host_name[] = "mx.local.net";
    const gchar ip_address[] = "192.168.123.123";

    cut_trace(test_negotiate());

    milter_manager_children_foreach(children, connect_pass_check_signals, NULL);

    address.sin_family = AF_INET;
    address.sin_port = g_htons(50443);
    inet_aton(ip_address, &(address.sin_addr));

    milter_manager_children_connect(children,
                                    host_name,
                                    (struct sockaddr *)(&address),
                                    sizeof(address));
    wait_reply(1, n_accept_emitted);
}

void
test_connect_half_pass (void)
{
    GList *child_list = NULL;
    struct sockaddr_in address;
    const gchar host_name[] = "mx.local.net";
    const gchar ip_address[] = "192.168.123.123";

    cut_trace(test_negotiate());

    child_list = milter_manager_children_get_children(children);
    connect_pass_check_signals(child_list->data, NULL);

    address.sin_family = AF_INET;
    address.sin_port = g_htons(50443);
    inet_aton(ip_address, &(address.sin_addr));

    milter_manager_children_connect(children,
                                    host_name,
                                    (struct sockaddr *)(&address),
                                    sizeof(address));
    wait_reply(1, n_continue_emitted);
}

void
test_helo (void)
{
    const gchar fqdn[] = "delian";

    cut_trace(test_connect());

    milter_manager_children_helo(children, fqdn);
    wait_reply(2, n_continue_emitted);
}

void
test_envelope_from (void)
{
    const gchar from[] = "example@example.com";

    cut_trace(test_helo());

    milter_manager_children_envelope_from(children, from);
    wait_reply(3, n_continue_emitted);
}

void
test_envelope_recipient (void)
{
    const gchar recipient[] = "example@example.com";

    cut_trace(test_envelope_from());

    milter_manager_children_envelope_recipient(children, recipient);
    wait_reply(4, n_continue_emitted);
}

void
test_data (void)
{
    cut_trace(test_envelope_recipient());

    milter_manager_children_data(children);
    wait_reply(5, n_continue_emitted);
}

void
test_header (void)
{
    const gchar name[] = "X-Test-Header";
    const gchar value[] = "Test Header Value";

    cut_trace(test_data());

    milter_manager_children_header(children, name, value);
    wait_reply(6, n_continue_emitted);
}

void
test_end_of_header (void)
{
    cut_trace(test_header());

    milter_manager_children_end_of_header(children);
    wait_reply(7, n_continue_emitted);
}

#define is_important_status(children, state, next_status)                    \
    milter_manager_children_is_important_status(children, state, next_status)

#define milter_manager_assert_important(children, state, next_status)        \
    cut_assert_true(is_important_status(children, state, next_status))

#define milter_manager_assert_not_important(children, state, next_status)    \
    cut_assert_false(is_important_status(children, state, next_status))

typedef struct _StateTestData
{
    MilterServerContextState state;
    MilterStatus status;
    MilterStatus next_status;
} StateTestData;

static StateTestData *
state_test_data_new (MilterServerContextState state,
                     MilterStatus status, MilterStatus next_status)
{
    StateTestData *data;

    data = g_new(StateTestData, 1);

    data->state = state;
    data->status = status;
    data->next_status = next_status;

    return data;
}

static void
prepare_children_state (MilterManagerChildren *children,
                        const StateTestData *data)
{
    milter_manager_children_set_status(children, data->state, data->status);
}

void
data_important_status (void)
{
#define test_data(state, status, next_status)                   \
    state_test_data_new(MILTER_SERVER_CONTEXT_STATE_ ## state,  \
                        MILTER_STATUS_ ## status,               \
                        MILTER_STATUS_ ## next_status)

    cut_add_data("connect - default - accept",
                 test_data(CONNECT, DEFAULT, ACCEPT), g_free,
                 "envelope_from - temporary_failure - continue",
                 test_data(ENVELOPE_FROM, TEMPORARY_FAILURE, CONTINUE), g_free,
                 "envelope_from - discard - reject",
                 test_data(ENVELOPE_FROM, DISCARD, REJECT), g_free,
                 "envelope_recipient - reject - discard",
                 test_data(ENVELOPE_RECIPIENT, REJECT, DISCARD), g_free,
                 "envelope_recipient - continue - reject",
                 test_data(ENVELOPE_RECIPIENT, CONTINUE, REJECT), g_free,
                 "envelope_recipient - accept - reject",
                 test_data(ENVELOPE_RECIPIENT, ACCEPT, REJECT), g_free,
                 "envelope_recipient - temporary_failure - reject",
                 test_data(ENVELOPE_RECIPIENT, TEMPORARY_FAILURE, REJECT),
                 g_free,
                 "envelope_recipient - temporary_failure - discard",
                 test_data(ENVELOPE_RECIPIENT, TEMPORARY_FAILURE, DISCARD),
                 g_free,
                 "envelope_recipient - temporary_failure - accept",
                 test_data(ENVELOPE_RECIPIENT, TEMPORARY_FAILURE, ACCEPT),
                 g_free,
                 "envelope_recipient - default - temporary_failure",
                 test_data(ENVELOPE_RECIPIENT, DEFAULT, TEMPORARY_FAILURE),
                 g_free,
                 "body - skip - discard",
                 test_data(BODY, SKIP, DISCARD),
                 g_free,
                 "body - skip - continue",
                 test_data(BODY, SKIP, CONTINUE),
                 g_free,
                 "body - accept - skip",
                 test_data(BODY, ACCEPT, SKIP),
                 g_free);
#undef test_data
}

void
test_important_status (gconstpointer data)
{
    const StateTestData *test_data = data;

    prepare_children_state(children, test_data);
    milter_manager_assert_important(children,
                                    test_data->state,
                                    test_data->next_status);
}

void
data_not_important_status (void)
{
#define test_data(state, status, next_status)                   \
    state_test_data_new(MILTER_SERVER_CONTEXT_STATE_ ## state,  \
                        MILTER_STATUS_ ## status,               \
                        MILTER_STATUS_ ## next_status)

    cut_add_data("connect - continue - accept",
                 test_data(CONNECT, CONTINUE, ACCEPT), g_free,
                 "envelope_from - reject - discard",
                 test_data(ENVELOPE_FROM, REJECT, DISCARD), g_free,
                 "envelope_recipient - discard - reject",
                 test_data(ENVELOPE_RECIPIENT, DISCARD, REJECT), g_free,
                 "envelope_recipient - continue - temporary_failure",
                 test_data(ENVELOPE_RECIPIENT, CONTINUE, TEMPORARY_FAILURE),
                 g_free,
                 "envelope_recipient - accept - temporary_failure",
                 test_data(ENVELOPE_RECIPIENT, ACCEPT, TEMPORARY_FAILURE),
                 g_free,
                 "body - continue - skip",
                 test_data(BODY, CONTINUE, SKIP),
                 g_free,
                 "body - skip - temporary_failure",
                 test_data(BODY, SKIP, TEMPORARY_FAILURE),
                 g_free);
#undef test_data
}

void
test_not_important_status (gconstpointer data)
{
    const StateTestData *test_data = data;

    prepare_children_state(children, test_data);
    milter_manager_assert_not_important(children, test_data->state, test_data->next_status);
}

static void
set_timeout (gpointer data, gpointer user_data)
{
    MilterServerContext *context = MILTER_SERVER_CONTEXT(data);

    milter_server_context_set_connection_timeout(context, 0);
    milter_server_context_set_reading_timeout(context, 0);
    milter_server_context_set_writing_timeout(context, 0);
    milter_server_context_set_end_of_message_timeout(context, 0);
}

static void
cb_reading_timeout (MilterServerContext *context, gpointer user_data)
{
    n_reading_timeout_emitted++;
}

static void
cb_end_of_message_timeout (MilterServerContext *context, gpointer user_data)
{
    n_end_of_message_timeout_emitted++;
}

static void
connect_timeout_signal (gpointer data, gpointer user_data)
{
    MilterServerContext *context = MILTER_SERVER_CONTEXT(data);

#define CONNECT(name)                                                   \
    g_signal_connect_after(context, #name, G_CALLBACK(cb_ ## name), NULL)

    CONNECT(end_of_message_timeout);
    CONNECT(reading_timeout);

#undef CONNECT
}

static void
cb_log (MilterLogger *logger, const gchar *domain,
        MilterLogLevelFlags level, const gchar *file,
        guint line, const gchar *function,
        GTimeVal *time_value, const gchar *message, gpointer user_data)
{
    if (level != MILTER_LOG_LEVEL_ERROR)
        return;

    if (g_str_equal(domain, "milter-manager"))
        error_message = g_strdup(message);
}

void
test_connection_timeout (void)
{
    MilterLogger *logger;

    logger = milter_logger();
    log_signal_id = g_signal_connect(logger, "log", G_CALLBACK(cb_log), NULL);

    add_child("milter@10026", "inet:10026@192.168.99.1");

    milter_manager_children_foreach(children, set_timeout, NULL);

    option = milter_option_new(2,
                               MILTER_ACTION_ADD_HEADERS |
                               MILTER_ACTION_CHANGE_BODY,
                               MILTER_STEP_NONE);
    cut_assert_true(milter_manager_children_negotiate(children, option));

    g_main_context_iteration(NULL, FALSE);
    cut_assert_equal_string("connection to milter@10026 is timed out.",
                            error_message);
}

static void
prepare_timeout_test (void)
{
    GList *child_list;
    child_list = milter_manager_children_get_children(children);
    
    set_timeout(g_list_next(child_list)->data, NULL);
    connect_timeout_signal(g_list_next(child_list)->data, NULL);
}

void
test_end_of_message_timeout (void)
{
    MilterLogger *logger;

    arguments_append(arguments2,
                     "--action", "no_response",
                     "--end-of-message", "end",
                     "--wait-second", "10",
                     NULL);

    cut_trace(test_end_of_header());

    logger = milter_logger();
    log_signal_id = g_signal_connect(logger, "log", G_CALLBACK(cb_log), NULL);

    prepare_timeout_test();

    milter_manager_children_end_of_message(children, "end", strlen("end"));

    wait_reply(7, n_continue_emitted);
    wait_reply(1, n_end_of_message_timeout_emitted);

    cut_assert_equal_string("The response of end-of-message from "
                            "milter@10027 is timed out.",
                            error_message);
}

void
test_writing_timeout (void)
{
    cut_notify("writing-timeout is never occured on the current implementation.");
}

void
test_reading_timeout (void)
{
    struct sockaddr_in address;
    const gchar host_name[] = "mx.local.net";
    const gchar ip_address[] = "192.168.123.123";
    MilterLogger *logger;

    arguments_append(arguments2,
                     "--action", "no_response",
                     "--connect-host", host_name,
                     "--wait-second", "10",
                     NULL);

    cut_trace(test_negotiate());

    logger = milter_logger();
    log_signal_id = g_signal_connect(logger, "log", G_CALLBACK(cb_log), NULL);

    prepare_timeout_test();

    address.sin_family = AF_INET;
    address.sin_port = g_htons(50443);
    inet_aton(ip_address, &(address.sin_addr));

    milter_manager_children_connect(children,
                                    host_name,
                                    (struct sockaddr *)(&address),
                                    sizeof(address));

    wait_reply(1, n_continue_emitted);
    wait_reply(1, n_reading_timeout_emitted);

    cut_assert_equal_string("reading from milter@10027 is timed out.",
                            error_message);
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
