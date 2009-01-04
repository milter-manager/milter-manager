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

#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <arpa/inet.h>

#include <milter/manager/milter-manager-control-command-decoder.h>

#include <gcutter.h>

void test_decode_set_configuration (void);
void test_decode_get_configuration (void);
void test_decode_reload (void);
void test_decode_get_status (void);
void test_decode_unknown (void);

static MilterDecoder *decoder;
static GString *buffer;

static GError *expected_error;
static GError *actual_error;

static gint n_set_configuration_received;
static gint n_get_configuration_received;
static gint n_reload_received;
static gint n_get_status_received;

static gchar *actual_configuration;
static gsize actual_configuration_size;

static void
cb_set_configuration (MilterManagerControlCommandDecoder *decoder,
                      const gchar *configuration, gsize size,
                      gpointer user_data)
{
    n_set_configuration_received++;
    actual_configuration = g_memdup(configuration, size);
    actual_configuration_size = size;
}

static void
cb_get_configuration (MilterManagerControlCommandDecoder *decoder,
                      gpointer user_data)
{
    n_get_configuration_received++;
}

static void
cb_reload (MilterManagerControlCommandDecoder *decoder, gpointer user_data)
{
    n_reload_received++;
}

static void
cb_get_status (MilterManagerControlCommandDecoder *decoder, gpointer user_data)
{
    n_get_status_received++;
}

static void
setup_signals (MilterDecoder *decoder)
{
#define CONNECT(name)                                                   \
    g_signal_connect(decoder, #name, G_CALLBACK(cb_ ## name), NULL)

    CONNECT(set_configuration);
    CONNECT(get_configuration);
    CONNECT(reload);
    CONNECT(get_status);

#undef CONNECT
}

void
setup (void)
{
    decoder = milter_manager_control_command_decoder_new();
    setup_signals(decoder);

    expected_error = NULL;
    actual_error = NULL;

    n_set_configuration_received = 0;
    n_get_configuration_received = 0;
    n_reload_received = 0;
    n_get_status_received = 0;

    buffer = g_string_new(NULL);

    actual_configuration = NULL;
    actual_configuration_size = 0;
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
test_decode_set_configuration (void)
{
    const gchar configuration[] =
        "security.privilege_mode = true\n"
        "# comment\n";

    g_string_append(buffer, "set-configuration");
    g_string_append_c(buffer, '\0');
    g_string_append(buffer, configuration);

    gcut_assert_error(decode());
    cut_assert_equal_int(1, n_set_configuration_received);
    cut_assert_equal_memory(configuration, sizeof(configuration) - 1,
                            actual_configuration, actual_configuration_size);
}

void
test_decode_get_configuration (void)
{
    g_string_append(buffer, "get-configuration");
    g_string_append_c(buffer, '\0');

    gcut_assert_error(decode());
    cut_assert_equal_int(1, n_get_configuration_received);
}

void
test_decode_reload (void)
{
    g_string_append(buffer, "reload");
    g_string_append_c(buffer, '\0');

    gcut_assert_error(decode());
    cut_assert_equal_int(1, n_reload_received);
}

void
test_decode_get_status (void)
{
    g_string_append(buffer, "get-status");
    g_string_append_c(buffer, '\0');

    gcut_assert_error(decode());
    cut_assert_equal_int(1, n_get_status_received);
}

void
test_decode_unknown (void)
{
    gchar unknown_command[] = "unknown";

    g_string_append(buffer, unknown_command);
    g_string_append_c(buffer, '\0');

    expected_error = g_error_new(
        MILTER_MANAGER_CONTROL_COMMAND_DECODER_ERROR,
        MILTER_MANAGER_CONTROL_COMMAND_DECODER_ERROR_UNEXPECTED_COMMAND,
        "unexpected command was received: <%s>", unknown_command);
    actual_error = decode();
    gcut_assert_equal_error(expected_error, actual_error);
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
