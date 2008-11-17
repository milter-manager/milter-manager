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

#ifndef __MILTER_MANAGER_CONTROL_PROTOCOL_H__
#define __MILTER_MANAGER_CONTROL_PROTOCOL_H__

#include <glib-object.h>

G_BEGIN_DECLS

typedef enum
{
    MILTER_MANAGER_CONTROL_PROTOCOL_COMMAND_IMPORT_CONFIGURATION = 'I',
    MILTER_MANAGER_CONTROL_PROTOCOL_COMMAND_RELOAD_CONFIGURATION = 'R',
    MILTER_MANAGER_CONTROL_PROTOCOL_COMMAND_GET_CONFIGURATION = 'G',
    MILTER_MANAGER_CONTROL_PROTOCOL_COMMAND_STOP_CHILD = 's',
    MILTER_MANAGER_CONTROL_PROTOCOL_COMMAND_GET_CHILDREN_INFO = 'g'
} MilterManagerControlProtocolCommand;

G_END_DECLS

#endif /* __MILTER_MANAGER_CONTROL_PROTOCOL_H__ */

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
