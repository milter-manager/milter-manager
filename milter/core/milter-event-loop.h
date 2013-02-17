/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Copyright (C) 2010  Nobuyoshi Nakada <nakada@clear-code.com>
 *  Copyright (C) 2011-2013  Kouhei Sutou <nakada@clear-code.com>
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

#ifndef __MILTER_EVENT_LOOP_H__
#define __MILTER_EVENT_LOOP_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define MILTER_EVENT_LOOP_ERROR           (milter_event_loop_error_quark())

#define MILTER_TYPE_EVENT_LOOP            (milter_event_loop_get_type())
#define MILTER_EVENT_LOOP(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), MILTER_TYPE_EVENT_LOOP, MilterEventLoop))
#define MILTER_EVENT_LOOP_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), MILTER_TYPE_EVENT_LOOP, MilterEventLoopClass))
#define MILTER_IS_EVENT_LOOP(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), MILTER_TYPE_EVENT_LOOP))
#define MILTER_IS_EVENT_LOOP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), MILTER_TYPE_EVENT_LOOP))
#define MILTER_EVENT_LOOP_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), MILTER_TYPE_EVENT_LOOP, MilterEventLoopClass))

typedef enum
{
    MILTER_EVENT_LOOP_ERROR_MAX
} MilterEventLoopError;

typedef struct _MilterEventLoop         MilterEventLoop;
typedef struct _MilterEventLoopClass    MilterEventLoopClass;

struct _MilterEventLoop
{
    GObject object;
};

struct _MilterEventLoopClass
{
    GObjectClass parent_class;

    gboolean (*iterate)          (MilterEventLoop *loop,
                                  gboolean         may_block);
    void     (*quit)             (MilterEventLoop *loop);

    guint    (*watch_io_full)    (MilterEventLoop *loop,
                                  gint             priority,
                                  GIOChannel      *channel,
                                  GIOCondition     condition,
                                  GIOFunc          function,
                                  gpointer         data,
                                  GDestroyNotify   notify);
    guint    (*watch_child_full) (MilterEventLoop *loop,
                                  gint             priority,
                                  GPid             pid,
                                  GChildWatchFunc  function,
                                  gpointer         data,
                                  GDestroyNotify   notify);
    guint    (*add_timeout_full) (MilterEventLoop *loop,
                                  gint             priority,
                                  gdouble          interval_in_seconds,
                                  GSourceFunc      function,
                                  gpointer         data,
                                  GDestroyNotify   notify);
    guint    (*add_idle_full)    (MilterEventLoop *loop,
                                  gint             priority,
                                  GSourceFunc      function,
                                  gpointer         data,
                                  GDestroyNotify   notify);
    gboolean (*remove)           (MilterEventLoop *loop,
                                  guint            id);
};

typedef void        (*MilterEventLoopCustomRunFunc)      (MilterEventLoop *loop);
typedef gboolean    (*MilterEventLoopCustomIterateFunc)  (MilterEventLoop *loop,
                                                          gboolean         may_block,
                                                          gpointer         user_data);

GQuark               milter_event_loop_error_quark       (void);
GType                milter_event_loop_get_type          (void) G_GNUC_CONST;

void                 milter_event_loop_run               (MilterEventLoop *loop);
gboolean             milter_event_loop_is_running        (MilterEventLoop *loop);
void                 milter_event_loop_run_without_custom(MilterEventLoop *loop);
void                 milter_event_loop_set_custom_run_func
                                                         (MilterEventLoop *loop,
                                                          MilterEventLoopCustomRunFunc custom_run);
MilterEventLoopCustomRunFunc milter_event_loop_get_custom_run_func
                                                         (MilterEventLoop *loop);
gboolean             milter_event_loop_iterate           (MilterEventLoop *loop,
                                                          gboolean         may_block);
gboolean             milter_event_loop_iterate_without_custom
                                                         (MilterEventLoop *loop,
                                                          gboolean         may_block);
void                 milter_event_loop_set_custom_iterate_func
                                                         (MilterEventLoop *loop,
                                                          MilterEventLoopCustomIterateFunc custom_iterate,
                                                          gpointer         user_data,
                                                          GDestroyNotify   destroy);
MilterEventLoopCustomIterateFunc
                     milter_event_loop_get_custom_iterate_func
                                                         (MilterEventLoop *loop);
void                 milter_event_loop_quit              (MilterEventLoop *loop);

guint                milter_event_loop_watch_io          (MilterEventLoop *loop,
                                                          GIOChannel      *channel,
                                                          GIOCondition     condition,
                                                          GIOFunc          function,
                                                          gpointer         data);
guint                milter_event_loop_watch_io_full     (MilterEventLoop *loop,
                                                          gint             priority,
                                                          GIOChannel      *channel,
                                                          GIOCondition     condition,
                                                          GIOFunc          function,
                                                          gpointer         data,
                                                          GDestroyNotify   notify);


guint                milter_event_loop_watch_child       (MilterEventLoop *loop,
                                                          GPid             pid,
                                                          GChildWatchFunc  function,
                                                          gpointer         data);

guint                milter_event_loop_watch_child_full  (MilterEventLoop *loop,
                                                          gint             priority,
                                                          GPid             pid,
                                                          GChildWatchFunc  function,
                                                          gpointer         data,
                                                          GDestroyNotify   notify);

guint                milter_event_loop_add_timeout       (MilterEventLoop *loop,
                                                          gdouble          interval_in_seconds,
                                                          GSourceFunc      function,
                                                          gpointer         data);

guint                milter_event_loop_add_timeout_full  (MilterEventLoop *loop,
                                                          gint             priority,
                                                          gdouble          interval_in_seconds,
                                                          GSourceFunc      function,
                                                          gpointer         data,
                                                          GDestroyNotify   notify);

guint                milter_event_loop_add_idle          (MilterEventLoop *loop,
                                                          GSourceFunc      function,
                                                          gpointer         data);
guint                milter_event_loop_add_idle_full     (MilterEventLoop *loop,
                                                          gint             priority,
                                                          GSourceFunc      function,
                                                          gpointer         data,
                                                          GDestroyNotify   notify);

gboolean             milter_event_loop_remove            (MilterEventLoop *loop,
                                                          guint            id);

G_END_DECLS

#endif /* __MILTER_EVENT_LOOP_H__ */

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
