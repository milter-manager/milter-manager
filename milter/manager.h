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

#ifndef __MILTER_MANAGER_H__
#define __MILTER_MANAGER_H__

#include <milter/manager/milter-manager-context.h>
#include <milter/manager/milter-manager-configuration.h>
#include <milter/manager/milter-manager-ruby.h>
#include <milter/manager/milter-manager-enum-types.h>

G_BEGIN_DECLS

void milter_manager_init (int *argc, char ***argv);
void milter_manager_quit (void);
void milter_manager_main (void);


G_END_DECLS

#endif /* __MILTER_MANAGER_H__ */

/*
vi:nowrap:ai:expandtab:sw=4
*/
