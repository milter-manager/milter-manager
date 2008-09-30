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

#ifdef HAVE_CONFIG_H
#  include "../config.h"
#endif /* HAVE_CONFIG_H */

#include <string.h>
#include <sys/un.h>

#include "milter-utils.h"

GQuark
milter_utils_error_quark (void)
{
    return g_quark_from_static_string("milter-utils-error-quark");
}

gboolean
milter_utils_parse_connection_spec (const gchar      *spec,
                                    struct sockaddr **address,
                                    socklen_t        *address_size,
                                    GError          **error)
{
    const gchar *colon;

    colon = strstr(spec, ":");
    if (!colon) {
        g_set_error(error,
                    MILTER_UTILS_ERROR,
                    MILTER_UTILS_ERROR_INVALID_FORMAT,
                    "spec doesn't have colon: <%s>", spec);
        return FALSE;
    }

    if (g_str_has_prefix(spec, "unix")) {
        struct sockaddr_un *address_unix;

        address_unix = g_new(struct sockaddr_un, 1);
        address_unix->sun_family = AF_UNIX;
        strcpy(address_unix->sun_path, colon + 1);

        *address = (struct sockaddr *)address_unix;
        *address_size = sizeof(address_unix);
    } else {
        return FALSE;
    }

    return TRUE;
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
