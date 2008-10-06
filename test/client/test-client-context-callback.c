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

void test_feed_negotiate (void);
void test_feed_connect_ipv4 (void);
void test_feed_connect_ipv6 (void);
void test_feed_connect_unix (void);
void test_feed_connect_with_macro (void);
void data_feed_helo (void);
void test_feed_helo (gconstpointer data);
void data_feed_envelope_from (void);
void test_feed_envelope_from (gconstpointer data);
void data_feed_envelope_receipt (void);
void test_feed_envelope_receipt (gconstpointer data);
void data_feed_header (void);
void test_feed_header (gconstpointer data);
void data_feed_end_of_header (void);
void test_feed_end_of_header (gconstpointer data);
void data_feed_body (void);
void test_feed_body (gconstpointer data);
void data_feed_end_of_message (void);
void test_feed_end_of_message (gconstpointer data);
void test_feed_close (void);
void test_feed_abort (void);

static MilterClientContext *context;
static MilterEncoder *encoder;

static gchar *packet;
static gsize packet_size;

static gint n_negotiates;
static gint n_negotiate_responses;
static gint n_connects;
static gint n_connect_responses;
static gint n_helos;
static gint n_helo_responses;
static gint n_envelope_froms;
static gint n_envelope_from_responses;
static gint n_envelope_receipts;
static gint n_envelope_receipt_responses;
static gint n_headers;
static gint n_header_responses;
static gint n_end_of_headers;
static gint n_end_of_header_responses;
static gint n_bodies;
static gint n_body_responses;
static gint n_end_of_messages;
static gint n_end_of_message_responses;
static gint n_aborts;
static gint n_abort_responses;
static gint n_closes;
static gint n_close_responses;
static gint n_unknowns;
static gint n_unknown_responses;

static MilterOption *negotiate_option;

static GHashTable *expected_macros;
static GHashTable *defined_macros;

static gchar *macro_name;
static gchar *macro_value;
static gchar *expected_macro_value;

static gchar *connect_host_name;
static struct sockaddr *connect_address;
static socklen_t connect_address_size;

static gchar *helo_fqdn;

static gchar *envelope_from_address;

static gchar *envelope_receipt_address;

static gchar *header_name;
static gchar *header_value;

static gchar *body_chunk;
static gsize body_chunk_size;
static gchar *body_chunk_test_data;
static gsize body_chunk_test_data_size;

static gchar *unknown_command;
static gsize unknown_command_size;

static void
retrieve_context_info (MilterClientContext *context)
{
    MilterHandler *handler = MILTER_HANDLER(context);

    if (defined_macros)
        g_hash_table_unref(defined_macros);
    defined_macros = milter_handler_get_macros(handler);
    if (macro_name) {
        const gchar *value;

        if (macro_value)
            g_free(macro_value);
        value = milter_handler_get_macro(handler, macro_name);
        if (value)
            macro_value = g_strdup(value);
    }
    if (defined_macros)
        g_hash_table_ref(defined_macros);
}

static MilterStatus
cb_negotiate (MilterClientContext *context, MilterOption *option,
              gpointer user_data)
{
    n_negotiates++;
    negotiate_option = g_object_ref(option);

    retrieve_context_info(context);

    return MILTER_STATUS_CONTINUE;
}

static MilterStatus
cb_negotiate_response (MilterClientContext *context, MilterOption *option,
                       MilterStatus status, gpointer user_data)
{
    n_negotiate_responses++;

    return status;
}

static MilterStatus
cb_connect (MilterClientContext *context, const gchar *host_name,
            const struct sockaddr *address, socklen_t address_size,
            gpointer user_data)
{
    n_connects++;

    connect_host_name = g_strdup(host_name);
    connect_address = malloc(address_size);
    memcpy(connect_address, address, address_size);
    connect_address_size = address_size;

    retrieve_context_info(context);

    return MILTER_STATUS_CONTINUE;
}

static MilterStatus
cb_connect_response (MilterClientContext *context, const gchar *host_name,
                     const struct sockaddr *address, socklen_t address_size,
                     MilterStatus status, gpointer user_data)
{
    n_connect_responses++;

    return status;
}

static MilterStatus
cb_helo (MilterClientContext *context, const gchar *fqdn, gpointer user_data)
{
    n_helos++;

    helo_fqdn = g_strdup(fqdn);

    retrieve_context_info(context);

    return MILTER_STATUS_CONTINUE;
}

static MilterStatus
cb_helo_response (MilterClientContext *context, const gchar *fqdn,
                  MilterStatus status, gpointer user_data)
{
    n_helo_responses++;

    return status;
}

static MilterStatus
cb_envelope_from (MilterClientContext *context, const gchar *from,
                  gpointer user_data)
{
    n_envelope_froms++;

    envelope_from_address = g_strdup(from);

    retrieve_context_info(context);

    return MILTER_STATUS_CONTINUE;
}

static MilterStatus
cb_envelope_from_response (MilterClientContext *context, const gchar *from,
                           MilterStatus status, gpointer user_data)
{
    n_envelope_from_responses++;

    return status;
}

static MilterStatus
cb_envelope_receipt (MilterClientContext *context, const gchar *to,
                     gpointer user_data)
{
    n_envelope_receipts++;

    envelope_receipt_address = g_strdup(to);

    retrieve_context_info(context);

    return MILTER_STATUS_CONTINUE;
}

static MilterStatus
cb_envelope_receipt_response (MilterClientContext *context, const gchar *to,
                              MilterStatus status, gpointer user_data)
{
    n_envelope_receipt_responses++;

    return status;
}

static MilterStatus
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

    retrieve_context_info(context);

    return MILTER_STATUS_CONTINUE;
}

static MilterStatus
cb_header_response (MilterClientContext *context,
                    const gchar *name, const gchar *value,
                    MilterStatus status, gpointer user_data)
{
    n_header_responses++;

    return status;
}

static MilterStatus
cb_end_of_header (MilterClientContext *context, gpointer user_data)
{
    n_end_of_headers++;

    retrieve_context_info(context);

    return MILTER_STATUS_CONTINUE;
}

static MilterStatus
cb_end_of_header_response (MilterClientContext *context,
                           MilterStatus status, gpointer user_data)
{
    n_end_of_header_responses++;

    return status;
}

static MilterStatus
cb_body (MilterClientContext *context, const gchar *chunk, gsize size,
         gpointer user_data)
{
    n_bodies++;

    if (body_chunk)
        g_free(body_chunk);
    body_chunk = g_strndup(chunk, size);
    body_chunk_size = size;

    retrieve_context_info(context);

    return MILTER_STATUS_CONTINUE;
}

static MilterStatus
cb_body_response (MilterClientContext *context, const gchar *chunk, gsize size,
                  MilterStatus status, gpointer user_data)
{
    n_body_responses++;

    return status;
}

static MilterStatus
cb_end_of_message (MilterClientContext *context, gpointer user_data)
{
    n_end_of_messages++;

    retrieve_context_info(context);

    return MILTER_STATUS_CONTINUE;
}

static MilterStatus
cb_end_of_message_response (MilterClientContext *context,
                            MilterStatus status, gpointer user_data)
{
    n_end_of_message_responses++;

    return status;
}

static MilterStatus
cb_abort (MilterClientContext *context, gpointer user_data)
{
    n_aborts++;

    retrieve_context_info(context);

    return MILTER_STATUS_CONTINUE;
}

static MilterStatus
cb_abort_response (MilterClientContext *context, MilterStatus status,
                   gpointer user_data)
{
    n_abort_responses++;

    return status;
}

static MilterStatus
cb_close (MilterClientContext *context, gpointer user_data)
{
    n_closes++;

    retrieve_context_info(context);

    return MILTER_STATUS_CONTINUE;
}

static MilterStatus
cb_close_response (MilterClientContext *context, MilterStatus status,
                   gpointer user_data)
{
    n_close_responses++;

    return status;
}

static MilterStatus
cb_unknown (MilterClientContext *context, const gchar *command,
            gpointer user_data)
{
    n_unknowns++;

    if (unknown_command)
        g_free(unknown_command);
    unknown_command = g_strdup(command);

    retrieve_context_info(context);

    return MILTER_STATUS_CONTINUE;
}

static MilterStatus
cb_unknown_response (MilterClientContext *context, const gchar *command,
                     MilterStatus status, gpointer user_data)
{
    n_unknown_responses++;

    return status;
}

static void
setup_signals (MilterClientContext *context)
{
#define CONNECT(name) do                                                \
{                                                                       \
    g_signal_connect(context, #name, G_CALLBACK(cb_ ## name), NULL);    \
    g_signal_connect(context, #name "_response",                        \
                         G_CALLBACK(cb_ ## name ## _response), NULL);   \
} while (0)

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
    setup_signals(context);

    encoder = milter_encoder_new();
    packet = NULL;
    packet_size = 0;

    n_negotiates = 0;
    n_negotiate_responses = 0;
    n_connects = 0;
    n_connect_responses = 0;
    n_helos = 0;
    n_helo_responses = 0;
    n_envelope_froms = 0;
    n_envelope_from_responses = 0;
    n_envelope_receipts = 0;
    n_envelope_receipt_responses = 0;
    n_headers = 0;
    n_header_responses = 0;
    n_end_of_headers = 0;
    n_end_of_header_responses = 0;
    n_bodies = 0;
    n_body_responses = 0;
    n_end_of_messages = 0;
    n_end_of_message_responses = 0;
    n_aborts = 0;
    n_abort_responses = 0;
    n_closes = 0;
    n_close_responses = 0;
    n_unknowns = 0;
    n_unknown_responses = 0;

    negotiate_option = NULL;

    expected_macros = NULL;
    defined_macros = NULL;

    macro_name = NULL;
    macro_value = NULL;
    expected_macro_value = NULL;

    connect_host_name = NULL;
    connect_address = NULL;
    connect_address_size = 0;

    helo_fqdn = NULL;

    envelope_from_address = NULL;

    envelope_receipt_address = NULL;

    header_name = NULL;
    header_value = NULL;

    body_chunk = NULL;
    body_chunk_size = 0;
    body_chunk_test_data = NULL;
    body_chunk_test_data_size = 0;

    unknown_command = NULL;
    unknown_command_size = 0;
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

    packet_free();

    if (negotiate_option)
        g_object_unref(negotiate_option);

    if (expected_macros)
        g_hash_table_unref(expected_macros);
    if (defined_macros)
        g_hash_table_unref(defined_macros);
    if (macro_name)
        g_free(macro_name);
    if (macro_value)
        g_free(macro_value);
    if (expected_macro_value)
        g_free(expected_macro_value);

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
    if (body_chunk_test_data)
        g_free(body_chunk_test_data);

    if (unknown_command)
        g_free(unknown_command);
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
test_feed_negotiate (void)
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
    option = milter_option_new(version, action, step);

    milter_encoder_encode_negotiate(encoder,
                                    &packet, &packet_size,
                                    option);
    g_object_unref(option);

    gcut_assert_error(feed());
    cut_assert_equal_int(1, n_negotiates);
    cut_assert_equal_int(1, n_negotiate_responses);

    negotiated_option = negotiate_option;
    cut_assert_equal_int(version, milter_option_get_version(negotiated_option));
    gcut_assert_equal_flags(MILTER_TYPE_ACTION_FLAGS,
                            action,
                            milter_option_get_action(negotiated_option));
    gcut_assert_equal_flags(MILTER_TYPE_STEP_FLAGS,
                            step,
                            milter_option_get_step(negotiated_option));

    gcut_assert_equal_hash_table_string_string(NULL, defined_macros);
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
    cut_assert_equal_int(sizeof(struct sockaddr_in), connect_address_size);

    connected_address = (struct sockaddr_in *)connect_address;
    cut_assert_equal_int(AF_INET, connected_address->sin_family);
    cut_assert_equal_uint(port, connected_address->sin_port);
    cut_assert_equal_string(ip_address, inet_ntoa(connected_address->sin_addr));
}

void
test_feed_connect_ipv6 (void)
{
    struct sockaddr_in6 address;
    struct sockaddr_in6 *connected_address;
    const gchar host_name[] = "mx.local.net";
    const gchar ipv6_address[] = "2001:c90:625:12e8:290:ccff:fee2:80c5";
    gchar connected_ipv6_address[INET6_ADDRSTRLEN];
    guint16 port;

    port = htons(50443);
    address.sin6_family = AF_INET6;
    address.sin6_port = port;
    inet_pton(AF_INET6, ipv6_address, &(address.sin6_addr));

    milter_encoder_encode_connect(encoder,
                                  &packet, &packet_size,
                                  host_name,
                                  (const struct sockaddr *)&address,
                                  sizeof(address));
    gcut_assert_error(feed());
    cut_assert_equal_int(1, n_connects);
    cut_assert_equal_int(1, n_connect_responses);
    cut_assert_equal_string(host_name, connect_host_name);
    cut_assert_equal_int(sizeof(struct sockaddr_in6), connect_address_size);

    connected_address = (struct sockaddr_in6 *)connect_address;
    cut_assert_equal_int(AF_INET6, connected_address->sin6_family);
    cut_assert_equal_uint(port, connected_address->sin6_port);
    cut_assert_equal_string(ipv6_address,
                            inet_ntop(AF_INET6,
                                      &(connected_address->sin6_addr),
                                      connected_ipv6_address,
                                      sizeof(connected_ipv6_address)));
}

void
test_feed_connect_unix (void)
{
    struct sockaddr_un address;
    struct sockaddr_un *connected_address;
    const gchar host_name[] = "mx.local.net";
    const gchar path[] = "/tmp/unix.sock";

    address.sun_family = AF_UNIX;
    strcpy(address.sun_path, path);

    milter_encoder_encode_connect(encoder,
                                  &packet, &packet_size,
                                  host_name,
                                  (const struct sockaddr *)&address,
                                  sizeof(address));
    gcut_assert_error(feed());
    cut_assert_equal_int(1, n_connects);
    cut_assert_equal_int(1, n_connect_responses);
    cut_assert_equal_string(host_name, connect_host_name);
    cut_assert_equal_int(sizeof(struct sockaddr_un), connect_address_size);

    connected_address = (struct sockaddr_un *)connect_address;
    cut_assert_equal_int(AF_UNIX, connected_address->sun_family);
    cut_assert_equal_string(path, connected_address->sun_path);


    gcut_assert_equal_hash_table_string_string(NULL, defined_macros);
}

void
test_feed_connect_with_macro (void)
{
    struct sockaddr_in address;
    struct sockaddr_in *connected_address;
    const gchar host_name[] = "mx.local.net";
    const gchar ip_address[] = "192.168.123.123";
    guint16 port;

    expected_macros =
        gcut_hash_table_string_string_new("j", "debian.cozmixng.org",
                                          "daemon_name", "debian.cozmixng.org",
                                          "v", "Postfix 2.5.5",
                                          NULL);
    milter_encoder_encode_define_macro(encoder,
                                       &packet, &packet_size,
                                       MILTER_COMMAND_CONNECT,
                                       expected_macros);
    gcut_assert_error(feed());
    packet_free();

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
    cut_assert_equal_int(1, n_connect_responses);
    cut_assert_equal_string(host_name, connect_host_name);
    cut_assert_equal_int(sizeof(struct sockaddr_in), connect_address_size);

    connected_address = (struct sockaddr_in *)connect_address;
    cut_assert_equal_int(AF_INET, connected_address->sin_family);
    cut_assert_equal_uint(port, connected_address->sin_port);
    cut_assert_equal_string(ip_address, inet_ntoa(connected_address->sin_addr));

    gcut_assert_equal_hash_table_string_string(expected_macros, defined_macros);
}

static void
feed_helo_pre_hook_with_macro (void)
{
    expected_macros = gcut_hash_table_string_string_new(NULL, NULL);
    milter_encoder_encode_define_macro(encoder,
                                       &packet, &packet_size,
                                       MILTER_COMMAND_HELO,
                                       expected_macros);
    gcut_assert_error(feed());
    packet_free();

    macro_name = g_strdup("nonexistent");
}

void
data_feed_helo (void)
{
    cut_add_data("without macro", NULL, NULL,
                 "with macro", feed_helo_pre_hook_with_macro, NULL);
}

void
test_feed_helo (gconstpointer data)
{
    const HookFunction pre_hook = data;
    const gchar fqdn[] = "delian";

    if (pre_hook)
        pre_hook();

    milter_encoder_encode_helo(encoder, &packet, &packet_size, fqdn);
    gcut_assert_error(feed());
    cut_assert_equal_int(1, n_helos);
    cut_assert_equal_int(1, n_helo_responses);
    cut_assert_equal_string(fqdn, helo_fqdn);

    gcut_assert_equal_hash_table_string_string(expected_macros, defined_macros);
    if (macro_name)
        cut_assert_equal_string(expected_macro_value, macro_value);
}

static void
feed_envelope_from_pre_hook_with_macro (void)
{
    GHashTable *macros;

    macro_name = g_strdup("mail_addr");
    expected_macro_value = g_strdup("kou@cozmixng.org");
    macros =
        gcut_hash_table_string_string_new("{mail_addr}", "kou@cozmixng.org",
                                          NULL);
    expected_macros =
        gcut_hash_table_string_string_new("mail_addr", "kou@cozmixng.org",
                                          NULL);
    milter_encoder_encode_define_macro(encoder,
                                       &packet, &packet_size,
                                       MILTER_COMMAND_MAIL,
                                       macros);
    g_hash_table_unref(macros);
    gcut_assert_error(feed());
    packet_free();
}

void
data_feed_envelope_from (void)
{
    cut_add_data("without macro", NULL, NULL,
                 "with macro", feed_envelope_from_pre_hook_with_macro, NULL);
}

void
test_feed_envelope_from (gconstpointer data)
{
    const HookFunction pre_hook = data;
    const gchar from[] = "<kou@cozmixng.org>";

    if (pre_hook)
        pre_hook();

    milter_encoder_encode_mail(encoder, &packet, &packet_size, from);
    gcut_assert_error(feed());
    cut_assert_equal_int(1, n_envelope_froms);
    cut_assert_equal_int(1, n_envelope_from_responses);
    cut_assert_equal_string(from, envelope_from_address);

    gcut_assert_equal_hash_table_string_string(expected_macros, defined_macros);
    if (macro_name)
        cut_assert_equal_string(expected_macro_value, macro_value);
}

static void
feed_envelope_receipt_pre_hook_with_macro (void)
{
    GHashTable *macros;

    macro_name = g_strdup("{rcpt_addr}");
    expected_macro_value = g_strdup("kou@cozmixng.org");
    macros =
        gcut_hash_table_string_string_new("{rcpt_addr}", "kou@cozmixng.org",
                                          NULL);
    expected_macros =
        gcut_hash_table_string_string_new("rcpt_addr", "kou@cozmixng.org",
                                          NULL);
    milter_encoder_encode_define_macro(encoder,
                                       &packet, &packet_size,
                                       MILTER_COMMAND_RCPT,
                                       macros);
    g_hash_table_unref(macros);
    gcut_assert_error(feed());
    packet_free();
}

void
data_feed_envelope_receipt (void)
{
    cut_add_data("without macro", NULL, NULL,
                 "with macro", feed_envelope_receipt_pre_hook_with_macro, NULL);
}

void
test_feed_envelope_receipt (gconstpointer data)
{
    const HookFunction pre_hook = data;
    const gchar to[] = "<kou@cozmixng.org>";

    if (pre_hook)
        pre_hook();

    milter_encoder_encode_rcpt(encoder, &packet, &packet_size, to);
    gcut_assert_error(feed());
    cut_assert_equal_int(1, n_envelope_receipts);
    cut_assert_equal_int(1, n_envelope_receipt_responses);
    cut_assert_equal_string(to, envelope_receipt_address);

    gcut_assert_equal_hash_table_string_string(expected_macros, defined_macros);
    if (macro_name)
        cut_assert_equal_string(expected_macro_value, macro_value);
}

static void
feed_header_pre_hook_with_macro (void)
{
    macro_name = g_strdup("i");
    expected_macro_value = g_strdup("69FDD42DF4A");
    expected_macros =
        gcut_hash_table_string_string_new("i", "69FDD42DF4A", NULL);
    milter_encoder_encode_define_macro(encoder,
                                       &packet, &packet_size,
                                       MILTER_COMMAND_HEADER,
                                       expected_macros);
    gcut_assert_error(feed());
    packet_free();
}

void
data_feed_header (void)
{
    cut_add_data("without macro", NULL, NULL,
                 "with macro", feed_header_pre_hook_with_macro, NULL);
}

void
test_feed_header (gconstpointer data)
{
    const HookFunction pre_hook = data;
    const gchar date[] = "Fri,  5 Sep 2008 09:19:56 +0900 (JST)";

    if (pre_hook)
        pre_hook();

    milter_encoder_encode_header(encoder, &packet, &packet_size, "Date", date);
    gcut_assert_error(feed());
    cut_assert_equal_int(1, n_headers);
    cut_assert_equal_int(1, n_header_responses);
    cut_assert_equal_string("Date", header_name);
    cut_assert_equal_string(date, header_value);

    gcut_assert_equal_hash_table_string_string(expected_macros, defined_macros);
    if (macro_name)
        cut_assert_equal_string(expected_macro_value, macro_value);
}

static void
feed_end_of_header_pre_hook_with_macro (void)
{
    macro_name = g_strdup("i");
    expected_macro_value = g_strdup("69FDD42DF4A");
    expected_macros =
        gcut_hash_table_string_string_new("i", "69FDD42DF4A", NULL);
    milter_encoder_encode_define_macro(encoder,
                                       &packet, &packet_size,
                                       MILTER_COMMAND_END_OF_HEADER,
                                       expected_macros);
    gcut_assert_error(feed());
    packet_free();
}

void
data_feed_end_of_header (void)
{
    cut_add_data("without macro", NULL, NULL,
                 "with macro", feed_end_of_header_pre_hook_with_macro, NULL);
}

void
test_feed_end_of_header (gconstpointer data)
{
    const HookFunction pre_hook = data;

    if (pre_hook)
        pre_hook();

    milter_encoder_encode_end_of_header(encoder, &packet, &packet_size);
    gcut_assert_error(feed());
    cut_assert_equal_int(1, n_end_of_headers);
    cut_assert_equal_int(1, n_end_of_header_responses);

    gcut_assert_equal_hash_table_string_string(expected_macros, defined_macros);
    if (macro_name)
        cut_assert_equal_string(expected_macro_value, macro_value);
}

static void
feed_body_pre_hook_with_macro (void)
{
    macro_name = g_strdup("i");
    expected_macro_value = g_strdup("69FDD42DF4A");
    expected_macros =
        gcut_hash_table_string_string_new("i", "69FDD42DF4A", NULL);
    milter_encoder_encode_define_macro(encoder,
                                       &packet, &packet_size,
                                       MILTER_COMMAND_BODY,
                                       expected_macros);
    gcut_assert_error(feed());
    packet_free();
}

void
data_feed_body (void)
{
    cut_add_data("without macro", NULL, NULL,
                 "with macro", feed_body_pre_hook_with_macro, NULL);
}

void
test_feed_body (gconstpointer data)
{
    const HookFunction pre_hook = data;
    const gchar body[] =
        "La de da de da 1.\n"
        "La de da de da 2.\n"
        "La de da de da 3.\n"
        "La de da de da 4.";

    if (pre_hook)
        pre_hook();

    milter_encoder_encode_body(encoder, &packet, &packet_size,
                               body, sizeof(body));
    gcut_assert_error(feed());
    cut_assert_equal_int(1, n_bodies);
    cut_assert_equal_int(1, n_body_responses);
    cut_assert_equal_string(body, body_chunk);
    cut_assert_equal_int(sizeof(body), body_chunk_size);

    gcut_assert_equal_hash_table_string_string(expected_macros, defined_macros);
    if (macro_name)
        cut_assert_equal_string(expected_macro_value, macro_value);
}

static void
feed_end_of_message_pre_hook_with_body (void)
{
    body_chunk_test_data = g_strdup("La de da de da 1.\n"
                                    "La de da de da 2.\n"
                                    "La de da de da 3.\n"
                                    "La de da de da 4.");
    body_chunk_test_data_size = strlen(body_chunk_test_data);
}

static void
feed_end_of_message_pre_hook_with_macro (void)
{
    macro_name = g_strdup("i");
    expected_macro_value = g_strdup("69FDD42DF4A");
    expected_macros =
        gcut_hash_table_string_string_new("i", "69FDD42DF4A", NULL);
    milter_encoder_encode_define_macro(encoder,
                                       &packet, &packet_size,
                                       MILTER_COMMAND_END_OF_MESSAGE,
                                       expected_macros);
    gcut_assert_error(feed());
    packet_free();
}

static void
feed_end_of_message_pre_hook_with_body_and_macro (void)
{
    feed_end_of_message_pre_hook_with_body();
    feed_end_of_message_pre_hook_with_macro();
}

void
data_feed_end_of_message (void)
{
    cut_add_data("without body",
                 NULL, NULL,
                 "without body - with macro",
                 feed_end_of_message_pre_hook_with_macro, NULL,
                 "with body",
                 feed_end_of_message_pre_hook_with_body, NULL,
                 "with body - with macro",
                 feed_end_of_message_pre_hook_with_body_and_macro, NULL);
}

void
test_feed_end_of_message (gconstpointer data)
{
    const HookFunction pre_hook = data;

    if (pre_hook)
        pre_hook();

    milter_encoder_encode_end_of_message(encoder, &packet, &packet_size,
                                         body_chunk_test_data,
                                         body_chunk_test_data_size);
    gcut_assert_error(feed());
    if (body_chunk_test_data) {
        cut_assert_equal_int(1, n_bodies);
        cut_assert_equal_int(1, n_body_responses);
        cut_assert_equal_string(body_chunk_test_data, body_chunk);
        cut_assert_equal_int(body_chunk_test_data_size, body_chunk_size);
    } else {
        cut_assert_equal_int(0, n_bodies);
        cut_assert_equal_int(0, n_body_responses);
    }
    cut_assert_equal_int(1, n_end_of_messages);
    cut_assert_equal_int(1, n_end_of_message_responses);

    gcut_assert_equal_hash_table_string_string(expected_macros, defined_macros);
    if (macro_name)
        cut_assert_equal_string(expected_macro_value, macro_value);
}

void
test_feed_close (void)
{
    milter_encoder_encode_quit(encoder, &packet, &packet_size);
    gcut_assert_error(feed());
    cut_assert_equal_int(1, n_closes);
    cut_assert_equal_int(1, n_close_responses);
}

void
test_feed_abort (void)
{
    milter_encoder_encode_abort(encoder, &packet, &packet_size);
    gcut_assert_error(feed());
    cut_assert_equal_int(1, n_aborts);
    cut_assert_equal_int(1, n_abort_responses);
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
