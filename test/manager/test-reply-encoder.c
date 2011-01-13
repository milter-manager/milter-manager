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

#include <string.h>

#include <milter/manager/milter-manager-reply-encoder.h>

#include <gcutter.h>

void test_encode_success (void);
void test_encode_failure (void);
void test_encode_error (void);

static MilterManagerReplyEncoder *encoder;
static GString *expected;

void
cut_setup (void)
{
    MilterEncoder *base_encoder;

    base_encoder = milter_manager_reply_encoder_new();
    encoder = MILTER_MANAGER_REPLY_ENCODER(base_encoder);

    expected = g_string_new(NULL);
}

void
cut_teardown (void)
{
    if (encoder)
        g_object_unref(encoder);

    if (expected)
        g_string_free(expected, TRUE);
}

static void
pack (GString *buffer)
{
    guint32 content_size;
    gchar content_string[sizeof(guint32)];

    content_size = g_htonl(buffer->len);
    memcpy(content_string, &content_size, sizeof(content_size));
    g_string_prepend_len(buffer, content_string, sizeof(content_size));
}

void
test_encode_success (void)
{
    const gchar *actual;
    gsize actual_size;

    g_string_append(expected, "success");
    g_string_append_c(expected, '\0');

    pack(expected);
    milter_manager_reply_encoder_encode_success(encoder,
                                                        &actual, &actual_size);

    cut_assert_equal_memory(expected->str, expected->len,
                            actual, actual_size);
}

void
test_encode_failure (void)
{
    const gchar message[] = "Failure!";
    const gchar *actual;
    gsize actual_size;

    g_string_append(expected, "failure");
    g_string_append_c(expected, '\0');
    g_string_append(expected, message);

    pack(expected);
    milter_manager_reply_encoder_encode_failure(encoder,
                                                        &actual, &actual_size,
                                                        message);

    cut_assert_equal_memory(expected->str, expected->len,
                            actual, actual_size);
}

void
test_encode_error (void)
{
    const gchar message[] = "Error!";
    const gchar *actual;
    gsize actual_size;

    g_string_append(expected, "error");
    g_string_append_c(expected, '\0');
    g_string_append(expected, message);

    pack(expected);
    milter_manager_reply_encoder_encode_error(encoder,
                                                      &actual, &actual_size,
                                                      message);

    cut_assert_equal_memory(expected->str, expected->len,
                            actual, actual_size);
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
