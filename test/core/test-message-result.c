/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Copyright (C) 2009  Kouhei Sutou <kou@clear-code.com>
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

#if defined(CUTTER_CHECK_VERSION)
#  if CUTTER_CHECK_VERSION(1, 0, 7)

void test_from (void);

static MilterMessageResult *result;

void
setup (void)
{
    result = milter_message_result_new();
}

void
teardown (void)
{
    if (result)
        g_object_unref(result);
}

void
test_from (void)
{
    cut_assert_equal_string(NULL,
                            milter_masagge_result_get_from(result));
    milter_masagge_result_set_from(result, "sender@example.com");
    cut_assert_equal_string("sender@example.com",
                            milter_masagge_result_get_from(result));
}
