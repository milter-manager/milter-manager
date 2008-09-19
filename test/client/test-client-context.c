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

void test_feed_option_negotiation (void);
void test_feed_connect_ipv4 (void);
void test_feed_helo (void);
void test_feed_envelope_from (void);
void test_feed_envelope_receipt (void);
void test_feed_header (void);

static MilterClientContext *context;
static MilterEncoder *encoder;

static gchar *packet;
static gsize packet_size;

static GError *expected_error;
static GError *actual_error;

static gint n_option_negotiations;
static gint n_connects;
static gint n_helos;
static gint n_envelope_froms;
static gint n_envelope_receipts;
static gint n_headers;
static gint n_end_of_headers;
static gint n_bodies;
static gint n_end_of_messages;
static gint n_aborts;
static gint n_closes;
static gint n_unknowns;

static MilterOption *option_negotiation_option;

static GHashTable *defined_macros;

static gchar *connect_host_name;
static struct sockaddr *connect_address;
static socklen_t connect_address_length;

static gchar *helo_fqdn;

static gchar *envelope_from_address;

static gchar *envelope_receipt_address;

static gchar *header_name;
static gchar *header_value;

static gchar *body_chunk;
static gsize body_chunk_length;

static gchar *unknown_command;
static gsize unknown_command_length;

static MilterClientStatus
cb_option_negotiation (MilterClientContext *context, MilterOption *option,
                       gpointer user_data)
{
    n_option_negotiations++;
    option_negotiation_option = g_object_ref(option);

    return MILTER_CLIENT_STATUS_CONTINUE;
}

static MilterClientStatus
cb_connect (MilterClientContext *context, const gchar *host_name,
            const struct sockaddr *address, socklen_t address_length,
            gpointer user_data)
{
    n_connects++;

    connect_host_name = g_strdup(host_name);
    connect_address = malloc(address_length);
    memcpy(connect_address, address, address_length);
    connect_address_length = address_length;

    return MILTER_CLIENT_STATUS_CONTINUE;
}

static MilterClientStatus
cb_helo (MilterClientContext *context, const gchar *fqdn, gpointer user_data)
{
    n_helos++;

    helo_fqdn = g_strdup(fqdn);

    return MILTER_CLIENT_STATUS_CONTINUE;
}

static MilterClientStatus
cb_envelope_from (MilterClientContext *context, const gchar *from,
                  gpointer user_data)
{
    n_envelope_froms++;

    envelope_from_address = g_strdup(from);

    return MILTER_CLIENT_STATUS_CONTINUE;
}

static MilterClientStatus
cb_envelope_receipt (MilterClientContext *context, const gchar *to,
                     gpointer user_data)
{
    n_envelope_receipts++;

    envelope_receipt_address = g_strdup(to);

    return MILTER_CLIENT_STATUS_CONTINUE;
}

static MilterClientStatus
cb_header (MilterClientContext *context, const gchar *name, const gchar *value,
           gpointer user_data)
{
    n_headers++;

    if (header_name)
        g_free(header_name);
    header_name = g_strdup(name);

    if (header_value)
        g_free(header_value);
    header_value = g_strdup(value);

    return MILTER_CLIENT_STATUS_CONTINUE;
}

static MilterClientStatus
cb_end_of_header (MilterClientContext *context, gpointer user_data)
{
    n_end_of_headers++;

    return MILTER_CLIENT_STATUS_CONTINUE;
}

static MilterClientStatus
cb_body (MilterClientContext *context, const gchar *chunk, gsize length,
         gpointer user_data)
{
    n_bodies++;

    if (body_chunk)
        g_free(body_chunk);
    body_chunk = g_strndup(chunk, length);
    body_chunk_length = length;

    return MILTER_CLIENT_STATUS_CONTINUE;
}

static MilterClientStatus
cb_end_of_message (MilterClientContext *context, gpointer user_data)
{
    n_end_of_messages++;

    return MILTER_CLIENT_STATUS_CONTINUE;
}

static MilterClientStatus
cb_abort (MilterClientContext *context, gpointer user_data)
{
    n_aborts++;

    return MILTER_CLIENT_STATUS_CONTINUE;
}

static MilterClientStatus
cb_close (MilterClientContext *context, gpointer user_data)
{
    n_closes++;

    return MILTER_CLIENT_STATUS_CONTINUE;
}

static MilterClientStatus
cb_unknown (MilterClientContext *context, const gchar *command,
            gpointer user_data)
{
    n_unknowns++;

    if (unknown_command)
        g_free(unknown_command);
    unknown_command = g_strdup(command);

    return MILTER_CLIENT_STATUS_CONTINUE;
}

static void
setup_signals (MilterClientContext *context)
{
#define CONNECT(name)                                                   \
    g_signal_connect(context, #name, G_CALLBACK(cb_ ## name), NULL)

    CONNECT(option_negotiation);
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
    setup_signals(context);

    encoder = milter_encoder_new();
    packet = NULL;
    packet_size = 0;

    expected_error = NULL;
    actual_error = NULL;

    n_option_negotiations = 0;
    n_connects = 0;
    n_helos = 0;
    n_envelope_froms = 0;
    n_envelope_receipts = 0;
    n_headers = 0;
    n_end_of_headers = 0;
    n_bodies = 0;
    n_end_of_messages = 0;
    n_aborts = 0;
    n_closes = 0;
    n_unknowns = 0;

    option_negotiation_option = NULL;

    defined_macros = NULL;

    connect_host_name = NULL;
    connect_address = NULL;
    connect_address_length = 0;

    helo_fqdn = NULL;

    envelope_from_address = NULL;

    envelope_receipt_address = NULL;

    header_name = NULL;
    header_value = NULL;

    body_chunk = NULL;
    body_chunk_length = 0;

    unknown_command = NULL;
    unknown_command_length = 0;
}

void
teardown (void)
{
    if (context)
        g_object_unref(context);

    if (encoder)
        g_object_unref(encoder);

    if (packet)
        g_free(packet);

    if (option_negotiation_option)
        g_object_unref(option_negotiation_option);

    if (defined_macros)
        g_hash_table_unref(defined_macros);

    if (connect_host_name)
        g_free(connect_host_name);
    if (connect_address)
        g_free(connect_address);

    if (helo_fqdn)
        g_free(helo_fqdn);

    if (envelope_from_address)
        g_free(envelope_from_address);

    if (envelope_receipt_address)
        g_free(envelope_receipt_address);

    if (header_name)
        g_free(header_name);
    if (header_value)
        g_free(header_value);

    if (body_chunk)
        g_free(body_chunk);

    if (unknown_command)
        g_free(unknown_command);
}

static GError *
feed (void)
{
    GError *error = NULL;

    milter_client_context_feed(context, packet, packet_size, &error);

    return error;
}

void
test_feed_option_negotiation (void)
{
    MilterOption *option;
    MilterOption *negotiated_option;
    guint32 version;
    MilterActionFlags action;
    MilterStepFlags step;

    version = 2;
    action = MILTER_ACTION_ADD_HEADERS |
        MILTER_ACTION_CHANGE_BODY |
        MILTER_ACTION_ADD_RCPT |
        MILTER_ACTION_DELETE_RCPT |
        MILTER_ACTION_CHANGE_HEADERS |
        MILTER_ACTION_QUARANTINE |
        MILTER_ACTION_SET_SYMBOL_LIST;
    step = MILTER_STEP_NO_CONNECT |
        MILTER_STEP_NO_HELO |
        MILTER_STEP_NO_MAIL |
        MILTER_STEP_NO_RCPT |
        MILTER_STEP_NO_BODY |
        MILTER_STEP_NO_HEADERS |
        MILTER_STEP_NO_END_OF_HEADER;
    option = milter_option_new(2, action, step);

    milter_encoder_encode_option_negotiation(encoder,
                                             &packet, &packet_size,
                                             option);
    g_object_unref(option);

    gcut_assert_error(feed());
    cut_assert_equal_int(1, n_option_negotiations);

    negotiated_option = option_negotiation_option;
    cut_assert_equal_int(version, milter_option_get_version(negotiated_option));
    gcut_assert_equal_enum(MILTER_TYPE_ACTION_FLAGS,
                           action,
                           milter_option_get_action(negotiated_option));
    gcut_assert_equal_enum(MILTER_TYPE_STEP_FLAGS,
                           step,
                           milter_option_get_step(negotiated_option));
}

void
test_feed_connect_ipv4 (void)
{
    struct sockaddr_in address;
    struct sockaddr_in *connected_address;
    const gchar host_name[] = "mx.local.net";
    const gchar ip_address[] = "192.168.123.123";
    guint16 port;

    port = htons(50443);
    address.sin_family = AF_INET;
    address.sin_port = port;
    inet_aton(ip_address, &(address.sin_addr));

    milter_encoder_encode_connect(encoder,
                                  &packet, &packet_size,
                                  host_name,
                                  (const struct sockaddr *)&address,
                                  sizeof(address));
    gcut_assert_error(feed());
    cut_assert_equal_int(1, n_connects);
    cut_assert_equal_string(host_name, connect_host_name);
    cut_assert_equal_int(sizeof(struct sockaddr_in), connect_address_length);

    connected_address = (struct sockaddr_in *)connect_address;
    cut_assert_equal_int(AF_INET, connected_address->sin_family);
    cut_assert_equal_uint(port, connected_address->sin_port);
    cut_assert_equal_string(ip_address, inet_ntoa(connected_address->sin_addr));
}

void
test_feed_helo (void)
{
    const gchar fqdn[] = "delian";

    milter_encoder_encode_helo(encoder, &packet, &packet_size, fqdn);
    gcut_assert_error(feed());
    cut_assert_equal_int(1, n_helos);
    cut_assert_equal_string(fqdn, helo_fqdn);
}

void
test_feed_envelope_from (void)
{
    const gchar from[] = "<kou@cozmixng.org>";

    milter_encoder_encode_mail(encoder, &packet, &packet_size, from);
    gcut_assert_error(feed());
    cut_assert_equal_int(1, n_envelope_froms);
    cut_assert_equal_string(from, envelope_from_address);
}

void
test_feed_envelope_receipt (void)
{
    const gchar to[] = "<kou@cozmixng.org>";

    milter_encoder_encode_rcpt(encoder, &packet, &packet_size, to);
    gcut_assert_error(feed());
    cut_assert_equal_int(1, n_envelope_receipts);
    cut_assert_equal_string(to, envelope_receipt_address);
}

void
test_feed_header (void)
{
    const gchar date[] = "Fri,  5 Sep 2008 09:19:56 +0900 (JST)";

    milter_encoder_encode_header(encoder, &packet, &packet_size, "Date", date);
    gcut_assert_error(feed());
    cut_assert_equal_int(1, n_headers);
    cut_assert_equal_string("Date", header_name);
    cut_assert_equal_string(date, header_value);
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
