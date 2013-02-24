/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Copyright (C) 2008-2013  Kouhei Sutou <kou@clear-code.com>
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

#include <milter/manager/milter-manager-configuration.h>
#include <milter/manager/milter-manager-children.h>

#include <milter-test-utils.h>
#include <milter-manager-test-utils.h>

#include <gcutter.h>

void test_children (void);
void test_privilege_mode (void);
void test_effective_user (void);
void test_effective_group (void);
void test_manager_unix_socket_mode (void);
void test_controller_unix_socket_mode (void);
void test_manager_unix_socket_group (void);
void test_controller_unix_socket_group (void);
void test_remove_manager_unix_socket_on_close (void);
void test_remove_controller_unix_socket_on_close (void);
void test_remove_manager_unix_socket_on_create (void);
void test_remove_controller_unix_socket_on_create (void);
void test_daemon (void);
void test_pid_file (void);
void test_maintenance_interval (void);
void test_suspend_time_on_unacceptable (void);
void test_max_connections (void);
void test_max_file_descriptors (void);
void test_custom_configuration_directory (void);
void test_controller_connection_spec (void);
void test_manager_connection_spec (void);
void test_fallback_status (void);
void test_fallback_status_at_disconnect (void);
void test_package_platform (void);
void test_package_options (void);
void test_event_loop_backend (void);
void test_connection_check_interval (void);
void test_location (void);
void test_n_workers (void);
void test_default_packet_buffer_size (void);
void test_prefix (void);
void test_use_syslog (void);
void test_syslog_facility (void);
void test_chunk_size (void);
void test_chunk_size_over (void);
void test_max_pending_finished_sessions (void);
void test_egg (void);
void test_find_egg (void);
void test_remove_egg (void);
void test_applicable_condition (void);
void test_find_applicable_condition (void);
void test_remove_applicable_condition (void);
void test_clear (void);
void test_load_paths (void);
void test_load_absolute_path (void);
void test_save_custom (void);
void test_to_xml_full (void);
void test_to_xml_signal (void);

static MilterManagerConfiguration *config;
static MilterEventLoop *loop;
static MilterManagerEgg *egg;
static MilterManagerEgg *another_egg;
static MilterManagerApplicableCondition *condition;

static MilterManagerChildren *expected_children;
static MilterManagerChildren *actual_children;

static GList *expected_load_paths;

static GList *expected_eggs;
static GList *expected_applicable_conditions;

static gchar *actual_xml;

static gchar *tmp_dir;

void
cut_setup (void)
{
    config = milter_manager_configuration_new(NULL);
    loop = milter_test_event_loop_new();

    egg = NULL;
    another_egg = NULL;
    condition = NULL;

    expected_children = milter_manager_children_new(config, loop);
    actual_children = NULL;

    expected_load_paths = NULL;

    expected_eggs = NULL;
    expected_applicable_conditions = NULL;

    actual_xml = NULL;

    tmp_dir = g_build_filename(milter_test_get_base_dir(),
                               "tmp",
                               NULL);
    cut_remove_path(tmp_dir, NULL);
    if (g_mkdir_with_parents(tmp_dir, 0700) == -1)
        cut_assert_errno();
}

void
cut_teardown (void)
{
    if (config)
        g_object_unref(config);
    if (loop)
        g_object_unref(loop);
    if (egg)
        g_object_unref(egg);
    if (another_egg)
        g_object_unref(another_egg);
    if (condition)
        g_object_unref(condition);

    if (expected_children)
        g_object_unref(expected_children);
    if (actual_children)
        g_object_unref(actual_children);

    if (expected_load_paths) {
        g_list_foreach(expected_load_paths, (GFunc)g_free, NULL);
        g_list_free(expected_load_paths);
    }

    if (expected_eggs)
        g_list_free(expected_eggs);
    if (expected_applicable_conditions)
        g_list_free(expected_applicable_conditions);

    if (actual_xml)
        g_free(actual_xml);

    if (tmp_dir) {
        cut_remove_path(tmp_dir, NULL);
        g_free(tmp_dir);
    }
}

static gboolean
child_equal (gconstpointer a, gconstpointer b)
{
    MilterServerContext *context1, *context2;

    context1 = MILTER_SERVER_CONTEXT(a);
    context2 = MILTER_SERVER_CONTEXT(b);

    return g_str_equal(milter_server_context_get_name(context1),
                       milter_server_context_get_name(context2));
}

#define milter_assert_equal_children(expected, actual)             \
    gcut_assert_equal_list_object_custom(                          \
        milter_manager_children_get_children(expected),            \
        milter_manager_children_get_children(actual),              \
        child_equal)

static void
cb_attach_to (MilterManagerApplicableCondition *condition,
              MilterManagerChild *child, MilterManagerChildren *children,
              MilterClientContext *client_context, gpointer user_data)
{
    gboolean *attached_to = user_data;

    *attached_to = TRUE;
}

void
test_children (void)
{
    MilterManagerChild *child;
    gboolean attached_to = FALSE;
    GError *error = NULL;

    egg = milter_manager_egg_new("child-milter");
    milter_manager_egg_set_connection_spec(egg, "inet:2929@localhost", &error);
    gcut_assert_error(error);

    condition = milter_manager_applicable_condition_new("S25R");
    milter_manager_egg_add_applicable_condition(egg, condition);
    g_signal_connect(condition, "attach-to",
                     G_CALLBACK(cb_attach_to), &attached_to);

    another_egg = milter_manager_egg_new("brother-milter");
    milter_manager_egg_set_connection_spec(another_egg,
                                           "inet:29292@localhost", &error);
    gcut_assert_error(error);
    milter_manager_egg_set_enabled(another_egg, FALSE);

    milter_manager_configuration_add_egg(config, egg);
    milter_manager_configuration_add_egg(config, another_egg);

    child = milter_manager_egg_hatch(egg);
    milter_manager_children_add_child(expected_children, child);
    g_object_unref(child);

    cut_assert_false(attached_to);
    if (actual_children)
        g_object_unref(actual_children);
    actual_children = milter_manager_children_new(config, loop);
    milter_manager_configuration_setup_children(config, actual_children, NULL);
    milter_assert_equal_children(expected_children, actual_children);
    cut_assert_true(attached_to);
}

void
test_privilege_mode (void)
{
    cut_assert_false(milter_manager_configuration_is_privilege_mode(config));
    milter_manager_configuration_set_privilege_mode(config, TRUE);
    cut_assert_true(milter_manager_configuration_is_privilege_mode(config));
}

void
test_effective_user (void)
{
    cut_assert_equal_string(
        NULL,
        milter_manager_configuration_get_effective_user(config));
    milter_manager_configuration_set_effective_user(config, "nobody");
    cut_assert_equal_string(
        "nobody",
        milter_manager_configuration_get_effective_user(config));
}

void
test_effective_group (void)
{
    cut_assert_equal_string(
        NULL,
        milter_manager_configuration_get_effective_group(config));
    milter_manager_configuration_set_effective_group(config, "nogroup");
    cut_assert_equal_string(
        "nogroup",
        milter_manager_configuration_get_effective_group(config));
}

void
test_manager_unix_socket_mode (void)
{
    cut_assert_equal_uint(
        0660,
        milter_manager_configuration_get_manager_unix_socket_mode(config));
    milter_manager_configuration_set_manager_unix_socket_mode(config, 0600);
    cut_assert_equal_uint(
        0600,
        milter_manager_configuration_get_manager_unix_socket_mode(config));
}

void
test_controller_unix_socket_mode (void)
{
    cut_assert_equal_uint(
        0660,
        milter_manager_configuration_get_controller_unix_socket_mode(config));
    milter_manager_configuration_set_controller_unix_socket_mode(config, 0600);
    cut_assert_equal_uint(
        0600,
        milter_manager_configuration_get_controller_unix_socket_mode(config));
}

void
test_manager_unix_socket_group (void)
{
    cut_assert_equal_string(
        NULL,
        milter_manager_configuration_get_manager_unix_socket_group(config));
    milter_manager_configuration_set_manager_unix_socket_group(config, "nobody");
    cut_assert_equal_string(
        "nobody",
        milter_manager_configuration_get_manager_unix_socket_group(config));
}

void
test_controller_unix_socket_group (void)
{
    cut_assert_equal_string(
        NULL,
        milter_manager_configuration_get_controller_unix_socket_group(config));
    milter_manager_configuration_set_controller_unix_socket_group(config, "nobody");
    cut_assert_equal_string(
        "nobody",
        milter_manager_configuration_get_controller_unix_socket_group(config));
}

void
test_remove_manager_unix_socket_on_close (void)
{
    cut_assert_true(milter_manager_configuration_is_remove_manager_unix_socket_on_close(config));
    milter_manager_configuration_set_remove_manager_unix_socket_on_close(config, FALSE);
    cut_assert_false(milter_manager_configuration_is_remove_manager_unix_socket_on_close(config));
}

void
test_remove_controller_unix_socket_on_close (void)
{
    cut_assert_true(milter_manager_configuration_is_remove_controller_unix_socket_on_close(config));
    milter_manager_configuration_set_remove_controller_unix_socket_on_close(config, FALSE);
    cut_assert_false(milter_manager_configuration_is_remove_controller_unix_socket_on_close(config));
}

void
test_remove_manager_unix_socket_on_create (void)
{
    cut_assert_true(milter_manager_configuration_is_remove_manager_unix_socket_on_create(config));
    milter_manager_configuration_set_remove_manager_unix_socket_on_create(config, FALSE);
    cut_assert_false(milter_manager_configuration_is_remove_manager_unix_socket_on_create(config));
}

void
test_remove_controller_unix_socket_on_create (void)
{
    cut_assert_true(milter_manager_configuration_is_remove_controller_unix_socket_on_create(config));
    milter_manager_configuration_set_remove_controller_unix_socket_on_create(config, FALSE);
    cut_assert_false(milter_manager_configuration_is_remove_controller_unix_socket_on_create(config));
}

void
test_daemon (void)
{
    cut_assert_false(milter_manager_configuration_is_daemon(config));
    milter_manager_configuration_set_daemon(config, TRUE);
    cut_assert_true(milter_manager_configuration_is_daemon(config));
}

void
test_pid_file (void)
{
    const gchar pid_file[] = "/var/run/milter-manager.pid";

    cut_assert_equal_string(NULL,
                            milter_manager_configuration_get_pid_file(config));
    milter_manager_configuration_set_pid_file(config, pid_file);
    cut_assert_equal_string(pid_file,
                            milter_manager_configuration_get_pid_file(config));
}

void
test_maintenance_interval (void)
{
    cut_assert_equal_uint(
        10,
        milter_manager_configuration_get_maintenance_interval(config));
    milter_manager_configuration_set_maintenance_interval(config, 100);
    cut_assert_equal_uint(
        100,
        milter_manager_configuration_get_maintenance_interval(config));
}

void
test_suspend_time_on_unacceptable (void)
{
    cut_assert_equal_uint(
        MILTER_CLIENT_DEFAULT_SUSPEND_TIME_ON_UNACCEPTABLE,
        milter_manager_configuration_get_suspend_time_on_unacceptable(config));
    milter_manager_configuration_set_suspend_time_on_unacceptable(config, 10);
    cut_assert_equal_uint(
        10,
        milter_manager_configuration_get_suspend_time_on_unacceptable(config));
}

void
test_max_connections (void)
{
    cut_assert_equal_uint(
        MILTER_CLIENT_DEFAULT_MAX_CONNECTIONS,
        milter_manager_configuration_get_max_connections(config));
    milter_manager_configuration_set_max_connections(config, 10);
    cut_assert_equal_uint(
        10,
        milter_manager_configuration_get_max_connections(config));
}

void
test_max_file_descriptors (void)
{
    cut_assert_equal_uint(
        0,
        milter_manager_configuration_get_max_file_descriptors(config));
    milter_manager_configuration_set_max_file_descriptors(config, 1024);
    cut_assert_equal_uint(
        1024,
        milter_manager_configuration_get_max_file_descriptors(config));
}

void
test_custom_configuration_directory (void)
{
    const gchar custom_configuration_directory[] = "/tmp/milter-manager/";

    cut_assert_equal_string(
        NULL,
        milter_manager_configuration_get_custom_configuration_directory(config));
    milter_manager_configuration_set_custom_configuration_directory(
        config, custom_configuration_directory);
    cut_assert_equal_string(
        custom_configuration_directory,
        milter_manager_configuration_get_custom_configuration_directory(config));
}

void
test_controller_connection_spec (void)
{
    const gchar spec[] = "inet:2929@localhost";
    const gchar *actual_spec;

    actual_spec =
        milter_manager_configuration_get_controller_connection_spec(config);
    cut_assert_equal_string(NULL, actual_spec);

    milter_manager_configuration_set_controller_connection_spec(config, spec);

    actual_spec =
        milter_manager_configuration_get_controller_connection_spec(config);
    cut_assert_equal_string(spec, actual_spec);
}

void
test_manager_connection_spec (void)
{
    const gchar spec[] = "inet:2929@localhost";
    const gchar *actual_spec;

    actual_spec =
        milter_manager_configuration_get_manager_connection_spec(config);
    cut_assert_equal_string("inet:10025@[127.0.0.1]", actual_spec);

    milter_manager_configuration_set_manager_connection_spec(config, spec);

    actual_spec =
        milter_manager_configuration_get_manager_connection_spec(config);
    cut_assert_equal_string(spec, actual_spec);
}

void
test_fallback_status (void)
{
    MilterStatus actual_status;

    actual_status = milter_manager_configuration_get_fallback_status(config);
    gcut_assert_equal_enum(MILTER_TYPE_STATUS,
                           MILTER_STATUS_ACCEPT, actual_status);

    milter_manager_configuration_set_fallback_status(config,
                                                     MILTER_STATUS_REJECT);

    actual_status = milter_manager_configuration_get_fallback_status(config);
    gcut_assert_equal_enum(MILTER_TYPE_STATUS,
                           MILTER_STATUS_REJECT, actual_status);
}

void
test_fallback_status_at_disconnect (void)
{
    MilterStatus actual_status;

    actual_status =
        milter_manager_configuration_get_fallback_status_at_disconnect(config);
    gcut_assert_equal_enum(MILTER_TYPE_STATUS,
                           MILTER_STATUS_TEMPORARY_FAILURE, actual_status);

    milter_manager_configuration_set_fallback_status_at_disconnect(
        config, MILTER_STATUS_REJECT);

    actual_status =
        milter_manager_configuration_get_fallback_status_at_disconnect(config);
    gcut_assert_equal_enum(MILTER_TYPE_STATUS,
                           MILTER_STATUS_REJECT, actual_status);
}

void
test_package_platform (void)
{
    const gchar *actual_package_platform;

    actual_package_platform =
        milter_manager_configuration_get_package_platform(config);
    cut_assert_equal_string(MILTER_MANAGER_PACKAGE_PLATFORM,
                            actual_package_platform);

    milter_manager_configuration_set_package_platform(config, "new-platform");

    actual_package_platform =
        milter_manager_configuration_get_package_platform(config);
    cut_assert_equal_string("new-platform",
                            actual_package_platform);
}

void
test_package_options (void)
{
    const gchar *actual_package_options;

    actual_package_options =
        milter_manager_configuration_get_package_options(config);
    cut_assert_equal_string(MILTER_MANAGER_PACKAGE_OPTIONS,
                            actual_package_options);

    milter_manager_configuration_set_package_options(config, "prefix=/etc");

    actual_package_options =
        milter_manager_configuration_get_package_options(config);
    cut_assert_equal_string("prefix=/etc",
                            actual_package_options);
}

void
test_event_loop_backend (void)
{
    MilterClientEventLoopBackend backend;

    gcut_assert_equal_enum(
        MILTER_TYPE_CLIENT_EVENT_LOOP_BACKEND,
        MILTER_CLIENT_EVENT_LOOP_BACKEND_GLIB,
        milter_manager_configuration_get_event_loop_backend(config));

    backend = MILTER_CLIENT_EVENT_LOOP_BACKEND_LIBEV;
    milter_manager_configuration_set_event_loop_backend(config, backend);
    gcut_assert_equal_enum(
        MILTER_TYPE_CLIENT_EVENT_LOOP_BACKEND,
        backend,
        milter_manager_configuration_get_event_loop_backend(config));
}

void
test_connection_check_interval (void)
{
    cut_assert_equal_uint(
        0,
        milter_manager_configuration_get_connection_check_interval(config));
    milter_manager_configuration_set_connection_check_interval(config, 5);
    cut_assert_equal_uint(
        5,
        milter_manager_configuration_get_connection_check_interval(config));
}

void
test_n_workers (void)
{
    cut_assert_equal_uint(
        0,
        milter_manager_configuration_get_n_workers(config));
    milter_manager_configuration_set_n_workers(config, 5);
    cut_assert_equal_uint(
        5,
        milter_manager_configuration_get_n_workers(config));
}

void
test_default_packet_buffer_size (void)
{
    cut_assert_equal_uint(
        0,
        milter_manager_configuration_get_default_packet_buffer_size(config));
    milter_manager_configuration_set_default_packet_buffer_size(config, 4096);
    cut_assert_equal_uint(
        4096,
        milter_manager_configuration_get_default_packet_buffer_size(config));
}

void
test_prefix (void)
{
    cut_assert_equal_string(PREFIX,
                            milter_manager_configuration_get_prefix(config));
}

void
test_use_syslog (void)
{
    cut_assert_equal_boolean(
        TRUE,
        milter_manager_configuration_get_use_syslog(config));
    milter_manager_configuration_set_use_syslog(config, FALSE);
    cut_assert_equal_boolean(
        FALSE,
        milter_manager_configuration_get_use_syslog(config));
}

void
test_syslog_facility (void)
{
    cut_assert_equal_string(
        NULL,
        milter_manager_configuration_get_syslog_facility(config));
    milter_manager_configuration_set_syslog_facility(config, "local1");
    cut_assert_equal_string(
        "local1",
        milter_manager_configuration_get_syslog_facility(config));
}

void
test_chunk_size (void)
{
    cut_assert_equal_uint(
        MILTER_CHUNK_SIZE,
        milter_manager_configuration_get_chunk_size(config));
    milter_manager_configuration_set_chunk_size(config, 29);
    cut_assert_equal_uint(
        29,
        milter_manager_configuration_get_chunk_size(config));
}

void
test_chunk_size_over (void)
{
    milter_manager_configuration_set_chunk_size(config, MILTER_CHUNK_SIZE + 1);
    cut_assert_equal_uint(
        MILTER_CHUNK_SIZE,
        milter_manager_configuration_get_chunk_size(config));
}

void
test_max_pending_finished_sessions (void)
{
    cut_assert_equal_uint(
        0,
        milter_manager_configuration_get_max_pending_finished_sessions(config));
    milter_manager_configuration_set_max_pending_finished_sessions(config, 29);
    cut_assert_equal_uint(
        29,
        milter_manager_configuration_get_max_pending_finished_sessions(config));
}

static void
milter_assert_default_configuration_helper (MilterManagerConfiguration *config)
{
    cut_assert_false(milter_manager_configuration_is_privilege_mode(config));

    cut_assert_equal_string(
        "inet:10025@[127.0.0.1]",
        milter_manager_configuration_get_manager_connection_spec(config));
    cut_assert_equal_string(
        NULL,
        milter_manager_configuration_get_controller_connection_spec(config));
    cut_assert_equal_string(
        NULL,
        milter_manager_configuration_get_effective_user(config));
    cut_assert_equal_string(
        NULL,
        milter_manager_configuration_get_effective_group(config));
    cut_assert_equal_string(
        NULL,
        milter_manager_configuration_get_pid_file(config));
    cut_assert_equal_string(
        NULL,
        milter_manager_configuration_get_custom_configuration_directory(config));

    cut_assert_equal_uint(
        0660,
        milter_manager_configuration_get_manager_unix_socket_mode(config));
    cut_assert_equal_string(
        NULL,
        milter_manager_configuration_get_manager_unix_socket_group(config));
    cut_assert_equal_uint(
        0660,
        milter_manager_configuration_get_controller_unix_socket_mode(config));
    cut_assert_equal_string(
        NULL,
        milter_manager_configuration_get_controller_unix_socket_group(config));

    cut_assert_true(milter_manager_configuration_is_remove_manager_unix_socket_on_close(config));
    cut_assert_true(milter_manager_configuration_is_remove_controller_unix_socket_on_close(config));
    cut_assert_true(milter_manager_configuration_is_remove_manager_unix_socket_on_create(config));
    cut_assert_true(milter_manager_configuration_is_remove_controller_unix_socket_on_create(config));
    cut_assert_false(milter_manager_configuration_is_daemon(config));

    cut_assert_equal_uint(
        10,
        milter_manager_configuration_get_maintenance_interval(config));
    cut_assert_equal_uint(
        MILTER_CLIENT_DEFAULT_SUSPEND_TIME_ON_UNACCEPTABLE,
        milter_manager_configuration_get_suspend_time_on_unacceptable(config));
    cut_assert_equal_uint(
        MILTER_CLIENT_DEFAULT_MAX_CONNECTIONS,
        milter_manager_configuration_get_max_connections(config));
    cut_assert_equal_uint(
        0,
        milter_manager_configuration_get_max_file_descriptors(config));

    gcut_assert_equal_enum(
        MILTER_TYPE_STATUS,
        MILTER_STATUS_ACCEPT,
        milter_manager_configuration_get_fallback_status(config));

    cut_assert_equal_string(
        MILTER_MANAGER_PACKAGE_PLATFORM,
        milter_manager_configuration_get_package_platform(config));
    cut_assert_equal_string(
        MILTER_MANAGER_PACKAGE_OPTIONS,
        milter_manager_configuration_get_package_options(config));

    cut_assert_equal_uint(
        0,
        milter_manager_configuration_get_connection_check_interval(config));

    gcut_assert_equal_enum(
        MILTER_TYPE_CLIENT_EVENT_LOOP_BACKEND,
        MILTER_CLIENT_EVENT_LOOP_BACKEND_GLIB,
        milter_manager_configuration_get_event_loop_backend(config));
    cut_assert_equal_uint(
        0,
        milter_manager_configuration_get_n_workers(config));

    cut_assert_equal_uint(
        0,
        milter_manager_configuration_get_default_packet_buffer_size(config));

    cut_assert_equal_boolean(
        TRUE,
        milter_manager_configuration_get_use_syslog(config));
    cut_assert_equal_string(
        NULL,
        milter_manager_configuration_get_syslog_facility(config));

    cut_assert_equal_uint(
        MILTER_CHUNK_SIZE,
        milter_manager_configuration_get_chunk_size(config));

    cut_assert_equal_uint(
        0,
        milter_manager_configuration_get_max_pending_finished_sessions(config));

    if (expected_children)
        g_object_unref(expected_children);
    expected_children = milter_manager_children_new(config, loop);
    if (actual_children)
        g_object_unref(actual_children);
    actual_children = milter_manager_children_new(config, loop);
    milter_manager_configuration_setup_children(config, actual_children, NULL);
    milter_assert_equal_children(expected_children, actual_children);
    g_object_unref(actual_children);
    actual_children = NULL;
}

void
test_egg (void)
{
    GError *error = NULL;

    gcut_assert_equal_list_object_custom(
        NULL,
        milter_manager_configuration_get_eggs(config),
        milter_manager_test_egg_equal);

    egg = milter_manager_egg_new("child-milter");
    milter_manager_egg_set_connection_spec(egg, "inet:2929@localhost", &error);
    gcut_assert_error(error);

    milter_manager_configuration_add_egg(config, egg);
    expected_eggs = g_list_append(expected_eggs, egg);
    gcut_assert_equal_list_object_custom(
        expected_eggs,
        milter_manager_configuration_get_eggs(config),
        milter_manager_test_egg_equal);

    milter_manager_configuration_clear_eggs(config);
    gcut_assert_equal_list_object_custom(
        NULL,
        milter_manager_configuration_get_eggs(config),
        milter_manager_test_egg_equal);
}

void
test_find_egg (void)
{
    MilterManagerEgg *egg1, *egg2;

    egg1 = milter_manager_egg_new("milter1");
    egg2 = milter_manager_egg_new("milter2");
    milter_manager_configuration_add_egg(config, egg1);
    milter_manager_configuration_add_egg(config, egg2);

    gcut_take_object(G_OBJECT(egg1));
    gcut_take_object(G_OBJECT(egg2));

    gcut_assert_equal_object(
        egg1,
        milter_manager_configuration_find_egg(config, "milter1"));
    gcut_assert_equal_object(
        NULL,
        milter_manager_configuration_find_egg(config, "nonexistent"));
}

void
test_remove_egg (void)
{
    MilterManagerEgg *egg1, *egg2;

    egg1 = milter_manager_egg_new("milter1");
    egg2 = milter_manager_egg_new("milter2");
    milter_manager_configuration_add_egg(config, egg1);
    milter_manager_configuration_add_egg(config, egg2);

    gcut_take_object(G_OBJECT(egg1));
    gcut_take_object(G_OBJECT(egg2));

    expected_eggs = g_list_append(expected_eggs, egg1);
    expected_eggs = g_list_append(expected_eggs, egg2);
    gcut_assert_equal_list_object_custom(
        expected_eggs,
        milter_manager_configuration_get_eggs(config),
        milter_manager_test_egg_equal);

    milter_manager_configuration_remove_egg_by_name(config, "milter1");
    g_list_free(expected_applicable_conditions);
    expected_eggs = g_list_append(NULL, egg2);
    gcut_assert_equal_list_object_custom(
        expected_eggs,
        milter_manager_configuration_get_eggs(config),
        milter_manager_test_egg_equal);

    milter_manager_configuration_remove_egg(config, egg2);
    gcut_assert_equal_list_object_custom(
        NULL,
        milter_manager_configuration_get_eggs(config),
        milter_manager_test_egg_equal);
}

void
test_applicable_condition (void)
{
    gcut_assert_equal_list_object_custom(
        NULL,
        milter_manager_configuration_get_applicable_conditions(config),
        milter_manager_test_applicable_condition_equal);

    condition = milter_manager_applicable_condition_new("S25R");
    expected_applicable_conditions =
        g_list_append(expected_applicable_conditions, condition);
    milter_manager_configuration_add_applicable_condition(config, condition);
    gcut_assert_equal_list_object_custom(
        expected_applicable_conditions,
        milter_manager_configuration_get_applicable_conditions(config),
        milter_manager_test_applicable_condition_equal);

    milter_manager_configuration_clear_applicable_conditions(config);
    gcut_assert_equal_list_object_custom(
        NULL,
        milter_manager_configuration_get_applicable_conditions(config),
        milter_manager_test_applicable_condition_equal);
}

void
test_find_applicable_condition (void)
{
    MilterManagerApplicableCondition *s25r, *remote_network;

    s25r = milter_manager_applicable_condition_new("S25R");
    remote_network = milter_manager_applicable_condition_new("Remote Network");
    milter_manager_configuration_add_applicable_condition(config, s25r);
    milter_manager_configuration_add_applicable_condition(config,
                                                          remote_network);
    gcut_take_object(G_OBJECT(s25r));
    gcut_take_object(G_OBJECT(remote_network));

    gcut_assert_equal_object(
        s25r,
        milter_manager_configuration_find_applicable_condition(config, "S25R"));
    gcut_assert_equal_object(
        NULL,
        milter_manager_configuration_find_applicable_condition(config,
                                                               "nonexistent"));
}

void
test_remove_applicable_condition (void)
{
    MilterManagerApplicableCondition *s25r, *remote_network;

    s25r = milter_manager_applicable_condition_new("S25R");
    remote_network = milter_manager_applicable_condition_new("Remote Network");
    milter_manager_configuration_add_applicable_condition(config, s25r);
    milter_manager_configuration_add_applicable_condition(config,
                                                          remote_network);
    gcut_take_object(G_OBJECT(s25r));
    gcut_take_object(G_OBJECT(remote_network));

    expected_applicable_conditions =
        g_list_append(expected_applicable_conditions, s25r);
    expected_applicable_conditions =
        g_list_append(expected_applicable_conditions, remote_network);
    gcut_assert_equal_list_object_custom(
        expected_applicable_conditions,
        milter_manager_configuration_get_applicable_conditions(config),
        milter_manager_test_applicable_condition_equal);

    milter_manager_configuration_remove_applicable_condition_by_name(config,
                                                                     "S25R");
    g_list_free(expected_applicable_conditions);
    expected_applicable_conditions = g_list_append(NULL, remote_network);
    gcut_assert_equal_list_object_custom(
        expected_applicable_conditions,
        milter_manager_configuration_get_applicable_conditions(config),
        milter_manager_test_applicable_condition_equal);

    milter_manager_configuration_remove_applicable_condition(config,
                                                             remote_network);
    gcut_assert_equal_list_object_custom(
        NULL,
        milter_manager_configuration_get_applicable_conditions(config),
        milter_manager_test_applicable_condition_equal);
}

#define milter_assert_default_configuration(config)             \
    cut_trace_with_info_expression(                             \
        milter_assert_default_configuration_helper(config),     \
        milter_assert_default_configuration(config))

static gboolean
cb_connected (MilterManagerConfiguration *configuration,
              MilterManagerLeader *leader,
              gpointer user_data)
{
    return TRUE;
}

void
test_clear (void)
{
    gulong handler_id;
    GError *error = NULL;

    milter_assert_default_configuration(config);

    test_privilege_mode();
    test_effective_user();
    test_effective_group();
    test_manager_unix_socket_group();
    test_controller_unix_socket_group();
    test_manager_connection_spec();
    test_controller_connection_spec();
    test_fallback_status();
    test_children();
    test_daemon();
    test_maintenance_interval();
    test_suspend_time_on_unacceptable();
    test_max_connections();
    test_max_file_descriptors();
    test_event_loop_backend();
    test_connection_check_interval();
    test_n_workers();
    test_default_packet_buffer_size();
    test_use_syslog();
    test_syslog_facility();
    test_chunk_size();
    test_max_pending_finished_sessions();

    handler_id = g_signal_connect(config, "connected",
                                  G_CALLBACK(cb_connected), NULL);
    cut_assert_true(g_signal_handler_is_connected(config, handler_id));

    milter_manager_configuration_clear(config, &error);
    gcut_assert_error(error);
    milter_assert_default_configuration(config);

    cut_assert_false(g_signal_handler_is_connected(config, handler_id));
}

void
test_load_paths (void)
{
    const gchar *config_dir;

    config_dir = g_getenv("MILTER_MANAGER_CONFIG_DIR");
    if (config_dir)
        expected_load_paths = g_list_append(expected_load_paths,
                                            g_strdup(config_dir));
    expected_load_paths = g_list_append(expected_load_paths,
                                        g_strdup(CONFIG_DIR));
    gcut_assert_equal_list_string(
        expected_load_paths,
        milter_manager_configuration_get_load_paths(config));

    expected_load_paths = g_list_append(expected_load_paths,
                                        g_strdup("append/XXX"));
    milter_manager_configuration_append_load_path(config, "append/XXX");
    gcut_assert_equal_list_string(
        expected_load_paths,
        milter_manager_configuration_get_load_paths(config));

    expected_load_paths = g_list_prepend(expected_load_paths,
                                         g_strdup("prepend/XXX"));
    milter_manager_configuration_prepend_load_path(config, "prepend/XXX");
    gcut_assert_equal_list_string(
        expected_load_paths,
        milter_manager_configuration_get_load_paths(config));


    milter_manager_configuration_clear_load_paths(config);
    gcut_assert_equal_list_string(
        NULL,
        milter_manager_configuration_get_load_paths(config));
}

void
test_load_absolute_path (void)
{
    const gchar *config_file;
    GError *error = NULL;

    config_file = cut_build_path(tmp_dir, "temporary.conf", NULL);
    g_file_set_contents(config_file, "", 0, &error);
    gcut_assert_error(error);

    milter_manager_configuration_load(config, config_file, &error);
    gcut_assert_error(error);
}

void
test_save_custom (void)
{
    GError *error = NULL;
    gchar *custom_config_path;

    custom_config_path = g_build_filename(tmp_dir,
                                          CUSTOM_CONFIG_FILE_NAME,
                                          NULL);
    cut_take_string(custom_config_path);

    milter_manager_configuration_prepend_load_path(config, tmp_dir);

    cut_assert_path_not_exist(custom_config_path);
    milter_manager_configuration_save_custom(config, "XXX", -1, &error);
    gcut_assert_error(error);
    cut_assert_path_exist(custom_config_path);
}

void
test_to_xml_full (void)
{
    GError *error = NULL;

    egg = milter_manager_egg_new("milter@10025");
    milter_manager_egg_set_connection_spec(egg, "inet:10025", &error);
    gcut_assert_error(error);

    milter_manager_egg_set_command(egg, "test-milter");

    milter_manager_configuration_add_egg(config, egg);


    condition = milter_manager_applicable_condition_new("S25R");
    milter_manager_applicable_condition_set_description(
        condition, "Selective SMTP Rejection");
    milter_manager_configuration_add_applicable_condition(config, condition);

    actual_xml = milter_manager_configuration_to_xml(config);
    cut_assert_equal_string(
        "<configuration>\n"
        "  <applicable-conditions>\n"
        "    <applicable-condition>\n"
        "      <name>S25R</name>\n"
        "      <description>Selective SMTP Rejection</description>\n"
        "    </applicable-condition>\n"
        "  </applicable-conditions>\n"
        "  <milters>\n"
        "    <milter>\n"
        "      <name>milter@10025</name>\n"
        "      <enabled>true</enabled>\n"
        "      <fallback-status>accept</fallback-status>\n"
        "      <evaluation-mode>false</evaluation-mode>\n"
        "      <connection-spec>inet:10025</connection-spec>\n"
        "      <command>test-milter</command>\n"
        "    </milter>\n"
        "  </milters>\n"
        "</configuration>\n",
        actual_xml);
}

static void
cb_to_xml (MilterManagerConfiguration *configuration,
           GString *xml, guint indent, gpointer user_data)
{
    milter_utils_append_indent(xml, indent);
    g_string_append_printf(xml, "<additional-field>VALUE</additional-field>\n");
}

void
test_to_xml_signal (void)
{
    GError *error = NULL;

    egg = milter_manager_egg_new("milter@10025");
    milter_manager_egg_set_connection_spec(egg, "inet:10025", &error);
    gcut_assert_error(error);

    milter_manager_egg_set_command(egg, "test-milter");
    milter_manager_egg_set_command_options(egg, "--user nobody");

    milter_manager_configuration_add_egg(config, egg);

    actual_xml = milter_manager_configuration_to_xml(config);
    cut_assert_equal_string(
        "<configuration>\n"
        "  <milters>\n"
        "    <milter>\n"
        "      <name>milter@10025</name>\n"
        "      <enabled>true</enabled>\n"
        "      <fallback-status>accept</fallback-status>\n"
        "      <evaluation-mode>false</evaluation-mode>\n"
        "      <connection-spec>inet:10025</connection-spec>\n"
        "      <command>test-milter</command>\n"
        "      <command-options>--user nobody</command-options>\n"
        "    </milter>\n"
        "  </milters>\n"
        "</configuration>\n",
        actual_xml);

    g_signal_connect(config, "to-xml", G_CALLBACK(cb_to_xml), NULL);
    g_free(actual_xml);
    actual_xml = milter_manager_configuration_to_xml(config);
    cut_assert_equal_string(
        "<configuration>\n"
        "  <milters>\n"
        "    <milter>\n"
        "      <name>milter@10025</name>\n"
        "      <enabled>true</enabled>\n"
        "      <fallback-status>accept</fallback-status>\n"
        "      <evaluation-mode>false</evaluation-mode>\n"
        "      <connection-spec>inet:10025</connection-spec>\n"
        "      <command>test-milter</command>\n"
        "      <command-options>--user nobody</command-options>\n"
        "    </milter>\n"
        "  </milters>\n"
        "  <additional-field>VALUE</additional-field>\n"
        "</configuration>\n",
        actual_xml);
}

#define milter_assert_equal_location_keys(expected)             \
    cut_trace_with_info_expression(                             \
        milter_assert_equal_location_keys_helper(expected),     \
        milter_assert_equal_location_keys(expected))

static void
milter_assert_equal_location_keys_helper (const GList *expected)
{
    GHashTable *locations;
    GList *keys;

    locations = milter_manager_configuration_get_locations(config);
    keys = g_list_sort(g_hash_table_get_keys(locations), (GCompareFunc)strcmp);
    gcut_assert_equal_list_string(expected, gcut_take_list(keys, NULL));
}

void
test_location (void)
{
    const gchar key[] = "package.platform";
    gconstpointer location;

    milter_assert_equal_location_keys(NULL);

    milter_manager_configuration_set_location(config, key,
                                              "milter-manager.local.conf", 29);
    milter_assert_equal_location_keys(gcut_take_new_list_string(key, NULL));
    location = milter_manager_configuration_get_location(config, key);
    cut_assert_equal_string("milter-manager.local.conf",
                            g_dataset_get_data(location, "file"));
    cut_assert_equal_int(29,
                         GPOINTER_TO_INT(g_dataset_get_data(location, "line")));

    milter_manager_configuration_reset_location(config, "package.platform");
    milter_assert_equal_location_keys(NULL);
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
