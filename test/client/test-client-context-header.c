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

void test_add_header (void);
void test_insert_header (void);
void test_change_header (void);
void test_remove_header (void);

static MilterClientContext *context;
static MilterEncoder *encoder;

static GString *output;
static MilterWriter *writer;

static gchar *packet;
static gsize packet_size;

static gboolean add_header;
static gboolean insert_header;
static gboolean change_header;
static gboolean remove_header;

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
cb_envelope_receipt (MilterClientContext *context, const gchar *to,
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
        milter_handler_set_writer(MILTER_HANDLER(context), NULL);
    }

    if (insert_header) {
        milter_client_context_insert_header(context, header_index,
                                            header_name, header_value);
        milter_handler_set_writer(MILTER_HANDLER(context), NULL);
    }

    if (change_header) {
        milter_client_context_change_header(context,
                                            header_name,
                                            header_index,
                                            header_value);
        milter_handler_set_writer(MILTER_HANDLER(context), NULL);
    }

    if (remove_header) {
        milter_client_context_remove_header(context, header_name, header_index);
        milter_handler_set_writer(MILTER_HANDLER(context), NULL);
    }

    return MILTER_STATUS_CONTINUE;
}

static MilterStatus
cb_abort (MilterClientContext *context, gpointer user_data)
{
    return MILTER_STATUS_CONTINUE;
}

static MilterStatus
cb_close (MilterClientContext *context, gpointer user_data)
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
    CONNECT(envelope_receipt);
    CONNECT(header);
    CONNECT(end_of_header);
    CONNECT(body);
    CONNECT(end_of_message);
    CONNECT(abort);
    CONNECT(close);
    CONNECT(unknown);

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

    header_name = NULL;
    header_value = NULL;
    header_index = 0;

    add_header = FALSE;
    insert_header = FALSE;
    change_header = FALSE;
    remove_header = FALSE;
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

    milter_handler_feed(MILTER_HANDLER(context), packet, packet_size, &error);

    return error;
}

void
test_add_header (void)
{
    add_header = TRUE;
    cut_assert_equal_memory("", 0, output->str, output->len);
    header_name = g_strdup("X-Test-Header");
    header_value = g_strdup("Test Value");
    milter_encoder_encode_end_of_message(encoder, &packet, &packet_size,
                                         NULL, 0);
    gcut_assert_error(feed());

    packet_free();
    milter_encoder_encode_reply_add_header(encoder, &packet, &packet_size,
                                           header_name, header_value);
    cut_assert_equal_memory(packet, packet_size,
                            output->str, output->len);
}

void
test_insert_header (void)
{
    insert_header = TRUE;
    cut_assert_equal_memory("", 0, output->str, output->len);
    header_name = g_strdup("X-Test-Header");
    header_value = g_strdup("Test Value");
    header_index = 2;
    milter_encoder_encode_end_of_message(encoder, &packet, &packet_size,
                                         NULL, 0);
    gcut_assert_error(feed());

    packet_free();
    milter_encoder_encode_reply_insert_header(encoder, &packet, &packet_size,
                                              header_index,
                                              header_name, header_value);
    cut_assert_equal_memory(packet, packet_size,
                            output->str, output->len);
}

void
test_change_header (void)
{
    change_header = TRUE;
    cut_assert_equal_memory("", 0, output->str, output->len);
    header_name = g_strdup("X-Test-Header");
    header_value = g_strdup("Test Value");
    header_index = 2;
    milter_encoder_encode_end_of_message(encoder, &packet, &packet_size,
                                         NULL, 0);
    gcut_assert_error(feed());

    packet_free();
    milter_encoder_encode_reply_change_header(encoder, &packet, &packet_size,
                                              header_name,
                                              header_index,
                                              header_value);
    cut_assert_equal_memory(packet, packet_size,
                            output->str, output->len);
}

void
test_remove_header (void)
{
    remove_header = TRUE;
    cut_assert_equal_memory("", 0, output->str, output->len);
    header_name = g_strdup("X-Test-Header");
    header_value = NULL;
    header_index = 2;
    milter_encoder_encode_end_of_message(encoder, &packet, &packet_size,
                                         NULL, 0);
    gcut_assert_error(feed());

    packet_free();
    milter_encoder_encode_reply_change_header(encoder, &packet, &packet_size,
                                              header_name,
                                              header_index,
                                              header_value);
    cut_assert_equal_memory(packet, packet_size,
                            output->str, output->len);
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
