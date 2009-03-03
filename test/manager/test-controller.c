/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Copyright (C) 2008-2009  Kouhei Sutou <kou@cozmixng.org>
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

#include <glib/gstdio.h>

#include <milter/manager/milter-manager-controller.h>
#include <milter/manager/milter-manager-enum-types.h>

#include <milter-test-utils.h>
#include <milter-manager-test-utils.h>

#include <gcutter.h>

void test_listen_no_spec (void);
void test_listen_with_remove_on_create (void);
void test_listen_without_remove_on_create (void);

static MilterManager *manager;
static MilterManagerConfiguration *config;
static MilterManagerController *controller;

static GError *expected_error;
static GError *actual_error;

static gchar *tmp_dir;
static gchar *socket_path;

void
setup (void)
{
    gchar *spec;

    config = milter_manager_configuration_new(NULL);
    manager = milter_manager_new(config);
    controller = milter_manager_controller_new(manager);

    expected_error = NULL;
    actual_error = NULL;

    tmp_dir = g_build_filename(milter_test_get_base_dir(),
                               "tmp",
                               NULL);
    if (g_mkdir_with_parents(tmp_dir, 0700) == -1)
        cut_assert_errno();
    milter_manager_configuration_prepend_load_path(config, tmp_dir);

    socket_path = g_build_filename(tmp_dir, "controller.sock", NULL);
    spec = g_strdup_printf("unix:%s", socket_path);
    milter_manager_configuration_set_controller_connection_spec(config, spec);
    g_free(spec);
}

void
teardown (void)
{
    if (config)
        g_object_unref(config);
    if (manager)
        g_object_unref(manager);
    if (controller)
        g_object_unref(controller);

    if (actual_error)
        g_error_free(actual_error);
    if (expected_error)
        g_error_free(expected_error);

    if (tmp_dir) {
        g_chmod(tmp_dir, 0700);
        cut_remove_path(tmp_dir, NULL);
        g_free(tmp_dir);
    }

    if (socket_path)
        g_free(socket_path);
}

void
test_listen_no_spec (void)
{
    milter_manager_configuration_set_controller_connection_spec(config, NULL);

    cut_assert_false(milter_manager_controller_listen(controller,
                                                      &actual_error));
    expected_error = g_error_new(MILTER_MANAGER_CONTROLLER_ERROR,
                                 MILTER_MANAGER_CONTROLLER_ERROR_NO_SPEC,
                                 "connection spec isn't specified");
    gcut_assert_equal_error(expected_error, actual_error);
}

void
test_listen_with_remove_on_create (void)
{
    GError *error = NULL;

    g_file_set_contents(socket_path, "", 0, &error);
    gcut_assert_error(error);

    milter_manager_configuration_set_remove_controller_unix_socket_on_create(config, TRUE);

    cut_assert_true(milter_manager_controller_listen(controller, &actual_error));
    gcut_assert_equal_error(NULL, actual_error);

    cut_assert_path_exist(socket_path);
}

void
test_listen_without_remove_on_create (void)
{
    GError *error = NULL;

    g_file_set_contents(socket_path, "", 0, &error);
    gcut_assert_error(error);

    milter_manager_configuration_set_remove_controller_unix_socket_on_create(config, FALSE);

    cut_assert_false(milter_manager_controller_listen(controller,
                                                      &actual_error));
    expected_error = g_error_new(MILTER_CONNECTION_ERROR,
                                 MILTER_CONNECTION_ERROR_BIND_FAILURE,
                                 "failed to bind(): unix:%s: %s",
                                 socket_path, g_strerror(EADDRINUSE));
    gcut_assert_equal_error(expected_error, actual_error);
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
