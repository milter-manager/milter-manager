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

#ifdef HAVE_CONFIG_H
#  include "../../config.h"
#endif /* HAVE_CONFIG_H */

#include "milter-libevent-event-loop.h"
#include <math.h>
#include <ev.h>

#define MILTER_LIBEVENT_EVENT_LOOP_GET_PRIVATE(obj)              \
  (G_TYPE_INSTANCE_GET_PRIVATE((obj),                   \
                               MILTER_TYPE_LIBEVENT_EVENT_LOOP,  \
                               MilterLibeventEventLoopPrivate))

G_DEFINE_TYPE(MilterLibeventEventLoop, milter_libevent_event_loop, MILTER_TYPE_EVENT_LOOP)

typedef struct _MilterLibeventEventLoopPrivate	MilterLibeventEventLoopPrivate;
struct _MilterLibeventEventLoopPrivate
{
    struct ev_loop *base;
    guint tag;
};

enum
{
    PROP_0,
    PROP_LAST
};

static void     dispose          (GObject         *object);
static void     constructed      (GObject         *object);

static void     run              (MilterEventLoop *loop);
static gboolean iterate          (MilterEventLoop *loop,
                                  gboolean         may_block);
static void     quit             (MilterEventLoop *loop);

static guint    watch_io         (MilterEventLoop *loop,
                                  GIOChannel      *channel,
                                  GIOCondition     condition,
                                  GIOFunc          function,
                                  gpointer         data);

static guint    watch_child_full (MilterEventLoop *loop,
                                  gint             priority,
                                  GPid             pid,
                                  GChildWatchFunc  function,
                                  gpointer         data,
                                  GDestroyNotify   notify);

static guint    add_timeout_full (MilterEventLoop *loop,
                                  gint             priority,
                                  gdouble          interval_in_seconds,
                                  GSourceFunc      function,
                                  gpointer         data,
                                  GDestroyNotify   notify);

static guint    add_idle_full    (MilterEventLoop *loop,
                                  gint             priority,
                                  GSourceFunc      function,
                                  gpointer         data,
                                  GDestroyNotify   notify);

static gboolean remove           (MilterEventLoop *loop,
                                  guint            tag);

static void
milter_libevent_event_loop_class_init (MilterLibeventEventLoopClass *klass)
{
    GObjectClass *gobject_class;

    gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->dispose      = dispose;
    gobject_class->constructed  = constructed;

    klass->parent_class.run = run;
    klass->parent_class.iterate = iterate;
    klass->parent_class.quit = quit;
    klass->parent_class.watch_io = watch_io;
    klass->parent_class.watch_child_full = watch_child_full;
    klass->parent_class.add_timeout_full = add_timeout_full;
    klass->parent_class.add_idle_full = add_idle_full;
    klass->parent_class.remove = remove;

    g_type_class_add_private(gobject_class, sizeof(MilterLibeventEventLoopPrivate));
}

static void
milter_libevent_event_loop_init (MilterLibeventEventLoop *loop)
{
    MilterLibeventEventLoopPrivate *priv;

    priv = MILTER_LIBEVENT_EVENT_LOOP_GET_PRIVATE(loop);
    priv->base = NULL;
    priv->tag = 0;
}

static void
dispose (GObject *object)
{
    MilterLibeventEventLoopPrivate *priv;

    priv = MILTER_LIBEVENT_EVENT_LOOP_GET_PRIVATE(object);

    if (priv->base) {
        ev_loop_destroy(priv->base);
        priv->base = NULL;
    }

    G_OBJECT_CLASS(milter_libevent_event_loop_parent_class)->dispose(object);
}

static void
constructed (GObject *object)
{
    MilterLibeventEventLoop *loop;
    MilterLibeventEventLoopPrivate *priv;

    loop = MILTER_LIBEVENT_EVENT_LOOP(object);
    priv = MILTER_LIBEVENT_EVENT_LOOP_GET_PRIVATE(loop);
#ifdef HAVE_LIBEV
    priv->base = ev_default_loop(0);
#else
    priv->base = event_init();
#endif
}

MilterEventLoop *
milter_libevent_event_loop_new (void)
{
    return g_object_new(MILTER_TYPE_LIBEVENT_EVENT_LOOP,
                        NULL);
}

static void
run (MilterEventLoop *loop)
{
    MilterLibeventEventLoopPrivate *priv;

    priv = MILTER_LIBEVENT_EVENT_LOOP_GET_PRIVATE(loop);
    ev_loop(priv->base, EVLOOP_NONBLOCK);
}

static gboolean
iterate (MilterEventLoop *loop, gboolean may_block)
{
    MilterLibeventEventLoopPrivate *priv;

    /* TODO: may_block */
    priv = MILTER_LIBEVENT_EVENT_LOOP_GET_PRIVATE(loop);
    ev_loop(priv->base, EVLOOP_ONESHOT);
    return TRUE;                /* TODO */
}

static void
quit (MilterEventLoop *loop)
{
    MilterLibeventEventLoopPrivate *priv;

    priv = MILTER_LIBEVENT_EVENT_LOOP_GET_PRIVATE(loop);
    ev_unloop(priv->base, EVUNLOOP_ONE);
}

static short
evcond_from_g_io_condition(GIOCondition condition)
{
    short event_condition = 0;
    if (condition & (G_IO_IN | G_IO_PRI)) {
        event_condition |= EV_READ;
    }
    if (condition & (G_IO_OUT)) {
        event_condition |= EV_WRITE;
    }
    return event_condition;
}

static GIOCondition
evcond_to_g_io_condition(short event)
{
    GIOCondition condition = 0;
    if (event & EV_READ) {
        condition |= G_IO_IN;
    }
    if (event & EV_WRITE) {
        condition |= G_IO_OUT;
    }
    return condition;
}

struct io_callback_data {
    ev_io event;
    MilterLibeventEventLoop *loop;
    GIOChannel *channel;
    GIOFunc function;
    void *user_data;
};

static void
watch_func (int fd, short event, void *data)
{
    struct io_callback_data *cb = data;
    MilterLibeventEventLoopPrivate *priv;

    priv = MILTER_LIBEVENT_EVENT_LOOP_GET_PRIVATE(cb->loop);
    if (!cb->function(cb->channel, evcond_to_g_io_condition(event), cb->user_data)) {
        ev_io_stop(priv->base, &cb->event);
        g_free(cb);
    }
}

static guint
watch_io (MilterEventLoop *loop,
          GIOChannel      *channel,
          GIOCondition     condition,
          GIOFunc          function,
          gpointer         data)
{
    int fd = g_io_channel_unix_get_fd(channel);
    struct io_callback_data *cb;
    MilterLibeventEventLoopPrivate *priv;

    priv = MILTER_LIBEVENT_EVENT_LOOP_GET_PRIVATE(loop);
    if (fd == -1) return 0;
    cb = g_malloc0(sizeof(*cb));
    cb->channel = channel;
    cb->function = function;
    cb->user_data = data;
    cb->loop = MILTER_LIBEVENT_EVENT_LOOP(loop);
    ev_init(&cb->event, watch_func);
    ev_io_set(&cb->event, fd,
              evcond_from_g_io_condition(condition));
    ev_set_userdata(&cb->event, cb);
    ev_io_start(priv->base, &cb->event);
    return ++priv->tag;
}

static guint
watch_child_full (MilterEventLoop *loop,
                  gint             priority,
                  GPid             pid,
                  GChildWatchFunc  function,
                  gpointer         data,
                  GDestroyNotify   notify)
{
    /* TODO: implement */
    return 0;
}

struct timer_callback_data {
    ev_timeout event;
    MilterLibeventEventLoop *loop;
    GSourceFunc function;
    void *user_data;
};

static void
timer_func (int fd, short event, void *data)
{
    struct timer_callback_data *cb = data;
    if (!cb->function(cb->user_data)) {
        event_del(&cb->event);
        g_free(cb);
    }
}

static guint
add_timeout_full (MilterEventLoop *loop,
                  gint             priority,
                  gdouble          interval_in_seconds,
                  GSourceFunc      function,
                  gpointer         data,
                  GDestroyNotify   notify)
{
    struct timer_callback_data *cb;
    struct timeval tv;
    MilterLibeventEventLoopPrivate *priv;

    priv = MILTER_LIBEVENT_EVENT_LOOP_GET_PRIVATE(loop);
    if (interval_in_seconds < 0) return 0;
    /* should use modf() and round() */
    tv.tv_sec = (time_t)interval_in_seconds;
    tv.tv_usec = (unsigned long)((interval_in_seconds - tv.tv_sec) * 1000);
    cb = g_malloc0(sizeof(*cb));
    cb->function = function;
    cb->user_data = data;
    cb->loop = MILTER_LIBEVENT_EVENT_LOOP(loop);
    evtimer_set(&cb->event, timer_func, cb);
    evtimer_add(&cb->event, &tv);
    return ++priv->tag;
}

static guint
add_idle_full (MilterEventLoop *loop,
               gint             priority,
               GSourceFunc      function,
               gpointer         data,
               GDestroyNotify   notify)
{
    /* TODO: implement */
    /* maybe using lowest priority timeout */
    return 0;
}

static gboolean
remove (MilterEventLoop *loop,
        guint            tag)
{
    return FALSE;
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
