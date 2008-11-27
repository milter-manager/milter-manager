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

#ifndef __MILTER_ASSERTIONS_H__
#define __MILTER_ASSERTIONS_H__

#include <gcutter.h>
#define shutdown inet_shutdown
#include <milter/core.h>
#undef inet_shutdown

G_BEGIN_DECLS

#define milter_assert_equal_option(expected, actual, ...)               \
    cut_trace_with_info_expression(                                     \
        milter_assert_equal_option_helper(expected, actual,             \
                                          #expected, #actual,           \
                                          ## __VA_ARGS__, NULL),        \
        milter_assert_equal_option(expected, actual, __VA_ARGS__))

void milter_assert_equal_option_helper (MilterOption *expected,
                                        MilterOption *actual,
                                        const gchar  *expression_expected,
                                        const gchar  *expression_actual,
                                        const gchar  *user_message_format,
                                        ...);

#define milter_assert_equal_list_string_with_sort(expected, actual, ...) \
    cut_trace_with_info_expression(                                     \
        milter_assert_equal_list_string_with_sort_helper(               \
            expected, actual,                                           \
            #expected, #actual,                                         \
            ## __VA_ARGS__, NULL),                                      \
        milter_assert_equal_list_string_with_sort(expected, actual,     \
                                                  __VA_ARGS__))

void milter_assert_equal_list_string_with_sort_helper
                                       (GList *expected, GList *actual,
                                        const gchar  *expression_expected,
                                        const gchar  *expression_actual,
                                        const gchar  *user_message_format,
                                        ...);

#define milter_assert_equal_symbols_table(expected, actual, ...)        \
    cut_trace_with_info_expression(                                     \
        milter_assert_equal_symbols_table_helper(                       \
            expected, actual,                                           \
            #expected, #actual,                                         \
            ## __VA_ARGS__, NULL),                                      \
        milter_assert_equal_symbols_table(expected, actual, __VA_ARGS__))

void milter_assert_equal_symbols_table_helper
                                       (GHashTable  *expected,
                                        GHashTable  *actual,
                                        const gchar *expression_expected,
                                        const gchar *expression_actual,
                                        const gchar *user_message_format,
                                        ...);

#define milter_assert_equal_macros_requests(expected, actual, ...)      \
    cut_trace_with_info_expression(                                     \
        milter_assert_equal_macros_requests_helper(                     \
            expected, actual,                                           \
            #expected, #actual,                                         \
            ## __VA_ARGS__, NULL),                                      \
        milter_assert_equal_macros_requests(expected, actual,           \
                                            __VA_ARGS__))

void milter_assert_equal_macros_requests_helper
                                       (MilterMacrosRequests *expected,
                                        MilterMacrosRequests *actual,
                                        const gchar *expression_expected,
                                        const gchar *expression_actual,
                                        const gchar *user_message_format,
                                        ...);

#define milter_assert_equal_header(expected, actual, ...)               \
    cut_trace_with_info_expression(                                     \
        milter_assert_equal_header_helper(                              \
            expected, actual,                                           \
            #expected, #actual,                                         \
            ## __VA_ARGS__, NULL),                                      \
        milter_assert_equal_header(expected, actual,                    \
                                            __VA_ARGS__))

void milter_assert_equal_header_helper
                                       (MilterHeader *expected,
                                        MilterHeader *actual,
                                        const gchar *expression_expected,
                                        const gchar *expression_actual,
                                        const gchar *user_message_format,
                                        ...);

G_END_DECLS

#endif /* __MILTER_ASSERTIONS_H__ */

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
