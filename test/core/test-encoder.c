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

#include <milter-core/milter-encoder.h>
#include <milter-core/milter-enum-types.h>

void test_encode_negotiate (void);
void test_encode_negotiate_null (void);
void data_encode_define_macro (void);
void test_encode_define_macro (gconstpointer data);
void test_encode_connect_ipv4 (void);
void test_encode_connect_ipv6 (void);
void test_encode_connect_unix (void);
void test_encode_helo (void);
void test_encode_mail (void);
void test_encode_rcpt (void);
void test_encode_header (void);
void test_encode_end_of_header (void);
void test_encode_body (void);
void test_encode_end_of_message (void);
void test_encode_end_of_message_with_data (void);
void test_encode_abort (void);
void test_encode_quit (void);
void test_encode_unknown (void);

void test_encode_reply_continue (void);
void test_encode_reply_reply_code (void);
void data_encode_reply_add_header (void);
void test_encode_reply_add_header (gconstpointer data);
void data_encode_reply_insert_header (void);
void test_encode_reply_insert_header (gconstpointer data);
void data_encode_reply_change_header (void);
void test_encode_reply_change_header (gconstpointer data);
void test_encode_reply_replace_body (void);
void test_encode_reply_replace_body_large (void);
void test_encode_reply_progress (void);

static MilterEncoder *encoder;
static GString *expected;
static gchar *actual;

static GHashTable *macros;

void
setup (void)
{
    encoder = milter_encoder_new();

    expected = g_string_new(NULL);
    actual = NULL;

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

    if (actual)
        g_free(actual);

    if (macros)
        g_hash_table_unref(macros);
}

static void
pack (GString *buffer)
{
    guint32 content_size;
    gchar content_string[sizeof(guint32)];

    content_size = htonl(buffer->len);
    memcpy(content_string, &content_size, sizeof(content_size));
    g_string_prepend_len(buffer, content_string, sizeof(content_size));
}

void
test_encode_negotiate (void)
{
    MilterOption *option;
    guint32 version_network_byte_order;
    guint32 action_network_byte_order;
    guint32 step_network_byte_order;
    gchar version_string[sizeof(guint32)];
    gchar action_string[sizeof(guint32)];
    gchar step_string[sizeof(guint32)];
    gsize actual_size = 0;

    option = milter_option_new(2,
                               MILTER_ACTION_ADD_HEADERS |
                               MILTER_ACTION_CHANGE_BODY |
                               MILTER_ACTION_ADD_RCPT |
                               MILTER_ACTION_DELETE_RCPT |
                               MILTER_ACTION_CHANGE_HEADERS |
                               MILTER_ACTION_QUARANTINE |
                               MILTER_ACTION_SET_SYMBOL_LIST,
                               MILTER_STEP_NO_CONNECT |
                               MILTER_STEP_NO_HELO |
                               MILTER_STEP_NO_MAIL |
                               MILTER_STEP_NO_RCPT |
                               MILTER_STEP_NO_BODY |
                               MILTER_STEP_NO_HEADERS |
                               MILTER_STEP_NO_END_OF_HEADER);

    version_network_byte_order = htonl(milter_option_get_version(option));
    action_network_byte_order = htonl(milter_option_get_action(option));
    step_network_byte_order = htonl(milter_option_get_step(option));

    memcpy(version_string, &version_network_byte_order, sizeof(guint32));
    memcpy(action_string, &action_network_byte_order, sizeof(guint32));
    memcpy(step_string, &step_network_byte_order, sizeof(guint32));

    g_string_append_c(expected, 'O');
    g_string_append_len(expected, version_string, sizeof(version_string));
    g_string_append_len(expected, action_string, sizeof(action_string));
    g_string_append_len(expected, step_string, sizeof(step_string));
    pack(expected);

    milter_encoder_encode_negotiate(encoder,
                                             &actual, &actual_size,
                                             option);
    g_object_unref(option);

    cut_assert_equal_memory(expected->str, expected->len, actual, actual_size);
}

void
test_encode_negotiate_null (void)
{
    gsize actual_size = 0;

    g_string_append_c(expected, 'O');
    pack(expected);
    milter_encoder_encode_negotiate(encoder,
                                             &actual, &actual_size,
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
    g_string_append(expected, "C");
    append_name_and_value(expected, "j", "debian.cozmixng.org");
    append_name_and_value(expected, "daemon_name", "debian.cozmixng.org");
    append_name_and_value(expected, "v", "Postfix 2.5.5");
}

static void
setup_define_macro_helo_packet (void)
{
    g_string_append(expected, "H");
}

static void
setup_define_macro_mail_packet (void)
{
    g_string_append(expected, "M");
    append_name_and_value(expected, "{mail_addr}", "kou@cozmixng.org");
}

static void
setup_define_macro_rcpt_packet (void)
{
    g_string_append(expected, "R");
    append_name_and_value(expected, "{rcpt_addr}", "kou@cozmixng.org");
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
                                            "j", "debian.cozmixng.org",
                                            "daemon_name", "debian.cozmixng.org",
                                            "v", "Postfix 2.5.5",
                                            NULL),
                 define_macro_test_data_free);

    cut_add_data("HELO",
                 define_macro_test_data_new(setup_define_macro_helo_packet,
                                            MILTER_COMMAND_HELO,
                                            NULL, NULL),
                 define_macro_test_data_free);

    cut_add_data("MAIL FROM",
                 define_macro_test_data_new(setup_define_macro_mail_packet,
                                            MILTER_COMMAND_MAIL,
                                            "{mail_addr}", "kou@cozmixng.org",
                                            NULL),
                 define_macro_test_data_free);

    cut_add_data("RCPT TO",
                 define_macro_test_data_new(setup_define_macro_rcpt_packet,
                                            MILTER_COMMAND_RCPT,
                                            "{rcpt_addr}", "kou@cozmixng.org",
                                            NULL),
                 define_macro_test_data_free);

    cut_add_data("header",
                 define_macro_test_data_new(setup_define_macro_header_packet,
                                            MILTER_COMMAND_HEADER,
                                            "i", "69FDD42DF4A",
                                            NULL),
                 define_macro_test_data_free);
}

void
test_encode_define_macro (gconstpointer data)
{
    const DefineMacroTestData *test_data = data;
    gsize actual_size = 0;

    g_string_append(expected, "D");
    test_data->setup_packet();
    pack(expected);

    milter_encoder_encode_define_macro(encoder,
                                       &actual, &actual_size,
                                       test_data->context, test_data->expected);
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
    gsize actual_size = 0;


    port = htons(50443);
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
    inet_aton(ip_address, &(address.sin_addr));

    milter_encoder_encode_connect(encoder,
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
    gsize actual_size = 0;

    port = htons(50443);
    memcpy(port_string, &port, sizeof(port));

    g_string_append(expected, "C");
    g_string_append(expected, host_name);
    g_string_append_c(expected, '\0');
    g_string_append(expected, "6");
    g_string_append_len(expected, port_string, sizeof(port_string));
    g_string_append(expected, ipv6_address);
    g_string_append_c(expected, '\0');
    pack(expected);

    address.sin6_family = AF_INET6;
    address.sin6_port = port;
    inet_pton(AF_INET6, ipv6_address, &(address.sin6_addr));

    milter_encoder_encode_connect(encoder,
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
    gsize actual_size = 0;

    port = htons(0);
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

    milter_encoder_encode_connect(encoder,
                                  &actual, &actual_size,
                                  host_name,
                                  (const struct sockaddr *)&address,
                                  sizeof(address));
    cut_assert_equal_memory(expected->str, expected->len, actual, actual_size);
}

void
test_encode_helo (void)
{
    const gchar fqdn[] = "delian";
    gsize actual_size = 0;

    g_string_append(expected, "H");
    g_string_append(expected, fqdn);
    g_string_append_c(expected, '\0');
    pack(expected);

    milter_encoder_encode_helo(encoder, &actual, &actual_size, fqdn);
    cut_assert_equal_memory(expected->str, expected->len, actual, actual_size);
}

void
test_encode_mail (void)
{
    const gchar from[] = "<kou@cozmixng.org>";
    gsize actual_size = 0;

    g_string_append(expected, "M");
    g_string_append(expected, from);
    g_string_append_c(expected, '\0');
    pack(expected);

    milter_encoder_encode_mail(encoder, &actual, &actual_size, from);
    cut_assert_equal_memory(expected->str, expected->len, actual, actual_size);
}

void
test_encode_rcpt (void)
{
    const gchar to[] = "<kou@cozmixng.org>";
    gsize actual_size = 0;

    g_string_append(expected, "R");
    g_string_append(expected, to);
    g_string_append_c(expected, '\0');
    pack(expected);

    milter_encoder_encode_rcpt(encoder, &actual, &actual_size, to);
    cut_assert_equal_memory(expected->str, expected->len, actual, actual_size);
}

void
test_encode_header (void)
{
    const gchar from[] = "<kou@cozmixng.org>";
    gsize actual_size = 0;

    g_string_append(expected, "L");
    g_string_append(expected, "From");
    g_string_append_c(expected, '\0');
    g_string_append(expected, from);
    g_string_append_c(expected, '\0');
    pack(expected);

    milter_encoder_encode_header(encoder, &actual, &actual_size, "From", from);
    cut_assert_equal_memory(expected->str, expected->len, actual, actual_size);
}

void
test_encode_end_of_header (void)
{
    gsize actual_size = 0;

    g_string_append(expected, "N");
    pack(expected);

    milter_encoder_encode_end_of_header(encoder, &actual, &actual_size);
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
    gsize actual_size = 0;

    g_string_append(expected, "B");
    g_string_append_len(expected, body, sizeof(body));
    pack(expected);

    milter_encoder_encode_body(encoder, &actual, &actual_size,
                               body, sizeof(body));
    cut_assert_equal_memory(expected->str, expected->len, actual, actual_size);
}

void
test_encode_end_of_message (void)
{
    gsize actual_size = 0;

    g_string_append(expected, "E");
    pack(expected);

    milter_encoder_encode_end_of_message(encoder, &actual, &actual_size,
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
    gsize actual_size = 0;

    g_string_append(expected, "E");
    g_string_append_len(expected, body, sizeof(body));
    pack(expected);

    milter_encoder_encode_end_of_message(encoder, &actual, &actual_size,
                                         body, sizeof(body));
    cut_assert_equal_memory(expected->str, expected->len, actual, actual_size);
}

void
test_encode_abort (void)
{
    gsize actual_size = 0;

    g_string_append(expected, "A");
    pack(expected);

    milter_encoder_encode_abort(encoder, &actual, &actual_size);
    cut_assert_equal_memory(expected->str, expected->len, actual, actual_size);
}

void
test_encode_quit (void)
{
    gsize actual_size = 0;

    g_string_append(expected, "Q");
    pack(expected);

    milter_encoder_encode_quit(encoder, &actual, &actual_size);
    cut_assert_equal_memory(expected->str, expected->len, actual, actual_size);
}

void
test_encode_unknown (void)
{
    const gchar command[] = "UNKNOWN COMMAND";
    gsize actual_size = 0;

    g_string_append(expected, "U");
    g_string_append(expected, command);
    g_string_append_c(expected, '\0');
    pack(expected);

    milter_encoder_encode_unknown(encoder, &actual, &actual_size, command);
    cut_assert_equal_memory(expected->str, expected->len, actual, actual_size);
}

void
test_encode_reply_continue (void)
{
    gsize actual_size = 0;

    g_string_append(expected, "c");
    pack(expected);

    milter_encoder_encode_reply_continue(encoder, &actual, &actual_size);
    cut_assert_equal_memory(expected->str, expected->len, actual, actual_size);
}

void
test_encode_reply_reply_code (void)
{
    gsize actual_size = 0;
    const gchar code[] = "554 5.7.1 1% 2%% 3%%%";

    g_string_append(expected, "y");
    g_string_append(expected, "554 5.7.1 1% 2%% 3%%%");
    g_string_append_c(expected, '\0');
    pack(expected);

    milter_encoder_encode_reply_reply_code(encoder, &actual, &actual_size,
                                           code);
    cut_assert_equal_memory(expected->str, expected->len, actual, actual_size);
}

typedef struct _HeaderTestData
{
    gchar *name;
    gchar *value;
    guint32 index;
} HeaderTestData;

static HeaderTestData *
header_test_data_new (const gchar *name, const gchar *value, guint32 index)
{
    HeaderTestData *data;

    data = g_new(HeaderTestData, 1);
    data->name = g_strdup(name);
    data->value = g_strdup(value);
    data->index = index;

    return data;
}

static void
header_test_data_free (HeaderTestData *data)
{
    g_free(data->name);
    g_free(data->value);
    g_free(data);
}

void
data_encode_reply_add_header (void)
{
    cut_add_data("normal",
                 header_test_data_new("X-Virus-Status", "Clean", 0),
                 header_test_data_free,
                 "no name",
                 header_test_data_new(NULL, "Clean", 0),
                 header_test_data_free,
                 "empty name",
                 header_test_data_new("", "Clean", 0),
                 header_test_data_free,
                 "no value",
                 header_test_data_new("X-Virus-Status", NULL, 0),
                 header_test_data_free,
                 "no name - no value",
                 header_test_data_new(NULL, NULL, 0),
                 header_test_data_free);
}

void
test_encode_reply_add_header (gconstpointer data)
{
    const HeaderTestData *test_data = data;
    gsize actual_size = 0;

    if (test_data->name && test_data->name[0] != '\0') {
        g_string_append(expected, "h");
        g_string_append(expected, test_data->name);
        g_string_append_c(expected, '\0');
        if (test_data->value)
            g_string_append(expected, test_data->value);
        g_string_append_c(expected, '\0');
        pack(expected);
    }

    milter_encoder_encode_reply_add_header(encoder, &actual, &actual_size,
                                           test_data->name, test_data->value);
    cut_assert_equal_memory(expected->str, expected->len, actual, actual_size);
}

void
data_encode_reply_insert_header (void)
{
    cut_add_data("normal",
                 header_test_data_new("X-Virus-Status", "Clean", 2),
                 header_test_data_free,
                 "no name",
                 header_test_data_new(NULL, "Clean", 2),
                 header_test_data_free,
                 "empty name",
                 header_test_data_new("", "Clean", 2),
                 header_test_data_free,
                 "no value",
                 header_test_data_new("X-Virus-Status", NULL, 2),
                 header_test_data_free,
                 "no name - no value",
                 header_test_data_new(NULL, NULL, 2),
                 header_test_data_free);
}

void
test_encode_reply_insert_header (gconstpointer data)
{
    const HeaderTestData *test_data = data;
    gsize actual_size = 0;

    if (test_data->name && test_data->name[0] != '\0') {
        guint32 normalized_index;
        gchar encoded_index[sizeof(guint32)];

        normalized_index = htonl(test_data->index);
        memcpy(encoded_index, &normalized_index, sizeof(guint32));

        g_string_append(expected, "i");
        g_string_append_len(expected, encoded_index, sizeof(guint32));
        g_string_append(expected, test_data->name);
        g_string_append_c(expected, '\0');
        if (test_data->value)
            g_string_append(expected, test_data->value);
        g_string_append_c(expected, '\0');
        pack(expected);
    }

    milter_encoder_encode_reply_insert_header(encoder, &actual, &actual_size,
                                              test_data->index,
                                              test_data->name, test_data->value);
    cut_assert_equal_memory(expected->str, expected->len, actual, actual_size);
}

void
data_encode_reply_change_header (void)
{
    cut_add_data("normal",
                 header_test_data_new("X-Virus-Status", "Clean", 0),
                 header_test_data_free,
                 "no name",
                 header_test_data_new(NULL, "Clean", 0),
                 header_test_data_free,
                 "empty name",
                 header_test_data_new("", "Clean", 0),
                 header_test_data_free,
                 "no value",
                 header_test_data_new("X-Virus-Status", NULL, 0),
                 header_test_data_free,
                 "no name - no value",
                 header_test_data_new(NULL, NULL, 0),
                 header_test_data_free);
}

void
test_encode_reply_change_header (gconstpointer data)
{
    const HeaderTestData *test_data = data;
    gsize actual_size = 0;

    if (test_data->name && test_data->name[0] != '\0') {
        guint32 normalized_index;
        gchar encoded_index[sizeof(guint32)];

        normalized_index = htonl(test_data->index);
        memcpy(encoded_index, &normalized_index, sizeof(guint32));

        g_string_append(expected, "m");
        g_string_append_len(expected, encoded_index, sizeof(guint32));
        g_string_append(expected, test_data->name);
        g_string_append_c(expected, '\0');
        if (test_data->value)
            g_string_append(expected, test_data->value);
        g_string_append_c(expected, '\0');
        pack(expected);
    }

    milter_encoder_encode_reply_change_header(encoder, &actual, &actual_size,
                                              test_data->name,
                                              test_data->index,
                                              test_data->value);
    cut_assert_equal_memory(expected->str, expected->len, actual, actual_size);
}

void
test_encode_reply_replace_body (void)
{
    const gchar body[] =
        "La de da de da 1.\n"
        "La de da de da 2.\n"
        "La de da de da 3.\n"
        "La de da de da 4.";
    gsize written_size = 0;
    gsize actual_size = 0;

    g_string_append(expected, "b");
    g_string_append(expected, body);
    pack(expected);

    milter_encoder_encode_reply_replace_body(encoder, &actual, &actual_size,
                                             body, sizeof(body) - 1,
                                             &written_size);
    cut_assert_equal_memory(expected->str, expected->len, actual, actual_size);
    cut_assert_equal_uint(sizeof(body) - 1, written_size);
}

void
test_encode_reply_replace_body_large (void)
{
    GString *body;
    gsize i, body_size;
    gsize written_size = 0;
    gsize actual_size = 0;

    body = g_string_new(NULL);
    for (i = 0; i < MILTER_CHUNK_SIZE + 1; i++) {
        g_string_append_c(body, 'X');
    }

    g_string_append(expected, "b");
    g_string_append_len(expected, body->str, MILTER_CHUNK_SIZE);
    pack(expected);

    milter_encoder_encode_reply_replace_body(encoder, &actual, &actual_size,
                                             body->str, body->len,
                                             &written_size);
    body_size = body->len;
    g_string_free(body, TRUE);
    cut_assert_operator_int(body_size, >, MILTER_CHUNK_SIZE);
    cut_assert_equal_memory(expected->str, expected->len, actual, actual_size);
    cut_assert_equal_uint(MILTER_CHUNK_SIZE, written_size);
}

void
test_encode_reply_progress (void)
{
    gsize actual_size = 0;

    g_string_append(expected, "p");
    pack(expected);

    milter_encoder_encode_reply_progress(encoder, &actual, &actual_size);
    cut_assert_equal_memory(expected->str, expected->len, actual, actual_size);
}


/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
