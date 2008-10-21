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

#include <milter/manager/milter-manager-child-milter.h>

G_BEGIN_DECLS

#define MILTER_MANAGER_TYPE_CONFIGURATION            (milter_manager_configuration_get_type())
#define MILTER_MANAGER_CONFIGURATION(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), MILTER_MANAGER_TYPE_CONFIGURATION, MilterManagerConfiguration))
#define MILTER_MANAGER_CONFIGURATION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), MILTER_MANAGER_TYPE_CONFIGURATION, MilterManagerConfigurationClass))
#define MILTER_MANAGER_IS_CONFIGURATION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), MILTER_MANAGER_TYPE_CONFIGURATION))
#define MILTER_MANAGER_IS_CONFIGURATION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), MILTER_MANAGER_TYPE_CONFIGURATION))
#define MILTER_MANAGER_CONFIGURATION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), MILTER_MANAGER_TYPE_CONFIGURATION, MilterManagerConfigurationClass))

typedef struct _MilterManagerConfiguration      MilterManagerConfiguration;
typedef struct _MilterManagerConfigurationClass MilterManagerConfigurationClass;

struct _MilterManagerConfiguration
{
    GObject object;
};

struct _MilterManagerConfigurationClass
{
    GObjectClass parent_class;
};

GType         milter_manager_configuration_get_type    (void) G_GNUC_CONST;

MilterManagerConfiguration *
              milter_manager_configuration_new         (void);

gboolean      milter_manager_configuration_is_privilege_mode
                                     (MilterManagerConfiguration *configuration);
MilterStatus  milter_manager_configuration_get_return_status_if_filter_unavailable
                                     (MilterManagerConfiguration *configuration);
void          milter_manager_configuration_add_child_milter
                                     (MilterManagerConfiguration *configuration,
                                      MilterManagerChildMilter   *milter);
const GList  *milter_manager_configuration_get_child_milters
                                     (MilterManagerConfiguration *configuration);

G_END_DECLS

#endif /* __MILTER_MANAGER_CONFIGURATION_H__ */

/*
vi:nowrap:ai:expandtab:sw=4
*/
