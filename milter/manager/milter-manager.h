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

#ifndef __MILTER_MANAGER_MANAGER_H__
#define __MILTER_MANAGER_MANAGER_H__

#include <glib-object.h>

#include <milter/client.h>
#include <milter/server.h>
#include <milter/manager/milter-manager-configuration.h>

G_BEGIN_DECLS

#define MILTER_TYPE_MANAGER            (milter_manager_get_type())
#define MILTER_MANAGER(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), MILTER_TYPE_MANAGER, MilterManager))
#define MILTER_MANAGER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), MILTER_TYPE_MANAGER, MilterManagerClass))
#define MILTER_MANAGER_IS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), MILTER_TYPE_MANAGER))
#define MILTER_MANAGER_IS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), MILTER_TYPE_MANAGER))
#define MILTER_MANAGER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), MILTER_TYPE_MANAGER, MilterManagerClass))

typedef struct _MilterManager         MilterManager;
typedef struct _MilterManagerClass    MilterManagerClass;

struct _MilterManager
{
    MilterClient object;
};

struct _MilterManagerClass
{
    MilterClientClass parent_class;
};

GType                 milter_manager_get_type    (void) G_GNUC_CONST;


MilterManager        *milter_manager_new         (MilterManagerConfiguration *configuration);

MilterManagerConfiguration *milter_manager_get_configuration (MilterManager *manager);
const GList          *milter_manager_get_leaders (MilterManager *manager);

gboolean              milter_manager_reload      (MilterManager *manager,
                                                  GError       **error);
void                  milter_manager_set_launcher_channel
                                                 (MilterManager *manager,
                                                  GIOChannel *read_channel,
                                                  GIOChannel *write_channel);

G_END_DECLS

#endif /* __MILTER_MANAGER_MANAGER_H__ */

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
