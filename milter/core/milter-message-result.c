/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Copyright (C) 2009-2011  Kouhei Sutou <kou@clear-code.com>
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

#include "milter-message-result.h"
#include "milter-enum-types.h"
#include "milter-utils.h"

#define MILTER_MESSAGE_RESULT_GET_PRIVATE(obj)                  \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj),                         \
                                 MILTER_TYPE_MESSAGE_RESULT,    \
                                 MilterMessageResultPrivate))

typedef struct _MilterMessageResultPrivate MilterMessageResultPrivate;
struct _MilterMessageResultPrivate
{
    gchar *from;
    GList *recipients;
    GList *temporary_failed_recipients;
    GList *rejected_recipients;
    MilterHeaders *headers;
    guint64 body_size;
    MilterState state;
    MilterStatus status;
    MilterHeaders *added_headers;
    MilterHeaders *removed_headers;
    gboolean quarantine;
    GTimeVal start_time;
    GTimeVal end_time;
    gdouble elapsed_time;
    gboolean elapsed_time_set;
};

enum
{
    PROP_0,
    PROP_FROM,
    PROP_HEADERS,
    PROP_BODY_SIZE,
    PROP_STATE,
    PROP_STATUS,
    PROP_ADDED_HEADERS,
    PROP_REMOVED_HEADERS,
    PROP_QUARANTINE,
    PROP_ELAPSED_TIME
};

G_DEFINE_TYPE(MilterMessageResult, milter_message_result, G_TYPE_OBJECT);

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
milter_message_result_class_init (MilterMessageResultClass *klass)
{
    GObjectClass *gobject_class;
    GParamSpec *spec;

    gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->dispose      = dispose;
    gobject_class->set_property = set_property;
    gobject_class->get_property = get_property;


    spec = g_param_spec_string("from",
                               "From",
                               "The sender of the message",
                               NULL,
                               G_PARAM_READWRITE);
    g_object_class_install_property(gobject_class, PROP_FROM, spec);

    spec = g_param_spec_object("headers",
                               "Header",
                               "The headers of the message",
                               MILTER_TYPE_HEADERS,
                               G_PARAM_READWRITE);
    g_object_class_install_property(gobject_class, PROP_HEADERS, spec);

    spec = g_param_spec_uint64("body-size",
                               "Body size",
                               "The size of body of the message",
                               0,
                               G_MAXUINT64,
                               0,
                               G_PARAM_READWRITE);
    g_object_class_install_property(gobject_class, PROP_BODY_SIZE, spec);

    spec = g_param_spec_enum("state",
                             "State",
                             "The last state of the message",
                             MILTER_TYPE_STATE,
                             MILTER_STATE_INVALID,
                             G_PARAM_READWRITE);
    g_object_class_install_property(gobject_class, PROP_STATE, spec);

    spec = g_param_spec_enum("status",
                             "Status",
                             "The status of the message",
                             MILTER_TYPE_STATUS,
                             MILTER_STATUS_DEFAULT,
                             G_PARAM_READWRITE);
    g_object_class_install_property(gobject_class, PROP_STATUS, spec);

    spec = g_param_spec_object("added-headers",
                               "Added headers",
                               "The added headers of the message",
                               MILTER_TYPE_HEADERS,
                               G_PARAM_READWRITE);
    g_object_class_install_property(gobject_class, PROP_ADDED_HEADERS, spec);

    spec = g_param_spec_object("removed-headers",
                               "Removed headers",
                               "The removed headers of the message",
                               MILTER_TYPE_HEADERS,
                               G_PARAM_READWRITE);
    g_object_class_install_property(gobject_class, PROP_REMOVED_HEADERS, spec);

    spec = g_param_spec_boolean("quarantine",
                                "Quarantine",
                                "The status of the message",
                                FALSE,
                                G_PARAM_READWRITE);
    g_object_class_install_property(gobject_class, PROP_QUARANTINE, spec);

    spec = g_param_spec_double("elapsed-time",
                               "Elapsed time",
                               "The elapsed time of the message",
                               0.0,
                               G_MAXDOUBLE,
                               0.0,
                               G_PARAM_READWRITE);
    g_object_class_install_property(gobject_class, PROP_ELAPSED_TIME, spec);

    g_type_class_add_private(gobject_class,
                             sizeof(MilterMessageResultPrivate));
}

static void
milter_message_result_init (MilterMessageResult *headers)
{
    MilterMessageResultPrivate *priv;

    priv = MILTER_MESSAGE_RESULT_GET_PRIVATE(headers);
    priv->from = NULL;
    priv->recipients = NULL;
    priv->temporary_failed_recipients = NULL;
    priv->rejected_recipients = NULL;
    priv->headers = NULL;
    priv->body_size = 0;
    priv->state = MILTER_STATE_INVALID;
    priv->status = MILTER_STATUS_DEFAULT;
    priv->added_headers = NULL;
    priv->removed_headers = NULL;
    priv->quarantine = FALSE;
    priv->start_time.tv_sec = 0;
    priv->start_time.tv_usec = 0;
    priv->end_time.tv_sec = 0;
    priv->end_time.tv_usec = 0;
    priv->elapsed_time = 0.0;
    priv->elapsed_time_set = FALSE;
}

static void
free_recipients (GList *recipients)
{
    g_list_foreach(recipients, (GFunc)g_free, NULL);
    g_list_free(recipients);
}

static void
dispose_recipients (MilterMessageResultPrivate *priv)
{
    if (priv->recipients) {
        free_recipients(priv->recipients);
        priv->recipients = NULL;
    }
}

static void
dispose_temporary_failed_recipients (MilterMessageResultPrivate *priv)
{
    if (priv->temporary_failed_recipients) {
        free_recipients(priv->temporary_failed_recipients);
        priv->temporary_failed_recipients = NULL;
    }
}

static void
dispose_rejected_recipients (MilterMessageResultPrivate *priv)
{
    if (priv->rejected_recipients) {
        free_recipients(priv->rejected_recipients);
        priv->rejected_recipients = NULL;
    }
}

static void
dispose_headers (MilterMessageResultPrivate *priv)
{
    if (priv->headers) {
        g_object_unref(priv->headers);
        priv->headers = NULL;
    }
}

static void
dispose_added_headers (MilterMessageResultPrivate *priv)
{
    if (priv->added_headers) {
        g_object_unref(priv->added_headers);
        priv->added_headers = NULL;
    }
}

static void
dispose_removed_headers (MilterMessageResultPrivate *priv)
{
    if (priv->removed_headers) {
        g_object_unref(priv->removed_headers);
        priv->removed_headers = NULL;
    }
}

static void
dispose (GObject *object)
{
    MilterMessageResultPrivate *priv;

    priv = MILTER_MESSAGE_RESULT_GET_PRIVATE(object);

    if (priv->from) {
        g_free(priv->from);
        priv->from = NULL;
    }

    dispose_recipients(priv);
    dispose_temporary_failed_recipients(priv);
    dispose_rejected_recipients(priv);
    dispose_headers(priv);
    dispose_added_headers(priv);
    dispose_removed_headers(priv);

    G_OBJECT_CLASS(milter_message_result_parent_class)->dispose(object);
}

static void
set_property (GObject      *object,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
    MilterMessageResult *result;

    result = MILTER_MESSAGE_RESULT(object);
    switch (prop_id) {
    case PROP_FROM:
        milter_message_result_set_from(result, g_value_get_string(value));
        break;
    case PROP_HEADERS:
        milter_message_result_set_headers(result, g_value_get_object(value));
        break;
    case PROP_BODY_SIZE:
        milter_message_result_set_body_size(result, g_value_get_uint64(value));
        break;
    case PROP_STATE:
        milter_message_result_set_state(result, g_value_get_enum(value));
        break;
    case PROP_STATUS:
        milter_message_result_set_status(result, g_value_get_enum(value));
        break;
    case PROP_ADDED_HEADERS:
        milter_message_result_set_added_headers(result,
                                                g_value_get_object(value));
        break;
    case PROP_REMOVED_HEADERS:
        milter_message_result_set_removed_headers(result,
                                                  g_value_get_object(value));
        break;
    case PROP_QUARANTINE:
        milter_message_result_set_quarantine(result,
                                             g_value_get_boolean(value));
        break;
    case PROP_ELAPSED_TIME:
        milter_message_result_set_elapsed_time(result,
                                               g_value_get_double(value));
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
    MilterMessageResult *result;
    MilterMessageResultPrivate *priv;

    result = MILTER_MESSAGE_RESULT(object);
    priv = MILTER_MESSAGE_RESULT_GET_PRIVATE(result);
    switch (prop_id) {
    case PROP_FROM:
        g_value_set_string(value, priv->from);
        break;
    case PROP_HEADERS:
        g_value_set_object(value, priv->headers);
        break;
    case PROP_BODY_SIZE:
        g_value_set_uint64(value, priv->body_size);
        break;
    case PROP_STATE:
        g_value_set_enum(value, priv->state);
        break;
    case PROP_STATUS:
        g_value_set_enum(value, priv->status);
        break;
    case PROP_ADDED_HEADERS:
        g_value_set_object(value, priv->added_headers);
        break;
    case PROP_REMOVED_HEADERS:
        g_value_set_object(value, priv->removed_headers);
        break;
    case PROP_QUARANTINE:
        g_value_set_boolean(value, priv->quarantine);
        break;
    case PROP_ELAPSED_TIME:
        g_value_set_double(value,
                           milter_message_result_get_elapsed_time(result));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

MilterMessageResult *
milter_message_result_new (void)
{
    return g_object_new(MILTER_TYPE_MESSAGE_RESULT, NULL);
}

void
milter_message_result_start (MilterMessageResult *result)
{
    GTimeVal current_time;
    MilterHeaders *headers, *added_headers, *removed_headers;

    g_get_current_time(&current_time);
    milter_message_result_set_start_time(result, &current_time);

    headers = milter_headers_new();
    milter_message_result_set_headers(result, headers);
    g_object_unref(headers);

    added_headers = milter_headers_new();
    milter_message_result_set_added_headers(result, added_headers);
    g_object_unref(added_headers);

    removed_headers = milter_headers_new();
    milter_message_result_set_removed_headers(result, removed_headers);
    g_object_unref(removed_headers);
}

static gdouble
compute_elapsed (GTimeVal *start_time, GTimeVal *end_time)
{
    return (end_time->tv_sec - start_time->tv_sec) +
        (end_time->tv_usec - start_time->tv_usec) / 1000000.0;
}

void
milter_message_result_stop (MilterMessageResult *result)
{
    GTimeVal current_time;
    GTimeVal *start_time;

    g_get_current_time(&current_time);
    milter_message_result_set_end_time(result, &current_time);

    start_time = milter_message_result_get_start_time(result);
    if (start_time->tv_sec > 0) {
        gdouble elapsed;

        elapsed = compute_elapsed(start_time, &current_time);
        milter_message_result_set_elapsed_time(result, elapsed);
    }
}

const gchar *
milter_message_result_get_from (MilterMessageResult *result)
{
    return MILTER_MESSAGE_RESULT_GET_PRIVATE(result)->from;
}

void
milter_message_result_set_from (MilterMessageResult *result,
                                const gchar *from)
{
    MilterMessageResultPrivate *priv;

    priv = MILTER_MESSAGE_RESULT_GET_PRIVATE(result);

    if (priv->from)
        g_free(priv->from);
    priv->from = g_strdup(from);
}

GList *
milter_message_result_get_recipients (MilterMessageResult *result)
{
    return MILTER_MESSAGE_RESULT_GET_PRIVATE(result)->recipients;
}

void
milter_message_result_set_recipients (MilterMessageResult *result,
                                      const GList *recipients)
{
    MilterMessageResultPrivate *priv;
    const GList *node;

    priv = MILTER_MESSAGE_RESULT_GET_PRIVATE(result);
    dispose_recipients(priv);

    for (node = recipients; node; node = g_list_next(node)) {
        const gchar *recipient = node->data;

        priv->recipients = g_list_append(priv->recipients, g_strdup(recipient));
    }
}

void
milter_message_result_add_recipient (MilterMessageResult *result,
                                     const gchar *recipient)
{
    MilterMessageResultPrivate *priv;

    priv = MILTER_MESSAGE_RESULT_GET_PRIVATE(result);
    priv->recipients = g_list_append(priv->recipients, g_strdup(recipient));
}

void
milter_message_result_remove_recipient (MilterMessageResult *result,
                                        const gchar *recipient)
{
    MilterMessageResultPrivate *priv;
    GList *node;

    priv = MILTER_MESSAGE_RESULT_GET_PRIVATE(result);
    for (node = priv->recipients; node; node = g_list_next(node)) {
        gchar *_recipient = node->data;

        if (strcmp(_recipient, recipient) == 0) {
            g_free(_recipient);
            priv->recipients = g_list_remove_link(priv->recipients, node);
        }
    }
}

GList *
milter_message_result_get_temporary_failed_recipients (MilterMessageResult *result)
{
    return MILTER_MESSAGE_RESULT_GET_PRIVATE(result)->temporary_failed_recipients;
}

void
milter_message_result_set_temporary_failed_recipients (MilterMessageResult *result,
                                                       const GList *recipients)
{
    MilterMessageResultPrivate *priv;
    const GList *node;

    priv = MILTER_MESSAGE_RESULT_GET_PRIVATE(result);
    dispose_temporary_failed_recipients(priv);

    for (node = recipients; node; node = g_list_next(node)) {
        const gchar *recipient = node->data;

        priv->temporary_failed_recipients =
            g_list_append(priv->temporary_failed_recipients,
                          g_strdup(recipient));
    }
}

void
milter_message_result_add_temporary_failed_recipient (MilterMessageResult *result,
                                                      const gchar *recipient)
{
    MilterMessageResultPrivate *priv;

    priv = MILTER_MESSAGE_RESULT_GET_PRIVATE(result);
    priv->temporary_failed_recipients =
        g_list_append(priv->temporary_failed_recipients, g_strdup(recipient));
}

GList *
milter_message_result_get_rejected_recipients (MilterMessageResult *result)
{
    return MILTER_MESSAGE_RESULT_GET_PRIVATE(result)->rejected_recipients;
}

void
milter_message_result_set_rejected_recipients (MilterMessageResult *result,
                                               const GList *recipients)
{
    MilterMessageResultPrivate *priv;
    const GList *node;

    priv = MILTER_MESSAGE_RESULT_GET_PRIVATE(result);
    dispose_rejected_recipients(priv);

    for (node = recipients; node; node = g_list_next(node)) {
        const gchar *recipient = node->data;

        priv->rejected_recipients =
            g_list_append(priv->rejected_recipients, g_strdup(recipient));
    }
}

void
milter_message_result_add_rejected_recipient (MilterMessageResult *result,
                                              const gchar *recipient)
{
    MilterMessageResultPrivate *priv;

    priv = MILTER_MESSAGE_RESULT_GET_PRIVATE(result);
    priv->rejected_recipients =
        g_list_append(priv->rejected_recipients, g_strdup(recipient));
}

MilterHeaders *
milter_message_result_get_headers (MilterMessageResult *result)
{
    return MILTER_MESSAGE_RESULT_GET_PRIVATE(result)->headers;
}

void
milter_message_result_set_headers (MilterMessageResult *result,
                                   MilterHeaders *headers)
{
    MilterMessageResultPrivate *priv;

    priv = MILTER_MESSAGE_RESULT_GET_PRIVATE(result);
    dispose_headers(priv);
    priv->headers = headers;
    if (priv->headers)
        g_object_ref(priv->headers);
}

guint64
milter_message_result_get_body_size (MilterMessageResult *result)
{
    return MILTER_MESSAGE_RESULT_GET_PRIVATE(result)->body_size;
}

void
milter_message_result_set_body_size (MilterMessageResult *result,
                                     guint64 body_size)
{
    MilterMessageResultPrivate *priv;

    priv = MILTER_MESSAGE_RESULT_GET_PRIVATE(result);
    priv->body_size = body_size;
}

void
milter_message_result_add_body_size (MilterMessageResult *result,
                                     guint64 body_size)
{
    MilterMessageResultPrivate *priv;

    priv = MILTER_MESSAGE_RESULT_GET_PRIVATE(result);
    priv->body_size += body_size;
}

MilterState
milter_message_result_get_state (MilterMessageResult *result)
{
    return MILTER_MESSAGE_RESULT_GET_PRIVATE(result)->state;
}

void
milter_message_result_set_state (MilterMessageResult *result,
                                 MilterState state)
{
    MilterMessageResultPrivate *priv;

    priv = MILTER_MESSAGE_RESULT_GET_PRIVATE(result);
    priv->state = state;
}

MilterStatus
milter_message_result_get_status (MilterMessageResult *result)
{
    return MILTER_MESSAGE_RESULT_GET_PRIVATE(result)->status;
}

void
milter_message_result_set_status (MilterMessageResult *result,
                                  MilterStatus status)
{
    MilterMessageResultPrivate *priv;

    priv = MILTER_MESSAGE_RESULT_GET_PRIVATE(result);
    priv->status = status;
}

MilterHeaders *
milter_message_result_get_added_headers (MilterMessageResult *result)
{
    return MILTER_MESSAGE_RESULT_GET_PRIVATE(result)->added_headers;
}

void
milter_message_result_set_added_headers (MilterMessageResult *result,
                                         MilterHeaders *added_headers)
{
    MilterMessageResultPrivate *priv;

    priv = MILTER_MESSAGE_RESULT_GET_PRIVATE(result);
    dispose_added_headers(priv);
    priv->added_headers = added_headers;
    if (priv->added_headers)
        g_object_ref(priv->added_headers);
}

MilterHeaders *
milter_message_result_get_removed_headers (MilterMessageResult *result)
{
    return MILTER_MESSAGE_RESULT_GET_PRIVATE(result)->removed_headers;
}

void
milter_message_result_set_removed_headers (MilterMessageResult *result,
                                           MilterHeaders *removed_headers)
{
    MilterMessageResultPrivate *priv;

    priv = MILTER_MESSAGE_RESULT_GET_PRIVATE(result);
    dispose_removed_headers(priv);
    priv->removed_headers = removed_headers;
    if (priv->removed_headers)
        g_object_ref(priv->removed_headers);
}

gboolean
milter_message_result_is_quarantine (MilterMessageResult *result)
{
    return MILTER_MESSAGE_RESULT_GET_PRIVATE(result)->quarantine;
}

void
milter_message_result_set_quarantine (MilterMessageResult *result,
                                      gboolean quarantine)
{
    MILTER_MESSAGE_RESULT_GET_PRIVATE(result)->quarantine = quarantine;
}

GTimeVal *
milter_message_result_get_start_time (MilterMessageResult *result)
{
    return &MILTER_MESSAGE_RESULT_GET_PRIVATE(result)->start_time;
}

void
milter_message_result_set_start_time (MilterMessageResult *result,
                                      GTimeVal *time)
{
    MilterMessageResultPrivate *priv;

    priv = MILTER_MESSAGE_RESULT_GET_PRIVATE(result);
    if (time) {
        priv->start_time = *time;
    } else {
        priv->start_time.tv_sec = 0;
        priv->start_time.tv_usec = 0;
    }
}

GTimeVal *
milter_message_result_get_end_time (MilterMessageResult *result)
{
    return &MILTER_MESSAGE_RESULT_GET_PRIVATE(result)->end_time;
}

void
milter_message_result_set_end_time (MilterMessageResult *result,
                                    GTimeVal *time)
{
    MilterMessageResultPrivate *priv;

    priv = MILTER_MESSAGE_RESULT_GET_PRIVATE(result);
    if (time) {
        priv->end_time = *time;
    } else {
        priv->end_time.tv_sec = 0;
        priv->end_time.tv_usec = 0;
    }
}

gdouble
milter_message_result_get_elapsed_time (MilterMessageResult *result)
{
    MilterMessageResultPrivate *priv;

    priv = MILTER_MESSAGE_RESULT_GET_PRIVATE(result);
    if (!priv->elapsed_time_set &&
        priv->start_time.tv_sec > 0 && priv->end_time.tv_sec > 0) {
        return compute_elapsed(&(priv->start_time), &(priv->end_time));
    } else {
        return priv->elapsed_time;
    }
}

void
milter_message_result_set_elapsed_time (MilterMessageResult *result,
                                        gdouble elapsed_time)
{
    MilterMessageResultPrivate *priv;

    priv = MILTER_MESSAGE_RESULT_GET_PRIVATE(result);
    priv->elapsed_time = elapsed_time;
    priv->elapsed_time_set = TRUE;
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
