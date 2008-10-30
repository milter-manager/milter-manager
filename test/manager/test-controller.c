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
#undef shutdown

void test_negotiate (void);
void test_connect (void);
void test_helo (void);
void test_envelope_from (void);
void test_envelope_receipt (void);
void test_envelope_receipt_accept (void);
void test_envelope_receipt_reject (void);
void test_envelope_receipt_discard (void);
void test_envelope_receipt_temporary_failure (void);
void test_data (void);
void test_header (void);
void test_end_of_header (void);
void test_body (void);
void data_end_of_message (void);
void test_end_of_message (gconstpointer data);
void test_quit (void);
void test_abort (void);
void test_unknown (void);

static MilterManagerConfiguration *config;
static MilterClientContext *client_context;
static MilterManagerController *controller;
static MilterOption *option;

static GArray *arguments1;

static GError *controller_error;
static gboolean finished;

static GList *test_clients;

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
    config = milter_manager_configuration_new(NULL);
    add_load_path(g_getenv("MILTER_MANAGER_CONFIG_DIR"));
    milter_manager_configuration_load(config, "milter-manager.conf");

    client_context = milter_client_context_new();

    controller = milter_manager_controller_new(config, client_context);
    controller_error = NULL;
    setup_controller_signals(controller);

    option = NULL;

    arguments1 = g_array_new(TRUE, TRUE, sizeof(gchar *));

    test_clients = NULL;

    response_status = MILTER_STATUS_DEFAULT;

    finished = FALSE;
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
    if (client_context)
        g_object_unref(client_context);
    if (controller)
        g_object_unref(controller);
    if (option)
        g_object_unref(option);

    if (arguments1)
        arguments_free(arguments1);

    if (test_clients) {
        g_list_foreach(test_clients, (GFunc)g_object_unref, NULL);
        g_list_free(test_clients);
    }
}

#define collect_n_received(event)                                       \
    milter_manager_test_clients_collect_n_received(                     \
        test_clients,                                                   \
        milter_manager_test_client_get_n_ ## event ## _received)

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

void
test_negotiate (void)
{
    option = milter_option_new(2,
                               MILTER_ACTION_ADD_HEADERS |
                               MILTER_ACTION_CHANGE_BODY,
                               MILTER_STEP_NONE);

    start_client(10026, arguments1);
    start_client(10027, NULL);

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

void
test_envelope_from (void)
{
    const gchar from[] = "kou+sender@cozmixng.org";

    cut_trace(test_helo());

    milter_manager_controller_envelope_from(controller, from);
    wait_response("envelope-from");
    cut_assert_equal_uint(g_list_length(test_clients),
                          collect_n_received(envelope_from));
}

void
test_envelope_receipt (void)
{
    const gchar receipt[] = "kou+receiver@cozmixng.org";

    cut_trace(test_envelope_from());

    milter_manager_controller_envelope_receipt(controller, receipt);
    wait_response("envelope-receipt");
    cut_assert_equal_uint(g_list_length(test_clients),
                          collect_n_received(envelope_receipt));
}

void
test_envelope_receipt_accept (void)
{
    const gchar receipt[] = "kou+accepted@cozmixng.org";

    arguments_append(arguments1,
                     "--action", "accept",
                     "--envelope-receipt", receipt,
                     NULL);

    cut_trace(test_envelope_receipt());

    milter_manager_controller_envelope_receipt(controller, receipt);
    wait_response("envelope-receipt");
    cut_assert_equal_uint(g_list_length(test_clients) * 2,
                          collect_n_received(envelope_receipt));
    gcut_assert_equal_enum(MILTER_TYPE_STATUS,
                           MILTER_STATUS_CONTINUE, response_status);
    milter_manager_controller_data(controller);
    wait_response("data");
    cut_assert_equal_uint(g_list_length(test_clients) - 1, /* the child which returns "ACCEPT" must die */
                          collect_n_received(data));
}

void
test_envelope_receipt_reject (void)
{
    const gchar receipt[] = "kou+rejected@cozmixng.org";

    arguments_append(arguments1,
                     "--action", "reject",
                     "--envelope-receipt", receipt,
                     NULL);

    cut_trace(test_envelope_receipt());

    milter_manager_controller_envelope_receipt(controller, receipt);
    wait_response("envelope-receipt");
    cut_assert_equal_uint(g_list_length(test_clients) * 2,
                          collect_n_received(envelope_receipt));
    gcut_assert_equal_enum(MILTER_TYPE_STATUS,
                           MILTER_STATUS_REJECT, response_status);
    milter_manager_controller_data(controller);
    wait_response("data");
    cut_assert_equal_uint(g_list_length(test_clients),
                          collect_n_received(data));
}

void
test_envelope_receipt_discard (void)
{
    const gchar receipt[] = "kou+discarded@cozmixng.org";

    arguments_append(arguments1,
                     "--action", "discard",
                     "--envelope-receipt", receipt,
                     NULL);

    cut_trace(test_envelope_receipt());

    milter_manager_controller_envelope_receipt(controller, receipt);
    wait_response("envelope-receipt");
    cut_assert_equal_uint(g_list_length(test_clients) * 2,
                          collect_n_received(envelope_receipt));
    gcut_assert_equal_enum(MILTER_TYPE_STATUS,
                           MILTER_STATUS_DISCARD, response_status);
}

void
test_envelope_receipt_temporary_failure (void)
{
    const gchar receipt[] = "kou+temporary_failureed@cozmixng.org";

    arguments_append(arguments1,
                     "--action", "temporary_failure",
                     "--envelope-receipt", receipt,
                     NULL);

    cut_trace(test_envelope_receipt());

    milter_manager_controller_envelope_receipt(controller, receipt);
    wait_response("envelope-receipt");
    cut_assert_equal_uint(g_list_length(test_clients) * 2,
                          collect_n_received(envelope_receipt));
    gcut_assert_equal_enum(MILTER_TYPE_STATUS,
                           MILTER_STATUS_CONTINUE, response_status);
    milter_manager_controller_data(controller);
    wait_response("data");
    cut_assert_equal_uint(g_list_length(test_clients), /* the child which returns "TEMPORARY_FAILURE" must live */
                          collect_n_received(data));
}

void
test_data (void)
{
    cut_trace(test_envelope_receipt());

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
    cut_assert_equal_string(name,
                            milter_manager_test_client_get_header_name(client));
    cut_assert_equal_string(value,
                            milter_manager_test_client_get_header_value(client));
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

void
test_body (void)
{
    MilterManagerTestClient *client;
    const gchar chunk[] = "Hi\n\nThanks,";

    cut_trace(test_end_of_header());

    milter_manager_controller_body(controller, chunk, strlen(chunk));
    wait_response("body");
    cut_assert_equal_uint(g_list_length(test_clients),
                          collect_n_received(body));

    client = test_clients->data;
    cut_assert_equal_string(chunk,
                            milter_manager_test_client_get_body_chunk(client));
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

    cut_trace(test_body());

    if (chunk)
        chunk_size = strlen(chunk);
    else
        chunk_size = 0;

    milter_manager_controller_end_of_message(controller, chunk, chunk_size);
    wait_response("end-of-message");
    gcut_assert_equal_enum(MILTER_TYPE_STATUS,
                           MILTER_STATUS_CONTINUE, response_status);
    cut_assert_equal_uint(g_list_length(test_clients),
                          collect_n_received(end_of_message));

    client = test_clients->data;
    cut_assert_equal_string(chunk,
                            milter_manager_test_client_get_end_of_message_chunk(client));
}

void
test_quit (void)
{
    cut_trace(test_end_of_message(NULL));

    milter_manager_controller_quit(controller);
    wait_finished();
    cut_assert_equal_uint(g_list_length(test_clients), collect_n_received(quit));
}

void
test_abort (void)
{
    cut_trace(test_end_of_message(NULL));

    milter_manager_controller_abort(controller);
    wait_reply(abort);
}

void
test_unknown (void)
{
    MilterManagerTestClient *client;
    const gchar command[] = "UNKNOWN COMMAND";

    cut_trace(test_helo());

    milter_manager_controller_unknown(controller, command);
    wait_response("unknown");
    wait_finished();

    client = test_clients->data;
    cut_assert_equal_string(command,
                            milter_manager_test_client_get_unknown_command(client));
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
