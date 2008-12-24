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
#include <locale.h>
#include <signal.h>

#include <milter/manager/milter-manager-process-launcher.h>
#include <milter/manager/milter-manager-launch-command-encoder.h>
#include <milter/manager/milter-manager-reply-encoder.h>

#include <milter-manager-test-utils.h>
#include <milter-test-utils.h>

#include <gcutter.h>

void test_launch (void);
void test_launch_no_privilege_mode (void);
void test_launch_error (gconstpointer data);
void data_launch_error (void);
void test_already_launched (void);

static MilterManagerProcessLauncher *launcher;
static MilterManagerLaunchCommandEncoder *command_encoder;
static MilterManagerReplyEncoder *reply_encoder;
static MilterManagerConfiguration *config;

static MilterWriter *writer;
static GIOChannel *output_channel;

static GString *expected_packet;
static gchar *expected_error_message;

static gchar *packet;
static gsize packet_size;

static gchar *current_locale;

void
cut_startup (void)
{
    current_locale = g_strdup(setlocale(LC_ALL, NULL));
    setlocale(LC_ALL, "C");
}

void
cut_shutdown (void)
{
    setlocale(LC_ALL, current_locale);
    if (current_locale)
        g_free(current_locale);
}

static void
setup_input_io (void)
{
    GIOChannel *channel;
    MilterReader *input_reader;

    channel = gcut_string_io_channel_new(NULL);
    g_io_channel_set_encoding(channel, NULL, NULL);
    gcut_string_io_channel_set_pipe_mode(channel, TRUE);

    input_reader = milter_reader_io_channel_new(channel);
    milter_agent_set_reader(MILTER_AGENT(launcher), input_reader);
    g_object_unref(input_reader);

    writer = milter_writer_io_channel_new(channel);

    g_io_channel_unref(channel);

    milter_agent_start(MILTER_AGENT(launcher));
}

static void
setup_output_io (void)
{
    MilterWriter *output_writer;

    output_channel = gcut_string_io_channel_new(NULL);
    g_io_channel_set_encoding(output_channel, NULL, NULL);
    gcut_string_io_channel_set_pipe_mode(output_channel, TRUE);

    output_writer = milter_writer_io_channel_new(output_channel);
    milter_agent_set_writer(MILTER_AGENT(launcher), output_writer);
    g_object_unref(output_writer);
}

static void
setup_io (void)
{
    setup_input_io();
    setup_output_io();
}

void
setup (void)
{
    MilterEncoder *encoder;

    config = milter_manager_configuration_new(NULL);
    milter_manager_configuration_set_privilege_mode(config, TRUE);

    launcher = milter_manager_process_launcher_new(config);

    setup_io();

    encoder = milter_manager_launch_command_encoder_new();
    command_encoder = MILTER_MANAGER_LAUNCH_COMMAND_ENCODER(encoder);
    encoder = milter_manager_reply_encoder_new();
    reply_encoder = MILTER_MANAGER_REPLY_ENCODER(encoder);

    expected_error_message = NULL;
}

void
teardown (void)
{
    if (launcher)
        g_object_unref(launcher);
    if (config)
        g_object_unref(config);

    if (writer)
        g_object_unref(writer);
    if (output_channel)
        g_io_channel_unref(output_channel);

    if (command_encoder)
        g_object_unref(command_encoder);
    if (reply_encoder)
        g_object_unref(reply_encoder);

    if (packet)
        g_free(packet);

    if (expected_packet)
        g_string_free(expected_packet, TRUE);
    if (expected_error_message)
        g_free(expected_error_message);
}

void
test_launch (void)
{
    GError *error = NULL;
    GString *output;
    const gchar command_line[] = "/bin/echo -n";
    const gchar *user_name;

    user_name = g_get_user_name();
    milter_manager_launch_command_encoder_encode_launch(command_encoder,
                                                        &packet, &packet_size,
                                                        command_line, user_name);
    milter_writer_write(writer, packet, packet_size, NULL, &error);
    gcut_assert_error(error);
    milter_test_pump_all_events();

    g_free(packet);
    milter_manager_reply_encoder_encode_success(reply_encoder,
                                                &packet, &packet_size);
    output = gcut_string_io_channel_get_string(output_channel);
    cut_assert_equal_memory(packet, packet_size,
                            output->str, output->len);
}

void
test_launch_no_privilege_mode (void)
{
    GString *output;
    GError *error = NULL;
    const gchar command_line[] = "/bin/echo";
    const gchar *user_name;

    milter_manager_configuration_set_privilege_mode(config, FALSE);

    user_name = g_get_user_name();
    milter_manager_launch_command_encoder_encode_launch(command_encoder,
                                                        &packet, &packet_size,
                                                        command_line, user_name);
    milter_writer_write(writer, packet, packet_size, NULL, &error);
    gcut_assert_error(error);
    milter_test_pump_all_events();

    g_free(packet);
    milter_manager_reply_encoder_encode_error(reply_encoder,
                                              &packet, &packet_size,
                                              "MilterManager is not running on privilege mode.");
    output = gcut_string_io_channel_get_string(output_channel);
    cut_assert_equal_memory(packet, packet_size,
                            output->str, output->len);
}

void
test_already_launched (void)
{
    GString *output;
    GString *expected_packet;
    GError *error = NULL;
    const gchar command_line[] = "/bin/cat";
    const gchar *user_name;

    user_name = g_get_user_name();
    milter_manager_launch_command_encoder_encode_launch(command_encoder,
                                                        &packet, &packet_size,
                                                        command_line, user_name);
    milter_writer_write(writer, packet, packet_size, NULL, &error);
    milter_writer_write(writer, packet, packet_size, NULL, &error);
    gcut_assert_error(error);
    milter_test_pump_all_events();

    g_free(packet);
    milter_manager_reply_encoder_encode_success(reply_encoder,
                                                &packet, &packet_size);
    expected_packet = g_string_new_len(packet, packet_size);
    g_free(packet);
    milter_manager_reply_encoder_encode_error(reply_encoder,
                                              &packet, &packet_size,
                                              "/bin/cat has been already launched.");
    g_string_append_len(expected_packet, packet, packet_size);
    output = gcut_string_io_channel_get_string(output_channel);
    cut_assert_equal_memory(expected_packet->str, expected_packet->len,
                            output->str, output->len);
}

void
data_launch_error (void)
{
#define ADD(label, command, user_name, expected_error_message)  \
    gcut_add_datum(label,                                       \
                   "command", G_TYPE_STRING, command,           \
                   "user-name", G_TYPE_STRING, user_name,       \
                   "expected-error-message",                    \
                   G_TYPE_STRING, expected_error_message,       \
                   NULL)


    ADD("bad command string",
        "/bin/echo \"-n",
        g_get_user_name(),
        cut_take_printf("Command string has invalid character(s): "
                        "</bin/echo \"-n>: %s:%d: %s (%s)",
                        g_quark_to_string(g_shell_error_quark()),
                        G_SHELL_ERROR_BAD_QUOTING,
                        "Text ended before matching quote was found for \".",
                        "The text was '/bin/echo \"-n'"));

    ADD("nonexistent user",
        "/bin/echo",
        "nonexistent",
        "No password entry: <nonexistent>");

    ADD("nonexistent command",
        "/bin/oecho",
        g_get_user_name(),
        cut_take_printf("Couldn't start new process: </bin/oecho>: "
                        "%s:%d: %s \"%s\" (%s)",
                        g_quark_to_string(g_spawn_error_quark()),
                        G_SPAWN_ERROR_NOENT,
                        "Failed to execute child process",
                        "/bin/oecho",
                        g_strerror(ENOENT)));
#undef ADD
}

void
test_launch_error (gconstpointer _data)
{
    GString *output;
    GError *error = NULL;
    const GCutData *data = _data;
    const gchar *command;
    const gchar *user_name;
    const gchar *expected_error_message;

    command = gcut_data_get_string(data, "command");
    user_name = gcut_data_get_string(data, "user-name");
    expected_error_message = gcut_data_get_string(data,
                                                  "expected-error-message");

    milter_manager_launch_command_encoder_encode_launch(command_encoder,
                                                        &packet, &packet_size,
                                                        command, user_name);
    milter_writer_write(writer, packet, packet_size, NULL, &error);
    gcut_assert_error(error);
    milter_test_pump_all_events();

    g_free(packet);
    milter_manager_reply_encoder_encode_error(reply_encoder,
                                              &packet, &packet_size,
                                              expected_error_message);
    output = gcut_string_io_channel_get_string(output_channel);
    cut_assert_equal_memory(packet, packet_size,
                            output->str, output->len);
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
