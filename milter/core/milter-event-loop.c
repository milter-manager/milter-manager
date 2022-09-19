/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Copyright (C) 2010  Nobuyoshi Nakada <nakada@clear-code.com>
 *  Copyright (C) 2011-2022  Sutou Kouhei <kou@clear-code.com>
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
#  include <config.h>
#endif /* HAVE_CONFIG_H */

#include "milter-event-loop.h"
#include "milter-logger.h"

#define MILTER_EVENT_LOOP_GET_PRIVATE(obj)              \
  (G_TYPE_INSTANCE_GET_PRIVATE((obj),                   \
                               MILTER_TYPE_EVENT_LOOP,  \
                               MilterEventLoopPrivate))

G_DEFINE_ABSTRACT_TYPE(MilterEventLoop, milter_event_loop, G_TYPE_OBJECT)

typedef struct _MilterEventLoopPrivate	MilterEventLoopPrivate;
struct _MilterEventLoopPrivate
{
    MilterEventLoopCustomRunFunc custom_run;
    MilterEventLoopCustomIterateFunc custom_iterate;
    gpointer custom_iterate_user_data;
    GDestroyNotify custom_iterate_destroy;
    guint depth;
};

enum
{
    PROP_0,
    PROP_CUSTOM_RUN,
    PROP_LAST
};

static void     dispose        (GObject         *object);
static void     set_property   (GObject         *object,
                                guint            prop_id,
                                const GValue    *value,
                                GParamSpec      *pspec);
static void     get_property   (GObject         *object,
                                guint            prop_id,
                                GValue          *value,
                                GParamSpec      *pspec);

static void
milter_event_loop_class_init (MilterEventLoopClass *klass)
{
    GObjectClass *gobject_class;
    GParamSpec *spec;

    gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->dispose      = dispose;
    gobject_class->set_property = set_property;
    gobject_class->get_property = get_property;

    klass->quit = NULL;
    klass->watch_io_full = NULL;
    klass->watch_child_full = NULL;
    klass->add_timeout_full = NULL;
    klass->add_idle_full = NULL;
    klass->remove = NULL;

    spec = g_param_spec_pointer("custom-run",
                                "Custom run",
                                "The custom run",
                                G_PARAM_READWRITE);
    g_object_class_install_property(gobject_class, PROP_CUSTOM_RUN, spec);

    g_type_class_add_private(gobject_class, sizeof(MilterEventLoopPrivate));
}

static void
milter_event_loop_init (MilterEventLoop *loop)
{
    MilterEventLoopPrivate *priv;

    priv = MILTER_EVENT_LOOP_GET_PRIVATE(loop);
    priv->depth = 0;
    priv->custom_iterate = NULL;
    priv->custom_iterate_user_data = NULL;
    priv->custom_iterate_destroy = NULL;
}

static void
dispose_custom_iterate (MilterEventLoopPrivate *priv)
{
    if (!priv->custom_iterate) {
        return;
    }

    if (priv->custom_iterate_destroy) {
        priv->custom_iterate_destroy(priv->custom_iterate_user_data);
    }
    priv->custom_iterate           = NULL;
    priv->custom_iterate_user_data = NULL;
    priv->custom_iterate_destroy   = NULL;
}

static void
dispose (GObject *object)
{
    MilterEventLoopPrivate *priv;

    milter_debug("[event-loop][dispose]");

    priv = MILTER_EVENT_LOOP_GET_PRIVATE(object);
    dispose_custom_iterate(priv);

    G_OBJECT_CLASS(milter_event_loop_parent_class)->dispose(object);
}

static void
set_property (GObject      *object,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
    MilterEventLoop *loop;
    MilterEventLoopPrivate *priv;

    loop = MILTER_EVENT_LOOP(object);
    priv = MILTER_EVENT_LOOP_GET_PRIVATE(loop);
    switch (prop_id) {
    case PROP_CUSTOM_RUN:
        priv->custom_run = g_value_get_pointer(value);
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
    MilterEventLoop *loop;
    MilterEventLoopPrivate *priv;

    loop = MILTER_EVENT_LOOP(object);
    priv = MILTER_EVENT_LOOP_GET_PRIVATE(loop);
    switch (prop_id) {
    case PROP_CUSTOM_RUN:
        g_value_set_pointer(value, priv->custom_run);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

GQuark
milter_event_loop_error_quark (void)
{
    return g_quark_from_static_string("milter-event-loop-error-quark");
}

void
milter_event_loop_run (MilterEventLoop *loop)
{
    MilterEventLoopPrivate *priv;

    g_return_if_fail(loop != NULL);

    priv = MILTER_EVENT_LOOP_GET_PRIVATE(loop);
    if (priv->custom_run) {
        priv->custom_run(loop);
    } else {
        milter_event_loop_run_without_custom(loop);
    }
}

gboolean
milter_event_loop_is_running (MilterEventLoop *loop)
{
    MilterEventLoopPrivate *priv;

    g_return_val_if_fail(loop != NULL, FALSE);

    priv = MILTER_EVENT_LOOP_GET_PRIVATE(loop);
    return priv->depth > 0;
}

void
milter_event_loop_run_without_custom (MilterEventLoop *loop)
{
    MilterEventLoopPrivate *priv;
    guint depth;

    g_return_if_fail(loop != NULL);

    priv = MILTER_EVENT_LOOP_GET_PRIVATE(loop);
    depth = priv->depth++;
    while (priv->depth > depth) {
        milter_event_loop_iterate(loop, TRUE);
    }
}

/**
 * milter_event_loop_set_custom_run_func: (skip)
 * @loop: A #MilterEventLoop.
 * @custom_run: A #MilterEventLoopCustomRunFunc.
 *
 * Set the custom event runner function to the event loop.
 */
void
milter_event_loop_set_custom_run_func (MilterEventLoop             *loop,
                                       MilterEventLoopCustomRunFunc custom_run)
{
    MilterEventLoopPrivate *priv;

    g_return_if_fail(loop != NULL);

    priv = MILTER_EVENT_LOOP_GET_PRIVATE(loop);
    priv->custom_run = custom_run;
}

/**
 * milter_event_loop_get_custom_run_func: (skip)
 * @loop: A #MilterEventLoop.
 *
 * Returns: (nullable): The current custom runner function of this event loop.
 */
MilterEventLoopCustomRunFunc
milter_event_loop_get_custom_run_func (MilterEventLoop *loop)
{
    MilterEventLoopPrivate *priv;

    g_return_val_if_fail(loop != NULL, NULL);

    priv = MILTER_EVENT_LOOP_GET_PRIVATE(loop);
    return priv->custom_run;
}

gboolean
milter_event_loop_iterate (MilterEventLoop *loop, gboolean may_block)
{
    MilterEventLoopPrivate *priv;

    g_return_val_if_fail(loop != NULL, FALSE);

    priv = MILTER_EVENT_LOOP_GET_PRIVATE(loop);
    if (priv->custom_iterate) {
        return priv->custom_iterate(loop, may_block,
                                    priv->custom_iterate_user_data);
    } else {
        return milter_event_loop_iterate_without_custom(loop, may_block);
    }
}

gboolean
milter_event_loop_iterate_without_custom (MilterEventLoop *loop,
                                          gboolean may_block)
{
    MilterEventLoopClass *loop_class;

    g_return_val_if_fail(loop != NULL, FALSE);

    loop_class = MILTER_EVENT_LOOP_GET_CLASS(loop);
    return loop_class->iterate(loop, may_block);
}

void
milter_event_loop_set_custom_iterate_func (MilterEventLoop             *loop,
                                           MilterEventLoopCustomIterateFunc custom_iterate,
                                           gpointer                     user_data,
                                           GDestroyNotify               destroy)
{
    MilterEventLoopPrivate *priv;

    g_return_if_fail(loop != NULL);

    priv = MILTER_EVENT_LOOP_GET_PRIVATE(loop);

    dispose_custom_iterate(priv);

    priv->custom_iterate           = custom_iterate;
    priv->custom_iterate_user_data = user_data;
    priv->custom_iterate_destroy   = destroy;
}

/**
 * milter_event_loop_get_custom_iterate_func: (skip)
 * @loop: A #MilterEventLoop.
 *
 * Returns: (nullable): The current custom iterate function of this event loop.
 */
MilterEventLoopCustomIterateFunc
milter_event_loop_get_custom_iterate_func (MilterEventLoop *loop)
{
    MilterEventLoopPrivate *priv;

    g_return_val_if_fail(loop != NULL, NULL);

    priv = MILTER_EVENT_LOOP_GET_PRIVATE(loop);
    return priv->custom_iterate;
}

void
milter_event_loop_quit (MilterEventLoop *loop)
{
    MilterEventLoopPrivate *priv;
    MilterEventLoopClass *loop_class;

    g_return_if_fail(loop != NULL);

    loop_class = MILTER_EVENT_LOOP_GET_CLASS(loop);
    loop_class->quit(loop);

    priv = MILTER_EVENT_LOOP_GET_PRIVATE(loop);
    g_return_if_fail(priv->depth > 0);
    priv->depth--;
}

/**
 * milter_event_loop_watch_io: (skip)
 * @loop: A #MilterEventLoop.
 * @channel: A #GIOChannel.
 * @condition: The condition to watch for.
 * @function: The function to call when the condition is satisfied.
 * @user_data: User data to pass to @function.
 *
 * This equals to `milter_event_loop_watch_io_full(loop,
 * G_PRIORITY_DEFAULT, channel, condition, function, user_data,
 * NULL)`.
 *
 * Returns: The event source ID.
 */
guint
milter_event_loop_watch_io (MilterEventLoop *loop,
                            GIOChannel      *channel,
                            GIOCondition     condition,
                            GIOFunc          function,
                            gpointer         user_data)
{
    return milter_event_loop_watch_io_full(loop, G_PRIORITY_DEFAULT,
                                           channel, condition,
                                           function, user_data,
                                           NULL);
}

/**
 * milter_event_loop_watch_io_full: (rename-to milter_event_loop_watch_io)
 * @loop: A #MilterEventLoop.
 * @priority: The priority of the #GIOChannel source.
 * @channel: A #GIOChannel.
 * @condition: The condition to watch for.
 * @function: The function to call when the condition is satisfied.
 * @user_data: User data to pass to @function.
 * @notify: (nullable): The function to call when the source is removed.
 *
 * Returns: The event source ID.
 */
guint
milter_event_loop_watch_io_full (MilterEventLoop *loop,
                                 gint             priority,
                                 GIOChannel      *channel,
                                 GIOCondition     condition,
                                 GIOFunc          function,
                                 gpointer         user_data,
                                 GDestroyNotify   notify)
{
    MilterEventLoopClass *loop_class;

    g_return_val_if_fail(loop != NULL, 0);

    loop_class = MILTER_EVENT_LOOP_GET_CLASS(loop);
    return loop_class->watch_io_full(loop, priority, channel, condition,
                                     function, user_data, notify);
}

/**
 * milter_event_loop_watch_child: (skip)
 * @loop: A #MilterEventLoop.
 * @pid: An ID of the process to watch.
 * @function: The function to call when the child is finished.
 * @user_data: User data to pass to @function.
 *
 * This equals to `milter_event_loop_watch_child_full(loop,
 * G_PRIORITY_DEFAULT, pid, function, user_data, NULL)`.
 *
 * Returns: The event source ID.
 */
guint
milter_event_loop_watch_child (MilterEventLoop *loop,
                               GPid             pid,
                               GChildWatchFunc  function,
                               gpointer         user_data)
{
    return milter_event_loop_watch_child_full(loop, G_PRIORITY_DEFAULT,
                                              pid,
                                              function, user_data,
                                              NULL);
}

/**
 * milter_event_loop_watch_child_full: (rename-to milter_event_loop_watch_child)
 * @loop: A #MilterEventLoop.
 * @priority: The priority of the child source.
 * @pid: An ID of the process to watch.
 * @function: The function to call when the child is finished.
 * @user_data: User data to pass to @function.
 * @notify: (nullable): The function to call when the source is removed.
 *
 * Returns: The event source ID.
 */
guint
milter_event_loop_watch_child_full (MilterEventLoop *loop,
                                    gint             priority,
                                    GPid             pid,
                                    GChildWatchFunc  function,
                                    gpointer         user_data,
                                    GDestroyNotify   notify)
{
    MilterEventLoopClass *loop_class;

    g_return_val_if_fail(loop != NULL, 0);

    loop_class = MILTER_EVENT_LOOP_GET_CLASS(loop);
    return loop_class->watch_child_full(loop, priority, pid,
                                        function, user_data, notify);
}

/**
 * milter_event_loop_add_timeout: (skip)
 * @loop: A #MilterEventLoop.
 * @interval_in_seconds: The time between calls to the @function, in seconds.
 * @function: The function to call when the interval is passed.
 * @user_data: User data to pass to @function.
 *
 * This equals to `milter_event_loop_add_timeout_full(loop,
 * G_PRIORITY_DEFAULT, interval_in_seconds, function, user_data,
 * NULL)`.
 *
 * Returns: The event source ID.
 */
guint
milter_event_loop_add_timeout (MilterEventLoop *loop,
                               gdouble          interval_in_seconds,
                               GSourceFunc      function,
                               gpointer         user_data)
{
    return milter_event_loop_add_timeout_full(loop, G_PRIORITY_DEFAULT,
                                              interval_in_seconds,
                                              function, user_data,
                                              NULL);
}

/**
 * milter_event_loop_add_timeout_full: (rename-to milter_event_loop_add_timeout)
 * @loop: A #MilterEventLoop.
 * @priority: The priority of the timeout source.
 * @interval_in_seconds: The time between calls to the @function, in seconds.
 * @function: The function to call when the interval is passed.
 * @user_data: User data to pass to @function.
 * @notify: (nullable): The function to call when the source is removed.
 *
 * Returns: The event source ID.
 */
guint
milter_event_loop_add_timeout_full (MilterEventLoop *loop,
                                    gint             priority,
                                    gdouble          interval_in_seconds,
                                    GSourceFunc      function,
                                    gpointer         user_data,
                                    GDestroyNotify   notify)
{
    MilterEventLoopClass *loop_class;

    g_return_val_if_fail(loop != NULL, 0);
    g_return_val_if_fail(interval_in_seconds >= 0, 0);

    loop_class = MILTER_EVENT_LOOP_GET_CLASS(loop);
    return loop_class->add_timeout_full(loop, priority, interval_in_seconds,
                                        function, user_data, notify);
}

/**
 * milter_event_loop_add_idle: (skip)
 * @loop: A #MilterEventLoop.
 * @function: The function to call when no source is dispatched.
 * @user_data: User data to pass to @function.
 *
 * This equals to `milter_event_loop_add_idle_full(loop,
 * G_PRIORITY_DEFAULT_IDLE, function, user_data, NULL)`.
 *
 * Returns: The event source ID.
 */
guint
milter_event_loop_add_idle (MilterEventLoop *loop,
                            GSourceFunc      function,
                            gpointer         user_data)
{
    return milter_event_loop_add_idle_full(loop, G_PRIORITY_DEFAULT_IDLE,
                                           function, user_data, NULL);
}

/**
 * milter_event_loop_add_idle_full: (rename-to milter_event_loop_add_idle)
 * @loop: A #MilterEventLoop.
 * @priority: The priority of the idle source.
 * @function: The function to call when no source is dispatched.
 * @user_data: User data to pass to @function.
 * @notify: (nullable): The function to call when the source is removed.
 *
 * Returns: The event source ID.
 */
guint
milter_event_loop_add_idle_full (MilterEventLoop *loop,
                                 gint             priority,
                                 GSourceFunc      function,
                                 gpointer         user_data,
                                 GDestroyNotify   notify)
{
    MilterEventLoopClass *loop_class;

    g_return_val_if_fail(loop != NULL, 0);

    loop_class = MILTER_EVENT_LOOP_GET_CLASS(loop);
    return loop_class->add_idle_full(loop, priority, function, user_data, notify);
}

gboolean
milter_event_loop_remove (MilterEventLoop *loop, guint tag)
{
    MilterEventLoopClass *loop_class;

    g_return_val_if_fail(loop != NULL, FALSE);

    loop_class = MILTER_EVENT_LOOP_GET_CLASS(loop);
    return loop_class->remove(loop, tag);
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
