/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Copyright (C) 2008-2011  Kouhei Sutou <kou@clear-code.com>
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

#include <string.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <arpa/inet.h>

#include <milter/core/milter-command-encoder.h>
#include <milter/core/milter-enum-types.h>
#include <milter/core/milter-macros-requests.h>

#include <gcutter.h>

void test_encode_negotiate (void);
void test_encode_negotiate_null (void);
void data_encode_define_macro (void);
void test_encode_define_macro (gconstpointer data);
void test_encode_connect_ipv4 (void);
void test_encode_connect_ipv6 (void);
void test_encode_connect_unix (void);
void test_encode_connect_unknown (void);
void test_encode_helo (void);
void test_encode_envelope_from (void);
void test_encode_envelope_recipient (void);
void test_encode_data (void);
void test_encode_header (void);
void test_encode_end_of_header (void);
void test_encode_body (void);
void test_encode_end_of_message (void);
void test_encode_end_of_message_with_data (void);
void test_encode_abort (void);
void test_encode_quit (void);
void test_encode_unknown (void);

static MilterCommandEncoder *encoder;
static GString *expected;
static GHashTable *macros;

void
setup (void)
{
    encoder = MILTER_COMMAND_ENCODER(milter_command_encoder_new());

    expected = g_string_new(NULL);

    macros = NULL;
}

void
teardown (void)
{
    if (encoder) {
        g_object_unref(encoder);
    }

    if (expected)
        g_string_free(expected, TRUE);

    if (macros)
        g_hash_table_unref(macros);
}

static void
pack (GString *buffer)
{
    guint32 content_size;
    gchar content_string[sizeof(guint32)];

    content_size = g_htonl(buffer->len);
    memcpy(content_string, &content_size, sizeof(content_size));
    g_string_prepend_len(buffer, content_string, sizeof(content_size));
}

static void
encode_negotiate_with_option (GString *buffer, MilterOption *option)
{
    guint32 version_network_byte_order;
    guint32 action_network_byte_order;
    guint32 step_network_byte_order;
    gchar version_string[sizeof(guint32)];
    gchar action_string[sizeof(guint32)];
    gchar step_string[sizeof(guint32)];

    version_network_byte_order = g_htonl(milter_option_get_version(option));
    action_network_byte_order = g_htonl(milter_option_get_action(option));
    step_network_byte_order = g_htonl(milter_option_get_step(option));

    memcpy(version_string, &version_network_byte_order, sizeof(guint32));
    memcpy(action_string, &action_network_byte_order, sizeof(guint32));
    memcpy(step_string, &step_network_byte_order, sizeof(guint32));

    g_string_append_c(buffer, 'O');
    g_string_append_len(buffer, version_string, sizeof(version_string));
    g_string_append_len(buffer, action_string, sizeof(action_string));
    g_string_append_len(buffer, step_string, sizeof(step_string));
}

void
test_encode_negotiate (void)
{
    MilterOption *option;
    const gchar *actual;
    gsize actual_size = 0;

    option = milter_option_new(2,
                               MILTER_ACTION_ADD_HEADERS |
                               MILTER_ACTION_CHANGE_BODY |
                               MILTER_ACTION_ADD_ENVELOPE_RECIPIENT |
                               MILTER_ACTION_DELETE_ENVELOPE_RECIPIENT |
                               MILTER_ACTION_CHANGE_HEADERS |
                               MILTER_ACTION_QUARANTINE |
                               MILTER_ACTION_SET_SYMBOL_LIST,
                               MILTER_STEP_NO_CONNECT |
                               MILTER_STEP_NO_HELO |
                               MILTER_STEP_NO_ENVELOPE_FROM |
                               MILTER_STEP_NO_ENVELOPE_RECIPIENT |
                               MILTER_STEP_NO_BODY |
                               MILTER_STEP_NO_HEADERS |
                               MILTER_STEP_NO_END_OF_HEADER);

    encode_negotiate_with_option(expected, option);
    pack(expected);

    milter_command_encoder_encode_negotiate(encoder, &actual, &actual_size,
                                            option);
    g_object_unref(option);

    cut_assert_equal_memory(expected->str, expected->len, actual, actual_size);
}

void
test_encode_negotiate_null (void)
{
    const gchar *actual;
    gsize actual_size = 0;

    g_string_append_c(expected, 'O');
    pack(expected);
    milter_command_encoder_encode_negotiate(encoder, &actual, &actual_size,
                                            NULL);
    cut_assert_equal_memory(expected->str, expected->len, actual, actual_size);
}

static void
append_name_and_value(GString *buffer, const gchar *name, const gchar *value)
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
    MilterCommand context;
    GHashTable *expected;
} DefineMacroTestData;

static DefineMacroTestData *define_macro_test_data_new(setup_packet_func setup_packet,
                                                       MilterCommand context,
                                                       const gchar *key,
                                                       ...) G_GNUC_NULL_TERMINATED;
static DefineMacroTestData *
define_macro_test_data_new (setup_packet_func setup_packet,
                            MilterCommand context, const gchar *key,
                            ...)
{
    DefineMacroTestData *data;
    va_list args;

    data = g_new0(DefineMacroTestData, 1);

    data->setup_packet = setup_packet;
    data->context = context;
    va_start(args, key);
    data->expected = gcut_hash_table_string_string_new_va_list(key, args);
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
    g_string_append(expected, "C");
    append_name_and_value(expected, "{daemon_name}", "debian.example.com");
    append_name_and_value(expected, "j", "debian.example.com");
    append_name_and_value(expected, "v", "Postfix 2.5.5");
}

static void
setup_define_macro_helo_packet (void)
{
    g_string_append(expected, "H");
}

static void
setup_define_macro_envelope_from_packet (void)
{
    g_string_append(expected, "M");
    append_name_and_value(expected, "{mail_addr}", "kou@example.com");
}

static void
setup_define_macro_envelope_recipient_packet (void)
{
    g_string_append(expected, "R");
    append_name_and_value(expected, "{rcpt_addr}", "kou@example.com");
}

static void
setup_define_macro_header_packet (void)
{
    g_string_append(expected, "L");
    append_name_and_value(expected, "i", "69FDD42DF4A");
}

void
data_encode_define_macro (void)
{
    cut_add_data("connect",
                 define_macro_test_data_new(setup_define_macro_connect_packet,
                                            MILTER_COMMAND_CONNECT,
                                            "j", "debian.example.com",
                                            "daemon_name", "debian.example.com",
                                            "v", "Postfix 2.5.5",
                                            NULL),
                 define_macro_test_data_free);

    cut_add_data("HELO",
                 define_macro_test_data_new(setup_define_macro_helo_packet,
                                            MILTER_COMMAND_HELO,
                                            NULL, NULL),
                 define_macro_test_data_free);

    cut_add_data("MAIL FROM",
                 define_macro_test_data_new(
                     setup_define_macro_envelope_from_packet,
                     MILTER_COMMAND_ENVELOPE_FROM,
                     "mail_addr", "kou@example.com",
                     NULL),
                 define_macro_test_data_free);

    cut_add_data("RCPT TO",
                 define_macro_test_data_new(
                     setup_define_macro_envelope_recipient_packet,
                     MILTER_COMMAND_ENVELOPE_RECIPIENT,
                     "rcpt_addr", "kou@example.com",
                     NULL),
                 define_macro_test_data_free);

    cut_add_data("header",
                 define_macro_test_data_new(
                     setup_define_macro_header_packet,
                     MILTER_COMMAND_HEADER,
                     "i", "69FDD42DF4A",
                     NULL),
                 define_macro_test_data_free);
}

void
test_encode_define_macro (gconstpointer data)
{
    const DefineMacroTestData *test_data = data;
    const gchar *actual;
    gsize actual_size = 0;

    g_string_append(expected, "D");
    test_data->setup_packet();
    pack(expected);

    milter_command_encoder_encode_define_macro(encoder,
                                               &actual, &actual_size,
                                               test_data->context,
                                               test_data->expected);
    cut_assert_equal_memory(expected->str, expected->len, actual, actual_size);
}

void
test_encode_connect_ipv4 (void)
{
    struct sockaddr_in address;
    const gchar host_name[] = "mx.local.net";
    const gchar ip_address[] = "192.168.123.123";
    gchar port_string[sizeof(uint16_t)];
    uint16_t port;
    const gchar *actual;
    gsize actual_size = 0;


    port = g_htons(50443);
    memcpy(port_string, &port, sizeof(port));

    g_string_append(expected, "C");
    g_string_append(expected, host_name);
    g_string_append_c(expected, '\0');
    g_string_append(expected, "4");
    g_string_append_len(expected, port_string, sizeof(port_string));
    g_string_append(expected, ip_address);
    g_string_append_c(expected, '\0');
    pack(expected);

    address.sin_family = AF_INET;
    address.sin_port = port;
    inet_pton(AF_INET, ip_address, &(address.sin_addr));

    milter_command_encoder_encode_connect(encoder,
                                          &actual, &actual_size,
                                          host_name,
                                          (const struct sockaddr *)&address,
                                          sizeof(address));
    cut_assert_equal_memory(expected->str, expected->len, actual, actual_size);
}

void
test_encode_connect_ipv6 (void)
{
    struct sockaddr_in6 address;
    const gchar host_name[] = "mx.local.net";
    const gchar ipv6_address[] = "2001:c90:625:12e8:290:ccff:fee2:80c5";
    gchar port_string[sizeof(uint16_t)];
    uint16_t port;
    const gchar *actual;
    gsize actual_size = 0;

    port = g_htons(50443);
    memcpy(port_string, &port, sizeof(port));

    g_string_append(expected, "C");
    g_string_append(expected, host_name);
    g_string_append_c(expected, '\0');
    g_string_append(expected, "6");
    g_string_append_len(expected, port_string, sizeof(port_string));
    g_string_append_printf(expected, "IPv6:%s", ipv6_address);
    g_string_append_c(expected, '\0');
    pack(expected);

    address.sin6_family = AF_INET6;
    address.sin6_port = port;
    inet_pton(AF_INET6, ipv6_address, &(address.sin6_addr));

    milter_command_encoder_encode_connect(encoder,
                                          &actual, &actual_size,
                                          host_name,
                                          (const struct sockaddr *)&address,
                                          sizeof(address));
    cut_assert_equal_memory(expected->str, expected->len, actual, actual_size);
}

void
test_encode_connect_unix (void)
{
    struct sockaddr_un address;
    const gchar host_name[] = "mx.local.net";
    const gchar path[] = "/tmp/unix.sock";
    gchar port_string[sizeof(uint16_t)];
    uint16_t port;
    const gchar *actual;
    gsize actual_size = 0;

    port = g_htons(0);
    memcpy(port_string, &port, sizeof(port));

    g_string_append(expected, "C");
    g_string_append(expected, host_name);
    g_string_append_c(expected, '\0');
    g_string_append(expected, "L");
    g_string_append_len(expected, port_string, sizeof(port_string));
    g_string_append(expected, path);
    g_string_append_c(expected, '\0');
    pack(expected);


    address.sun_family = AF_UNIX;
    strcpy(address.sun_path, path);

    milter_command_encoder_encode_connect(encoder,
                                          &actual, &actual_size,
                                          host_name,
                                          (const struct sockaddr *)&address,
                                          sizeof(address));
    cut_assert_equal_memory(expected->str, expected->len, actual, actual_size);
}

void
test_encode_connect_unknown (void)
{
    struct sockaddr address;
    const gchar host_name[] = "unknown";
    const gchar *actual;
    gsize actual_size = 0;

    g_string_append(expected, "C");
    g_string_append(expected, host_name);
    g_string_append_c(expected, '\0');
    g_string_append(expected, "U");
    pack(expected);

    address.sa_family = AF_UNSPEC;

    milter_command_encoder_encode_connect(encoder,
                                          &actual, &actual_size,
                                          host_name,
                                          &address,
                                          sizeof(address));
    cut_assert_equal_memory(expected->str, expected->len, actual, actual_size);
}

void
test_encode_helo (void)
{
    const gchar fqdn[] = "delian";
    const gchar *actual;
    gsize actual_size = 0;

    g_string_append(expected, "H");
    g_string_append(expected, fqdn);
    g_string_append_c(expected, '\0');
    pack(expected);

    milter_command_encoder_encode_helo(encoder, &actual, &actual_size, fqdn);
    cut_assert_equal_memory(expected->str, expected->len, actual, actual_size);
}

void
test_encode_envelope_from (void)
{
    const gchar from[] = "<kou@example.com>";
    const gchar *actual;
    gsize actual_size = 0;

    g_string_append(expected, "M");
    g_string_append(expected, from);
    g_string_append_c(expected, '\0');
    pack(expected);

    milter_command_encoder_encode_envelope_from(encoder,
                                                &actual, &actual_size, from);
    cut_assert_equal_memory(expected->str, expected->len, actual, actual_size);
}

void
test_encode_envelope_recipient (void)
{
    const gchar to[] = "<kou@example.com>";
    const gchar *actual;
    gsize actual_size = 0;

    g_string_append(expected, "R");
    g_string_append(expected, to);
    g_string_append_c(expected, '\0');
    pack(expected);

    milter_command_encoder_encode_envelope_recipient(encoder,
                                                     &actual, &actual_size,
                                                     to);
    cut_assert_equal_memory(expected->str, expected->len, actual, actual_size);
}

void
test_encode_data (void)
{
    const gchar *actual;
    gsize actual_size = 0;

    g_string_append(expected, "T");
    pack(expected);

    milter_command_encoder_encode_data(encoder, &actual, &actual_size);
    cut_assert_equal_memory(expected->str, expected->len, actual, actual_size);
}

void
test_encode_header (void)
{
    const gchar from[] = "<kou@example.com>";
    const gchar *actual;
    gsize actual_size = 0;

    g_string_append(expected, "L");
    g_string_append(expected, "From");
    g_string_append_c(expected, '\0');
    g_string_append(expected, from);
    g_string_append_c(expected, '\0');
    pack(expected);

    milter_command_encoder_encode_header(encoder,
                                         &actual, &actual_size,
                                         "From", from);
    cut_assert_equal_memory(expected->str, expected->len, actual, actual_size);
}

void
test_encode_end_of_header (void)
{
    const gchar *actual;
    gsize actual_size = 0;

    g_string_append(expected, "N");
    pack(expected);

    milter_command_encoder_encode_end_of_header(encoder, &actual, &actual_size);
    cut_assert_equal_memory(expected->str, expected->len, actual, actual_size);
}

void
test_encode_body (void)
{
    const gchar body[] =
        "La de da de da 1.\n"
        "La de da de da 2.\n"
        "La de da de da 3.\n"
        "La de da de da 4.";
    const gchar *actual;
    gsize actual_size = 0, packed_size;

    g_string_append(expected, "B");
    g_string_append_len(expected, body, sizeof(body));
    pack(expected);

    milter_command_encoder_encode_body(encoder, &actual, &actual_size,
                                       body, sizeof(body), &packed_size);
    cut_assert_equal_memory(expected->str, expected->len, actual, actual_size);
    cut_assert_equal_uint(sizeof(body), packed_size);
}

void
test_encode_end_of_message (void)
{
    const gchar *actual;
    gsize actual_size = 0;

    g_string_append(expected, "E");
    pack(expected);

    milter_command_encoder_encode_end_of_message(encoder, &actual, &actual_size,
                                                 NULL, 0);
    cut_assert_equal_memory(expected->str, expected->len, actual, actual_size);
}

void
test_encode_end_of_message_with_data (void)
{
    const gchar body[] =
        "La de da de da 1.\n"
        "La de da de da 2.\n"
        "La de da de da 3.\n"
        "La de da de da 4.";
    const gchar *actual;
    gsize actual_size = 0;

    g_string_append(expected, "E");
    g_string_append_len(expected, body, sizeof(body));
    pack(expected);

    milter_command_encoder_encode_end_of_message(encoder, &actual, &actual_size,
                                                 body, sizeof(body));
    cut_assert_equal_memory(expected->str, expected->len, actual, actual_size);
}

void
test_encode_abort (void)
{
    const gchar *actual;
    gsize actual_size = 0;

    g_string_append(expected, "A");
    pack(expected);

    milter_command_encoder_encode_abort(encoder, &actual, &actual_size);
    cut_assert_equal_memory(expected->str, expected->len, actual, actual_size);
}

void
test_encode_quit (void)
{
    const gchar *actual;
    gsize actual_size = 0;

    g_string_append(expected, "Q");
    pack(expected);

    milter_command_encoder_encode_quit(encoder, &actual, &actual_size);
    cut_assert_equal_memory(expected->str, expected->len, actual, actual_size);
}

void
test_encode_unknown (void)
{
    const gchar command[] = "UNKNOWN COMMAND";
    const gchar *actual;
    gsize actual_size = 0;

    g_string_append(expected, "U");
    g_string_append(expected, command);
    g_string_append_c(expected, '\0');
    pack(expected);

    milter_command_encoder_encode_unknown(encoder, &actual, &actual_size,
                                          command);
    cut_assert_equal_memory(expected->str, expected->len, actual, actual_size);
}


/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
