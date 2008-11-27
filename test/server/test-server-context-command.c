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

#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <arpa/inet.h>

#include <milter/server.h>
#include <milter/core.h>

#include <gcutter.h>

#include "milter-test-utils.h"

void test_negotiate (void);
void test_connect (void);
void test_helo (void);
void test_envelope_from (void);
void test_envelope_recipient (void);
void test_data (void);
void test_unknown (void);
void test_header (void);
void test_end_of_header (void);
void test_body (void);
void test_end_of_message (void);
void test_end_of_message_without_chunk (void);
void test_quit (void);
void test_abort (void);
void test_abort_and_quit (void);
void test_connect_with_macro (void);
void test_reading_timeout (void);

static MilterServerContext *context;

static MilterWriter *writer;

static MilterCommandEncoder *encoder;
static gchar *packet;
static gsize packet_size;
static GString *packet_string;
static GHashTable *macros;

static GIOChannel *channel;

static gboolean timed_out_before;
static gboolean timed_out_after;
static guint timeout_id;

void
setup (void)
{
    context = milter_server_context_new();

    channel = gcut_string_io_channel_new(NULL);
    g_io_channel_set_encoding(channel, NULL, NULL);
    writer = milter_writer_io_channel_new(channel);
    milter_agent_set_writer(MILTER_AGENT(context), writer);

    encoder = MILTER_COMMAND_ENCODER(milter_command_encoder_new());
    packet = NULL;
    packet_size = 0;
    packet_string = NULL;
    macros = NULL;

    timed_out_before = FALSE;
    timed_out_after = FALSE;
    timeout_id = 0;
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
    if (timeout_id > 0)
        g_source_remove(timeout_id);

    if (context)
        g_object_unref(context);

    if (writer)
        g_object_unref(writer);

    if (encoder)
        g_object_unref(encoder);

    if (channel)
        g_io_channel_unref(channel);

    packet_free();
    if (packet_string)
        g_string_free(packet_string, TRUE);
    if (macros)
        g_hash_table_unref(macros);
}

#define milter_test_assert_packet(channel, packet, packet_size)         \
    cut_trace(milter_test_assert_packet_helper(channel, packet, packet_size))

static void
milter_test_assert_packet_helper (GIOChannel *channel,
                                  gchar *packet, gsize packet_size)
{
    GString *actual;

    actual = gcut_string_io_channel_get_string(channel);
    cut_assert_equal_memory(packet, packet_size, actual->str, actual->len);
}

#define milter_test_assert_status(status)                               \
    cut_trace(                                                          \
        gcut_assert_equal_enum(MILTER_TYPE_SERVER_CONTEXT_STATE,        \
                               MILTER_SERVER_CONTEXT_STATE_ ## status,  \
                               milter_server_context_get_state(context)))

void
test_negotiate (void)
{
    MilterOption *option;

    option = milter_option_new(2, MILTER_ACTION_ADD_HEADERS, MILTER_STEP_NONE);
    gcut_take_object(G_OBJECT(option));
    cut_assert_true(milter_server_context_negotiate(context, option));
    milter_test_assert_status(NEGOTIATE);

    milter_command_encoder_encode_negotiate(encoder,
                                            &packet, &packet_size,
                                            option);
    milter_test_assert_packet(channel, packet, packet_size);
}

void
test_connect (void)
{
    struct sockaddr_in address;
    const gchar host_name[] = "mx.local.net";
    const gchar ip_address[] = "192.168.123.123";
    guint16 port;

    port = g_htons(50443);
    address.sin_family = AF_INET;
    address.sin_port = port;
    inet_aton(ip_address, &(address.sin_addr));

    cut_assert_true(milter_server_context_connect(context, host_name,
                                                  (struct sockaddr *)&address,
                                                  sizeof(address)));
    milter_test_assert_status(CONNECT);

    milter_command_encoder_encode_connect(encoder,
                                          &packet, &packet_size,
                                          host_name,
                                          (const struct sockaddr *)&address,
                                          sizeof(address));
    milter_test_assert_packet(channel, packet, packet_size);
}

void
test_connect_with_macro (void)
{
    struct sockaddr_in address;
    const gchar host_name[] = "mx.local.net";
    const gchar ip_address[] = "192.168.123.123";
    guint16 port;

    port = g_htons(50443);
    address.sin_family = AF_INET;
    address.sin_port = port;
    inet_aton(ip_address, &(address.sin_addr));

    macros = gcut_hash_table_string_string_new("G", "g",
                                               "N", "n",
                                               "U", "u",
                                               NULL);
    milter_protocol_agent_set_macros_hash_table(MILTER_PROTOCOL_AGENT(context),
                                                MILTER_COMMAND_CONNECT,
                                                macros);
    milter_command_encoder_encode_define_macro(encoder,
                                               &packet, &packet_size,
                                               MILTER_COMMAND_CONNECT,
                                               macros);
    cut_assert_true(milter_server_context_connect(context, host_name,
                                                  (struct sockaddr *)&address,
                                                  sizeof(address)));
    milter_test_assert_status(CONNECT);

    packet_string = g_string_new_len(packet, packet_size);
    packet_free();
    milter_command_encoder_encode_connect(encoder,
                                          &packet, &packet_size,
                                          host_name,
                                          (const struct sockaddr *)&address,
                                          sizeof(address));
    g_string_append_len(packet_string, packet, packet_size);
    milter_test_assert_packet(channel, packet_string->str, packet_string->len);
}

void
test_helo (void)
{
    const gchar fqdn[] = "delian";

    cut_assert_true(milter_server_context_helo(context, fqdn));
    milter_test_assert_status(HELO);

    milter_command_encoder_encode_helo(encoder, &packet, &packet_size, fqdn);
    milter_test_assert_packet(channel, packet, packet_size);
}

void
test_envelope_from (void)
{
    const gchar from[] = "example@example.com";

    cut_assert_true(milter_server_context_envelope_from(context, from));
    milter_test_assert_status(ENVELOPE_FROM);

    milter_command_encoder_encode_envelope_from(encoder,
                                                &packet, &packet_size, from);
    milter_test_assert_packet(channel, packet, packet_size);
}

void
test_envelope_recipient (void)
{
    const gchar recipient[] = "example@example.com";

    cut_assert_true(milter_server_context_envelope_recipient(context, recipient));
    milter_test_assert_status(ENVELOPE_RECIPIENT);

    milter_command_encoder_encode_envelope_recipient(encoder,
                                                     &packet, &packet_size,
                                                     recipient);
    milter_test_assert_packet(channel, packet, packet_size);
}

void
test_data (void)
{
    cut_assert_true(milter_server_context_data(context));
    milter_test_assert_status(DATA);

    milter_command_encoder_encode_data(encoder, &packet, &packet_size);
    milter_test_assert_packet(channel, packet, packet_size);
}

void
test_unknown (void)
{
    const gchar command[] = "Who am I?";

    cut_assert_true(milter_server_context_unknown(context, command));
    milter_test_assert_status(UNKNOWN);

    milter_command_encoder_encode_unknown(encoder,
                                          &packet, &packet_size,
                                          command);
    milter_test_assert_packet(channel, packet, packet_size);
}

void
test_header (void)
{
    const gchar name[] = "X-HEADER-NAME";
    const gchar value[] = "MilterServerContext test";

    cut_assert_true(milter_server_context_header(context, name, value));
    milter_test_assert_status(HEADER);

    milter_command_encoder_encode_header(encoder,
                                         &packet, &packet_size,
                                         name, value);
    milter_test_assert_packet(channel, packet, packet_size);
}

void
test_end_of_header (void)
{
    cut_assert_true(milter_server_context_end_of_header(context));
    milter_test_assert_status(END_OF_HEADER);

    milter_command_encoder_encode_end_of_header(encoder, &packet, &packet_size);
    milter_test_assert_packet(channel, packet, packet_size);
}

void
test_body (void)
{
    const gchar chunk[] = "This is a body text.";
    gsize packed_size;

    cut_assert_true(milter_server_context_body(context, chunk, strlen(chunk)));
    milter_test_assert_status(BODY);

    milter_command_encoder_encode_body(encoder, &packet, &packet_size,
                                       chunk, strlen(chunk), &packed_size);
    milter_test_assert_packet(channel, packet, packet_size);
    cut_assert_equal_uint(strlen(chunk), packed_size);
}

void
test_end_of_message (void)
{
    const gchar chunk[] = "This is a end_of_message text.";

    cut_assert_true(milter_server_context_end_of_message(context,
                                                         chunk, strlen(chunk)));
    milter_test_assert_status(END_OF_MESSAGE);

    milter_command_encoder_encode_end_of_message(encoder, &packet, &packet_size,
                                                 chunk, strlen(chunk));
    milter_test_assert_packet(channel, packet, packet_size);
}

void
test_end_of_message_without_chunk (void)
{
    cut_assert_true(milter_server_context_end_of_message(context, NULL, 0));
    milter_test_assert_status(END_OF_MESSAGE);

    milter_command_encoder_encode_end_of_message(encoder, &packet, &packet_size,
                                                 NULL, 0);
    milter_test_assert_packet(channel, packet, packet_size);
}

void
test_quit (void)
{
    cut_assert_true(milter_server_context_quit(context));
    milter_test_assert_status(QUIT);

    milter_command_encoder_encode_quit(encoder, &packet, &packet_size);
    milter_test_assert_packet(channel, packet, packet_size);
}

void
test_abort (void)
{
    cut_assert_true(milter_server_context_abort(context));
    milter_test_assert_status(ABORT);

    milter_command_encoder_encode_abort(encoder, &packet, &packet_size);
    milter_test_assert_packet(channel, packet, packet_size);
}

void
test_abort_and_quit (void)
{
    cut_assert_true(milter_server_context_abort(context));
    milter_test_assert_status(ABORT);

    cut_assert_true(milter_server_context_quit(context));
    milter_test_assert_status(QUIT);

    milter_command_encoder_encode_abort(encoder, &packet, &packet_size);
    packet_string = g_string_new_len(packet, packet_size);
    packet_free();

    milter_command_encoder_encode_quit(encoder, &packet, &packet_size);
    g_string_append_len(packet_string, packet, packet_size);

    milter_test_assert_packet(channel, packet_string->str, packet_string->len);
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
