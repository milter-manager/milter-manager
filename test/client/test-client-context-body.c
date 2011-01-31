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

#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <arpa/inet.h>

#include <milter/client.h>

#include <gcutter.h>

#include <milter-test-utils.h>

void test_replace_body (void);
void test_replace_body_large (void);

static MilterEventLoop *loop;

static MilterClientContext *context;
static MilterCommandEncoder *command_encoder;
static MilterReplyEncoder *reply_encoder;

static GIOChannel *channel;
static MilterWriter *writer;

static GError *error_in_callback;

static GString *expected_packet;

static GString *body;

static MilterStatus
cb_end_of_message (MilterClientContext *context,
                   const gchar *chunk, gsize size,
                   gpointer user_data)
{
    milter_client_context_replace_body(context, body->str, body->len,
                                       &error_in_callback);
    milter_agent_set_writer(MILTER_AGENT(context), NULL);

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

static void
setup_option (MilterClientContext *context)
{
    MilterOption *option;

    option = milter_option_new(2, MILTER_ACTION_CHANGE_BODY, 0);
    milter_client_context_set_option(context, option);
    g_object_unref(option);
}

void
cut_setup (void)
{
    GError *error = NULL;

    loop = milter_test_event_loop_new();

    context = milter_client_context_new(NULL);
    milter_agent_set_event_loop(MILTER_AGENT(context), loop);

    channel = gcut_string_io_channel_new(NULL);
    g_io_channel_set_encoding(channel, NULL, NULL);
    writer = milter_writer_io_channel_new(channel);

    milter_agent_set_writer(MILTER_AGENT(context), writer);
    milter_agent_start(MILTER_AGENT(context), &error);
    gcut_assert_error(error);
    setup_signals(context);

    setup_option(context);

    error_in_callback = NULL;

    command_encoder = MILTER_COMMAND_ENCODER(milter_command_encoder_new());
    reply_encoder = MILTER_REPLY_ENCODER(milter_reply_encoder_new());

    expected_packet = g_string_new(NULL);

    body = g_string_new(NULL);
}

void
cut_teardown (void)
{
    if (context)
        g_object_unref(context);

    if (command_encoder)
        g_object_unref(command_encoder);
    if (reply_encoder)
        g_object_unref(reply_encoder);

    if (channel)
        g_io_channel_unref(channel);
    if (writer)
        g_object_unref(writer);

    if (loop)
        g_object_unref(loop);

    if (expected_packet)
        g_string_free(expected_packet, TRUE);

    if (body)
        g_string_free(body, TRUE);
}

typedef void (*HookFunction) (void);

static GError *
feed (const gchar *packet, gsize packet_size)
{
    GError *error = NULL;

    gcut_assert_error(error_in_callback);
    milter_client_context_feed(context, packet, packet_size, &error);
    gcut_assert_error(error_in_callback);

    if (MILTER_IS_LIBEV_EVENT_LOOP(loop))
        cut_omit("MilterLibevEventLoop doesn't support GCutStringIOChannel.");
    milter_test_pump_all_events(loop);

    return error;
}

void
test_replace_body (void)
{
    GString *actual_data;
    const gchar *packet;
    gsize packet_size;
    gsize packed_size = 0;

    g_string_append(body, "XXX");
    milter_command_encoder_encode_end_of_message(command_encoder,
                                                 &packet, &packet_size,
                                                 NULL, 0);
    gcut_assert_error(feed(packet, packet_size));

    milter_reply_encoder_encode_replace_body(reply_encoder,
                                             &packet, &packet_size,
                                             body->str, body->len,
                                             &packed_size);
    actual_data = gcut_string_io_channel_get_string(channel);
    cut_assert_equal_memory(packet, packet_size,
                            actual_data->str, actual_data->len);
}

void
test_replace_body_large (void)
{
    GString *actual_data;
    gsize i, rest_size;
    const gchar *packet;
    gsize packet_size;
    gsize packed_size = 0;

    for (i = 0; i < MILTER_CHUNK_SIZE + 500; i++) {
        g_string_append_c(body, 'X');
    }
    milter_command_encoder_encode_end_of_message(command_encoder,
                                                 &packet, &packet_size,
                                                 NULL, 0);
    gcut_assert_error(feed(packet, packet_size));

    milter_reply_encoder_encode_replace_body(reply_encoder,
                                             &packet, &packet_size,
                                             body->str, body->len,
                                             &packed_size);
    g_string_append_len(expected_packet, packet, packet_size);

    rest_size = body->len - packed_size;
    milter_reply_encoder_encode_replace_body(reply_encoder,
                                             &packet, &packet_size,
                                             body->str + packed_size,
                                             rest_size, &packed_size);
    g_string_append_len(expected_packet, packet, packet_size);
    cut_assert_equal_int(rest_size, packed_size);

    actual_data = gcut_string_io_channel_get_string(channel);
    cut_assert_equal_memory(expected_packet->str, expected_packet->len,
                            actual_data->str, actual_data->len);
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
