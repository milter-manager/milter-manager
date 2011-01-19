/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Copyright (C) 2008-2011  Kouhei Sutou <kou@clear-code.com>
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

#include <milter/manager.h>

#include "milter-manager-test-utils.h"

static gboolean
cb_check_emitted (gpointer data)
{
    gboolean *emitted = data;

    *emitted = TRUE;
    return FALSE;
}

void
milter_manager_test_wait_signal (MilterEventLoop *loop, gboolean should_timeout)
{
    gboolean timeout_emitted = FALSE;
    gboolean idle_emitted = FALSE;
    guint timeout_id, idle_id;

    idle_id = milter_event_loop_add_idle_full(loop,
                                              G_PRIORITY_DEFAULT,
                                              cb_check_emitted, &idle_emitted,
                                              NULL);

    timeout_id = milter_event_loop_add_timeout(loop, 1,
                                               cb_check_emitted,
                                               &timeout_emitted);
    while (!timeout_emitted && !idle_emitted) {
        milter_event_loop_iterate(loop, TRUE);
    }

    milter_event_loop_remove(loop, idle_id);
    milter_event_loop_remove(loop, timeout_id);

    if (should_timeout)
        cut_assert_true(timeout_emitted, cut_message("should timeout"));
    else
        cut_assert_false(timeout_emitted, cut_message("timeout"));
}

void
milter_manager_test_pair_inspect (GString *string,
                                  gconstpointer data,
                                  gpointer user_data)
{
    const MilterManagerTestPair *pair = data;

    g_string_append_printf(string, "<<%s>=<%s>>", pair->name, pair->value);
}

static gboolean
string_equal (const gchar *string1, const gchar *string2)
{
    if (string1 == string2)
        return TRUE;

    if (string1 == NULL || string2 == NULL)
        return FALSE;

    return strcmp(string1, string2) == 0;
}

gboolean
milter_manager_test_pair_equal (gconstpointer data1, gconstpointer data2)
{
    const MilterManagerTestPair *pair1 = data1;
    const MilterManagerTestPair *pair2 = data2;

    return string_equal(pair1->name, pair2->name) &&
        string_equal(pair1->value, pair2->value);
}

static gint
compare_string (const gchar *string1, const gchar *string2)
{
    if (cut_equal_string(string1, string2))
        return 0;

    if (string1 == NULL)
        return -1;
    if (string2 == NULL)
        return 1;

    return strcmp(string1, string2);
}

gint
milter_manager_test_pair_compare (gconstpointer data1, gconstpointer data2)
{
    const MilterManagerTestPair *pair1 = data1;
    const MilterManagerTestPair *pair2 = data2;
    gint compare;

    compare = compare_string(pair1->name, pair2->name);
    if (compare == 0) {
        return compare_string(pair1->value, pair2->value);
    } else {
        return compare;
    }
}

MilterManagerTestPair *
milter_manager_test_pair_new (const gchar *name, const gchar *value)
{
    MilterManagerTestPair *pair;

    pair = g_new0(MilterManagerTestPair, 1);
    pair->name = g_strdup(name);
    pair->value = g_strdup(value);

    return pair;
}

void
milter_manager_test_pair_free (MilterManagerTestPair *pair)
{
    g_free(pair->name);
    g_free(pair->value);

    g_free(pair);
}

static void
macro_context_inspect (GString *string,
                       gconstpointer macro_context,
                       gpointer user_data)
{
    gchar *inspected_macro_context;

    inspected_macro_context = gcut_enum_inspect(MILTER_TYPE_COMMAND,
                                                GPOINTER_TO_INT(macro_context));
    g_string_append_printf(string, "<%s>", inspected_macro_context);
    g_free(inspected_macro_context);
}

void
milter_manager_test_defined_macros_inspect (GString *string,
                                            gconstpointer defined_macros,
                                            gpointer user_data)
{
    GHashTable *_defined_macros = (GHashTable *)defined_macros;
    gchar *inspected_defined_macros;

    inspected_defined_macros =
        gcut_hash_table_inspect(_defined_macros,
                                macro_context_inspect,
                                (GCutInspectFunction)gcut_hash_table_string_string_inspect,
                                NULL);
    g_string_append(string, inspected_defined_macros);
    g_free(inspected_defined_macros);
}

gboolean
milter_manager_test_defined_macros_equal (gconstpointer defined_macros1,
                                          gconstpointer defined_macros2)
{
    GHashTable *_defined_macros1 = (GHashTable *)defined_macros1;
    GHashTable *_defined_macros2 = (GHashTable *)defined_macros2;

    return gcut_hash_table_equal(_defined_macros1,
                                 _defined_macros2,
                                 (GEqualFunc)gcut_hash_table_string_equal);
}

gboolean
milter_manager_test_applicable_condition_equal (gconstpointer _condition1,
                                                gconstpointer _condition2)
{
    MilterManagerApplicableCondition *condition1, *condition2;
    const gchar *name1, *name2;
    const gchar *desc1, *desc2;

    condition1 = MILTER_MANAGER_APPLICABLE_CONDITION(_condition1);
    condition2 = MILTER_MANAGER_APPLICABLE_CONDITION(_condition2);

    name1 = milter_manager_applicable_condition_get_name(condition1);
    name2 = milter_manager_applicable_condition_get_name(condition2);
    if (name1 != name2) {
        if (name1 == NULL || name2 == NULL)
            return FALSE;
        if (!g_str_equal(name1, name2))
            return FALSE;
    }

    desc1 = milter_manager_applicable_condition_get_description(condition1);
    desc2 = milter_manager_applicable_condition_get_description(condition2);
    if (desc1 == desc2)
        return TRUE;
    if (desc1 == NULL || desc2 == NULL)
        return FALSE;
    return g_str_equal(desc1, desc2);
}

gboolean
milter_manager_test_egg_equal (gconstpointer _egg1,
                               gconstpointer _egg2)
{
    MilterManagerEgg *egg1, *egg2;

    if (_egg1 == _egg2)
        return TRUE;

    if (_egg1 == NULL || _egg2 == NULL)
        return FALSE;

    egg1 = MILTER_MANAGER_EGG(_egg1);
    egg2 = MILTER_MANAGER_EGG(_egg2);
    return g_str_equal(milter_manager_egg_get_name(egg1),
                       milter_manager_egg_get_name(egg2));
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
