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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef __GCUT_MILTER_EVENT_LOOP_H__
#define __GCUT_MILTER_EVENT_LOOP_H__

#include <gcutter.h>
#include <milter/core.h>

G_BEGIN_DECLS

#define GCUT_TYPE_MILTER_EVENT_LOOP            (gcut_milter_event_loop_get_type())
#define GCUT_MILTER_EVENT_LOOP(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), GCUT_TYPE_MILTER_EVENT_LOOP, GCutMilterEventLoop))
#define GCUT_MILTER_EVENT_LOOP_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), GCUT_TYPE_MILTER_EVENT_LOOP, GCutMilterEventLoopClass))
#define GCUT_IS_MILTER_EVENT_LOOP(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), GCUT_TYPE_MILTER_EVENT_LOOP))
#define GCUT_IS_MILTER_EVENT_LOOP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), GCUT_TYPE_MILTER_EVENT_LOOP))
#define GCUT_MILTER_EVENT_LOOP_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GCUT_TYPE_MILTER_EVENT_LOOP, GCutMilterEventLoopClass))

typedef struct _GCutMilterEventLoop         GCutMilterEventLoop;
typedef struct _GCutMilterEventLoopClass    GCutMilterEventLoopClass;

struct _GCutMilterEventLoop
{
    GCutEventLoop object;
};

struct _GCutMilterEventLoopClass
{
    GCutEventLoopClass parent_class;
};

GType                gcut_milter_event_loop_get_type     (void) G_GNUC_CONST;

GCutEventLoop       *gcut_milter_event_loop_new          (MilterEventLoop *loop);


G_END_DECLS

#endif /* __GCUT_MILTER_EVENT_LOOP_H__ */

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
