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

#include "milter-manager-egg.h"
#include "milter-manager-enum-types.h"

#define MILTER_MANAGER_EGG_GET_PRIVATE(obj)                     \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj),                         \
                                 MILTER_TYPE_MANAGER_EGG,       \
                                 MilterManagerEggPrivate))

typedef struct _MilterManagerEggPrivate MilterManagerEggPrivate;
struct _MilterManagerEggPrivate
{
    gchar *name;
    guint connection_timeout;
    guint writing_timeout;
    guint reading_timeout;
    guint end_of_message_timeout;
    gchar *user_name;
    gchar *command;
};

enum
{
    PROP_0,
    PROP_NAME,
    PROP_CONNECTION_TIMEOUT,
    PROP_WRITING_TIMEOUT,
    PROP_READING_TIMEOUT,
    PROP_END_OF_MESSAGE_TIMEOUT,
    PROP_USER_NAME,
    PROP_COMMAND
};

MILTER_DEFINE_ERROR_EMITABLE_TYPE(MilterManagerEgg,
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

    spec = g_param_spec_uint("connection-timeout",
                             "Connection timeout",
                             "The connection timeout of the milter egg",
                             0, G_MAXUINT, 0,
                             G_PARAM_READWRITE);
    g_object_class_install_property(gobject_class, PROP_CONNECTION_TIMEOUT,
                                    spec);

    spec = g_param_spec_uint("writing-timeout",
                             "Writing timeout",
                             "The writing timeout of the milter egg",
                             0, G_MAXUINT, 0,
                             G_PARAM_READWRITE);
    g_object_class_install_property(gobject_class, PROP_WRITING_TIMEOUT,
                                    spec);

    spec = g_param_spec_uint("reading-timeout",
                             "Reading timeout",
                             "The reading timeout of the milter egg",
                             0, G_MAXUINT, 0,
                             G_PARAM_READWRITE);
    g_object_class_install_property(gobject_class, PROP_READING_TIMEOUT,
                                    spec);

    spec = g_param_spec_uint("end-of-message-timeout",
                             "End of message timeout",
                             "The end-of-message timeout of the milter egg",
                             0, G_MAXUINT, 0,
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

    g_type_class_add_private(gobject_class,
                             sizeof(MilterManagerEggPrivate));
}

static void
milter_manager_egg_init (MilterManagerEgg *egg)
{
    MilterManagerEggPrivate *priv;

    priv = MILTER_MANAGER_EGG_GET_PRIVATE(egg);
    priv->name = NULL;
    priv->connection_timeout = 0;
    priv->writing_timeout = 0;
    priv->reading_timeout = 0;
    priv->end_of_message_timeout = 0;
    priv->user_name = NULL;
    priv->command = NULL;
}

static void
dispose (GObject *object)
{
    MilterManagerEggPrivate *priv;

    priv = MILTER_MANAGER_EGG_GET_PRIVATE(object);

    if (priv->name) {
        g_free(priv->name);
        priv->name = NULL;
    }

    if (priv->user_name) {
        g_free(priv->user_name);
        priv->user_name = NULL;
    }

    if (priv->command) {
        g_free(priv->command);
        priv->command = NULL;
    }

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
      case PROP_CONNECTION_TIMEOUT:
        priv->connection_timeout = g_value_get_uint(value);
        break;
      case PROP_WRITING_TIMEOUT:
        priv->writing_timeout = g_value_get_uint(value);
        break;
      case PROP_READING_TIMEOUT:
        priv->reading_timeout = g_value_get_uint(value);
        break;
      case PROP_END_OF_MESSAGE_TIMEOUT:
        priv->end_of_message_timeout = g_value_get_uint(value);
        break;
      case PROP_USER_NAME:
        milter_manager_egg_set_user_name(egg, g_value_get_string(value));
        break;
      case PROP_COMMAND:
        milter_manager_egg_set_command(egg, g_value_get_string(value));
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
      case PROP_CONNECTION_TIMEOUT:
        g_value_set_uint(value, priv->connection_timeout);
        break;
      case PROP_WRITING_TIMEOUT:
        g_value_set_uint(value, priv->writing_timeout);
        break;
      case PROP_READING_TIMEOUT:
        g_value_set_uint(value, priv->reading_timeout);
        break;
      case PROP_END_OF_MESSAGE_TIMEOUT:
        g_value_set_uint(value, priv->end_of_message_timeout);
        break;
      case PROP_USER_NAME:
        g_value_set_string(value, priv->user_name);
        break;
      case PROP_COMMAND:
        g_value_set_string(value, priv->command);
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

MilterManagerEgg *
milter_manager_egg_new (const gchar *name)
{
    return g_object_new(MILTER_TYPE_MANAGER_EGG,
                        "name", name,
                        NULL);
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
milter_manager_egg_set_connection_timeout (MilterManagerEgg *egg,
                                             guint connection_timeout)
{
    MilterManagerEggPrivate *priv;

    priv = MILTER_MANAGER_EGG_GET_PRIVATE(egg);
    priv->connection_timeout = connection_timeout;
}

guint
milter_manager_egg_get_connection_timeout (MilterManagerEgg *egg)
{
    return MILTER_MANAGER_EGG_GET_PRIVATE(egg)->connection_timeout;
}

void
milter_manager_egg_set_writing_timeout (MilterManagerEgg *egg,
                                          guint writing_timeout)
{
    MilterManagerEggPrivate *priv;

    priv = MILTER_MANAGER_EGG_GET_PRIVATE(egg);
    priv->writing_timeout = writing_timeout;
}

guint
milter_manager_egg_get_writing_timeout (MilterManagerEgg *egg)
{
    return MILTER_MANAGER_EGG_GET_PRIVATE(egg)->writing_timeout;
}

void
milter_manager_egg_set_reading_timeout (MilterManagerEgg *egg,
                                          guint reading_timeout)
{
    MilterManagerEggPrivate *priv;

    priv = MILTER_MANAGER_EGG_GET_PRIVATE(egg);
    priv->reading_timeout = reading_timeout;
}

guint
milter_manager_egg_get_reading_timeout (MilterManagerEgg *egg)
{
    return MILTER_MANAGER_EGG_GET_PRIVATE(egg)->reading_timeout;
}

void
milter_manager_egg_set_end_of_message_timeout (MilterManagerEgg *egg,
                                                 guint end_of_message_timeout)
{
    MilterManagerEggPrivate *priv;

    priv = MILTER_MANAGER_EGG_GET_PRIVATE(egg);
    priv->end_of_message_timeout = end_of_message_timeout;
}

guint
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

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
