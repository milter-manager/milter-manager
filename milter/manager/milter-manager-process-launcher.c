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

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <pwd.h>
#include <errno.h>

#include "milter-manager-process-launcher.h"
#include "milter-manager-enum-types.h"
#include "milter-manager-launch-command-decoder.h"
#include "milter-manager-reply-encoder.h"
#include "milter-manager-configuration.h"

#define MILTER_MANAGER_PROCESS_LAUNCHER_GET_PRIVATE(obj)                   \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj),                              \
                                 MILTER_TYPE_MANAGER_PROCESS_LAUNCHER,     \
                                 MilterManagerProcessLauncherPrivate))

typedef struct _MilterManagerProcessLauncherPrivate MilterManagerProcessLauncherPrivate;
struct _MilterManagerProcessLauncherPrivate
{
    MilterManagerConfiguration *configuration;
    GList *processes;
};

enum
{
    PROP_0,
    PROP_CONFIGURATION
};

G_DEFINE_TYPE(MilterManagerProcessLauncher, milter_manager_process_launcher,
              MILTER_TYPE_AGENT)

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
    GParamSpec *spec;

    gobject_class = G_OBJECT_CLASS(klass);
    agent_class = MILTER_AGENT_CLASS(klass);

    gobject_class->dispose      = dispose;
    gobject_class->set_property = set_property;
    gobject_class->get_property = get_property;

    agent_class->decoder_new   = decoder_new;
    agent_class->encoder_new   = encoder_new;

    spec = g_param_spec_object("configuration",
                               "Configuration",
                               "The configuration of the milter manager",
                               MILTER_TYPE_MANAGER_CONFIGURATION,
                               G_PARAM_READWRITE);
    g_object_class_install_property(gobject_class, PROP_CONFIGURATION, spec);

    g_type_class_add_private(gobject_class,
                             sizeof(MilterManagerProcessLauncherPrivate));
}

static gboolean
set_user (const gchar *user_name, GError **error)
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

    if (setregid(-1, password->pw_gid) == -1) {
        g_set_error(error,
                    MILTER_MANAGER_CHILD_ERROR,
                    MILTER_MANAGER_CHILD_ERROR_NO_PRIVILEGE_MODE,
                    "MilterManager is not running on privilege mode.");
        return FALSE;
    }

    if (setreuid(-1, password->pw_uid) == -1) {
        g_set_error(error,
                    MILTER_MANAGER_CHILD_ERROR,
                    MILTER_MANAGER_CHILD_ERROR_NO_PRIVILEGE_MODE,
                    "MilterManager is not running on privilege mode.");
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
    MilterManagerProcessLauncherPrivate *priv;

    priv = MILTER_MANAGER_PROCESS_LAUNCHER_GET_PRIVATE(user_data);

    if (WIFSIGNALED(status)) {
        /* FIXME restart */
    } else {
        GList *process;
        process = g_list_find_custom(priv->processes, GINT_TO_POINTER(pid),
                                     compare_process_data_pid);
        if (!process)
            return;
        process_data_free(process->data);
        priv->processes = g_list_remove_link(priv->processes, process);
    }
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
                "%s has been already launched.", 
                command_line);
    return TRUE;
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

    if (is_already_launched(launcher, command_line, error))
        return FALSE;

    if (user_name && !set_user(user_name, error))
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
                            NULL,
                            NULL,
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

    milter_debug("started: <%s>", command_line);

    priv = MILTER_MANAGER_PROCESS_LAUNCHER_GET_PRIVATE(launcher);
    watch_id = g_child_watch_add(pid, (GChildWatchFunc)child_watch_func, launcher);
    priv->processes = g_list_prepend(priv->processes,
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

    if (!milter_manager_configuration_is_privilege_mode(priv->configuration)) {
        milter_manager_reply_encoder_encode_error(
            encoder, &packet, &packet_size,
            "MilterManager is not running on privilege mode.");
    } else if (!launch(launcher, command_line, user_name, &error)) {
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
        milter_error("failed to write reply success packet: <%s>",
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

    priv->configuration = NULL;
    priv->processes = NULL;
}

static void
dispose (GObject *object)
{
    MilterManagerProcessLauncherPrivate *priv;

    priv = MILTER_MANAGER_PROCESS_LAUNCHER_GET_PRIVATE(object);

    if (priv->configuration) {
        g_object_unref(priv->configuration);
        priv->configuration = NULL;
    }

    if (priv->processes) {
        g_list_foreach(priv->processes, (GFunc)process_data_free, NULL);
        g_list_free(priv->processes);
        priv->processes = NULL;
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
      case PROP_CONFIGURATION:
        if (priv->configuration)
            g_object_unref(priv->configuration);
        priv->configuration = g_value_get_object(value);
        if (priv->configuration)
            g_object_ref(priv->configuration);
        break;
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
      case PROP_CONFIGURATION:
        g_value_set_object(value, priv->configuration);
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

GQuark
milter_manager_process_launcher_error_quark (void)
{
    return g_quark_from_static_string("milter-manager-process-launcher-error-quark");
}

MilterManagerProcessLauncher *
milter_manager_process_launcher_new (MilterManagerConfiguration *configuration)
{
    return g_object_new(MILTER_TYPE_MANAGER_PROCESS_LAUNCHER,
                        "configuration", configuration,
                        NULL);
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
