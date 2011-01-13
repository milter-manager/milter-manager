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

#include <milter/core/milter-reply-encoder.h>
#include <milter/core/milter-enum-types.h>
#include <milter/core/milter-macros-requests.h>

#include <gcutter.h>

void test_encode_continue (void);
void test_encode_reply_code (void);
void data_encode_add_header (void);
void test_encode_add_header (gconstpointer data);
void data_encode_insert_header (void);
void test_encode_insert_header (gconstpointer data);
void data_encode_change_header (void);
void test_encode_change_header (gconstpointer data);
void data_encode_delete_header (void);
void test_encode_delete_header (gconstpointer data);
void data_encode_change_from (void);
void test_encode_change_from (gconstpointer data);
void data_encode_add_recipient (void);
void test_encode_add_recipient (gconstpointer data);
void data_encode_delete_recipient (void);
void test_encode_delete_recipient (gconstpointer data);
void test_encode_replace_body (void);
void test_encode_replace_body_large (void);
void test_encode_progress (void);
void data_encode_quarantine (void);
void test_encode_quarantine (gconstpointer data);
void test_encode_connection_failure (void);
void test_encode_shutdown (void);
void test_encode_skip (void);
void test_encode_negotiate (void);

static MilterReplyEncoder *encoder;
static GString *expected;
static MilterMacrosRequests *macros_requests;

void
setup (void)
{
    encoder = MILTER_REPLY_ENCODER(milter_reply_encoder_new());

    expected = g_string_new(NULL);

    macros_requests = NULL;
}

void
teardown (void)
{
    if (encoder) {
        g_object_unref(encoder);
    }

    if (expected)
        g_string_free(expected, TRUE);

    if (macros_requests)
        g_object_unref(macros_requests);
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
test_encode_continue (void)
{
    const gchar *actual;
    gsize actual_size = 0;

    g_string_append(expected, "c");
    pack(expected);

    milter_reply_encoder_encode_continue(encoder, &actual, &actual_size);
    cut_assert_equal_memory(expected->str, expected->len, actual, actual_size);
}

void
test_encode_reply_code (void)
{
    const gchar *actual;
    gsize actual_size = 0;
    const gchar code[] = "554 5.7.1 1% 2%% 3%%%";

    g_string_append(expected, "y");
    g_string_append(expected, "554 5.7.1 1% 2%% 3%%%");
    g_string_append_c(expected, '\0');
    pack(expected);

    milter_reply_encoder_encode_reply_code(encoder, &actual, &actual_size,
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
data_encode_add_header (void)
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
test_encode_add_header (gconstpointer data)
{
    const HeaderTestData *test_data = data;
    const gchar *actual;
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

    milter_reply_encoder_encode_add_header(encoder, &actual, &actual_size,
                                           test_data->name, test_data->value);
    cut_assert_equal_memory(expected->str, expected->len, actual, actual_size);
}

void
data_encode_insert_header (void)
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
test_encode_insert_header (gconstpointer data)
{
    const HeaderTestData *test_data = data;
    const gchar *actual;
    gsize actual_size = 0;

    if (test_data->name && test_data->name[0] != '\0') {
        guint32 normalized_index;
        gchar encoded_index[sizeof(guint32)];

        normalized_index = g_htonl(test_data->index);
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

    milter_reply_encoder_encode_insert_header(encoder, &actual, &actual_size,
                                              test_data->index,
                                              test_data->name, test_data->value);
    cut_assert_equal_memory(expected->str, expected->len, actual, actual_size);
}

void
data_encode_change_header (void)
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
test_encode_change_header (gconstpointer data)
{
    const HeaderTestData *test_data = data;
    const gchar *actual;
    gsize actual_size = 0;

    if (test_data->name && test_data->name[0] != '\0') {
        guint32 normalized_index;
        gchar encoded_index[sizeof(guint32)];

        normalized_index = g_htonl(test_data->index);
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

    milter_reply_encoder_encode_change_header(encoder, &actual, &actual_size,
                                              test_data->name,
                                              test_data->index,
                                              test_data->value);
    cut_assert_equal_memory(expected->str, expected->len, actual, actual_size);
}


void
data_encode_delete_header (void)
{
    cut_add_data("normal",
                 header_test_data_new("X-Virus-Status", NULL, 0),
                 header_test_data_free,
                 "no name",
                 header_test_data_new(NULL, NULL, 0),
                 header_test_data_free);
}

void
test_encode_delete_header (gconstpointer data)
{
    const HeaderTestData *test_data = data;
    const gchar *actual;
    gsize actual_size = 0;

    if (test_data->name && test_data->name[0] != '\0') {
        guint32 normalized_index;
        gchar encoded_index[sizeof(guint32)];

        normalized_index = g_htonl(test_data->index);
        memcpy(encoded_index, &normalized_index, sizeof(guint32));

        g_string_append(expected, "m");
        g_string_append_len(expected, encoded_index, sizeof(guint32));
        g_string_append(expected, test_data->name);
        g_string_append_c(expected, '\0');
        g_string_append_c(expected, '\0');
        pack(expected);
    }

    milter_reply_encoder_encode_delete_header(encoder, &actual, &actual_size,
                                              test_data->name,
                                              test_data->index);
    cut_assert_equal_memory(expected->str, expected->len, actual, actual_size);
}


void
data_encode_change_from (void)
{
    cut_add_data("from",
                 g_strsplit("kou@localhost", " ", 2),
                 g_strfreev,
                 "from - parameters",
                 g_strsplit("kou@localhost XXX", " ", 2),
                 g_strfreev,
                 "NULL",
                 NULL,
                 NULL,
                 "empty from",
                 g_strsplit("", " ", 2),
                 g_strfreev,
                 "empty from - parameters",
                 g_strsplit(" XXX", " ", 2),
                 g_strfreev,
                 "NULL from - parameters",
                 g_strsplit("(null) XXX", " ", 2),
                 g_strfreev);
}

void
test_encode_change_from (gconstpointer data)
{
    gchar * const *test_data = data;
    gchar *from = NULL;
    gchar *parameters = NULL;
    const gchar *actual;
    gsize actual_size = 0;

    if (test_data && test_data[0]) {
        from = test_data[0];
        parameters = test_data[1];
    }
    if (from && g_str_equal(from, "(null)"))
        from = NULL;

    if (from && from[0] != '\0') {
        g_string_append(expected, "e");
        g_string_append(expected, from);
        g_string_append_c(expected, '\0');
        if (parameters) {
            g_string_append(expected, parameters);
            g_string_append_c(expected, '\0');
        }
        pack(expected);
    }

    milter_reply_encoder_encode_change_from(encoder, &actual, &actual_size,
                                            from, parameters);
    cut_assert_equal_memory(expected->str, expected->len, actual, actual_size);
}

void
data_encode_add_recipient (void)
{
    cut_add_data("recipient",
                 g_strsplit("kou@localhost", " ", 2),
                 g_strfreev,
                 "recipient - parameters",
                 g_strsplit("kou@localhost XXX", " ", 2),
                 g_strfreev,
                 "NULL",
                 NULL,
                 NULL,
                 "empty recipient",
                 g_strsplit("", " ", 2),
                 g_strfreev,
                 "empty recipient - parameters",
                 g_strsplit(" XXX", " ", 2),
                 g_strfreev,
                 "NULL recipient - parameters",
                 g_strsplit("(null) XXX", " ", 2),
                 g_strfreev);
}

void
test_encode_add_recipient (gconstpointer data)
{
    gchar * const *test_data = data;
    gchar *recipient = NULL;
    gchar *parameters = NULL;
    const gchar *actual;
    gsize actual_size = 0;

    if (test_data && test_data[0]) {
        recipient = test_data[0];
        parameters = test_data[1];
    }
    if (recipient && g_str_equal(recipient, "(null)"))
        recipient = NULL;

    if (recipient && recipient[0] != '\0') {
        if (parameters)
            g_string_append(expected, "2");
        else
            g_string_append(expected, "+");
        g_string_append(expected, recipient);
        g_string_append_c(expected, '\0');
        if (parameters) {
            g_string_append(expected, parameters);
            g_string_append_c(expected, '\0');
        }
        pack(expected);
    }

    milter_reply_encoder_encode_add_recipient(encoder, &actual, &actual_size,
                                              recipient, parameters);
    cut_assert_equal_memory(expected->str, expected->len, actual, actual_size);
}

void
data_encode_delete_recipient (void)
{
    cut_add_data("recipient",
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
test_encode_delete_recipient (gconstpointer data)
{
    const gchar *recipient = data;
    const gchar *actual;
    gsize actual_size = 0;

    if (recipient && recipient[0] != '\0') {
        g_string_append(expected, "-");
        g_string_append(expected, recipient);
        g_string_append_c(expected, '\0');
        pack(expected);
    }

    milter_reply_encoder_encode_delete_recipient(encoder, &actual, &actual_size,
                                                 recipient);
    cut_assert_equal_memory(expected->str, expected->len, actual, actual_size);
}

void
test_encode_replace_body (void)
{
    const gchar body[] =
        "La de da de da 1.\n"
        "La de da de da 2.\n"
        "La de da de da 3.\n"
        "La de da de da 4.";
    gsize written_size = 0;
    const gchar *actual;
    gsize actual_size = 0;

    g_string_append(expected, "b");
    g_string_append(expected, body);
    pack(expected);

    milter_reply_encoder_encode_replace_body(encoder, &actual, &actual_size,
                                             body, sizeof(body) - 1,
                                             &written_size);
    cut_assert_equal_memory(expected->str, expected->len, actual, actual_size);
    cut_assert_equal_uint(sizeof(body) - 1, written_size);
}

void
test_encode_replace_body_large (void)
{
    GString *body;
    gsize i, body_size;
    gsize written_size = 0;
    const gchar *actual;
    gsize actual_size = 0;

    body = g_string_new(NULL);
    for (i = 0; i < MILTER_CHUNK_SIZE + 1; i++) {
        g_string_append_c(body, 'X');
    }

    g_string_append(expected, "b");
    g_string_append_len(expected, body->str, MILTER_CHUNK_SIZE);
    pack(expected);

    milter_reply_encoder_encode_replace_body(encoder, &actual, &actual_size,
                                             body->str, body->len,
                                             &written_size);
    body_size = body->len;
    g_string_free(body, TRUE);
    cut_assert_operator_int(body_size, >, MILTER_CHUNK_SIZE);
    cut_assert_equal_memory(expected->str, expected->len, actual, actual_size);
    cut_assert_equal_uint(MILTER_CHUNK_SIZE, written_size);
}

void
test_encode_progress (void)
{
    const gchar *actual;
    gsize actual_size = 0;

    g_string_append(expected, "p");
    pack(expected);

    milter_reply_encoder_encode_progress(encoder, &actual, &actual_size);
    cut_assert_equal_memory(expected->str, expected->len, actual, actual_size);
}

void
data_encode_quarantine (void)
{
    cut_add_data("reasonable reason",
                 g_strdup("virus mail!"),
                 g_free,
                 "no reason",
                 NULL,
                 NULL,
                 "empty reason",
                 g_strdup(""),
                 g_free);
}

void
test_encode_quarantine (gconstpointer data)
{
    const gchar *reason = data;
    const gchar *actual;
    gsize actual_size = 0;

    if (reason && reason[0] != '\0') {
        g_string_append(expected, "q");
        g_string_append(expected, reason);
        g_string_append_c(expected, '\0');
        pack(expected);
    }

    milter_reply_encoder_encode_quarantine(encoder, &actual, &actual_size,
                                           reason);
    cut_assert_equal_memory(expected->str, expected->len, actual, actual_size);
}

void
test_encode_connection_failure (void)
{
    const gchar *actual;
    gsize actual_size = 0;

    g_string_append(expected, "f");
    pack(expected);

    milter_reply_encoder_encode_connection_failure(encoder, &actual, &actual_size);
    cut_assert_equal_memory(expected->str, expected->len, actual, actual_size);
}

void
test_encode_shutdown (void)
{
    const gchar *actual;
    gsize actual_size = 0;

    g_string_append(expected, "4");
    pack(expected);

    milter_reply_encoder_encode_shutdown(encoder, &actual, &actual_size);
    cut_assert_equal_memory(expected->str, expected->len, actual, actual_size);
}

void
test_encode_skip (void)
{
    const gchar *actual;
    gsize actual_size = 0;

    g_string_append(expected, "s");
    pack(expected);

    milter_reply_encoder_encode_skip(encoder, &actual, &actual_size);
    cut_assert_equal_memory(expected->str, expected->len, actual, actual_size);
}

void
test_encode_negotiate (void)
{
    const gchar *actual;
    gsize actual_size = 0;
    gint32 stage;
    gchar encoded_stage[sizeof(gint32)];
    MilterOption *option;

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

    stage = g_htonl(MILTER_MACRO_STAGE_END_OF_HEADER);
    memcpy(encoded_stage, &stage, sizeof(encoded_stage));
    g_string_append_len(expected, encoded_stage, sizeof(encoded_stage));
    g_string_append(expected, "first second third"); /* The order is impornt here! FIXME! */
    g_string_append_c(expected, '\0');
    pack(expected);

    macros_requests = milter_macros_requests_new();
    milter_macros_requests_set_symbols(macros_requests,
                                       MILTER_COMMAND_END_OF_HEADER,
                                       "first",
                                       "second",
                                       "third",
                                       NULL);
    milter_reply_encoder_encode_negotiate(encoder, &actual, &actual_size,
                                          option, macros_requests);
    cut_assert_equal_memory(expected->str, expected->len, actual, actual_size);
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
