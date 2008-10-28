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
#include <milter/manager/milter-manager-controller.h>
#undef shutdown

void test_negotiate (void);
void test_connect (void);
void test_helo (void);
void test_envelope_from (void);
void test_envelope_receipt (void);
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

static GError *controller_error;

static GList *test_milters;

static gchar *test_client_path;

static gboolean client_ready;
static gboolean client_negotiated;
static gboolean client_connected;
static gboolean client_greeted;
static gboolean client_envelope_from_received;
static gboolean client_envelope_receipt_received;
static gboolean client_data_received;
static gboolean client_header_received;
static gboolean client_end_of_header_received;
static gboolean client_body_received;
static gboolean client_end_of_message_received;
static gboolean client_quit_received;
static gboolean client_abort_received;
static gboolean client_unknown_received;

static gboolean client_reaped;

static gchar *client_header_name;
static gchar *client_header_value;

static gchar *client_body_chunk;
static gchar *client_end_of_message_chunk;
static gchar *client_unknown_command;


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
    controller_error = g_error_copy(error);
}

static void
setup_controller_signals (MilterManagerController *controller)
{
#define CONNECT(name)                                                   \
    g_signal_connect(controller, #name, G_CALLBACK(cb_ ## name), NULL)

    CONNECT(error);

#undef CONNECT
}

static void
cb_output_received (GCutEgg *egg, const gchar *chunk, gsize size,
                    gpointer user_data)
{
    if (g_str_has_prefix(chunk, "ready")) {
        client_ready = TRUE;
    } else if (g_str_has_prefix(chunk, "receive: negotiate")) {
        client_negotiated = TRUE;
    } else if (g_str_has_prefix(chunk, "receive: connect")) {
        client_connected = TRUE;
    } else if (g_str_has_prefix(chunk, "receive: helo")) {
        client_greeted = TRUE;
    } else if (g_str_has_prefix(chunk, "receive: mail")) {
        client_envelope_from_received = TRUE;
    } else if (g_str_has_prefix(chunk, "receive: rcpt")) {
        client_envelope_receipt_received = TRUE;
    } else if (g_str_has_prefix(chunk, "receive: data")) {
        client_data_received = TRUE;
    } else if (g_str_has_prefix(chunk, "receive: header")) {
        gchar **items;

        client_header_received = TRUE;
        items = g_strsplit_set(chunk, " \n", -1);
        if (items[0] && items[1] && items[2] && items[3]) {
            client_header_name = g_strdup(items[2]);
            client_header_value = g_strdup(items[3]);
        }
        g_strfreev(items);
    } else if (g_str_has_prefix(chunk, "receive: end-of-header")) {
        client_end_of_header_received = TRUE;
    } else if (g_str_has_prefix(chunk, "receive: body")) {
        gsize receive_body_mark_position;

        client_body_received = TRUE;
        receive_body_mark_position = strlen("receive: body: ");
        client_body_chunk = g_strndup(chunk + receive_body_mark_position,
                                      size - receive_body_mark_position - 1);
    } else if (g_str_has_prefix(chunk, "receive: end-of-message")) {
        gsize receive_end_of_message_mark_position;

        client_end_of_message_received = TRUE;
        receive_end_of_message_mark_position =
            strlen("receive: end-of-message: ");
        if (receive_end_of_message_mark_position < size)
            client_end_of_message_chunk =
                g_strndup(chunk + receive_end_of_message_mark_position,
                          size - receive_end_of_message_mark_position - 1);
    } else if (g_str_has_prefix(chunk, "receive: quit")) {
        client_quit_received = TRUE;
    } else if (g_str_has_prefix(chunk, "receive: abort")) {
        client_abort_received = TRUE;
    } else if (g_str_has_prefix(chunk, "receive: unknown")) {
        gsize receive_unknown_mark_position;

        client_unknown_received = TRUE;
        receive_unknown_mark_position = strlen("receive: unknown: ");
        if (receive_unknown_mark_position < size)
            client_unknown_command =
                g_strndup(chunk + receive_unknown_mark_position,
                          size - receive_unknown_mark_position - 1);
    } else {
        GString *string;

        string = g_string_new_len(chunk, size);
        g_print("[OUTPUT] <%s>\n", string->str);
        g_string_free(string, TRUE);
    }
}

static void
cb_error_received (GCutEgg *egg, const gchar *chunk, gsize size,
                   gpointer user_data)
{
    GString *string;

    string = g_string_new_len(chunk, size);
    g_print("[ERROR] <%s>\n", string->str);
    g_string_free(string, TRUE);
}

static void
cb_reaped (GCutEgg *egg, gint status, gpointer user_data)
{
    client_reaped = TRUE;
}

static void
setup_egg_signals (GCutEgg *egg)
{
#define CONNECT(name)                                                   \
    g_signal_connect(egg, #name, G_CALLBACK(cb_ ## name), NULL)

    CONNECT(output_received);
    CONNECT(error_received);
    CONNECT(reaped);

#undef CONNECT
}

static void
teardown_egg_signals (GCutEgg *egg)
{
#define DISCONNECT(name)                                                \
    g_signal_handlers_disconnect_by_func(egg,                           \
                                         G_CALLBACK(cb_ ## name),       \
                                         NULL)

    DISCONNECT(output_received);
    DISCONNECT(error_received);
    DISCONNECT(reaped);

#undef DISCONNECT
}

static void
free_egg (GCutEgg *egg)
{
    teardown_egg_signals(egg);
    g_object_unref(egg);
}

static void
make_egg (const gchar *command, ...)
{
    GCutEgg *egg;
    va_list args;

    va_start(args, command);
    egg = gcut_egg_new_va_list(command, args);
    va_end(args);

    setup_egg_signals(egg);

    test_milters = g_list_prepend(test_milters, egg);
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

    test_milters = NULL;

    test_client_path = g_build_filename(milter_manager_test_get_base_dir(),
                                        "lib",
                                        "milter-test-client.rb",
                                        NULL);

    client_ready = FALSE;
    client_negotiated = FALSE;
    client_connected = FALSE;
    client_greeted = FALSE;
    client_envelope_from_received = FALSE;
    client_envelope_receipt_received = FALSE;
    client_data_received = FALSE;
    client_header_received = FALSE;
    client_body_received = FALSE;
    client_end_of_message_received = FALSE;
    client_unknown_received = FALSE;

    client_reaped = FALSE;

    client_header_name = NULL;
    client_header_value = NULL;
    client_body_chunk = NULL;
    client_end_of_message_chunk = NULL;
    client_unknown_command = NULL;
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

    if (test_milters) {
        g_list_foreach(test_milters, (GFunc)free_egg, NULL);
        g_list_free(test_milters);
    }

    if (test_client_path)
        g_free(test_client_path);

    if (client_header_name)
        g_free(client_header_name);
    if (client_header_value)
        g_free(client_header_value);

    if (client_body_chunk)
        g_free(client_body_chunk);

    if (client_end_of_message_chunk)
        g_free(client_end_of_message_chunk);

    if (client_unknown_command)
        g_free(client_unknown_command);
}

static void
hatch_egg (GCutEgg *egg)
{
    GError *error = NULL;
    client_ready = FALSE;
    client_reaped = FALSE;

    gcut_egg_hatch(egg, &error);
    gcut_assert_error(error);

    while (!client_ready && !client_reaped)
        g_main_context_iteration(NULL, TRUE);
    cut_assert_false(client_reaped);
}

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

void
test_negotiate (void)
{
    option = milter_option_new(2,
                               MILTER_ACTION_ADD_HEADERS |
                               MILTER_ACTION_CHANGE_BODY,
                               MILTER_STEP_NONE);

    make_egg(test_client_path, "--print-status",
             "--timeout", "1.0", "--port", "10025", NULL);
    make_egg(test_client_path, "--print-status",
             "--timeout", "1.0", "--port", "10026", NULL);
    g_list_foreach(test_milters, (GFunc)hatch_egg, NULL);

    milter_manager_controller_negotiate(controller, option);
    wait_response("negotiate");
    cut_assert_true(client_negotiated);
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
    cut_assert_true(client_connected);
}

void
test_helo (void)
{
    const gchar fqdn[] = "delian";

    cut_trace(test_connect());

    milter_manager_controller_helo(controller, fqdn);
    wait_response("helo");
    cut_assert_true(client_greeted);
}

void
test_envelope_from (void)
{
    const gchar from[] = "kou+sender@cozmixng.org";

    cut_trace(test_helo());

    milter_manager_controller_envelope_from(controller, from);
    wait_response("envelope-from");
    cut_assert_true(client_envelope_from_received);
}

void
test_envelope_receipt (void)
{
    const gchar receipt[] = "kou+receiver@cozmixng.org";

    cut_trace(test_envelope_from());

    milter_manager_controller_envelope_receipt(controller, receipt);
    wait_response("envelope-receipt");
    cut_assert_true(client_envelope_receipt_received);
}

void
test_data (void)
{
    cut_trace(test_envelope_receipt());

    milter_manager_controller_data(controller);
    wait_response("data");
    cut_assert_true(client_data_received);
}

void
test_header (void)
{
    const gchar name[] = "From";
    const gchar value[] = "kou+sender@cozmixng.org";

    cut_trace(test_data());

    milter_manager_controller_header(controller, name, value);
    wait_response("header");
    cut_assert_true(client_header_received);
    cut_assert_equal_string(name, client_header_name);
    cut_assert_equal_string(value, client_header_value);
}

void
test_end_of_header (void)
{
    cut_trace(test_header());

    milter_manager_controller_end_of_header(controller);
    wait_response("end-of-header");
    cut_assert_true(client_end_of_header_received);
}

void
test_body (void)
{
    const gchar chunk[] = "Hi\n\nThanks,";

    cut_trace(test_end_of_header());

    milter_manager_controller_body(controller, chunk, strlen(chunk));
    wait_response("body");
    cut_assert_true(client_body_received);
    cut_assert_equal_string(chunk, client_body_chunk);
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
    const gchar *chunk = data;
    gsize chunk_size;

    cut_trace(test_body());

    if (chunk)
        chunk_size = strlen(chunk);
    else
        chunk_size = 0;

    milter_manager_controller_end_of_message(controller, chunk, chunk_size);
    wait_response("end-of-message");
    cut_assert_true(client_end_of_message_received);
    cut_assert_equal_string(chunk, client_end_of_message_chunk);
}

void
test_quit (void)
{
    cut_trace(test_end_of_message(NULL));

    milter_manager_controller_quit(controller);
    g_main_context_iteration(NULL, TRUE);
    cut_assert_true(client_quit_received);
}

void
test_abort (void)
{
    cut_trace(test_end_of_message(NULL));

    milter_manager_controller_abort(controller);
    g_main_context_iteration(NULL, TRUE);
    cut_assert_true(client_abort_received);
}

void
test_unknown (void)
{
    const gchar command[] = "UNKNOWN COMMAND";

    cut_trace(test_helo());

    milter_manager_controller_unknown(controller, command);
    g_main_context_iteration(NULL, TRUE);
    cut_assert_true(client_unknown_received);
    cut_assert_equal_string(command, client_unknown_command);
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
