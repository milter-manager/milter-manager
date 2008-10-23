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
#include <milter/manager/milter-manager-configuration.h>
#undef shutdown

void test_children (void);

static MilterManagerConfiguration *config;
static MilterManagerChild *milter;

static GList *expected_children;

void
setup (void)
{
    config = milter_manager_configuration_new(NULL);
    milter = NULL;
    expected_children = NULL;
}

void
teardown (void)
{
    if (config)
        g_object_unref(config);
    if (milter)
        g_object_unref(milter);

    if (expected_children) {
        g_list_foreach(expected_children, (GFunc)g_object_unref, NULL);
        g_list_free(expected_children);
    }
}

#define milter_assert_equal_children(expected)                     \
    gcut_assert_equal_list_object(                                      \
        expected,                                                       \
        milter_manager_configuration_get_children(config))

void
test_children (void)
{
    milter_assert_equal_children(NULL);

    milter = milter_manager_child_new("child-milter");
    milter_manager_configuration_add_child(config, milter);

    expected_children = g_list_append(NULL, milter);
    g_object_ref(milter);

    milter_assert_equal_children(expected_children);
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
