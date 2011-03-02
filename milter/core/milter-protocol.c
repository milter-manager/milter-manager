/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Copyright (C) 2011  Kouhei Sutou <kou@clear-code.com>
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

#include "milter-protocol.h"

gint
milter_status_compare (MilterStatus current_status,
                       MilterStatus new_status)
{
    if (current_status == new_status)
        return 0;

    switch (current_status) {
    case MILTER_STATUS_DISCARD:
        return 1;
        break;
    case MILTER_STATUS_REJECT:
        if (new_status == MILTER_STATUS_DISCARD)
            return -1;
        return 1;
        break;
    case MILTER_STATUS_TEMPORARY_FAILURE:
        if (new_status == MILTER_STATUS_DISCARD ||
            new_status == MILTER_STATUS_REJECT)
            return -1;
        return 1;
        break;
    case MILTER_STATUS_CONTINUE:
        if (new_status == MILTER_STATUS_DISCARD ||
            new_status == MILTER_STATUS_REJECT ||
            new_status == MILTER_STATUS_TEMPORARY_FAILURE)
            return -1;
        return 1;
    case MILTER_STATUS_SKIP:
        if (new_status == MILTER_STATUS_DISCARD ||
            new_status == MILTER_STATUS_REJECT ||
            new_status == MILTER_STATUS_TEMPORARY_FAILURE ||
            new_status == MILTER_STATUS_CONTINUE)
            return -1;
        return 1;
        break;
    case MILTER_STATUS_ACCEPT:
        if (new_status == MILTER_STATUS_DISCARD ||
            new_status == MILTER_STATUS_REJECT ||
            new_status == MILTER_STATUS_TEMPORARY_FAILURE ||
            new_status == MILTER_STATUS_SKIP ||
            new_status == MILTER_STATUS_CONTINUE)
            return -1;
        return 1;
        break;
    default:
        return -1;
        break;
    }

    return -1;
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
