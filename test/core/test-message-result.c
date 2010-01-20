/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Copyright (C) 2009-2010  Kouhei Sutou <kou@clear-code.com>
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

#include <milter/core/milter-message-result.h>
#include <milter-test-utils.h>

#include <gcutter.h>

void test_from (void);
void test_recipients (void);
void test_temporary_failed_recipients (void);
void test_rejected_recipients (void);
void test_start_stop (void);

static MilterMessageResult *result;

void
cut_setup (void)
{
    result = milter_message_result_new();
}

void
cut_teardown (void)
{
    if (result)
        g_object_unref(result);
}

void
test_from (void)
{
    cut_assert_equal_string(NULL,
                            milter_message_result_get_from(result));
    milter_message_result_set_from(result, "sender@example.com");
    cut_assert_equal_string("sender@example.com",
                            milter_message_result_get_from(result));
}

void
test_recipients (void)
{
    gcut_assert_equal_list_string(
        NULL,
        milter_message_result_get_recipients(result));

    milter_message_result_add_recipient(result, "receiver1@example.com");
    milter_message_result_add_recipient(result, "receiver2@example.com");
    milter_message_result_add_recipient(result, "receiver3@example.com");
    gcut_assert_equal_list_string(
        gcut_take_new_list_string("receiver1@example.com",
                                  "receiver2@example.com",
                                  "receiver3@example.com",
                                  NULL),
        milter_message_result_get_recipients(result));

    milter_message_result_remove_recipient(result, "receiver2@example.com");
    gcut_assert_equal_list_string(
        gcut_take_new_list_string("receiver1@example.com",
                                  "receiver3@example.com",
                                  NULL),
        milter_message_result_get_recipients(result));

    milter_message_result_set_recipients(
        result,
        gcut_take_new_list_string("receiver4@example.com",
                                  "receiver5@example.com",
                                  NULL));
    gcut_assert_equal_list_string(
        gcut_take_new_list_string("receiver4@example.com",
                                  "receiver5@example.com",
                                  NULL),
        milter_message_result_get_recipients(result));
}

void
test_temporary_failed_recipients (void)
{
    gcut_assert_equal_list_string(
        NULL,
        milter_message_result_get_temporary_failed_recipients(result));

    milter_message_result_add_temporary_failed_recipient(
        result, "receiver1@example.com");
    milter_message_result_add_temporary_failed_recipient(
        result, "receiver2@example.com");
    milter_message_result_add_temporary_failed_recipient(
        result, "receiver3@example.com");
    gcut_assert_equal_list_string(
        gcut_take_new_list_string("receiver1@example.com",
                                  "receiver2@example.com",
                                  "receiver3@example.com",
                                  NULL),
        milter_message_result_get_temporary_failed_recipients(result));

    milter_message_result_set_temporary_failed_recipients(
        result,
        gcut_take_new_list_string("receiver4@example.com",
                                  "receiver5@example.com",
                                  NULL));
    gcut_assert_equal_list_string(
        gcut_take_new_list_string("receiver4@example.com",
                                  "receiver5@example.com",
                                  NULL),
        milter_message_result_get_temporary_failed_recipients(result));
}

void
test_rejected_recipients (void)
{
    gcut_assert_equal_list_string(
        NULL,
        milter_message_result_get_rejected_recipients(result));

    milter_message_result_add_rejected_recipient(result,
                                                 "receiver1@example.com");
    milter_message_result_add_rejected_recipient(result,
                                                 "receiver2@example.com");
    milter_message_result_add_rejected_recipient(result,
                                                 "receiver3@example.com");
    gcut_assert_equal_list_string(
        gcut_take_new_list_string("receiver1@example.com",
                                  "receiver2@example.com",
                                  "receiver3@example.com",
                                  NULL),
        milter_message_result_get_rejected_recipients(result));

    milter_message_result_set_rejected_recipients(
        result,
        gcut_take_new_list_string("receiver4@example.com",
                                  "receiver5@example.com",
                                  NULL));
    gcut_assert_equal_list_string(
        gcut_take_new_list_string("receiver4@example.com",
                                  "receiver5@example.com",
                                  NULL),
        milter_message_result_get_rejected_recipients(result));
}

void
test_start_stop (void)
{
    GTimeVal *start_time, *start_time_after_stop;
    GTimeVal *end_time;
    gdouble elapsed, elapsed_after_stop;

    start_time = milter_message_result_get_start_time(result);
    cut_assert_equal_int(0, start_time->tv_sec);
    cut_assert_equal_int(0, start_time->tv_usec);
    end_time = milter_message_result_get_end_time(result);
    cut_assert_equal_int(0, end_time->tv_sec);
    cut_assert_equal_int(0, end_time->tv_usec);
    cut_assert_equal_double(0.0,
                            0.0001,
                            milter_message_result_get_elapsed_time(result));

    milter_message_result_start(result);
    start_time = milter_message_result_get_start_time(result);
    cut_assert_operator_int(0, <, start_time->tv_sec);
    cut_assert_operator_int(0, <, start_time->tv_usec);

    end_time = milter_message_result_get_end_time(result);
    cut_assert_equal_int(0, end_time->tv_sec);
    cut_assert_equal_int(0, end_time->tv_usec);

    elapsed = milter_message_result_get_elapsed_time(result);
    cut_assert_equal_double(0.0, 0.0001, elapsed);

    g_usleep(1);

    milter_message_result_stop(result);
    start_time_after_stop = milter_message_result_get_start_time(result);
    cut_assert_equal_int(start_time->tv_sec, start_time_after_stop->tv_sec);
    cut_assert_equal_int(start_time->tv_usec, start_time_after_stop->tv_usec);

    end_time = milter_message_result_get_end_time(result);
    cut_assert_operator_int(0, <, end_time->tv_sec);
    cut_assert_operator_int(0, <, end_time->tv_usec);

    elapsed_after_stop = milter_message_result_get_elapsed_time(result);
    cut_assert_operator_double(elapsed, <, elapsed_after_stop);
    cut_assert_equal_double(elapsed_after_stop,
                            0.0001,
                            milter_message_result_get_elapsed_time(result));
}
