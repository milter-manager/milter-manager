/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Copyright (C) 2009-2011  Kouhei Sutou <kou@clear-code.com>
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
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <arpa/inet.h>

#include <milter/client.h>
#include <milter-test-utils.h>
#undef shutdown

void test_tag (void);
void test_socket_address (void);
void test_n_processing_sessions (void);
void test_packet_buffer_size (void);
void test_quarantine_reason (void);

static MilterClientContext *context;

static GIOChannel *output_channel;
static GIOChannel *input_channel;
static MilterWriter *writer;
static MilterReader *reader;

void
cut_setup (void)
{
    context = milter_client_context_new(NULL);

    output_channel = gcut_string_io_channel_new(NULL);
    g_io_channel_set_encoding(output_channel, NULL, NULL);
    writer = milter_writer_io_channel_new(output_channel);

    input_channel = gcut_string_io_channel_new(NULL);
    g_io_channel_set_encoding(input_channel, NULL, NULL);
    reader = milter_reader_io_channel_new(input_channel);

    milter_agent_set_writer(MILTER_AGENT(context), writer);
    milter_agent_set_reader(MILTER_AGENT(context), reader);
}

void
cut_teardown (void)
{
    if (context)
        g_object_unref(context);

    if (writer)
        g_object_unref(writer);

    if (reader)
        g_object_unref(reader);

    if (output_channel)
        g_io_channel_unref(output_channel);

    if (input_channel)
        g_io_channel_unref(input_channel);
}

void
test_tag (void)
{
    MilterAgent *agent;
    MilterEncoder *encoder;
    MilterDecoder *decoder;
    guint tag;

    agent = MILTER_AGENT(context);
    tag = milter_agent_get_tag(agent);
    cut_assert_operator_uint(0, !=, tag);

    encoder = milter_agent_get_encoder(agent);
    decoder = milter_agent_get_decoder(agent);
    cut_assert_equal_uint(tag, milter_encoder_get_tag(encoder));
    cut_assert_equal_uint(tag, milter_decoder_get_tag(decoder));
    cut_assert_equal_uint(tag, milter_writer_get_tag(writer));
    cut_assert_equal_uint(tag, milter_reader_get_tag(reader));


    milter_agent_set_tag(agent, 29);
    cut_assert_equal_uint(29, milter_agent_get_tag(agent));
    cut_assert_equal_uint(29, milter_encoder_get_tag(encoder));
    cut_assert_equal_uint(29, milter_decoder_get_tag(decoder));
    cut_assert_equal_uint(29, milter_writer_get_tag(writer));
    cut_assert_equal_uint(29, milter_reader_get_tag(reader));

    milter_agent_set_reader(agent, NULL);
    cut_assert_equal_uint(29, milter_reader_get_tag(reader));
    milter_agent_set_writer(agent, NULL);
    cut_assert_equal_uint(29, milter_writer_get_tag(writer));

    milter_reader_set_tag(reader, 2929);
    cut_assert_equal_uint(2929, milter_reader_get_tag(reader));
    milter_writer_set_tag(writer, 2929);
    cut_assert_equal_uint(2929, milter_writer_get_tag(writer));

    milter_agent_set_reader(agent, reader);
    cut_assert_equal_uint(29, milter_reader_get_tag(reader));
    milter_agent_set_writer(agent, writer);
    cut_assert_equal_uint(29, milter_writer_get_tag(writer));
}

void
test_socket_address (void)
{
    MilterGenericSocketAddress address;
    MilterGenericSocketAddress *actual_address;

    cut_assert_null(milter_client_context_get_socket_address(context));

    address.address.un.sun_family = AF_UNIX;
    strcpy(address.address.un.sun_path, "/tmp/socket");
    milter_client_context_set_socket_address(context, &address);

    actual_address = milter_client_context_get_socket_address(context);
    cut_assert_equal_memory(&address, sizeof(address),
                            actual_address, sizeof(*actual_address));
}

void
test_n_processing_sessions (void)
{
    cut_assert_equal_uint(
        0, milter_client_context_get_n_processing_sessions(context));
}

void
test_packet_buffer_size (void)
{
    cut_assert_equal_uint(0,
                          milter_client_context_get_packet_buffer_size(context));
    milter_client_context_set_packet_buffer_size(context, 4096);
    cut_assert_equal_uint(4096,
                          milter_client_context_get_packet_buffer_size(context));
}

void
test_quarantine_reason (void)
{
    cut_assert_equal_string(
        NULL,
        milter_client_context_get_quarantine_reason(context));
    milter_client_context_set_quarantine_reason(context, "maybe a virus.");
    cut_assert_equal_string(
        "maybe a virus.",
        milter_client_context_get_quarantine_reason(context));
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
