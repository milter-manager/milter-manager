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

void test_private (void);
void data_set_reply (void);
void test_set_reply (gconstpointer data);

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

static GList *private_data_list;
static GList *expected_private_data_list;

static void
append_private_data (MilterClientContext *context, const gchar *tag)
{
    private_data_list = milter_client_context_get_private_data(context);
    private_data_list = g_list_append(private_data_list, g_strdup(tag));
    milter_client_context_set_private_data(context,
                                           private_data_list,
                                           NULL);
}

static MilterClientStatus
cb_option_negotiation (MilterClientContext *context, MilterOption *option,
                       gpointer user_data)
{
    n_option_negotiations++;
    append_private_data(context, "option negotiation");

    return MILTER_CLIENT_STATUS_CONTINUE;
}

static MilterClientStatus
cb_connect (MilterClientContext *context, const gchar *host_name,
            const struct sockaddr *address, socklen_t address_size,
            gpointer user_data)
{
    n_connects++;
    append_private_data(context, "connect");

    return MILTER_CLIENT_STATUS_CONTINUE;
}

static MilterClientStatus
cb_helo (MilterClientContext *context, const gchar *fqdn, gpointer user_data)
{
    n_helos++;
    append_private_data(context, "helo");

    return MILTER_CLIENT_STATUS_CONTINUE;
}

static MilterClientStatus
cb_envelope_from (MilterClientContext *context, const gchar *from,
                  gpointer user_data)
{
    n_envelope_froms++;
    append_private_data(context, "envelope from");

    return MILTER_CLIENT_STATUS_CONTINUE;
}

static MilterClientStatus
cb_envelope_receipt (MilterClientContext *context, const gchar *to,
                     gpointer user_data)
{
    n_envelope_receipts++;
    append_private_data(context, "envelope receipt");

    return MILTER_CLIENT_STATUS_CONTINUE;
}

static MilterClientStatus
cb_header (MilterClientContext *context, const gchar *name, const gchar *value,
           gpointer user_data)
{
    n_headers++;
    append_private_data(context, "header");

    return MILTER_CLIENT_STATUS_CONTINUE;
}

static MilterClientStatus
cb_end_of_header (MilterClientContext *context, gpointer user_data)
{
    n_end_of_headers++;
    append_private_data(context, "end of header");

    return MILTER_CLIENT_STATUS_CONTINUE;
}

static MilterClientStatus
cb_body (MilterClientContext *context, const gchar *chunk, gsize size,
         gpointer user_data)
{
    n_bodies++;
    append_private_data(context, "body");

    return MILTER_CLIENT_STATUS_CONTINUE;
}

static MilterClientStatus
cb_end_of_message (MilterClientContext *context, gpointer user_data)
{
    n_end_of_messages++;
    append_private_data(context, "end of message");

    return MILTER_CLIENT_STATUS_CONTINUE;
}

static MilterClientStatus
cb_abort (MilterClientContext *context, gpointer user_data)
{
    n_aborts++;
    append_private_data(context, "abort");

    return MILTER_CLIENT_STATUS_CONTINUE;
}

static MilterClientStatus
cb_close (MilterClientContext *context, gpointer user_data)
{
    n_closes++;
    append_private_data(context, "close");

    return MILTER_CLIENT_STATUS_CONTINUE;
}

static MilterClientStatus
cb_unknown (MilterClientContext *context, const gchar *command,
            gpointer user_data)
{
    n_unknowns++;
    append_private_data(context, "unknown");

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

    private_data_list = NULL;
    expected_private_data_list = NULL;
}

static void
packet_free (void)
{
    if (packet)
        g_free(packet);
    packet = NULL;
    packet_size = 0;
}

static void
errors_free (void)
{
    if (expected_error) {
        g_error_free(expected_error);
        expected_error = NULL;
    }

    if (actual_error) {
        g_error_free(actual_error);
        expected_error = NULL;
    }
}

void
teardown (void)
{
    if (context)
        g_object_unref(context);

    if (encoder)
        g_object_unref(encoder);

    packet_free();

    errors_free();

    if (private_data_list)
        gcut_list_string_free(private_data_list);
    if (expected_private_data_list)
        gcut_list_string_free(expected_private_data_list);
}

typedef void (*HookFunction) (void);

static GError *
feed (void)
{
    GError *error = NULL;

    milter_client_context_feed(context, packet, packet_size, &error);

    return error;
}

static GList *
append_expected_private_data (const gchar *tag)
{
    expected_private_data_list = g_list_append(expected_private_data_list,
                                               g_strdup(tag));
    return expected_private_data_list;
}

static void
create_option_negotiate_packet (void)
{
    MilterOption *option;

    option = milter_option_new(2, MILTER_ACTION_ADD_HEADERS, MILTER_STEP_NONE);
    packet_free();
    milter_encoder_encode_option_negotiation(encoder,
                                             &packet, &packet_size,
                                             option);
    g_object_unref(option);
}

static void
create_connect_packet_ipv4 (void)
{
    struct sockaddr_in address;
    const gchar host_name[] = "mx.local.net";
    const gchar ip_address[] = "192.168.123.123";
    guint16 port;

    port = htons(50443);
    address.sin_family = AF_INET;
    address.sin_port = port;
    inet_aton(ip_address, &(address.sin_addr));

    packet_free();
    milter_encoder_encode_connect(encoder,
                                  &packet, &packet_size,
                                  host_name,
                                  (const struct sockaddr *)&address,
                                  sizeof(address));
}

static void
create_helo_packet (void)
{
    const gchar fqdn[] = "delian";

    packet_free();
    milter_encoder_encode_helo(encoder, &packet, &packet_size, fqdn);
}

void
test_private (void)
{
    create_option_negotiate_packet();
    gcut_assert_equal_list_string(NULL, private_data_list);
    gcut_assert_error(feed());
    cut_assert_equal_int(1, n_option_negotiations);
    append_expected_private_data("option negotiation");
    gcut_assert_equal_list_string(expected_private_data_list,
                                  private_data_list);

    create_connect_packet_ipv4();
    gcut_assert_error(feed());
    cut_assert_equal_int(1, n_connects);
    append_expected_private_data("connect");
    gcut_assert_equal_list_string(expected_private_data_list,
                                  private_data_list);

    create_helo_packet();
    gcut_assert_error(feed());
    cut_assert_equal_int(1, n_helos);
    append_expected_private_data("helo");
    gcut_assert_equal_list_string(expected_private_data_list,
                                  private_data_list);
}

typedef struct _SetReplyTestData
{
    GError *expected_error;
    guint code;
    gchar *extended_code;
    gchar *message;
} SetReplyTestData;

static SetReplyTestData *
set_reply_test_data_new (GError *expected_error,
                         guint code, const gchar *extended_code,
                         const gchar *message)
{
    SetReplyTestData *data;

    data = g_new(SetReplyTestData, 1);
    data->expected_error = expected_error;
    data->code = code;
    data->extended_code = g_strdup(extended_code);
    data->message = g_strdup(message);

    return data;
}

static void
set_reply_test_data_free (SetReplyTestData *data)
{
    if (data->expected_error)
        g_error_free(data->expected_error);
    if (data->extended_code)
        g_free(data->extended_code);
    if (data->message)
        g_free(data->message);
    g_free(data);
}


static GError *
set_reply (guint code, const gchar *extended_code, const gchar *message)
{
    GError *error = NULL;

    milter_client_context_set_reply(context, code, extended_code, message,
                                    &error);

    return error;
}

void
data_set_reply (void)
{
    cut_add_data("success - no extended",
                 set_reply_test_data_new(NULL,
                                         450, NULL, "rejected"),
                 set_reply_test_data_free,
                 "success - extended",
                 set_reply_test_data_new(NULL,
                                         450, "4.2.0", "rejected by Greylist"),
                 set_reply_test_data_free);

    cut_add_data("fail - invalid code range",
                 set_reply_test_data_new(
                     g_error_new(MILTER_CLIENT_CONTEXT_ERROR,
                                 MILTER_CLIENT_CONTEXT_ERROR_INVALID_CODE,
                                 "return code should be 4XX or 5XX: <399>"),
                     399, NULL, "message"),
                 set_reply_test_data_free,
                 "fail - extended code is miss match",
                 set_reply_test_data_new(
                     g_error_new(MILTER_CLIENT_CONTEXT_ERROR,
                                 MILTER_CLIENT_CONTEXT_ERROR_INVALID_CODE,
                                 "extended code should use the same class of "
                                 "status code: <450>:<5.7.1>"),
                     450, "5.7.1", "Forwarding to remote hosts disabled"),
                 set_reply_test_data_free);

    cut_add_data("fail - empty extended code",
                 set_reply_test_data_new(
                     g_error_new(MILTER_CLIENT_CONTEXT_ERROR,
                                 MILTER_CLIENT_CONTEXT_ERROR_INVALID_CODE,
                                 "class of extended status code is missing: <>"),
                     551, "", "Forwarding to remote hosts disabled"),
                 set_reply_test_data_free,
                 "fail - class only extended code",
                 set_reply_test_data_new(
                     g_error_new(MILTER_CLIENT_CONTEXT_ERROR,
                                 MILTER_CLIENT_CONTEXT_ERROR_INVALID_CODE,
                                 "subject of extended status code is missing: "
                                 "<5>"),
                     551, "5", "Forwarding to remote hosts disabled"),
                 set_reply_test_data_free,
                 "fail - class with dot only extended code",
                 set_reply_test_data_new(
                     g_error_new(MILTER_CLIENT_CONTEXT_ERROR,
                                 MILTER_CLIENT_CONTEXT_ERROR_INVALID_CODE,
                                 "subject of extended status code is missing: "
                                 "<5.>"),
                     551, "5.", "Forwarding to remote hosts disabled"),
                 set_reply_test_data_free,
                 "fail - class and subject only extended code",
                 set_reply_test_data_new(
                     g_error_new(MILTER_CLIENT_CONTEXT_ERROR,
                                 MILTER_CLIENT_CONTEXT_ERROR_INVALID_CODE,
                                 "detail of extended status code is missing: "
                                 "<5.7>"),
                     551, "5.7", "Forwarding to remote hosts disabled"),
                 set_reply_test_data_free,
                 "fail - class and subject with dot only extended code",
                 set_reply_test_data_new(
                     g_error_new(MILTER_CLIENT_CONTEXT_ERROR,
                                 MILTER_CLIENT_CONTEXT_ERROR_INVALID_CODE,
                                 "detail of extended status code is missing: "
                                 "<5.7.>"),
                     551, "5.7.", "Forwarding to remote hosts disabled"),
                 set_reply_test_data_free);

    cut_add_data("fail - large class extended code",
                 set_reply_test_data_new(
                     g_error_new(MILTER_CLIENT_CONTEXT_ERROR,
                                 MILTER_CLIENT_CONTEXT_ERROR_INVALID_CODE,
                                 "class of extended status code should be "
                                 "'2', '4' or '5': <55.7.1>"),
                     551, "55.7.1", "Forwarding to remote hosts disabled"),
                 set_reply_test_data_free,
                 "fail - large subject extended code",
                 set_reply_test_data_new(
                     g_error_new(MILTER_CLIENT_CONTEXT_ERROR,
                                 MILTER_CLIENT_CONTEXT_ERROR_INVALID_CODE,
                                 "subject of extended status code should be "
                                 "less than 1000: <7489>: <5.7489.1>"),
                     551, "5.7489.1", "Forwarding to remote hosts disabled"),
                 set_reply_test_data_free,
                 "fail - large detail extended code",
                 set_reply_test_data_new(
                     g_error_new(MILTER_CLIENT_CONTEXT_ERROR,
                                 MILTER_CLIENT_CONTEXT_ERROR_INVALID_CODE,
                                 "detail of extended status code should be "
                                 "less than 1000: <1234>: <5.7.1234>"),
                     551, "5.7.1234", "Forwarding to remote hosts disabled"),
                 set_reply_test_data_free);
}

void
test_set_reply (gconstpointer data)
{
    const SetReplyTestData *test_data = data;

    actual_error = set_reply(test_data->code,
                             test_data->extended_code,
                             test_data->message);
    gcut_assert_equal_error(test_data->expected_error, actual_error);
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
