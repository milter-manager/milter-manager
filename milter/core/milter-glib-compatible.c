/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Copyright (C) 2013  Kouhei Sutou <kou@clear-code.com>
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

#ifdef HAVE_CONFIG_H
#  include "../../config.h"
#endif /* HAVE_CONFIG_H */

#include "milter-glib-compatible.h"

#if GLIB_CHECK_VERSION(2, 32, 0)
GMutex *
milter_glib_compatible_mutex_new(void)
{
    GMutex *mutex;
    mutex = g_new(GMutex, 1);
    g_mutex_init(mutex);
    return mutex;
}

void
milter_glib_compatible_mutex_free(GMutex *mutex)
{
    g_mutex_clear(mutex);
    g_free(mutex);
}
#endif

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
