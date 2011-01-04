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

#ifndef __MILTER_MANAGER_CONTROLLER_H__
#define __MILTER_MANAGER_CONTROLLER_H__

#include <glib-object.h>

#include <milter/client.h>
#include <milter/server.h>
#include <milter/manager/milter-manager-controller-context.h>

G_BEGIN_DECLS

#define MILTER_MANAGER_CONTROLLER_ERROR           (milter_manager_controller_error_quark())

#define MILTER_TYPE_MANAGER_CONTROLLER            (milter_manager_controller_get_type())
#define MILTER_MANAGER_CONTROLLER(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), MILTER_TYPE_MANAGER_CONTROLLER, MilterManagerController))
#define MILTER_MANAGER_CONTROLLER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), MILTER_TYPE_MANAGER_CONTROLLER, MilterManagerControllerClass))
#define MILTER_MANAGER_IS_CONTROLLER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), MILTER_TYPE_MANAGER_CONTROLLER))
#define MILTER_MANAGER_IS_CONTROLLER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), MILTER_TYPE_MANAGER_CONTROLLER))
#define MILTER_MANAGER_CONTROLLER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), MILTER_TYPE_MANAGER_CONTROLLER, MilterManagerControllerClass))

typedef enum
{
    MILTER_MANAGER_CONTROLLER_ERROR_LISTING,
    MILTER_MANAGER_CONTROLLER_ERROR_NO_SPEC
} MilterManagerControllerError;

typedef struct _MilterManagerController         MilterManagerController;
typedef struct _MilterManagerControllerClass    MilterManagerControllerClass;

struct _MilterManagerController
{
    GObject object;
};

struct _MilterManagerControllerClass
{
    GObjectClass parent_class;
};

GQuark                milter_manager_controller_error_quark (void);

GType                 milter_manager_controller_get_type    (void) G_GNUC_CONST;

MilterManagerController *milter_manager_controller_new
                                          (MilterManager   *manager,
                                           MilterEventLoop *event_loop);

gboolean              milter_manager_controller_listen
                                          (MilterManagerController  *controller,
                                           GError                  **error);

G_END_DECLS

#endif /* __MILTER_MANAGER_CONTROLLER_H__ */

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
