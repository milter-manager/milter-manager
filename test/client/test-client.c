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

#include <errno.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <unistd.h>
#include <grp.h>

#include <glib/gstdio.h>

#include <milter/client.h>
#include <milter/client/milter-client-private.h>
#include <milter-test-utils.h>

#include <gcutter.h>

void test_negotiate (void);
void test_helo (void);
void test_listen_started (void);
void test_unix_socket_mode (void);
void test_unix_socket_group (void);
void test_change_unix_socket_mode (void);
void test_change_unix_socket_group (void);
void test_remove_unix_socket_on_close_accessor (void);
void test_remove_unix_socket_on_close (void);
void test_remove_unix_socket_on_create_accessor (void);
void test_remove_unix_socket_on_create (void);
void test_not_remove_unix_socket_on_create (void);
void test_remove_pid_file_on_exit_accessor (void);
void test_remove_pid_file_on_exit (void);
void test_syslog_identify_accessor (void);
void test_syslog_facility_accessor (void);
void test_suspend_time_on_unacceptable (void);
void test_max_connections (void);
void test_effective_user (void);
void test_effective_group (void);
void test_event_loop_backend (void);
void test_maintenance_interval (void);
void test_connection_spec (void);
void test_need_maintain (void);
void test_need_maintain_over (void);
void test_need_maintain_over_interval (void);
void test_need_maintain_never (void);
void test_need_maintain_no_processing_sessions (void);
void test_need_maintain_no_processing_sessions_below_processed_sessions (void);
void test_need_maintain_no_processing_sessions_no_interval (void);
void test_n_workers (void);
void test_custom_fork (void);
void test_default_packet_buffer_size (void);
void test_worker_id (void);
void test_max_pending_finished_sessions (void);

static MilterEventLoop *loop;

static MilterClient *client;
static MilterTestServer *server;

static MilterDecoder *decoder;
static MilterCommandEncoder *encoder;

static gint n_negotiates;
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
static gint n_unknowns;
static gint n_finished_emissions;

static guint idle_id;
static guint idle_shutdown_id;
static guint shutdown_count;

static const gchar *spec;

static MilterOption *option;
static MilterOption *negotiate_option;

static MilterMacrosRequests *macros_requests;
static MilterMacrosRequests *negotiate_macros_requests;

static gchar *connect_host_name;
static struct sockaddr *connect_address;
static socklen_t connect_address_length;

static const gchar fqdn[] = "delian";
static gchar *helo_fqdn;

static gchar *envelope_from;

static gchar *envelope_recipient_to;

static gchar *header_name;
static gchar *header_value;

static gchar *body_chunk;
static gsize body_chunk_length;

static gchar *unknown_command;
static gsize unknown_command_length;

static GError *actual_error;
static GError *expected_error;

static struct sockaddr *actual_address;
static socklen_t actual_address_size;

static gchar *tmp_dir;

static guint loop_run_count;

static guint n_worker_fork_called;
static guint64 n_workers;

static void
cb_negotiate (MilterClientContext *context, MilterOption *option,
              MilterMacrosRequests *macros_requests, gpointer user_data)
{
    n_negotiates++;
    negotiate_option = g_object_ref(option);
    negotiate_macros_requests = g_object_ref(macros_requests);
}

static void
cb_connect (MilterClientContext *context, const gchar *host_name,
            const struct sockaddr *address, socklen_t address_length,
            gpointer user_data)
{
    n_connects++;

    connect_host_name = g_strdup(host_name);
    connect_address = malloc(address_length);
    memcpy(connect_address, address, address_length);
    connect_address_length = address_length;
}

static void
cb_helo (MilterClientContext *context, const gchar *fqdn, gpointer user_data)
{
    n_helos++;

    helo_fqdn = g_strdup(fqdn);
}

static void
cb_envelope_from (MilterClientContext *context, const gchar *from, gpointer user_data)
{
    n_envelope_froms++;

    envelope_from = g_strdup(from);
}

static void
cb_envelope_recipient (MilterClientContext *context, const gchar *to,
                       gpointer user_data)
{
    n_envelope_recipients++;

    envelope_recipient_to = g_strdup(to);
}

static void
cb_data (MilterClientContext *context, gpointer user_data)
{
    n_datas++;
}

static void
cb_header (MilterClientContext *context, const gchar *name, const gchar *value,
           gpointer user_data)
{
    n_headers++;

    if (header_name)
        g_free(header_name);
    header_name = g_strdup(name);

    if (header_value)
        g_free(header_value);
    header_value = g_strdup(value);
}

static void
cb_end_of_header (MilterClientContext *context, gpointer user_data)
{
    n_end_of_headers++;
}

static void
cb_body (MilterClientContext *context, const gchar *chunk, gsize length,
         gpointer user_data)
{
    n_bodies++;

    if (body_chunk)
        g_free(body_chunk);
    body_chunk = g_strndup(chunk, length);
    body_chunk_length = length;
}

static void
cb_end_of_message (MilterClientContext *context,
                   const gchar *chunk, gsize size,
                   gpointer user_data)
{
    n_end_of_messages++;
}

static void
cb_abort (MilterClientContext *context, MilterClientContextState state,
          gpointer user_data)
{
    n_aborts++;
}

static void
cb_unknown (MilterClientContext *context, const gchar *command,
            gpointer user_data)
{
    n_unknowns++;

    if (unknown_command)
        g_free(unknown_command);
    unknown_command = g_strdup(command);
}

static void
cb_finished (MilterFinishedEmittable *emittable, gpointer user_data)
{
    n_finished_emissions++;
}

static void
setup_context_signals (MilterClientContext *context)
{
#define CONNECT(name)                                                   \
    g_signal_connect(context, #name, G_CALLBACK(cb_ ## name), NULL)

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

    CONNECT(finished);
#undef CONNECT
}

static void
cb_connection_established (MilterClient *client, MilterClientContext *context,
                           gpointer user_data)
{
    setup_context_signals(context);
}

static void
cb_listen_started (MilterClient *client,
                   struct sockaddr *address, socklen_t address_size,
                   gpointer user_data)
{
    if (actual_address)
        g_free(actual_address);
    actual_address = g_memdup(address, address_size);
    actual_address_size = address_size;
}

static void
cb_error (MilterErrorEmittable *emittable, GError *error,
          gpointer user_data)
{
    actual_error = g_error_copy(error);
}

static void
loop_run (MilterEventLoop *loop)
{
    loop_run_count++;
    milter_event_loop_run_without_custom(loop);
}

static void
setup_client_signals (void)
{
#define CONNECT(name)                                                   \
    g_signal_connect(client, #name, G_CALLBACK(cb_ ## name), NULL)

    CONNECT(connection_established);
    CONNECT(listen_started);
    CONNECT(error);

#undef CONNECT
}

void
cut_setup (void)
{
    const gchar *env;

    spec = "inet:9999@127.0.0.1";

    client = milter_client_new();
    env = g_getenv("MILTER_N_WORKERS");
    if (env && (n_workers = g_ascii_strtoull(env, NULL, 10)) > 0) {
        if (n_workers > UINT_MAX)
            n_workers = 0;
        milter_client_set_n_workers(client, n_workers);
    }
    loop_run_count = 0;
    loop = milter_client_get_event_loop(client);
    g_object_ref(loop);
    milter_event_loop_set_custom_run_func(loop, loop_run);
    setup_client_signals();
    server = NULL;

    decoder = milter_reply_decoder_new();
    encoder = MILTER_COMMAND_ENCODER(milter_command_encoder_new());

    option = NULL;
    negotiate_option = NULL;

    macros_requests = NULL;
    negotiate_macros_requests = NULL;

    idle_id = 0;
    idle_shutdown_id = 0;
    shutdown_count = 10;

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
    n_unknowns = 0;

    n_finished_emissions = 0;

    connect_host_name = NULL;
    connect_address = NULL;
    connect_address_length = 0;

    helo_fqdn = NULL;

    envelope_from = NULL;

    envelope_recipient_to = NULL;

    header_name = NULL;
    header_value = NULL;

    body_chunk = NULL;
    body_chunk_length = 0;

    unknown_command = NULL;
    unknown_command_length = 0;

    actual_error = NULL;
    expected_error = NULL;

    actual_address = NULL;
    actual_address_size = 0;

    tmp_dir = milter_test_get_tmp_dir();

    n_worker_fork_called = 0;
}

void
cut_teardown (void)
{
    if (idle_id > 0)
        milter_event_loop_remove(loop, idle_id);
    if (idle_shutdown_id > 0)
        milter_event_loop_remove(loop, idle_shutdown_id);

    if (client)
        g_object_unref(client);
    if (server)
        g_object_unref(server);

    if (decoder)
        g_object_unref(decoder);
    if (encoder)
        g_object_unref(encoder);

    if (loop)
        g_object_unref(loop);

    if (option)
        g_object_unref(option);
    if (negotiate_option)
        g_object_unref(negotiate_option);

    if (macros_requests)
        g_object_unref(macros_requests);
    if (negotiate_macros_requests)
        g_object_unref(negotiate_macros_requests);

    if (connect_host_name)
        g_free(connect_host_name);
    if (connect_address)
        g_free(connect_address);

    if (helo_fqdn)
        g_free(helo_fqdn);

    if (envelope_from)
        g_free(envelope_from);

    if (envelope_recipient_to)
        g_free(envelope_recipient_to);

    if (header_name)
        g_free(header_name);
    if (header_value)
        g_free(header_value);

    if (body_chunk)
        g_free(body_chunk);

    if (unknown_command)
        g_free(unknown_command);

    if (actual_error)
        g_error_free(actual_error);
    if (expected_error)
        g_error_free(expected_error);

    if (tmp_dir) {
        cut_remove_path(tmp_dir, NULL);
        g_free(tmp_dir);
    }
}

static gboolean
cb_idle_shutdown (gpointer user_data)
{
    shutdown_count--;
    g_usleep(1);
    if (shutdown_count == 0) {
        milter_client_shutdown(client);
        if (server) {
            g_object_unref(server);
            server = NULL;
        }
    }

    return shutdown_count > 0;
}

static void
setup_client (void)
{
    GError *error = NULL;

    milter_client_set_connection_spec(client, spec, &error);
    gcut_assert_error(error);

    idle_shutdown_id = milter_event_loop_add_idle_full(loop,
                                                       G_PRIORITY_LOW,
                                                       cb_idle_shutdown,
                                                       NULL,
                                                       NULL);
}

static void
setup_test_server (void)
{
    cut_trace(server = milter_test_server_new(spec, decoder, loop));
}

static gboolean
cb_idle_negotiate (gpointer user_data)
{
    const gchar *packet;
    gsize packet_size;

    cut_trace(setup_test_server());
    milter_command_encoder_encode_negotiate(encoder,
                                            &packet, &packet_size, option);
    milter_test_server_write(server, packet, packet_size);

    return FALSE;
}

void
test_negotiate (void)
{
    guint32 version;
    MilterActionFlags action;
    MilterStepFlags step;
    GError *error = NULL;

    if (n_workers > 0)
        cut_omit("can't obtain the result from callbacks in child process");

    version = 2;
    action = MILTER_ACTION_ADD_HEADERS |
        MILTER_ACTION_CHANGE_BODY |
        MILTER_ACTION_ADD_ENVELOPE_RECIPIENT |
        MILTER_ACTION_DELETE_ENVELOPE_RECIPIENT |
        MILTER_ACTION_CHANGE_HEADERS |
        MILTER_ACTION_QUARANTINE |
        MILTER_ACTION_SET_SYMBOL_LIST;
    step = MILTER_STEP_NO_CONNECT |
        MILTER_STEP_NO_HELO |
        MILTER_STEP_NO_ENVELOPE_FROM |
        MILTER_STEP_NO_ENVELOPE_RECIPIENT |
        MILTER_STEP_NO_BODY |
        MILTER_STEP_NO_HEADERS |
        MILTER_STEP_NO_END_OF_HEADER;
    option = milter_option_new(version, action, step);

    idle_id = milter_event_loop_add_idle(loop, cb_idle_negotiate, NULL);

    cut_trace(setup_client());
    milter_client_run(client, &error);
    gcut_assert_error(error);

    milter_assert_equal_option(option, negotiate_option);

    macros_requests = milter_macros_requests_new();
    milter_assert_equal_macros_requests(macros_requests,
                                        negotiate_macros_requests);
    cut_assert_true(loop_run_count > 0);
}

static gboolean
cb_idle_helo (gpointer user_data)
{
    const gchar *packet;
    gsize packet_size;

    cut_trace(setup_test_server());
    milter_command_encoder_encode_helo(encoder, &packet, &packet_size, fqdn);
    milter_test_server_write(server, packet, packet_size);

    return FALSE;
}

void
test_helo (void)
{
    GError *error = NULL;

    if (n_workers > 0)
        cut_omit("can't obtain the result from callbacks in child process");

    idle_id = milter_event_loop_add_idle(loop, cb_idle_helo, NULL);

    cut_trace(setup_client());
    milter_client_run(client, &error);
    gcut_assert_error(error);

    cut_assert_equal_string(fqdn, helo_fqdn);
    cut_assert_true(loop_run_count > 0);
}

void
test_listen_started (void)
{
    struct sockaddr_in *address_inet;
    gchar actual_address_string[INET6_ADDRSTRLEN];
    GError *error = NULL;

    cut_trace(setup_client());
    milter_client_run(client, &error);
    gcut_assert_error(error);

    cut_assert_not_null(actual_address);

    address_inet = (struct sockaddr_in *)actual_address;
    cut_assert_equal_uint(AF_INET, address_inet->sin_family);
    cut_assert_equal_uint(9999, g_ntohs(address_inet->sin_port));
    cut_assert_equal_string("127.0.0.1",
                            inet_ntop(AF_INET, &(address_inet->sin_addr),
                                      actual_address_string,
                                      sizeof(actual_address_string)));
    cut_assert_equal_uint(sizeof(*address_inet), actual_address_size);
}

void
test_unix_socket_mode (void)
{
    cut_assert_equal_uint(0660,
                          milter_client_get_default_unix_socket_mode(client));
    cut_assert_equal_uint(0660,
                          milter_client_get_unix_socket_mode(client));

    milter_client_set_default_unix_socket_mode(client, 0600);
    cut_assert_equal_uint(0600,
                          milter_client_get_default_unix_socket_mode(client));
    cut_assert_equal_uint(0600,
                          milter_client_get_unix_socket_mode(client));

    milter_client_set_unix_socket_mode(client, 0666);
    cut_assert_equal_uint(0600,
                          milter_client_get_default_unix_socket_mode(client));
    cut_assert_equal_uint(0666,
                          milter_client_get_unix_socket_mode(client));

    milter_client_set_unix_socket_mode(client, 0);
    cut_assert_equal_uint(0600,
                          milter_client_get_default_unix_socket_mode(client));
    cut_assert_equal_uint(0600,
                          milter_client_get_unix_socket_mode(client));
}

void
test_unix_socket_group (void)
{
    cut_assert_equal_string(NULL,
                            milter_client_get_default_unix_socket_group(client));
    cut_assert_equal_string(NULL,
                            milter_client_get_unix_socket_group(client));

    milter_client_set_default_unix_socket_group(client, "nogroup");
    cut_assert_equal_string("nogroup",
                            milter_client_get_default_unix_socket_group(client));
    cut_assert_equal_string("nogroup",
                            milter_client_get_unix_socket_group(client));

    milter_client_set_default_unix_socket_group(client, NULL);
    cut_assert_equal_string(NULL,
                            milter_client_get_default_unix_socket_group(client));
    cut_assert_equal_string(NULL,
                            milter_client_get_unix_socket_group(client));

    milter_client_set_unix_socket_group(client, "nogroup");
    cut_assert_equal_string(NULL,
                            milter_client_get_default_unix_socket_group(client));
    cut_assert_equal_string("nogroup",
                            milter_client_get_unix_socket_group(client));

}

void
test_change_unix_socket_mode (void)
{
    const gchar *path;
    struct stat stat_buffer;
    GError *error = NULL;

    path = cut_take_printf("%s/milter.sock", tmp_dir);
    spec = cut_take_printf("unix:%s", path);

    milter_client_set_default_unix_socket_mode(client, 0666);
    milter_client_set_default_remove_unix_socket_on_close(client, FALSE);

    cut_trace(setup_client());
    milter_client_run(client, &error);
    gcut_assert_error(error);

    if (stat(path, &stat_buffer) == -1)
        cut_assert_errno();

    cut_assert_equal_uint(S_IFSOCK | 0666, stat_buffer.st_mode);
}

void
test_change_unix_socket_group (void)
{
    const gchar *path;
    struct stat stat_buffer;
    gint i, n_groups, max_n_groups;
    gid_t *groups;
    struct group *group = NULL;
    GError *error = NULL;

    path = cut_take_printf("%s/milter.sock", tmp_dir);
    spec = cut_take_printf("unix:%s", path);

    max_n_groups = getgroups(0, NULL);
    if (max_n_groups < 0)
        max_n_groups = 5;

    groups = g_new0(gid_t, max_n_groups);
    cut_take_memory(groups);

    n_groups = getgroups(max_n_groups, groups);
    if (n_groups == -1)
        cut_assert_errno();

    for (i = 0; i < n_groups; i++) {
        if (groups[i] == getegid())
            continue;

        errno = 0;
        group = getgrgid(groups[i]);
        if (!group) {
            if (errno == 0)
                cut_omit("can't find supplementary group: %u", groups[i]);
            else
                cut_assert_errno();
        }
    }

    if (!group)
        cut_omit("no supplementary group");

    milter_client_set_default_unix_socket_group(client, group->gr_name);
    milter_client_set_default_remove_unix_socket_on_close(client, FALSE);

    cut_trace(setup_client());
    milter_client_run(client, &error);
    gcut_assert_error(error);

    if (stat(path, &stat_buffer) == -1)
        cut_assert_errno();

    cut_assert_equal_uint(group->gr_gid, stat_buffer.st_gid);
}

void
test_remove_unix_socket_on_close_accessor (void)
{
    cut_assert_true(milter_client_get_default_remove_unix_socket_on_close(client));
    cut_assert_true(milter_client_is_remove_unix_socket_on_close(client));
    milter_client_set_default_remove_unix_socket_on_close(client, FALSE);
    cut_assert_false(milter_client_get_default_remove_unix_socket_on_close(client));
    cut_assert_false(milter_client_is_remove_unix_socket_on_close(client));
}

void
test_remove_unix_socket_on_close (void)
{
    const gchar *path;

    path = cut_take_printf("%s/milter.sock", tmp_dir);
    spec = cut_take_printf("unix:%s", path);

    cut_trace(setup_client());
    cut_assert_true(milter_client_run(client, &actual_error));
    gcut_assert_error(actual_error);

    cut_assert_false(g_file_test(path, G_FILE_TEST_EXISTS));
}

void
test_remove_unix_socket_on_create_accessor (void)
{
    cut_assert_true(milter_client_is_remove_unix_socket_on_create(client));
    milter_client_set_remove_unix_socket_on_create(client, FALSE);
    cut_assert_false(milter_client_is_remove_unix_socket_on_create(client));
}

void
test_remove_unix_socket_on_create (void)
{
    const gchar *path;
    GError *error = NULL;

    path = cut_take_printf("%s/milter.sock", tmp_dir);
    spec = cut_take_printf("unix:%s", path);

    g_file_set_contents(path, "", 0, &error);
    gcut_assert_error(error);

    cut_trace(setup_client());
    cut_assert_true(milter_client_run(client, &error));
    gcut_assert_error(error);
    gcut_assert_error(actual_error);
}

void
test_not_remove_unix_socket_on_create (void)
{
    const gchar *path;
    GError *error = NULL;

    path = cut_take_printf("%s/milter.sock", tmp_dir);
    spec = cut_take_printf("unix:%s", path);

    g_file_set_contents(path, "", 0, &error);
    gcut_assert_error(error);

    milter_client_set_remove_unix_socket_on_create(client, FALSE);

    cut_trace(setup_client());
    cut_assert_false(milter_client_run(client, &error));
    gcut_assert_error(actual_error);

    actual_error = error;
    expected_error = g_error_new(MILTER_CONNECTION_ERROR,
                                 MILTER_CONNECTION_ERROR_BIND_FAILURE,
                                 "failed to bind(): %s: %s",
                                 spec, g_strerror(EADDRINUSE));
    gcut_assert_equal_error(expected_error, actual_error);
}

void
test_remove_pid_file_on_exit_accessor (void)
{
    cut_assert_true(milter_client_is_remove_pid_file_on_exit(client));
    milter_client_set_remove_pid_file_on_exit(client, FALSE);
    cut_assert_false(milter_client_is_remove_pid_file_on_exit(client));
}

static const gchar *
pid_file_path (void)
{
    return cut_take_printf("%s/milter.pid", tmp_dir);
}

static gboolean
cb_idle_check_pid_file_existence (gpointer user_data)
{
    gboolean *pid_file_exist = user_data;

    *pid_file_exist = g_file_test(pid_file_path(), G_FILE_TEST_EXISTS);

    return FALSE;
}

void
test_remove_pid_file_on_exit (void)
{
    gboolean pid_file_exist = FALSE;

    cut_trace(setup_client());
    milter_client_set_pid_file(client, pid_file_path());
    milter_event_loop_add_idle_full(loop,
                                    G_PRIORITY_HIGH,
                                    cb_idle_check_pid_file_existence,
                                    &pid_file_exist,
                                    NULL);
    cut_assert_true(milter_client_run(client, &actual_error));
    gcut_assert_error(actual_error);
    cut_assert_true(pid_file_exist);

    cut_assert_false(g_file_test(pid_file_path(), G_FILE_TEST_EXISTS));
}

void
test_syslog_identify_accessor (void)
{
    cut_assert_equal_string(NULL, milter_client_get_syslog_identify(client));
    milter_client_set_syslog_identify(client, "milter-test-client");
    cut_assert_equal_string("milter-test-client",
                            milter_client_get_syslog_identify(client));
}

void
test_syslog_facility_accessor (void)
{
    cut_assert_equal_string(NULL, milter_client_get_syslog_facility(client));
    milter_client_set_syslog_facility(client, "mail");
    cut_assert_equal_string("mail", milter_client_get_syslog_facility(client));
}

void
test_suspend_time_on_unacceptable (void)
{
    cut_assert_equal_uint(
        5, milter_client_get_suspend_time_on_unacceptable(client));
    milter_client_set_suspend_time_on_unacceptable(client, 1);
    cut_assert_equal_uint(
        1, milter_client_get_suspend_time_on_unacceptable(client));
}

void
test_connection_spec (void)
{
    GError *error = NULL;
    cut_assert_equal_string(NULL,
                            milter_client_get_connection_spec(client));
    milter_client_set_connection_spec(client, "inet:12345", &error);
    gcut_assert_error(error);
    cut_assert_equal_string("inet:12345",
                            milter_client_get_connection_spec(client));
}

void
test_max_connections (void)
{
    cut_assert_equal_uint(
        0, milter_client_get_max_connections(client));
    milter_client_set_max_connections(client, 10);
    cut_assert_equal_uint(
        10, milter_client_get_max_connections(client));
}

void
test_effective_user (void)
{
    cut_assert_equal_string(
        NULL, milter_client_get_effective_user(client));
    milter_client_set_effective_user(client, "nobody");
    cut_assert_equal_string(
        "nobody", milter_client_get_effective_user(client));
}

void
test_effective_group (void)
{
    cut_assert_equal_string(
        NULL, milter_client_get_effective_group(client));
    milter_client_set_effective_group(client, "nogroup");
    cut_assert_equal_string(
        "nogroup", milter_client_get_effective_group(client));
}

void
test_event_loop_backend (void)
{
    gcut_assert_equal_enum(
        MILTER_TYPE_CLIENT_EVENT_LOOP_BACKEND,
        MILTER_CLIENT_EVENT_LOOP_BACKEND_DEFAULT,
        milter_client_get_event_loop_backend(client));
    milter_client_set_event_loop_backend(client,
                                         MILTER_CLIENT_EVENT_LOOP_BACKEND_LIBEV);
    gcut_assert_equal_enum(
        MILTER_TYPE_CLIENT_EVENT_LOOP_BACKEND,
        MILTER_CLIENT_EVENT_LOOP_BACKEND_LIBEV,
        milter_client_get_event_loop_backend(client));
}

void
test_maintenance_interval (void)
{
    cut_assert_equal_uint(
        0, milter_client_get_maintenance_interval(client));
    milter_client_set_maintenance_interval(client, 10);
    cut_assert_equal_uint(
        10, milter_client_get_maintenance_interval(client));
}

void
test_need_maintain (void)
{
    milter_client_set_maintenance_interval(client, 10);
    milter_client_set_n_processing_sessions(client, 3);

    milter_client_set_n_processed_sessions(client, 10);
    cut_assert_false(milter_client_need_maintain(client, 0));
    cut_assert_true(milter_client_need_maintain(client, 1));

    milter_client_set_n_processed_sessions(client, 11);
    cut_assert_false(milter_client_need_maintain(client, 1));
    cut_assert_true(milter_client_need_maintain(client, 2));
}

void
test_need_maintain_over (void)
{
    milter_client_set_maintenance_interval(client, 10);
    milter_client_set_n_processing_sessions(client, 3);

    milter_client_set_n_processed_sessions(client, 21);
    cut_assert_false(milter_client_need_maintain(client, 1));
    cut_assert_true(milter_client_need_maintain(client, 2));
}

void
test_need_maintain_over_interval (void)
{
    milter_client_set_maintenance_interval(client, 2);
    milter_client_set_n_processing_sessions(client, 3);

    milter_client_set_n_processed_sessions(client, 4);
    cut_assert_true(milter_client_need_maintain(client, 3));
}

void
test_need_maintain_never (void)
{
    milter_client_set_maintenance_interval(client, 0);
    milter_client_set_n_processing_sessions(client, 3);

    cut_assert_false(milter_client_need_maintain(client, 0));
    cut_assert_false(milter_client_need_maintain(client, 1));

    milter_client_set_n_processed_sessions(client, 100);
    cut_assert_false(milter_client_need_maintain(client, 1));
}

void
test_need_maintain_no_processing_sessions (void)
{
    milter_client_set_maintenance_interval(client, 5);
    milter_client_set_n_processing_sessions(client, 0);
    cut_assert_false(milter_client_need_maintain(client, 0));
    cut_assert_true(milter_client_need_maintain(client, 1));
}

void
test_need_maintain_no_processing_sessions_below_processed_sessions (void)
{
    milter_client_set_maintenance_interval(client, 5);
    milter_client_set_n_processed_sessions(client, 3);
    cut_assert_false(milter_client_need_maintain(client, 0));
    cut_assert_true(milter_client_need_maintain(client, 1));
}

void
test_need_maintain_no_processing_sessions_no_interval (void)
{
    milter_client_set_maintenance_interval(client, 0);
    milter_client_set_n_processing_sessions(client, 0);
    cut_assert_false(milter_client_need_maintain(client, 0));
    cut_assert_true(milter_client_need_maintain(client, 1));
}

void
test_n_workers (void)
{
    cut_assert_equal_uint(
        n_workers, milter_client_get_n_workers(client));
    milter_client_set_n_workers(client, 10);
    cut_assert_equal_uint(
        10, milter_client_get_n_workers(client));
}

static GPid
worker_fork (MilterClient *loop)
{
    n_worker_fork_called++;
    return 29;
}

void
test_custom_fork (void)
{
    milter_client_set_custom_fork_func(client, worker_fork);
    cut_assert_equal_uint(29, milter_client_fork(client));
    cut_assert_equal_uint(1, n_worker_fork_called);
}

void
test_default_packet_buffer_size (void)
{
    MilterClientContext *context;

    cut_assert_equal_uint(0,
                          milter_client_get_default_packet_buffer_size(client));
    context = milter_client_create_context(client);
    gcut_take_object(G_OBJECT(context));
    cut_assert_equal_uint(0,
                          milter_client_context_get_packet_buffer_size(context));

    milter_client_set_default_packet_buffer_size(client, 4096);
    cut_assert_equal_uint(4096,
                          milter_client_get_default_packet_buffer_size(client));
    context = milter_client_create_context(client);
    gcut_take_object(G_OBJECT(context));
    cut_assert_equal_uint(4096,
                          milter_client_context_get_packet_buffer_size(context));
}

void
test_worker_id (void)
{
    cut_assert_equal_uint(0, milter_client_get_worker_id(client));
}

void
test_max_pending_finished_sessions (void)
{
    cut_assert_equal_uint(
        0, milter_client_get_max_pending_finished_sessions(client));
    milter_client_set_max_pending_finished_sessions(client, 29);
    cut_assert_equal_uint(
        29, milter_client_get_max_pending_finished_sessions(client));
}


/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
