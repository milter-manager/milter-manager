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

#ifndef __MILTER_MANAGER_PROCESS_LAUNCHER_H__
#define __MILTER_MANAGER_PROCESS_LAUNCHER_H__

#include <glib-object.h>

#include <milter/client.h>
#include <milter/server.h>
#include <milter/manager/milter-manager.h>

G_BEGIN_DECLS

#define MILTER_MANAGER_PROCESS_LAUNCHER_ERROR           (milter_manager_process_launcher_error_quark())

#define MILTER_TYPE_MANAGER_PROCESS_LAUNCHER            (milter_manager_process_launcher_get_type())
#define MILTER_MANAGER_PROCESS_LAUNCHER(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), MILTER_TYPE_MANAGER_PROCESS_LAUNCHER, MilterManagerProcessLauncher))
#define MILTER_MANAGER_PROCESS_LAUNCHER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), MILTER_TYPE_MANAGER_PROCESS_LAUNCHER, MilterManagerProcessLauncherClass))
#define MILTER_MANAGER_IS_PROCESS_LAUNCHER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), MILTER_TYPE_MANAGER_PROCESS_LAUNCHER))
#define MILTER_MANAGER_IS_PROCESS_LAUNCHER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), MILTER_TYPE_MANAGER_PROCESS_LAUNCHER))
#define MILTER_MANAGER_PROCESS_LAUNCHER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), MILTER_TYPE_MANAGER_PROCESS_LAUNCHER, MilterManagerProcessLauncherClass))

typedef enum
{
    MILTER_MANAGER_PROCESS_LAUNCHER_ERROR_BAD_COMMAND_STRING,
    MILTER_MANAGER_PROCESS_LAUNCHER_ERROR_START_FAILURE,
    MILTER_MANAGER_PROCESS_LAUNCHER_ERROR_INVALID_USER_NAME,
    MILTER_MANAGER_PROCESS_LAUNCHER_ERROR_ALREADY_LAUNCHED,
    MILTER_MANAGER_PROCESS_LAUNCHER_ERROR_MILTER_EXIT
} MilterManagerProcessLauncherError;

typedef struct _MilterManagerProcessLauncher         MilterManagerProcessLauncher;
typedef struct _MilterManagerProcessLauncherClass    MilterManagerProcessLauncherClass;

struct _MilterManagerProcessLauncher
{
    MilterAgent object;
};

struct _MilterManagerProcessLauncherClass
{
    MilterAgentClass parent_class;
};

GQuark                        milter_manager_process_launcher_error_quark (void);

GType                         milter_manager_process_launcher_get_type (void) G_GNUC_CONST;

MilterManagerProcessLauncher *milter_manager_process_launcher_new      (void);

void                          milter_manager_process_launcher_run      (MilterManagerProcessLauncher *launcher);
void                          milter_manager_process_launcher_shutdown (MilterManagerProcessLauncher *launcher);

G_END_DECLS

#endif /* __MILTER_MANAGER_PROCESS_LAUNCHER_H__ */

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
