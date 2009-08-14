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

#ifndef __MILTER_MANAGER_CHILD_H__
#define __MILTER_MANAGER_CHILD_H__

#include <sys/types.h>
#include <sys/socket.h>

#include <glib-object.h>

#include <milter/server.h>

G_BEGIN_DECLS

#define MILTER_MANAGER_CHILD_ERROR           (milter_manager_child_error_quark())

#define MILTER_TYPE_MANAGER_CHILD            (milter_manager_child_get_type())
#define MILTER_MANAGER_CHILD(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), MILTER_TYPE_MANAGER_CHILD, MilterManagerChild))
#define MILTER_MANAGER_CHILD_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), MILTER_TYPE_MANAGER_CHILD, MilterManagerChildClass))
#define MILTER_MANAGER_IS_CHILD(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), MILTER_TYPE_MANAGER_CHILD))
#define MILTER_MANAGER_IS_CHILD_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), MILTER_TYPE_MANAGER_CHILD))
#define MILTER_MANAGER_CHILD_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), MILTER_TYPE_MANAGER_CHILD, MilterManagerChildClass))

typedef enum
{
    MILTER_MANAGER_CHILD_ERROR_BAD_COMMAND_STRING,
    MILTER_MANAGER_CHILD_ERROR_START_FAILURE,
    MILTER_MANAGER_CHILD_ERROR_INVALID_USER_NAME,
    MILTER_MANAGER_CHILD_ERROR_NO_PRIVILEGE_MODE,
    MILTER_MANAGER_CHILD_ERROR_MILTER_TERMINATED_BY_SIGNAL,
    MILTER_MANAGER_CHILD_ERROR_MILTER_EXIT
} MilterManagerChildError;

typedef struct _MilterManagerChild         MilterManagerChild;
typedef struct _MilterManagerChildClass    MilterManagerChildClass;

struct _MilterManagerChild
{
    MilterServerContext object;
};

struct _MilterManagerChildClass
{
    MilterServerContextClass parent_class;
};

GQuark                milter_manager_child_error_quark (void);

GType                 milter_manager_child_get_type    (void) G_GNUC_CONST;

MilterManagerChild   *milter_manager_child_new         (const gchar *name);
MilterManagerChild   *milter_manager_child_new_va_list (const gchar *first_name,
                                                        va_list      args);

gchar                *milter_manager_child_get_command_line_string
                                                       (MilterManagerChild *milter);
gchar                *milter_manager_child_get_user_name
                                                       (MilterManagerChild *milter);

MilterStatus          milter_manager_child_get_fallback_status
                                                       (MilterManagerChild *milter);

void                  milter_manager_child_set_evaluation_mode
                                                       (MilterManagerChild *milter,
                                                        gboolean evaluation_mode);
gboolean              milter_manager_child_is_evaluation_mode
                                                       (MilterManagerChild *milter);

#endif /* __MILTER_MANAGER_CHILD_H__ */

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
