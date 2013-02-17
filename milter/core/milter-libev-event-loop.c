/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Copyright (C) 2010, 2011  Nobuyoshi Nakada <nakada@clear-code.com>
 *  Copyright (C) 2011-2013  Kouhei Sutou <kou@clear-code.com>
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

#include "milter-libev-event-loop.h"
#include "milter-logger.h"
#include <math.h>
#include <ev.h>

#define MILTER_LIBEV_EVENT_LOOP_GET_PRIVATE(obj)                \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj),                         \
                                 MILTER_TYPE_LIBEV_EVENT_LOOP,  \
                                 MilterLibevEventLoopPrivate))

G_DEFINE_TYPE(MilterLibevEventLoop, milter_libev_event_loop, MILTER_TYPE_EVENT_LOOP)

typedef void (*LibevCallbackFunc) (ev_loop *ev_loop);

typedef struct _MilterLibevEventLoopPrivate	MilterLibevEventLoopPrivate;
struct _MilterLibevEventLoopPrivate
{
    ev_loop *ev_loop;
    guint id;
    GHashTable *watchers;
    guint n_called;
    GFunc release_func;
    GFunc acquire_func;
    gpointer release_data;
    GDestroyNotify release_notify;
    ev_async *breaker;
};

enum
{
    PROP_0,
    PROP_EV_LOOP
};

static MilterEventLoop *default_event_loop = NULL;

static void     dispose          (GObject         *object);
static void     set_property     (GObject         *object,
                                  guint            prop_id,
                                  const GValue    *value,
                                  GParamSpec      *pspec);
static void     get_property     (GObject         *object,
                                  guint            prop_id,
                                  GValue          *value,
                                  GParamSpec      *pspec);

static void     destroy_watcher  (gpointer         data);

static gboolean iterate          (MilterEventLoop *loop,
                                  gboolean         may_block);
static void     quit             (MilterEventLoop *loop);

static guint    watch_io_full    (MilterEventLoop *loop,
                                  gint             priority,
                                  GIOChannel      *channel,
                                  GIOCondition     condition,
                                  GIOFunc          function,
                                  gpointer         data,
                                  GDestroyNotify   notify);

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
                                  guint            id);

static void
milter_libev_event_loop_class_init (MilterLibevEventLoopClass *klass)
{
    GObjectClass *gobject_class;
    GParamSpec *spec;

    gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->dispose      = dispose;
    gobject_class->set_property = set_property;
    gobject_class->get_property = get_property;

    klass->parent_class.iterate = iterate;
    klass->parent_class.quit = quit;
    klass->parent_class.watch_io_full = watch_io_full;
    klass->parent_class.watch_child_full = watch_child_full;
    klass->parent_class.add_timeout_full = add_timeout_full;
    klass->parent_class.add_idle_full = add_idle_full;
    klass->parent_class.remove = remove;

    spec = g_param_spec_pointer("ev-loop",
                                "EV loop",
                                "The ev_loop instance for the event loop",
                                G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    g_object_class_install_property(gobject_class, PROP_EV_LOOP, spec);

    g_type_class_add_private(gobject_class, sizeof(MilterLibevEventLoopPrivate));
}

static void
milter_libev_event_loop_init (MilterLibevEventLoop *loop)
{
    MilterLibevEventLoopPrivate *priv;

    priv = MILTER_LIBEV_EVENT_LOOP_GET_PRIVATE(loop);
    priv->ev_loop = NULL;
    priv->id = 0;
    priv->watchers = g_hash_table_new_full(g_direct_hash, g_direct_equal,
                                           NULL, destroy_watcher);
    priv->n_called = 0;
    priv->release_func = NULL;
    priv->acquire_func = NULL;
    priv->release_data = NULL;
    priv->release_notify = NULL;
    priv->breaker = NULL;
}

static void
dispose_release (MilterLibevEventLoopPrivate *priv)
{
    if (priv->release_data && priv->release_notify) {
        priv->release_notify(priv->release_data);
    }
    priv->release_data = NULL;
    priv->release_notify = NULL;
}

static void
cb_break (ev_loop *ev_loop, ev_async *watcher, int events)
{
    ev_break(ev_loop, EVBREAK_ONE);
}

static void
setup_ev_loop (MilterLibevEventLoop *loop)
{
    MilterLibevEventLoopPrivate *priv;

    priv = MILTER_LIBEV_EVENT_LOOP_GET_PRIVATE(loop);
    if (priv->ev_loop) {
        ev_set_userdata(priv->ev_loop, loop);
        priv->breaker = g_new0(ev_async, 1);
        ev_async_init(priv->breaker, cb_break);
        ev_async_start(priv->ev_loop, priv->breaker);
    }
}

static void
dispose_ev_loop (MilterLibevEventLoopPrivate *priv)
{
    dispose_release(priv);
    if (priv->ev_loop) {
        if (priv->breaker) {
            ev_async_stop(priv->ev_loop, priv->breaker);
            g_free(priv->breaker);
            priv->breaker = NULL;
        }
        ev_set_userdata(priv->ev_loop, NULL);
        ev_loop_destroy(priv->ev_loop);
        priv->ev_loop = NULL;
    }
}

static void
dispose (GObject *object)
{
    MilterLibevEventLoopPrivate *priv;

    if (default_event_loop == MILTER_EVENT_LOOP(object)) {
        default_event_loop = NULL;
    }

    priv = MILTER_LIBEV_EVENT_LOOP_GET_PRIVATE(object);

    if (priv->watchers) {
        g_hash_table_unref(priv->watchers);
        priv->watchers = NULL;
    }

    dispose_ev_loop(priv);

    G_OBJECT_CLASS(milter_libev_event_loop_parent_class)->dispose(object);
}

static void
set_property (GObject      *object,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
    MilterLibevEventLoop *loop;
    MilterLibevEventLoopPrivate *priv;

    loop = MILTER_LIBEV_EVENT_LOOP(object);
    priv = MILTER_LIBEV_EVENT_LOOP_GET_PRIVATE(loop);
    switch (prop_id) {
    case PROP_EV_LOOP:
        dispose_ev_loop(priv);
        priv->ev_loop = g_value_get_pointer(value);
        setup_ev_loop(loop);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static void
get_property (GObject    *object,
              guint       prop_id,
              GValue     *value,
              GParamSpec *pspec)
{
    MilterLibevEventLoopPrivate *priv;

    priv = MILTER_LIBEV_EVENT_LOOP_GET_PRIVATE(object);
    switch (prop_id) {
    case PROP_EV_LOOP:
        g_value_set_pointer(value, priv->ev_loop);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

MilterEventLoop *
milter_libev_event_loop_default (void)
{
    if (!default_event_loop) {
        default_event_loop =
            g_object_new(MILTER_TYPE_LIBEV_EVENT_LOOP,
                         "ev-loop", ev_default_loop(EVFLAG_FORKCHECK),
                         NULL);
    } else {
        g_object_ref(default_event_loop);
    }

    return default_event_loop;
}

MilterEventLoop *
milter_libev_event_loop_new (void)
{
    return g_object_new(MILTER_TYPE_LIBEV_EVENT_LOOP,
                        "ev-loop", ev_loop_new(EVFLAG_FORKCHECK),
                        NULL);
}

#define WATCHER_STOP_FUNC(func)          ((WatcherStopFunc)(func))
#define WATCHER_DESTROY_FUNC(func)       ((WatcherDestroyFunc)(func))
#define WATCHER_PRIVATE(object)          ((WatcherPrivate *)(object))

typedef void (*WatcherStopFunc) (ev_loop *loop, ev_watcher *watcher);
typedef void (*WatcherDestroyFunc) (ev_watcher *watcher);

typedef struct _WatcherPrivate WatcherPrivate;
struct _WatcherPrivate
{
    MilterLibevEventLoop *loop;
    guint id;
    WatcherStopFunc stop_func;
    WatcherDestroyFunc destroy_func;
    GDestroyNotify notify;
    gpointer user_data;
};

static guint
add_watcher (MilterLibevEventLoop *loop, ev_watcher *watcher,
             WatcherStopFunc stop_func, WatcherDestroyFunc destroy_func,
             GDestroyNotify notify, gpointer user_data)
{
    MilterLibevEventLoopPrivate *priv;
    WatcherPrivate *watcher_priv;

    priv = MILTER_LIBEV_EVENT_LOOP_GET_PRIVATE(loop);

    while (TRUE) {
        priv->id++;

        if (priv->id == 0) {
            continue;
        }
        if (g_hash_table_lookup_extended(priv->watchers,
                                         GUINT_TO_POINTER(priv->id),
                                         NULL, NULL)) {
            continue;
        }
        break;
    }

    watcher_priv = watcher->data;
    watcher_priv->loop = loop;
    watcher_priv->id = priv->id;
    watcher_priv->stop_func = stop_func;
    watcher_priv->destroy_func = destroy_func;
    watcher_priv->notify = notify;
    watcher_priv->user_data = user_data;

    g_hash_table_insert(priv->watchers, GUINT_TO_POINTER(priv->id), watcher);

    return priv->id;
}

static void
destroy_watcher (gpointer data)
{
    ev_watcher *watcher = data;
    WatcherPrivate *watcher_priv;
    MilterLibevEventLoopPrivate *priv;

    watcher_priv = watcher->data;
    priv = MILTER_LIBEV_EVENT_LOOP_GET_PRIVATE(watcher_priv->loop);
    watcher_priv->stop_func(priv->ev_loop, watcher);

    if (watcher_priv->notify)
        watcher_priv->notify(watcher_priv->user_data);

    if (watcher_priv->destroy_func)
        watcher_priv->destroy_func(watcher);
    g_free(watcher_priv);
    g_free(watcher);
}

static gboolean
remove_watcher (MilterLibevEventLoop *loop, guint id)
{
    MilterLibevEventLoopPrivate *priv;

    priv = MILTER_LIBEV_EVENT_LOOP_GET_PRIVATE(loop);
    return g_hash_table_remove(priv->watchers, GUINT_TO_POINTER(id));
}

static gboolean
iterate (MilterEventLoop *loop, gboolean may_block)
{
    MilterLibevEventLoopPrivate *priv;

    priv = MILTER_LIBEV_EVENT_LOOP_GET_PRIVATE(loop);
    priv->n_called = 0;
    if (may_block) {
        ev_run(priv->ev_loop, EVRUN_ONCE);
    } else {
        ev_run(priv->ev_loop, EVRUN_NOWAIT | EVRUN_ONCE);
    }
    return priv->n_called > 0;
}

static void
quit (MilterEventLoop *loop)
{
    MilterLibevEventLoopPrivate *priv;

    priv = MILTER_LIBEV_EVENT_LOOP_GET_PRIVATE(loop);
    ev_async_send(priv->ev_loop, priv->breaker);
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
    if (condition & (G_IO_ERR | G_IO_HUP | G_IO_NVAL)) {
        event_condition |= EV_ERROR;
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
    if (event & EV_ERROR) {
        condition |= (G_IO_ERR | G_IO_HUP | G_IO_NVAL);
    }

    return condition;
}

typedef struct _IOWatcherPrivate IOWatcherPrivate;
struct _IOWatcherPrivate {
    WatcherPrivate priv;
    GIOChannel *channel;
    GIOFunc function;
};

static void
io_watcher_destroy (ev_watcher *watcher)
{
    IOWatcherPrivate *watcher_priv;

    watcher_priv = watcher->data;
    g_io_channel_unref(watcher_priv->channel);
}

static void
io_func (struct ev_loop *loop, ev_io *watcher, int revents)
{
    IOWatcherPrivate *io_watcher_priv;
    WatcherPrivate *watcher_priv;
    MilterLibevEventLoop *milter_event_loop;
    guint id;

    io_watcher_priv = watcher->data;
    watcher_priv = WATCHER_PRIVATE(io_watcher_priv);

    milter_event_loop = watcher_priv->loop;
    MILTER_LIBEV_EVENT_LOOP_GET_PRIVATE(milter_event_loop)->n_called++;
    id = watcher_priv->id;
    if (!io_watcher_priv->function(io_watcher_priv->channel,
                                   evcond_to_g_io_condition(revents),
                                   watcher_priv->user_data)) {
        remove_watcher(milter_event_loop, id);
    }
}

static guint
watch_io_full (MilterEventLoop *loop,
               gint             priority,
               GIOChannel      *channel,
               GIOCondition     condition,
               GIOFunc          function,
               gpointer         user_data,
               GDestroyNotify   notify)
{
    guint id;
    int fd;
    ev_io *watcher;
    IOWatcherPrivate *watcher_priv;
    MilterLibevEventLoopPrivate *priv;

    fd = g_io_channel_unix_get_fd(channel);
    if (fd == -1)
        return 0;

    priv = MILTER_LIBEV_EVENT_LOOP_GET_PRIVATE(loop);

    watcher_priv = g_new0(IOWatcherPrivate, 1);
    watcher_priv->channel = channel;
    g_io_channel_ref(watcher_priv->channel);
    watcher_priv->function = function;

    watcher = g_new0(ev_io, 1);
    watcher->data = watcher_priv;
    id = add_watcher(MILTER_LIBEV_EVENT_LOOP(loop),
                      (ev_watcher *)watcher,
                      WATCHER_STOP_FUNC(ev_io_stop),
                      io_watcher_destroy,
                      notify, user_data);

    ev_io_init(watcher, io_func, fd, evcond_from_g_io_condition(condition));
    ev_io_start(priv->ev_loop, watcher);

    return id;
}

typedef struct _ChildWatcherPrivate ChildWatcherPrivate;
struct _ChildWatcherPrivate {
    WatcherPrivate   priv;
    GChildWatchFunc  function;
};

static void
child_func (struct ev_loop *loop, ev_child *watcher, int revents)
{
    ChildWatcherPrivate *child_watcher_priv;
    WatcherPrivate *watcher_priv;
    MilterLibevEventLoop *milter_event_loop;
    guint id;

    child_watcher_priv = watcher->data;
    watcher_priv = WATCHER_PRIVATE(child_watcher_priv);

    milter_event_loop = watcher_priv->loop;
    MILTER_LIBEV_EVENT_LOOP_GET_PRIVATE(milter_event_loop)->n_called++;
    id = watcher_priv->id;
    child_watcher_priv->function((GPid)(watcher->rpid),
                                 watcher->rstatus,
                                 watcher_priv->user_data);
    remove_watcher(milter_event_loop, id);
}

static guint
watch_child_full (MilterEventLoop *loop,
                  gint             priority,
                  GPid             pid,
                  GChildWatchFunc  function,
                  gpointer         data,
                  GDestroyNotify   notify)
{
    guint id;
    ev_child *watcher;
    ChildWatcherPrivate *watcher_priv;
    MilterLibevEventLoopPrivate *priv;

    priv = MILTER_LIBEV_EVENT_LOOP_GET_PRIVATE(loop);

    watcher_priv = g_new0(ChildWatcherPrivate, 1);
    watcher_priv->function = function;

    watcher = g_new0(ev_child, 1);
    watcher->data = watcher_priv;
    id = add_watcher(MILTER_LIBEV_EVENT_LOOP(loop),
                      (ev_watcher *)watcher,
                      WATCHER_STOP_FUNC(ev_child_stop),
                      NULL,
                      notify, data);

    ev_child_init(watcher, child_func, pid, FALSE);
    ev_child_start(priv->ev_loop, watcher);

    return id;
}

typedef struct _TimerWatcherPrivate TimerWatcherPrivate;
struct _TimerWatcherPrivate {
    WatcherPrivate priv;
    GSourceFunc function;
};

static void
timer_func (struct ev_loop *loop, ev_timer *watcher, int revents)
{
    TimerWatcherPrivate *timer_watcher_priv;
    WatcherPrivate *watcher_priv;
    MilterLibevEventLoop *milter_event_loop;
    guint id;

    timer_watcher_priv = watcher->data;
    watcher_priv = WATCHER_PRIVATE(timer_watcher_priv);

    milter_event_loop = watcher_priv->loop;
    MILTER_LIBEV_EVENT_LOOP_GET_PRIVATE(milter_event_loop)->n_called++;
    id = watcher_priv->id;
    if (!timer_watcher_priv->function(watcher_priv->user_data)) {
        remove_watcher(milter_event_loop, id);
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
    guint id;
    ev_timer *watcher;
    TimerWatcherPrivate *watcher_priv;
    MilterLibevEventLoopPrivate *priv;

    priv = MILTER_LIBEV_EVENT_LOOP_GET_PRIVATE(loop);
    if (interval_in_seconds < 0)
        return 0;

    watcher_priv = g_new0(TimerWatcherPrivate, 1);
    watcher_priv->function = function;

    watcher = g_new0(ev_timer, 1);
    watcher->data = watcher_priv;
    id = add_watcher(MILTER_LIBEV_EVENT_LOOP(loop),
                      (ev_watcher *)watcher,
                      WATCHER_STOP_FUNC(ev_timer_stop),
                      NULL,
                      notify, data);

    ev_timer_init(watcher, timer_func, interval_in_seconds, interval_in_seconds);
    ev_timer_start(priv->ev_loop, watcher);

    return id;
}

typedef struct _IdleWatcherPrivate IdleWatcherPrivate;
struct _IdleWatcherPrivate {
    WatcherPrivate priv;
    GSourceFunc    function;
};


static void
idle_func (struct ev_loop *loop, ev_idle *watcher, int revents)
{
    IdleWatcherPrivate *idle_watcher_priv;
    WatcherPrivate *watcher_priv;
    MilterLibevEventLoop *milter_event_loop;
    guint id;

    idle_watcher_priv = watcher->data;
    watcher_priv = WATCHER_PRIVATE(idle_watcher_priv);

    milter_event_loop = watcher_priv->loop;
    MILTER_LIBEV_EVENT_LOOP_GET_PRIVATE(milter_event_loop)->n_called++;
    id = watcher_priv->id;
    if (!idle_watcher_priv->function(watcher_priv->user_data)) {
        remove_watcher(milter_event_loop, id);
    }
}

static guint
add_idle_full (MilterEventLoop *loop,
               gint             priority,
               GSourceFunc      function,
               gpointer         data,
               GDestroyNotify   notify)
{
    guint id;
    ev_idle *watcher;
    IdleWatcherPrivate *watcher_priv;
    MilterLibevEventLoopPrivate *priv;

    priv = MILTER_LIBEV_EVENT_LOOP_GET_PRIVATE(loop);

    watcher_priv = g_new0(IdleWatcherPrivate, 1);
    watcher_priv->function = function;

    watcher = g_new0(ev_idle, 1);
    watcher->data = watcher_priv;
    id = add_watcher(MILTER_LIBEV_EVENT_LOOP(loop),
                      (ev_watcher *)watcher,
                      WATCHER_STOP_FUNC(ev_idle_stop),
                      NULL,
                      notify, data);

    ev_idle_init(watcher, idle_func);
    ev_idle_start(priv->ev_loop, watcher);

    return id;
}

static gboolean
remove (MilterEventLoop *loop,
        guint            id)
{
    return remove_watcher(MILTER_LIBEV_EVENT_LOOP(loop), id);
}

static void
cb_release (ev_loop *ev_loop)
{
    MilterEventLoop *loop;
    MilterLibevEventLoopPrivate *priv;

    loop = ev_userdata(ev_loop);
    priv = MILTER_LIBEV_EVENT_LOOP_GET_PRIVATE(loop);
    priv->release_func(loop, priv->release_data);
}

static void
cb_acquire (ev_loop *ev_loop)
{
    MilterEventLoop *loop;
    MilterLibevEventLoopPrivate *priv;

    loop = ev_userdata(ev_loop);
    priv = MILTER_LIBEV_EVENT_LOOP_GET_PRIVATE(loop);
    priv->acquire_func(loop, priv->release_data);
}

void
milter_libev_event_loop_set_release_func (MilterEventLoop *loop,
                                          GFunc            release,
                                          GFunc            acquire,
                                          gpointer         data,
                                          GDestroyNotify   notify)
{
    MilterLibevEventLoopPrivate *priv;
    LibevCallbackFunc release_func = NULL;
    LibevCallbackFunc acquire_func = NULL;

    priv = MILTER_LIBEV_EVENT_LOOP_GET_PRIVATE(loop);
    dispose_release(priv);
    priv->release_func = release;
    priv->acquire_func = acquire;
    if (priv->release_func)
        release_func = cb_release;
    if (priv->acquire_func)
        acquire_func = cb_acquire;
    ev_set_loop_release_cb(priv->ev_loop, release_func, acquire_func);
    priv->release_data = data;
    priv->release_notify = notify;
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
