/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Copyright (C) 2008-2009  Kouhei Sutou <kou@cozmixng.org>
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

#ifndef __MILTER_MANAGER_CONTROLLER_CONTEXT_H__
#define __MILTER_MANAGER_CONTROLLER_CONTEXT_H__

#include <glib-object.h>

#include <milter/client.h>
#include <milter/server.h>
#include <milter/manager/milter-manager.h>

G_BEGIN_DECLS

#define MILTER_TYPE_MANAGER_CONTROLLER_CONTEXT            (milter_manager_controller_context_get_type())
#define MILTER_MANAGER_CONTROLLER_CONTEXT(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), MILTER_TYPE_MANAGER_CONTROLLER_CONTEXT, MilterManagerControllerContext))
#define MILTER_MANAGER_CONTROLLER_CONTEXT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), MILTER_TYPE_MANAGER_CONTROLLER_CONTEXT, MilterManagerControllerContextClass))
#define MILTER_MANAGER_IS_CONTROLLER_CONTEXT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), MILTER_TYPE_MANAGER_CONTROLLER_CONTEXT))
#define MILTER_MANAGER_IS_CONTROLLER_CONTEXT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), MILTER_TYPE_MANAGER_CONTROLLER_CONTEXT))
#define MILTER_MANAGER_CONTROLLER_CONTEXT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), MILTER_TYPE_MANAGER_CONTROLLER_CONTEXT, MilterManagerControllerContextClass))

typedef struct _MilterManagerControllerContext         MilterManagerControllerContext;
typedef struct _MilterManagerControllerContextClass    MilterManagerControllerContextClass;

struct _MilterManagerControllerContext
{
    MilterAgent object;
};

struct _MilterManagerControllerContextClass
{
    MilterAgentClass parent_class;
};

GType                 milter_manager_controller_context_get_type    (void) G_GNUC_CONST;

MilterManagerControllerContext *milter_manager_controller_context_new
                                          (MilterManager *manager);

G_END_DECLS

#endif /* __MILTER_MANAGER_CONTROLLER_CONTEXT_H__ */

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
