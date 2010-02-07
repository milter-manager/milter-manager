/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Copyright (C) 2008-2010  Kouhei Sutou <kou@clear-code.com>
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

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <pwd.h>
#include <grp.h>
#include <errno.h>

#include "milter-manager-process-launcher.h"
#include "milter-manager-enum-types.h"
#include "milter-manager-launch-command-decoder.h"
#include "milter-manager-reply-encoder.h"

#define MILTER_MANAGER_PROCESS_LAUNCHER_GET_PRIVATE(obj)                   \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj),                              \
                                 MILTER_TYPE_MANAGER_PROCESS_LAUNCHER,     \
                                 MilterManagerProcessLauncherPrivate))

typedef struct _MilterManagerProcessLauncherPrivate MilterManagerProcessLauncherPrivate;
struct _MilterManagerProcessLauncherPrivate
{
    GList *processes;
    GMainLoop *main_loop;
};

enum
{
    PROP_0
};

static void         finished           (MilterFinishedEmittable *emittable);

MILTER_IMPLEMENT_FINISHED_EMITTABLE_WITH_CODE(finished_emittable_init,
                                              iface->finished = finished)
G_DEFINE_TYPE_WITH_CODE(MilterManagerProcessLauncher,
                        milter_manager_process_launcher,
                        MILTER_TYPE_AGENT,
                        G_IMPLEMENT_INTERFACE(MILTER_TYPE_FINISHED_EMITTABLE,
                                              finished_emittable_init))

typedef struct _ProcessData
{
    GPid pid;
    guint watch_id;
    gchar *command_line;
    gchar *user_name;
} ProcessData;

static void dispose        (GObject         *object);
static void set_property   (GObject         *object,
                            guint            prop_id,
                            const GValue    *value,
                            GParamSpec      *pspec);
static void get_property   (GObject         *object,
                            guint            prop_id,
                            GValue          *value,
                            GParamSpec      *pspec);

static MilterDecoder *decoder_new    (MilterAgent *agent);
static MilterEncoder *encoder_new    (MilterAgent *agent);

static void
milter_manager_process_launcher_class_init (MilterManagerProcessLauncherClass *klass)
{
    GObjectClass *gobject_class;
    MilterAgentClass *agent_class;

    gobject_class = G_OBJECT_CLASS(klass);
    agent_class = MILTER_AGENT_CLASS(klass);

    gobject_class->dispose      = dispose;
    gobject_class->set_property = set_property;
    gobject_class->get_property = get_property;

    agent_class->decoder_new   = decoder_new;
    agent_class->encoder_new   = encoder_new;

    g_type_class_add_private(gobject_class,
                             sizeof(MilterManagerProcessLauncherPrivate));
}

static gboolean
is_valid_user (const gchar *user_name, GError **error)
{
    struct passwd *password;

    errno = 0;
    password = getpwnam(user_name);
    if (!password) {
        g_set_error(error,
                    MILTER_MANAGER_CHILD_ERROR,
                    MILTER_MANAGER_CHILD_ERROR_INVALID_USER_NAME,
                    "No password entry: <%s>%s%s",
                    user_name,
                    errno == 0 ? "" : ": ",
                    errno == 0 ? "" : g_strerror(errno));
        return FALSE;
    }

    return TRUE;
}

static ProcessData *
process_data_new (GPid pid, guint watch_id,
                  const gchar *command_line, const gchar *user_name)
{
    ProcessData *data;

    data = g_new0(ProcessData, 1);

    data->pid = pid;
    data->watch_id = watch_id;
    data->command_line = g_strdup(command_line);
    data->user_name = g_strdup(user_name);

    return data;
}

static void
process_data_free (ProcessData *data)
{
    g_source_remove(data->watch_id);
    g_spawn_close_pid(data->pid);
    g_free(data->command_line);
    g_free(data->user_name);
    g_free(data);
}

static gint
compare_process_data_command_line (gconstpointer a, gconstpointer b)
{
    ProcessData *data;
    data = (ProcessData*)a;

    return strcmp(data->command_line, b);
}

static gint
compare_process_data_pid (gconstpointer a, gconstpointer b)
{
    ProcessData *data;
    data = (ProcessData*)a;

    return (data->pid - GPOINTER_TO_INT(b));
}

static void
child_watch_func (GPid pid, gint status, gpointer user_data)
{
    GList *process;
    ProcessData *data;
    MilterManagerProcessLauncherPrivate *priv;

    priv = MILTER_MANAGER_PROCESS_LAUNCHER_GET_PRIVATE(user_data);

    process = g_list_find_custom(priv->processes, GINT_TO_POINTER(pid),
                                 compare_process_data_pid);
    if (!process)
        return;

    data = process->data;
    if (WIFSIGNALED(status)) {
        /* FIXME restart */
    } else if (WIFEXITED(status)){
        milter_debug("[launcher][finish][success] <%s>", data->command_line);
    } else {
        milter_debug("[launcher][finish][failure] %d: <%s>",
                    status, data->command_line);
    }
    process_data_free(process->data);
    priv->processes = g_list_remove_link(priv->processes, process);
}

static gboolean
is_already_launched (MilterManagerProcessLauncher *launcher,
                     const gchar *command_line,
                     GError **error)
{
    MilterManagerProcessLauncherPrivate *priv;

    priv = MILTER_MANAGER_PROCESS_LAUNCHER_GET_PRIVATE(launcher);

    if (!g_list_find_custom(priv->processes, command_line,
                            compare_process_data_command_line))
        return FALSE;

    g_set_error(error,
                MILTER_MANAGER_PROCESS_LAUNCHER_ERROR,
                MILTER_MANAGER_PROCESS_LAUNCHER_ERROR_ALREADY_LAUNCHED,
                "already launched: <%s>",
                command_line);
    return TRUE;
}

static void
setup_child (gpointer user_data)
{
    const gchar *user_name = user_data;
    struct passwd *password;

    if (!user_name)
        return;

    errno = 0;
    password = getpwnam(user_name);
    if (!password) {
        milter_error("No password entry: <%s>%s%s",
                     user_name,
                     errno == 0 ? "" : ": ",
                     errno == 0 ? "" : g_strerror(errno));
        return;
    }

    if (setgid(password->pw_gid) == -1) {
        milter_error("failed to change GID: %s", g_strerror(errno));
        return;
    }

    if (initgroups(password->pw_name, password->pw_gid) == -1) {
        milter_error("failed to initialize group access: %s", g_strerror(errno));
        return;
    }

    if (setuid(password->pw_uid) == -1) {
        milter_error("failed to change UID: %s", g_strerror(errno));
        return;
    }
}

static gboolean
launch (MilterManagerProcessLauncher *launcher,
        const gchar *command_line, const gchar *user_name,
        GError **error)
{
    gint argc;
    gchar **argv;
    gboolean success;
    GError *internal_error = NULL;
    GSpawnFlags flags;
    GPid pid;
    guint watch_id;
    MilterManagerProcessLauncherPrivate *priv;

    milter_debug("[launcher][launch] <%s>@<%s>", command_line, user_name);

    if (is_already_launched(launcher, command_line, error))
        return FALSE;

    if (user_name && !is_valid_user(user_name, error))
        return FALSE;

    success = g_shell_parse_argv(command_line,
                                 &argc, &argv,
                                 &internal_error);

    if (!success) {
        milter_utils_set_error_with_sub_error(
            error,
            MILTER_MANAGER_PROCESS_LAUNCHER_ERROR,
            MILTER_MANAGER_PROCESS_LAUNCHER_ERROR_BAD_COMMAND_STRING,
            internal_error,
            "Command string has invalid character(s): <%s>",
            command_line);
        return FALSE;
    }

    flags = G_SPAWN_DO_NOT_REAP_CHILD |
        G_SPAWN_STDOUT_TO_DEV_NULL |
        G_SPAWN_STDERR_TO_DEV_NULL;

    success = g_spawn_async(NULL,
                            argv,
                            NULL,
                            flags,
                            setup_child,
                            (gpointer)user_name,
                            &pid,
                            &internal_error);
    g_strfreev(argv);

    if (!success) {
        milter_utils_set_error_with_sub_error(
            error,
            MILTER_MANAGER_PROCESS_LAUNCHER_ERROR,
            MILTER_MANAGER_PROCESS_LAUNCHER_ERROR_START_FAILURE,
            internal_error,
            "Couldn't start new process: <%s>",
            command_line);
        return FALSE;
    }

    milter_debug("[launcher][start] <%s>", command_line);

    priv = MILTER_MANAGER_PROCESS_LAUNCHER_GET_PRIVATE(launcher);
    watch_id = g_child_watch_add(pid,
                                 (GChildWatchFunc)child_watch_func, launcher);
    priv->processes =
        g_list_prepend(priv->processes,
                       process_data_new(pid, watch_id, command_line, user_name));

    return TRUE;
}

static void
cb_decoder_launch (MilterManagerLaunchCommandDecoder *decoder,
                   const gchar *command_line,
                   const gchar *user_name,
                   gpointer user_data)
{
    MilterManagerProcessLauncher *launcher = user_data;
    MilterManagerProcessLauncherPrivate *priv;
    GError *error = NULL;
    MilterAgent *agent;
    MilterEncoder *_encoder;
    MilterManagerReplyEncoder *encoder;
    gchar *packet;
    gsize packet_size;

    priv = MILTER_MANAGER_PROCESS_LAUNCHER_GET_PRIVATE(launcher);

    agent = MILTER_AGENT(launcher);
    _encoder = milter_agent_get_encoder(agent);
    encoder = MILTER_MANAGER_REPLY_ENCODER(_encoder);

    if (!launch(launcher, command_line, user_name, &error)) {
        milter_error("[launcher][error][launch] <%s>@<%s>: %s",
                     command_line,
                     user_name ? user_name : "NULL",
                     error->message);
        milter_manager_reply_encoder_encode_error(encoder,
                                                  &packet,
                                                  &packet_size,
                                                  error->message);
        g_clear_error(&error);
    } else {
        milter_manager_reply_encoder_encode_success(encoder,
                                                    &packet,
                                                    &packet_size);
    }

    if (!milter_agent_write_packet(agent, packet, packet_size, &error)) {
        milter_error("[launcher][error][write] <%s>@<%s>: %s",
                     command_line,
                     user_name ? user_name : "NULL",
                     error->message);
        g_error_free(error);
    }
}

static MilterDecoder *
decoder_new (MilterAgent *agent)
{
    MilterDecoder *decoder;

    decoder = milter_manager_launch_command_decoder_new();

#define CONNECT(name)                                                   \
    g_signal_connect(decoder, #name, G_CALLBACK(cb_decoder_ ## name),   \
                     agent)

    CONNECT(launch);

#undef CONNECT

    return decoder;
}

static MilterEncoder *
encoder_new (MilterAgent *agent)
{
    return milter_manager_reply_encoder_new();
}

static void
milter_manager_process_launcher_init (MilterManagerProcessLauncher *launcher)
{
    MilterManagerProcessLauncherPrivate *priv;

    priv = MILTER_MANAGER_PROCESS_LAUNCHER_GET_PRIVATE(launcher);

    priv->processes = NULL;
    priv->main_loop = g_main_loop_new(NULL, FALSE);
}

static void
dispose (GObject *object)
{
    MilterManagerProcessLauncherPrivate *priv;

    priv = MILTER_MANAGER_PROCESS_LAUNCHER_GET_PRIVATE(object);

    if (priv->processes) {
        g_list_foreach(priv->processes, (GFunc)process_data_free, NULL);
        g_list_free(priv->processes);
        priv->processes = NULL;
    }

    if (priv->main_loop) {
        g_main_loop_unref(priv->main_loop);
        priv->main_loop = NULL;
    }

    G_OBJECT_CLASS(milter_manager_process_launcher_parent_class)->dispose(object);
}

static void
set_property (GObject      *object,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
    MilterManagerProcessLauncherPrivate *priv;

    priv = MILTER_MANAGER_PROCESS_LAUNCHER_GET_PRIVATE(object);
    switch (prop_id) {
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static void
get_property (GObject    *object,
              guint       prop_id,
              GValue     *value,
              GParamSpec *pspec)
{
    MilterManagerProcessLauncherPrivate *priv;

    priv = MILTER_MANAGER_PROCESS_LAUNCHER_GET_PRIVATE(object);
    switch (prop_id) {
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static void
finished (MilterFinishedEmittable *emittable)
{
    MilterManagerProcessLauncher *launcher;
    MilterManagerProcessLauncherPrivate *priv;

    launcher = MILTER_MANAGER_PROCESS_LAUNCHER(emittable);
    priv = MILTER_MANAGER_PROCESS_LAUNCHER_GET_PRIVATE(launcher);
    g_main_loop_quit(priv->main_loop);

    if (finished_emittable_parent->finished)
        finished_emittable_parent->finished(emittable);
}

GQuark
milter_manager_process_launcher_error_quark (void)
{
    return g_quark_from_static_string("milter-manager-process-launcher-error-quark");
}

MilterManagerProcessLauncher *
milter_manager_process_launcher_new (void)
{
    return g_object_new(MILTER_TYPE_MANAGER_PROCESS_LAUNCHER,
                        NULL);
}

void
milter_manager_process_launcher_run (MilterManagerProcessLauncher *launcher)
{
    MilterManagerProcessLauncherPrivate *priv;

    priv = MILTER_MANAGER_PROCESS_LAUNCHER_GET_PRIVATE(launcher);
    g_main_loop_run(priv->main_loop);
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
