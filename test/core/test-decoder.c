/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Copyright (C) 2008-2009  Kouhei Sutou <kou@clear-code.com>
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

#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <arpa/inet.h>

#include <milter/core/milter-command-decoder.h>
#include <milter/core/milter-enum-types.h>

#include <gcutter.h>

void test_decode_empty_text (void);
void test_end_decode_immediately (void);
void test_end_decode_in_command_length_decoding (void);
void test_end_decode_in_command_content_decoding (void);
void test_tag (void);

static MilterDecoder *decoder;
static GString *buffer;

static GError *expected_error;
static GError *actual_error;

void
cut_setup (void)
{
    decoder = milter_command_decoder_new();

    expected_error = NULL;
    actual_error = NULL;

    buffer = g_string_new(NULL);
}

void
cut_teardown (void)
{
    if (decoder) {
        g_object_unref(decoder);
    }

    if (buffer)
        g_string_free(buffer, TRUE);

    if (expected_error)
        g_error_free(expected_error);
    if (actual_error)
        g_error_free(actual_error);
}

void
test_decode_empty_text (void)
{
    cut_assert_true(milter_decoder_decode(decoder, "", 0, &actual_error));
    cut_assert_true(milter_decoder_end_decode(decoder, &actual_error));
}

void
test_end_decode_immediately (void)
{
    cut_assert_true(milter_decoder_end_decode(decoder, &actual_error));
}

void
test_end_decode_in_command_length_decoding (void)
{
    cut_assert_true(milter_decoder_decode(decoder, "\0X", 2, &actual_error));
    cut_assert_false(milter_decoder_end_decode(decoder, &actual_error));

    expected_error = g_error_new(MILTER_DECODER_ERROR,
                                 MILTER_DECODER_ERROR_UNEXPECTED_END,
                                 "stream is ended unexpectedly: "
                                 "need more 2 bytes for decoding "
                                 "command length: 0x00 0x58");
    gcut_assert_equal_error(expected_error, actual_error);
}

void
test_end_decode_in_command_content_decoding (void)
{
    guint32 content_size;

    content_size = g_htonl(4);
    g_string_append_len(buffer, "\0\0\0\0abc", 7);
    memcpy(buffer->str, &content_size, sizeof(content_size));
    cut_assert_true(milter_decoder_decode(decoder, buffer->str, buffer->len,
                                        &actual_error));
    cut_assert_false(milter_decoder_end_decode(decoder, &actual_error));

    expected_error = g_error_new(MILTER_DECODER_ERROR,
                                 MILTER_DECODER_ERROR_UNEXPECTED_END,
                                 "stream is ended unexpectedly: "
                                 "need more 1 byte for decoding "
                                 "command content: 0x61 0x62 0x63 (abc)");
    gcut_assert_equal_error(expected_error, actual_error);
}

void
test_tag (void)
{
    cut_assert_equal_uint(0, milter_decoder_get_tag(decoder));

    milter_decoder_set_tag(decoder, 29);
    cut_assert_equal_uint(29, milter_decoder_get_tag(decoder));
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
