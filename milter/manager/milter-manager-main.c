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

#ifdef HAVE_CONFIG_H
#  include "../../config.h"
#endif /* HAVE_CONFIG_H */

#include <locale.h>
#include <glib/gi18n.h>

#include <stdlib.h>
#include <signal.h>
#include <errno.h>

#include "../manager.h"

static gboolean initialized = FALSE;
static MilterClient *current_client = NULL;
static gchar *option_spec = NULL;

static gboolean
print_version (const gchar *option_name, const gchar *value,
               gpointer data, GError **error)
{
    g_print("%s %s\n", PACKAGE, VERSION);
    exit(EXIT_SUCCESS);
    return TRUE;
}

static gboolean
parse_spec_arg (const gchar *option_name,
                const gchar *value,
                gpointer data,
                GError **error)
{
    GError *spec_error = NULL;
    gboolean success;

    success = milter_connection_parse_spec(value, NULL, NULL, NULL, &spec_error);
    if (success) {
        option_spec = g_strdup(value);
    } else {
        g_set_error(error,
                    G_OPTION_ERROR,
                    G_OPTION_ERROR_BAD_VALUE,
                    _("%s"), spec_error->message);
        g_error_free(spec_error);
    }

    return success;
}

static const GOptionEntry option_entries[] =
{
    {"spec", 's', 0, G_OPTION_ARG_CALLBACK, parse_spec_arg,
     N_("The address of the desired communication socket."), "PROTOCOL:ADDRESS"},
    {"version", 0, G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK, print_version,
     N_("Show version"), NULL},
    {NULL}
};

void
milter_manager_init (int *argc, char ***argv)
{
    GOptionContext *option_context;
    GOptionGroup *main_group;
    GError *error = NULL;

    if (initialized)
        return;

    initialized = TRUE;

    setlocale(LC_ALL, "");
    bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
    textdomain(GETTEXT_PACKAGE);

    g_type_init();
    if (!g_thread_supported())
        g_thread_init(NULL);

    option_context = g_option_context_new(NULL);
    g_option_context_add_main_entries(option_context, option_entries, NULL);
    main_group = g_option_context_get_main_group(option_context);

    if (!g_option_context_parse(option_context, argc, argv, &error)) {
        g_print("%s\n", error->message);
        g_error_free(error);
        g_option_context_free(option_context);
        exit(EXIT_FAILURE);
    }

    _milter_manager_configuration_init();
}

void
milter_manager_quit (void)
{
    if (!initialized)
        return;

    _milter_manager_configuration_quit();
}

static void
shutdown_client (int signum)
{
    if (current_client)
        milter_client_shutdown(current_client);
}

static void
cb_error (MilterAgent *agent, GError *error, gpointer user_data)
{
    g_print("MilterManager error: %s\n", error->message);
}

static gboolean
cb_free_controller (gpointer data)
{
    MilterManagerController *controller = data;

    g_object_unref(controller);
    return FALSE;
}

static void
cb_control_reader_finished (MilterReader *reader, gpointer data)
{
    MilterManagerController *controller = data;

    g_idle_add(cb_free_controller, controller);
}

static void
process_control_connection(MilterManager *manager, GIOChannel *agent_channel)
{
    MilterManagerController *controller;
    MilterWriter *writer;
    MilterReader *reader;

    controller = milter_manager_controller_new(manager);

    writer = milter_writer_io_channel_new(agent_channel);
    milter_agent_set_writer(MILTER_AGENT(controller), writer);
    g_object_unref(writer);

    reader = milter_reader_io_channel_new(agent_channel);
    milter_agent_set_reader(MILTER_AGENT(controller), reader);
    g_object_unref(reader);

    g_signal_connect(reader, "finished",
                     G_CALLBACK(cb_control_reader_finished), controller);
}

static gboolean
accept_control_connection (gint control_fd, MilterManager *manager)
{
    gint agent_fd;
    GIOChannel *agent_channel;

    milter_info("start accepting...: %d", control_fd);
    agent_fd = accept(control_fd, NULL, NULL);
    if (agent_fd == -1) {
        milter_error("failed to accept(): %s", g_strerror(errno));
        return TRUE;
    }

    milter_info("accepted!: %d", agent_fd);
    agent_channel = g_io_channel_unix_new(agent_fd);
    g_io_channel_set_encoding(agent_channel, NULL, NULL);
    g_io_channel_set_flags(agent_channel, G_IO_FLAG_NONBLOCK, NULL);
    g_io_channel_set_close_on_unref(agent_channel, TRUE);
    process_control_connection(manager, agent_channel);
    g_io_channel_unref(agent_channel);

    return TRUE;
}

static gboolean
control_watch_func (GIOChannel *channel, GIOCondition condition, gpointer data)
{
    MilterManager *manager = data;
    gboolean keep_callback = TRUE;

    if (condition & G_IO_IN ||
        condition & G_IO_PRI) {
        guint fd;

        fd = g_io_channel_unix_get_fd(channel);
        keep_callback = accept_control_connection(fd, manager);
    }

    if (condition & G_IO_ERR ||
        condition & G_IO_HUP ||
        condition & G_IO_NVAL) {
        gchar *message;

        message = milter_utils_inspect_io_condition_error(condition);
        milter_error("%s", message);
        g_free(message);
        keep_callback = FALSE;
    }

    return keep_callback;
}


static guint
setup_control_connection (MilterManager *manager)
{
    MilterManagerConfiguration *config;
    const gchar *spec;
    GIOChannel *channel;
    GError *error = NULL;
    guint watch_id = 0;

    config = milter_manager_get_configuration(manager);
    spec = milter_manager_configuration_get_control_connection_spec(config);
    if (!spec) {
        milter_info("control connection spec is missing. "
                    "control connection is disabled");
        return 0;
    }

    channel = milter_connection_listen(spec, 5, &error);
    if (!channel) {
        milter_error("failed to listen control connection: <%s>:<%s>",
                     spec, error->message);
        g_error_free(error);
        return 0;
    }

    watch_id = g_io_add_watch(channel,
                              G_IO_IN | G_IO_PRI |
                              G_IO_ERR | G_IO_HUP | G_IO_NVAL,
                              control_watch_func, manager);

    return watch_id;
}


void
milter_manager_main (void)
{
    MilterClient *client;
    MilterManager *manager;
    void (*sigint_handler) (int signum);
    guint control_connection_watch_id = 0;
    GError *error = NULL;

    manager = milter_manager_new();
    client = MILTER_CLIENT(manager);

    /* FIXME */
    control_connection_watch_id = setup_control_connection(manager);

    g_signal_connect(client, "error",
                     G_CALLBACK(cb_error), NULL);

    if (!milter_client_set_connection_spec(client, option_spec, &error)) {
        g_object_unref(client);
        g_print("%s\n", error->message);
        return;
    }

    current_client = client;
    sigint_handler = signal(SIGINT, shutdown_client);
    if (!milter_client_main(client))
        g_print("Failed to start milter-manager process.\n");
    signal(SIGINT, sigint_handler);

    current_client = NULL;
    g_object_unref(client);
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
