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

#ifndef __MILTER_MANAGER_CONFIGURATION_H__
#define __MILTER_MANAGER_CONFIGURATION_H__

#include <glib-object.h>

#include <milter/manager/milter-manager-child.h>

G_BEGIN_DECLS

#define MILTER_TYPE_MANAGER_CONFIGURATION            (milter_manager_configuration_get_type())
#define MILTER_MANAGER_CONFIGURATION(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), MILTER_TYPE_MANAGER_CONFIGURATION, MilterManagerConfiguration))
#define MILTER_MANAGER_CONFIGURATION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), MILTER_TYPE_MANAGER_CONFIGURATION, MilterManagerConfigurationClass))
#define MILTER_MANAGER_IS_CONFIGURATION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), MILTER_TYPE_MANAGER_CONFIGURATION))
#define MILTER_MANAGER_IS_CONFIGURATION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), MILTER_TYPE_MANAGER_CONFIGURATION))
#define MILTER_MANAGER_CONFIGURATION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), MILTER_TYPE_MANAGER_CONFIGURATION, MilterManagerConfigurationClass))

typedef struct _MilterManagerConfiguration      MilterManagerConfiguration;
typedef struct _MilterManagerConfigurationClass MilterManagerConfigurationClass;

struct _MilterManagerConfiguration
{
    GObject object;
};

struct _MilterManagerConfigurationClass
{
    GObjectClass parent_class;

    void         (*add_load_path) (MilterManagerConfiguration *configuration,
                                   const gchar                *path);
    void         (*load)          (MilterManagerConfiguration *configuration,
                                   const gchar                *file_name);
};

GType         milter_manager_configuration_get_type    (void) G_GNUC_CONST;

void         _milter_manager_configuration_init        (void);
void         _milter_manager_configuration_quit        (void);


MilterManagerConfiguration *
              milter_manager_configuration_new         (const gchar *first_property,
                                                        ...);

void          milter_manager_configuration_add_load_path
                                     (MilterManagerConfiguration *configuration,
                                      const gchar                *path);
void          milter_manager_configuration_load
                                     (MilterManagerConfiguration *configuration,
                                      const gchar                *file_name);

gboolean      milter_manager_configuration_is_privilege_mode
                                     (MilterManagerConfiguration *configuration);
MilterStatus  milter_manager_configuration_get_return_status_if_filter_unavailable
                                     (MilterManagerConfiguration *configuration);
guint         milter_manager_configuration_get_connection_timeout
                                     (MilterManagerConfiguration *configuration);
guint         milter_manager_configuration_get_sending_timeout
                                     (MilterManagerConfiguration *configuration);
guint         milter_manager_configuration_get_reading_timeout
                                     (MilterManagerConfiguration *configuration);
guint         milter_manager_configuration_get_end_of_message_timeout
                                     (MilterManagerConfiguration *configuration);
void          milter_manager_configuration_add_child
                                     (MilterManagerConfiguration *configuration,
                                      MilterManagerChild *milter);
const GList  *milter_manager_configuration_get_children
                                     (MilterManagerConfiguration *configuration);
GList        *milter_manager_configuration_create_children
                                     (MilterManagerConfiguration *configuration);

G_END_DECLS

#endif /* __MILTER_MANAGER_CONFIGURATION_H__ */

/*
vi:nowrap:ai:expandtab:sw=4
*/
