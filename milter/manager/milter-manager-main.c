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

#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/un.h>
#include <pwd.h>
#include <unistd.h>

#include <locale.h>
#include <glib/gi18n.h>

#include "../manager.h"
#include "milter-manager-process-launcher.h"

static gboolean initialized = FALSE;
static MilterManager *the_manager = NULL;
static gchar *option_spec = NULL;
static gchar *config_dir = NULL;
static pid_t launcher_pid = -1;

static void (*default_sigint_handler) (int signum);
static void (*default_sigterm_handler) (int signum);
static void (*default_sighup_handler) (int signum);


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
    {"config-dir", 0, 0, G_OPTION_ARG_STRING, &config_dir,
     N_("The configuration directory that has configuration file."),
     "DIRECTORY"},
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

    milter_init();

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
    if (the_manager)
        milter_client_shutdown(MILTER_CLIENT(the_manager));

    switch (signum) {
    case SIGINT:
        signal(SIGINT, default_sigint_handler);
        break;
    case SIGTERM:
        signal(SIGTERM, default_sigterm_handler);
        break;
    default:
        signal(signum, SIG_DFL);
        break;
    }
}

static void
reload_configuration (int signum)
{
    MilterManagerConfiguration *config;

    if (!the_manager)
        return;

    config = milter_manager_get_configuration(the_manager);
    if (!config)
        return;

    milter_manager_configuration_reload(config);
}

static void
cb_error (MilterErrorEmittable *emittable, GError *error, gpointer user_data)
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
cb_controller_reader_finished (MilterReader *reader, gpointer data)
{
    MilterManagerController *controller = data;

    g_idle_add(cb_free_controller, controller);
}

static void
process_controller_connection(MilterManager *manager, GIOChannel *agent_channel)
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
                     G_CALLBACK(cb_controller_reader_finished), controller);
}

static gboolean
accept_controller_connection (gint controller_fd, MilterManager *manager)
{
    gint agent_fd;
    GIOChannel *agent_channel;

    milter_debug("start accepting...: %d", controller_fd);
    agent_fd = accept(controller_fd, NULL, NULL);
    if (agent_fd == -1) {
        milter_error("failed to accept(): %s", g_strerror(errno));
        return TRUE;
    }

    milter_debug("accepted!: %d", agent_fd);
    agent_channel = g_io_channel_unix_new(agent_fd);
    g_io_channel_set_encoding(agent_channel, NULL, NULL);
    g_io_channel_set_flags(agent_channel, G_IO_FLAG_NONBLOCK, NULL);
    g_io_channel_set_close_on_unref(agent_channel, TRUE);
    process_controller_connection(manager, agent_channel);
    g_io_channel_unref(agent_channel);

    return TRUE;
}

static gboolean
controller_watch_func (GIOChannel *channel, GIOCondition condition,
                       gpointer data)
{
    MilterManager *manager = data;
    gboolean keep_callback = TRUE;

    if (condition & G_IO_IN ||
        condition & G_IO_PRI) {
        guint fd;

        fd = g_io_channel_unix_get_fd(channel);
        keep_callback = accept_controller_connection(fd, manager);
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
setup_controller_connection (MilterManager *manager)
{
    MilterManagerConfiguration *config;
    const gchar *spec;
    GIOChannel *channel;
    GError *error = NULL;
    guint watch_id = 0;

    config = milter_manager_get_configuration(manager);
    spec = milter_manager_configuration_get_controller_connection_spec(config);
    if (!spec) {
        milter_debug("controller connection spec is missing. "
                     "controller connection is disabled");
        return 0;
    }

    channel = milter_connection_listen(spec, -1, NULL, NULL, &error);
    if (!channel) {
        milter_error("failed to listen controller connection: <%s>: <%s>",
                     spec, error->message);
        g_error_free(error);
        return 0;
    }

    watch_id = g_io_add_watch(channel,
                              G_IO_IN | G_IO_PRI |
                              G_IO_ERR | G_IO_HUP | G_IO_NVAL,
                              controller_watch_func, manager);

    return watch_id;
}

typedef enum {
    READ_PIPE,
    WRITE_PIPE
} PipeMode;

static void
close_pipe (int *pipe, PipeMode mode)
{
    if (pipe[mode] == -1)
        return;
    close(pipe[mode]);
    pipe[mode] = -1;
}

static GIOChannel *
create_io_channel (int pipe, GIOFlags flag)
{
    GIOChannel *channel;

    channel = g_io_channel_unix_new(pipe);
    g_io_channel_set_encoding(channel, NULL, NULL);
    g_io_channel_set_flags(channel, flag, NULL);
    g_io_channel_set_close_on_unref(channel, TRUE);

    return channel;
}

static GIOChannel *
create_read_io_channel (int pipe)
{
    return create_io_channel(pipe, G_IO_FLAG_IS_READABLE | G_IO_FLAG_NONBLOCK);
}

static GIOChannel *
create_write_io_channel (int pipe)
{
    return create_io_channel(pipe, G_IO_FLAG_IS_WRITEABLE);
}

static pid_t
prepare_pipes (MilterManager *manager,
               GIOChannel **read_channel,
               GIOChannel **write_channel)
{
    int fork_errno = 0;
    int launcher_command_pipe[2];
    int launcher_reply_pipe[2];
    pid_t pid;

    if (pipe(launcher_command_pipe) < 0 ||
        pipe(launcher_reply_pipe) < 0) {
        return -1;
    }

    pid = fork();
    if (pid == -1)
        fork_errno = errno;

    if (pid == 0) {
        close_pipe(launcher_command_pipe, WRITE_PIPE);
        close_pipe(launcher_reply_pipe, READ_PIPE);

        *write_channel = create_write_io_channel(launcher_reply_pipe[WRITE_PIPE]);
        *read_channel = create_read_io_channel(launcher_command_pipe[READ_PIPE]);
    } else {
        close_pipe(launcher_command_pipe, READ_PIPE);
        close_pipe(launcher_reply_pipe, WRITE_PIPE);

        *write_channel = create_write_io_channel(launcher_command_pipe[WRITE_PIPE]);
        *read_channel = create_read_io_channel(launcher_reply_pipe[READ_PIPE]);
    }

    errno = fork_errno;
    return pid;
}

static void
start_process_launcher (MilterManager *manager,
                        GIOChannel *read_channel, GIOChannel *write_channel)
{
    MilterManagerProcessLauncher *launcher;
    MilterReader *reader;
    MilterWriter *writer;
    MilterManagerConfiguration *configuration;

    reader = milter_reader_io_channel_new(read_channel);
    g_io_channel_unref(read_channel);

    writer = milter_writer_io_channel_new(write_channel);

    configuration = milter_manager_get_configuration(manager);
    launcher = milter_manager_process_launcher_new(configuration);
    milter_agent_set_reader(MILTER_AGENT(launcher), reader);
    milter_agent_set_writer(MILTER_AGENT(launcher), writer);
    g_object_unref(reader);
    g_object_unref(writer);
}

static pid_t
fork_launcher_process (MilterManager *manager)
{
    pid_t pid;
    GIOChannel *read_channel = NULL, *write_channel = NULL;

    pid = prepare_pipes(manager, &read_channel, &write_channel);
    if (pid == -1)
        return -1;

    if (pid == 0) {
        start_process_launcher(manager, read_channel, write_channel);
    } else {
        milter_manager_set_launcher_channel(manager,
                                            read_channel,
                                            write_channel);
    }

    return pid;
}

static gboolean
switch_user (MilterManager *manager)
{
    MilterManagerConfiguration *configuration;
    const gchar *effective_user;
    struct passwd *password;

    configuration = milter_manager_get_configuration(manager);

    effective_user = milter_manager_configuration_get_effective_user(configuration);
    if (!effective_user)
        effective_user = "nobody";

    password = getpwnam(effective_user);
    if (!password)
        return FALSE;

    if (setuid(password->pw_uid) == -1)
        return FALSE;

    return TRUE;
}

void
milter_manager_main (void)
{
    MilterClient *client;
    MilterManager *manager;
    MilterManagerConfiguration *config;
    guint controller_connection_watch_id = 0;
    GError *error = NULL;
    pid_t pid = -1;

    config = milter_manager_configuration_new(NULL);
    if (config_dir)
        milter_manager_configuration_prepend_load_path(config, config_dir);
    milter_manager_configuration_reload(config);
    manager = milter_manager_new(config);
    g_object_unref(config);

    client = MILTER_CLIENT(manager);

    /* FIXME */
    controller_connection_watch_id = setup_controller_connection(manager);

    g_signal_connect(client, "error",
                     G_CALLBACK(cb_error), NULL);


    if (!milter_client_set_connection_spec(client, option_spec, &error)) {
        g_object_unref(manager);
        g_print("%s\n", error->message);
        return;
    }

    if (milter_manager_configuration_is_privilege_mode(config)) {
        pid = fork_launcher_process(manager);
        if (pid == -1) {
            g_object_unref(manager);
            g_print("%s\n", g_strerror(errno));
            return;
        }
        if (pid == 0) {
            while (TRUE) {
                g_main_context_iteration(NULL, FALSE);
            }
        } else {
            launcher_pid = pid;
        }
    }

    if (geteuid() == 0 && !switch_user(manager)) {
        g_object_unref(manager);
        g_print("Could not change effective user\n");
        if (launcher_pid > 0)
            kill(launcher_pid, SIGKILL);
        return;
    }

    the_manager = manager;
    default_sigint_handler = signal(SIGINT, shutdown_client);
    default_sigterm_handler = signal(SIGTERM, shutdown_client);
    default_sighup_handler = signal(SIGHUP, reload_configuration);
    if (!milter_client_main(client))
        g_print("Failed to start milter-manager process.\n");
    signal(SIGHUP, default_sighup_handler);
    signal(SIGTERM, default_sigterm_handler);
    signal(SIGINT, default_sigint_handler);

    if (the_manager) {
        g_object_unref(the_manager);
        the_manager = NULL;
    }

    if (launcher_pid > 0) {
        kill(launcher_pid, SIGKILL);
        launcher_pid = -1;
    }
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
