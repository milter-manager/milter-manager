/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Copyright (C) 2008-2010  Nobuyoshi Nakada <nakada@clear-code.com>
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

#ifndef __MILTER_EVENTLOOP_H__
#define __MILTER_EVENTLOOP_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define MILTER_EVENTLOOP_ERROR           (milter_eventloop_error_quark())

#define MILTER_TYPE_EVENTLOOP            (milter_eventloop_get_type())
#define MILTER_EVENTLOOP(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), MILTER_TYPE_EVENTLOOP, MilterEventloop))
#define MILTER_EVENTLOOP_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), MILTER_TYPE_EVENTLOOP, MilterEventloopClass))
#define MILTER_IS_EVENTLOOP(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), MILTER_TYPE_EVENTLOOP))
#define MILTER_IS_EVENTLOOP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), MILTER_TYPE_EVENTLOOP))
#define MILTER_EVENTLOOP_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), MILTER_TYPE_EVENTLOOP, MilterEventloopClass))

typedef enum
{
    MILTER_EVENTLOOP_ERROR_MAX
} MilterEventloopError;

typedef struct _MilterEventloop         MilterEventloop;
typedef struct _MilterEventloopClass    MilterEventloopClass;

struct _MilterEventloop
{
    GObject object;
};

struct _MilterEventloopClass
{
    GObjectClass parent_class;
};

GQuark               milter_eventloop_error_quark       (void);
GType                milter_eventloop_get_type          (void) G_GNUC_CONST;

G_END_DECLS

#endif /* __MILTER_EVENTLOOP_H__ */

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
