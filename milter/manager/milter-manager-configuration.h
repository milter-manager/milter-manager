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

#include <milter/manager/milter-manager-objects.h>
#include <milter/manager/milter-manager-child.h>
#include <milter/manager/milter-manager-egg.h>

G_BEGIN_DECLS

#define MILTER_MANAGER_CONFIGURATION_ERROR           (milter_manager_configuration_error_quark())

#define MILTER_TYPE_MANAGER_CONFIGURATION            (milter_manager_configuration_get_type())
#define MILTER_MANAGER_CONFIGURATION(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), MILTER_TYPE_MANAGER_CONFIGURATION, MilterManagerConfiguration))
#define MILTER_MANAGER_CONFIGURATION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), MILTER_TYPE_MANAGER_CONFIGURATION, MilterManagerConfigurationClass))
#define MILTER_MANAGER_IS_CONFIGURATION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), MILTER_TYPE_MANAGER_CONFIGURATION))
#define MILTER_MANAGER_IS_CONFIGURATION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), MILTER_TYPE_MANAGER_CONFIGURATION))
#define MILTER_MANAGER_CONFIGURATION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), MILTER_TYPE_MANAGER_CONFIGURATION, MilterManagerConfigurationClass))

typedef enum
{
    MILTER_MANAGER_CONFIGURATION_ERROR_NOT_IMPLEMENTED,
    MILTER_MANAGER_CONFIGURATION_ERROR_NOT_EXIST,
    MILTER_MANAGER_CONFIGURATION_ERROR_UNKNOWN,
    MILTER_MANAGER_CONFIGURATION_ERROR_SAVE
} MilterManagerConfigurationError;

typedef struct _MilterManagerConfigurationClass MilterManagerConfigurationClass;

struct _MilterManagerConfiguration
{
    GObject object;
};

struct _MilterManagerConfigurationClass
{
    GObjectClass parent_class;

    gboolean     (*load)          (MilterManagerConfiguration *configuration,
                                   const gchar                *file_name,
                                   GError                    **error);
};

GQuark        milter_manager_configuration_error_quark (void);
GType         milter_manager_configuration_get_type    (void) G_GNUC_CONST;

void         _milter_manager_configuration_init        (void);
void         _milter_manager_configuration_quit        (void);


MilterManagerConfiguration *
              milter_manager_configuration_new         (const gchar *first_property,
                                                        ...);

void          milter_manager_configuration_add_load_path
                                     (MilterManagerConfiguration *configuration,
                                      const gchar                *path);
gboolean      milter_manager_configuration_load
                                     (MilterManagerConfiguration *configuration,
                                      const gchar                *file_name,
                                      GError                    **error);
gboolean      milter_manager_configuration_load_if_exist
                                     (MilterManagerConfiguration *configuration,
                                      const gchar                *file_name,
                                      GError                    **error);
void          milter_manager_configuration_reload
                                     (MilterManagerConfiguration *configuration);

gboolean      milter_manager_configuration_save_custom
                                     (MilterManagerConfiguration *configuration,
                                      const gchar                *content,
                                      gssize                      size,
                                      GError                    **error);

gboolean      milter_manager_configuration_is_privilege_mode
                                     (MilterManagerConfiguration *configuration);
void          milter_manager_configuration_set_privilege_mode
                                     (MilterManagerConfiguration *configuration,
                                      gboolean                    mode);
const gchar  *milter_manager_configuration_get_control_connection_spec
                                     (MilterManagerConfiguration *configuration);
void          milter_manager_configuration_set_control_connection_spec
                                     (MilterManagerConfiguration *configuration,
                                      const gchar                *spec);

const gchar  *milter_manager_configuration_get_manager_connection_spec
                                     (MilterManagerConfiguration *configuration);
void          milter_manager_configuration_set_manager_connection_spec
                                     (MilterManagerConfiguration *configuration,
                                      const gchar                *spec);

/* what about milter_manager_configuration_get_fallback_status()? */
MilterStatus  milter_manager_configuration_get_return_status_if_filter_unavailable
                                     (MilterManagerConfiguration *configuration);
void          milter_manager_configuration_set_return_status_if_filter_unavailable
                                     (MilterManagerConfiguration *configuration,
                                      MilterStatus                status);
void          milter_manager_configuration_add_egg
                                     (MilterManagerConfiguration *configuration,
                                      MilterManagerEgg *egg);
MilterManagerChildren *milter_manager_configuration_create_children
                                     (MilterManagerConfiguration *configuration);

void          milter_manager_configuration_clear
                                     (MilterManagerConfiguration *configuration);

G_END_DECLS

#endif /* __MILTER_MANAGER_CONFIGURATION_H__ */

/*
vi:nowrap:ai:expandtab:sw=4
*/
