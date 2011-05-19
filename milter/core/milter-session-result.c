/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Copyright (C) 2010-2011  Kouhei Sutou <kou@clear-code.com>
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

#include <string.h>

#include "milter-session-result.h"
#include "milter-enum-types.h"
#include "milter-utils.h"

#define MILTER_SESSION_RESULT_GET_PRIVATE(obj)                  \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj),                         \
                                 MILTER_TYPE_SESSION_RESULT,    \
                                 MilterSessionResultPrivate))

typedef struct _MilterSessionResultPrivate MilterSessionResultPrivate;
struct _MilterSessionResultPrivate
{
    GList *message_results;
    GTimeVal start_time;
    GTimeVal end_time;
    gdouble elapsed_time;
    gboolean elapsed_time_set;
    gboolean disconnected;
};

enum
{
    PROP_0,
    PROP_ELAPSED_TIME,
    PROP_DISCONNECTED
};

G_DEFINE_TYPE(MilterSessionResult, milter_session_result, G_TYPE_OBJECT);

static void dispose        (GObject         *object);
static void set_property   (GObject         *object,
                            guint            prop_id,
                            const GValue    *value,
                            GParamSpec      *pspec);
static void get_property   (GObject         *object,
                            guint            prop_id,
                            GValue          *value,
                            GParamSpec      *pspec);

static void
milter_session_result_class_init (MilterSessionResultClass *klass)
{
    GObjectClass *gobject_class;
    GParamSpec *spec;

    gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->dispose      = dispose;
    gobject_class->set_property = set_property;
    gobject_class->get_property = get_property;

    spec = g_param_spec_double("elapsed-time",
                               "Elapsed time",
                               "The elapsed time of the session",
                               0.0,
                               G_MAXDOUBLE,
                               0.0,
                               G_PARAM_READWRITE);
    g_object_class_install_property(gobject_class, PROP_ELAPSED_TIME, spec);

    spec = g_param_spec_boolean("disconnected",
                                "Disconnected",
                                "Whether disconnected the connection from peer",
                                FALSE,
                                G_PARAM_READWRITE);
    g_object_class_install_property(gobject_class, PROP_DISCONNECTED, spec);

    g_type_class_add_private(gobject_class,
                             sizeof(MilterSessionResultPrivate));
}

static void
milter_session_result_init (MilterSessionResult *headers)
{
    MilterSessionResultPrivate *priv;

    priv = MILTER_SESSION_RESULT_GET_PRIVATE(headers);
    priv->message_results = NULL;
    priv->start_time.tv_sec = 0;
    priv->start_time.tv_usec = 0;
    priv->end_time.tv_sec = 0;
    priv->end_time.tv_usec = 0;
    priv->elapsed_time = 0.0;
    priv->elapsed_time_set = FALSE;
    priv->disconnected = FALSE;
}

static void
dispose_message_results (MilterSessionResultPrivate *priv)
{
    if (priv->message_results) {
        g_list_foreach(priv->message_results, (GFunc)g_object_unref, NULL);
        g_list_free(priv->message_results);
        priv->message_results = NULL;
    }
}

static void
dispose (GObject *object)
{
    MilterSessionResultPrivate *priv;

    priv = MILTER_SESSION_RESULT_GET_PRIVATE(object);

    dispose_message_results(priv);

    G_OBJECT_CLASS(milter_session_result_parent_class)->dispose(object);
}

static void
set_property (GObject      *object,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
    MilterSessionResult *result;

    result = MILTER_SESSION_RESULT(object);
    switch (prop_id) {
    case PROP_ELAPSED_TIME:
        milter_session_result_set_elapsed_time(result,
                                               g_value_get_double(value));
        break;
    case PROP_DISCONNECTED:
        milter_session_result_set_disconnected(result,
                                               g_value_get_boolean(value));
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
    MilterSessionResult *result;

    result = MILTER_SESSION_RESULT(object);
    switch (prop_id) {
    case PROP_ELAPSED_TIME:
        g_value_set_double(value,
                           milter_session_result_get_elapsed_time(result));
        break;
    case PROP_DISCONNECTED:
        g_value_set_boolean(value,
                            milter_session_result_is_disconnected(result));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

MilterSessionResult *
milter_session_result_new (void)
{
    return g_object_new(MILTER_TYPE_SESSION_RESULT, NULL);
}

void
milter_session_result_start (MilterSessionResult *result)
{
    GTimeVal current_time;

    g_get_current_time(&current_time);
    milter_session_result_set_start_time(result, &current_time);
}

void
milter_session_result_add_message_result (MilterSessionResult *result,
                                          MilterMessageResult *message_result)
{
    MilterSessionResultPrivate *priv;

    priv = MILTER_SESSION_RESULT_GET_PRIVATE(result);

    g_object_ref(message_result);
    priv->message_results = g_list_append(priv->message_results, message_result);
}

void
milter_session_result_remove_message_result (MilterSessionResult *result,
                                             MilterMessageResult *message_result)
{
    MilterSessionResultPrivate *priv;
    GList *node;

    priv = MILTER_SESSION_RESULT_GET_PRIVATE(result);
    for (node = priv->message_results; node; node = g_list_next(node)) {
        MilterMessageResult *_message_result = node->data;

        if (_message_result == message_result) {
            g_object_unref(_message_result);
            priv->message_results =
                g_list_remove_link(priv->message_results, node);
        }
    }
}

static gdouble
compute_elapsed (GTimeVal *start_time, GTimeVal *end_time)
{
    return (end_time->tv_sec - start_time->tv_sec) +
        (end_time->tv_usec - start_time->tv_usec) / 1000000.0;
}

void
milter_session_result_stop (MilterSessionResult *result)
{
    GTimeVal current_time;
    GTimeVal *start_time;

    g_get_current_time(&current_time);
    milter_session_result_set_end_time(result, &current_time);

    start_time = milter_session_result_get_start_time(result);
    if (start_time->tv_sec > 0) {
        gdouble elapsed;

        elapsed = compute_elapsed(start_time, &current_time);
        milter_session_result_set_elapsed_time(result, elapsed);
    }
}

GList *
milter_session_result_get_message_results (MilterSessionResult *result)
{
    return MILTER_SESSION_RESULT_GET_PRIVATE(result)->message_results;
}

void
milter_session_result_set_message_results (MilterSessionResult *result,
                                           const GList *message_results)
{
    MilterSessionResultPrivate *priv;
    const GList *node;

    priv = MILTER_SESSION_RESULT_GET_PRIVATE(result);
    dispose_message_results(priv);

    for (node = message_results; node; node = g_list_next(node)) {
        MilterMessageResult *message_result = node->data;
        milter_session_result_add_message_result(result, message_result);
    }
}

gboolean
milter_session_result_is_disconnected (MilterSessionResult *result)
{
    return MILTER_SESSION_RESULT_GET_PRIVATE(result)->disconnected;
}

void
milter_session_result_set_disconnected (MilterSessionResult *result,
                                      gboolean disconnected)
{
    MILTER_SESSION_RESULT_GET_PRIVATE(result)->disconnected = disconnected;
}

GTimeVal *
milter_session_result_get_start_time (MilterSessionResult *result)
{
    return &MILTER_SESSION_RESULT_GET_PRIVATE(result)->start_time;
}

void
milter_session_result_set_start_time (MilterSessionResult *result,
                                      GTimeVal *time)
{
    MilterSessionResultPrivate *priv;

    priv = MILTER_SESSION_RESULT_GET_PRIVATE(result);
    if (time) {
        priv->start_time = *time;
    } else {
        priv->start_time.tv_sec = 0;
        priv->start_time.tv_usec = 0;
    }
}

GTimeVal *
milter_session_result_get_end_time (MilterSessionResult *result)
{
    return &MILTER_SESSION_RESULT_GET_PRIVATE(result)->end_time;
}

void
milter_session_result_set_end_time (MilterSessionResult *result,
                                    GTimeVal *time)
{
    MilterSessionResultPrivate *priv;

    priv = MILTER_SESSION_RESULT_GET_PRIVATE(result);
    if (time) {
        priv->end_time = *time;
    } else {
        priv->end_time.tv_sec = 0;
        priv->end_time.tv_usec = 0;
    }
}

gdouble
milter_session_result_get_elapsed_time (MilterSessionResult *result)
{
    MilterSessionResultPrivate *priv;

    priv = MILTER_SESSION_RESULT_GET_PRIVATE(result);
    if (!priv->elapsed_time_set &&
        priv->start_time.tv_sec > 0 && priv->end_time.tv_sec > 0) {
        return compute_elapsed(&(priv->start_time), &(priv->end_time));
    } else {
        return priv->elapsed_time;
    }
}

void
milter_session_result_set_elapsed_time (MilterSessionResult *result,
                                        gdouble elapsed_time)
{
    MilterSessionResultPrivate *priv;

    priv = MILTER_SESSION_RESULT_GET_PRIVATE(result);
    priv->elapsed_time = elapsed_time;
    priv->elapsed_time_set = TRUE;
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
