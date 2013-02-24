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

#include <milter/core/milter-marshalers.h>
#include "milter-manager-egg.h"
#include "milter-manager-enum-types.h"

#define TIMEOUT_LEEWAY 3
#define DEFAULT_CONNECTION_TIMEOUT \
    (MILTER_SERVER_CONTEXT_DEFAULT_CONNECTION_TIMEOUT - TIMEOUT_LEEWAY)
#define DEFAULT_WRITING_TIMEOUT \
    (MILTER_SERVER_CONTEXT_DEFAULT_WRITING_TIMEOUT - TIMEOUT_LEEWAY)
#define DEFAULT_READING_TIMEOUT \
    (MILTER_SERVER_CONTEXT_DEFAULT_READING_TIMEOUT - TIMEOUT_LEEWAY)
#define DEFAULT_END_OF_MESSAGE_TIMEOUT \
    (MILTER_SERVER_CONTEXT_DEFAULT_END_OF_MESSAGE_TIMEOUT - TIMEOUT_LEEWAY)

#define MILTER_MANAGER_EGG_GET_PRIVATE(obj)                     \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj),                         \
                                 MILTER_TYPE_MANAGER_EGG,       \
                                 MilterManagerEggPrivate))

typedef struct _MilterManagerEggPrivate MilterManagerEggPrivate;
struct _MilterManagerEggPrivate
{
    gchar *name;
    gchar *description;
    gboolean enabled;
    gchar *connection_spec;
    gdouble connection_timeout;
    gdouble writing_timeout;
    gdouble reading_timeout;
    gdouble end_of_message_timeout;
    gchar *user_name;
    gchar *command;
    gchar *command_options;
    GList *applicable_conditions;
    MilterStatus fallback_status;
    gboolean evaluation_mode;
};

enum
{
    PROP_0,
    PROP_NAME,
    PROP_DESCRIPTION,
    PROP_ENABLED,
    PROP_CONNECTION_SPEC,
    PROP_CONNECTION_TIMEOUT,
    PROP_WRITING_TIMEOUT,
    PROP_READING_TIMEOUT,
    PROP_END_OF_MESSAGE_TIMEOUT,
    PROP_USER_NAME,
    PROP_COMMAND,
    PROP_COMMAND_OPTIONS,
    PROP_FALLBACK_STATUS,
    PROP_REPUTATION_MODE
};

enum
{
    HATCHED,
    TO_XML,
    LAST_SIGNAL
};

static gint signals[LAST_SIGNAL] = {0};

MILTER_DEFINE_ERROR_EMITTABLE_TYPE(MilterManagerEgg,
                                   milter_manager_egg,
                                   G_TYPE_OBJECT)

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
milter_manager_egg_class_init (MilterManagerEggClass *klass)
{
    GObjectClass *gobject_class;
    GParamSpec *spec;

    gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->dispose      = dispose;
    gobject_class->set_property = set_property;
    gobject_class->get_property = get_property;

    spec = g_param_spec_string("name",
                               "Name",
                               "The name of the milter egg",
                               NULL,
                               G_PARAM_READWRITE);
    g_object_class_install_property(gobject_class, PROP_NAME, spec);

    spec = g_param_spec_string("description",
                               "Description",
                               "The description of the milter egg",
                               NULL,
                               G_PARAM_READWRITE);
    g_object_class_install_property(gobject_class, PROP_DESCRIPTION, spec);

    spec = g_param_spec_boolean("enabled",
                                "Enabled",
                                "Whether the milter egg is enabled or not",
                                TRUE,
                                G_PARAM_READWRITE);
    g_object_class_install_property(gobject_class, PROP_ENABLED, spec);

    spec = g_param_spec_string("connection-spec",
                               "Connection spec",
                               "The connection spec of the milter egg",
                               NULL,
                               G_PARAM_READABLE);
    g_object_class_install_property(gobject_class, PROP_CONNECTION_SPEC, spec);

    spec = g_param_spec_double("connection-timeout",
                               "Connection timeout",
                               "The connection timeout of the milter egg",
                               0,
                               G_MAXDOUBLE,
                               DEFAULT_CONNECTION_TIMEOUT,
                               G_PARAM_READWRITE);
    g_object_class_install_property(gobject_class, PROP_CONNECTION_TIMEOUT,
                                    spec);

    spec = g_param_spec_double("writing-timeout",
                               "Writing timeout",
                               "The writing timeout of the milter egg",
                               0,
                               G_MAXDOUBLE,
                               DEFAULT_WRITING_TIMEOUT,
                               G_PARAM_READWRITE);
    g_object_class_install_property(gobject_class, PROP_WRITING_TIMEOUT,
                                    spec);

    spec = g_param_spec_double("reading-timeout",
                               "Reading timeout",
                               "The reading timeout of the milter egg",
                               0,
                               G_MAXDOUBLE,
                               DEFAULT_READING_TIMEOUT,
                               G_PARAM_READWRITE);
    g_object_class_install_property(gobject_class, PROP_READING_TIMEOUT,
                                    spec);

    spec = g_param_spec_double("end-of-message-timeout",
                               "End of message timeout",
                               "The end-of-message timeout of the milter egg",
                               0,
                               G_MAXDOUBLE,
                               DEFAULT_END_OF_MESSAGE_TIMEOUT,
                               G_PARAM_READWRITE);
    g_object_class_install_property(gobject_class, PROP_END_OF_MESSAGE_TIMEOUT,
                                    spec);

    spec = g_param_spec_string("user-name",
                               "User name",
                               "The user name of a egged milter",
                               NULL,
                               G_PARAM_READWRITE);
    g_object_class_install_property(gobject_class, PROP_USER_NAME, spec);

    spec = g_param_spec_string("command",
                               "Command",
                               "The command of the milter egg",
                               NULL,
                               G_PARAM_READWRITE);
    g_object_class_install_property(gobject_class, PROP_COMMAND, spec);

    spec = g_param_spec_string("command-options",
                               "Command options",
                               "The command options string of the milter egg",
                               NULL,
                               G_PARAM_READWRITE);
    g_object_class_install_property(gobject_class, PROP_COMMAND_OPTIONS, spec);

    spec = g_param_spec_enum("fallback-status",
                             "Fallback status",
                             "Fallback status on error",
                             MILTER_TYPE_STATUS,
                             MILTER_STATUS_ACCEPT,
                             G_PARAM_READWRITE);
    g_object_class_install_property(gobject_class, PROP_FALLBACK_STATUS, spec);

    spec = g_param_spec_boolean("evaluation-mode",
                                "Reputation mode",
                                "Whether the milter's result is ignored or not",
                                FALSE,
                                G_PARAM_READWRITE);
    g_object_class_install_property(gobject_class, PROP_REPUTATION_MODE, spec);

    signals[HATCHED] =
        g_signal_new("hatched",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterManagerEggClass, hatched),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__OBJECT,
                     G_TYPE_NONE, 1, MILTER_TYPE_MANAGER_CHILD);

    signals[TO_XML] =
        g_signal_new("to-xml",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterManagerEggClass, to_xml),
                     NULL, NULL,
                     _milter_marshal_VOID__POINTER_UINT,
                     G_TYPE_NONE, 2, G_TYPE_POINTER, G_TYPE_UINT);


    g_type_class_add_private(gobject_class,
                             sizeof(MilterManagerEggPrivate));
}

static void
milter_manager_egg_init (MilterManagerEgg *egg)
{
    MilterManagerEggPrivate *priv;

    priv = MILTER_MANAGER_EGG_GET_PRIVATE(egg);
    priv->name = NULL;
    priv->description = NULL;
    priv->enabled = TRUE;
    priv->connection_spec = NULL;
    priv->connection_timeout = DEFAULT_CONNECTION_TIMEOUT;
    priv->writing_timeout = DEFAULT_WRITING_TIMEOUT;
    priv->reading_timeout = DEFAULT_READING_TIMEOUT;
    priv->end_of_message_timeout = DEFAULT_END_OF_MESSAGE_TIMEOUT;
    priv->user_name = NULL;
    priv->command = NULL;
    priv->command_options = NULL;
    priv->applicable_conditions = NULL;
    priv->fallback_status = MILTER_STATUS_ACCEPT;
    priv->evaluation_mode = FALSE;
}

static void
dispose (GObject *object)
{
    MilterManagerEgg *egg;
    MilterManagerEggPrivate *priv;

    egg = MILTER_MANAGER_EGG(object);
    priv = MILTER_MANAGER_EGG_GET_PRIVATE(egg);

    if (priv->name) {
        g_free(priv->name);
        priv->name = NULL;
    }

    if (priv->description) {
        g_free(priv->description);
        priv->description = NULL;
    }

    if (priv->connection_spec) {
        g_free(priv->connection_spec);
        priv->connection_spec = NULL;
    }

    if (priv->user_name) {
        g_free(priv->user_name);
        priv->user_name = NULL;
    }

    if (priv->command) {
        g_free(priv->command);
        priv->command = NULL;
    }

    if (priv->command_options) {
        g_free(priv->command_options);
        priv->command_options = NULL;
    }

    milter_manager_egg_clear_applicable_conditions(egg);

    G_OBJECT_CLASS(milter_manager_egg_parent_class)->dispose(object);
}

static void
set_property (GObject      *object,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
    MilterManagerEgg *egg;
    MilterManagerEggPrivate *priv;

    egg = MILTER_MANAGER_EGG(object);
    priv = MILTER_MANAGER_EGG_GET_PRIVATE(object);

    switch (prop_id) {
    case PROP_NAME:
        milter_manager_egg_set_name(egg, g_value_get_string(value));
        break;
    case PROP_DESCRIPTION:
        milter_manager_egg_set_description(egg, g_value_get_string(value));
        break;
    case PROP_ENABLED:
        milter_manager_egg_set_enabled(egg, g_value_get_boolean(value));
        break;
    case PROP_CONNECTION_TIMEOUT:
        priv->connection_timeout = g_value_get_double(value);
        break;
    case PROP_WRITING_TIMEOUT:
        priv->writing_timeout = g_value_get_double(value);
        break;
    case PROP_READING_TIMEOUT:
        priv->reading_timeout = g_value_get_double(value);
        break;
    case PROP_END_OF_MESSAGE_TIMEOUT:
        priv->end_of_message_timeout = g_value_get_double(value);
        break;
    case PROP_USER_NAME:
        milter_manager_egg_set_user_name(egg, g_value_get_string(value));
        break;
    case PROP_COMMAND:
        milter_manager_egg_set_command(egg, g_value_get_string(value));
        break;
    case PROP_COMMAND_OPTIONS:
        if (priv->command_options)
            g_free(priv->command_options);
        priv->command_options = g_value_dup_string(value);
        break;
    case PROP_FALLBACK_STATUS:
        milter_manager_egg_set_fallback_status(egg, g_value_get_enum(value));
        break;
    case PROP_REPUTATION_MODE:
        milter_manager_egg_set_evaluation_mode(egg, g_value_get_boolean(value));
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
    MilterManagerEggPrivate *priv;

    priv = MILTER_MANAGER_EGG_GET_PRIVATE(object);
    switch (prop_id) {
    case PROP_NAME:
        g_value_set_string(value, priv->name);
        break;
    case PROP_DESCRIPTION:
        g_value_set_string(value, priv->description);
        break;
    case PROP_ENABLED:
        g_value_set_boolean(value, priv->enabled);
        break;
    case PROP_CONNECTION_SPEC:
        g_value_set_string(value, priv->connection_spec);
        break;
    case PROP_CONNECTION_TIMEOUT:
        g_value_set_double(value, priv->connection_timeout);
        break;
    case PROP_WRITING_TIMEOUT:
        g_value_set_double(value, priv->writing_timeout);
        break;
    case PROP_READING_TIMEOUT:
        g_value_set_double(value, priv->reading_timeout);
        break;
    case PROP_END_OF_MESSAGE_TIMEOUT:
        g_value_set_double(value, priv->end_of_message_timeout);
        break;
    case PROP_USER_NAME:
        g_value_set_string(value, priv->user_name);
        break;
    case PROP_COMMAND:
        g_value_set_string(value, priv->command);
        break;
    case PROP_COMMAND_OPTIONS:
        g_value_set_string(value, priv->command_options);
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
milter_manager_egg_error_quark (void)
{
    return g_quark_from_static_string("milter-manager-egg-error-quark");
}

MilterManagerEgg *
milter_manager_egg_new (const gchar *name)
{
    return g_object_new(MILTER_TYPE_MANAGER_EGG,
                        "name", name,
                        NULL);
}

static MilterManagerChild *
hatch (const gchar *first_name, ...)
{
    MilterManagerChild *child;
    va_list args;

    va_start(args, first_name);
    child = milter_manager_child_new_va_list(first_name, args);
    va_end(args);

    return child;
}

MilterManagerChild *
milter_manager_egg_hatch (MilterManagerEgg *egg)
{
    MilterManagerChild *child;
    MilterManagerEggPrivate *priv;

    priv = MILTER_MANAGER_EGG_GET_PRIVATE(egg);

    child = hatch("name", priv->name,
                  "connection-timeout", priv->connection_timeout,
                  "writing-timeout", priv->writing_timeout,
                  "reading-timeout", priv->reading_timeout,
                  "end-of-message-timeout", priv->end_of_message_timeout,
                  "user-name", priv->user_name,
                  "command", priv->command,
                  "command-options", priv->command_options,
                  "fallback-status", priv->fallback_status,
                  "evaluation-mode", priv->evaluation_mode,
                  NULL);

    if (priv->connection_spec) {
        GError *error = NULL;
        MilterServerContext *context;

        context = MILTER_SERVER_CONTEXT(child);
        if (milter_server_context_set_connection_spec(context,
                                                      priv->connection_spec,
                                                      &error)) {
            g_signal_emit(egg, signals[HATCHED], 0, child);
        } else {
            milter_error("[egg][error] invalid connection spec: %s: %s",
                         error->message,
                         priv->name ? priv->name : "(null)");
            g_error_free(error);
        }
    } else {
        milter_error("[egg][error] must set connection spec: %s",
                     priv->name ? priv->name : "(null)");
        g_object_unref(child);
        child = NULL;
    }

    return child;
}

void
milter_manager_egg_set_name (MilterManagerEgg *egg, const gchar *name)
{
    MilterManagerEggPrivate *priv;

    priv = MILTER_MANAGER_EGG_GET_PRIVATE(egg);
    if (priv->name)
        g_free(priv->name);
    priv->name = g_strdup(name);
}

const gchar *
milter_manager_egg_get_name (MilterManagerEgg *egg)
{
    return MILTER_MANAGER_EGG_GET_PRIVATE(egg)->name;
}

void
milter_manager_egg_set_description (MilterManagerEgg *egg,
                                    const gchar *description)
{
    MilterManagerEggPrivate *priv;

    priv = MILTER_MANAGER_EGG_GET_PRIVATE(egg);
    if (priv->description)
        g_free(priv->description);
    priv->description = g_strdup(description);
}

const gchar *
milter_manager_egg_get_description (MilterManagerEgg *egg)
{
    return MILTER_MANAGER_EGG_GET_PRIVATE(egg)->description;
}

void
milter_manager_egg_set_enabled (MilterManagerEgg *egg, gboolean enabled)
{
    MILTER_MANAGER_EGG_GET_PRIVATE(egg)->enabled = enabled;
}

gboolean
milter_manager_egg_is_enabled (MilterManagerEgg *egg)
{
    return MILTER_MANAGER_EGG_GET_PRIVATE(egg)->enabled;
}

gboolean
milter_manager_egg_set_connection_spec (MilterManagerEgg *egg,
                                        const gchar *spec, GError **error)

{
    MilterManagerEggPrivate *priv;
    GError *spec_error = NULL;
    gboolean success = TRUE;
    gint domain;
    struct sockaddr *address = NULL;
    socklen_t address_length;

    priv = MILTER_MANAGER_EGG_GET_PRIVATE(egg);

    if (spec)
        success = milter_connection_parse_spec(spec,
                                               &domain,
                                               &address,
                                               &address_length,
                                               &spec_error);

    if (address)
        g_free(address);

    if (success) {
        if (priv->connection_spec)
            g_free(priv->connection_spec);
        priv->connection_spec = g_strdup(spec);
    } else {
        GError *wrapped_error = NULL;

        milter_utils_set_error_with_sub_error(&wrapped_error,
                                              MILTER_MANAGER_EGG_ERROR,
                                              MILTER_MANAGER_EGG_ERROR_INVALID,
                                              spec_error,
                                              "<%s>: invalid connection spec",
                                              priv->name ? priv->name : "(null)");
        milter_error("[egg][error][set-spec] %s", wrapped_error->message);
        if (error)
            *error = g_error_copy(wrapped_error);
        g_error_free(wrapped_error);
    }

    return success;
}

const gchar *
milter_manager_egg_get_connection_spec (MilterManagerEgg *egg)
{
    return MILTER_MANAGER_EGG_GET_PRIVATE(egg)->connection_spec;
}

void
milter_manager_egg_set_connection_timeout (MilterManagerEgg *egg,
                                           gdouble connection_timeout)
{
    MilterManagerEggPrivate *priv;

    priv = MILTER_MANAGER_EGG_GET_PRIVATE(egg);
    priv->connection_timeout = connection_timeout;
}

gdouble
milter_manager_egg_get_connection_timeout (MilterManagerEgg *egg)
{
    return MILTER_MANAGER_EGG_GET_PRIVATE(egg)->connection_timeout;
}

void
milter_manager_egg_set_writing_timeout (MilterManagerEgg *egg,
                                        gdouble writing_timeout)
{
    MilterManagerEggPrivate *priv;

    priv = MILTER_MANAGER_EGG_GET_PRIVATE(egg);
    priv->writing_timeout = writing_timeout;
}

gdouble
milter_manager_egg_get_writing_timeout (MilterManagerEgg *egg)
{
    return MILTER_MANAGER_EGG_GET_PRIVATE(egg)->writing_timeout;
}

void
milter_manager_egg_set_reading_timeout (MilterManagerEgg *egg,
                                        gdouble reading_timeout)
{
    MilterManagerEggPrivate *priv;

    priv = MILTER_MANAGER_EGG_GET_PRIVATE(egg);
    priv->reading_timeout = reading_timeout;
}

gdouble
milter_manager_egg_get_reading_timeout (MilterManagerEgg *egg)
{
    return MILTER_MANAGER_EGG_GET_PRIVATE(egg)->reading_timeout;
}

void
milter_manager_egg_set_end_of_message_timeout (MilterManagerEgg *egg,
                                               gdouble end_of_message_timeout)
{
    MilterManagerEggPrivate *priv;

    priv = MILTER_MANAGER_EGG_GET_PRIVATE(egg);
    priv->end_of_message_timeout = end_of_message_timeout;
}

gdouble
milter_manager_egg_get_end_of_message_timeout (MilterManagerEgg *egg)
{
    return MILTER_MANAGER_EGG_GET_PRIVATE(egg)->end_of_message_timeout;
}

void
milter_manager_egg_set_user_name (MilterManagerEgg *egg,
                                  const gchar *user_name)
{
    MilterManagerEggPrivate *priv;

    priv = MILTER_MANAGER_EGG_GET_PRIVATE(egg);
    if (priv->user_name)
        g_free(priv->user_name);
    priv->user_name = g_strdup(user_name);
}

const gchar *
milter_manager_egg_get_user_name (MilterManagerEgg *egg)
{
    return MILTER_MANAGER_EGG_GET_PRIVATE(egg)->user_name;
}

void
milter_manager_egg_set_command (MilterManagerEgg *egg,
                                const gchar *command)
{
    MilterManagerEggPrivate *priv;

    priv = MILTER_MANAGER_EGG_GET_PRIVATE(egg);
    if (priv->command)
        g_free(priv->command);
    priv->command = g_strdup(command);
}

const gchar *
milter_manager_egg_get_command (MilterManagerEgg *egg)
{
    return MILTER_MANAGER_EGG_GET_PRIVATE(egg)->command;
}

void
milter_manager_egg_set_command_options (MilterManagerEgg *egg,
                                        const gchar *command_options)
{
    MilterManagerEggPrivate *priv;

    priv = MILTER_MANAGER_EGG_GET_PRIVATE(egg);
    if (priv->command_options)
        g_free(priv->command_options);
    priv->command_options = g_strdup(command_options);
}

const gchar *
milter_manager_egg_get_command_options (MilterManagerEgg *egg)
{
    return MILTER_MANAGER_EGG_GET_PRIVATE(egg)->command_options;
}

void
milter_manager_egg_set_fallback_status (MilterManagerEgg *egg,
                                        MilterStatus      status)
{
    MILTER_MANAGER_EGG_GET_PRIVATE(egg)->fallback_status = status;
}

MilterStatus
milter_manager_egg_get_fallback_status (MilterManagerEgg *egg)
{
    return MILTER_MANAGER_EGG_GET_PRIVATE(egg)->fallback_status;
}

void
milter_manager_egg_set_evaluation_mode (MilterManagerEgg *egg,
                                        gboolean          evaluation_mode)
{
    MILTER_MANAGER_EGG_GET_PRIVATE(egg)->evaluation_mode = evaluation_mode;
}

gboolean
milter_manager_egg_is_evaluation_mode (MilterManagerEgg *egg)
{
    return MILTER_MANAGER_EGG_GET_PRIVATE(egg)->evaluation_mode;
}

void
milter_manager_egg_add_applicable_condition (MilterManagerEgg *egg,
                                             MilterManagerApplicableCondition *condition)
{
    MilterManagerEggPrivate *priv;

    priv = MILTER_MANAGER_EGG_GET_PRIVATE(egg);

    g_object_ref(condition);
    priv->applicable_conditions =
        g_list_append(priv->applicable_conditions, condition);
}

const GList *
milter_manager_egg_get_applicable_conditions (MilterManagerEgg *egg)
{
    return MILTER_MANAGER_EGG_GET_PRIVATE(egg)->applicable_conditions;
}

void
milter_manager_egg_clear_applicable_conditions (MilterManagerEgg *egg)
{
    MilterManagerEggPrivate *priv;

    priv = MILTER_MANAGER_EGG_GET_PRIVATE(egg);
    if (priv->applicable_conditions) {
        g_list_foreach(priv->applicable_conditions, (GFunc)g_object_unref, NULL);
        g_list_free(priv->applicable_conditions);
        priv->applicable_conditions = NULL;
    }
}

void
milter_manager_egg_attach_applicable_conditions (MilterManagerEgg      *egg,
                                                 MilterManagerChild    *child,
                                                 MilterManagerChildren *children,
                                                 MilterClientContext   *context)
{
    MilterManagerEggPrivate *priv;
    GList *node;

    priv = MILTER_MANAGER_EGG_GET_PRIVATE(egg);
    for (node = priv->applicable_conditions; node; node = g_list_next(node)) {
        MilterManagerApplicableCondition *condition = node->data;
        milter_manager_applicable_condition_attach_to(condition,
                                                      child, children, context);
    }
}

gboolean
milter_manager_egg_merge (MilterManagerEgg *egg,
                          MilterManagerEgg *other_egg,
                          GError           **error)
{
    const gchar *connection_spec;
    const gchar *description;
    const gchar *user_name;
    const gchar *command;
    const GList *node;

    connection_spec = milter_manager_egg_get_connection_spec(other_egg);
    if (connection_spec &&
        !milter_manager_egg_set_connection_spec(egg, connection_spec, error))
        return FALSE;

#define MERGE_TIMEOUT(name)                                     \
    milter_manager_egg_set_ ## name ## _timeout(                \
        egg,                                                    \
        milter_manager_egg_get_ ## name ## _timeout(other_egg))

    MERGE_TIMEOUT(connection);
    MERGE_TIMEOUT(writing);
    MERGE_TIMEOUT(reading);
    MERGE_TIMEOUT(end_of_message);

#undef MERGE_TIMEOUT

    description = milter_manager_egg_get_description(other_egg);
    if (description)
        milter_manager_egg_set_description(egg, description);

    milter_manager_egg_set_enabled(egg,
                                   milter_manager_egg_is_enabled(other_egg));

    user_name = milter_manager_egg_get_user_name(other_egg);
    if (user_name)
        milter_manager_egg_set_user_name(egg, user_name);

    command = milter_manager_egg_get_command(other_egg);
    if (command) {
        const gchar *command_options;

        milter_manager_egg_set_command(egg, command);
        command_options = milter_manager_egg_get_command_options(other_egg);
        milter_manager_egg_set_command_options(egg, command_options);
    }

    milter_manager_egg_clear_applicable_conditions(egg);
    node = milter_manager_egg_get_applicable_conditions(other_egg);
    for (; node; node = g_list_next(node)) {
        MilterManagerApplicableCondition *condition = node->data;
        milter_manager_egg_add_applicable_condition(egg, condition);
    }

    return TRUE;
}

gchar *
milter_manager_egg_to_xml (MilterManagerEgg *egg)
{
    GString *string;

    string = g_string_new(NULL);
    milter_manager_egg_to_xml_string(egg, string, 0);
    return g_string_free(string, FALSE);
}

void
milter_manager_egg_to_xml_string (MilterManagerEgg *egg,
                                  GString *string, guint indent)
{
    MilterManagerEggPrivate *priv;

    priv = MILTER_MANAGER_EGG_GET_PRIVATE(egg);

    milter_utils_append_indent(string, indent);
    g_string_append(string, "<milter>\n");

    if (priv->name)
        milter_utils_xml_append_text_element(string,
                                             "name", priv->name,
                                             indent + 2);
    if (priv->description)
        milter_utils_xml_append_text_element(string,
                                             "description", priv->description,
                                             indent + 2);
    milter_utils_xml_append_boolean_element(string,
                                            "enabled", priv->enabled,
                                            indent + 2);
    milter_utils_xml_append_enum_element(string,
                                         "fallback-status",
                                         MILTER_TYPE_STATUS,
                                         priv->fallback_status,
                                         indent + 2);
    milter_utils_xml_append_boolean_element(string,
                                            "evaluation-mode",
                                            priv->evaluation_mode,
                                            indent + 2);
    if (priv->connection_spec)
        milter_utils_xml_append_text_element(string,
                                             "connection-spec",
                                             priv->connection_spec,
                                             indent + 2);

    if (priv->command)
        milter_utils_xml_append_text_element(string,
                                             "command", priv->command,
                                             indent + 2);
    if (priv->command_options)
        milter_utils_xml_append_text_element(string,
                                             "command-options",
                                             priv->command_options,
                                             indent + 2);

    if (priv->applicable_conditions) {
        GList *node = priv->applicable_conditions;

        milter_utils_append_indent(string, indent + 2);
        g_string_append(string, "<applicable-conditions>\n");
        for (; node; node = g_list_next(node)) {
            MilterManagerApplicableCondition *condition = node->data;
            const gchar *name;

            name = milter_manager_applicable_condition_get_name(condition);
            milter_utils_xml_append_text_element(string,
                                                 "applicable-condition",
                                                 name,
                                                 indent + 4);
        }
        milter_utils_append_indent(string, indent + 2);
        g_string_append(string, "</applicable-conditions>\n");
    }

    g_signal_emit(egg, signals[TO_XML], 0, string, indent + 2);

    milter_utils_append_indent(string, indent);
    g_string_append(string, "</milter>\n");
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
