/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Copyright (C) 2010  Nobuyoshi Nakada <nakada@clear-code.com>
 *  Copyright (C) 2011  Kouhei Sutou <nakada@clear-code.com>
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

#include "milter-event-loop.h"

#define MILTER_EVENT_LOOP_GET_PRIVATE(obj)              \
  (G_TYPE_INSTANCE_GET_PRIVATE((obj),                   \
                               MILTER_TYPE_EVENT_LOOP,  \
                               MilterEventLoopPrivate))

G_DEFINE_ABSTRACT_TYPE(MilterEventLoop, milter_event_loop, G_TYPE_OBJECT)

typedef struct _MilterEventLoopPrivate	MilterEventLoopPrivate;
struct _MilterEventLoopPrivate
{
    gboolean new_context;
};

enum
{
    PROP_0,
    PROP_LAST
};

static void     dispose        (GObject         *object);

static void
milter_event_loop_class_init (MilterEventLoopClass *klass)
{
    GObjectClass *gobject_class;

    gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->dispose      = dispose;

    klass->run = NULL;
    klass->quit = NULL;
    klass->watch_io = NULL;
    klass->add_timeout_full = NULL;
    klass->add_idle_full = NULL;
    klass->remove = NULL;

    g_type_class_add_private(gobject_class, sizeof(MilterEventLoopPrivate));
}

static void
milter_event_loop_init (MilterEventLoop *loop)
{
}

static void
dispose (GObject *object)
{
    MilterEventLoopPrivate *priv;

    priv = MILTER_EVENT_LOOP_GET_PRIVATE(object);

    G_OBJECT_CLASS(milter_event_loop_parent_class)->dispose(object);
}

GQuark
milter_event_loop_error_quark (void)
{
    return g_quark_from_static_string("milter-event-loop-error-quark");
}

void
milter_event_loop_run (MilterEventLoop *loop)
{
    MilterEventLoopClass *loop_class;
    loop_class = MILTER_EVENT_LOOP_GET_CLASS(loop);
    loop_class->run(loop);
}

gboolean
milter_event_loop_iterate (MilterEventLoop *loop, gboolean may_block)
{
    MilterEventLoopClass *loop_class;
    loop_class = MILTER_EVENT_LOOP_GET_CLASS(loop);
    return loop_class->iterate(loop, may_block);
}

void
milter_event_loop_quit (MilterEventLoop *loop)
{
    MilterEventLoopClass *loop_class;
    loop_class = MILTER_EVENT_LOOP_GET_CLASS(loop);
    loop_class->quit(loop);
}

guint
milter_event_loop_watch_io (MilterEventLoop *loop,
                            GIOChannel      *channel,
                            GIOCondition     condition,
                            GIOFunc          function,
                            gpointer         data)
{
    MilterEventLoopClass *loop_class;
    loop_class = MILTER_EVENT_LOOP_GET_CLASS(loop);
    return loop_class->watch_io(loop, channel, condition, function, data);
}

guint
milter_event_loop_add_timeout (MilterEventLoop *loop,
                               gdouble          interval_in_seconds,
                               GSourceFunc      function,
                               gpointer         data)
{
    return milter_event_loop_add_timeout_full(loop, G_PRIORITY_DEFAULT,
                                              interval_in_seconds,
                                              function, data,
                                              NULL);
}

guint
milter_event_loop_add_timeout_full (MilterEventLoop *loop,
                                    gint             priority,
                                    gdouble          interval_in_seconds,
                                    GSourceFunc      function,
                                    gpointer         data,
                                    GDestroyNotify   notify)
{
    MilterEventLoopClass *loop_class;
    loop_class = MILTER_EVENT_LOOP_GET_CLASS(loop);
    return loop_class->add_timeout_full(loop, priority, interval_in_seconds,
                                        function, data, notify);
}

guint
milter_event_loop_add_idle (MilterEventLoop *loop,
                            GSourceFunc      function,
                            gpointer         data)
{
    return milter_event_loop_add_idle_full(loop, G_PRIORITY_DEFAULT_IDLE,
                                           function, data, NULL);
}

guint
milter_event_loop_add_idle_full (MilterEventLoop *loop,
                                 gint             priority,
                                 GSourceFunc      function,
                                 gpointer         data,
                                 GDestroyNotify   notify)
{
    MilterEventLoopClass *loop_class;
    loop_class = MILTER_EVENT_LOOP_GET_CLASS(loop);
    return loop_class->add_idle_full(loop, priority, function, data, notify);
}

gboolean
milter_event_loop_remove (MilterEventLoop *loop, guint tag)
{
    MilterEventLoopClass *loop_class;
    loop_class = MILTER_EVENT_LOOP_GET_CLASS(loop);
    return loop_class->remove(loop, tag);
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
