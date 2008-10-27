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

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
