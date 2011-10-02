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

#ifdef HAVE_CONFIG_H
#  include "../../config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/time.h>
#include <sys/resource.h>
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
static gchar *option_config_dir = NULL;
static gboolean option_show_config = FALSE;

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

static const GOptionEntry option_entries[] =
{
    {"config-dir", 'c',
     0, G_OPTION_ARG_FILENAME, &option_config_dir,
     N_("The configuration directory that has configuration file."),
     "DIRECTORY"},
    {"show-config", 0, 0, G_OPTION_ARG_NONE, &option_show_config,
     N_("Show configuration and exit"), NULL},
    {"version", 0, G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK, print_version,
     N_("Show version"), NULL},
    {NULL}
};

void
milter_manager_init (int *argc, char ***argv)
{
    GOptionContext *option_context;
    GOptionGroup *milter_group;
    GError *error = NULL;
    MilterManager *manager;
    MilterManagerConfiguration *config;

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
    g_option_context_add_main_entries(option_context, option_entries,
                                      GETTEXT_PACKAGE);

    _milter_manager_configuration_init();
    config = milter_manager_configuration_new(NULL);
    manager = milter_manager_new(config);
    g_object_unref(config);
    milter_group = milter_client_get_option_group(MILTER_CLIENT(manager));
    g_option_context_add_group(option_context, milter_group);

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
        g_object_unref(manager);

        exit(EXIT_FAILURE);
    }

    g_option_context_free(option_context);
    the_manager = manager;
}

void
milter_manager_quit (void)
{
    if (!initialized)
        return;

    if (the_manager) {
        g_object_unref(the_manager);
        the_manager = NULL;
    }

    _milter_manager_configuration_quit();

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

static gboolean
cb_idle_reload_configuration (gpointer user_data)
{
    if (the_manager) {
        GError *error = NULL;

        if (!milter_manager_reload(the_manager, &error)) {
            milter_error("[manager][reload][signal][error] %s",
                         error->message);
            g_error_free(error);
        }
    }

    return FALSE;
}

static void
reload_configuration_request (int signum)
{
    if (the_manager) {
        MilterEventLoop *loop;

        loop = milter_client_get_event_loop(MILTER_CLIENT(the_manager));
        milter_event_loop_add_idle_full(loop,
                                        G_PRIORITY_DEFAULT,
                                        cb_idle_reload_configuration,
                                        NULL,
                                        NULL);
    }
}

static void
cb_error (MilterErrorEmittable *emittable, GError *error, gpointer user_data)
{
    g_print("milter-manager error: %s\n", error->message);
}

static void
close_pipe (int *pipe, MilterUtilsPipeMode mode)
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
        close((*command_pipe)[MILTER_UTILS_WRITE_PIPE]);
        close((*command_pipe)[MILTER_UTILS_READ_PIPE]);
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
    close_pipe(command_pipe, MILTER_UTILS_WRITE_PIPE);
    close_pipe(reply_pipe, MILTER_UTILS_READ_PIPE);

    *write_channel = create_write_io_channel(reply_pipe[MILTER_UTILS_WRITE_PIPE]);
    *read_channel = create_read_io_channel(command_pipe[MILTER_UTILS_READ_PIPE]);
}

static void
prepare_process_launcher_pipes_for_manager (gint *command_pipe,
                                            gint *reply_pipe,
                                            GIOChannel **read_channel,
                                            GIOChannel **write_channel)
{
    close_pipe(command_pipe, MILTER_UTILS_READ_PIPE);
    close_pipe(reply_pipe, MILTER_UTILS_WRITE_PIPE);

    *write_channel = create_write_io_channel(command_pipe[MILTER_UTILS_WRITE_PIPE]);
    *read_channel = create_read_io_channel(reply_pipe[MILTER_UTILS_READ_PIPE]);
}

static void
cb_launcher_finished (MilterFinishedEmittable *emittable,
                      gpointer user_data)
{
    MilterManagerProcessLauncher *launcher;

    launcher = MILTER_MANAGER_PROCESS_LAUNCHER(emittable);
    milter_manager_process_launcher_shutdown(launcher);
}

static gboolean
start_process_launcher (GIOChannel *read_channel, GIOChannel *write_channel,
                        gboolean daemon)
{
    gboolean success = TRUE;
    MilterManagerProcessLauncher *launcher;
    MilterReader *reader;
    MilterWriter *writer;
    gchar *error_message = NULL;
    MilterSyslogLogger *logger;
    MilterEventLoop *loop;
    GError *error = NULL;

    logger = milter_syslog_logger_new("milter-manager-process-launcher",
                                      NULL);

    reader = milter_reader_io_channel_new(read_channel);
    g_io_channel_unref(read_channel);

    writer = milter_writer_io_channel_new(write_channel);

    launcher = milter_manager_process_launcher_new();
    milter_agent_set_reader(MILTER_AGENT(launcher), reader);
    milter_agent_set_writer(MILTER_AGENT(launcher), writer);
    g_object_unref(reader);
    g_object_unref(writer);

    loop = milter_glib_event_loop_new(NULL);
    milter_agent_set_event_loop(MILTER_AGENT(launcher), loop);
    g_object_unref(loop);

    if (!milter_agent_start(MILTER_AGENT(launcher), &error)) {
        milter_error("[process-launcher][error][start] %s",
                     error->message);
        g_error_free(error);
        success = FALSE;
    }

    if (success && daemon && !milter_utils_detach_io(&error_message)) {
        milter_error("[process-launcher][error] failed to detach IO: %s",
                     error_message);
        g_free(error_message);
        success = FALSE;
    }

    if (success) {
        g_signal_connect(launcher, "finished",
                         G_CALLBACK(cb_launcher_finished), NULL);
        milter_manager_process_launcher_run(launcher);
    }

    g_object_unref(launcher);

    g_object_unref(logger);

    return success;
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
    gboolean daemon;

    command_pipe_p = command_pipe;
    reply_pipe_p = reply_pipe;
    if (!create_process_launcher_pipes(&command_pipe_p, &reply_pipe_p))
        return FALSE;

    milter_debug("[manager][process-launcher][pipe] "
                 "command:<%d:%d>, reply:<%d:%d>",
                 command_pipe[MILTER_UTILS_READ_PIPE],
                 command_pipe[MILTER_UTILS_WRITE_PIPE],
                 reply_pipe[MILTER_UTILS_READ_PIPE],
                 reply_pipe[MILTER_UTILS_WRITE_PIPE]);

    switch (milter_client_fork(MILTER_CLIENT(manager))) {
    case 0:
        /* FIXME: change process name to
           'milter-manager: process-launcher' by
           setproctitle() on *BSD, memcpy() argv on Linux or
           spawning another process. */
        daemon = milter_client_is_run_as_daemon(MILTER_CLIENT(manager));
        g_object_unref(manager);
        prepare_process_launcher_pipes_for_process_launcher(command_pipe_p,
                                                            reply_pipe_p,
                                                            &read_channel,
                                                            &write_channel);
        if (start_process_launcher(read_channel, write_channel, daemon)) {
            _exit(EXIT_SUCCESS);
        } else {
            _exit(EXIT_FAILURE);
        }
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

/* TODO: unify with find_password() in milter_client.c */
static struct passwd *
find_password (const gchar *effective_user)
{
    struct passwd *password;

    if (!effective_user)
        return NULL;

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
    }

    return password;
}

static void
update_max_file_descriptors (MilterManager *manager)
{
    MilterManagerConfiguration *configuration;
    guint max_file_descriptors;
    struct rlimit original_rlimit;
    struct rlimit rlimit;

    configuration = milter_manager_get_configuration(manager);

    max_file_descriptors =
        milter_manager_configuration_get_max_file_descriptors(configuration);
    if (max_file_descriptors == 0)
        return;

    if (getrlimit(RLIMIT_NOFILE, &original_rlimit) != 0) {
        milter_manager_error("failed to get limit for RLIMIT_NOFILE: %s",
                             g_strerror(errno));
        return;
    }

    memcpy(&rlimit, &original_rlimit, sizeof(original_rlimit));
    rlimit.rlim_cur = max_file_descriptors;
    rlimit.rlim_max = max_file_descriptors;
    if (setrlimit(RLIMIT_NOFILE, &rlimit) != 0) {
        milter_manager_error("failed to set limit for RLIMIT_NOFILE: "
                             "%u "
                             "original soft limit: %" G_GINT64_FORMAT " "
                             "original hard limit: %" G_GINT64_FORMAT " "
                             ": %s",
                             max_file_descriptors,
                             (gint64)original_rlimit.rlim_cur,
                             (gint64)original_rlimit.rlim_max,
                             g_strerror(errno));
        return;
    }
}

static void
append_custom_configuration_directory (MilterManagerConfiguration *config)
{
    struct passwd *password;
    const gchar *effective_user;
    gchar *custom_config_directory;

    effective_user = milter_manager_configuration_get_effective_user(config);
    if (!effective_user)
        return;

    password = find_password(effective_user);
    if (!password)
        return;

    custom_config_directory =
        g_strdup(milter_manager_configuration_get_custom_configuration_directory(config));
    if (!custom_config_directory)
        custom_config_directory = g_build_filename(password->pw_dir,
                                                   ".milter-manager",
                                                   NULL);
    if (!g_file_test(custom_config_directory, G_FILE_TEST_EXISTS)) {
        if (g_mkdir(custom_config_directory, 0700) == -1) {
            milter_manager_error("failed to create custom "
                                 "configuration directory: <%s>: %s",
                                 custom_config_directory,
                                 g_strerror(errno));
            return;
        }
        if (chown(custom_config_directory,
                  password->pw_uid, password->pw_gid) == -1) {
            milter_manager_error("failed to change owner and group of "
                                 "configuration directory: <%s>: <%u:%u>: %s",
                                 custom_config_directory,
                                 password->pw_uid, password->pw_gid,
                                 g_strerror(errno));
            return;
        }
    }

    milter_manager_configuration_append_load_path(config,
                                                  custom_config_directory);
    g_free(custom_config_directory);
}

static void
load_configuration (MilterManager *manager)
{
    GError *error = NULL;

    if (!milter_manager_reload(manager, &error)) {
        milter_manager_error("[manager][reload][custom-load-path][error] %s",
                             error->message);
        g_error_free(error);
    }
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
    MilterEventLoop *loop;
    gboolean remove_socket, daemon;
    GError *error = NULL;
    struct sigaction report_stack_trace_action;
    struct sigaction shutdown_client_action;
    struct sigaction reload_configuration_request_action;

    manager = the_manager;
    config = milter_manager_get_configuration(manager);

    if (option_config_dir) {
        milter_manager_configuration_prepend_load_path(config,
                                                       option_config_dir);
        if (!milter_manager_reload(manager, &error)) {
            milter_manager_error("[manager][configuration][reload]"
                                 "[command-line-load-path][error] %s",
                                 error->message);
            g_error_free(error);
            error = NULL;
        }
    }
    if (getuid() != 0)
        milter_manager_configuration_set_privilege_mode(config, FALSE);


    client = MILTER_CLIENT(manager);
    g_signal_connect(client, "error", G_CALLBACK(cb_error), NULL);

    append_custom_configuration_directory(config);
    load_configuration(manager);

    if (option_show_config) {
        gchar *dumped_config;

        dumped_config = milter_manager_configuration_dump(config);
        if (dumped_config) {
            g_print("%s", dumped_config);
            if (!g_str_has_suffix(dumped_config, "\n"))
                g_print("\n");
            g_free(dumped_config);
        }
        return TRUE;
    }

    update_max_file_descriptors(manager);

    if (milter_manager_configuration_is_privilege_mode(config) &&
        !start_process_launcher_process(manager)) {
        return FALSE;
    }

    remove_socket = milter_manager_configuration_is_remove_manager_unix_socket_on_create(config);
    milter_client_set_remove_unix_socket_on_create(client, remove_socket);

    if (!milter_client_listen(client, &error)) {
        milter_manager_error("failed to listen: %s", error->message);
        g_error_free(error);
        return FALSE;
    }

    if (!milter_client_drop_privilege(client, &error)) {
        milter_manager_error("failed to drop privilege: %s", error->message);
        g_error_free(error);
        return FALSE;
    }

    loop = milter_client_get_event_loop(client);
    controller = milter_manager_controller_new(manager, loop);
    if (controller && !milter_manager_controller_listen(controller, &error)) {
        milter_manager_error("failed to listen controller socket: %s",
                             error->message);
        g_error_free(error);
        error = NULL;
        g_object_unref(controller);
        controller = NULL;
    }

    daemon = milter_client_is_run_as_daemon(client);
    if (daemon) {
        if (milter_client_daemonize(client, &error)) {
            io_detached = TRUE;
        } else {
            milter_manager_error("failed to daemonize: %s", error->message);
            g_error_free(error);
            if (controller)
                g_object_unref(controller);
            return FALSE;
        }
    }

    the_manager = manager;

#define SETUP_SIGNAL_ACTION(handler)            \
    handler ## _action.sa_handler = handler;    \
    sigemptyset(&handler ## _action.sa_mask);   \
    handler ## _action.sa_flags = 0

    SETUP_SIGNAL_ACTION(report_stack_trace);
    SETUP_SIGNAL_ACTION(shutdown_client);
    SETUP_SIGNAL_ACTION(reload_configuration_request);
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
    SET_SIGNAL_ACTION(HUP, hup, reload_configuration_request_action);
#undef SET_SIGNAL_ACTION

    if (!milter_client_run(client, &error)) {
        milter_manager_error("failed to start milter-manager process: %s",
                             error->message);
        g_error_free(error);
    }

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

    return TRUE;
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
