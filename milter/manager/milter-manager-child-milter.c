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
#include "milter-manager-child-milter.h"

#define MILTER_MANAGER_CHILD_MILTER_GET_PRIVATE(obj)                    \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj),                                 \
                                 MILTER_MANAGER_TYPE_CHILD_MILTER,      \
                                 MilterManagerChildMilterPrivate))

typedef struct _MilterManagerChildMilterPrivate	MilterManagerChildMilterPrivate;
struct _MilterManagerChildMilterPrivate
{
    gchar *name;
    gchar *user;
    gchar *group;
    gchar *working_directory;
    gchar *command;
    gchar *command_options;
    GSpawnChildSetupFunc child_setup;
    guint child_watch_id;
};

enum
{
    PROP_0,
    PROP_NAME
};

G_DEFINE_TYPE(MilterManagerChildMilter, milter_manager_child_milter,
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
milter_manager_child_milter_class_init (MilterManagerChildMilterClass *klass)
{
    GObjectClass *gobject_class;
    GParamSpec *spec;

    gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->dispose      = dispose;
    gobject_class->set_property = set_property;
    gobject_class->get_property = get_property;

    spec = g_param_spec_string("name",
                               "Name",
                               "The name of the child milter",
                               NULL,
                               G_PARAM_READWRITE);
    g_object_class_install_property(gobject_class, PROP_NAME, spec);

    g_type_class_add_private(gobject_class,
                             sizeof(MilterManagerChildMilterPrivate));
}

static void
milter_manager_child_milter_init (MilterManagerChildMilter *milter)
{
    MilterManagerChildMilterPrivate *priv;

    priv = MILTER_MANAGER_CHILD_MILTER_GET_PRIVATE(milter);
    priv->name = NULL;
    priv->user = NULL;
    priv->group = NULL;
    priv->working_directory = NULL;
    priv->command = NULL;
    priv->command_options = NULL;
    priv->child_setup = NULL;
    priv->child_watch_id = 0;
}

static void
dispose (GObject *object)
{
    MilterManagerChildMilterPrivate *priv;

    priv = MILTER_MANAGER_CHILD_MILTER_GET_PRIVATE(object);

    if (priv->name) {
        g_free(priv->name);
        priv->name = NULL;
    }

    if (priv->user) {
        g_free(priv->user);
        priv->user = NULL;
    }

    if (priv->group) {
        g_free(priv->group);
        priv->group = NULL;
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

    if (priv->child_watch_id > 0) {
        g_source_remove(priv->child_watch_id);
        priv->child_watch_id = 0;
    }

    G_OBJECT_CLASS(milter_manager_child_milter_parent_class)->dispose(object);
}

static void
set_property (GObject      *object,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
    MilterManagerChildMilterPrivate *priv;

    priv = MILTER_MANAGER_CHILD_MILTER_GET_PRIVATE(object);
    switch (prop_id) {
      case PROP_NAME:
        if (priv->name)
            g_free(priv->name);
        priv->name = g_value_dup_string(value);
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
    MilterManagerChildMilterPrivate *priv;

    priv = MILTER_MANAGER_CHILD_MILTER_GET_PRIVATE(object);
    switch (prop_id) {
      case PROP_NAME:
        g_value_set_string(value, priv->name);
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

GQuark
milter_manager_child_milter_error_quark (void)
{
    return g_quark_from_static_string("milter-manager-child-milter-error-quark");
}

MilterManagerChildMilter *
milter_manager_child_milter_new (const gchar *name)
{
    return g_object_new(MILTER_MANAGER_TYPE_CHILD_MILTER,
                        "name", name,
                        NULL);
}

static void
child_watch_func (GPid pid, gint status, gpointer user_data)
{
    MilterManagerChildMilterPrivate *priv;

    priv = MILTER_MANAGER_CHILD_MILTER_GET_PRIVATE(user_data);

    if (WCOREDUMP(status)) {
        milter_critical("%s produced a core dump", priv->name);
        /* restart? */
    } else if (WIFEXITED(status)) {
        milter_warning("%s exits with status: %d", priv->name, status);
        /* restart? */
    }
}

static void
set_error_message (GError **error,
                   gint error_code,
                   GError *sub_error,
                   const gchar *format,
                   ...)
{
    GString *message;
    va_list var_args;

    message = g_string_new(NULL);

    va_start(var_args, format);
    g_string_append_vprintf(message, format, var_args);
    va_end(var_args);

    if (sub_error) {
        g_string_append_printf(message,
                               ": %s:%d",
                               g_quark_to_string(sub_error->domain),
                               sub_error->code);
        if (sub_error->message)
            g_string_append_printf(message, ": %s", sub_error->message);

        g_error_free(sub_error);
    }

    g_set_error_literal(error, 
                        MILTER_MANAGER_CHILD_MILTER_ERROR,
                        error_code, message->str);
    g_string_free(message, TRUE);
}

gboolean
milter_manager_child_milter_start (MilterManagerChildMilter *milter,
                                   GError **error)
{
    GPid pid;
    gint argc;
    gchar **argv;
    gchar *command_line;
    gboolean success;
    MilterManagerChildMilterPrivate *priv;
    GError *internal_error = NULL;
    struct passwd *password;

    priv = MILTER_MANAGER_CHILD_MILTER_GET_PRIVATE(milter);

    command_line = g_strdup_printf("%s %s", priv->command, priv->command_options);
    success = g_shell_parse_argv(command_line,
                                 &argc, &argv,
                                 &internal_error);
    g_free(command_line);

    if (!success) {
        set_error_message(error,
                          MILTER_MANAGER_CHILD_MILTER_ERROR_BAD_COMMAND_STRING,
                          internal_error,
                          "Command string has invalid character(s).");
        return FALSE;
    }

    password = getpwnam(priv->user);
    if (!password) {
        set_error_message(error,
                          MILTER_MANAGER_CHILD_MILTER_ERROR_INVALID_USER_NAME,
                          NULL,
                          "Couldn't start new milter process. :%s",
                          strerror(errno));
        return FALSE;
    }

    if (setregid(-1, password->pw_gid) == -1) {
        set_error_message(error,
                          MILTER_MANAGER_CHILD_MILTER_ERROR_START_FAILURE,
                          NULL,
                          "Couldn't start new milter process. :%s",
                          strerror(errno));
        return FALSE;
    }

    if (setreuid(-1, password->pw_uid) == -1) {
        set_error_message(error,
                          MILTER_MANAGER_CHILD_MILTER_ERROR_START_FAILURE,
                          NULL,
                          "Couldn't start new milter process. :%s",
                          strerror(errno));
        return FALSE;
    }

    success = g_spawn_async(priv->working_directory,
                            argv,
                            NULL,
                            0,
                            priv->child_setup,
                            milter,
                            &pid,
                            &internal_error);
    g_strfreev(argv);

    if (!success) {
        set_error_message(error,
                          MILTER_MANAGER_CHILD_MILTER_ERROR_START_FAILURE,
                          internal_error,
                          "Couldn't start new milter process.");
        return FALSE;
    }
    priv->child_watch_id = 
        g_child_watch_add(pid, (GChildWatchFunc)child_watch_func, NULL);

    return success;
}

#if 0
MilterStatus
milter_manager_child_milter_option_negotiate (MilterManagerChildMilter *milter,
                                              MilterOption             *option)
{
    return MILTER_STATUS_DEFAULT;
}

MilterStatus
milter_manager_child_milter_connect (MilterManagerChildMilter *milter,
                                     const gchar              *host_name,
                                     struct sockaddr          *address,
                                     socklen_t                 address_length)
{
    return MILTER_STATUS_DEFAULT;
}

MilterStatus
milter_manager_child_milter_helo (MilterManagerChildMilter *milter,
                                  const gchar              *fqdn)
{
    return MILTER_STATUS_DEFAULT;
}

MilterStatus
milter_manager_child_milter_envelope_from (MilterManagerChildMilter *milter,
                                           const gchar              *from)
{
    return MILTER_STATUS_DEFAULT;
}

MilterStatus
milter_manager_child_milter_envelope_receipt (MilterManagerChildMilter *milter,
                                              const gchar              *receipt)
{
    return MILTER_STATUS_DEFAULT;
}

MilterStatus
milter_manager_child_milter_data (MilterManagerChildMilter *milter)
{
    return MILTER_STATUS_DEFAULT;
}

MilterStatus
milter_manager_child_milter_unknown (MilterManagerChildMilter *milter,
                                     const gchar              *command)
{
    return MILTER_STATUS_DEFAULT;
}

MilterStatus
milter_manager_child_milter_header (MilterManagerChildMilter *milter,
                                    const gchar              *name,
                                    const gchar              *value)
{
    return MILTER_STATUS_DEFAULT;
}

MilterStatus
milter_manager_child_milter_end_of_header (MilterManagerChildMilter *milter)
{
    return MILTER_STATUS_DEFAULT;
}

MilterStatus
milter_manager_child_milter_body (MilterManagerChildMilter *milter,
                                  const guchar             *chunk,
                                  gsize                     size)
{
    return MILTER_STATUS_DEFAULT;
}

MilterStatus
milter_manager_child_milter_end_of_message (MilterManagerChildMilter *milter)
{
    return MILTER_STATUS_DEFAULT;
}

MilterStatus
milter_manager_child_milter_quit (MilterManagerChildMilter *milter)
{
    return MILTER_STATUS_DEFAULT;
}

MilterStatus
milter_manager_child_milter_abort (MilterManagerChildMilter *milter)
{
    return MILTER_STATUS_DEFAULT;
}
#endif

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
