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
#include <milter-core/milter-writer.h>
#undef shutdown

void test_writer_string (void);

static MilterWriter *writer;

static GString *actual_string;

static GError *expected_error;
static GError *actual_error;

void
setup (void)
{
    writer = NULL;

    actual_string = g_string_new(NULL);

    expected_error = NULL;
    actual_error = NULL;
}

void
teardown (void)
{
    if (writer) {
        g_object_unref(writer);
    }

    if (actual_string)
        g_string_free(actual_string, TRUE);

    if (expected_error)
        g_error_free(expected_error);
    if (actual_error)
        g_error_free(actual_error);
}

void
test_writer_string (void)
{
    const gchar first_chunk[] = "first\n";
    const gchar second_chunk[] = "sec\0ond\n";
    const gchar third_chunk[] = "third\n";
    gsize written_size;

    writer = milter_writer_string_new(actual_string);

    milter_writer_write(writer, first_chunk, sizeof(first_chunk) - 1,
                        &written_size, &actual_error);
    cut_assert_equal_memory("first\n", sizeof(first_chunk) - 1,
                            actual_string->str, actual_string->len);
    cut_assert_equal_uint(sizeof(first_chunk) - 1, written_size);
    gcut_assert_error(actual_error);

    actual_error = NULL;
    milter_writer_write(writer, second_chunk, sizeof(second_chunk) - 1,
                        &written_size, &actual_error);
    cut_assert_equal_memory("first\n"
                            "sec\0ond\n",
                            (sizeof(first_chunk) - 1) +
                            (sizeof(second_chunk) - 1),
                            actual_string->str, actual_string->len);
    cut_assert_equal_uint(sizeof(second_chunk) - 1, written_size);
    gcut_assert_error(actual_error);

    actual_error = NULL;
    milter_writer_write(writer, third_chunk, sizeof(third_chunk) - 1,
                        &written_size, &actual_error);
    cut_assert_equal_memory("first\n"
                            "sec\0ond\n"
                            "third\n",
                            (sizeof(first_chunk) - 1) +
                            (sizeof(second_chunk) - 1) +
                            (sizeof(third_chunk) - 1),
                            actual_string->str, actual_string->len);
    cut_assert_equal_uint(sizeof(third_chunk) - 1, written_size);
    gcut_assert_error(actual_error);
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
