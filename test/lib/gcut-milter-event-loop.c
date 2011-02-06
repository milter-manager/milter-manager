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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif /* HAVE_CONFIG_H */

#include "gcut-milter-event-loop.h"

#define GCUT_MILTER_EVENT_LOOP_GET_PRIVATE(obj)                   \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj),                           \
                                 GCUT_TYPE_MILTER_EVENT_LOOP,     \
                                 GCutMilterEventLoopPrivate))

G_DEFINE_TYPE(GCutMilterEventLoop, gcut_milter_event_loop,
              GCUT_TYPE_EVENT_LOOP)

typedef struct _GCutMilterEventLoopPrivate	GCutMilterEventLoopPrivate;
struct _GCutMilterEventLoopPrivate
{
    MilterEventLoop *loop;
};

enum
{
    PROP_0,
    PROP_LOOP,
    PROP_LAST
};

static void     dispose          (GObject         *object);
static void     set_property     (GObject         *object,
                                  guint            prop_id,
                                  const GValue    *value,
                                  GParamSpec      *pspec);
static void     get_property     (GObject         *object,
                                  guint            prop_id,
                                  GValue          *value,
                                  GParamSpec      *pspec);

static void     run              (GCutEventLoop   *loop);
static gboolean iterate          (GCutEventLoop   *loop,
                                  gboolean         may_block);
static void     quit             (GCutEventLoop   *loop);

static guint    watch_io         (GCutEventLoop   *loop,
                                  GIOChannel      *channel,
                                  GIOCondition     condition,
                                  GIOFunc          function,
                                  gpointer         data);

static guint    watch_child_full (GCutEventLoop   *loop,
                                  gint             priority,
                                  GPid             pid,
                                  GChildWatchFunc  function,
                                  gpointer         data,
                                  GDestroyNotify   notify);

static guint    add_timeout_full (GCutEventLoop   *loop,
                                  gint             priority,
                                  gdouble          interval_in_seconds,
                                  GSourceFunc      function,
                                  gpointer         data,
                                  GDestroyNotify   notify);

static guint    add_idle_full    (GCutEventLoop   *loop,
                                  gint             priority,
                                  GSourceFunc      function,
                                  gpointer         data,
                                  GDestroyNotify   notify);

static gboolean remove           (GCutEventLoop   *loop,
                                  guint            tag);

static void
gcut_milter_event_loop_class_init (GCutMilterEventLoopClass *klass)
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

    spec = g_param_spec_object("loop",
                               "loop",
                               "Use the MilterEventLoop for the GCutEventLoop",
                               MILTER_TYPE_EVENT_LOOP,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    g_object_class_install_property(gobject_class, PROP_LOOP, spec);

    g_type_class_add_private(gobject_class, sizeof(GCutMilterEventLoopPrivate));
}

static void
gcut_milter_event_loop_init (GCutMilterEventLoop *loop)
{
    GCutMilterEventLoopPrivate *priv;

    priv = GCUT_MILTER_EVENT_LOOP_GET_PRIVATE(loop);
    priv->loop = NULL;
}

static void
dispose (GObject *object)
{
    GCutMilterEventLoopPrivate *priv;

    priv = GCUT_MILTER_EVENT_LOOP_GET_PRIVATE(object);

    if (priv->loop) {
        g_object_unref(priv->loop);
        priv->loop = NULL;
    }

    G_OBJECT_CLASS(gcut_milter_event_loop_parent_class)->dispose(object);
}

static void
set_property (GObject      *object,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
    GCutMilterEventLoopPrivate *priv;

    priv = GCUT_MILTER_EVENT_LOOP_GET_PRIVATE(object);
    switch (prop_id) {
    case PROP_LOOP:
        if (priv->loop)
            g_object_unref(priv->loop);
        priv->loop = g_value_dup_object(value);
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
    GCutMilterEventLoop *loop;
    GCutMilterEventLoopPrivate *priv;

    loop = GCUT_MILTER_EVENT_LOOP(object);
    priv = GCUT_MILTER_EVENT_LOOP_GET_PRIVATE(loop);
    switch (prop_id) {
    case PROP_LOOP:
        g_value_set_object(value, priv->loop);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

GCutEventLoop *
gcut_milter_event_loop_new (MilterEventLoop *loop)
{
    return g_object_new(GCUT_TYPE_MILTER_EVENT_LOOP,
                        "loop", loop,
                        NULL);
}

static void
run (GCutEventLoop *loop)
{
    GCutMilterEventLoopPrivate *priv;

    priv = GCUT_MILTER_EVENT_LOOP_GET_PRIVATE(loop);
    milter_event_loop_run(priv->loop);
}

static gboolean
iterate (GCutEventLoop *loop, gboolean may_block)
{
    GCutMilterEventLoopPrivate *priv;

    priv = GCUT_MILTER_EVENT_LOOP_GET_PRIVATE(loop);
    return milter_event_loop_iterate(priv->loop, may_block);
}

static void
quit (GCutEventLoop *loop)
{
    GCutMilterEventLoopPrivate *priv;

    priv = GCUT_MILTER_EVENT_LOOP_GET_PRIVATE(loop);
    milter_event_loop_quit(priv->loop);
}

static guint
watch_io (GCutEventLoop   *loop,
          GIOChannel      *channel,
          GIOCondition     condition,
          GIOFunc          function,
          gpointer         data)
{
    GCutMilterEventLoopPrivate *priv;

    priv = GCUT_MILTER_EVENT_LOOP_GET_PRIVATE(loop);
    return milter_event_loop_watch_io(priv->loop, channel, condition,
                                      function, data);
}

static guint
watch_child_full (GCutEventLoop   *loop,
                  gint             priority,
                  GPid             pid,
                  GChildWatchFunc  function,
                  gpointer         data,
                  GDestroyNotify   notify)
{
    GCutMilterEventLoopPrivate *priv;

    priv = GCUT_MILTER_EVENT_LOOP_GET_PRIVATE(loop);
    return milter_event_loop_watch_child_full(priv->loop, priority, pid,
                                              function, data, notify);
}

static guint
add_timeout_full (GCutEventLoop   *loop,
                  gint             priority,
                  gdouble          interval_in_seconds,
                  GSourceFunc      function,
                  gpointer         data,
                  GDestroyNotify   notify)
{
    GCutMilterEventLoopPrivate *priv;

    priv = GCUT_MILTER_EVENT_LOOP_GET_PRIVATE(loop);
    return milter_event_loop_add_timeout_full(priv->loop, priority,
                                              interval_in_seconds,
                                              function, data, notify);
}

static guint
add_idle_full (GCutEventLoop   *loop,
               gint             priority,
               GSourceFunc      function,
               gpointer         data,
               GDestroyNotify   notify)
{
    GCutMilterEventLoopPrivate *priv;

    priv = GCUT_MILTER_EVENT_LOOP_GET_PRIVATE(loop);
    return milter_event_loop_add_idle_full(priv->loop, priority,
                                           function, data, notify);
}

static gboolean
remove (GCutEventLoop *loop, guint tag)
{
    GCutMilterEventLoopPrivate *priv;

    priv = GCUT_MILTER_EVENT_LOOP_GET_PRIVATE(loop);
    return milter_event_loop_remove(priv->loop, tag);
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
