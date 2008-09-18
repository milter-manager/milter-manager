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
#undef shutdown

#include <milter-core/milter-decoder.h>
#include <milter-core/milter-enum-types.h>

void test_decode_empty_text (void);
void test_decode_option_negotiation (void);
void test_decode_option_negotiation_empty (void);
void test_decode_option_negotiation_without_version (void);
void test_decode_option_negotiation_only_version (void);
void test_decode_option_negotiation_without_action (void);
void test_decode_option_negotiation_only_version_and_action (void);
void test_decode_option_negotiation_without_step (void);
void data_decode_define_macro (void);
void test_decode_define_macro (gconstpointer data);
void test_decode_define_macro_with_unknown_macro_context (void);
void test_decode_define_macro_without_name_null (void);
void test_decode_define_macro_without_value_null (void);
void test_decode_connect_ipv4 (void);
void test_decode_connect_ipv6 (void);
void test_decode_connect_unix (void);
void test_decode_connect_without_host_name_null (void);
void test_decode_connect_with_unknown_family (void);
void test_decode_connect_with_unexpected_family (void);
void test_decode_connect_with_unexpected_family_and_port (void);
void test_decode_connect_with_short_port_size (void);
void test_decode_connect_without_ip_address_null (void);
void test_decode_connect_with_invalid_ipv4_address (void);
void test_decode_connect_with_invalid_ipv6_address (void);
void test_decode_helo (void);
void test_decode_helo_without_null (void);
void test_decode_mail (void);
void test_decode_mail_without_null (void);
void test_decode_rcpt (void);
void test_decode_rcpt_without_null (void);
void test_decode_header (void);
void test_decode_header_without_name_null (void);
void test_decode_header_without_value_null (void);
void test_decode_end_of_header (void);
void test_decode_end_of_header_with_garbage (void);
void test_decode_body (void);
void test_decode_body_end_with_data (void);
void test_decode_body_end_without_data (void);
void test_decode_abort (void);
void test_decode_abort_with_garbage (void);
void test_decode_quit (void);
void test_decode_quit_with_garbage (void);
void test_decode_unknown (void);
void test_decode_unknown_without_null (void);
void test_decode_unexpected_command (void);

void test_end_decode_immediately (void);
void test_end_decode_in_command_length_decoding (void);
void test_end_decode_in_command_content_decoding (void);

static MilterDecoder *decoder;
static GString *buffer;

static GError *expected_error;
static GError *actual_error;

static gint n_option_negotiations;
static gint n_define_macros;
static gint n_connects;
static gint n_helos;
static gint n_mails;
static gint n_rcpts;
static gint n_headers;
static gint n_end_of_headers;
static gint n_bodies;
static gint n_end_of_messages;
static gint n_aborts;
static gint n_quits;
static gint n_unknowns;

static MilterOption *option_negotiation_option;

static MilterContextType macro_context;
static GHashTable *defined_macros;

static gchar *connect_host_name;
static struct sockaddr *connect_address;
static socklen_t connect_address_length;

static gchar *helo_fqdn;

static gchar *mail_from;

static gchar *rcpt_to;

static gchar *header_name;
static gchar *header_value;

static gchar *body_chunk;
static gsize body_chunk_length;

static gchar *unknown_command;
static gsize unknown_command_length;

static void
cb_option_negotiation (MilterDecoder *decoder, MilterOption *option,
                       gpointer user_data)
{
    n_option_negotiations++;
    option_negotiation_option = g_object_ref(option);
}

static void
cb_define_macro (MilterDecoder *decoder, MilterContextType context, GHashTable *macros,
                 gpointer user_data)
{
    n_define_macros++;

    macro_context = context;
    if (defined_macros)
        g_hash_table_unref(defined_macros);
    defined_macros = macros;
    if (defined_macros)
        g_hash_table_ref(defined_macros);
}

static void
cb_connect (MilterDecoder *decoder, const gchar *host_name,
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
cb_helo (MilterDecoder *decoder, const gchar *fqdn, gpointer user_data)
{
    n_helos++;

    helo_fqdn = g_strdup(fqdn);
}

static void
cb_mail (MilterDecoder *decoder, const gchar *from, gpointer user_data)
{
    n_mails++;

    mail_from = g_strdup(from);
}

static void
cb_rcpt (MilterDecoder *decoder, const gchar *to, gpointer user_data)
{
    n_rcpts++;

    rcpt_to = g_strdup(to);
}


static void
cb_header (MilterDecoder *decoder, const gchar *name, const gchar *value,
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
cb_end_of_header (MilterDecoder *decoder, gpointer user_data)
{
    n_end_of_headers++;
}

static void
cb_body (MilterDecoder *decoder, const gchar *chunk, gsize length, gpointer user_data)
{
    n_bodies++;

    if (body_chunk)
        g_free(body_chunk);
    body_chunk = g_strndup(chunk, length);
    body_chunk_length = length;
}

static void
cb_end_of_message (MilterDecoder *decoder, gpointer user_data)
{
    n_end_of_messages++;
}

static void
cb_abort (MilterDecoder *decoder, gpointer user_data)
{
    n_aborts++;
}

static void
cb_quit (MilterDecoder *decoder, gpointer user_data)
{
    n_quits++;
}

static void
cb_unknown (MilterDecoder *decoder, const gchar *command, gpointer user_data)
{
    n_unknowns++;

    if (unknown_command)
        g_free(unknown_command);
    unknown_command = g_strdup(command);
}

static void
setup_signals (MilterDecoder *decoder)
{
#define CONNECT(name)                                                   \
    g_signal_connect(decoder, #name, G_CALLBACK(cb_ ## name), NULL)

    CONNECT(option_negotiation);
    CONNECT(define_macro);
    CONNECT(connect);
    CONNECT(helo);
    CONNECT(mail);
    CONNECT(rcpt);
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
    decoder = milter_decoder_new();
    setup_signals(decoder);

    expected_error = NULL;
    actual_error = NULL;

    n_option_negotiations = 0;
    n_define_macros = 0;
    n_connects = 0;
    n_helos = 0;
    n_mails = 0;
    n_rcpts = 0;
    n_headers = 0;
    n_end_of_headers = 0;
    n_bodies = 0;
    n_end_of_messages = 0;
    n_aborts = 0;
    n_quits = 0;
    n_unknowns = 0;

    buffer = g_string_new(NULL);

    option_negotiation_option = NULL;

    defined_macros = NULL;

    connect_host_name = NULL;
    connect_address = NULL;
    connect_address_length = 0;

    helo_fqdn = NULL;

    mail_from = NULL;

    rcpt_to = NULL;

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
    if (decoder) {
        g_object_unref(decoder);
    }

    if (buffer)
        g_string_free(buffer, TRUE);

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

    if (mail_from)
        g_free(mail_from);

    if (rcpt_to)
        g_free(rcpt_to);

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
decode (void)
{
    gchar *packet;
    gssize packet_size;
    guint32 content_size;
    GError *error = NULL;

    packet_size = sizeof(content_size) + buffer->len;
    packet = g_new0(gchar, packet_size);

    content_size = htonl(buffer->len);
    memcpy(packet, &content_size, sizeof(content_size));
    memcpy(packet + sizeof(content_size), buffer->str, buffer->len);

    milter_decoder_decode(decoder, packet, packet_size, &error);
    g_free(packet);

    g_string_free(buffer, TRUE);
    buffer = g_string_new(NULL);

    return error;
}

void
test_decode_empty_text (void)
{
    cut_assert_true(milter_decoder_decode(decoder, "", 0, &actual_error));
    cut_assert_true(milter_decoder_end_decode(decoder, &actual_error));
}

void
test_decode_option_negotiation (void)
{
    guint32 version;
    MilterActionFlags action;
    MilterStepFlags step;
    guint32 version_network_byte_order;
    guint32 action_network_byte_order;
    guint32 step_network_byte_order;
    gchar version_string[sizeof(guint32)];
    gchar action_string[sizeof(guint32)];
    gchar step_string[sizeof(guint32)];

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

    version_network_byte_order = htonl(version);
    action_network_byte_order = htonl(action);
    step_network_byte_order = htonl(step);

    memcpy(version_string, &version_network_byte_order, sizeof(guint32));
    memcpy(action_string, &action_network_byte_order, sizeof(guint32));
    memcpy(step_string, &step_network_byte_order, sizeof(guint32));

    g_string_append_c(buffer, 'O');
    g_string_append_len(buffer, version_string, sizeof(version_string));
    g_string_append_len(buffer, action_string, sizeof(action_string));
    g_string_append_len(buffer, step_string, sizeof(step_string));

    gcut_assert_error(decode());
    cut_assert_equal_int(1, n_option_negotiations);
    cut_assert_equal_int(2,
                         milter_option_get_version(option_negotiation_option));
    gcut_assert_equal_flags(MILTER_TYPE_ACTION_FLAGS,
                            action,
                            milter_option_get_action(option_negotiation_option));
    gcut_assert_equal_flags(MILTER_TYPE_STEP_FLAGS,
                            step,
                            milter_option_get_step(option_negotiation_option));
}

void
test_decode_option_negotiation_empty (void)
{
    g_string_append_c(buffer, 'O');

    expected_error = g_error_new(MILTER_DECODER_ERROR,
                                 MILTER_DECODER_ERROR_SHORT_COMMAND_LENGTH,
                                 "need more 4 bytes for decoding version "
                                 "on option negotiation command");
    actual_error = decode();
    gcut_assert_equal_error(expected_error, actual_error);
}

void
test_decode_option_negotiation_without_version (void)
{
    guint32 version;
    guint32 version_network_byte_order;
    gchar version_string[sizeof(guint32)];

    version = 2;
    version_network_byte_order = htonl(version);
    memcpy(version_string, &version_network_byte_order, sizeof(guint32));

    g_string_append_c(buffer, 'O');
    g_string_append_len(buffer, version_string, sizeof(version_string) - 1);

    expected_error = g_error_new(MILTER_DECODER_ERROR,
                                 MILTER_DECODER_ERROR_SHORT_COMMAND_LENGTH,
                                 "need more 1 byte for decoding version "
                                 "on option negotiation command: "
                                 "0x00 0x00 0x00");
    actual_error = decode();
    gcut_assert_equal_error(expected_error, actual_error);
}

void
test_decode_option_negotiation_only_version (void)
{
    guint32 version;
    guint32 version_network_byte_order;
    gchar version_string[sizeof(guint32)];

    version = 2;
    version_network_byte_order = htonl(version);
    memcpy(version_string, &version_network_byte_order, sizeof(guint32));

    g_string_append_c(buffer, 'O');
    g_string_append_len(buffer, version_string, sizeof(version_string));

    expected_error = g_error_new(MILTER_DECODER_ERROR,
                                 MILTER_DECODER_ERROR_SHORT_COMMAND_LENGTH,
                                 "need more 4 bytes for decoding action flags "
                                 "on option negotiation command");
    actual_error = decode();
    gcut_assert_equal_error(expected_error, actual_error);
}

void
test_decode_option_negotiation_without_action (void)
{
    guint32 version;
    MilterActionFlags action;
    guint32 version_network_byte_order;
    guint32 action_network_byte_order;
    gchar version_string[sizeof(guint32)];
    gchar action_string[sizeof(guint32)];

    version = 2;
    action = MILTER_ACTION_ADD_HEADERS |
        MILTER_ACTION_CHANGE_BODY |
        MILTER_ACTION_ADD_RCPT |
        MILTER_ACTION_DELETE_RCPT |
        MILTER_ACTION_CHANGE_HEADERS |
        MILTER_ACTION_QUARANTINE |
        MILTER_ACTION_SET_SYMBOL_LIST;

    version_network_byte_order = htonl(version);
    action_network_byte_order = htonl(action);

    memcpy(version_string, &version_network_byte_order, sizeof(guint32));
    memcpy(action_string, &action_network_byte_order, sizeof(guint32));

    g_string_append_c(buffer, 'O');
    g_string_append_len(buffer, version_string, sizeof(version_string));
    g_string_append_len(buffer, action_string, sizeof(action_string) - 1);

    expected_error = g_error_new(MILTER_DECODER_ERROR,
                                 MILTER_DECODER_ERROR_SHORT_COMMAND_LENGTH,
                                 "need more 1 byte for decoding action flags "
                                 "on option negotiation command: "
                                 "0x00 0x00 0x01");
    actual_error = decode();
    gcut_assert_equal_error(expected_error, actual_error);
}

void
test_decode_option_negotiation_only_version_and_action (void)
{
    guint32 version;
    MilterActionFlags action;
    guint32 version_network_byte_order;
    guint32 action_network_byte_order;
    gchar version_string[sizeof(guint32)];
    gchar action_string[sizeof(guint32)];

    version = 2;
    action = MILTER_ACTION_ADD_HEADERS |
        MILTER_ACTION_CHANGE_BODY |
        MILTER_ACTION_ADD_RCPT |
        MILTER_ACTION_DELETE_RCPT |
        MILTER_ACTION_CHANGE_HEADERS |
        MILTER_ACTION_QUARANTINE |
        MILTER_ACTION_SET_SYMBOL_LIST;

    version_network_byte_order = htonl(version);
    action_network_byte_order = htonl(action);

    memcpy(version_string, &version_network_byte_order, sizeof(guint32));
    memcpy(action_string, &action_network_byte_order, sizeof(guint32));

    g_string_append_c(buffer, 'O');
    g_string_append_len(buffer, version_string, sizeof(version_string));
    g_string_append_len(buffer, action_string, sizeof(action_string));

    expected_error = g_error_new(MILTER_DECODER_ERROR,
                                 MILTER_DECODER_ERROR_SHORT_COMMAND_LENGTH,
                                 "need more 4 bytes for decoding step flags "
                                 "on option negotiation command");
    actual_error = decode();
    gcut_assert_equal_error(expected_error, actual_error);
}

void
test_decode_option_negotiation_without_step (void)
{
    guint32 version;
    MilterActionFlags action;
    MilterStepFlags step;
    guint32 version_network_byte_order;
    guint32 action_network_byte_order;
    guint32 step_network_byte_order;
    gchar version_string[sizeof(guint32)];
    gchar action_string[sizeof(guint32)];
    gchar step_string[sizeof(guint32)];

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

    version_network_byte_order = htonl(version);
    action_network_byte_order = htonl(action);
    step_network_byte_order = htonl(step);

    memcpy(version_string, &version_network_byte_order, sizeof(guint32));
    memcpy(action_string, &action_network_byte_order, sizeof(guint32));
    memcpy(step_string, &step_network_byte_order, sizeof(guint32));

    g_string_append_c(buffer, 'O');
    g_string_append_len(buffer, version_string, sizeof(version_string));
    g_string_append_len(buffer, action_string, sizeof(action_string));
    g_string_append_len(buffer, step_string, sizeof(step_string) - 1);

    expected_error = g_error_new(MILTER_DECODER_ERROR,
                                 MILTER_DECODER_ERROR_SHORT_COMMAND_LENGTH,
                                 "need more 1 byte for decoding step flags "
                                 "on option negotiation command: "
                                 "0x00 0x00 0x00");
    actual_error = decode();
    gcut_assert_equal_error(expected_error, actual_error);
}

static void
append_name_and_value(const gchar *name, const gchar *value)
{
    if (name)
        g_string_append(buffer, name);
    g_string_append_c(buffer, '\0');
    if (value)
        g_string_append(buffer, value);
    g_string_append_c(buffer, '\0');
}

typedef void (*setup_packet_func) (void);

typedef struct _DefineMacroTestData
{
    setup_packet_func setup_packet;
    MilterContextType context;
    GHashTable *expected;
} DefineMacroTestData;

static DefineMacroTestData *define_macro_test_data_new(setup_packet_func setup_packet,
                                                       MilterContextType context,
                                                       const gchar *key,
                                                       ...) G_GNUC_NULL_TERMINATED;
static DefineMacroTestData *
define_macro_test_data_new (setup_packet_func setup_packet,
                            MilterContextType context,
                            const gchar *key,
                            ...)
{
    DefineMacroTestData *data;
    va_list args;

    data = g_new0(DefineMacroTestData, 1);

    data->setup_packet = setup_packet;
    data->context = context;
    va_start(args, key);
    data->expected = gcut_hash_table_string_string_newv(key, args);
    va_end(args);

    return data;
}

static void
define_macro_test_data_free (DefineMacroTestData *data)
{
    g_hash_table_unref(data->expected);
    g_free(data);
}

static void
setup_define_macro_connect_packet (void)
{
    g_string_append(buffer, "C");
    append_name_and_value("j", "debian.cozmixng.org");
    append_name_and_value("daemon_name", "debian.cozmixng.org");
    append_name_and_value("v", "Postfix 2.5.5");
}

static void
setup_define_macro_helo_packet (void)
{
    g_string_append(buffer, "H");
}

static void
setup_define_macro_mail_packet (void)
{
    g_string_append(buffer, "M");
    append_name_and_value("{mail_addr}", "kou@cozmixng.org");
}

static void
setup_define_macro_rcpt_packet (void)
{
    g_string_append(buffer, "R");
    append_name_and_value("{rcpt_addr}", "kou@cozmixng.org");
}

static void
setup_define_macro_header_packet (void)
{
    g_string_append(buffer, "L");
    append_name_and_value("i", "69FDD42DF4A");
}

void
data_decode_define_macro (void)
{
    cut_add_data("connect",
                 define_macro_test_data_new(setup_define_macro_connect_packet,
                                            MILTER_CONTEXT_TYPE_CONNECT,
                                            "j", "debian.cozmixng.org",
                                            "daemon_name", "debian.cozmixng.org",
                                            "v", "Postfix 2.5.5",
                                            NULL),
                 define_macro_test_data_free);

    cut_add_data("HELO",
                 define_macro_test_data_new(setup_define_macro_helo_packet,
                                            MILTER_CONTEXT_TYPE_HELO,
                                            NULL, NULL),
                 define_macro_test_data_free);

    cut_add_data("MAIL FROM",
                 define_macro_test_data_new(setup_define_macro_mail_packet,
                                            MILTER_CONTEXT_TYPE_MAIL,
                                            "mail_addr", "kou@cozmixng.org",
                                            NULL),
                 define_macro_test_data_free);

    cut_add_data("RCPT TO",
                 define_macro_test_data_new(setup_define_macro_rcpt_packet,
                                            MILTER_CONTEXT_TYPE_RCPT,
                                            "rcpt_addr", "kou@cozmixng.org",
                                            NULL),
                 define_macro_test_data_free);

    cut_add_data("header",
                 define_macro_test_data_new(setup_define_macro_header_packet,
                                            MILTER_CONTEXT_TYPE_HEADER,
                                            "i", "69FDD42DF4A",
                                            NULL),
                 define_macro_test_data_free);
}

void
test_decode_define_macro (gconstpointer data)
{
    const DefineMacroTestData *test_data = data;

    g_string_append(buffer, "D");
    test_data->setup_packet();

    gcut_assert_error(decode());
    cut_assert_equal_int(1, n_define_macros);
    cut_assert_equal_int(test_data->context, macro_context);
    gcut_assert_equal_hash_table_string_string(test_data->expected,
                                               defined_macros);
}

void
test_decode_define_macro_with_unknown_macro_context (void)
{
    g_string_append(buffer, "D");
    g_string_append(buffer, "Z");

    expected_error = g_error_new(MILTER_DECODER_ERROR,
                                 MILTER_DECODER_ERROR_UNKNOWN_MACRO_CONTEXT,
                                 "unknown macro context: Z");
    actual_error = decode();
    gcut_assert_equal_error(expected_error, actual_error);
}

void
test_decode_define_macro_without_name_null (void)
{
    g_string_append(buffer, "D");
    g_string_append(buffer, "L");
    g_string_append(buffer, "i");

    expected_error = g_error_new(MILTER_DECODER_ERROR,
                                 MILTER_DECODER_ERROR_MISSING_NULL,
                                 "name isn't terminated by NULL "
                                 "on define macro command: <i>");
    actual_error = decode();
    gcut_assert_equal_error(expected_error, actual_error);
}

void
test_decode_define_macro_without_value_null (void)
{
    g_string_append(buffer, "D");
    g_string_append(buffer, "L");
    g_string_append(buffer, "i");
    g_string_append_c(buffer, '\0');
    g_string_append(buffer, "69FDD42DF4A");

    expected_error = g_error_new(MILTER_DECODER_ERROR,
                                 MILTER_DECODER_ERROR_MISSING_NULL,
                                 "value isn't terminated by NULL "
                                 "on define macro command: "
                                 "<69FDD42DF4A>");
    actual_error = decode();
    gcut_assert_equal_error(expected_error, actual_error);
}

void
test_decode_connect_ipv4 (void)
{
    struct sockaddr_in *address;
    const gchar host_name[] = "mx.local.net";
    const gchar ip_address[] = "192.168.123.123";
    gchar port_string[sizeof(uint16_t)];
    uint16_t port;

    port = htons(50443);
    memcpy(port_string, &port, sizeof(port));

    g_string_append(buffer, "C");
    g_string_append(buffer, host_name);
    g_string_append_c(buffer, '\0');
    g_string_append(buffer, "4");
    g_string_append_len(buffer, port_string, sizeof(port_string));
    g_string_append(buffer, ip_address);
    g_string_append_c(buffer, '\0');

    gcut_assert_error(decode());
    cut_assert_equal_int(1, n_connects);
    cut_assert_equal_string(host_name, connect_host_name);
    cut_assert_equal_int(sizeof(struct sockaddr_in), connect_address_length);

    address = (struct sockaddr_in *)connect_address;
    cut_assert_equal_int(AF_INET, address->sin_family);
    cut_assert_equal_uint(port, address->sin_port);
    cut_assert_equal_string(ip_address, inet_ntoa(address->sin_addr));
}

void
test_decode_connect_ipv6 (void)
{
    struct sockaddr_in6 *address;
    const gchar host_name[] = "mx.local.net";
    const gchar ipv6_address[] = "2001:c90:625:12e8:290:ccff:fee2:80c5";
    gchar actual_address[4096];
    gchar port_string[sizeof(uint16_t)];
    uint16_t port;

    port = htons(50443);
    memcpy(port_string, &port, sizeof(port));

    g_string_append(buffer, "C");
    g_string_append(buffer, host_name);
    g_string_append_c(buffer, '\0');
    g_string_append(buffer, "6");
    g_string_append_len(buffer, port_string, sizeof(port_string));
    g_string_append(buffer, ipv6_address);
    g_string_append_c(buffer, '\0');

    gcut_assert_error(decode());
    cut_assert_equal_int(1, n_connects);
    cut_assert_equal_string(host_name, connect_host_name);
    cut_assert_equal_int(sizeof(struct sockaddr_in6), connect_address_length);

    address = (struct sockaddr_in6 *)connect_address;
    cut_assert_equal_int(AF_INET6, address->sin6_family);
    cut_assert_equal_uint(port, address->sin6_port);
    cut_assert_equal_string(ipv6_address,
                            inet_ntop(AF_INET6,
                                      &(address->sin6_addr),
                                      actual_address,
                                      sizeof(actual_address)));
}

void
test_decode_connect_unix (void)
{
    struct sockaddr_un *address;
    const gchar host_name[] = "mx.local.net";
    const gchar path[] = "/tmp/unix.sock";
    gchar port_string[sizeof(uint16_t)];
    uint16_t port;

    port = htons(0);
    memcpy(port_string, &port, sizeof(port));

    g_string_append(buffer, "C");
    g_string_append(buffer, host_name);
    g_string_append_c(buffer, '\0');
    g_string_append(buffer, "L");
    g_string_append_len(buffer, port_string, sizeof(port_string));
    g_string_append(buffer, path);
    g_string_append_c(buffer, '\0');

    gcut_assert_error(decode());
    cut_assert_equal_int(1, n_connects);
    cut_assert_equal_string(host_name, connect_host_name);
    cut_assert_equal_int(sizeof(struct sockaddr_un), connect_address_length);

    address = (struct sockaddr_un *)connect_address;
    cut_assert_equal_int(AF_UNIX, address->sun_family);
    cut_assert_equal_string(path, address->sun_path);
}

void
test_decode_connect_without_host_name_null (void)
{
    const gchar host_name[] = "mx.local.net";

    g_string_append(buffer, "C");
    g_string_append(buffer, host_name);

    expected_error = g_error_new(MILTER_DECODER_ERROR,
                                 MILTER_DECODER_ERROR_MISSING_NULL,
                                 "host name isn't terminated by NULL "
                                 "on connect command: <mx.local.net>");
    actual_error = decode();
    gcut_assert_equal_error(expected_error, actual_error);
}

void
test_decode_connect_with_unknown_family (void)
{
    const gchar host_name[] = "mx.local.net";

    g_string_append(buffer, "C");
    g_string_append(buffer, host_name);
    g_string_append_c(buffer, '\0');
    g_string_append(buffer, "U");

    expected_error = g_error_new(MILTER_DECODER_ERROR,
                                 MILTER_DECODER_ERROR_UNKNOWN_FAMILY,
                                 "unknown family on connect command: "
                                 "<mx.local.net>: <U>");
    actual_error = decode();
    gcut_assert_equal_error(expected_error, actual_error);
}

void
test_decode_connect_with_unexpected_family (void)
{
    const gchar host_name[] = "mx.local.net";

    g_string_append(buffer, "C");
    g_string_append(buffer, host_name);
    g_string_append_c(buffer, '\0');
    g_string_append(buffer, "X");

    expected_error = g_error_new(MILTER_DECODER_ERROR,
                                 MILTER_DECODER_ERROR_UNKNOWN_FAMILY,
                                 "unknown family on connect command: "
                                 "<mx.local.net>: <X>");
    actual_error = decode();
    gcut_assert_equal_error(expected_error, actual_error);
}

void
test_decode_connect_with_unexpected_family_and_port (void)
{
    const gchar host_name[] = "mx.local.net";
    gchar port_string[sizeof(guint16)];
    guint16 port;

    port = htons(2929);
    memcpy(port_string, &port, sizeof(port));

    g_string_append(buffer, "C");
    g_string_append(buffer, host_name);
    g_string_append_c(buffer, '\0');
    g_string_append(buffer, "X");
    g_string_append_len(buffer, port_string, sizeof(port));

    expected_error = g_error_new(MILTER_DECODER_ERROR,
                                 MILTER_DECODER_ERROR_UNKNOWN_FAMILY,
                                 "unknown family on connect command: "
                                 "<mx.local.net>: <X>");
    actual_error = decode();
    gcut_assert_equal_error(expected_error, actual_error);
}

void
test_decode_connect_with_short_port_size (void)
{
    const gchar host_name[] = "mx.local.net";
    gchar port_string[sizeof(guint16)];
    guint16 port;

    port = htons(2929);
    memcpy(port_string, &port, sizeof(port));

    g_string_append(buffer, "C");
    g_string_append(buffer, host_name);
    g_string_append_c(buffer, '\0');
    g_string_append(buffer, "4");
    g_string_append_len(buffer, port_string, sizeof(port) - 1);

    expected_error = g_error_new(MILTER_DECODER_ERROR,
                                 MILTER_DECODER_ERROR_SHORT_COMMAND_LENGTH,
                                 "need more 1 byte for decoding port number "
                                 "on connect command: 0x%02x",
                                 port_string[0]);
    actual_error = decode();
    gcut_assert_equal_error(expected_error, actual_error);
}

void
test_decode_connect_without_ip_address_null (void)
{
    const gchar host_name[] = "mx.local.net";
    const gchar ip_address[] = "192.168.123.123";
    gchar port_string[sizeof(guint16)];
    guint16 port;

    port = htons(2929);
    memcpy(port_string, &port, sizeof(port));

    g_string_append(buffer, "C");
    g_string_append(buffer, host_name);
    g_string_append_c(buffer, '\0');
    g_string_append(buffer, "4");
    g_string_append_len(buffer, port_string, sizeof(port));
    g_string_append(buffer, ip_address);

    expected_error = g_error_new(MILTER_DECODER_ERROR,
                                 MILTER_DECODER_ERROR_MISSING_NULL,
                                 "address name isn't terminated by NULL "
                                 "on connect command: "
                                 "<mx.local.net>: <4>: <2929>: "
                                 "<192.168.123.123>");
    actual_error = decode();
    gcut_assert_equal_error(expected_error, actual_error);
}

void
test_decode_connect_with_invalid_ipv4_address (void)
{
    const gchar host_name[] = "mx.local.net";
    const gchar ip_address[] = "192.168.123.12x";
    gchar port_string[sizeof(guint16)];
    guint16 port;

    port = htons(2929);
    memcpy(port_string, &port, sizeof(port));

    g_string_append(buffer, "C");
    g_string_append(buffer, host_name);
    g_string_append_c(buffer, '\0');
    g_string_append(buffer, "4");
    g_string_append_len(buffer, port_string, sizeof(port));
    g_string_append(buffer, ip_address);
    g_string_append_c(buffer, '\0');

    expected_error = g_error_new(MILTER_DECODER_ERROR,
                                 MILTER_DECODER_ERROR_INVALID_FORMAT,
                                 "invalid IPv4 address on connect command: "
                                 "<mx.local.net>: <4>: <2929>: "
                                 "<192.168.123.12x>");
    actual_error = decode();
    gcut_assert_equal_error(expected_error, actual_error);
}

void
test_decode_connect_with_invalid_ipv6_address (void)
{
    const gchar host_name[] = "mx.local.net";
    const gchar ipv6_address[] = "2001:c90:625:12e8:290:ccff:fee2:80c5x";
    gchar port_string[sizeof(guint16)];
    guint16 port;

    port = htons(2929);
    memcpy(port_string, &port, sizeof(port));

    g_string_append(buffer, "C");
    g_string_append(buffer, host_name);
    g_string_append_c(buffer, '\0');
    g_string_append(buffer, "6");
    g_string_append_len(buffer, port_string, sizeof(port));
    g_string_append(buffer, ipv6_address);
    g_string_append_c(buffer, '\0');

    expected_error = g_error_new(MILTER_DECODER_ERROR,
                                 MILTER_DECODER_ERROR_INVALID_FORMAT,
                                 "invalid IPv6 address on connect command: "
                                 "<mx.local.net>: <6>: <2929>: "
                                 "<2001:c90:625:12e8:290:ccff:fee2:80c5x>");
    actual_error = decode();
    gcut_assert_equal_error(expected_error, actual_error);
}

void
test_decode_helo (void)
{
    const gchar fqdn[] = "delian";

    g_string_append(buffer, "H");
    g_string_append(buffer, fqdn);
    g_string_append_c(buffer, '\0');

    gcut_assert_error(decode());
    cut_assert_equal_int(1, n_helos);
    cut_assert_equal_string(fqdn, helo_fqdn);
}

void
test_decode_helo_without_null (void)
{
    const gchar fqdn[] = "delian";

    g_string_append(buffer, "H");
    g_string_append(buffer, fqdn);

    expected_error = g_error_new(MILTER_DECODER_ERROR,
                                 MILTER_DECODER_ERROR_MISSING_NULL,
                                 "FQDN isn't terminated by NULL "
                                 "on HELO command: <delian>");
    actual_error = decode();
    gcut_assert_equal_error(expected_error, actual_error);
}

void
test_decode_mail (void)
{
    const gchar from[] = "<kou@cozmixng.org>";

    g_string_append(buffer, "M");
    g_string_append(buffer, from);
    g_string_append_c(buffer, '\0');

    gcut_assert_error(decode());
    cut_assert_equal_int(1, n_mails);
    cut_assert_equal_string(from, mail_from);
}

void
test_decode_mail_without_null (void)
{
    const gchar from[] = "<kou@cozmixng.org>";

    g_string_append(buffer, "M");
    g_string_append(buffer, from);

    expected_error = g_error_new(MILTER_DECODER_ERROR,
                                 MILTER_DECODER_ERROR_MISSING_NULL,
                                 "FROM isn't terminated by NULL "
                                 "on MAIL command: <<kou@cozmixng.org>>");
    actual_error = decode();
    gcut_assert_equal_error(expected_error, actual_error);
}

void
test_decode_rcpt (void)
{
    const gchar to[] = "<kou@cozmixng.org>";

    g_string_append(buffer, "R");
    g_string_append(buffer, to);
    g_string_append_c(buffer, '\0');

    gcut_assert_error(decode());
    cut_assert_equal_int(1, n_rcpts);
    cut_assert_equal_string(to, rcpt_to);
}

void
test_decode_rcpt_without_null (void)
{
    const gchar to[] = "<kou@cozmixng.org>";

    g_string_append(buffer, "R");
    g_string_append(buffer, to);

    expected_error = g_error_new(MILTER_DECODER_ERROR,
                                 MILTER_DECODER_ERROR_MISSING_NULL,
                                 "TO isn't terminated by NULL "
                                 "on RCPT command: <<kou@cozmixng.org>>");
    actual_error = decode();
    gcut_assert_equal_error(expected_error, actual_error);
}

void
test_decode_header (void)
{
    const gchar from[] = "<kou@cozmixng.org>";
    const gchar to[] = "<kou@cozmixng.org>";
    const gchar date[] = "Fri,  5 Sep 2008 09:19:56 +0900 (JST)";
    const gchar message_id[] = "<1ed5.0003.0000@delian>";

    g_string_append(buffer, "D");
    g_string_append(buffer, "L");
    append_name_and_value("i", "69FDD42DF4A");
    gcut_assert_error(decode());

    g_string_append(buffer, "L");
    append_name_and_value("From", from);
    gcut_assert_error(decode());
    cut_assert_equal_int(1, n_headers);
    cut_assert_equal_string("From", header_name);
    cut_assert_equal_string(from, header_value);


    g_string_append(buffer, "D");
    g_string_append(buffer, "L");
    append_name_and_value("i", "69FDD42DF4A");
    gcut_assert_error(decode());

    g_string_append(buffer, "L");
    append_name_and_value("To", to);
    gcut_assert_error(decode());
    cut_assert_equal_int(2, n_headers);
    cut_assert_equal_string("To", header_name);
    cut_assert_equal_string(to, header_value);


    g_string_append(buffer, "D");
    g_string_append(buffer, "L");
    append_name_and_value("i", "69FDD42DF4A");
    gcut_assert_error(decode());

    g_string_append(buffer, "L");
    append_name_and_value("Date", date);
    gcut_assert_error(decode());
    cut_assert_equal_int(3, n_headers);
    cut_assert_equal_string("Date", header_name);
    cut_assert_equal_string(date, header_value);


    g_string_append(buffer, "D");
    g_string_append(buffer, "L");
    append_name_and_value("i", "69FDD42DF4A");
    gcut_assert_error(decode());

    g_string_append(buffer, "L");
    append_name_and_value("Message-Id", message_id);
    gcut_assert_error(decode());
    cut_assert_equal_int(4, n_headers);
    cut_assert_equal_string("Message-Id", header_name);
    cut_assert_equal_string(message_id, header_value);
}

void
test_decode_header_without_name_null (void)
{
    g_string_append(buffer, "L");
    g_string_append(buffer, "From");

    expected_error = g_error_new(MILTER_DECODER_ERROR,
                                 MILTER_DECODER_ERROR_MISSING_NULL,
                                 "name isn't terminated by NULL "
                                 "on header command: <From>");
    actual_error = decode();
    gcut_assert_equal_error(expected_error, actual_error);
}

void
test_decode_header_without_value_null (void)
{
    const gchar from[] = "<kou@cozmixng.org>";

    g_string_append(buffer, "L");
    g_string_append(buffer, "From");
    g_string_append_c(buffer, '\0');
    g_string_append(buffer, from);

    expected_error = g_error_new(MILTER_DECODER_ERROR,
                                 MILTER_DECODER_ERROR_MISSING_NULL,
                                 "value isn't terminated by NULL "
                                 "on header command: <From>: "
                                 "<<kou@cozmixng.org>>");
    actual_error = decode();
    gcut_assert_equal_error(expected_error, actual_error);
}

void
test_decode_end_of_header (void)
{
    g_string_append(buffer, "D");
    g_string_append(buffer, "N");
    append_name_and_value("i", "69FDD42DF4A");
    gcut_assert_error(decode());

    g_string_append(buffer, "N");
    gcut_assert_error(decode());
    cut_assert_equal_int(1, n_end_of_headers);
}

void
test_decode_end_of_header_with_garbage (void)
{
    GString *message;
    const gchar garbage[] = "garbage";
    gsize i, garbage_length;

    g_string_append(buffer, "N");
    g_string_append(buffer, garbage);

    garbage_length = strlen(garbage);
    message = g_string_new(NULL);
    g_string_append_printf(message,
                           "needless %" G_GSIZE_FORMAT " bytes "
                           "were received on END OF HEADER command: ",
                           garbage_length);
    for (i = 0; i < garbage_length; i++) {
        g_string_append_printf(message, "0x%02x ", garbage[i]);
    }
    g_string_append_printf(message, "(%s)", garbage);
    expected_error = g_error_new(MILTER_DECODER_ERROR,
                                 MILTER_DECODER_ERROR_LONG_COMMAND_LENGTH,
                                 "%s", message->str);
    g_string_free(message, TRUE);

    actual_error = decode();
    gcut_assert_equal_error(expected_error, actual_error);
}

void
test_decode_body (void)
{
    const gchar body[] =
        "La de da de da 1.\n"
        "La de da de da 2.\n"
        "La de da de da 3.\n"
        "La de da de da 4.";

    g_string_append(buffer, "D");
    g_string_append(buffer, "B");
    append_name_and_value("i", "69FDD42DF4A");
    gcut_assert_error(decode());

    g_string_append(buffer, "B");
    g_string_append(buffer, body);
    gcut_assert_error(decode());
    cut_assert_equal_int(1, n_bodies);
    cut_assert_equal_string(body, body_chunk);
    cut_assert_equal_uint(strlen(body), body_chunk_length);
}

void
test_decode_body_end_with_data (void)
{
    const gchar body[] =
        "La de da de da 1.\n"
        "La de da de da 2.\n"
        "La de da de da 3.\n"
        "La de da de da 4.";

    g_string_append(buffer, "D");
    g_string_append(buffer, "E");
    append_name_and_value("i", "69FDD42DF4A");
    gcut_assert_error(decode());

    g_string_append(buffer, "E");
    g_string_append(buffer, body);
    gcut_assert_error(decode());
    cut_assert_equal_int(1, n_bodies);
    cut_assert_equal_string(body, body_chunk);
    cut_assert_equal_int(strlen(body), body_chunk_length);
    cut_assert_equal_int(1, n_end_of_messages);
}

void
test_decode_body_end_without_data (void)
{
    g_string_append(buffer, "D");
    g_string_append(buffer, "E");
    append_name_and_value("i", "69FDD42DF4A");
    gcut_assert_error(decode());

    g_string_append(buffer, "E");
    gcut_assert_error(decode());
    cut_assert_equal_int(0, n_bodies);
    cut_assert_equal_int(1, n_end_of_messages);
}

void
test_decode_abort (void)
{
    g_string_append(buffer, "A");
    gcut_assert_error(decode());

    cut_assert_equal_int(1, n_aborts);
}

void
test_decode_abort_with_garbage (void)
{
    g_string_append(buffer, "A");
    g_string_append_c(buffer, 0x09);

    expected_error = g_error_new(MILTER_DECODER_ERROR,
                                 MILTER_DECODER_ERROR_LONG_COMMAND_LENGTH,
                                 "needless 1 byte was received "
                                 "on ABORT command: 0x09");
    actual_error = decode();
    gcut_assert_equal_error(expected_error, actual_error);
}

void
test_decode_quit (void)
{
    g_string_append(buffer, "Q");
    gcut_assert_error(decode());

    cut_assert_equal_int(1, n_quits);
}

void
test_decode_quit_with_garbage (void)
{
    g_string_append(buffer, "Q");
    g_string_append(buffer, "XXX");

    expected_error = g_error_new(MILTER_DECODER_ERROR,
                                 MILTER_DECODER_ERROR_LONG_COMMAND_LENGTH,
                                 "needless 3 bytes were received "
                                 "on QUIT command: 0x58 0x58 0x58 (XXX)");
    actual_error = decode();
    gcut_assert_equal_error(expected_error, actual_error);
}

void
test_decode_unknown (void)
{
    const gchar command[] = "UNKNOWN COMMAND WITH ARGUMENT";

    g_string_append(buffer, "U");
    g_string_append(buffer, command);
    g_string_append_c(buffer, '\0');
    gcut_assert_error(decode());

    cut_assert_equal_int(1, n_unknowns);
    cut_assert_equal_string(command, unknown_command);
}

void
test_decode_unknown_without_null (void)
{
    const gchar command[] = "UNKNOWN COMMAND WITH ARGUMENT";

    g_string_append(buffer, "U");
    g_string_append(buffer, command);

    expected_error = g_error_new(MILTER_DECODER_ERROR,
                                 MILTER_DECODER_ERROR_MISSING_NULL,
                                 "command value isn't terminated by NULL "
                                 "on unknown command: <%s>",
                                 command);
    actual_error = decode();
    gcut_assert_equal_error(expected_error, actual_error);
}

void
test_decode_unexpected_command (void)
{
    g_string_append(buffer, "X");

    expected_error = g_error_new(MILTER_DECODER_ERROR,
                                 MILTER_DECODER_ERROR_UNEXPECTED_COMMAND,
                                 "unexpected command was received: X");
    actual_error = decode();
    gcut_assert_equal_error(expected_error, actual_error);
}


void
test_end_decode_immediately (void)
{
    cut_assert_true(milter_decoder_end_decode(decoder, &actual_error));
}

void
test_end_decode_in_command_length_decoding (void)
{
    cut_assert_true(milter_decoder_decode(decoder, "\0X", 2, &actual_error));
    cut_assert_false(milter_decoder_end_decode(decoder, &actual_error));

    expected_error = g_error_new(MILTER_DECODER_ERROR,
                                 MILTER_DECODER_ERROR_UNEXPECTED_END,
                                 "stream is ended unexpectedly: "
                                 "need more 2 bytes for decoding "
                                 "command length: 0x00 0x58");
    gcut_assert_equal_error(expected_error, actual_error);
}

void
test_end_decode_in_command_content_decoding (void)
{
    guint32 content_size;

    content_size = htonl(4);
    g_string_append_len(buffer, "\0\0\0\0abc", 7);
    memcpy(buffer->str, &content_size, sizeof(content_size));
    cut_assert_true(milter_decoder_decode(decoder, buffer->str, buffer->len,
                                        &actual_error));
    cut_assert_false(milter_decoder_end_decode(decoder, &actual_error));

    expected_error = g_error_new(MILTER_DECODER_ERROR,
                                 MILTER_DECODER_ERROR_UNEXPECTED_END,
                                 "stream is ended unexpectedly: "
                                 "need more 1 byte for decoding "
                                 "command content: 0x61 0x62 0x63 (abc)");
    gcut_assert_equal_error(expected_error, actual_error);
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
