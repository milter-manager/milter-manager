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

void test_start (void);
void test_exit_error (void);

static MilterManagerChild *milter;
static GError *actual_error;
static GError *expected_error;
static gboolean error_occured;
static const gchar *milter_log_level;

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
}

void
test_start (void)
{
    GError *error = NULL;
    g_object_set(milter,
                 "command", "/bin/echo -n",
                 NULL);
    milter_manager_child_start(milter, &error);
    gcut_assert_error(error);
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
