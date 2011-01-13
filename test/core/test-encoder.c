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

#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <arpa/inet.h>

#include <milter/core/milter-encoder.h>
#include <milter/core/milter-enum-types.h>
#include <milter/core/milter-macros-requests.h>

#include <gcutter.h>

void test_encode_negotiate (void);
void test_encode_negotiate_null (void);
void test_tag (void);

static MilterEncoder *encoder;
static GString *expected;

void
cut_setup (void)
{
    encoder = milter_encoder_new();

    expected = g_string_new(NULL);
}

void
cut_teardown (void)
{
    if (encoder) {
        g_object_unref(encoder);
    }

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

static void
encode_negotiate_with_option (GString *buffer, MilterOption *option)
{
    guint32 version_network_byte_order;
    guint32 action_network_byte_order;
    guint32 step_network_byte_order;
    gchar version_string[sizeof(guint32)];
    gchar action_string[sizeof(guint32)];
    gchar step_string[sizeof(guint32)];

    version_network_byte_order = g_htonl(milter_option_get_version(option));
    action_network_byte_order = g_htonl(milter_option_get_action(option));
    step_network_byte_order = g_htonl(milter_option_get_step(option));

    memcpy(version_string, &version_network_byte_order, sizeof(guint32));
    memcpy(action_string, &action_network_byte_order, sizeof(guint32));
    memcpy(step_string, &step_network_byte_order, sizeof(guint32));

    g_string_append_c(buffer, 'O');
    g_string_append_len(buffer, version_string, sizeof(version_string));
    g_string_append_len(buffer, action_string, sizeof(action_string));
    g_string_append_len(buffer, step_string, sizeof(step_string));
}

void
test_encode_negotiate (void)
{
    MilterOption *option;
    const gchar *actual;
    gsize actual_size = 0;

    option = milter_option_new(2,
                               MILTER_ACTION_ADD_HEADERS |
                               MILTER_ACTION_CHANGE_BODY |
                               MILTER_ACTION_ADD_ENVELOPE_RECIPIENT |
                               MILTER_ACTION_DELETE_ENVELOPE_RECIPIENT |
                               MILTER_ACTION_CHANGE_HEADERS |
                               MILTER_ACTION_QUARANTINE |
                               MILTER_ACTION_SET_SYMBOL_LIST,
                               MILTER_STEP_NO_CONNECT |
                               MILTER_STEP_NO_HELO |
                               MILTER_STEP_NO_ENVELOPE_FROM |
                               MILTER_STEP_NO_ENVELOPE_RECIPIENT |
                               MILTER_STEP_NO_BODY |
                               MILTER_STEP_NO_HEADERS |
                               MILTER_STEP_NO_END_OF_HEADER);

    encode_negotiate_with_option(expected, option);
    pack(expected);

    milter_encoder_encode_negotiate(encoder, option);
    milter_encoder_pack(encoder, &actual, &actual_size);
    g_object_unref(option);

    cut_assert_equal_memory(expected->str, expected->len, actual, actual_size);
}

void
test_encode_negotiate_null (void)
{
    const gchar *actual;
    gsize actual_size = 0;

    g_string_append_c(expected, 'O');
    pack(expected);
    milter_encoder_encode_negotiate(encoder, NULL);
    milter_encoder_pack(encoder, &actual, &actual_size);
    cut_assert_equal_memory(expected->str, expected->len, actual, actual_size);
}

void
test_tag (void)
{
    cut_assert_equal_uint(0, milter_encoder_get_tag(encoder));

    milter_encoder_set_tag(encoder, 29);
    cut_assert_equal_uint(29, milter_encoder_get_tag(encoder));
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
