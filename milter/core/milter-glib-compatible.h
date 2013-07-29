/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Copyright (C) 2013 Kenji Okimoto <okimoto@clear-code.com>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef __MILTER_GLIB_COMPATIBLE_H__
#define __MILTER_GLIB_COMPATIBLE_H__

#include <glib.h>

#if !GLIB_CHECK_VERSION(2, 32, 0)
#define g_thread_try_new(name, func, data, error) \
    g_thread_create((func), (data), TRUE, (error))
#endif

#endif
