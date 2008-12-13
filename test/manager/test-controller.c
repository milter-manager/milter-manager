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

#include <string.h>

#include <milter-test-utils.h>
#include <milter-manager-test-utils.h>
#include <milter-manager-test-client.h>
#include <milter-manager-test-scenario.h>
#include <milter/manager/milter-manager-controller.h>
#include <milter/manager/milter-manager-control-command-encoder.h>
#include <milter/manager/milter-manager-control-reply-encoder.h>
#include <milter/manager/milter-manager-enum-types.h>

#include <gcutter.h>

void test_set_configuration (void);
void test_reload (void);

static MilterManager *manager;
static MilterManagerController *controller;

static GError *expected_error;
static GError *actual_error;

static MilterWriter *writer;
static GIOChannel *output_channel;

static MilterManagerControlCommandEncoder *command_encoder;
static MilterManagerControlReplyEncoder *reply_encoder;

static gchar *packet;
static gsize packet_size;

static gchar *tmp_dir;
static gchar *custom_config_path;

static void
setup_input_io (void)
{
    GIOChannel *channel;
    MilterReader *input_reader;

    channel = gcut_string_io_channel_new(NULL);
    g_io_channel_set_encoding(channel, NULL, NULL);
    gcut_string_io_channel_set_pipe_mode(channel, TRUE);

    input_reader = milter_reader_io_channel_new(channel);
    milter_agent_set_reader(MILTER_AGENT(controller), input_reader);
    g_object_unref(input_reader);

    writer = milter_writer_io_channel_new(channel);

    g_io_channel_unref(channel);
}

static void
setup_output_io (void)
{
    MilterWriter *output_writer;

    output_channel = gcut_string_io_channel_new(NULL);
    g_io_channel_set_encoding(output_channel, NULL, NULL);
    gcut_string_io_channel_set_pipe_mode(output_channel, TRUE);

    output_writer = milter_writer_io_channel_new(output_channel);
    milter_agent_set_writer(MILTER_AGENT(controller), output_writer);
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
    MilterManagerConfiguration *config;
    MilterEncoder *encoder;

    config = milter_manager_configuration_new(NULL);
    manager = milter_manager_new(config);
    g_object_unref(config);
    controller = milter_manager_controller_new(manager);

    setup_io();

    encoder = milter_manager_control_command_encoder_new();
    command_encoder = MILTER_MANAGER_CONTROL_COMMAND_ENCODER(encoder);
    encoder = milter_manager_control_reply_encoder_new();
    reply_encoder = MILTER_MANAGER_CONTROL_REPLY_ENCODER(encoder);

    expected_error = NULL;
    actual_error = NULL;

    packet = NULL;
    packet_size = 0;

    tmp_dir = g_build_filename(milter_test_get_base_dir(),
                               "tmp",
                               NULL);
    if (g_mkdir_with_parents(tmp_dir, 0700) == -1)
        cut_assert_errno();
    milter_manager_configuration_prepend_load_path(config, tmp_dir);

    custom_config_path = g_build_filename(tmp_dir,
                                          CUSTOM_CONFIG_FILE_NAME,
                                          NULL);
}

void
teardown (void)
{
    if (manager)
        g_object_unref(manager);
    if (controller)
        g_object_unref(controller);

    if (writer)
        g_object_unref(writer);
    if (output_channel)
        g_io_channel_unref(output_channel);

    if (command_encoder)
        g_object_unref(command_encoder);
    if (reply_encoder)
        g_object_unref(reply_encoder);

    if (actual_error)
        g_error_free(actual_error);
    if (expected_error)
        g_error_free(expected_error);

    if (packet)
        g_free(packet);

    if (tmp_dir) {
        cut_remove_path(tmp_dir, NULL);
        g_free(tmp_dir);
    }

    if (custom_config_path)
        g_free(custom_config_path);
}

static gboolean
cb_idle_check (gpointer data)
{
    gboolean *idle_emitted = data;

    *idle_emitted = TRUE;
    return FALSE;
}

static void
pump_all_events (void)
{
    gboolean idle_emitted = FALSE;

    g_idle_add(cb_idle_check, &idle_emitted);
    while (!idle_emitted) {
        g_main_context_iteration(NULL, TRUE);
    }
}

void
test_set_configuration (void)
{
    const gchar configuration[] = "XXX";
    GString *output;
    GError *error = NULL;

    return;
    cut_assert_path_not_exist(custom_config_path);

    milter_manager_control_command_encoder_encode_set_configuration(
        command_encoder,
        &packet, &packet_size,
        configuration, strlen(configuration));
    milter_writer_write(writer, packet, packet_size, NULL, &error);
    gcut_assert_error(error);
    pump_all_events();

    cut_assert_path_exist(custom_config_path);

    g_free(packet);
    milter_manager_control_reply_encoder_encode_success(reply_encoder,
                                                        &packet, &packet_size);
    output = gcut_string_io_channel_get_string(output_channel);
    cut_assert_equal_memory(packet, packet_size,
                            output->str, output->len);
}

void
test_reload (void)
{
    MilterManagerConfiguration *config;
    GString *output;
    GError *error = NULL;

    return;
    config = milter_manager_get_configuration(manager);
    cut_assert_false(milter_manager_configuration_is_privilege_mode(config));
    g_file_set_contents(custom_config_path,
                        "security.privilege_mode = true", -1,
                        &error);
    gcut_assert_error(error);

    milter_manager_control_command_encoder_encode_reload(command_encoder,
                                                         &packet, &packet_size);
    milter_writer_write(writer, packet, packet_size, NULL, &error);
    gcut_assert_error(error);
    pump_all_events();
    cut_assert_true(milter_manager_configuration_is_privilege_mode(config));

    g_free(packet);
    milter_manager_control_reply_encoder_encode_success(reply_encoder,
                                                        &packet, &packet_size);
    output = gcut_string_io_channel_get_string(output_channel);
    cut_assert_equal_memory(packet, packet_size,
                            output->str, output->len);
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
