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

#include "milter-g-event-loop.h"

#define MILTER_G_EVENT_LOOP_GET_PRIVATE(obj)              \
  (G_TYPE_INSTANCE_GET_PRIVATE((obj),                   \
                               MILTER_TYPE_G_EVENT_LOOP,  \
                               MilterGEventLoopPrivate))

G_DEFINE_TYPE(MilterGEventLoop, milter_g_event_loop, MILTER_TYPE_EVENT_LOOP)

typedef struct _MilterGEventLoopPrivate	MilterGEventLoopPrivate;
struct _MilterGEventLoopPrivate
{
  gint dummy;
};

enum
{
    PROP_0,
    PROP_LAST
};

static GObject *constructor  (GType                  type,
                              guint                  n_props,
                              GObjectConstructParam *props);

static guint add_watch       (MilterEventLoop *eventloop,
                              GIOChannel      *channel,
                              GIOCondition     condition,
                              GIOFunc          func,
                              gpointer         user_data);

static guint add_timeout     (MilterEventLoop *eventloop,
                              gdouble interval,
                              GSourceFunc func,
                              gpointer user_data);

static guint add_idle_full (MilterEventLoop *eventloop,
                            gint             priority,
                            GSourceFunc      function,
                            gpointer         data,
                            GDestroyNotify   notify);

static void
milter_g_event_loop_class_init (MilterGEventLoopClass *klass)
{
    GObjectClass *gobject_class;

    gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->constructor  = constructor;

    klass->parent_class.add_watch = add_watch;
    klass->parent_class.add_timeout = add_timeout;
    klass->parent_class.add_idle_full = add_idle_full;

    g_type_class_add_private(gobject_class, sizeof(MilterGEventLoopPrivate));
}

static GObject *
constructor (GType type, guint n_props, GObjectConstructParam *props)
{
    GObject *object;
    MilterGEventLoop *eventloop;
    GObjectClass *klass;
    MilterGEventLoopClass *eventloop_class;
    MilterGEventLoopPrivate *priv;

    klass = G_OBJECT_CLASS(milter_g_event_loop_parent_class);
    object = klass->constructor(type, n_props, props);

    priv = MILTER_G_EVENT_LOOP_GET_PRIVATE(object);

    eventloop = MILTER_G_EVENT_LOOP(object);
    eventloop_class = MILTER_G_EVENT_LOOP_GET_CLASS(object);

    priv->dummy = 0;

    return object;
}

static void
milter_g_event_loop_init (MilterGEventLoop *eventloop)
{
    MilterGEventLoopPrivate *priv;

    priv = MILTER_G_EVENT_LOOP_GET_PRIVATE(eventloop);
    priv->dummy = -1;
}

static guint
add_watch (MilterEventLoop *eventloop,
           GIOChannel      *channel,
           GIOCondition     condition,
           GIOFunc          func,
           gpointer         user_data)
{
    return g_io_add_watch(channel, condition, func, user_data);
}

static guint
add_timeout (MilterEventLoop *eventloop,
             gdouble interval,
             GSourceFunc func,
             gpointer user_data)
{
    return g_timeout_add(interval * 1000,
                         func, user_data);
}

static guint
add_idle_full (MilterEventLoop *eventloop,
               gint             priority,
               GSourceFunc      function,
               gpointer         data,
               GDestroyNotify   notify)
{
    return g_idle_add_full (priority,
                            function,
                            data,
                            notify);
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
