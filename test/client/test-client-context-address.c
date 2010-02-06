/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Copyright (C) 2008-2010  Kouhei Sutou <kou@clear-code.com>
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

void data_change_from (void);
void test_change_from (gconstpointer data);
void data_add_recipient (void);
void test_add_recipient (gconstpointer data);
void data_delete_recipient (void);
void test_delete_recipient (gconstpointer data);

static MilterClientContext *context;
static MilterCommandEncoder *command_encoder;
static MilterReplyEncoder *reply_encoder;

static GIOChannel *channel;
static MilterWriter *writer;

static gchar *packet;
static gsize packet_size;

static gboolean do_change_from;
static gboolean do_add_recipient;
static gboolean do_delete_recipient;

static gchar *change_from;
static gchar *change_from_parameters;
static gchar *add_recipient;
static gchar *add_recipient_parameters;
static gchar *delete_recipient;

static MilterStatus
cb_end_of_message (MilterClientContext *context,
                   const gchar *chunk, gsize size,
                   gpointer user_data)
{
    if (do_change_from) {
        milter_client_context_change_from(context,
                                          change_from, change_from_parameters);
        milter_agent_set_writer(MILTER_AGENT(context), NULL);
    }

    if (do_add_recipient) {
        milter_client_context_add_recipient(context,
                                            add_recipient,
                                            add_recipient_parameters);
        milter_agent_set_writer(MILTER_AGENT(context), NULL);
    }

    if (do_delete_recipient) {
        milter_client_context_delete_recipient(context, delete_recipient);
        milter_agent_set_writer(MILTER_AGENT(context), NULL);
    }

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

    channel = gcut_string_io_channel_new(NULL);
    g_io_channel_set_encoding(channel, NULL, NULL);
    writer = milter_writer_io_channel_new(channel);

    milter_agent_set_writer(MILTER_AGENT(context), writer);
    milter_agent_start(MILTER_AGENT(context));
    setup_signals(context);

    command_encoder = MILTER_COMMAND_ENCODER(milter_command_encoder_new());
    reply_encoder = MILTER_REPLY_ENCODER(milter_reply_encoder_new());
    packet = NULL;
    packet_size = 0;

    do_change_from = FALSE;
    do_add_recipient = FALSE;
    do_delete_recipient = FALSE;

    change_from = NULL;
    change_from_parameters = NULL;
    add_recipient = NULL;
    add_recipient_parameters = NULL;
    delete_recipient = NULL;
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

    if (writer)
        g_object_unref(writer);

    if (channel)
        g_io_channel_unref(channel);

    packet_free();

    if (change_from)
        g_free(change_from);
    if (change_from_parameters)
        g_free(change_from_parameters);
    if (add_recipient)
        g_free(add_recipient);
    if (add_recipient_parameters)
        g_free(add_recipient_parameters);
    if (delete_recipient)
        g_free(delete_recipient);
}

static GError *
feed (void)
{
    GError *error = NULL;

    milter_client_context_feed(context, packet, packet_size, &error);

    return error;
}

typedef struct _AddressTestData
{
    gchar *address;
    gchar *parameters;
} AddressTestData;

static AddressTestData *
address_test_data_new (const gchar *address, const gchar *parameters)
{
    AddressTestData *data;

    data = g_new(AddressTestData, 1);
    data->address = g_strdup(address);
    data->parameters = g_strdup(parameters);

    return data;
}

static void
address_test_data_free (AddressTestData *data)
{
    g_free(data->address);
    g_free(data->parameters);
    g_free(data);
}

void
data_change_from (void)
{
    cut_add_data("from",
                 address_test_data_new("kou@localhost", NULL),
                 address_test_data_free,
                 "from - parameters",
                 address_test_data_new("kou@localhost", "XXX"),
                 address_test_data_free,
                 "no from",
                 address_test_data_new(NULL, NULL),
                 address_test_data_free,
                 "no from - parameters",
                 address_test_data_new(NULL, "XXX"),
                 address_test_data_free);
}

void
test_change_from (gconstpointer data)
{
    GString *actual_data;
    const AddressTestData *test_data = data;

    do_change_from = TRUE;

    change_from = g_strdup(test_data->address);
    change_from_parameters = g_strdup(test_data->parameters);
    milter_command_encoder_encode_end_of_message(command_encoder,
                                                 &packet, &packet_size,
                                                 NULL, 0);
    gcut_assert_error(feed());

    packet_free();
    if (change_from)
        milter_reply_encoder_encode_change_from(reply_encoder,
                                                &packet, &packet_size,
                                                change_from,
                                                change_from_parameters);
    actual_data = gcut_string_io_channel_get_string(channel);
    cut_assert_equal_memory(packet, packet_size,
                            actual_data->str, actual_data->len);
}

void
data_add_recipient (void)
{
    cut_add_data("recipient",
                 address_test_data_new("kou@localhost", NULL),
                 address_test_data_free,
                 "recipient - parameters",
                 address_test_data_new("kou@localhost", "XXX"),
                 address_test_data_free,
                 "no recipient",
                 address_test_data_new(NULL, NULL),
                 address_test_data_free,
                 "no recipient - parameters",
                 address_test_data_new(NULL, "XXX"),
                 address_test_data_free);
}

void
test_add_recipient (gconstpointer data)
{
    GString *actual_data;
    const AddressTestData *test_data = data;

    do_add_recipient = TRUE;

    add_recipient = g_strdup(test_data->address);
    add_recipient_parameters = g_strdup(test_data->parameters);
    milter_command_encoder_encode_end_of_message(command_encoder,
                                                 &packet, &packet_size,
                                                 NULL, 0);
    gcut_assert_error(feed());

    packet_free();
    if (add_recipient)
        milter_reply_encoder_encode_add_recipient(reply_encoder,
                                                  &packet, &packet_size,
                                                  add_recipient,
                                                  add_recipient_parameters);
    actual_data = gcut_string_io_channel_get_string(channel);
    cut_assert_equal_memory(packet, packet_size,
                            actual_data->str, actual_data->len);
}

void
data_delete_recipient (void)
{
    cut_add_data("valid",
                 g_strdup("kou@localhost"),
                 g_free,
                 "NULL",
                 NULL,
                 NULL,
                 "empty",
                 g_strdup(""),
                 g_free);
}

void
test_delete_recipient (gconstpointer data)
{
    GString *actual_data;
    const gchar *recipient = data;

    do_delete_recipient = TRUE;

    delete_recipient = g_strdup(recipient);
    milter_command_encoder_encode_end_of_message(command_encoder,
                                                 &packet, &packet_size,
                                                 NULL, 0);
    gcut_assert_error(feed());

    packet_free();
    if (delete_recipient)
        milter_reply_encoder_encode_delete_recipient(reply_encoder,
                                                     &packet, &packet_size,
                                                     delete_recipient);
    actual_data = gcut_string_io_channel_get_string(channel);
    cut_assert_equal_memory(packet, packet_size,
                            actual_data->str, actual_data->len);
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
