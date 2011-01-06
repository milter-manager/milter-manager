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

#ifndef __MILTER_CLIENT_OBJECTS_H__
#define __MILTER_CLIENT_OBJECTS_H__

#include <glib-object.h>

G_BEGIN_DECLS

typedef struct _MilterClient         MilterClient;
typedef struct _MilterClientContext  MilterClientContext;

/**
 * MilterClientEventLoopBackendMode:
 * @MILTER_CLIENT_EVENT_LOOP_BACKEND_GLIB: Let main loop use GLib.
 * @MILTER_CLIENT_EVENT_LOOP_BACKEND_LIBEV: Let main loop use libev.
 */
typedef enum
{
    MILTER_CLIENT_EVENT_LOOP_BACKEND_GLIB,
    MILTER_CLIENT_EVENT_LOOP_BACKEND_LIBEV,
    MILTER_CLIENT_EVENT_LOOP_BACKEND__MAX /*< skip >*/
} MilterClientEventLoopBackendMode;

G_END_DECLS

#endif /* __MILTER_CLIENT_OBJECTS_H__ */

/*
vi:nowrap:ai:expandtab:sw=4
*/
