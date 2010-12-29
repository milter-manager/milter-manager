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
    PROP_NEW_CONTEXT,
    PROP_LAST
};

static GObject *constructor  (GType                  type,
                              guint                  n_props,
                              GObjectConstructParam *props);

static void dispose        (GObject         *object);
static void set_property   (GObject         *object,
                            guint            prop_id,
                            const GValue    *value,
                            GParamSpec      *pspec);
static void get_property   (GObject         *object,
                            guint            prop_id,
                            GValue          *value,
                            GParamSpec      *pspec);
static void constructed    (GObject         *object);

static void
milter_event_loop_class_init (MilterEventLoopClass *klass)
{
    GObjectClass *gobject_class;
    GParamSpec *spec;

    gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->constructor  = constructor;
    gobject_class->dispose      = dispose;
    gobject_class->set_property = set_property;
    gobject_class->get_property = get_property;
    gobject_class->constructed  = constructed;

    klass->run_loop = NULL;
    klass->quit_loop = NULL;
    klass->add_watch = NULL;
    klass->add_timeout = NULL;
    klass->add_idle_full = NULL;
    klass->remove_source = NULL;

    spec = g_param_spec_boolean("new-context",
                                "New Context",
                                "Use separate context for the event loop if TRUE",
                                FALSE,
                                G_PARAM_READABLE | G_PARAM_WRITABLE |
                                G_PARAM_CONSTRUCT | G_PARAM_CONSTRUCT_ONLY);
    g_object_class_install_property(gobject_class, PROP_NEW_CONTEXT, spec);

    g_type_class_add_private(gobject_class, sizeof(MilterEventLoopPrivate));
}

static GObject *
constructor (GType type, guint n_props, GObjectConstructParam *props)
{
    GObject *object;
    MilterEventLoop *eventloop;
    GObjectClass *klass;
    MilterEventLoopClass *eventloop_class;
    MilterEventLoopPrivate *priv;

    klass = G_OBJECT_CLASS(milter_event_loop_parent_class);
    object = klass->constructor(type, n_props, props);

    priv = MILTER_EVENT_LOOP_GET_PRIVATE(object);

    eventloop = MILTER_EVENT_LOOP(object);
    eventloop_class = MILTER_EVENT_LOOP_GET_CLASS(object);

    priv->new_context = FALSE;

    return object;
}

static void
milter_event_loop_init (MilterEventLoop *eventloop)
{
    MilterEventLoopPrivate *priv;

    priv = MILTER_EVENT_LOOP_GET_PRIVATE(eventloop);
    priv->new_context = FALSE;
}

static void
dispose (GObject *object)
{
    MilterEventLoopPrivate *priv;

    priv = MILTER_EVENT_LOOP_GET_PRIVATE(object);

    G_OBJECT_CLASS(milter_event_loop_parent_class)->dispose(object);
}

static void
set_property (GObject      *object,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
    MilterEventLoopPrivate *priv;

    priv = MILTER_EVENT_LOOP_GET_PRIVATE(object);
    switch (prop_id) {
    case PROP_NEW_CONTEXT:
        priv->new_context = g_value_get_boolean(value);
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
    MilterEventLoop *eventloop;
    MilterEventLoopPrivate *priv;

    eventloop = MILTER_EVENT_LOOP(object);
    priv = MILTER_EVENT_LOOP_GET_PRIVATE(eventloop);
    switch (prop_id) {
    case PROP_NEW_CONTEXT:
        g_value_set_boolean(value, priv->new_context);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static void
constructed (GObject *object)
{
    MilterEventLoop *eventloop;
    MilterEventLoopClass *eventloop_class;
    MilterEventLoopPrivate *priv;

    eventloop = MILTER_EVENT_LOOP(object);
    eventloop_class = MILTER_EVENT_LOOP_GET_CLASS(object);
    priv = MILTER_EVENT_LOOP_GET_PRIVATE(eventloop);
    eventloop_class->initialize(eventloop, priv->new_context);
}

GQuark
milter_event_loop_error_quark (void)
{
    return g_quark_from_static_string("milter-eventloop-error-quark");
}

MilterEventLoop *
milter_event_loop_new (gboolean new_context)
{
#define DEFAULT_MILTER_EVENT_LOOP_NAME "MilterGEventLoopClass"
    GType eventloop_type = g_type_from_name(DEFAULT_MILTER_EVENT_LOOP_NAME);

    return g_object_new(eventloop_type,
                        "new-context", new_context,
                        NULL);
}

void 
milter_event_loop_run (MilterEventLoop *eventloop)
{
    MilterEventLoopClass *eventloop_class;
    eventloop_class = MILTER_EVENT_LOOP_GET_CLASS(eventloop);
    eventloop_class->run_loop(eventloop);
}

void 
milter_event_loop_quit (MilterEventLoop *eventloop)
{
    MilterEventLoopClass *eventloop_class;
    eventloop_class = MILTER_EVENT_LOOP_GET_CLASS(eventloop);
    eventloop_class->quit_loop(eventloop);
}

guint
milter_event_loop_add_watch (MilterEventLoop *eventloop,
                             GIOChannel      *channel,
                             GIOCondition     condition,
                             GIOFunc          func,
                             gpointer         user_data)
{
    MilterEventLoopClass *eventloop_class;
    eventloop_class = MILTER_EVENT_LOOP_GET_CLASS(eventloop);
    return eventloop_class->add_watch(eventloop,
                                      channel,
                                      condition,
                                      func,
                                      user_data);
}

guint
milter_event_loop_add_timeout (MilterEventLoop *eventloop,
                               gdouble interval,
                               GSourceFunc func,
                               gpointer user_data)
{
    MilterEventLoopClass *eventloop_class;
    eventloop_class = MILTER_EVENT_LOOP_GET_CLASS(eventloop);
    return eventloop_class->add_timeout(eventloop,
                                        interval,
                                        func,
                                        user_data);
}

guint
milter_event_loop_add_idle_full (MilterEventLoop *eventloop,
                                 gint             priority,
                                 GSourceFunc      function,
                                 gpointer         data,
                                 GDestroyNotify   notify)
{
    MilterEventLoopClass *eventloop_class;
    eventloop_class = MILTER_EVENT_LOOP_GET_CLASS(eventloop);
    return eventloop_class->add_idle_full(eventloop,
                                          priority,
                                          function,
                                          data,
                                          notify);
}

gboolean
milter_event_loop_remove_source (MilterEventLoop *eventloop,
                                 guint            tag)
{
    MilterEventLoopClass *eventloop_class;
    eventloop_class = MILTER_EVENT_LOOP_GET_CLASS(eventloop);
    return eventloop_class->remove_source(eventloop, tag);
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
