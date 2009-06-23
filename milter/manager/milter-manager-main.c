/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Copyright (C) 2008-2009  Kouhei Sutou <kou@clear-code.com>
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

#ifdef HAVE_CONFIG_H
#  include "../../config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/un.h>
#include <pwd.h>
#include <grp.h>
#include <unistd.h>
#include <fcntl.h>

#include <locale.h>
#include <glib/gi18n.h>

#include <glib/gstdio.h>

#include "../manager.h"

static gboolean initialized = FALSE;
static MilterManager *the_manager = NULL;
static gchar *option_spec = NULL;
static gchar *option_config_dir = NULL;
static gchar *option_pid_file = NULL;
static gchar *option_user_name = NULL;
static gchar *option_group_name = NULL;
static gchar *option_socket_group_name = NULL;
static gboolean option_daemon = FALSE;
static gboolean option_show_config = FALSE;
static gboolean option_verbose = FALSE;

static gboolean io_detached = FALSE;

static guint milter_manager_log_handler_id = 0;

static struct sigaction default_sigsegv_action;
static struct sigaction default_sigabort_action;
static struct sigaction default_sigint_action;
static struct sigaction default_sigterm_action;
static struct sigaction default_sighup_action;

static gboolean set_sigsegv_action = TRUE;
static gboolean set_sigabort_action = TRUE;
static gboolean set_sigint_action = TRUE;
static gboolean set_sigterm_action = TRUE;
static gboolean set_sighup_action = TRUE;

#define milter_manager_error(...) G_STMT_START  \
{                                               \
    if (io_detached) {                          \
        milter_error(__VA_ARGS__);              \
    } else {                                    \
        g_print(__VA_ARGS__);                   \
        g_print("\n");                          \
    }                                           \
} G_STMT_END

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
        if (option_spec)
            g_free(option_spec);
        option_spec = g_strdup(value);
    } else {
        g_set_error(error,
                    G_OPTION_ERROR,
                    G_OPTION_ERROR_BAD_VALUE,
                    "invalid connection spec: %s",
                    spec_error->message);
        g_error_free(spec_error);
    }

    return success;
}

static const GOptionEntry option_entries[] =
{
    {"connection-spec", 's', 0, G_OPTION_ARG_CALLBACK, parse_spec_arg,
     N_("The spec of socket. (unix:PATH|inet:PORT[@HOST]|inet6:PORT[@HOST])"),
     "SPEC"},
    {"config-dir", 'c',
     G_OPTION_FLAG_FILENAME, G_OPTION_ARG_STRING, &option_config_dir,
     N_("The configuration directory that has configuration file."),
     "DIRECTORY"},
    {"pid-file", 0,
     G_OPTION_FLAG_FILENAME, G_OPTION_ARG_STRING, &option_pid_file,
     N_("The file name to be saved PID."),
     "FILE"},
    {"user-name", 'u', 0, G_OPTION_ARG_STRING, &option_user_name,
     N_("The user name for running milter-manager."),
     "NAME"},
    {"group-name", 'g', 0, G_OPTION_ARG_STRING, &option_group_name,
     N_("The group name for running milter-manager."),
     "NAME"},
    {"socket-group-name", 0, 0, G_OPTION_ARG_STRING, &option_socket_group_name,
     N_("The group name for UNIX domain socket."),
     "NAME"},
    {"daemon", 0, 0, G_OPTION_ARG_NONE, &option_daemon,
     N_("Run as daemon process."), NULL},
    {"no-daemon", 0, G_OPTION_FLAG_REVERSE, G_OPTION_ARG_NONE, &option_daemon,
     N_("Cancel the prior --daemon options."), NULL},
    {"show-config", 0, 0, G_OPTION_ARG_NONE, &option_show_config,
     N_("Show configuration and exit"), NULL},
    {"verbose", 0, 0, G_OPTION_ARG_NONE, &option_verbose,
     N_("Be verbose"), NULL},
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
    milter_client_init();
    milter_server_init();
    milter_manager_log_handler_id = MILTER_GLIB_LOG_DELEGATE("milter-manager");

    option_context = g_option_context_new(NULL);
    g_option_context_add_main_entries(option_context, option_entries, NULL);
    main_group = g_option_context_get_main_group(option_context);

    if (!g_option_context_parse(option_context, argc, argv, &error)) {
        g_print("%s\n", error->message);

#if GLIB_CHECK_VERSION(2, 14, 0)
        if (g_error_matches(error,
                            G_OPTION_ERROR, G_OPTION_ERROR_UNKNOWN_OPTION)) {
            gchar *help;
            help = g_option_context_get_help(option_context, TRUE, NULL);
            g_print("\n%s", help);
            g_free(help);
        }
#endif

        g_error_free(error);

        g_option_context_free(option_context);

        exit(EXIT_FAILURE);
    }

    if (option_verbose)
        g_setenv("MILTER_LOG_LEVEL", "all", TRUE);

    _milter_manager_configuration_init();
    g_option_context_free(option_context);
}

void
milter_manager_quit (void)
{
    if (!initialized)
        return;

    _milter_manager_configuration_quit();

    if (option_spec) {
        g_free(option_spec);
        option_spec = NULL;
    }

    g_log_remove_handler("milter-manager", milter_manager_log_handler_id);
    milter_server_quit();
    milter_client_quit();
    milter_quit();
}

static void
shutdown_client (int signum)
{
    struct sigaction default_action;

    if (the_manager)
        milter_client_shutdown(MILTER_CLIENT(the_manager));

    switch (signum) {
    case SIGINT:
        if (sigaction(SIGINT, &default_sigint_action, NULL) != -1)
            set_sigint_action = FALSE;
        break;
    case SIGTERM:
        if (sigaction(SIGTERM, &default_sigterm_action, NULL) != -1)
            set_sigterm_action = FALSE;
        break;
    default:
        default_action.sa_handler = SIG_DFL;
        sigemptyset(&default_action.sa_mask);
        default_action.sa_flags = 0;
        sigaction(signum, &default_action, NULL);
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
    g_print("milter-manager error: %s\n", error->message);
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

static gboolean
create_process_launcher_pipes (gint **command_pipe, gint **reply_pipe)
{
    if (pipe(*command_pipe) == -1) {
        milter_manager_error("failed to create pipe for launcher command: %s",
                             g_strerror(errno));
        return FALSE;
    }

    if (pipe(*reply_pipe) == -1) {
        milter_manager_error("failed to create pipe for launcher reply: %s",
                             g_strerror(errno));
        close((*command_pipe)[WRITE_PIPE]);
        close((*command_pipe)[READ_PIPE]);
        return FALSE;
    }

    return TRUE;
}

static void
prepare_process_launcher_pipes_for_process_launcher (gint *command_pipe,
                                                     gint *reply_pipe,
                                                     GIOChannel **read_channel,
                                                     GIOChannel **write_channel)
{
    close_pipe(command_pipe, WRITE_PIPE);
    close_pipe(reply_pipe, READ_PIPE);

    *write_channel = create_write_io_channel(reply_pipe[WRITE_PIPE]);
    *read_channel = create_read_io_channel(command_pipe[READ_PIPE]);
}

static void
prepare_process_launcher_pipes_for_manager (gint *command_pipe,
                                            gint *reply_pipe,
                                            GIOChannel **read_channel,
                                            GIOChannel **write_channel)
{
    close_pipe(command_pipe, READ_PIPE);
    close_pipe(reply_pipe, WRITE_PIPE);

    *write_channel = create_write_io_channel(command_pipe[WRITE_PIPE]);
    *read_channel = create_read_io_channel(reply_pipe[READ_PIPE]);
}

static void
start_process_launcher (GIOChannel *read_channel, GIOChannel *write_channel)
{
    MilterManagerProcessLauncher *launcher;
    MilterReader *reader;
    MilterWriter *writer;

    reader = milter_reader_io_channel_new(read_channel);
    g_io_channel_unref(read_channel);

    writer = milter_writer_io_channel_new(write_channel);

    launcher = milter_manager_process_launcher_new();
    milter_agent_set_reader(MILTER_AGENT(launcher), reader);
    milter_agent_set_writer(MILTER_AGENT(launcher), writer);
    g_object_unref(reader);
    g_object_unref(writer);

    milter_agent_start(MILTER_AGENT(launcher));

    milter_manager_process_launcher_run(launcher);

    g_object_unref(launcher);
}

static gboolean
start_process_launcher_process (MilterManager *manager)
{
    gint command_pipe[2];
    gint reply_pipe[2];
    gint *command_pipe_p;
    gint *reply_pipe_p;
    GIOChannel *read_channel = NULL;
    GIOChannel *write_channel = NULL;

    command_pipe_p = command_pipe;
    reply_pipe_p = reply_pipe;
    if (!create_process_launcher_pipes(&command_pipe_p, &reply_pipe_p))
        return FALSE;

    switch (fork()) {
    case 0:
        /* FIXME: change process name to
           'milter-manager: process-launcher' by
           setproctitle() on *BSD, memcpy() argv on Linux or
           spawning another process. */
        g_object_unref(manager);
        prepare_process_launcher_pipes_for_process_launcher(command_pipe_p,
                                                            reply_pipe_p,
                                                            &read_channel,
                                                            &write_channel);
        start_process_launcher(read_channel, write_channel);
        _exit(EXIT_FAILURE);
        break;
    case -1:
        milter_manager_error("failed to fork process launcher process: %s",
                             g_strerror(errno));
        return FALSE;
    default:
        prepare_process_launcher_pipes_for_manager(command_pipe_p,
                                                   reply_pipe_p,
                                                   &read_channel,
                                                   &write_channel);
        milter_manager_set_launcher_channel(manager,
                                            read_channel,
                                            write_channel);
        break;
    }

    return TRUE;
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

    errno = 0;
    password = getpwnam(effective_user);
    if (!password) {
        if (errno == 0) {
            milter_manager_error(
                "failed to find password entry for effective user: %s",
                effective_user);
        } else {
            milter_manager_error(
                "failed to get password entry for effective user: %s: %s",
                effective_user, g_strerror(errno));
        }
        return FALSE;
    }


    if (setuid(password->pw_uid) == -1) {
        milter_manager_error("failed to change effective user: %s: %s",
                             effective_user, g_strerror(errno));
        return FALSE;
    }

    return TRUE;
}

static gboolean
switch_group (MilterManager *manager)
{
    MilterManagerConfiguration *configuration;
    const gchar *effective_group;
    struct group *group;

    configuration = milter_manager_get_configuration(manager);

    effective_group = milter_manager_configuration_get_effective_group(configuration);
    if (!effective_group)
        return TRUE;

    errno = 0;
    group = getgrnam(effective_group);
    if (!group) {
        if (errno == 0) {
            milter_manager_error(
                "failed to find group entry for effective group: %s",
                effective_group);
        } else {
            milter_manager_error(
                "failed to get group entry for effective group: %s: %s",
                effective_group, g_strerror(errno));
        }
        return FALSE;
    }


    if (setgid(group->gr_gid) == -1) {
        milter_manager_error("failed to change effective group: %s: %s",
                             effective_group, g_strerror(errno));
        return FALSE;
    }

    return TRUE;
}

static gboolean
detach_io (void)
{
    gboolean success = FALSE;
    int null_stdin_fd = -1;
    int null_stdout_fd = -1;
    int null_stderr_fd = -1;

    null_stdin_fd = open("/dev/null", O_RDONLY);
    if (null_stdin_fd == -1) {
        g_print("failed to open /dev/null for STDIN: %s\n", g_strerror(errno));
        goto cleanup;
    }

    null_stdout_fd = open("/dev/null", O_WRONLY);
    if (null_stdout_fd == -1) {
        g_print("failed to open /dev/null for STDOUT: %s\n", g_strerror(errno));
        goto cleanup;
    }

    null_stderr_fd = open("/dev/null", O_WRONLY);
    if (null_stderr_fd == -1) {
        g_print("failed to open /dev/null for STDERR: %s\n", g_strerror(errno));
        goto cleanup;
    }

    if (dup2(null_stdin_fd, STDIN_FILENO) == -1) {
        g_print("failed to detach STDIN: %s\n", g_strerror(errno));
        goto cleanup;
    }

    if (dup2(null_stdout_fd, STDOUT_FILENO) == -1) {
        g_print("failed to detach STDOUT: %s\n", g_strerror(errno));
        goto cleanup;
    }

    if (dup2(null_stderr_fd, STDERR_FILENO) == -1) {
        g_printerr("failed to detach STDERR: %s\n", g_strerror(errno));
        goto cleanup;
    }

    success = TRUE;

cleanup:
    if (null_stdin_fd == -1)
        close(null_stdin_fd);
    if (null_stdout_fd == -1)
        close(null_stdout_fd);
    if (null_stderr_fd == -1)
        close(null_stderr_fd);

    if (success)
        io_detached = TRUE;

    return success;
}

static gboolean
daemonize (void)
{
    switch (fork()) {
    case 0:
        break;
    case -1:
        g_print("failed to fork child process: %s\n", g_strerror(errno));
        return FALSE;
    default:
        _exit(EXIT_SUCCESS);
        break;
    }

    if (setsid() == -1) {
        g_print("failed to create session: %s\n", g_strerror(errno));
        return FALSE;
    }

    switch (fork()) {
    case 0:
        break;
    case -1:
        g_print("failed to fork grandchild process: %s\n", g_strerror(errno));
        return FALSE;
    default:
        _exit(EXIT_SUCCESS);
        break;
    }

    if (g_chdir("/") == -1) {
        g_print("failed to change working directory to '/': %s\n",
                g_strerror(errno));
        return FALSE;
    }

    if (!detach_io())
        return FALSE;

    return TRUE;
}

static void
apply_command_line_options (MilterManagerConfiguration *config)
{
    if (option_spec)
        milter_manager_configuration_set_manager_connection_spec(config,
                                                                 option_spec);
    if (option_pid_file)
        milter_manager_configuration_set_pid_file(config, option_pid_file);
    if (option_user_name)
        milter_manager_configuration_set_effective_user(config,
                                                        option_user_name);
    if (option_group_name)
        milter_manager_configuration_set_effective_group(config,
                                                         option_group_name);
    if (option_socket_group_name)
        milter_manager_configuration_set_manager_unix_socket_group(
            config, option_socket_group_name);
    if (option_daemon)
        milter_manager_configuration_set_daemon(config, TRUE);
    if (getuid() != 0)
        milter_manager_configuration_set_privilege_mode(config, FALSE);
}

static void
append_custom_configuration_directory (MilterManagerConfiguration *config)
{
    uid_t uid;
    struct passwd *password;
    gchar *custom_config_directory;

    uid = geteuid();
    if (uid == 0)
        return;

    password = getpwuid(uid);

    custom_config_directory =
        g_strdup(milter_manager_configuration_get_custom_configuration_directory(config));
    if (!custom_config_directory)
        custom_config_directory = g_build_filename(password->pw_dir,
                                                   ".milter-manager",
                                                   NULL);
    if (!g_file_test(custom_config_directory, G_FILE_TEST_EXISTS)) {
        if (g_mkdir(custom_config_directory, 0700) == -1) {
            milter_manager_error("failed to create custom "
                                 "configuration directory: %s: %s",
                                 custom_config_directory,
                                 g_strerror(errno));
            return;
        }
    }

    milter_manager_configuration_append_load_path(config,
                                                  custom_config_directory);
    g_free(custom_config_directory);

    milter_manager_configuration_reload(config);
    apply_command_line_options(config);
}

static gchar report_stack_trace_window[4096];
static void
report_stack_trace (int signum)
{
    int fds[2];
    int in_fd;
    const char *label = "unknown";
    int original_stdout_fileno;
    gssize i, last_new_line, size;

    switch (signum) {
    case SIGSEGV:
        label = "SEGV";
        if (sigaction(SIGSEGV, &default_sigsegv_action, NULL) != -1)
            set_sigsegv_action = FALSE;
        break;
    case SIGABRT:
        label = "ABORT";
        if (sigaction(SIGABRT, &default_sigabort_action, NULL) != -1)
            set_sigabort_action = FALSE;
        break;
    }

    if (pipe(fds) == -1) {
        milter_critical("[%s] unable to open pipe for collecting stack trace",
                        label);
        return;
    }

    original_stdout_fileno = dup(STDOUT_FILENO);
    dup2(fds[1], STDOUT_FILENO);
    g_on_error_stack_trace(g_get_prgname());
    dup2(original_stdout_fileno, STDOUT_FILENO);
    close(original_stdout_fileno);
    close(fds[1]);

    in_fd = fds[0];
    while ((size = read(in_fd,
                        report_stack_trace_window,
                        sizeof(report_stack_trace_window))) > 0) {
        last_new_line = 0;
        for (i = 0; i < size; i++) {
            if (report_stack_trace_window[i] == '\n') {
                report_stack_trace_window[i] = '\0';
                milter_critical("%s", report_stack_trace_window + last_new_line);
                last_new_line = i + 1;
            }
        }
        if (last_new_line < size) {
            report_stack_trace_window[size] = '\0';
            milter_critical("%s", report_stack_trace_window + last_new_line);
        }
    }

    close(fds[0]);

    shutdown_client(SIGINT);
}

gboolean
milter_manager_main (void)
{
    MilterClient *client;
    MilterManager *manager;
    MilterManagerController *controller;
    MilterManagerConfiguration *config;
    gboolean daemon;
    gchar *pid_file = NULL;
    struct sigaction report_stack_trace_action;
    struct sigaction shutdown_client_action;
    struct sigaction reload_configuration_action;

    config = milter_manager_configuration_new(NULL);
    if (option_config_dir)
        milter_manager_configuration_prepend_load_path(config,
                                                       option_config_dir);
    milter_manager_configuration_reload(config);
    apply_command_line_options(config);

    manager = milter_manager_new(config);
    g_object_unref(config);

    client = MILTER_CLIENT(manager);
    g_signal_connect(client, "error", G_CALLBACK(cb_error), NULL);

    daemon = milter_manager_configuration_is_daemon(config);
    if (!option_show_config && daemon && !daemonize()) {
        g_object_unref(manager);
        return FALSE;
    }

    if (!option_show_config &&
        milter_manager_configuration_is_privilege_mode(config) &&
        !start_process_launcher_process(manager)) {
        g_object_unref(manager);
        return FALSE;
    }

    if (geteuid() == 0 &&
        (!switch_group(manager) || !switch_user(manager))) {
        g_object_unref(manager);
        return FALSE;
    }

    append_custom_configuration_directory(config);

    if (option_show_config) {
        gchar *dumped_config;

        dumped_config = milter_manager_configuration_dump(config);
        if (dumped_config) {
            g_print("%s", dumped_config);
            if (!g_str_has_suffix(dumped_config, "\n"))
                g_print("\n");
            g_free(dumped_config);
        }
        g_object_unref(manager);
        return TRUE;
    }

    controller = milter_manager_controller_new(manager);
    if (controller)
        milter_manager_controller_listen(controller, NULL);

    pid_file = g_strdup(milter_manager_configuration_get_pid_file(config));
    if (pid_file) {
        gchar *content;
        GError *error = NULL;

        content = g_strdup_printf("%u\n", getpid());
        if (!g_file_set_contents(pid_file, content, -1, &error)) {
            milter_error("[manager][error][pid][save] %s: %s",
                         pid_file, error->message);
            g_error_free(error);
            g_free(pid_file);
            pid_file = NULL;
        }
    }

    {
        gboolean remove;
        remove = milter_manager_configuration_is_remove_manager_unix_socket_on_create(config);
        milter_client_set_remove_unix_socket_on_create(client, remove);
    }

    the_manager = manager;

#define SETUP_SIGNAL_ACTION(handler)            \
    handler ## _action.sa_handler = handler;    \
    sigemptyset(&handler ## _action.sa_mask);   \
    handler ## _action.sa_flags = 0

    SETUP_SIGNAL_ACTION(report_stack_trace);
    SETUP_SIGNAL_ACTION(shutdown_client);
    SETUP_SIGNAL_ACTION(reload_configuration);
#undef SETUP_SIGNAL_ACTION

#define SET_SIGNAL_ACTION(SIGNAL, signal, action)               \
    if (sigaction(SIG ## SIGNAL,                                \
                  &action,                                      \
                  &default_sig ## signal ## _action) == -1)     \
        set_sig ## signal ## _action = FALSE

    SET_SIGNAL_ACTION(SEGV, segv, report_stack_trace_action);
    SET_SIGNAL_ACTION(ABRT, abort, report_stack_trace_action);
    SET_SIGNAL_ACTION(INT, int, shutdown_client_action);
    SET_SIGNAL_ACTION(TERM, term, shutdown_client_action);
    SET_SIGNAL_ACTION(HUP, hup, reload_configuration_action);
#undef SET_SIGNAL_ACTION

    if (!milter_client_main(client))
        milter_manager_error("failed to start milter-manager process.");

#define UNSET_SIGNAL_ACTION(SIGNAL, signal)                             \
    if (set_sig ## signal ## _action)                                   \
        sigaction(SIG ## SIGNAL, &default_sig ## signal ## _action, NULL)

    UNSET_SIGNAL_ACTION(SEGV, segv);
    UNSET_SIGNAL_ACTION(ABRT, abort);
    UNSET_SIGNAL_ACTION(INT, int);
    UNSET_SIGNAL_ACTION(TERM, term);
    UNSET_SIGNAL_ACTION(HUP, hup);
#undef UNSET_SIGNAL_ACTION

    if (controller)
        g_object_unref(controller);

    if (the_manager) {
        g_object_unref(the_manager);
        the_manager = NULL;
    }

    if (pid_file) {
        if (g_unlink(pid_file) == -1)
            milter_error("[manager][error][pid][remove] %s: %s",
                         pid_file, g_strerror(errno));
        g_free(pid_file);
    }

    return TRUE;
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
