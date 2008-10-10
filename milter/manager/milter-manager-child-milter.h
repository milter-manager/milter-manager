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

#ifndef __MILTER_MANAGER_CHILD_MILTER_H__
#define __MILTER_MANAGER_CHILD_MILTER_H__

#include <sys/types.h>
#include <sys/socket.h>

#include <glib-object.h>

#include <milter/server.h>

G_BEGIN_DECLS

#define MILTER_MANAGER_CHILD_MILTER_ERROR           (milter_manager_child_milter_error_quark())

#define MILTER_MANAGER_TYPE_CHILD_MILTER            (milter_manager_child_milter_get_type())
#define MILTER_MANAGER_CHILD_MILTER(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), MILTER_MANAGER_TYPE_CHILD_MILTER, MilterManagerChildMilter))
#define MILTER_MANAGER_CHILD_MILTER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), MILTER_MANAGER_TYPE_CHILD_MILTER, MilterManagerChildMilterClass))
#define MILTER_MANAGER_IS_CHILD_MILTER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), MILTER_MANAGER_TYPE_CHILD_MILTER))
#define MILTER_MANAGER_IS_CHILD_MILTER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), MILTER_MANAGER_TYPE_CHILD_MILTER))
#define MILTER_MANAGER_CHILD_MILTER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), MILTER_MANAGER_TYPE_CHILD_MILTER, MilterManagerChildMilterClass))

typedef enum
{
    MILTER_MANAGER_CHILD_MILTER_ERROR_FIXME
} MilterManagerChildMilterError;

typedef struct _MilterManagerChildMilter         MilterManagerChildMilter;
typedef struct _MilterManagerChildMilterClass    MilterManagerChildMilterClass;

struct _MilterManagerChildMilter
{
    MilterServerContext object;
};

struct _MilterManagerChildMilterClass
{
    MilterServerContextClass parent_class;
};

GQuark                milter_manager_child_milter_error_quark (void);

GType                 milter_manager_child_milter_get_type    (void) G_GNUC_CONST;

MilterManagerChildMilter *milter_manager_child_milter_new     (const gchar *name);

#endif /* __MILTER_MANAGER_CHILD_MILTER_H__ */

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
