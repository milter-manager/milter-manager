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

#include <milter/manager/milter-manager-launch-command-decoder.h>

#include <gcutter.h>

void test_decode_launch (void);
void test_decode_launch_without_user_name (void);
void test_decode_unknown (void);

static MilterDecoder *decoder;
static GString *buffer;

static GError *expected_error;
static GError *actual_error;

static gint n_launch_received;

static gchar *actual_command_line;
static gchar *actual_user_name;

static void
cb_launch (MilterManagerLaunchCommandDecoder *decoder,
           const gchar *command_line,
           const gchar *user_name,
           gpointer user_data)
{
    n_launch_received++;
    actual_command_line = g_strdup(command_line);
    actual_user_name = g_strdup(user_name);
}

static void
setup_signals (MilterDecoder *decoder)
{
#define CONNECT(name)                                                   \
    g_signal_connect(decoder, #name, G_CALLBACK(cb_ ## name), NULL)

    CONNECT(launch);

#undef CONNECT
}

void
setup (void)
{
    decoder = milter_manager_launch_command_decoder_new();
    setup_signals(decoder);

    expected_error = NULL;
    actual_error = NULL;

    n_launch_received = 0;

    buffer = g_string_new(NULL);

    actual_command_line = NULL;
    actual_user_name = NULL;
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

    if (actual_command_line)
        g_free(actual_command_line);
    if (actual_user_name)
        g_free(actual_user_name);
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
test_decode_launch (void)
{
    const gchar command_line[] = "/bin/echo -n";
    const gchar user_name[] = "echo_user";

    g_string_append(buffer, "launch");
    g_string_append_c(buffer, '\0');
    g_string_append(buffer, command_line);
    g_string_append_c(buffer, '\0');
    g_string_append(buffer, user_name);

    gcut_assert_error(decode());
    cut_assert_equal_int(1, n_launch_received);
    cut_assert_equal_string(command_line, actual_command_line);
    cut_assert_equal_string(user_name, actual_user_name);
}

void
test_decode_launch_without_user_name (void)
{
    const gchar command_line[] = "/bin/echo -n";

    g_string_append(buffer, "launch");
    g_string_append_c(buffer, '\0');
    g_string_append(buffer, command_line);
    g_string_append_c(buffer, '\0');

    gcut_assert_error(decode());
    cut_assert_equal_int(1, n_launch_received);
    cut_assert_equal_string(command_line, actual_command_line);
    cut_assert_equal_string(NULL, actual_user_name);
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
