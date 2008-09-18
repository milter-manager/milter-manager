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
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <arpa/inet.h>
#undef shutdown

#include <milter-core/milter-encoder.h>
#include <milter-core/milter-enum-types.h>

void test_encode_option_negotiation (void);

static MilterEncoder *encoder;
static GString *expected;
static GString *actual;

static GHashTable *macros;

void
setup (void)
{
    encoder = milter_encoder_new();

    expected = g_string_new(NULL);
    actual = g_string_new(NULL);

    macros = NULL;
}

void
teardown (void)
{
    if (encoder) {
        g_object_unref(encoder);
    }

    if (expected)
        g_string_free(expected, TRUE);
    if (actual)
        g_string_free(actual, TRUE);

    if (macros)
        g_hash_table_unref(macros);
}

static void
pack (GString *buffer)
{
    guint32 content_size;
    gchar content_string[sizeof(guint32)];

    content_size = htonl(buffer->len);
    memcpy(content_string, &content_size, sizeof(content_size));
    g_string_prepend_len(buffer, content_string, sizeof(content_size));
}

void
test_encode_option_negotiation (void)
{
    MilterOption *option;
    guint32 version_network_byte_order;
    guint32 action_network_byte_order;
    guint32 step_network_byte_order;
    gchar version_string[sizeof(guint32)];
    gchar action_string[sizeof(guint32)];
    gchar step_string[sizeof(guint32)];

    option = milter_option_new(2,
                               MILTER_ACTION_ADD_HEADERS |
                               MILTER_ACTION_CHANGE_BODY |
                               MILTER_ACTION_ADD_RCPT |
                               MILTER_ACTION_DELETE_RCPT |
                               MILTER_ACTION_CHANGE_HEADERS |
                               MILTER_ACTION_QUARANTINE |
                               MILTER_ACTION_SET_SYMBOL_LIST,
                               MILTER_STEP_NO_CONNECT |
                               MILTER_STEP_NO_HELO |
                               MILTER_STEP_NO_MAIL |
                               MILTER_STEP_NO_RCPT |
                               MILTER_STEP_NO_BODY |
                               MILTER_STEP_NO_HEADERS |
                               MILTER_STEP_NO_END_OF_HEADER);

    version_network_byte_order = htonl(milter_option_get_version(option));
    action_network_byte_order = htonl(milter_option_get_action(option));
    step_network_byte_order = htonl(milter_option_get_step(option));

    memcpy(version_string, &version_network_byte_order, sizeof(guint32));
    memcpy(action_string, &action_network_byte_order, sizeof(guint32));
    memcpy(step_string, &step_network_byte_order, sizeof(guint32));

    g_string_append_c(expected, 'O');
    g_string_append_len(expected, version_string, sizeof(version_string));
    g_string_append_len(expected, action_string, sizeof(action_string));
    g_string_append_len(expected, step_string, sizeof(step_string));
    pack(expected);

    milter_encoder_encode_option_negotiation(encoder, actual, option);
    g_object_unref(option);

    cut_assert_equal_memory(expected->str, expected->len,
                            actual->str, actual->len);
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
