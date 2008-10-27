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
#include <milter/manager/milter-manager-children.h>
#undef shutdown

void test_foreach (void);

static MilterManagerConfiguration *config;
static MilterManagerChildren *children;
static GError *actual_error;
static GError *expected_error;
static GList *actual_names;
static GList *expected_names;

void
setup (void)
{
    config = milter_manager_configuration_new(NULL);
    children = milter_manager_children_new(config);
    actual_names = NULL;
    expected_names = NULL;
    actual_error = NULL;
    expected_error = NULL;
}

void
teardown (void)
{
    if (config)
        g_object_unref(config);
    if (children)
        g_object_unref(children);
    if (actual_error)
        g_error_free(actual_error);
    if (expected_error)
        g_error_free(expected_error);
    if (actual_names)
        gcut_list_string_free(actual_names);
    if (expected_names)
        gcut_list_string_free(expected_names);
}

static void
each_child (MilterManagerChild *child, gpointer user_data)
{
    gchar *name = NULL;
    g_object_get(child,
                 "name", &name,
                 NULL);

    actual_names = g_list_append(actual_names, name);
}

void
test_foreach (void)
{
    expected_names = g_list_prepend(expected_names, g_strdup("1"));
    expected_names = g_list_prepend(expected_names, g_strdup("2"));
    expected_names = g_list_prepend(expected_names, g_strdup("3"));

    milter_manager_children_add_child(children,
                                      milter_manager_child_new("1"));
    milter_manager_children_add_child(children,
                                      milter_manager_child_new("2"));
    milter_manager_children_add_child(children,
                                      milter_manager_child_new("3"));
    milter_manager_children_foreach(children, 
                                    (GFunc)each_child, NULL);

    gcut_assert_equal_list_string(expected_names, actual_names);
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
