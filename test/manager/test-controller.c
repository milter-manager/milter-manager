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

#include <milter-manager-test-utils.h>
#include <milter-manager-test-client.h>
#include <milter-manager-test-scenario.h>
#include <milter/manager/milter-manager-controller.h>
#include <milter/manager/milter-manager-control-command-encoder.h>
#include <milter/manager/milter-manager-enum-types.h>

#include <gcutter.h>

void test_set_configuration (void);

static MilterManagerConfiguration *config;
static MilterManagerController *controller;

static GError *expected_error;
static GError *actual_error;

static MilterWriter *writer;

static MilterManagerControlCommandEncoder *encoder;

static gchar *packet;
static gsize packet_size;

static gchar *tmp_dir;
static gchar *custom_config_path;

static void
setup_io (void)
{
    GIOChannel *channel;
    MilterReader *reader;

    channel = gcut_io_channel_string_new(NULL);
    g_io_channel_set_encoding(channel, NULL, NULL);
    gcut_string_io_channel_set_pipe_mode(channel, TRUE);

    reader = milter_reader_io_channel_new(channel);
    milter_agent_set_reader(MILTER_AGENT(controller), reader);
    g_object_unref(reader);

    writer = milter_writer_io_channel_new(channel);

    g_io_channel_unref(channel);
}

void
setup (void)
{
    MilterEncoder *_encoder;

    config = milter_manager_configuration_new(NULL);
    controller = milter_manager_controller_new(config);

    setup_io();

    _encoder = milter_manager_control_command_encoder_new();
    encoder = MILTER_MANAGER_CONTROL_COMMAND_ENCODER(_encoder);

    expected_error = NULL;
    actual_error = NULL;

    packet = NULL;
    packet_size = 0;

    tmp_dir = g_build_filename(milter_manager_test_get_base_dir(),
                               "tmp",
                               NULL);
    if (g_mkdir_with_parents(tmp_dir, 0700) == -1)
        cut_assert_errno();
    milter_manager_configuration_add_load_path(config, tmp_dir);

    custom_config_path = g_build_filename(tmp_dir,
                                          CUSTOM_CONFIG_FILE_NAME,
                                          NULL);
}

void
teardown (void)
{
    if (config)
        g_object_unref(config);
    if (controller)
        g_object_unref(controller);

    if (writer)
        g_object_unref(writer);

    if (encoder)
        g_object_unref(encoder);

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
    GError *error = NULL;

    cut_assert_path_not_exist(custom_config_path);

    milter_manager_control_command_encoder_encode_set_configuration(
        encoder,
        &packet, &packet_size,
        configuration, strlen(configuration));
    milter_writer_write(writer, packet, packet_size, NULL, &error);
    gcut_assert_error(error);
    pump_all_events();

    cut_assert_path_exist(custom_config_path);

    /* FIXME: check reply */
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
