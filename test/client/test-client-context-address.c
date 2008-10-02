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

#include <milter-client.h>
#undef shutdown

void data_change_from (void);
void test_change_from (gconstpointer data);
void data_add_receipt (void);
void test_add_receipt (gconstpointer data);
void data_delete_receipt (void);
void test_delete_receipt (gconstpointer data);

static MilterClientContext *context;
static MilterEncoder *encoder;

static GString *output;
static MilterWriter *writer;

static gchar *packet;
static gsize packet_size;

static gboolean do_change_from;
static gboolean do_add_receipt;
static gboolean do_delete_receipt;

static gchar *change_from;
static gchar *change_from_parameters;
static gchar *add_receipt;
static gchar *add_receipt_parameters;
static gchar *delete_receipt;

static MilterStatus
cb_end_of_message (MilterClientContext *context, gpointer user_data)
{
    if (do_change_from) {
        milter_client_context_change_from(context,
                                          change_from, change_from_parameters);
        milter_handler_set_writer(MILTER_HANDLER(context), NULL);
    }

    if (do_add_receipt) {
        milter_client_context_add_receipt(context,
                                          add_receipt, add_receipt_parameters);
        milter_handler_set_writer(MILTER_HANDLER(context), NULL);
    }

    if (do_delete_receipt) {
        milter_client_context_delete_receipt(context, delete_receipt);
        milter_handler_set_writer(MILTER_HANDLER(context), NULL);
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
    output = g_string_new(NULL);
    writer = milter_writer_string_new(output);
    milter_handler_set_writer(MILTER_HANDLER(context), writer);
    setup_signals(context);

    encoder = milter_encoder_new();
    packet = NULL;
    packet_size = 0;

    do_change_from = FALSE;
    do_add_receipt = FALSE;
    do_delete_receipt = FALSE;

    change_from = NULL;
    change_from_parameters = NULL;
    add_receipt = NULL;
    add_receipt_parameters = NULL;
    delete_receipt = NULL;
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

    if (change_from)
        g_free(change_from);
    if (change_from_parameters)
        g_free(change_from_parameters);
    if (add_receipt)
        g_free(add_receipt);
    if (add_receipt_parameters)
        g_free(add_receipt_parameters);
    if (delete_receipt)
        g_free(delete_receipt);
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
    const AddressTestData *test_data = data;

    do_change_from = TRUE;

    change_from = g_strdup(test_data->address);
    change_from_parameters = g_strdup(test_data->parameters);
    milter_encoder_encode_end_of_message(encoder, &packet, &packet_size,
                                         NULL, 0);
    gcut_assert_error(feed());

    packet_free();
    if (change_from)
        milter_encoder_encode_reply_change_from(encoder, &packet, &packet_size,
                                                change_from,
                                                change_from_parameters);
    cut_assert_equal_memory(packet, packet_size,
                            output->str, output->len);
}

void
data_add_receipt (void)
{
    cut_add_data("receipt",
                 address_test_data_new("kou@localhost", NULL),
                 address_test_data_free,
                 "receipt - parameters",
                 address_test_data_new("kou@localhost", "XXX"),
                 address_test_data_free,
                 "no receipt",
                 address_test_data_new(NULL, NULL),
                 address_test_data_free,
                 "no receipt - parameters",
                 address_test_data_new(NULL, "XXX"),
                 address_test_data_free);
}

void
test_add_receipt (gconstpointer data)
{
    const AddressTestData *test_data = data;

    do_add_receipt = TRUE;

    add_receipt = g_strdup(test_data->address);
    add_receipt_parameters = g_strdup(test_data->parameters);
    milter_encoder_encode_end_of_message(encoder, &packet, &packet_size,
                                         NULL, 0);
    gcut_assert_error(feed());

    packet_free();
    if (add_receipt)
        milter_encoder_encode_reply_add_receipt(encoder, &packet, &packet_size,
                                                add_receipt,
                                                add_receipt_parameters);
    cut_assert_equal_memory(packet, packet_size,
                            output->str, output->len);
}

void
data_delete_receipt (void)
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
test_delete_receipt (gconstpointer data)
{
    const gchar *receipt = data;

    do_delete_receipt = TRUE;

    delete_receipt = g_strdup(receipt);
    milter_encoder_encode_end_of_message(encoder, &packet, &packet_size,
                                         NULL, 0);
    gcut_assert_error(feed());

    packet_free();
    if (delete_receipt)
        milter_encoder_encode_reply_delete_receipt(encoder,
                                                   &packet, &packet_size,
                                                   delete_receipt);
    cut_assert_equal_memory(packet, packet_size,
                            output->str, output->len);
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
