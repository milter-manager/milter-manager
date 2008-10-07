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

#include <milter/client.h>
#undef shutdown

void test_replace_body (void);
void test_replace_body_large (void);

static MilterClientContext *context;
static MilterEncoder *encoder;

static GString *output;
static MilterWriter *writer;

static gchar *packet;
static gsize packet_size;
static GString *expected_packet;

static GString *body;

static MilterStatus
cb_end_of_message (MilterClientContext *context, gpointer user_data)
{
    milter_client_context_replace_body(context, body->str, body->len);
    milter_handler_set_writer(MILTER_HANDLER(context), NULL);

    return MILTER_STATUS_CONTINUE;
}

static void
setup_signals (MilterClientContext *context)
{
#define CONNECT(name)                                                   \
    g_signal_connect(context, #name, G_CALLBACK(cb_ ## name), NULL)

    CONNECT(end_of_message);

#undef CONNECT
}

void
setup (void)
{
    context = milter_client_context_new();
    output = g_string_new(NULL);
    writer = milter_writer_string_new(output);
    milter_handler_set_writer(MILTER_HANDLER(context), writer);
    setup_signals(context);

    encoder = milter_encoder_new();
    packet = NULL;
    packet_size = 0;

    expected_packet = g_string_new(NULL);

    body = g_string_new(NULL);
}

static void
packet_free (void)
{
    if (packet)
        g_free(packet);
    packet = NULL;
    packet_size = 0;
}

void
teardown (void)
{
    if (context)
        g_object_unref(context);

    if (encoder)
        g_object_unref(encoder);

    if (writer)
        g_object_unref(writer);

    if (output)
        g_string_free(output, TRUE);

    packet_free();

    if (expected_packet)
        g_string_free(expected_packet, TRUE);

    if (body)
        g_string_free(body, TRUE);
}

typedef void (*HookFunction) (void);

static GError *
feed (void)
{
    GError *error = NULL;

    milter_handler_feed(MILTER_HANDLER(context), packet, packet_size, &error);

    return error;
}

void
test_replace_body (void)
{
    gsize packed_size = 0;

    g_string_append(body, "XXX");
    milter_encoder_encode_end_of_message(encoder, &packet, &packet_size,
                                         NULL, 0);
    gcut_assert_error(feed());

    packet_free();
    milter_encoder_encode_reply_replace_body(encoder, &packet, &packet_size,
                                             body->str, body->len,
                                             &packed_size);
    cut_assert_equal_memory(packet, packet_size,
                            output->str, output->len);
}

void
test_replace_body_large (void)
{
    gsize i, rest_size;
    gsize packed_size = 0;

    for (i = 0; i < MILTER_CHUNK_SIZE + 500; i++) {
        g_string_append_c(body, 'X');
    }
    milter_encoder_encode_end_of_message(encoder, &packet, &packet_size,
                                         NULL, 0);
    gcut_assert_error(feed());

    packet_free();
    milter_encoder_encode_reply_replace_body(encoder, &packet, &packet_size,
                                             body->str, body->len,
                                             &packed_size);
    g_string_append_len(expected_packet, packet, packet_size);

    packet_free();
    rest_size = body->len - packed_size;
    milter_encoder_encode_reply_replace_body(encoder, &packet, &packet_size,
                                             body->str + packed_size,
                                             rest_size, &packed_size);
    g_string_append_len(expected_packet, packet, packet_size);
    cut_assert_equal_int(rest_size, packed_size);

    cut_assert_equal_memory(expected_packet->str, expected_packet->len,
                            output->str, output->len);
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
