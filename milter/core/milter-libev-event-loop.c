/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Copyright (C) 2010, 2011  Nobuyoshi Nakada <nakada@clear-code.com>
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

typedef struct _MilterLibevEventLoopPrivate	MilterLibevEventLoopPrivate;
struct _MilterLibevEventLoopPrivate
{
    struct ev_loop *ev_loop;
    guint tag;
    GHashTable *callbacks;
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

static void     destroy_callback (gpointer         callback);

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
milter_libev_event_loop_class_init (MilterLibevEventLoopClass *klass)
{
    GObjectClass *gobject_class;
    GParamSpec *spec;

    gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->dispose      = dispose;
    gobject_class->set_property = set_property;
    gobject_class->get_property = get_property;

    klass->parent_class.run = run;
    klass->parent_class.iterate = iterate;
    klass->parent_class.quit = quit;
    klass->parent_class.watch_io = watch_io;
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
    priv->tag = 0;
    priv->callbacks = g_hash_table_new_full(g_direct_hash, g_direct_equal,
                                            NULL, destroy_callback);
}

static void
dispose (GObject *object)
{
    MilterLibevEventLoopPrivate *priv;

    if (default_event_loop == MILTER_EVENT_LOOP(object)) {
        default_event_loop = NULL;
    }

    priv = MILTER_LIBEV_EVENT_LOOP_GET_PRIVATE(object);

    if (priv->callbacks) {
        g_hash_table_destroy(priv->callbacks);
        priv->callbacks = NULL;
    }

    if (priv->ev_loop) {
        ev_loop_destroy(priv->ev_loop);
        priv->ev_loop = NULL;
    }

    G_OBJECT_CLASS(milter_libev_event_loop_parent_class)->dispose(object);
}

static void
set_property (GObject      *object,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
    MilterLibevEventLoopPrivate *priv;

    priv = MILTER_LIBEV_EVENT_LOOP_GET_PRIVATE(object);
    switch (prop_id) {
    case PROP_EV_LOOP:
        if (priv->ev_loop)
            ev_loop_destroy(priv->ev_loop);
        priv->ev_loop = g_value_get_pointer(value);
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

typedef void (*CallbackFunc) (struct ev_loop *loop, gpointer data);

struct callback_funcs {
    CallbackFunc stop;
};

typedef union {
    struct {
        MilterLibevEventLoop *loop;
        const struct callback_funcs *funcs;
        guint tag;
    } header;
    double alignment_double;
#ifdef HAVE_LONG_LONG
    long long alignment_long_long;
#endif
#ifdef HAVE_LONG_DOUBLE
    long double alignment_long_double;
#endif
} callback_header;

static guint
add_callback (MilterLibevEventLoopPrivate *priv, gpointer data)
{
    guint tag = ++priv->tag;
    callback_header *header = data;
    --header;
    header->header.tag = tag;
    g_hash_table_insert(priv->callbacks, GUINT_TO_POINTER(tag), data);
    return tag;
}

static gpointer
alloc_callback (MilterEventLoop *loop,
                const struct callback_funcs *funcs,
                gsize size)
{
    callback_header *header = g_malloc0(size + sizeof(*header));
    header->header.loop = (MilterLibevEventLoop *)loop;
    header->header.funcs = funcs;
    return header + 1;
}

#define new_callback(type) alloc_callback((loop), &type##_callback_funcs, sizeof(struct type##_callback_data))
#define callback_get_loop(data) (((const callback_header *)(data)-1)->header.loop)
#define callback_get_funcs(data) (((const callback_header *)(data)-1)->header.funcs)
#define callback_get_tag(data) (((const callback_header *)(data)-1)->header.tag)

static void
destroy_callback (gpointer data)
{
    callback_header *header = data;
    MilterLibevEventLoopPrivate *priv;
    const struct callback_funcs *funcs;
    --header;
    priv = MILTER_LIBEV_EVENT_LOOP_GET_PRIVATE(header->header.loop);
    funcs = header->header.funcs;
    funcs->stop(priv->ev_loop, data);
    g_free(header);
}

static void
remove_callback (MilterLibevEventLoop *loop, guint tag)
{
    MilterLibevEventLoopPrivate *priv;

    priv = MILTER_LIBEV_EVENT_LOOP_GET_PRIVATE(loop);
    g_hash_table_remove(priv->callbacks, GUINT_TO_POINTER(tag));
}

static void
run (MilterEventLoop *loop)
{
    MilterLibevEventLoopPrivate *priv;

    priv = MILTER_LIBEV_EVENT_LOOP_GET_PRIVATE(loop);
    ev_run(priv->ev_loop, 0);
}

static gboolean
iterate (MilterEventLoop *loop, gboolean may_block)
{
    MilterLibevEventLoopPrivate *priv;

    priv = MILTER_LIBEV_EVENT_LOOP_GET_PRIVATE(loop);
    if (may_block) {
        ev_run(priv->ev_loop, EVRUN_ONCE);
    } else {
        ev_run(priv->ev_loop, EVRUN_NOWAIT | EVRUN_ONCE);
    }
    return TRUE;
}

static void
quit (MilterEventLoop *loop)
{
    MilterLibevEventLoopPrivate *priv;

    priv = MILTER_LIBEV_EVENT_LOOP_GET_PRIVATE(loop);
    ev_break(priv->ev_loop, EVBREAK_ONE);
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

struct io_callback_data {
    ev_io event;
    GIOChannel *channel;
    GIOFunc function;
    gpointer user_data;
};

static void
io_func (struct ev_loop *loop, ev_io *watcher, int revents)
{
    struct io_callback_data *cb = (struct io_callback_data *)watcher;
    MilterLibevEventLoop *milter_event_loop = callback_get_loop(cb);
    guint tag = callback_get_tag(cb);

    if (!cb->function(cb->channel, evcond_to_g_io_condition(revents), cb->user_data)) {
        remove_callback(milter_event_loop, tag);
    }
}

static const struct callback_funcs io_callback_funcs = {
    (CallbackFunc)ev_io_stop,
};

static guint
watch_io (MilterEventLoop *loop,
          GIOChannel      *channel,
          GIOCondition     condition,
          GIOFunc          function,
          gpointer         data)
{
    int fd;
    struct io_callback_data *cb;
    MilterLibevEventLoopPrivate *priv;

    fd = g_io_channel_unix_get_fd(channel);
    if (fd == -1)
        return 0;

    priv = MILTER_LIBEV_EVENT_LOOP_GET_PRIVATE(loop);
    cb = new_callback(io);
    cb->channel = channel;
    cb->function = function;
    cb->user_data = data;
    ev_io_init(&cb->event, io_func, fd, evcond_from_g_io_condition(condition));
    ev_io_start(priv->ev_loop, &cb->event);
    return add_callback(priv, cb);
}

struct child_callback_data {
    ev_child event;
    GChildWatchFunc  function;
    gpointer         user_data;
    GDestroyNotify   notify;
};

static void
child_func (struct ev_loop *loop, ev_child *watcher, int revents)
{
    struct child_callback_data *cb = (struct child_callback_data *)watcher;
    MilterLibevEventLoop *milter_event_loop = callback_get_loop(cb);
    guint tag = callback_get_tag(cb);

    cb->function((GPid)watcher->rpid, watcher->rstatus, cb->user_data);
    remove_callback(milter_event_loop, tag);
}

static const struct callback_funcs child_callback_funcs = {
    (CallbackFunc)ev_child_stop,
};

static guint
watch_child_full (MilterEventLoop *loop,
                  gint             priority,
                  GPid             pid,
                  GChildWatchFunc  function,
                  gpointer         data,
                  GDestroyNotify   notify)
{
    struct child_callback_data *cb;
    MilterLibevEventLoopPrivate *priv;

    priv = MILTER_LIBEV_EVENT_LOOP_GET_PRIVATE(loop);
    cb = new_callback(child);
    cb->function = function;
    cb->user_data = data;
    cb->notify = notify;
    ev_child_init(&cb->event, child_func, pid, FALSE);
    ev_child_start(priv->ev_loop, &cb->event);
    return add_callback(priv, cb);
}

struct timer_callback_data {
    ev_timer event;
    GSourceFunc function;
    gpointer user_data;
};

static void
timer_func (struct ev_loop *loop, ev_timer *watcher, int revents)
{
    struct timer_callback_data *cb = (struct timer_callback_data *)watcher;
    MilterLibevEventLoop *milter_event_loop = callback_get_loop(cb);
    guint tag = callback_get_tag(cb);

    if (!cb->function(cb->user_data)) {
        remove_callback(milter_event_loop, tag);
    }
}

static const struct callback_funcs timer_callback_funcs = {
    (CallbackFunc)ev_timer_stop,
};

static guint
add_timeout_full (MilterEventLoop *loop,
                  gint             priority,
                  gdouble          interval_in_seconds,
                  GSourceFunc      function,
                  gpointer         data,
                  GDestroyNotify   notify)
{
    struct timer_callback_data *cb;
    MilterLibevEventLoopPrivate *priv;

    priv = MILTER_LIBEV_EVENT_LOOP_GET_PRIVATE(loop);
    if (interval_in_seconds < 0) return 0;
    cb = new_callback(timer);
    cb->function = function;
    cb->user_data = data;
    ev_timer_init(&cb->event, timer_func, interval_in_seconds, interval_in_seconds);
    ev_timer_start(priv->ev_loop, &cb->event);
    return add_callback(priv, cb);
}

struct idle_callback_data {
    ev_idle event;
    GSourceFunc function;
    gpointer user_data;
};

static void
idle_func (struct ev_loop *loop, ev_idle *watcher, int revents)
{
    struct idle_callback_data *cb = (struct idle_callback_data *)watcher;
    MilterLibevEventLoop *milter_event_loop = callback_get_loop(cb);
    guint tag = callback_get_tag(cb);

    if (!cb->function(cb->user_data)) {
        remove_callback(milter_event_loop, tag);
    }
}

static const struct callback_funcs idle_callback_funcs = {
    (CallbackFunc)ev_idle_stop,
};

static guint
add_idle_full (MilterEventLoop *loop,
               gint             priority,
               GSourceFunc      function,
               gpointer         data,
               GDestroyNotify   notify)
{
    struct idle_callback_data *cb;
    MilterLibevEventLoopPrivate *priv;

    priv = MILTER_LIBEV_EVENT_LOOP_GET_PRIVATE(loop);
    cb = new_callback(idle);
    cb->function = function;
    cb->user_data = data;
    ev_idle_init(&cb->event, idle_func);
    ev_idle_start(priv->ev_loop, &cb->event);
    return add_callback(priv, cb);
}

static gboolean
remove (MilterEventLoop *loop,
        guint            tag)
{
    MilterLibevEventLoopPrivate *priv;

    priv = MILTER_LIBEV_EVENT_LOOP_GET_PRIVATE(loop);
    return g_hash_table_remove(priv->callbacks, GUINT_TO_POINTER(tag));
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
