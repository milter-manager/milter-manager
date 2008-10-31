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

#include <milter-manager-test-utils.h>
#include <milter-manager-test-client.h>
#include <milter/manager/milter-manager-children.h>
#undef shutdown

void test_foreach (void);
void test_negotiate (void);
void test_retry_negotiate (void);
void test_connect (void);
void test_connect_pass (void);
void test_connect_half_pass (void);
void data_important_status (void);
void test_important_status (gconstpointer data);
void data_not_important_status (void);
void test_not_important_status (gconstpointer data);

static MilterManagerConfiguration *config;
static MilterManagerChildren *children;
static MilterOption *option;

static GError *actual_error;
static GError *expected_error;
static GList *actual_names;
static GList *expected_names;

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

static GList *test_clients;

static void
cb_negotiate_reply (MilterManagerChildren *children,
                    MilterOption *option, MilterMacrosRequests *macros_requests,
                    gpointer user_data)
{
    n_negotiate_reply_emitted++;
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
start_client (guint port)
{
    MilterManagerTestClient *client;
    GError *error = NULL;

    client = milter_manager_test_client_new(port);
    test_clients = g_list_append(test_clients, client);
    if (!milter_manager_test_client_run(client, &error)) {
        gcut_assert_error(error);
        cut_fail("couldn't run test client on port <%u>", port);
    }
}

void
setup (void)
{
    config = milter_manager_configuration_new(NULL);
    children = milter_manager_children_new(config);
    setup_signals(children);

    option = NULL;

    actual_error = NULL;
    expected_error = NULL;

    actual_names = NULL;
    expected_names = NULL;

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

    test_clients = NULL;
}

void
teardown (void)
{
    if (config)
        g_object_unref(config);
    if (children)
        g_object_unref(children);

    if (option)
        g_object_unref(option);

    if (actual_error)
        g_error_free(actual_error);
    if (expected_error)
        g_error_free(expected_error);

    if (actual_names)
        gcut_list_string_free(actual_names);
    if (expected_names)
        gcut_list_string_free(expected_names);

    if (test_clients) {
        g_list_foreach(test_clients, (GFunc)g_object_unref, NULL);
        g_list_free(test_clients);
    }
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

    milter_manager_children_add_child(children,
                                      milter_manager_child_new("1"));
    milter_manager_children_add_child(children,
                                      milter_manager_child_new("2"));
    milter_manager_children_add_child(children,
                                      milter_manager_child_new("3"));
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

#define wait_reply(actual)                      \
    cut_trace_with_info_expression(             \
        wait_reply_helper(1, &actual),          \
        wait_reply(actual))

static void
wait_reply_helper (guint expected, guint *actual)
{
    gboolean timeout_waiting = TRUE;
    guint timeout_waiting_id;

    timeout_waiting_id = g_timeout_add_seconds(5, cb_timeout_waiting,
                                               &timeout_waiting);
    while (timeout_waiting && expected > *actual) {
        g_main_context_iteration(NULL, TRUE);
    }
    g_source_remove(timeout_waiting_id);

    cut_assert_true(timeout_waiting, "timeout");
    cut_assert_equal_uint(expected, *actual);
}

static void
add_child (const gchar *name, const gchar *connection_spec)
{
    MilterManagerEgg *egg;
    GError *error = NULL;

    egg = milter_manager_egg_new(name);
    if (!milter_manager_egg_set_connection_spec(egg,
                                                connection_spec,
                                                &error)) {
        g_object_unref(egg);
        gcut_assert_error(error);
    } else {
        MilterManagerChild *child;

        child = milter_manager_egg_hatch(egg);
        milter_manager_children_add_child(children, child);
        g_object_unref(egg);
        g_object_unref(child);
    }
}

static void
add_child_with_command_and_options (const gchar *name,
                                    const gchar *connection_spec,
                                    const gchar *command,
                                    const gchar *command_options)
{
    MilterManagerEgg *egg;
    GError *error = NULL;

    egg = milter_manager_egg_new(name);
    if (!milter_manager_egg_set_connection_spec(egg,
                                                connection_spec,
                                                &error)) {
        g_object_unref(egg);
        gcut_assert_error(error);
    } else {
        MilterManagerChild *child;
        milter_manager_egg_set_command(egg, command);
        milter_manager_egg_set_command_options(egg, command_options);
        child = milter_manager_egg_hatch(egg);
        milter_manager_children_add_child(children, child);
        g_object_unref(egg);
        g_object_unref(child);
    }
}

void
test_negotiate (void)
{
    option = milter_option_new(2,
                               MILTER_ACTION_ADD_HEADERS |
                               MILTER_ACTION_CHANGE_BODY,
                               MILTER_STEP_NONE);

    start_client(10026);
    start_client(10027);

    add_child("milter@10026", "inet:10026@localhost");
    add_child("milter@10027", "inet:10027@localhost");

    milter_manager_children_negotiate(children, option);
    wait_reply(n_negotiate_reply_emitted);
    cut_assert_equal_uint(1, n_negotiate_reply_emitted);
}

void
test_retry_negotiate (void)
{
    gchar *command;
    option = milter_option_new(2,
                               MILTER_ACTION_ADD_HEADERS |
                               MILTER_ACTION_CHANGE_BODY,
                               MILTER_STEP_NONE);
    g_object_set(config,
                 "privilege-mode", TRUE,
                 NULL);
    command = g_build_filename(milter_manager_test_get_base_dir(),
                               "lib",
                               "milter-test-client.rb",
                               NULL);
    cut_take_string(command);
    start_client(10026);

    add_child_with_command_and_options("milter@10027",
                                       "inet:10027@localhost",
                                       command,
                                       "--print-status "
                                       "--timeout=5.0 "
                                       "--port=10027");
    add_child("milter@10026", "inet:10026@localhost");

    milter_manager_children_negotiate(children, option);
    wait_reply(n_negotiate_reply_emitted);
    milter_manager_children_quit(children);
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
    wait_reply(n_continue_emitted);
    cut_assert_equal_uint(1, n_continue_emitted);
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
    wait_reply(n_accept_emitted);
    cut_assert_equal_uint(1, n_accept_emitted);
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
    wait_reply(n_continue_emitted);
    cut_assert_equal_uint(1, n_continue_emitted);
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
    milter_manager_children_set_status(children, data->status);
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
                 "end_of_message - quarantine - reject",
                 test_data(END_OF_MESSAGE, QUARANTINE, REJECT),
                 g_free,
                 "end_of_message - quarantine - discard",
                 test_data(END_OF_MESSAGE, QUARANTINE, DISCARD),
                 g_free,
                 "end_of_message - skip - quarantine",
                 test_data(END_OF_MESSAGE, SKIP, QUARANTINE),
                 g_free,
                 "end_of_message - accept - quarantine",
                 test_data(END_OF_MESSAGE, ACCEPT, QUARANTINE),
                 g_free,
                 "end_of_message - skip - discard",
                 test_data(END_OF_MESSAGE, SKIP, DISCARD),
                 g_free,
                 "end_of_message - skip - continue",
                 test_data(END_OF_MESSAGE, SKIP, CONTINUE),
                 g_free,
                 "end_of_message - accept - skip",
                 test_data(END_OF_MESSAGE, ACCEPT, SKIP),
                 g_free);
#undef test_data
}

void
test_important_status (gconstpointer data)
{
    const StateTestData *test_data = data;

    prepare_children_state(children, test_data);
    milter_manager_assert_important(children, test_data->state, test_data->next_status);
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
                 "end_of_message - reject - quarantine",
                 test_data(END_OF_MESSAGE, REJECT, QUARANTINE),
                 g_free,
                 "end_of_message - quarantine - accept",
                 test_data(END_OF_MESSAGE, QUARANTINE, ACCEPT),
                 g_free,
                 "end_of_message - continue - skip",
                 test_data(END_OF_MESSAGE, CONTINUE, SKIP),
                 g_free,
                 "end_of_message - skip - temporary_failure",
                 test_data(END_OF_MESSAGE, SKIP, TEMPORARY_FAILURE),
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

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
