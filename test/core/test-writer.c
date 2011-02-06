/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Copyright (C) 2008-2011  Kouhei Sutou <kou@clear-code.com>
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

#include <gcutter.h>

#define shutdown inet_shutdown
#include <milter-test-utils.h>
#include <milter/core/milter-writer.h>
#undef shutdown
#include <errno.h>

void test_writer (void);
void test_writer_huge_data (void);
void test_writer_error (void);
void test_tag (void);

static MilterEventLoop *loop;

static MilterWriter *writer;

static GIOChannel *channel;

static GError *expected_error;
static GError *actual_error;

static void
cb_error (MilterErrorEmittable *emittable, GError *error)
{
    actual_error = g_error_copy(error);
}

static void
setup_error_callback (void)
{
    g_signal_connect(writer, "error", G_CALLBACK(cb_error), NULL);
}

void
cut_setup (void)
{
    loop = milter_test_event_loop_new();

    channel = gcut_string_io_channel_new(NULL);
    g_io_channel_set_encoding(channel, NULL, NULL);

    writer = milter_writer_io_channel_new(channel);
    milter_writer_start(writer, loop);
    setup_error_callback();

    expected_error = NULL;
    actual_error = NULL;
}

void
cut_teardown (void)
{
    if (channel)
        g_io_channel_unref(channel);

    if (writer)
        g_object_unref(writer);

    if (loop)
        g_object_unref(loop);

    if (expected_error)
        g_error_free(expected_error);
    if (actual_error)
        g_error_free(actual_error);
}

static void
pump_all_events (void)
{
    if (MILTER_IS_LIBEV_EVENT_LOOP(loop))
        cut_omit("MilterLibevEventLoop doesn't support GCutStringIOChannel.");

    milter_test_pump_all_events(loop);
    gcut_assert_error(actual_error);
}

void
test_writer (void)
{
    const gchar first_chunk[] = "first\n";
    const gchar second_chunk[] = "sec\0ond\n";
    const gchar third_chunk[] = "third\n";
    GString *actual_data;
    GError *error = NULL;

    milter_writer_write(writer, first_chunk, sizeof(first_chunk) - 1, &error);
    gcut_assert_error(error);

    milter_writer_flush(writer, &error);
    gcut_assert_error(error);

    pump_all_events();

    actual_data = gcut_string_io_channel_get_string(channel);
    cut_assert_equal_memory("first\n", sizeof(first_chunk) - 1,
                            actual_data->str, actual_data->len);
    gcut_string_io_channel_clear(channel);

    actual_error = NULL;
    milter_writer_write(writer, second_chunk, sizeof(second_chunk) - 1, &error);
    gcut_assert_error(error);

    milter_writer_flush(writer, &error);
    gcut_assert_error(error);

    pump_all_events();

    actual_data = gcut_string_io_channel_get_string(channel);
    cut_assert_equal_memory("sec\0ond\n", (sizeof(second_chunk) - 1),
                            actual_data->str, actual_data->len);
    gcut_string_io_channel_clear(channel);

    milter_writer_write(writer, third_chunk, sizeof(third_chunk) - 1, &error);
    gcut_assert_error(error);

    milter_writer_flush(writer, &error);
    gcut_assert_error(error);

    pump_all_events();

    actual_data = gcut_string_io_channel_get_string(channel);
    cut_assert_equal_memory("third\n", (sizeof(third_chunk) - 1),
                            actual_data->str, actual_data->len);
}

void
test_writer_huge_data (void)
{
    gchar *binary_data;
    gsize data_size;
    GString *actual_data;
    GError *error = NULL;

    data_size = 192 * 8192;
    binary_data = g_new(gchar, data_size);
    cut_take_memory(binary_data);
    memset(binary_data, '\0', data_size);

    milter_writer_write(writer, binary_data, data_size, &error);
    gcut_assert_error(error);

    milter_writer_flush(writer, &error);
    gcut_assert_error(error);

    pump_all_events();

    actual_data = gcut_string_io_channel_get_string(channel);
    cut_assert_equal_memory(binary_data, data_size,
                            actual_data->str, actual_data->len);
}

void
test_writer_error (void)
{
    GError *error = NULL;

    g_io_channel_set_buffered(channel, FALSE);
    g_io_channel_set_flags(channel, G_IO_FLAG_NONBLOCK, &error);
    gcut_assert_error(error);
    gcut_string_io_channel_set_limit(channel, 1);

    milter_writer_write(writer, "test-data", strlen("test-data"), &error);
    gcut_assert_error(error);

    if (MILTER_IS_LIBEV_EVENT_LOOP(loop))
        cut_omit("MilterLibevEventLoop doesn't support GCutStringIOChannel.");
    milter_test_pump_all_events(loop);

    expected_error = g_error_new(MILTER_WRITER_ERROR,
                                 MILTER_WRITER_ERROR_IO_ERROR,
                                 "failed to write: %s:%d: %s",
                                 g_quark_to_string(G_IO_CHANNEL_ERROR),
                                 G_IO_CHANNEL_ERROR_NOSPC,
                                 g_strerror(ENOSPC));
    gcut_assert_equal_error(expected_error, actual_error);
}

void
test_tag (void)
{
    cut_assert_equal_uint(0, milter_writer_get_tag(writer));

    milter_writer_set_tag(writer, 29);
    cut_assert_equal_uint(29, milter_writer_get_tag(writer));
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
