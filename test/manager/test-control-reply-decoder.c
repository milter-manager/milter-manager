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
 *  along with this library.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <string.h>

#include <milter/manager/milter-manager-control-reply-decoder.h>

#include <gcutter.h>

void test_decode_success (void);
void test_decode_failure (void);
void test_decode_error (void);
void test_decode_configuration (void);
void test_decode_unknown (void);

static MilterDecoder *decoder;
static GString *buffer;

static GError *expected_error;
static GError *actual_error;

static gint n_success_received;
static gint n_failure_received;
static gint n_error_received;
static gint n_configuration_received;

static gchar *actual_failure_message;
static gchar *actual_error_message;
static gchar *actual_configuration;
static gsize actual_configuration_size;

static void
cb_success (MilterManagerControlReplyDecoder *decoder, gpointer user_data)
{
    n_success_received++;
}

static void
cb_failure (MilterManagerControlReplyDecoder *decoder,
            const gchar *message, gpointer user_data)
{
    n_failure_received++;

    if (actual_failure_message)
        g_free(actual_failure_message);
    actual_failure_message = g_strdup(message);
}

static void
cb_error (MilterManagerControlReplyDecoder *decoder,
          const gchar *message, gpointer user_data)
{
    n_error_received++;

    if (actual_error_message)
        g_free(actual_error_message);
    actual_error_message = g_strdup(message);
}

static void
cb_configuration (MilterManagerControlReplyDecoder *decoder,
                  const gchar *configuration, gsize configuration_size,
                  gpointer user_data)
{
    n_configuration_received++;

    if (actual_configuration)
        g_free(actual_configuration);
    actual_configuration = g_memdup(configuration, configuration_size);
    actual_configuration_size = configuration_size;
}

static void
setup_signals (MilterDecoder *decoder)
{
#define CONNECT(name)                                                   \
    g_signal_connect(decoder, #name, G_CALLBACK(cb_ ## name), NULL)

    CONNECT(success);
    CONNECT(failure);
    CONNECT(error);
    CONNECT(configuration);

#undef CONNECT
}

void
setup (void)
{
    decoder = milter_manager_control_reply_decoder_new();
    setup_signals(decoder);

    expected_error = NULL;
    actual_error = NULL;

    n_success_received = 0;
    n_failure_received = 0;
    n_error_received = 0;
    n_configuration_received = 0;

    buffer = g_string_new(NULL);

    actual_failure_message = NULL;
    actual_error_message = NULL;
    actual_configuration = NULL;
}

void
teardown (void)
{
    if (decoder)
        g_object_unref(decoder);

    if (buffer)
        g_string_free(buffer, TRUE);

    if (expected_error)
        g_error_free(expected_error);
    if (actual_error)
        g_error_free(actual_error);

    if (actual_failure_message)
        g_free(actual_failure_message);
    if (actual_error_message)
        g_free(actual_error_message);
    if (actual_configuration)
        g_free(actual_configuration);
}

static GError *
decode (void)
{
    guint32 content_size;
    gchar content_string[sizeof(guint32)];
    GError *error = NULL;

    content_size = g_htonl(buffer->len);
    memcpy(content_string, &content_size, sizeof(content_size));
    g_string_prepend_len(buffer, content_string, sizeof(content_size));

    milter_decoder_decode(decoder, buffer->str, buffer->len, &error);
    g_string_truncate(buffer, 0);

    return error;
}

void
test_decode_success (void)
{
    g_string_append(buffer, "success");
    g_string_append_c(buffer, '\0');

    gcut_assert_error(decode());
    cut_assert_equal_int(1, n_success_received);
}

void
test_decode_failure (void)
{
    const gchar message[] = "Failure!";

    g_string_append(buffer, "failure");
    g_string_append_c(buffer, '\0');
    g_string_append(buffer, message);

    gcut_assert_error(decode());
    cut_assert_equal_int(1, n_failure_received);
    cut_assert_equal_string(message, actual_failure_message);
}

void
test_decode_error (void)
{
    const gchar message[] = "Error!";

    g_string_append(buffer, "error");
    g_string_append_c(buffer, '\0');
    g_string_append(buffer, message);

    gcut_assert_error(decode());
    cut_assert_equal_int(1, n_error_received);
    cut_assert_equal_string(message, actual_error_message);
}

void
test_decode_configuration (void)
{
    const gchar configuration[] =
        "security.privilege_mode = true\n"
        "# comment\n";

    g_string_append(buffer, "configuration");
    g_string_append_c(buffer, '\0');
    g_string_append(buffer, configuration);

    gcut_assert_error(decode());
    cut_assert_equal_int(1, n_configuration_received);
    cut_assert_equal_memory(configuration, strlen(configuration),
                            actual_configuration, actual_configuration_size);
}

void
test_decode_unknown (void)
{
    gchar unknown_command[] = "unknown";

    g_string_append(buffer, unknown_command);
    g_string_append_c(buffer, '\0');

    expected_error = g_error_new(
        MILTER_MANAGER_CONTROL_REPLY_DECODER_ERROR,
        MILTER_MANAGER_CONTROL_REPLY_DECODER_ERROR_UNEXPECTED_REPLY,
        "unexpected reply was received: <%s>", unknown_command);
    actual_error = decode();
    gcut_assert_equal_error(expected_error, actual_error);
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
