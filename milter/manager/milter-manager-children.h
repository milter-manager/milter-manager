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

#ifndef __MILTER_MANAGER_CHILDREN_H__
#define __MILTER_MANAGER_CHILDREN_H__

#include <glib-object.h>

#include <milter/manager/milter-manager-child.h>

G_BEGIN_DECLS

#define MILTER_MANAGER_CHILDREN_ERROR           (milter_manager_children_error_quark())

#define MILTER_MANAGER_TYPE_CHILDREN            (milter_manager_children_get_type())
#define MILTER_MANAGER_CHILDREN(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), MILTER_MANAGER_TYPE_CHILDREN, MilterManagerChildren))
#define MILTER_MANAGER_CHILDREN_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), MILTER_MANAGER_TYPE_CHILDREN, MilterManagerChildrenClass))
#define MILTER_MANAGER_IS_CHILDREN(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), MILTER_MANAGER_TYPE_CHILDREN))
#define MILTER_MANAGER_IS_CHILDREN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), MILTER_MANAGER_TYPE_CHILDREN))
#define MILTER_MANAGER_CHILDREN_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), MILTER_MANAGER_TYPE_CHILDREN, MilterManagerChildrenClass))

typedef enum
{
    MILTER_MANAGER_CHILDREN_ERROR_MILTER_EXIT
} MilterManagerChildrenError;

typedef struct _MilterManagerChildren         MilterManagerChildren;
typedef struct _MilterManagerChildrenClass    MilterManagerChildrenClass;

struct _MilterManagerChildren
{
    GObject object;
};

struct _MilterManagerChildrenClass
{
    GObjectClass parent_class;
};

GQuark                 milter_manager_children_error_quark (void);

GType                  milter_manager_children_get_type    (void) G_GNUC_CONST;

MilterManagerChildren *milter_manager_children_new         (void);
                        
void                   milter_manager_children_add_child   (MilterManagerChildren *children, 
                                                            MilterManagerChild *child);
GList                 *milter_manager_children_get_children(MilterManagerChildren *children);
void                   milter_manager_children_foreach     (MilterManagerChildren *children,
                                                            GFunc func,
                                                            gpointer user_data);

#endif /* __MILTER_MANAGER_CHILDREN_H__ */

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
