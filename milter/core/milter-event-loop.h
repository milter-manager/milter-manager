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

    void (*run_loop) (MilterEventLoop *eventloop);
    void (*quit_loop) (MilterEventLoop *eventloop);

    guint (*add_watch)(MilterEventLoop *eventloop,
                       GIOChannel      *channel,
                       GIOCondition     condition,
                       GIOFunc          func,
                       gpointer         user_data);
    guint (*add_timeout)(MilterEventLoop *eventloop,
                         gdouble interval,
                         GSourceFunc func,
                         gpointer user_data);
    guint (*add_idle_full)(MilterEventLoop *eventloop,
                           gint             priority,
                           GSourceFunc      function,
                           gpointer         data,
                           GDestroyNotify   notify);
    gboolean (*remove_source)(MilterEventLoop *eventloop,
                              guint            tag);
};

GQuark               milter_event_loop_error_quark       (void);
GType                milter_event_loop_get_type          (void) G_GNUC_CONST;

MilterEventLoop     *milter_event_loop_new               (void);

void                 milter_event_loop_run               (MilterEventLoop *eventloop);
void                 milter_event_loop_quit              (MilterEventLoop *eventloop);

guint                milter_event_loop_add_watch         (MilterEventLoop *eventloop,
                                                          GIOChannel      *channel,
                                                          GIOCondition     condition,
                                                          GIOFunc          func,
                                                          gpointer         user_data);

guint                milter_event_loop_add_timeout       (MilterEventLoop *eventloop,
                                                          gdouble interval,
                                                          GSourceFunc func,
                                                          gpointer user_data);

guint                milter_event_loop_add_idle_full     (MilterEventLoop *eventloop,
                                                          gint             priority,
                                                          GSourceFunc      function,
                                                          gpointer         data,
                                                          GDestroyNotify   notify);

gboolean             milter_event_loop_remove_source     (MilterEventLoop *eventloop,
                                                          guint            tag);

G_END_DECLS

#endif /* __MILTER_EVENT_LOOP_H__ */

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
