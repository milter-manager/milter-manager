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

void test_negotiate (void);
void test_connect (void);
void test_helo (void);
void test_envelope_from (void);
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
void test_quit (void);
void test_abort (void);
void test_unknown (void);
void test_add_header (void);

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
static GList *expected_headers;

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
    expected_headers = NULL;
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

    if (expected_headers) {
        g_list_foreach(expected_headers,
                       (GFunc)milter_manager_test_header_free, NULL);
        g_list_free(expected_headers);
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
test_envelope_recipient (void)
{
    const gchar recipient[] = "kou+receiver@cozmixng.org";

    cut_trace(test_envelope_from());

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
                          /* the child which returns "ACCEPT" must die */
                          collect_n_received(data));
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
                           MILTER_STATUS_ACCEPT, response_status);
    /* Check the process is ended if both children returns ACCEPT. */
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
    const gchar temporary_failure_recipient[] = "kou+temporary_failure@cozmixng.org";
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
                          /* the child which returns "TEMPORARY_FAILURE" must live */
                          collect_n_received(data));
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
                           MILTER_STATUS_TEMPORARY_FAILURE, response_status);
    /* The process goes on even if TEMPORARY_FAILURE returns in ENVELOPE_RECIPIENT. */
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

    milter_server_context_end_of_message(MILTER_SERVER_CONTEXT(server), NULL, 0);
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
    cut_assert_equal_string(command, get_received_data(unknown_command));
}

void
test_add_header (void)
{
    const gchar name[] = "X-Test-Header";
    const gchar value[] = "Test Header Value";
    const GList *headers;
    gchar *header_string;

    expected_headers =
        g_list_append(expected_headers,
                      milter_manager_test_header_new(name, value));

    header_string = g_strdup_printf("%s:%s", name, value);
    cut_take_string(header_string);
    arguments_append(arguments1,
                     "--add-header", header_string,
                     NULL);

    cut_trace(test_end_of_message(NULL));

    cut_trace(milter_manager_test_server_wait_signal(server));

    cut_assert_equal_uint(
        1,
        milter_manager_test_server_get_n_add_headers(server));

    headers = milter_manager_test_server_get_headers(server);
    gcut_assert_equal_list(expected_headers, headers,
                           milter_manager_test_header_equal,
                           milter_manager_test_header_inspect);
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
