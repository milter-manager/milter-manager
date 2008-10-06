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
#include <glib/gstdio.h>

#define shutdown inet_shutdown
#include <milter-core/milter-reader.h>
#undef shutdown

void test_ready_signal (void);
void test_reader_string (void);
void test_reader_io_channel (void);

static MilterReader *reader;

static GString *actual_string;
static GIOChannel *read_channel;
static GIOChannel *write_channel;

static GError *expected_error;
static GError *actual_error;
static gchar *temporary_name;
static gint n_readies;

void
setup (void)
{
    reader = NULL;

    actual_string = NULL;
    read_channel = NULL;
    write_channel = NULL;

    expected_error = NULL;
    actual_error = NULL;

    n_readies = 0;

    temporary_name = NULL;
}

void
teardown (void)
{
    if (reader) {
        g_object_unref(reader);
    }

    if (actual_string)
        g_string_free(actual_string, TRUE);
    if (read_channel)
        g_io_channel_unref(read_channel);
    if (write_channel)
        g_io_channel_unref(write_channel);

    if (expected_error)
        g_error_free(expected_error);
    if (actual_error)
        g_error_free(actual_error);

    if (temporary_name) {
        cut_remove_path(temporary_name);
        g_free(temporary_name);
    }
}

static void
cb_ready (MilterReader *reader, gpointer data)
{
    n_readies++;
}

void
test_ready_signal (void)
{
    actual_string = g_string_new("first");
    reader = milter_reader_string_new(actual_string);

    g_signal_connect(reader, "ready", G_CALLBACK(cb_ready), NULL);
    g_signal_emit_by_name(reader, "ready");

    cut_assert_equal_int(1, n_readies);
}

void
test_reader_string (void)
{
    gsize read_size;
    gchar *actual_read_string = NULL;

    actual_string = g_string_new("first");
    reader = milter_reader_string_new(actual_string);

    milter_reader_read(reader, &actual_read_string, &read_size,
                       &actual_error);
    gcut_assert_error(actual_error);
    cut_assert_equal_string_with_free("first", actual_read_string);
    cut_assert_equal_uint(strlen("first"), read_size);
}

static void
make_temporary_file (void)
{
    gint fd;

    temporary_name = g_strdup("test_readerXXXXXX");
    fd = g_mkstemp(temporary_name);
    if (fd == -1)
        return;

    read_channel = g_io_channel_unix_new(fd);
    g_io_channel_set_close_on_unref(read_channel, TRUE);

    write_channel = g_io_channel_new_file(temporary_name, "w", NULL);
}

static void
write_to_io_channel (const gchar *data)
{
    gsize bytes_written;

    g_io_channel_write_chars(write_channel,
                             data, strlen(data),
                             &bytes_written,
                             NULL);
    g_io_channel_flush(write_channel, NULL);
}

void
test_reader_io_channel (void)
{
    gsize read_size;
    gchar *expected_string = NULL;

    make_temporary_file();
    cut_assert_not_null(read_channel);
    cut_assert_not_null(write_channel);

    reader = milter_reader_io_channel_new(read_channel);

    write_to_io_channel("first");
    milter_reader_read(reader, &expected_string, &read_size,
                       &actual_error);
    expected_string = (gchar *)cut_take_string(expected_string);
    cut_assert_equal_string("first", expected_string);
    cut_assert_equal_uint(sizeof("first") - 1, read_size);
    gcut_assert_error(actual_error);

    write_to_io_channel("second");
    milter_reader_read(reader, &expected_string, &read_size,
                       &actual_error);
    expected_string = (gchar *)cut_take_string(expected_string);
    cut_assert_equal_string("second", expected_string);
    cut_assert_equal_uint(sizeof("second") - 1, read_size);
    gcut_assert_error(actual_error);

    write_to_io_channel("third");
    milter_reader_read(reader, &expected_string, &read_size,
                       &actual_error);
    expected_string = (gchar *)cut_take_string(expected_string);
    cut_assert_equal_string("third", expected_string);
    cut_assert_equal_uint(sizeof("third") - 1, read_size);
    gcut_assert_error(actual_error);
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
