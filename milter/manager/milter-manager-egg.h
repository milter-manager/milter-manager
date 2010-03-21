/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Copyright (C) 2008-2010  Kouhei Sutou <kou@clear-code.com>
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

#ifndef __MILTER_MANAGER_EGG_H__
#define __MILTER_MANAGER_EGG_H__

#include <glib-object.h>

#include <milter/client.h>
#include <milter/server.h>
#include <milter/manager/milter-manager-objects.h>
#include <milter/manager/milter-manager-child.h>
#include <milter/manager/milter-manager-applicable-condition.h>

G_BEGIN_DECLS

#define MILTER_MANAGER_EGG_ERROR           (milter_manager_egg_error_quark())

#define MILTER_TYPE_MANAGER_EGG            (milter_manager_egg_get_type())
#define MILTER_MANAGER_EGG(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), MILTER_TYPE_MANAGER_EGG, MilterManagerEgg))
#define MILTER_MANAGER_EGG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), MILTER_TYPE_MANAGER_EGG, MilterManagerEggClass))
#define MILTER_MANAGER_IS_EGG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), MILTER_TYPE_MANAGER_EGG))
#define MILTER_MANAGER_IS_EGG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), MILTER_TYPE_MANAGER_EGG))
#define MILTER_MANAGER_EGG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), MILTER_TYPE_MANAGER_EGG, MilterManagerEggClass))

typedef enum
{
    MILTER_MANAGER_EGG_ERROR_INVALID
} MilterManagerEggError;

typedef struct _MilterManagerEggClass    MilterManagerEggClass;

struct _MilterManagerEgg
{
    GObject object;
};

struct _MilterManagerEggClass
{
    GObjectClass parent_class;

    void (*hatched) (MilterManagerEgg   *egg,
                     MilterManagerChild *child);
    void (*to_xml)  (MilterManagerEgg   *egg,
                     GString            *xml,
                     guint               indent);
};

GQuark              milter_manager_egg_error_quark (void);

GType               milter_manager_egg_get_type (void) G_GNUC_CONST;

MilterManagerEgg   *milter_manager_egg_new      (const gchar *name);

MilterManagerChild *milter_manager_egg_hatch    (MilterManagerEgg *egg);

void                milter_manager_egg_set_name (MilterManagerEgg *egg,
                                                 const gchar *name);
const gchar        *milter_manager_egg_get_name (MilterManagerEgg *egg);
void                milter_manager_egg_set_description
                                                (MilterManagerEgg *egg,
                                                 const gchar *description);
const gchar        *milter_manager_egg_get_description
                                                (MilterManagerEgg *egg);
void                milter_manager_egg_set_enabled
                                                (MilterManagerEgg *egg,
                                                 gboolean          enabled);
gboolean            milter_manager_egg_is_enabled
                                                (MilterManagerEgg *egg);
gboolean            milter_manager_egg_set_connection_spec
                                                (MilterManagerEgg *egg,
                                                 const gchar *connection_spec,
                                                 GError      **error);
const gchar        *milter_manager_egg_get_connection_spec
                                                (MilterManagerEgg *egg);
void                milter_manager_egg_set_connection_timeout
                                                (MilterManagerEgg *egg,
                                                 gdouble connection_timeout);
gdouble             milter_manager_egg_get_connection_timeout
                                                (MilterManagerEgg *egg);
void                milter_manager_egg_set_writing_timeout
                                                (MilterManagerEgg *egg,
                                                 gdouble writing_timeout);
gdouble             milter_manager_egg_get_writing_timeout
                                                (MilterManagerEgg *egg);
void                milter_manager_egg_set_reading_timeout
                                                (MilterManagerEgg *egg,
                                                 gdouble reading_timeout);
gdouble             milter_manager_egg_get_reading_timeout
                                                (MilterManagerEgg *egg);
void                milter_manager_egg_set_end_of_message_timeout
                                                (MilterManagerEgg *egg,
                                                 gdouble end_of_message_timeout);
gdouble             milter_manager_egg_get_end_of_message_timeout
                                                (MilterManagerEgg *egg);
void                milter_manager_egg_set_user_name
                                                (MilterManagerEgg *egg,
                                                 const gchar *user_name);
const gchar        *milter_manager_egg_get_user_name
                                                (MilterManagerEgg *egg);
void                milter_manager_egg_set_command
                                                (MilterManagerEgg *egg,
                                                 const gchar *command);
const gchar        *milter_manager_egg_get_command
                                                (MilterManagerEgg *egg);
void                milter_manager_egg_set_command_options
                                                (MilterManagerEgg *egg,
                                                 const gchar *command_options);
const gchar        *milter_manager_egg_get_command_options
                                                (MilterManagerEgg *egg);
void                milter_manager_egg_set_fallback_status
                                                (MilterManagerEgg *egg,
                                                 MilterStatus      status);
MilterStatus        milter_manager_egg_get_fallback_status
                                                (MilterManagerEgg *egg);
void                milter_manager_egg_set_evaluation_mode
                                                (MilterManagerEgg *egg,
                                                 gboolean          evaluation_mode);
gboolean            milter_manager_egg_is_evaluation_mode
                                                (MilterManagerEgg *egg);

void                milter_manager_egg_add_applicable_condition
                                                (MilterManagerEgg *egg,
                                                 MilterManagerApplicableCondition *condition);
const GList        *milter_manager_egg_get_applicable_conditions
                                                (MilterManagerEgg *egg);
void                milter_manager_egg_clear_applicable_conditions
                                                (MilterManagerEgg *egg);
void                milter_manager_egg_attach_applicable_conditions
                                                (MilterManagerEgg      *egg,
                                                 MilterManagerChild    *child,
                                                 MilterManagerChildren *children,
                                                 MilterClientContext   *context);

gboolean            milter_manager_egg_merge    (MilterManagerEgg *egg,
                                                 MilterManagerEgg *other_egg,
                                                 GError           **error);

gchar              *milter_manager_egg_to_xml   (MilterManagerEgg *egg);
void                milter_manager_egg_to_xml_string
                                                (MilterManagerEgg *egg,
                                                 GString *string,
                                                 guint indent);

G_END_DECLS

#endif /* __MILTER_MANAGER_EGG_H__ */

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
