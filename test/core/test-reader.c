/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Copyright (C) 2008-2012  Kouhei Sutou <kou@clear-code.com>
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

#include <errno.h>

#include <gcutter.h>
#include <glib/gstdio.h>

#define shutdown inet_shutdown
#include <milter-test-utils.h>
#include <milter/core/milter-reader.h>
#undef shutdown
#include <unistd.h>

void test_reader_io_channel (void);
void test_reader_io_channel_binary (void);
void test_reader_huge_data (void);
void test_io_error (void);
void test_finished_signal (void);
void test_shutdown (void);
void test_tag (void);

static MilterEventLoop *loop;

static MilterReader *reader;

static GIOChannel *channel;

static gsize actual_read_size;
static GString *actual_read_string;

static guint signal_id;
static guint finished_signal_id;

static GError *actual_error;
static GError *expected_error;

static gboolean finished;

void
cut_setup (void)
{
    signal_id = 0;
    finished_signal_id = 0;

    loop = milter_test_event_loop_new();

    channel = gcut_string_io_channel_new(NULL);
    g_io_channel_set_encoding(channel, NULL, NULL);

    reader = milter_reader_io_channel_new(channel);
    milter_reader_start(reader, loop);

    actual_read_string = g_string_new(NULL);
    actual_read_size = 0;

    actual_error = NULL;
    expected_error = NULL;

    finished = FALSE;
}

void
cut_teardown (void)
{
    if (reader) {
        if (signal_id > 0)
            g_signal_handler_disconnect(reader, signal_id);
        if (finished_signal_id > 0)
            g_signal_handler_disconnect(reader, finished_signal_id);
        g_object_unref(reader);
    }

    if (channel)
        g_io_channel_unref(channel);

    if (loop)
        g_object_unref(loop);

    if (actual_read_string)
        g_string_free(actual_read_string, TRUE);

    if (actual_error)
        g_error_free(actual_error);
    if (expected_error)
        g_error_free(expected_error);
}

static void
cb_flow (MilterReader *reader, const gchar *data, gsize data_size)
{
    g_string_append_len(actual_read_string, data, data_size);
    actual_read_size += data_size;
}

static void
cb_error (MilterErrorEmittable *emittable, GError *error, gsize data_size)
{
    actual_error = g_error_copy(error);
}

static void
cb_finished (MilterFinishedEmittable *emittable)
{
    finished = TRUE;
}

#define pump_all_events()                       \
    cut_trace_with_info_expression(             \
        pump_all_events_helper(),               \
        pump_all_events())

static void
pump_all_events_helper (void)
{
    if (MILTER_IS_LIBEV_EVENT_LOOP(loop))
        cut_omit("MilterLibevEventLoop doesn't support GCutStringIOChannel.");
    milter_test_pump_all_events(loop);
}

static void
write_data (GIOChannel *channel, const gchar *data, gsize size)
{
    gsize bytes_written;
    gint64 read_offset;
    GError *error = NULL;

    g_io_channel_write_chars(channel, data, size, &bytes_written, &error);
    gcut_assert_error(error);

    read_offset = -(gint64)bytes_written;
    g_io_channel_seek_position(channel, read_offset, G_SEEK_CUR, &error);
    gcut_assert_error(error);

    pump_all_events();
}

void
test_reader_io_channel (void)
{
    const gchar data[] = "first\nsecond\nthird\n";

    signal_id = g_signal_connect(reader, "flow", G_CALLBACK(cb_flow), NULL);

    write_data(channel, data, strlen(data));
    cut_assert_equal_memory(data, strlen(data),
                            actual_read_string->str, actual_read_size);
}

void
test_reader_io_channel_binary (void)
{
    const gchar binary_data[4] = {0x00, 0x01, 0x02, 0x03};

    signal_id = g_signal_connect(reader, "flow", G_CALLBACK(cb_flow), NULL);

    write_data(channel, binary_data, 4);
    cut_assert_equal_memory(binary_data, 4,
                            actual_read_string->str, actual_read_size);
}

void
test_reader_huge_data (void)
{
    gchar *binary_data;
    guint data_size = 192 * 8192;
    guint i = 0;
    guint reader_buffer_size = 4096;

    signal_id = g_signal_connect(reader, "flow", G_CALLBACK(cb_flow), NULL);

    binary_data = g_new(gchar, data_size);
    cut_take_memory(binary_data);
    memset(binary_data, '\0', data_size);
    write_data(channel, binary_data, data_size);

    for (i = 0; i < data_size; i += reader_buffer_size)
        milter_event_loop_iterate(loop, FALSE);
    cut_assert_equal_memory(binary_data, data_size,
                            actual_read_string->str, actual_read_size);
}

void
test_io_error (void)
{
    gcut_string_io_channel_set_read_fail(channel, TRUE);

    signal_id = g_signal_connect(reader, "error", G_CALLBACK(cb_error), NULL);

    write_data(channel, "first", strlen("first"));

    expected_error = g_error_new(MILTER_READER_ERROR,
                                 MILTER_READER_ERROR_IO_ERROR,
                                 "I/O error: %s:%d: %s",
                                 g_quark_to_string(G_IO_CHANNEL_ERROR),
                                 g_io_channel_error_from_errno(EIO),
                                 g_strerror(EIO));
    gcut_assert_equal_error(expected_error, actual_error);
}

void
test_finished_signal (void)
{
    finished_signal_id = g_signal_connect(reader, "finished",
                                          G_CALLBACK(cb_finished), NULL);
    signal_id = g_signal_connect(reader, "flow", G_CALLBACK(cb_flow), NULL);

    write_data(channel, "first", strlen("first"));
    cut_assert_equal_memory("first", strlen("first"),
                            actual_read_string->str, actual_read_size);
    cut_assert_false(finished);
    {
        GError *error = NULL;
        g_io_channel_shutdown(channel, TRUE, &error);
        gcut_assert_error(error);
    }
    pump_all_events();
    cut_assert_true(finished);
}

void
test_shutdown (void)
{
    cut_assert_true(milter_reader_is_watching(reader));
    milter_reader_shutdown(reader);
    cut_assert_false(milter_reader_is_watching(reader));
}

void
test_tag (void)
{
    reader = milter_reader_io_channel_new(channel);
    cut_assert_equal_uint(0, milter_reader_get_tag(reader));

    milter_reader_set_tag(reader, 29);
    cut_assert_equal_uint(29, milter_reader_get_tag(reader));
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
