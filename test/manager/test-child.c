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

#define shutdown inet_shutdown
#include <milter-manager-test-utils.h>
#include <milter/manager/milter-manager-child.h>
#undef shutdown
#include <errno.h>
#include <locale.h>

void test_start (void);
void test_start_with_search_path (void);
void test_start_no_command (void);
void test_start_bad_command_string (void);
void test_start_by_user (void);
void test_start_no_privilege_mode (void);
void test_start_inexistent_command (void);
void test_start_by_inexistent_user (void);
void test_exit_error (void);

static MilterManagerChild *milter;
static GError *actual_error;
static GError *expected_error;
static gboolean error_occured;
static const gchar *milter_log_level;
static gchar *current_locale;

static void
cb_error (MilterErrorEmittable *emittable,
          GError *error, gpointer user_data)
{
    actual_error = g_error_copy(error);
    error_occured = TRUE;
}

void
setup (void)
{
    milter = milter_manager_child_new("test-milter");
    actual_error = NULL;
    expected_error = NULL;
    error_occured = FALSE;

    milter_log_level = g_getenv("MILTER_LOG_LEVEL");

    current_locale = g_strdup(setlocale(LC_ALL, NULL));
    setlocale(LC_ALL, "C");
}

void
teardown (void)
{
    if (milter)
        g_object_unref(milter);
    if (actual_error)
        g_error_free(actual_error);
    if (expected_error)
        g_error_free(expected_error);
    if (milter_log_level)
        g_setenv("MILTER_LOG_LEVEL", milter_log_level, TRUE);

    setlocale(LC_ALL, current_locale);
    if (current_locale)
        g_free(current_locale);
}

void
test_start (void)
{
    GError *error = NULL;
    g_object_set(milter,
                 "command", "/bin/echo",
                 "command-options", "-n",
                 NULL);
    milter_manager_child_start(milter, &error);
    gcut_assert_error(error);
}

void
test_start_with_search_path (void)
{
    GError *error = NULL;
    g_object_set(milter,
                 "command", "echo",
                 "command-options", "-n",
                 "search-path", TRUE,
                 NULL);
    milter_manager_child_start(milter, &error);
    gcut_assert_error(error);
}

void
test_start_no_command (void)
{
    expected_error = g_error_new(MILTER_MANAGER_CHILD_ERROR,
                                 MILTER_MANAGER_CHILD_ERROR_BAD_COMMAND_STRING,
                                 "No command set yet.");
    milter_manager_child_start(milter, &actual_error);
    gcut_assert_equal_error(expected_error, actual_error);
}

void
test_start_bad_command_string (void)
{
    expected_error = g_error_new(MILTER_MANAGER_CHILD_ERROR,
                                 MILTER_MANAGER_CHILD_ERROR_BAD_COMMAND_STRING,
                                 "Command string has invalid character(s).: "
                                 "%s:%d: %s (%s)",
                                 g_quark_to_string(g_shell_error_quark()),
                                 G_SHELL_ERROR_BAD_QUOTING,
                                 "Text ended before matching quote was found for \".",
                                 "The text was '/bin/echo \"-n'");
    g_object_set(milter,
                 "command", "/bin/echo",
                 "command-options", "\"-n",
                 NULL);
    milter_manager_child_start(milter, &actual_error);
    gcut_assert_equal_error(expected_error, actual_error);
}

void
test_start_inexistent_command (void)
{
    const gchar command[] = "bin/oecho";

    expected_error = g_error_new(MILTER_MANAGER_CHILD_ERROR,
                                 MILTER_MANAGER_CHILD_ERROR_START_FAILURE,
                                 "Couldn't start new milter process.: "
                                 "%s:%d: %s \"%s\" (%s)",
                                 g_quark_to_string(g_spawn_error_quark()),
                                 G_SPAWN_ERROR_NOENT,
                                 "Failed to execute child process",
                                 command,
                                 g_strerror(ENOENT));
    g_object_set(milter,
                 "command", command,
                 NULL);
    milter_manager_child_start(milter, &actual_error);
    gcut_assert_equal_error(expected_error, actual_error);
}


void
test_start_by_user (void)
{
    GError *error = NULL;
    g_object_set(milter,
                 "command", "/bin/echo",
                 "command-options", "-n",
                 "user-name", g_get_user_name(),
                 NULL);
    milter_manager_child_start(milter, &error);
    gcut_assert_error(error);
}

void
test_start_no_privilege_mode (void)
{
    expected_error = g_error_new(MILTER_MANAGER_CHILD_ERROR,
                                 MILTER_MANAGER_CHILD_ERROR_NO_PRIVILEGE_MODE,
                                 "MilterManager is not running on privilege mode.");
    g_object_set(milter,
                 "command", "/bin/echo",
                 "command-options", "-n",
                 "user-name", "root",
                 NULL);
    milter_manager_child_start(milter, &actual_error);
    gcut_assert_equal_error(expected_error, actual_error);
}

void
test_start_by_inexistent_user (void)
{
    const gchar inexistent_user[] = "Who are you?";

    expected_error = g_error_new(MILTER_MANAGER_CHILD_ERROR,
                                 MILTER_MANAGER_CHILD_ERROR_INVALID_USER_NAME,
                                 "No passwd entry for %s: %s",
                                 inexistent_user, g_strerror(0));
    g_object_set(milter,
                 "command", "/bin/echo",
                 "user-name", inexistent_user,
                 NULL);
    milter_manager_child_start(milter, &actual_error);
    gcut_assert_equal_error(expected_error, actual_error);
}

void
test_exit_error (void)
{
    expected_error = g_error_new(MILTER_MANAGER_CHILD_ERROR,
                                 MILTER_MANAGER_CHILD_ERROR_MILTER_EXIT,
                                 "test-milter exits with status: %d", 0);
    g_signal_connect(milter, "error", G_CALLBACK(cb_error), NULL);

    g_setenv("MILTER_LOG_LEVEL", NULL, TRUE);
    cut_trace(test_start());
    while (!error_occured)
        g_main_context_iteration(NULL, TRUE);

    gcut_assert_equal_error(expected_error, actual_error);
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
