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
 *  along with this library.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef __MILTER_MANAGER_H__
#define __MILTER_MANAGER_H__

#include <milter/manager/milter-manager-configuration.h>
#include <milter/manager/milter-manager-leader.h>
#include <milter/manager/milter-manager-child.h>
#include <milter/manager/milter-manager-children.h>
#include <milter/manager/milter-manager-egg.h>
#include <milter/manager/milter-manager-control-command-decoder.h>
#include <milter/manager/milter-manager-control-reply-decoder.h>
#include <milter/manager/milter-manager-control-command-encoder.h>
#include <milter/manager/milter-manager-control-reply-encoder.h>
#include <milter/manager/milter-manager-controller-context.h>
#include <milter/manager/milter-manager-controller.h>
#include <milter/manager/milter-manager-process-launcher.h>
#include <milter/manager/milter-manager-enum-types.h>
#include <milter/manager/milter-manager.h>

G_BEGIN_DECLS

void milter_manager_init (int *argc, char ***argv);
void milter_manager_quit (void);
gboolean milter_manager_main (void);


G_END_DECLS

#endif /* __MILTER_MANAGER_H__ */

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
