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
#include <milter-core/milter-reader.h>
#undef shutdown

void test_reader_string (void);

static MilterReader *reader;

static GString *actual_string;
static GIOChannel *io_channel;

static GError *expected_error;
static GError *actual_error;

void
setup (void)
{
    reader = NULL;

    actual_string = g_string_new(NULL);

    expected_error = NULL;
    actual_error = NULL;
}

void
teardown (void)
{
    if (reader) {
        g_object_unref(reader);
    }

    if (actual_string)
        g_string_free(actual_string, TRUE);

    if (expected_error)
        g_error_free(expected_error);
    if (actual_error)
        g_error_free(actual_error);
}

void
test_reader_string (void)
{
    gsize read_size;
    gchar *expected_string = NULL;

    actual_string = g_string_append(actual_string, "first");
    reader = milter_reader_string_new(actual_string);

    milter_reader_read(reader, &expected_string, &read_size,
                       &actual_error);
    expected_string = cut_take_string(expected_string);
    cut_assert_equal_string("first", expected_string);
    cut_assert_equal_uint(sizeof("first") - 1, read_size);
    gcut_assert_error(actual_error);
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
