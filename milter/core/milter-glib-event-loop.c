/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Copyright (C) 2010  Nobuyoshi Nakada <nakada@clear-code.com>
 *  Copyright (C) 2010  Kouhei Sutou <kou@clear-code.com>
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

static void     dispose      (GObject         *object);
static void     set_property (GObject         *object,
                              guint            prop_id,
                              const GValue    *value,
                              GParamSpec      *pspec);
static void     get_property (GObject         *object,
                              guint            prop_id,
                              GValue          *value,
                              GParamSpec      *pspec);
static void     constructed  (GObject         *object);

static void     run          (MilterEventLoop *loop);
static void     quit         (MilterEventLoop *loop);

static guint    add_watch    (MilterEventLoop *loop,
                              GIOChannel      *channel,
                              GIOCondition     condition,
                              GIOFunc          func,
                              gpointer         user_data);

static guint    add_timeout  (MilterEventLoop *loop,
                              gdouble interval,
                              GSourceFunc func,
                              gpointer user_data);

static guint    add_idle_full(MilterEventLoop *loop,
                              gint             priority,
                              GSourceFunc      function,
                              gpointer         data,
                              GDestroyNotify   notify);

static gboolean remove_source(MilterEventLoop *loop,
                              guint            tag);

static void
milter_glib_event_loop_class_init (MilterGLibEventLoopClass *klass)
{
    GObjectClass *gobject_class;
    GParamSpec *spec;

    gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->dispose      = dispose;
    gobject_class->set_property = set_property;
    gobject_class->get_property = get_property;
    gobject_class->constructed  = constructed;

    klass->parent_class.run = run;
    klass->parent_class.quit = quit;
    klass->parent_class.add_watch = add_watch;
    klass->parent_class.add_timeout = add_timeout;
    klass->parent_class.add_idle_full = add_idle_full;
    klass->parent_class.remove_source = remove_source;

    spec = g_param_spec_pointer("context",
                                "Context",
                                "Use the GMainContext for the event loop",
                                G_PARAM_READABLE | G_PARAM_WRITABLE |
                                G_PARAM_CONSTRUCT | G_PARAM_CONSTRUCT_ONLY);
    g_object_class_install_property(gobject_class, PROP_CONTEXT, spec);

    g_type_class_add_private(gobject_class, sizeof(MilterGLibEventLoopPrivate));
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

static void
constructed (GObject *object)
{
    MilterGLibEventLoop *loop;
    MilterGLibEventLoopPrivate *priv;

    loop = MILTER_GLIB_EVENT_LOOP(object);
    priv = MILTER_GLIB_EVENT_LOOP_GET_PRIVATE(loop);
    priv->loop = g_main_loop_new(priv->context, FALSE);
}

MilterEventLoop *
milter_glib_event_loop_new (GMainContext *context)
{
    return g_object_new(MILTER_TYPE_GLIB_EVENT_LOOP,
                        "context", context,
                        NULL);
}

static void
run (MilterEventLoop *loop)
{
    MilterGLibEventLoopPrivate *priv;

    priv = MILTER_GLIB_EVENT_LOOP_GET_PRIVATE(loop);
    g_main_loop_run(priv->loop);
}

static void
quit (MilterEventLoop *loop)
{
    MilterGLibEventLoopPrivate *priv;

    priv = MILTER_GLIB_EVENT_LOOP_GET_PRIVATE(loop);
    g_main_loop_quit(priv->loop);
}

static guint
add_watch (MilterEventLoop *loop,
           GIOChannel      *channel,
           GIOCondition     condition,
           GIOFunc          func,
           gpointer         user_data)
{
    return g_io_add_watch(channel, condition, func, user_data);
}

static guint
add_timeout (MilterEventLoop *loop,
             gdouble interval,
             GSourceFunc func,
             gpointer user_data)
{
    return g_timeout_add(interval * 1000,
                         func, user_data);
}

static guint
add_idle_full (MilterEventLoop *loop,
               gint             priority,
               GSourceFunc      function,
               gpointer         data,
               GDestroyNotify   notify)
{
    return g_idle_add_full(priority, function, data, notify);
}

static gboolean
remove_source (MilterEventLoop *loop,
               guint            tag)
{
    return g_source_remove(tag);
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
