/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Copyright (C) 2008-2009  Kouhei Sutou <kou@clear-code.com>
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

#ifndef __MILTER_MANAGER_APPLICABLE_CONDITION_H__
#define __MILTER_MANAGER_APPLICABLE_CONDITION_H__

#include <glib-object.h>

#include <milter/client.h>
#include <milter/server.h>
#include <milter/manager/milter-manager-objects.h>
#include <milter/manager/milter-manager-child.h>
#include <milter/manager/milter-manager-children.h>

G_BEGIN_DECLS

#define MILTER_TYPE_MANAGER_APPLICABLE_CONDITION            (milter_manager_applicable_condition_get_type())
#define MILTER_MANAGER_APPLICABLE_CONDITION(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), MILTER_TYPE_MANAGER_APPLICABLE_CONDITION, MilterManagerApplicableCondition))
#define MILTER_MANAGER_APPLICABLE_CONDITION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), MILTER_TYPE_MANAGER_APPLICABLE_CONDITION, MilterManagerApplicableConditionClass))
#define MILTER_MANAGER_IS_APPLICABLE_CONDITION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), MILTER_TYPE_MANAGER_APPLICABLE_CONDITION))
#define MILTER_MANAGER_IS_APPLICABLE_CONDITION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), MILTER_TYPE_MANAGER_APPLICABLE_CONDITION))
#define MILTER_MANAGER_APPLICABLE_CONDITION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), MILTER_TYPE_MANAGER_APPLICABLE_CONDITION, MilterManagerApplicableConditionClass))

typedef struct _MilterManagerApplicableConditionClass    MilterManagerApplicableConditionClass;

struct _MilterManagerApplicableCondition
{
    GObject object;
};

struct _MilterManagerApplicableConditionClass
{
    GObjectClass parent_class;

    void (*attach_to) (MilterManagerApplicableCondition *applicable_condition,
                       MilterManagerChild               *child,
                       MilterManagerChildren            *children,
                       MilterClientContext              *context);
};

GType        milter_manager_applicable_condition_get_type (void) G_GNUC_CONST;

MilterManagerApplicableCondition *milter_manager_applicable_condition_new
                                   (const gchar *name);

void         milter_manager_applicable_condition_set_name
                                   (MilterManagerApplicableCondition *condition,
                                    const gchar *name);
const gchar *milter_manager_applicable_condition_get_name
                                   (MilterManagerApplicableCondition *condition);

void         milter_manager_applicable_condition_set_description
                                   (MilterManagerApplicableCondition *condition,
                                    const gchar *description);
const gchar *milter_manager_applicable_condition_get_description
                                   (MilterManagerApplicableCondition *condition);
void         milter_manager_applicable_condition_set_data
                                   (MilterManagerApplicableCondition *condition,
                                    const gchar *data);
const gchar *milter_manager_applicable_condition_get_data
                                   (MilterManagerApplicableCondition *condition);
void         milter_manager_applicable_condition_merge
                                   (MilterManagerApplicableCondition *condition,
                                    MilterManagerApplicableCondition *other_condition);

void         milter_manager_applicable_condition_attach_to
                                   (MilterManagerApplicableCondition *condition,
                                    MilterManagerChild               *child,
                                    MilterManagerChildren            *children,
                                    MilterClientContext              *context);

G_END_DECLS

#endif /* __MILTER_MANAGER_APPLICABLE_CONDITION_H__ */

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
