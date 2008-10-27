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

#ifndef __MILTER_MANAGER_SPAWN_H__
#define __MILTER_MANAGER_SPAWN_H__

#include <glib-object.h>

#include <milter/client.h>
#include <milter/server.h>

G_BEGIN_DECLS

#define MILTER_TYPE_MANAGER_SPAWN            (milter_manager_spawn_get_type())
#define MILTER_MANAGER_SPAWN(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), MILTER_TYPE_MANAGER_SPAWN, MilterManagerSpawn))
#define MILTER_MANAGER_SPAWN_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), MILTER_TYPE_MANAGER_SPAWN, MilterManagerSpawnClass))
#define MILTER_MANAGER_IS_SPAWN(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), MILTER_TYPE_MANAGER_SPAWN))
#define MILTER_MANAGER_IS_SPAWN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), MILTER_TYPE_MANAGER_SPAWN))
#define MILTER_MANAGER_SPAWN_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), MILTER_TYPE_MANAGER_SPAWN, MilterManagerSpawnClass))

typedef struct _MilterManagerSpawn         MilterManagerSpawn;
typedef struct _MilterManagerSpawnClass    MilterManagerSpawnClass;

struct _MilterManagerSpawn
{
    GObject object;
};

struct _MilterManagerSpawnClass
{
    GObjectClass parent_class;
};

GType               milter_manager_spawn_get_type (void) G_GNUC_CONST;

MilterManagerSpawn *milter_manager_spawn_new      (const gchar *name);

void                milter_manager_spawn_set_name (MilterManagerSpawn *spawn,
                                                   const gchar *name);
const gchar        *milter_manager_spawn_get_name (MilterManagerSpawn *spawn);
void                milter_manager_spawn_set_connection_timeout
                                                  (MilterManagerSpawn *spawn,
                                                   guint connection_timeout);
guint               milter_manager_spawn_get_connection_timeout
                                                  (MilterManagerSpawn *spawn);
void                milter_manager_spawn_set_writing_timeout
                                                  (MilterManagerSpawn *spawn,
                                                   guint writing_timeout);
guint               milter_manager_spawn_get_writing_timeout
                                                  (MilterManagerSpawn *spawn);
void                milter_manager_spawn_set_reading_timeout
                                                  (MilterManagerSpawn *spawn,
                                                   guint reading_timeout);
guint               milter_manager_spawn_get_reading_timeout
                                                  (MilterManagerSpawn *spawn);
void                milter_manager_spawn_set_end_of_message_timeout
                                                  (MilterManagerSpawn *spawn,
                                                   guint end_of_message_timeout);
guint               milter_manager_spawn_get_end_of_message_timeout
                                                  (MilterManagerSpawn *spawn);
void                milter_manager_spawn_set_user_name
                                                  (MilterManagerSpawn *spawn,
                                                   const gchar *user_name);
const gchar        *milter_manager_spawn_get_user_name
                                                  (MilterManagerSpawn *spawn);

G_END_DECLS

#endif /* __MILTER_MANAGER_SPAWN_H__ */

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
