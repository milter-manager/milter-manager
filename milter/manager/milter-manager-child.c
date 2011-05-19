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
    MilterStatus fallback_status;
    gboolean evaluation_mode;
};

enum
{
    PROP_0,
    PROP_USER_NAME,
    PROP_COMMAND,
    PROP_COMMAND_OPTIONS,
    PROP_WORKING_DIRECTORY,
    PROP_SEARCH_PATH,
    PROP_FALLBACK_STATUS,
    PROP_REPUTATION_MODE
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

    spec = g_param_spec_enum("fallback-status",
                             "Fallback status",
                             "Fallback status on error",
                             MILTER_TYPE_STATUS,
                             MILTER_STATUS_ACCEPT,
                             G_PARAM_READWRITE);
    g_object_class_install_property(gobject_class, PROP_FALLBACK_STATUS, spec);

    spec = g_param_spec_boolean("evaluation-mode",
                                "Reputation mode",
                                "Whether the result of the child is used or not",
                                FALSE,
                                G_PARAM_READWRITE);
    g_object_class_install_property(gobject_class, PROP_REPUTATION_MODE, spec);

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
    priv->fallback_status = MILTER_STATUS_ACCEPT;
    priv->evaluation_mode = FALSE;
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

    G_OBJECT_CLASS(milter_manager_child_parent_class)->dispose(object);
}

static void
set_property (GObject      *object,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
    MilterManagerChildPrivate *priv;

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
    case PROP_FALLBACK_STATUS:
        priv->fallback_status = g_value_get_enum(value);
        break;
    case PROP_REPUTATION_MODE:
        priv->evaluation_mode = g_value_get_boolean(value);
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
    case PROP_FALLBACK_STATUS:
        g_value_set_enum(value, priv->fallback_status);
        break;
    case PROP_REPUTATION_MODE:
        g_value_set_boolean(value, priv->evaluation_mode);
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

gchar *
milter_manager_child_get_command_line_string (MilterManagerChild *milter)
{
    MilterManagerChildPrivate *priv;
    gchar *command_line;

    priv = MILTER_MANAGER_CHILD_GET_PRIVATE(milter);

    if (!priv->command)
        return NULL;

    if (priv->command_options)
        command_line = g_strdup_printf("%s %s",
                                       priv->command, priv->command_options);
    else
        command_line = g_strdup(priv->command);

    return command_line;
}

gchar *
milter_manager_child_get_user_name (MilterManagerChild *milter)
{
    return MILTER_MANAGER_CHILD_GET_PRIVATE(milter)->user_name;
}

MilterStatus
milter_manager_child_get_fallback_status (MilterManagerChild *milter)
{
    return MILTER_MANAGER_CHILD_GET_PRIVATE(milter)->fallback_status;
}

void
milter_manager_child_set_evaluation_mode (MilterManagerChild *milter,
                                          gboolean evaluation_mode)
{
    MILTER_MANAGER_CHILD_GET_PRIVATE(milter)->evaluation_mode = evaluation_mode;
}

gboolean
milter_manager_child_is_evaluation_mode (MilterManagerChild *milter)
{
    return MILTER_MANAGER_CHILD_GET_PRIVATE(milter)->evaluation_mode;
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
