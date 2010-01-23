/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Copyright (C) 2010  Kouhei Sutou <kou@clear-code.com>
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

#include <milter/core/milter-session-result.h>
#include <milter-test-utils.h>

#include <gcutter.h>

void test_recipients (void);
void test_start_stop (void);

static MilterSessionResult *result;
static MilterMessageResult *message_result1;
static MilterMessageResult *message_result2;
static MilterMessageResult *message_result3;
static MilterMessageResult *message_result4;
static MilterMessageResult *message_result5;

void
cut_setup (void)
{
    result = milter_session_result_new();
    message_result1 = milter_message_result_new();
    message_result2 = milter_message_result_new();
    message_result3 = milter_message_result_new();
    message_result4 = milter_message_result_new();
    message_result5 = milter_message_result_new();
}

void
cut_teardown (void)
{
    if (result)
        g_object_unref(result);

    if (message_result1)
        g_object_unref(message_result1);
    if (message_result2)
        g_object_unref(message_result2);
    if (message_result3)
        g_object_unref(message_result3);
    if (message_result4)
        g_object_unref(message_result4);
    if (message_result5)
        g_object_unref(message_result5);
}

static const GList *
take_new_list (gpointer element, ...)
{
    GList *list = NULL;
    va_list args;

    va_start(args, element);
    while (element) {
        list = g_list_prepend(list, element);
        element = va_arg(args, gpointer);
    }
    va_end(args);

    return gcut_take_list(g_list_reverse(list), NULL);
}

void
test_recipients (void)
{
    gcut_assert_equal_list_string(
        NULL,
        milter_session_result_get_message_results(result));

    milter_session_result_add_message_result(result, message_result1);
    milter_session_result_add_message_result(result, message_result2);
    milter_session_result_add_message_result(result, message_result3);
    gcut_assert_equal_list_object(
        take_new_list(message_result1, message_result2, message_result3, NULL),
        milter_session_result_get_message_results(result));

    milter_session_result_remove_message_result(result, message_result2);
    gcut_assert_equal_list_object(
        take_new_list(message_result1, message_result3, NULL),
        milter_session_result_get_message_results(result));

    milter_session_result_set_message_results(
        result,
        take_new_list(message_result4, message_result5, NULL));
    gcut_assert_equal_list_object(
        take_new_list(message_result4, message_result5, NULL),
        milter_session_result_get_message_results(result));
}

void
test_start_stop (void)
{
    GTimeVal *start_time, *start_time_after_stop;
    GTimeVal *end_time;
    gdouble elapsed, elapsed_after_stop;

    start_time = milter_session_result_get_start_time(result);
    cut_assert_equal_int(0, start_time->tv_sec);
    cut_assert_equal_int(0, start_time->tv_usec);
    end_time = milter_session_result_get_end_time(result);
    cut_assert_equal_int(0, end_time->tv_sec);
    cut_assert_equal_int(0, end_time->tv_usec);
    cut_assert_equal_double(0.0,
                            0.0001,
                            milter_session_result_get_elapsed_time(result));

    milter_session_result_start(result);
    start_time = milter_session_result_get_start_time(result);
    cut_assert_operator_int(0, <, start_time->tv_sec);
    cut_assert_operator_int(0, <, start_time->tv_usec);

    end_time = milter_session_result_get_end_time(result);
    cut_assert_equal_int(0, end_time->tv_sec);
    cut_assert_equal_int(0, end_time->tv_usec);

    elapsed = milter_session_result_get_elapsed_time(result);
    cut_assert_equal_double(0.0, 0.0001, elapsed);

    g_usleep(1);

    milter_session_result_stop(result);
    start_time_after_stop = milter_session_result_get_start_time(result);
    cut_assert_equal_int(start_time->tv_sec, start_time_after_stop->tv_sec);
    cut_assert_equal_int(start_time->tv_usec, start_time_after_stop->tv_usec);

    end_time = milter_session_result_get_end_time(result);
    cut_assert_operator_int(0, <, end_time->tv_sec);
    cut_assert_operator_int(0, <, end_time->tv_usec);

    elapsed_after_stop = milter_session_result_get_elapsed_time(result);
    cut_assert_operator_double(elapsed, <, elapsed_after_stop);
    cut_assert_equal_double(elapsed_after_stop,
                            0.0001,
                            milter_session_result_get_elapsed_time(result));
}
