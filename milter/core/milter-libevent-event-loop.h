/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Copyright (C) 2010, 2011  Nobuyoshi Nakada <nakada@clear-code.com>
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

#ifndef __MILTER_LIBEVENT_EVENT_LOOP_H__
#define __MILTER_LIBEVENT_EVENT_LOOP_H__

#include <milter/core/milter-event-loop.h>

G_BEGIN_DECLS

#define MILTER_TYPE_LIBEVENT_EVENT_LOOP            (milter_libevent_event_loop_get_type())
#define MILTER_LIBEVENT_EVENT_LOOP(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), MILTER_TYPE_LIBEVENT_EVENT_LOOP, MilterLibeventEventLoop))
#define MILTER_LIBEVENT_EVENT_LOOP_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), MILTER_TYPE_LIBEVENT_EVENT_LOOP, MilterLibeventEventLoopClass))
#define MILTER_IS_LIBEVENT_EVENT_LOOP(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), MILTER_TYPE_LIBEVENT_EVENT_LOOP))
#define MILTER_IS_LIBEVENT_EVENT_LOOP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), MILTER_TYPE_LIBEVENT_EVENT_LOOP))
#define MILTER_LIBEVENT_EVENT_LOOP_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), MILTER_TYPE_LIBEVENT_EVENT_LOOP, MilterLibeventEventLoopClass))

typedef struct _MilterLibeventEventLoop         MilterLibeventEventLoop;
typedef struct _MilterLibeventEventLoopClass    MilterLibeventEventLoopClass;

struct _MilterLibeventEventLoop
{
    MilterEventLoop object;
};

struct _MilterLibeventEventLoopClass
{
    MilterEventLoopClass parent_class;
};

GType                milter_libevent_event_loop_get_type     (void) G_GNUC_CONST;

MilterEventLoop     *milter_libevent_event_loop_new          (void);


G_END_DECLS

#endif /* __MILTER_LIBEVENT_EVENT_LOOP_H__ */

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
