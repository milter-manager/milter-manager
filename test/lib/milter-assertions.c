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
 *  along with this library.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "milter-assertions.h"
#include "milter-test-utils.h"

#include <string.h>

void
milter_assert_equal_option_helper (MilterOption *expected,
                                   MilterOption *actual,
                                   const gchar  *expression_expected,
                                   const gchar  *expression_actual)
{
    if (gcut_object_equal(G_OBJECT(expected), G_OBJECT(actual),
                          (GEqualFunc)milter_option_equal)) {
        cut_test_pass();
    } else {
        gchar *inspected_expected, *inspected_actual;
        const gchar *message;

        inspected_expected = gcut_object_inspect(G_OBJECT(expected));
        inspected_actual = gcut_object_inspect(G_OBJECT(actual));
        message = cut_take_printf("<%s> == <%s>\n"
                                  "expected: <%s>\n"
                                  "  actual: <%s>",
                                  expression_expected, expression_actual,
                                  inspected_expected, inspected_actual);
        message = cut_append_diff(message,
                                  inspected_expected, inspected_actual);
        cut_test_fail(message);
    }
}

void
milter_assert_equal_list_string_with_sort_helper (GList *expected,
                                                  GList *actual,
                                                  const gchar  *expression_expected,
                                                  const gchar  *expression_actual)
{
    GList *sorted_expected, *sorted_actual;
    sorted_expected = g_list_sort(expected, (GCompareFunc)strcmp);
    sorted_actual = g_list_sort(actual, (GCompareFunc)strcmp);

    if (gcut_list_equal_string(sorted_expected, sorted_actual)) {
        cut_test_pass();
    } else {
        const gchar *message;
        const gchar *inspected_expected, *inspected_actual;

        inspected_expected = cut_take_string(gcut_list_inspect_string(sorted_expected));
        inspected_actual = cut_take_string(gcut_list_inspect_string(sorted_actual));

        message = cut_take_printf("<%s == %s>\n"
                "expected: <%s>\n"
                "  actual: <%s>",
                expression_expected, expression_actual,
                inspected_expected,
                inspected_actual);
        message = cut_append_diff(message, inspected_expected, inspected_actual);
        cut_test_fail(message);
    }
}

void
milter_assert_equal_symbols_table_helper (GHashTable *expected,
                                          GHashTable *actual,
                                          const gchar *expression_expected,
                                          const gchar *expression_actual)
{
    if (milter_test_equal_symbols_table(expected, actual)) {
        cut_test_pass();
    } else {
        gchar *inspected_expected, *inspected_actual;
        const gchar *message;

        inspected_expected = milter_test_inspect_symbols_table(expected);
        inspected_actual = milter_test_inspect_symbols_table(actual);
        message = cut_take_printf("<%s> == <%s>\n"
                                  "expected: <%s>\n"
                                  "  actual: <%s>",
                                  expression_expected, expression_actual,
                                  inspected_expected,
                                  inspected_actual);
        if (expected && actual)
            message = cut_append_diff(message,
                                      inspected_expected, inspected_actual);
        g_free(inspected_expected);
        g_free(inspected_actual);
        cut_test_fail(message);
    }
}

void
milter_assert_equal_macros_requests_helper (MilterMacrosRequests *expected,
                                            MilterMacrosRequests *actual,
                                            const gchar *expression_expected,
                                            const gchar *expression_actual)
{
    GHashTable *expected_symbols_table = NULL, *actual_symbols_table = NULL;

    if (expected)
        g_object_get(expected, "symbols-table", &expected_symbols_table, NULL);
    if (actual)
        g_object_get(actual, "symbols-table", &actual_symbols_table, NULL);
    milter_assert_equal_symbols_table(expected_symbols_table,
                                      actual_symbols_table);
}

void
milter_assert_equal_header_helper (MilterHeader *expected,
                                   MilterHeader *actual,
                                   const gchar *expression_expected,
                                   const gchar *expression_actual)
{
    cut_assert_equal_string(expected->name, actual->name);
    cut_assert_equal_string(expected->value, actual->value);
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
