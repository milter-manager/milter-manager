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

#include <milter/client.h>
#include <milter-test-utils.h>

#include <gcutter.h>

void test_add_header (void);
void test_insert_header (void);
void test_change_header (void);
void test_delete_header (void);

static MilterClientContext *context;
static MilterCommandEncoder *command_encoder;
static MilterReplyEncoder *reply_encoder;

static GIOChannel *channel;
static MilterWriter *writer;

static gchar *packet;
static gsize packet_size;

static gboolean add_header;
static gboolean insert_header;
static gboolean change_header;
static gboolean delete_header;

static gchar *header_name;
static gchar *header_value;
static guint32 header_index;

static MilterStatus
cb_negotiate (MilterClientContext *context, MilterOption *option,
              gpointer user_data)
{
    return MILTER_STATUS_CONTINUE;
}

static MilterStatus
cb_connect (MilterClientContext *context, const gchar *host_name,
            const struct sockaddr *address, socklen_t address_size,
            gpointer user_data)
{
    return MILTER_STATUS_CONTINUE;
}

static MilterStatus
cb_helo (MilterClientContext *context, const gchar *fqdn, gpointer user_data)
{
    return MILTER_STATUS_CONTINUE;
}

static MilterStatus
cb_envelope_from (MilterClientContext *context, const gchar *from,
                  gpointer user_data)
{
    return MILTER_STATUS_CONTINUE;
}

static MilterStatus
cb_envelope_recipient (MilterClientContext *context, const gchar *to,
                       gpointer user_data)
{
    return MILTER_STATUS_CONTINUE;
}

static MilterStatus
cb_header (MilterClientContext *context, const gchar *name, const gchar *value,
           gpointer user_data)
{
    return MILTER_STATUS_CONTINUE;
}

static MilterStatus
cb_end_of_header (MilterClientContext *context, gpointer user_data)
{
    return MILTER_STATUS_CONTINUE;
}

static MilterStatus
cb_body (MilterClientContext *context, const gchar *chunk, gsize size,
         gpointer user_data)
{
    return MILTER_STATUS_CONTINUE;
}

static MilterStatus
cb_end_of_message (MilterClientContext *context, gpointer user_data)
{
    if (add_header) {
        milter_client_context_add_header(context, header_name, header_value);
        milter_agent_set_writer(MILTER_AGENT(context), NULL);
    }

    if (insert_header) {
        milter_client_context_insert_header(context, header_index,
                                            header_name, header_value);
        milter_agent_set_writer(MILTER_AGENT(context), NULL);
    }

    if (change_header) {
        milter_client_context_change_header(context,
                                            header_name,
                                            header_index,
                                            header_value);
        milter_agent_set_writer(MILTER_AGENT(context), NULL);
    }

    if (delete_header) {
        milter_client_context_delete_header(context, header_name, header_index);
        milter_agent_set_writer(MILTER_AGENT(context), NULL);
    }

    return MILTER_STATUS_CONTINUE;
}

static MilterStatus
cb_abort (MilterClientContext *context, gpointer user_data)
{
    return MILTER_STATUS_CONTINUE;
}

static MilterStatus
cb_quit (MilterClientContext *context, gpointer user_data)
{
    return MILTER_STATUS_CONTINUE;
}

static MilterStatus
cb_unknown (MilterClientContext *context, const gchar *command,
            gpointer user_data)
{
    return MILTER_STATUS_CONTINUE;
}

static void
setup_signals (MilterClientContext *context)
{
#define CONNECT(name)                                                   \
    g_signal_connect(context, #name, G_CALLBACK(cb_ ## name), NULL)

    CONNECT(negotiate);
    CONNECT(connect);
    CONNECT(helo);
    CONNECT(envelope_from);
    CONNECT(envelope_recipient);
    CONNECT(header);
    CONNECT(end_of_header);
    CONNECT(body);
    CONNECT(end_of_message);
    CONNECT(abort);
    CONNECT(quit);
    CONNECT(unknown);

#undef CONNECT
}

void
setup (void)
{
    context = milter_client_context_new();

    channel = gcut_string_io_channel_new(NULL);
    g_io_channel_set_encoding(channel, NULL, NULL);
    writer = milter_writer_io_channel_new(channel);

    milter_agent_set_writer(MILTER_AGENT(context), writer);
    setup_signals(context);

    command_encoder = MILTER_COMMAND_ENCODER(milter_command_encoder_new());
    reply_encoder = MILTER_REPLY_ENCODER(milter_reply_encoder_new());
    packet = NULL;
    packet_size = 0;

    header_name = NULL;
    header_value = NULL;
    header_index = 0;

    add_header = FALSE;
    insert_header = FALSE;
    change_header = FALSE;
    delete_header = FALSE;
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

    if (command_encoder)
        g_object_unref(command_encoder);
    if (reply_encoder)
        g_object_unref(reply_encoder);

    if (channel)
        g_io_channel_unref(channel);
    if (writer)
        g_object_unref(writer);

    packet_free();

    if (header_name)
        g_free(header_name);
    if (header_value)
        g_free(header_value);
}

typedef void (*HookFunction) (void);

static GError *
feed (void)
{
    GError *error = NULL;

    milter_client_context_feed(context, packet, packet_size, &error);

    return error;
}

void
test_add_header (void)
{
    GString *actual_data;

    add_header = TRUE;
    header_name = g_strdup("X-Test-Header");
    header_value = g_strdup("Test Value");
    milter_command_encoder_encode_end_of_message(command_encoder,
                                                 &packet, &packet_size,
                                                 NULL, 0);
    gcut_assert_error(feed());

    packet_free();
    milter_reply_encoder_encode_add_header(reply_encoder,
                                           &packet, &packet_size,
                                           header_name, header_value);
    actual_data = gcut_string_io_channel_get_string(channel);
    cut_assert_equal_memory(packet, packet_size,
                            actual_data->str, actual_data->len);
}

void
test_insert_header (void)
{
    GString *actual_data;

    insert_header = TRUE;

    header_name = g_strdup("X-Test-Header");
    header_value = g_strdup("Test Value");
    header_index = 2;
    milter_command_encoder_encode_end_of_message(command_encoder,
                                                 &packet, &packet_size,
                                                 NULL, 0);
    gcut_assert_error(feed());

    packet_free();
    milter_reply_encoder_encode_insert_header(reply_encoder,
                                              &packet, &packet_size,
                                              header_index,
                                              header_name, header_value);
    actual_data = gcut_string_io_channel_get_string(channel);
    cut_assert_equal_memory(packet, packet_size,
                            actual_data->str, actual_data->len);
}

void
test_change_header (void)
{
    GString *actual_data;

    change_header = TRUE;

    header_name = g_strdup("X-Test-Header");
    header_value = g_strdup("Test Value");
    header_index = 2;
    milter_command_encoder_encode_end_of_message(command_encoder,
                                                 &packet, &packet_size,
                                                 NULL, 0);
    gcut_assert_error(feed());

    packet_free();
    milter_reply_encoder_encode_change_header(reply_encoder,
                                              &packet, &packet_size,
                                              header_name,
                                              header_index,
                                              header_value);
    actual_data = gcut_string_io_channel_get_string(channel);
    cut_assert_equal_memory(packet, packet_size,
                            actual_data->str, actual_data->len);
}

void
test_delete_header (void)
{
    GString *actual_data;

    delete_header = TRUE;

    header_name = g_strdup("X-Test-Header");
    header_index = 2;
    milter_command_encoder_encode_end_of_message(command_encoder,
                                                 &packet, &packet_size,
                                                 NULL, 0);
    gcut_assert_error(feed());

    packet_free();
    milter_reply_encoder_encode_delete_header(reply_encoder,
                                              &packet, &packet_size,
                                              header_name,
                                              header_index);
    actual_data = gcut_string_io_channel_get_string(channel);
    cut_assert_equal_memory(packet, packet_size,
                            actual_data->str, actual_data->len);
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
