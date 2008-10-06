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

#include <milter-test-utils.h>
#define shutdown inet_shutdown
#include <milter-core/milter-reader.h>
#undef shutdown

void test_ready_signal (void);
void test_reader_string (void);
void test_reader_io_channel (void);

static MilterReader *reader;

static gchar *tmp_dir;

static GString *actual_string;
static GIOChannel *read_channel;
static GIOChannel *write_channel;

static gint n_readies;

void
setup (void)
{
    reader = NULL;

    actual_string = NULL;
    read_channel = NULL;
    write_channel = NULL;

    n_readies = 0;

    tmp_dir = g_build_filename(milter_test_get_base_dir(), "tmp", NULL);
    cut_remove_path(tmp_dir);
    g_mkdir_with_parents(tmp_dir, 0700);
    cut_assert_path_exist(tmp_dir);
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

    if (tmp_dir) {
        cut_remove_path(tmp_dir);
        g_free(tmp_dir);
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
    GError *error = NULL;

    actual_string = g_string_new("first");
    reader = milter_reader_string_new(actual_string);

    milter_reader_read(reader, &actual_read_string, &read_size, &error);
    gcut_assert_error(error);
    cut_assert_equal_string_with_free("first", actual_read_string);
    cut_assert_equal_uint(strlen("first"), read_size);
}

static void
milter_assert_make_channels_helper (void)
{
    GError *error = NULL;
    gchar *file_name;
    const gchar *taken_file_name;

    file_name = g_build_filename(tmp_dir, "reader-channel.txt", NULL);
    taken_file_name = cut_take_string(file_name);

    write_channel = g_io_channel_new_file(taken_file_name, "w", &error);
    gcut_assert_error(error);

    read_channel = g_io_channel_new_file(taken_file_name, "r", &error);
    gcut_assert_error(error);
    g_io_channel_set_close_on_unref(read_channel, TRUE);

}

#define milter_assert_make_channels()           \
    cut_trace_with_info_expression(             \
        milter_assert_make_channels_helper(),   \
        milter_assert_make_channels())

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
    gchar *actual_read_string = NULL;
    GError *error = NULL;

    milter_assert_make_channels();

    reader = milter_reader_io_channel_new(read_channel);

    write_to_io_channel("first");
    milter_reader_read(reader, &actual_read_string, &read_size, &error);
    gcut_assert_error(error);
    cut_assert_equal_string_with_free("first", actual_read_string);
    cut_assert_equal_uint(strlen("first"), read_size);

    write_to_io_channel("second");
    milter_reader_read(reader, &actual_read_string, &read_size, &error);
    gcut_assert_error(error);
    cut_assert_equal_string("second", actual_read_string);
    cut_assert_equal_uint(strlen("second"), read_size);

    write_to_io_channel("third");
    milter_reader_read(reader, &actual_read_string, &read_size, &error);
    gcut_assert_error(error);
    cut_assert_equal_string("third", actual_read_string);
    cut_assert_equal_uint(strlen("third"), read_size);
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
