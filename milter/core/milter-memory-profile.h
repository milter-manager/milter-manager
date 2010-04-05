/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Copyright (C) 2010  Kouhei Sutou <kou@clear-code.com>
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

#ifndef __MILTER_MEMORY_PROFILE_H__
#define __MILTER_MEMORY_PROFILE_H__

#include <glib-object.h>

#include <milter/core/milter-memory-profile.h>

G_BEGIN_DECLS

void             milter_memory_profile_init     (void);
void             milter_memory_profile_quit     (void);
void             milter_memory_profile_enable   (void);
void             milter_memory_profile_report   (void);
gboolean         milter_memory_profile_get_data (gsize *n_allocates,
                                                 gsize *n_zero_initializes,
                                                 gsize *n_frees);

G_END_DECLS

#endif /* __MILTER_MEMORY_PROFILE_H__ */

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
