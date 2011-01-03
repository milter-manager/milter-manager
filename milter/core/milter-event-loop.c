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
    klass->add_watch = NULL;
    klass->add_timeout = NULL;
    klass->add_idle_full = NULL;
    klass->remove_source = NULL;

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

void
milter_event_loop_quit (MilterEventLoop *loop)
{
    MilterEventLoopClass *loop_class;
    loop_class = MILTER_EVENT_LOOP_GET_CLASS(loop);
    loop_class->quit(loop);
}

guint
milter_event_loop_add_watch (MilterEventLoop *loop,
                             GIOChannel      *channel,
                             GIOCondition     condition,
                             GIOFunc          func,
                             gpointer         user_data)
{
    MilterEventLoopClass *loop_class;
    loop_class = MILTER_EVENT_LOOP_GET_CLASS(loop);
    return loop_class->add_watch(loop, channel, condition, func, user_data);
}

guint
milter_event_loop_add_timeout (MilterEventLoop *loop,
                               gdouble          interval_in_seconds,
                               GSourceFunc      function,
                               gpointer         user_data)
{
    MilterEventLoopClass *loop_class;
    loop_class = MILTER_EVENT_LOOP_GET_CLASS(loop);
    return loop_class->add_timeout(loop, interval_in_seconds,
                                   function, user_data);
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
milter_event_loop_remove_source (MilterEventLoop *loop,
                                 guint            tag)
{
    MilterEventLoopClass *loop_class;
    loop_class = MILTER_EVENT_LOOP_GET_CLASS(loop);
    return loop_class->remove_source(loop, tag);
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
