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

#include "milter-manager-spawn.h"
#include "milter-manager-enum-types.h"

#define MILTER_MANAGER_SPAWN_GET_PRIVATE(obj)                   \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj),                         \
                                 MILTER_TYPE_MANAGER_SPAWN,     \
                                 MilterManagerSpawnPrivate))

typedef struct _MilterManagerSpawnPrivate MilterManagerSpawnPrivate;
struct _MilterManagerSpawnPrivate
{
    gchar *name;
    guint connection_timeout;
    guint writing_timeout;
    guint reading_timeout;
    guint end_of_message_timeout;
};

enum
{
    PROP_0,
    PROP_NAME,
    PROP_CONNECTION_TIMEOUT,
    PROP_WRITING_TIMEOUT,
    PROP_READING_TIMEOUT,
    PROP_END_OF_MESSAGE_TIMEOUT
};

MILTER_DEFINE_ERROR_EMITABLE_TYPE(MilterManagerSpawn,
                                  milter_manager_spawn,
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
milter_manager_spawn_class_init (MilterManagerSpawnClass *klass)
{
    GObjectClass *gobject_class;
    GParamSpec *spec;

    gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->dispose      = dispose;
    gobject_class->set_property = set_property;
    gobject_class->get_property = get_property;

    spec = g_param_spec_string("name",
                               "Name",
                               "The name of the milter spawn",
                               NULL,
                               G_PARAM_READWRITE);
    g_object_class_install_property(gobject_class, PROP_NAME, spec);

    spec = g_param_spec_uint("connection-timeout",
                             "Connection timeout",
                             "The connection timeout of the milter spawn",
                             0, G_MAXUINT, 0,
                             G_PARAM_READWRITE);
    g_object_class_install_property(gobject_class, PROP_CONNECTION_TIMEOUT,
                                    spec);

    spec = g_param_spec_uint("writing-timeout",
                             "Writing timeout",
                             "The writing timeout of the milter spawn",
                             0, G_MAXUINT, 0,
                             G_PARAM_READWRITE);
    g_object_class_install_property(gobject_class, PROP_WRITING_TIMEOUT,
                                    spec);

    spec = g_param_spec_uint("reading-timeout",
                             "Reading timeout",
                             "The reading timeout of the milter spawn",
                             0, G_MAXUINT, 0,
                             G_PARAM_READWRITE);
    g_object_class_install_property(gobject_class, PROP_READING_TIMEOUT,
                                    spec);

    spec = g_param_spec_uint("end-of-message-timeout",
                             "End of message timeout",
                             "The end-of-message timeout of the milter spawn",
                             0, G_MAXUINT, 0,
                             G_PARAM_READWRITE);
    g_object_class_install_property(gobject_class, PROP_END_OF_MESSAGE_TIMEOUT,
                                    spec);

    g_type_class_add_private(gobject_class,
                             sizeof(MilterManagerSpawnPrivate));
}

static void
milter_manager_spawn_init (MilterManagerSpawn *spawn)
{
    MilterManagerSpawnPrivate *priv;

    priv = MILTER_MANAGER_SPAWN_GET_PRIVATE(spawn);
    priv->name = NULL;
    priv->connection_timeout = 0;
    priv->writing_timeout = 0;
    priv->reading_timeout = 0;
    priv->end_of_message_timeout = 0;
}

static void
dispose (GObject *object)
{
    MilterManagerSpawnPrivate *priv;

    priv = MILTER_MANAGER_SPAWN_GET_PRIVATE(object);

    if (priv->name) {
        g_free(priv->name);
        priv->name = NULL;
    }

    G_OBJECT_CLASS(milter_manager_spawn_parent_class)->dispose(object);
}

static void
set_property (GObject      *object,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
    MilterManagerSpawn *spawn;
    MilterManagerSpawnPrivate *priv;

    spawn = MILTER_MANAGER_SPAWN(object);
    priv = MILTER_MANAGER_SPAWN_GET_PRIVATE(object);

    switch (prop_id) {
      case PROP_NAME:
        milter_manager_spawn_set_name(spawn, g_value_get_string(value));
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
    MilterManagerSpawnPrivate *priv;

    priv = MILTER_MANAGER_SPAWN_GET_PRIVATE(object);
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
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

MilterManagerSpawn *
milter_manager_spawn_new (const gchar *name)
{
    return g_object_new(MILTER_TYPE_MANAGER_SPAWN,
                        "name", name,
                        NULL);
}

void
milter_manager_spawn_set_name (MilterManagerSpawn *spawn, const gchar *name)
{
    MilterManagerSpawnPrivate *priv;

    priv = MILTER_MANAGER_SPAWN_GET_PRIVATE(spawn);
    if (priv->name)
        g_free(priv->name);
    priv->name = g_strdup(name);
}

const gchar *
milter_manager_spawn_get_name (MilterManagerSpawn *spawn)
{
    return MILTER_MANAGER_SPAWN_GET_PRIVATE(spawn)->name;
}

void
milter_manager_spawn_set_connection_timeout (MilterManagerSpawn *spawn,
                                             guint connection_timeout)
{
    MilterManagerSpawnPrivate *priv;

    priv = MILTER_MANAGER_SPAWN_GET_PRIVATE(spawn);
    priv->connection_timeout = connection_timeout;
}

guint
milter_manager_spawn_get_connection_timeout (MilterManagerSpawn *spawn)
{
    return MILTER_MANAGER_SPAWN_GET_PRIVATE(spawn)->connection_timeout;
}

void
milter_manager_spawn_set_writing_timeout (MilterManagerSpawn *spawn,
                                          guint writing_timeout)
{
    MilterManagerSpawnPrivate *priv;

    priv = MILTER_MANAGER_SPAWN_GET_PRIVATE(spawn);
    priv->writing_timeout = writing_timeout;
}

guint
milter_manager_spawn_get_writing_timeout (MilterManagerSpawn *spawn)
{
    return MILTER_MANAGER_SPAWN_GET_PRIVATE(spawn)->writing_timeout;
}

void
milter_manager_spawn_set_reading_timeout (MilterManagerSpawn *spawn,
                                          guint reading_timeout)
{
    MilterManagerSpawnPrivate *priv;

    priv = MILTER_MANAGER_SPAWN_GET_PRIVATE(spawn);
    priv->reading_timeout = reading_timeout;
}

guint
milter_manager_spawn_get_reading_timeout (MilterManagerSpawn *spawn)
{
    return MILTER_MANAGER_SPAWN_GET_PRIVATE(spawn)->reading_timeout;
}

void
milter_manager_spawn_set_end_of_message_timeout (MilterManagerSpawn *spawn,
                                                 guint end_of_message_timeout)
{
    MilterManagerSpawnPrivate *priv;

    priv = MILTER_MANAGER_SPAWN_GET_PRIVATE(spawn);
    priv->end_of_message_timeout = end_of_message_timeout;
}

guint
milter_manager_spawn_get_end_of_message_timeout (MilterManagerSpawn *spawn)
{
    return MILTER_MANAGER_SPAWN_GET_PRIVATE(spawn)->end_of_message_timeout;
}


/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
