/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Copyright (C) 2009-2010  Kouhei Sutou <kou@clear-code.com>
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

#ifdef HAVE_CONFIG_H
#  include "../../config.h"
#endif /* HAVE_CONFIG_H */

#include <milter/server.h>

static gboolean initialized = FALSE;
static guint milter_server_log_handler_id = 0;

void
milter_server_init (void)
{
    if (initialized)
        return;

    initialized = TRUE;

    milter_server_log_handler_id = MILTER_GLIB_LOG_DELEGATE("milter-server");
}

void
milter_server_quit (void)
{
    if (!initialized)
        return;

    g_log_remove_handler("milter-server", milter_server_log_handler_id);

    milter_server_log_handler_id = 0;
    initialized = FALSE;
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
