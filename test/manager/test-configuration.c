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
#include <milter/manager/milter-manager-children.h>
#undef shutdown

void test_children (void);

static MilterManagerConfiguration *config;
static MilterManagerEgg *egg;

static MilterManagerChildren *expected_children;
static MilterManagerChildren *actual_children;

void
setup (void)
{
    config = milter_manager_configuration_new(NULL);
    egg = NULL;

    expected_children = milter_manager_children_new(config);
    actual_children = NULL;
}

void
teardown (void)
{
    if (config)
        g_object_unref(config);
    if (egg)
        g_object_unref(egg);

    if (expected_children)
        g_object_unref(expected_children);
    if (actual_children)
        g_object_unref(actual_children);
}

static gboolean
child_equal (gconstpointer a, gconstpointer b)
{
    MilterServerContext *context1, *context2;

    context1 = MILTER_SERVER_CONTEXT(a);
    context2 = MILTER_SERVER_CONTEXT(b);

    /* FIXME */
    return g_str_equal(milter_server_context_get_name(context1),
                       milter_server_context_get_name(context2));
}

#define milter_assert_equal_children(expected, actual)             \
    gcut_assert_equal_list_object_custom(                          \
        milter_manager_children_get_children(expected),            \
        milter_manager_children_get_children(actual),              \
        child_equal)

void
test_children (void)
{
    MilterManagerChild *child;
    GError *error = NULL;

    egg = milter_manager_egg_new("child-milter");
    milter_manager_egg_set_connection_spec(egg, "inet:2929@localhost", &error);
    gcut_assert_error(error);

    milter_manager_configuration_add_egg(config, egg);

    child = milter_manager_egg_hatch(egg);
    milter_manager_children_add_child(expected_children, child);

    actual_children = milter_manager_configuration_create_children(config);
    milter_assert_equal_children(expected_children, actual_children);
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
