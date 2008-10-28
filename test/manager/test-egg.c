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
#include <milter/manager/milter-manager-egg.h>
#undef shutdown

void test_new (void);
void test_hatch (void);
void test_name (void);
void test_connection_spec (void);
void test_connection_spec_error (void);
void test_connection_timeout (void);
void test_writing_timeout (void);
void test_reading_timeout (void);
void test_end_of_message_timeout (void);
void test_user_name (void);
void test_command (void);

static MilterManagerEgg *egg;
static MilterManagerChild *child;

static GError *expected_error;
static GError *actual_error;

static const gchar *milter_log_level;

void
setup (void)
{
    egg = NULL;
    child = NULL;

    expected_error = NULL;
    actual_error = NULL;

    milter_log_level = g_getenv("MILTER_LOG_LEVEL");
}

void
teardown (void)
{
    if (egg)
        g_object_unref(egg);
    if (child)
        g_object_unref(child);

    if (expected_error)
        g_error_free(expected_error);
    if (actual_error)
        g_error_free(actual_error);

    if (milter_log_level)
        g_setenv("MILTER_LOG_LEVEL", milter_log_level, TRUE);
}

void
test_new (void)
{
    const gchar name[] = "child-milter";

    egg = milter_manager_egg_new(name);
    cut_assert_equal_string(name, milter_manager_egg_get_name(egg));
    cut_assert_equal_uint(MILTER_SERVER_CONTEXT_DEFAULT_CONNECTION_TIMEOUT,
                          milter_manager_egg_get_connection_timeout(egg));
    cut_assert_equal_uint(MILTER_SERVER_CONTEXT_DEFAULT_WRITING_TIMEOUT,
                          milter_manager_egg_get_writing_timeout(egg));
    cut_assert_equal_uint(MILTER_SERVER_CONTEXT_DEFAULT_READING_TIMEOUT,
                          milter_manager_egg_get_reading_timeout(egg));
    cut_assert_equal_uint(MILTER_SERVER_CONTEXT_DEFAULT_END_OF_MESSAGE_TIMEOUT,
                          milter_manager_egg_get_end_of_message_timeout(egg));
}

void
test_hatch (void)
{
    const gchar spec[] = "inet:9999@127.0.0.1";
    GError *error = NULL;

    egg = milter_manager_egg_new("child-milter");

    milter_manager_egg_set_connection_spec(egg, spec, &error);
    gcut_assert_error(error);

    child = milter_manager_egg_hatch(egg);
    cut_assert_not_null(child);
    cut_assert_equal_string("child-milter",
                            milter_manager_child_get_name(child));
}

void
test_name (void)
{
    const gchar name[] = "child-milter";
    const gchar new_name[] = "new-child-milter";

    egg = milter_manager_egg_new(name);
    cut_assert_equal_string(name, milter_manager_egg_get_name(egg));
    milter_manager_egg_set_name(egg, new_name);
    cut_assert_equal_string(new_name, milter_manager_egg_get_name(egg));
}

void
test_connection_spec (void)
{
    const gchar spec[] = "inet:9999@127.0.0.1";
    GError *error = NULL;

    egg = milter_manager_egg_new("child-milter");

    milter_manager_egg_set_connection_spec(egg, spec, &error);
    gcut_assert_error(error);
    cut_assert_equal_string(spec, milter_manager_egg_get_connection_spec(egg));
}

void
test_connection_spec_error (void)
{
    const gchar spec[] = "unknown:9999@127.0.0.1";

    egg = milter_manager_egg_new("child-milter");

    expected_error = g_error_new(MILTER_MANAGER_EGG_ERROR,
                                 MILTER_MANAGER_EGG_ERROR_INVALID,
                                 "<child-milter>: invalid connection spec: "
                                 "milter-utils-error-quark:%d: "
                                 "protocol must be 'unix', 'inet' or 'inet6': "
                                 "<%s>: <unknown>",
                                 MILTER_UTILS_ERROR_INVALID_FORMAT,
                                 spec);
    g_setenv("MILTER_LOG_LEVEL", NULL, TRUE);
    milter_manager_egg_set_connection_spec(egg, spec, &actual_error);
    gcut_assert_equal_error(expected_error, actual_error);
}

void
test_connection_timeout (void)
{
    guint new_connection_timeout = 100;

    egg = milter_manager_egg_new("child-milter");

    milter_manager_egg_set_connection_timeout(egg, new_connection_timeout);
    cut_assert_equal_uint(new_connection_timeout,
                          milter_manager_egg_get_connection_timeout(egg));
}

void
test_writing_timeout (void)
{
    guint new_writing_timeout = 100;

    egg = milter_manager_egg_new("child-milter");

    milter_manager_egg_set_writing_timeout(egg, new_writing_timeout);
    cut_assert_equal_uint(new_writing_timeout,
                          milter_manager_egg_get_writing_timeout(egg));
}

void
test_reading_timeout (void)
{
    guint new_reading_timeout = 100;

    egg = milter_manager_egg_new("child-milter");

    milter_manager_egg_set_reading_timeout(egg, new_reading_timeout);
    cut_assert_equal_uint(new_reading_timeout,
                          milter_manager_egg_get_reading_timeout(egg));
}

void
test_end_of_message_timeout (void)
{
    guint new_end_of_message_timeout = 100;

    egg = milter_manager_egg_new("child-milter");

    milter_manager_egg_set_end_of_message_timeout(egg,
                                                    new_end_of_message_timeout);
    cut_assert_equal_uint(new_end_of_message_timeout,
                          milter_manager_egg_get_end_of_message_timeout(egg));
}

void
test_user_name (void)
{
    const gchar user_name[] = "milter-user";

    egg = milter_manager_egg_new("child-milter");
    cut_assert_equal_string(NULL,
                            milter_manager_egg_get_user_name(egg));
    milter_manager_egg_set_user_name(egg, user_name);
    cut_assert_equal_string(user_name,
                            milter_manager_egg_get_user_name(egg));
}

void
test_command (void)
{
    const gchar command[] = "/usr/bin/my-milter";

    egg = milter_manager_egg_new("child-milter");
    cut_assert_equal_string(NULL,
                            milter_manager_egg_get_command(egg));
    milter_manager_egg_set_command(egg, command);
    cut_assert_equal_string(command, milter_manager_egg_get_command(egg));
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
