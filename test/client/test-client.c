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

#include <errno.h>

#include <gcutter.h>

#define shutdown inet_shutdown
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <milter/client.h>
#include <milter-test-utils.h>
#undef shutdown

void test_negotiate (void);
void test_helo (void);

static MilterClient *client;
static MilterTestServer *server;

static MilterDecoder *decoder;
static MilterCommandEncoder *encoder;

static gint n_negotiates;
static gint n_connects;
static gint n_helos;
static gint n_envelope_froms;
static gint n_envelope_recipients;
static gint n_datas;
static gint n_headers;
static gint n_end_of_headers;
static gint n_bodies;
static gint n_end_of_messages;
static gint n_aborts;
static gint n_unknowns;
static gint n_finished_emissions;

static guint idle_id;
static guint idle_shutdown_id;
static guint shutdown_count;

static const gchar spec[] = "inet:9999@127.0.0.1";

static gchar *packet;

static MilterOption *option;
static MilterOption *negotiate_option;

static MilterMacrosRequests *macros_requests;
static MilterMacrosRequests *negotiate_macros_requests;

static gchar *connect_host_name;
static struct sockaddr *connect_address;
static socklen_t connect_address_length;

static const gchar fqdn[] = "delian";
static gchar *helo_fqdn;

static gchar *envelope_from;

static gchar *envelope_recipient_to;

static gchar *header_name;
static gchar *header_value;

static gchar *body_chunk;
static gsize body_chunk_length;

static gchar *unknown_command;
static gsize unknown_command_length;

static GError *actual_error;
static GError *expected_error;

static void
cb_negotiate (MilterClientContext *context, MilterOption *option,
              MilterMacrosRequests *macros_requests, gpointer user_data)
{
    n_negotiates++;
    negotiate_option = g_object_ref(option);
    negotiate_macros_requests = g_object_ref(macros_requests);
}

static void
cb_connect (MilterClientContext *context, const gchar *host_name,
            const struct sockaddr *address, socklen_t address_length,
            gpointer user_data)
{
    n_connects++;

    connect_host_name = g_strdup(host_name);
    connect_address = malloc(address_length);
    memcpy(connect_address, address, address_length);
    connect_address_length = address_length;
}

static void
cb_helo (MilterClientContext *context, const gchar *fqdn, gpointer user_data)
{
    n_helos++;

    helo_fqdn = g_strdup(fqdn);
}

static void
cb_envelope_from (MilterClientContext *context, const gchar *from, gpointer user_data)
{
    n_envelope_froms++;

    envelope_from = g_strdup(from);
}

static void
cb_envelope_recipient (MilterClientContext *context, const gchar *to,
                       gpointer user_data)
{
    n_envelope_recipients++;

    envelope_recipient_to = g_strdup(to);
}

static void
cb_data (MilterClientContext *context, gpointer user_data)
{
    n_datas++;
}

static void
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
}

static void
cb_end_of_header (MilterClientContext *context, gpointer user_data)
{
    n_end_of_headers++;
}

static void
cb_body (MilterClientContext *context, const gchar *chunk, gsize length,
         gpointer user_data)
{
    n_bodies++;

    if (body_chunk)
        g_free(body_chunk);
    body_chunk = g_strndup(chunk, length);
    body_chunk_length = length;
}

static void
cb_end_of_message (MilterClientContext *context, gpointer user_data)
{
    n_end_of_messages++;
}

static void
cb_abort (MilterClientContext *context, gpointer user_data)
{
    n_aborts++;
}

static void
cb_unknown (MilterClientContext *context, const gchar *command,
            gpointer user_data)
{
    n_unknowns++;

    if (unknown_command)
        g_free(unknown_command);
    unknown_command = g_strdup(command);
}

static void
cb_finished (MilterFinishedEmittable *emittable, gpointer user_data)
{
    n_finished_emissions++;
}

static void
setup_context_signals (MilterClientContext *context)
{
#define CONNECT(name)                                                   \
    g_signal_connect(context, #name, G_CALLBACK(cb_ ## name), NULL)

    CONNECT(negotiate);
    CONNECT(connect);
    CONNECT(helo);
    CONNECT(envelope_from);
    CONNECT(envelope_recipient);
    CONNECT(data);
    CONNECT(header);
    CONNECT(end_of_header);
    CONNECT(body);
    CONNECT(end_of_message);
    CONNECT(abort);
    CONNECT(unknown);

    CONNECT(finished);
#undef CONNECT
}

static void
cb_connection_established (MilterClient *client, MilterClientContext *context,
                           gpointer user_data)
{
    setup_context_signals(context);
}

static void
cb_error (MilterErrorEmittable *emittable, GError *error,
          gpointer user_data)
{
    actual_error = g_error_copy(error);
}

static void
setup_client_signals (void)
{
#define CONNECT(name)                                                   \
    g_signal_connect(client, #name, G_CALLBACK(cb_ ## name), NULL)

    CONNECT(connection_established);
    CONNECT(error);

#undef CONNECT
}

void
setup (void)
{
    client = milter_client_new();
    setup_client_signals();
    server = NULL;

    decoder = milter_reply_decoder_new();
    encoder = MILTER_COMMAND_ENCODER(milter_command_encoder_new());

    packet = NULL;

    option = NULL;
    negotiate_option = NULL;

    macros_requests = NULL;
    negotiate_macros_requests = NULL;

    idle_id = 0;
    idle_shutdown_id = 0;
    shutdown_count = 10;

    n_negotiates = 0;
    n_connects = 0;
    n_helos = 0;
    n_envelope_froms = 0;
    n_envelope_recipients = 0;
    n_datas = 0;
    n_headers = 0;
    n_end_of_headers = 0;
    n_bodies = 0;
    n_end_of_messages = 0;
    n_aborts = 0;
    n_unknowns = 0;

    n_finished_emissions = 0;

    connect_host_name = NULL;
    connect_address = NULL;
    connect_address_length = 0;

    helo_fqdn = NULL;

    envelope_from = NULL;

    envelope_recipient_to = NULL;

    header_name = NULL;
    header_value = NULL;

    body_chunk = NULL;
    body_chunk_length = 0;

    unknown_command = NULL;
    unknown_command_length = 0;

    actual_error = NULL;
    expected_error = NULL;
}

void
teardown (void)
{
    if (idle_id > 0)
        g_source_remove(idle_id);
    if (idle_shutdown_id > 0)
        g_source_remove(idle_shutdown_id);

    if (client)
        g_object_unref(client);
    if (server)
        g_object_unref(server);

    if (decoder)
        g_object_unref(decoder);
    if (encoder)
        g_object_unref(encoder);

    if (packet)
        g_free(packet);

    if (option)
        g_object_unref(option);
    if (negotiate_option)
        g_object_unref(negotiate_option);

    if (macros_requests)
        g_object_unref(macros_requests);
    if (negotiate_macros_requests)
        g_object_unref(negotiate_macros_requests);

    if (connect_host_name)
        g_free(connect_host_name);
    if (connect_address)
        g_free(connect_address);

    if (helo_fqdn)
        g_free(helo_fqdn);

    if (envelope_from)
        g_free(envelope_from);

    if (envelope_recipient_to)
        g_free(envelope_recipient_to);

    if (header_name)
        g_free(header_name);
    if (header_value)
        g_free(header_value);

    if (body_chunk)
        g_free(body_chunk);

    if (unknown_command)
        g_free(unknown_command);

    if (actual_error)
        g_error_free(actual_error);
    if (expected_error)
        g_error_free(expected_error);
}

static gboolean
cb_idle_shutdown (gpointer user_data)
{
    shutdown_count--;
    if (shutdown_count == 0) {
        milter_client_shutdown(client);
        if (server) {
            g_object_unref(server);
            server = NULL;
        }
    }

    return shutdown_count > 0;
}

static void
setup_client (void)
{
    GError *error = NULL;

    milter_client_set_connection_spec(client, spec, &error);
    gcut_assert_error(error);

    idle_shutdown_id = g_idle_add_full(G_PRIORITY_LOW, cb_idle_shutdown,
                                       NULL, NULL);
}

static void
setup_test_server (void)
{
    cut_trace(server = milter_test_server_new(spec, decoder));
}

static gboolean
cb_idle_negotiate (gpointer user_data)
{
    gsize packet_size;

    cut_trace(setup_test_server());
    milter_command_encoder_encode_negotiate(encoder,
                                            &packet, &packet_size, option);
    milter_test_server_write(server, packet, packet_size);

    return FALSE;
}

void
test_negotiate (void)
{
    guint32 version;
    MilterActionFlags action;
    MilterStepFlags step;

    version = 2;
    action = MILTER_ACTION_ADD_HEADERS |
        MILTER_ACTION_CHANGE_BODY |
        MILTER_ACTION_ADD_ENVELOPE_RECIPIENT |
        MILTER_ACTION_DELETE_ENVELOPE_RECIPIENT |
        MILTER_ACTION_CHANGE_HEADERS |
        MILTER_ACTION_QUARANTINE |
        MILTER_ACTION_SET_SYMBOL_LIST;
    step = MILTER_STEP_NO_CONNECT |
        MILTER_STEP_NO_HELO |
        MILTER_STEP_NO_ENVELOPE_FROM |
        MILTER_STEP_NO_ENVELOPE_RECIPIENT |
        MILTER_STEP_NO_BODY |
        MILTER_STEP_NO_HEADERS |
        MILTER_STEP_NO_END_OF_HEADER;
    option = milter_option_new(version, action, step);

    idle_id = g_idle_add(cb_idle_negotiate, NULL);

    cut_trace(setup_client());
    if (!milter_client_main(client))
        cut_assert_errno();

    milter_assert_equal_option(option, negotiate_option);

    macros_requests = milter_macros_requests_new();
    milter_assert_equal_macros_requests(macros_requests,
                                        negotiate_macros_requests);
}

static gboolean
cb_idle_helo (gpointer user_data)
{
    gchar *packet;
    gsize packet_size;

    cut_trace(setup_test_server());
    milter_command_encoder_encode_helo(encoder, &packet, &packet_size, fqdn);
    milter_test_server_write(server, packet, packet_size);

    return FALSE;
}

void
test_helo (void)
{
    idle_id = g_idle_add(cb_idle_helo, NULL);

    cut_trace(setup_client());
    if (!milter_client_main(client))
        cut_assert_errno();

    cut_assert_equal_string(fqdn, helo_fqdn);
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
