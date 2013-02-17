/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Copyright (C) 2010  Nobuyoshi Nakada <nakada@clear-code.com>
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

#include "milter-glib-event-loop.h"

#define MILTER_GLIB_EVENT_LOOP_GET_PRIVATE(obj)                 \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj),                         \
                                 MILTER_TYPE_GLIB_EVENT_LOOP,   \
                                 MilterGLibEventLoopPrivate))

G_DEFINE_TYPE(MilterGLibEventLoop, milter_glib_event_loop,
              MILTER_TYPE_EVENT_LOOP)

typedef struct _MilterGLibEventLoopPrivate	MilterGLibEventLoopPrivate;
struct _MilterGLibEventLoopPrivate
{
    GMainContext *context;
    GMainLoop *loop;
};

enum
{
    PROP_0,
    PROP_CONTEXT,
    PROP_LAST
};

static GObject *constructor      (GType                  type,
                                  guint                  n_props,
                                  GObjectConstructParam *props);

static void     dispose          (GObject         *object);
static void     set_property     (GObject         *object,
                                  guint            prop_id,
                                  const GValue    *value,
                                  GParamSpec      *pspec);
static void     get_property     (GObject         *object,
                                  guint            prop_id,
                                  GValue          *value,
                                  GParamSpec      *pspec);

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
milter_glib_event_loop_class_init (MilterGLibEventLoopClass *klass)
{
    GObjectClass *gobject_class;
    GParamSpec *spec;

    gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->constructor  = constructor;
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

    spec = g_param_spec_pointer("context",
                                "Context",
                                "Use the GMainContext for the event loop",
                                G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    g_object_class_install_property(gobject_class, PROP_CONTEXT, spec);

    g_type_class_add_private(gobject_class, sizeof(MilterGLibEventLoopPrivate));
}

static GObject *
constructor (GType type, guint n_props, GObjectConstructParam *props)
{
    GObject *object;
    GObjectClass *klass;
    MilterGLibEventLoop *loop;
    MilterGLibEventLoopPrivate *priv;

    klass = G_OBJECT_CLASS(milter_glib_event_loop_parent_class);
    object = klass->constructor(type, n_props, props);

    loop = MILTER_GLIB_EVENT_LOOP(object);
    priv = MILTER_GLIB_EVENT_LOOP_GET_PRIVATE(loop);
    priv->loop = g_main_loop_new(priv->context, FALSE);

    return object;
}

static void
milter_glib_event_loop_init (MilterGLibEventLoop *loop)
{
    MilterGLibEventLoopPrivate *priv;

    priv = MILTER_GLIB_EVENT_LOOP_GET_PRIVATE(loop);
    priv->context = NULL;
    priv->loop = NULL;
}

static void
dispose (GObject *object)
{
    MilterGLibEventLoopPrivate *priv;

    priv = MILTER_GLIB_EVENT_LOOP_GET_PRIVATE(object);

    if (priv->loop) {
        g_main_loop_unref(priv->loop);
        priv->loop = NULL;
    }

    if (priv->context) {
        g_main_context_unref(priv->context);
        priv->context = NULL;
    }

    G_OBJECT_CLASS(milter_glib_event_loop_parent_class)->dispose(object);
}

static void
set_property (GObject      *object,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
    MilterGLibEventLoopPrivate *priv;

    priv = MILTER_GLIB_EVENT_LOOP_GET_PRIVATE(object);
    switch (prop_id) {
    case PROP_CONTEXT:
        if (priv->context)
            g_main_context_unref(priv->context);
        priv->context = g_value_get_pointer(value);
        if (priv->context)
            g_main_context_ref(priv->context);
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
    MilterGLibEventLoop *loop;
    MilterGLibEventLoopPrivate *priv;

    loop = MILTER_GLIB_EVENT_LOOP(object);
    priv = MILTER_GLIB_EVENT_LOOP_GET_PRIVATE(loop);
    switch (prop_id) {
    case PROP_CONTEXT:
        g_value_set_pointer(value, priv->context);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

MilterEventLoop *
milter_glib_event_loop_new (GMainContext *context)
{
    return g_object_new(MILTER_TYPE_GLIB_EVENT_LOOP,
                        "context", context,
                        NULL);
}

static gboolean
iterate (MilterEventLoop *loop, gboolean may_block)
{
    MilterGLibEventLoopPrivate *priv;

    priv = MILTER_GLIB_EVENT_LOOP_GET_PRIVATE(loop);
    return g_main_context_iteration(g_main_loop_get_context(priv->loop),
                                    may_block);
}

static guint
attach_source (MilterGLibEventLoopPrivate *priv, GSource *source)
{
    guint id;

    id = g_source_attach(source, g_main_loop_get_context(priv->loop));

    return id;
}

static void
quit (MilterEventLoop *loop)
{
    MilterGLibEventLoopPrivate *priv;

    priv = MILTER_GLIB_EVENT_LOOP_GET_PRIVATE(loop);
    g_main_context_wakeup(g_main_loop_get_context(priv->loop));
}

static guint
watch_io_full (MilterEventLoop *loop,
               gint             priority,
               GIOChannel      *channel,
               GIOCondition     condition,
               GIOFunc          function,
               gpointer         data,
               GDestroyNotify   notify)
{
    MilterGLibEventLoopPrivate *priv;
    guint id;
    GSource *source;

    priv = MILTER_GLIB_EVENT_LOOP_GET_PRIVATE(loop);
    source = g_io_create_watch(channel, condition);
    if (priority != G_PRIORITY_DEFAULT)
        g_source_set_priority(source, priority);
    g_source_set_callback(source, (GSourceFunc)function, data, notify);
    id = attach_source(priv, source);
    g_source_unref(source);

    return id;
}

static guint
watch_child_full (MilterEventLoop *loop,
                  gint             priority,
                  GPid             pid,
                  GChildWatchFunc  function,
                  gpointer         data,
                  GDestroyNotify   notify)
{
    MilterGLibEventLoopPrivate *priv;
    guint id;
    GSource *source;

    priv = MILTER_GLIB_EVENT_LOOP_GET_PRIVATE(loop);
    source = g_child_watch_source_new(pid);
    if (priority != G_PRIORITY_DEFAULT)
        g_source_set_priority(source, priority);
    g_source_set_callback(source, (GSourceFunc)function, data, notify);
    id = attach_source(priv, source);
    g_source_unref(source);

    return id;
}

static guint
add_timeout_full (MilterEventLoop *loop,
                  gint             priority,
                  gdouble          interval_in_seconds,
                  GSourceFunc      function,
                  gpointer         data,
                  GDestroyNotify   notify)
{
    MilterGLibEventLoopPrivate *priv;
    guint id;
    GSource *source;

    priv = MILTER_GLIB_EVENT_LOOP_GET_PRIVATE(loop);
    source = g_timeout_source_new(interval_in_seconds * 1000);
    if (priority != G_PRIORITY_DEFAULT)
        g_source_set_priority(source, priority);
    g_source_set_callback(source, function, data, notify);
    id = attach_source(priv, source);
    g_source_unref(source);

    return id;
}

static guint
add_idle_full (MilterEventLoop *loop,
               gint             priority,
               GSourceFunc      function,
               gpointer         data,
               GDestroyNotify   notify)
{
    MilterGLibEventLoopPrivate *priv;
    GSource *source;
    guint id;

    priv = MILTER_GLIB_EVENT_LOOP_GET_PRIVATE(loop);
    source = g_idle_source_new();
    if (priority != G_PRIORITY_DEFAULT_IDLE)
        g_source_set_priority(source, priority);
    g_source_set_callback(source, function, data, notify);
    id = attach_source(priv, source);
    g_source_unref(source);

    return id;
}

static gboolean
remove (MilterEventLoop *loop, guint id)
{
    MilterGLibEventLoopPrivate *priv;
    GSource *source;
    GMainContext *context;

    priv = MILTER_GLIB_EVENT_LOOP_GET_PRIVATE(loop);
    context = g_main_loop_get_context(priv->loop);
    source = g_main_context_find_source_by_id(context, id);
    if (source)
        g_source_destroy(source);

    return source != NULL;
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
