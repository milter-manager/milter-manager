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
#include <milter/manager/milter-manager-spawn.h>
#undef shutdown

void test_new (void);
void test_name (void);
void test_connection_timeout (void);
void test_writing_timeout (void);
void test_reading_timeout (void);
void test_end_of_message_timeout (void);
void test_user_name (void);

static MilterManagerSpawn *spawn;

void
setup (void)
{
    spawn = NULL;
}

void
teardown (void)
{
    if (spawn)
        g_object_unref(spawn);
}

void
test_new (void)
{
    const gchar name[] = "child-milter";

    spawn = milter_manager_spawn_new(name);
    cut_assert_equal_string(name, milter_manager_spawn_get_name(spawn));
    cut_assert_equal_uint(0, milter_manager_spawn_get_connection_timeout(spawn));
    cut_assert_equal_uint(0, milter_manager_spawn_get_writing_timeout(spawn));
    cut_assert_equal_uint(0, milter_manager_spawn_get_reading_timeout(spawn));
    cut_assert_equal_uint(0, milter_manager_spawn_get_end_of_message_timeout(spawn));
}

void
test_name (void)
{
    const gchar name[] = "child-milter";
    const gchar new_name[] = "new-child-milter";

    spawn = milter_manager_spawn_new(name);
    cut_assert_equal_string(name, milter_manager_spawn_get_name(spawn));
    milter_manager_spawn_set_name(spawn, new_name);
    cut_assert_equal_string(new_name, milter_manager_spawn_get_name(spawn));
}

void
test_connection_timeout (void)
{
    guint new_connection_timeout = 100;

    spawn = milter_manager_spawn_new("child-milter");

    milter_manager_spawn_set_connection_timeout(spawn, new_connection_timeout);
    cut_assert_equal_uint(new_connection_timeout,
                          milter_manager_spawn_get_connection_timeout(spawn));
}

void
test_writing_timeout (void)
{
    guint new_writing_timeout = 100;

    spawn = milter_manager_spawn_new("child-milter");

    milter_manager_spawn_set_writing_timeout(spawn, new_writing_timeout);
    cut_assert_equal_uint(new_writing_timeout,
                          milter_manager_spawn_get_writing_timeout(spawn));
}

void
test_reading_timeout (void)
{
    guint new_reading_timeout = 100;

    spawn = milter_manager_spawn_new("child-milter");

    milter_manager_spawn_set_reading_timeout(spawn, new_reading_timeout);
    cut_assert_equal_uint(new_reading_timeout,
                          milter_manager_spawn_get_reading_timeout(spawn));
}

void
test_end_of_message_timeout (void)
{
    guint new_end_of_message_timeout = 100;

    spawn = milter_manager_spawn_new("child-milter");

    milter_manager_spawn_set_end_of_message_timeout(spawn,
                                                    new_end_of_message_timeout);
    cut_assert_equal_uint(new_end_of_message_timeout,
                          milter_manager_spawn_get_end_of_message_timeout(spawn));
}

void
test_user_name (void)
{
    const gchar user_name[] = "milter-user";

    spawn = milter_manager_spawn_new("child-milter");
    cut_assert_equal_string(NULL,
                            milter_manager_spawn_get_user_name(spawn));
    milter_manager_spawn_set_user_name(spawn, user_name);
    cut_assert_equal_string(user_name,
                            milter_manager_spawn_get_user_name(spawn));
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
