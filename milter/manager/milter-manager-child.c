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
#include <unistd.h>
#include <pwd.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>
#include "milter-manager-child.h"

#define MILTER_MANAGER_CHILD_GET_PRIVATE(obj)                    \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj),                          \
                                 MILTER_TYPE_MANAGER_CHILD,      \
                                 MilterManagerChildPrivate))

typedef struct _MilterManagerChildPrivate	MilterManagerChildPrivate;
struct _MilterManagerChildPrivate
{
    gchar *user_name;
    gchar *working_directory;
    gchar *command;
    gchar *command_options;
    gboolean search_path;
    GSpawnChildSetupFunc child_setup;
    GPid pid;
    guint child_watch_id;
};

enum
{
    PROP_0,
    PROP_USER_NAME,
    PROP_COMMAND,
    PROP_COMMAND_OPTIONS,
    PROP_WORKING_DIRECTORY,
    PROP_SEARCH_PATH
};

MILTER_DEFINE_ERROR_EMITTABLE_TYPE(MilterManagerChild,
                                   milter_manager_child,
                                   MILTER_TYPE_SERVER_CONTEXT);

static void dispose        (GObject         *object);
static void set_property   (GObject         *object,
                            guint            prop_id,
                            const GValue    *value,
                            GParamSpec      *pspec);
static void get_property   (GObject         *object,
                            guint            prop_id,
                            GValue          *value,
                            GParamSpec      *pspec);

static void
milter_manager_child_class_init (MilterManagerChildClass *klass)
{
    GObjectClass *gobject_class;
    GParamSpec *spec;

    gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->dispose      = dispose;
    gobject_class->set_property = set_property;
    gobject_class->get_property = get_property;

    spec = g_param_spec_string("user-name",
                               "User name",
                               "The name of process owner user",
                               NULL,
                               G_PARAM_READWRITE);
    g_object_class_install_property(gobject_class, PROP_USER_NAME, spec);

    spec = g_param_spec_string("command",
                               "Command",
                               "The command string of process",
                               NULL,
                               G_PARAM_READWRITE);
    g_object_class_install_property(gobject_class, PROP_COMMAND, spec);

    spec = g_param_spec_string("command-options",
                               "Command options",
                               "The command options string of process",
                               NULL,
                               G_PARAM_READWRITE);
    g_object_class_install_property(gobject_class, PROP_COMMAND_OPTIONS, spec);

    spec = g_param_spec_string("working-directory",
                               "Working directory",
                               "The working directory name of process",
                               NULL,
                               G_PARAM_READWRITE);
    g_object_class_install_property(gobject_class, PROP_WORKING_DIRECTORY, spec);

    spec = g_param_spec_boolean("search-path",
                                "Whether search in PATH",
                                "Whether the command of process is "
                                "searched in PATH or not",
                                TRUE,
                                G_PARAM_READWRITE);
    g_object_class_install_property(gobject_class, PROP_SEARCH_PATH, spec);

    g_type_class_add_private(gobject_class,
                             sizeof(MilterManagerChildPrivate));
}

static void
milter_manager_child_init (MilterManagerChild *milter)
{
    MilterManagerChildPrivate *priv;

    priv = MILTER_MANAGER_CHILD_GET_PRIVATE(milter);
    priv->user_name = NULL;
    priv->working_directory = NULL;
    priv->command = NULL;
    priv->command_options = NULL;
    priv->search_path = TRUE;
    priv->child_setup = NULL;
    priv->pid = -1;
    priv->child_watch_id = 0;
}

static void
dispose (GObject *object)
{
    MilterManagerChildPrivate *priv;

    priv = MILTER_MANAGER_CHILD_GET_PRIVATE(object);

    if (priv->user_name) {
        g_free(priv->user_name);
        priv->user_name = NULL;
    }

    if (priv->working_directory) {
        g_free(priv->working_directory);
        priv->working_directory = NULL;
    }

    if (priv->command) {
        g_free(priv->command);
        priv->command = NULL;
    }

    if (priv->command_options) {
        g_free(priv->command_options);
        priv->command_options = NULL;
    }

    if (priv->pid > 0) {
        g_spawn_close_pid(priv->pid);
        priv->pid = -1;
    }

    if (priv->child_watch_id > 0) {
        g_source_remove(priv->child_watch_id);
        priv->child_watch_id = 0;
    }

    G_OBJECT_CLASS(milter_manager_child_parent_class)->dispose(object);
}

static void
set_property (GObject      *object,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
    MilterManagerChild *milter;
    MilterManagerChildPrivate *priv;

    milter = MILTER_MANAGER_CHILD(object);
    priv = MILTER_MANAGER_CHILD_GET_PRIVATE(object);
    switch (prop_id) {
      case PROP_USER_NAME:
        if (priv->user_name)
            g_free(priv->user_name);
        priv->user_name = g_value_dup_string(value);
        break;
      case PROP_COMMAND:
        if (priv->command)
            g_free(priv->command);
        priv->command = g_value_dup_string(value);
        break;
      case PROP_COMMAND_OPTIONS:
        if (priv->command_options)
            g_free(priv->command_options);
        priv->command_options = g_value_dup_string(value);
        break;
      case PROP_WORKING_DIRECTORY:
        if (priv->working_directory)
            g_free(priv->working_directory);
        priv->working_directory = g_value_dup_string(value);
        break;
      case PROP_SEARCH_PATH:
        priv->search_path = g_value_get_boolean(value);
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
    MilterManagerChildPrivate *priv;

    priv = MILTER_MANAGER_CHILD_GET_PRIVATE(object);
    switch (prop_id) {
      case PROP_USER_NAME:
        g_value_set_string(value, priv->user_name);
        break;
      case PROP_COMMAND:
        g_value_set_string(value, priv->command);
        break;
      case PROP_COMMAND_OPTIONS:
        g_value_set_string(value, priv->command_options);
        break;
      case PROP_WORKING_DIRECTORY:
        g_value_set_string(value, priv->working_directory);
        break;
      case PROP_SEARCH_PATH:
        g_value_set_boolean(value, priv->search_path);
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

GQuark
milter_manager_child_error_quark (void)
{
    return g_quark_from_static_string("milter-manager-child-error-quark");
}

MilterManagerChild *
milter_manager_child_new (const gchar *name)
{
    return g_object_new(MILTER_TYPE_MANAGER_CHILD,
                        "name", name,
                        NULL);
}

MilterManagerChild *
milter_manager_child_new_va_list (const gchar *first_name, va_list args)
{
    return MILTER_MANAGER_CHILD(g_object_new_valist(MILTER_TYPE_MANAGER_CHILD,
                                                    first_name, args));
}

static void
child_watch_func (GPid pid, gint status, gpointer user_data)
{
    MilterManagerChildPrivate *priv;

    priv = MILTER_MANAGER_CHILD_GET_PRIVATE(user_data);

    if (WIFSIGNALED(status)) {
        GError *error = NULL;
        g_set_error(&error,
                    MILTER_MANAGER_CHILD_ERROR,
                    MILTER_MANAGER_CHILD_ERROR_MILTER_TERMINATED_BY_SIGNAL,
                    "%s terminated by signal(%d)", 
                    milter_server_context_get_name(MILTER_SERVER_CONTEXT(user_data)),
                    WTERMSIG(status));
        milter_error("%s", error->message);
        milter_error_emittable_emit(MILTER_ERROR_EMITTABLE(user_data),
                                    error);
        g_error_free(error);
    } else if (WIFEXITED(status)) {
        GError *error = NULL;
        g_set_error(&error,
                    MILTER_MANAGER_CHILD_ERROR,
                    MILTER_MANAGER_CHILD_ERROR_MILTER_EXIT,
                    "%s exits with status: %d", 
                    milter_server_context_get_name(MILTER_SERVER_CONTEXT(user_data)),
                    WEXITSTATUS(status));
        milter_error("%s", error->message);
        milter_error_emittable_emit(MILTER_ERROR_EMITTABLE(user_data),
                                    error);
        g_error_free(error);
    }

    g_spawn_close_pid(priv->pid);
    priv->pid = -1;
    priv->child_watch_id = 0;
}

static gboolean
set_user (const gchar *user_name, GError **error)
{
    struct passwd *password;

    password = getpwnam(user_name);
    if (!password) {
        g_set_error(error,
                    MILTER_MANAGER_CHILD_ERROR,
                    MILTER_MANAGER_CHILD_ERROR_INVALID_USER_NAME,
                    "No passwd entry for %s: %s",
                    user_name, strerror(errno));
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

gboolean
milter_manager_child_start (MilterManagerChild *milter, GError **error)
{
    gint argc;
    gchar **argv;
    gchar *command_line;
    gboolean success;
    MilterManagerChildPrivate *priv;
    GError *internal_error = NULL;
    GSpawnFlags flags;

    priv = MILTER_MANAGER_CHILD_GET_PRIVATE(milter);

    if (!priv->command) {
        g_set_error(error,
                    MILTER_MANAGER_CHILD_ERROR,
                    MILTER_MANAGER_CHILD_ERROR_BAD_COMMAND_STRING,
                    "No command set yet.");
        return FALSE;
    }

    if (priv->command_options)
        command_line = g_strdup_printf("%s %s",
                                       priv->command, priv->command_options);
    else
        command_line = g_strdup(priv->command);

    success = g_shell_parse_argv(command_line,
                                 &argc, &argv,
                                 &internal_error);
    g_free(command_line);

    if (!success) {
        milter_utils_set_error_with_sub_error(
            error,
            MILTER_MANAGER_CHILD_ERROR,
            MILTER_MANAGER_CHILD_ERROR_BAD_COMMAND_STRING,
            internal_error,
            "Command string has invalid character(s).");
        return FALSE;
    }

    if (priv->user_name && !set_user(priv->user_name, error))
        return FALSE;

    flags = G_SPAWN_DO_NOT_REAP_CHILD |
        G_SPAWN_STDOUT_TO_DEV_NULL |
        G_SPAWN_STDERR_TO_DEV_NULL;
    if (priv->search_path)
        flags |= G_SPAWN_SEARCH_PATH;
    success = g_spawn_async(priv->working_directory,
                            argv,
                            NULL,
                            flags,
                            priv->child_setup,
                            milter,
                            &priv->pid,
                            &internal_error);
    g_strfreev(argv);

    if (!success) {
        milter_utils_set_error_with_sub_error(
            error,
            MILTER_MANAGER_CHILD_ERROR,
            MILTER_MANAGER_CHILD_ERROR_START_FAILURE,
            internal_error,
            "Couldn't start new milter process.");
        return FALSE;
    }

    milter_debug("started %s.",
                 milter_server_context_get_name(MILTER_SERVER_CONTEXT(milter)));
    priv->child_watch_id =
        g_child_watch_add(priv->pid, (GChildWatchFunc)child_watch_func, milter);

    return success;
}

GPid
milter_manager_child_get_pid (MilterManagerChild *milter)
{
    return MILTER_MANAGER_CHILD_GET_PRIVATE(milter)->pid;
}


/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
