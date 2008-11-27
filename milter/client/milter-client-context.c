/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Copyright (C) 2008  Kouhei Sutou <kou@cozmixng.org>
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
#  include "../../config.h"
#endif /* HAVE_CONFIG_H */

#include <string.h>
#include <stdlib.h>

#include <milter/core.h>
#include "../client.h"
#include "milter-client-context.h"
#include "milter-client-enum-types.h"
#include "milter/core/milter-marshalers.h"

enum
{
    NEGOTIATE,
    NEGOTIATE_RESPONSE,
    CONNECT,
    CONNECT_RESPONSE,
    HELO,
    HELO_RESPONSE,
    ENVELOPE_FROM,
    ENVELOPE_FROM_RESPONSE,
    ENVELOPE_RECIPIENT,
    ENVELOPE_RECIPIENT_RESPONSE,
    DATA,
    DATA_RESPONSE,
    UNKNOWN,
    UNKNOWN_RESPONSE,
    HEADER,
    HEADER_RESPONSE,
    END_OF_HEADER,
    END_OF_HEADER_RESPONSE,
    BODY,
    BODY_RESPONSE,
    END_OF_MESSAGE,
    END_OF_MESSAGE_RESPONSE,
    QUIT,
    QUIT_RESPONSE,
    ABORT,
    ABORT_RESPONSE,
    DEFINE_MACRO,

    MTA_TIMEOUT,
    LAST_SIGNAL
};

static gint signals[LAST_SIGNAL] = {0};

static gboolean    status_accumulator  (GSignalInvocationHint *hint,
                                        GValue *return_accumulator,
                                        const GValue *context_return,
                                        gpointer data);

#define MILTER_CLIENT_CONTEXT_GET_PRIVATE(obj)                          \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj),                                 \
                                 MILTER_TYPE_CLIENT_CONTEXT,            \
                                 MilterClientContextPrivate))

typedef struct _MilterClientContextPrivate	MilterClientContextPrivate;
struct _MilterClientContextPrivate
{
    gpointer private_data;
    GDestroyNotify private_data_destroy;
    guint reply_code;
    gchar *extended_reply_code;
    gchar *reply_message;
    guint mta_timeout;
    guint timeout_id;
};

G_DEFINE_TYPE(MilterClientContext, milter_client_context,
              MILTER_TYPE_PROTOCOL_AGENT)

static void dispose        (GObject         *object);
static void set_property   (GObject         *object,
                            guint            prop_id,
                            const GValue    *value,
                            GParamSpec      *pspec);
static void get_property   (GObject         *object,
                            guint            prop_id,
                            GValue          *value,
                            GParamSpec      *pspec);

static MilterDecoder *decoder_new    (MilterAgent *agent);
static MilterEncoder *encoder_new    (MilterAgent *agent);

static MilterStatus default_negotiate  (MilterClientContext *context,
                                        MilterOption  *option);
static MilterStatus default_connect    (MilterClientContext *context,
                                        const gchar   *host_name,
                                        struct sockaddr *address,
                                        socklen_t      address_length);
static MilterStatus default_helo       (MilterClientContext *context,
                                        const gchar   *fqdn);
static MilterStatus default_envelope_from
                                       (MilterClientContext *context,
                                        const gchar   *from);
static MilterStatus default_envelope_recipient
                                       (MilterClientContext *context,
                                        const gchar   *recipient);
static MilterStatus default_data       (MilterClientContext *context);
static MilterStatus default_header     (MilterClientContext *context,
                                        const gchar   *name,
                                        const gchar   *value);
static MilterStatus default_end_of_header
                                       (MilterClientContext *context);
static MilterStatus default_body       (MilterClientContext *context,
                                        const gchar   *chunk,
                                        gsize          size);
static MilterStatus default_end_of_message
                                       (MilterClientContext *context);
static MilterStatus default_quit       (MilterClientContext *context);
static MilterStatus default_abort      (MilterClientContext *context);
static void         negotiate_response (MilterClientContext *context,
                                        MilterOption        *option,
                                        MilterStatus        status);
static void         connect_response   (MilterClientContext *context,
                                        MilterStatus         status);
static void         helo_response      (MilterClientContext *context,
                                        MilterStatus         status);
static void         envelope_from_response
                                       (MilterClientContext *context,
                                        MilterStatus         status);
static void         envelope_recipient_response
                                       (MilterClientContext *context,
                                        MilterStatus         status);
static void         data_response      (MilterClientContext *context,
                                        MilterStatus         status);
static void         header_response    (MilterClientContext *context,
                                        MilterStatus         status);
static void         end_of_header_response
                                       (MilterClientContext *context,
                                        MilterStatus         status);
static void         body_response      (MilterClientContext *context,
                                        MilterStatus         status);
static void         end_of_message_response
                                       (MilterClientContext *context,
                                        MilterStatus         status);
static void         quit_response      (MilterClientContext *context,
                                        MilterStatus         status);
static void         abort_response     (MilterClientContext *context,
                                        MilterStatus         status);

static void
milter_client_context_class_init (MilterClientContextClass *klass)
{
    GObjectClass *gobject_class;
    MilterAgentClass *agent_class;

    gobject_class = G_OBJECT_CLASS(klass);
    agent_class = MILTER_AGENT_CLASS(klass);

    gobject_class->dispose      = dispose;
    gobject_class->set_property = set_property;
    gobject_class->get_property = get_property;

    agent_class->decoder_new   = decoder_new;
    agent_class->encoder_new   = encoder_new;

    klass->negotiate = default_negotiate;
    klass->connect = default_connect;
    klass->helo = default_helo;
    klass->envelope_from = default_envelope_from;
    klass->envelope_recipient = default_envelope_recipient;
    klass->data = default_data;
    klass->header = default_header;
    klass->end_of_header = default_end_of_header;
    klass->body = default_body;
    klass->end_of_message = default_end_of_message;
    klass->quit = default_quit;
    klass->abort = default_abort;

    klass->negotiate_response = negotiate_response;
    klass->connect_response = connect_response;
    klass->helo_response = helo_response;
    klass->envelope_from_response = envelope_from_response;
    klass->envelope_recipient_response = envelope_recipient_response;
    klass->data_response = data_response;
    klass->header_response = header_response;
    klass->end_of_header_response = end_of_header_response;
    klass->body_response = body_response;
    klass->end_of_message_response = end_of_message_response;
    klass->quit_response = quit_response;
    klass->abort_response = abort_response;

    signals[NEGOTIATE] =
        g_signal_new("negotiate",
                     MILTER_TYPE_CLIENT_CONTEXT,
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterClientContextClass, negotiate),
                     status_accumulator, NULL,
                     _milter_marshal_ENUM__OBJECT,
                     MILTER_TYPE_STATUS, 1, MILTER_TYPE_OPTION);

    signals[NEGOTIATE_RESPONSE] =
        g_signal_new("negotiate-response",
                     MILTER_TYPE_CLIENT_CONTEXT,
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterClientContextClass,
                                     negotiate_response),
                     NULL, NULL,
                     _milter_marshal_VOID__OBJECT_ENUM,
                     G_TYPE_NONE, 2, MILTER_TYPE_OPTION, MILTER_TYPE_STATUS);

    signals[CONNECT] =
        g_signal_new("connect",
                     MILTER_TYPE_CLIENT_CONTEXT,
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterClientContextClass, connect),
                     status_accumulator, NULL,
                     _milter_marshal_ENUM__STRING_POINTER_UINT,
                     MILTER_TYPE_STATUS, 3,
                     G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_UINT);

    signals[CONNECT_RESPONSE] =
        g_signal_new("connect-response",
                     MILTER_TYPE_CLIENT_CONTEXT,
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterClientContextClass,
                                     connect_response),
                     NULL, NULL,
                     _milter_marshal_VOID__ENUM,
                     G_TYPE_NONE, 1, MILTER_TYPE_STATUS);

    signals[HELO] =
        g_signal_new("helo",
                     MILTER_TYPE_CLIENT_CONTEXT,
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterClientContextClass, helo),
                     status_accumulator, NULL,
                     _milter_marshal_ENUM__STRING,
                     MILTER_TYPE_STATUS, 1, G_TYPE_STRING);

    signals[HELO_RESPONSE] =
        g_signal_new("helo-response",
                     MILTER_TYPE_CLIENT_CONTEXT,
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterClientContextClass, helo_response),
                     NULL, NULL,
                     _milter_marshal_VOID__ENUM,
                     G_TYPE_NONE, 1, MILTER_TYPE_STATUS);

    signals[ENVELOPE_FROM] =
        g_signal_new("envelope-from",
                     MILTER_TYPE_CLIENT_CONTEXT,
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterClientContextClass, envelope_from),
                     status_accumulator, NULL,
                     _milter_marshal_ENUM__STRING,
                     MILTER_TYPE_STATUS, 1, G_TYPE_STRING);

    signals[ENVELOPE_FROM_RESPONSE] =
        g_signal_new("envelope-from-response",
                     MILTER_TYPE_CLIENT_CONTEXT,
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterClientContextClass,
                                     envelope_from_response),
                     NULL, NULL,
                     _milter_marshal_VOID__ENUM,
                     G_TYPE_NONE, 1, MILTER_TYPE_STATUS);

    signals[ENVELOPE_RECIPIENT] =
        g_signal_new("envelope-recipient",
                     MILTER_TYPE_CLIENT_CONTEXT,
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterClientContextClass,
                                     envelope_recipient),
                     status_accumulator, NULL,
                     _milter_marshal_ENUM__STRING,
                     MILTER_TYPE_STATUS, 1, G_TYPE_STRING);

    signals[ENVELOPE_RECIPIENT_RESPONSE] =
        g_signal_new("envelope-recipient-response",
                     MILTER_TYPE_CLIENT_CONTEXT,
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterClientContextClass,
                                     envelope_recipient_response),
                     NULL, NULL,
                     _milter_marshal_VOID__ENUM,
                     G_TYPE_NONE, 1, MILTER_TYPE_STATUS);

    signals[DATA] =
        g_signal_new("data",
                     MILTER_TYPE_CLIENT_CONTEXT,
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterClientContextClass, data),
                     status_accumulator, NULL,
                     _milter_marshal_ENUM__VOID,
                     MILTER_TYPE_STATUS, 0);

    signals[DATA_RESPONSE] =
        g_signal_new("data-response",
                     MILTER_TYPE_CLIENT_CONTEXT,
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterClientContextClass, data_response),
                     NULL, NULL,
                     _milter_marshal_VOID__ENUM,
                     G_TYPE_NONE, 1, MILTER_TYPE_STATUS);

    signals[UNKNOWN] =
        g_signal_new("unknown",
                     MILTER_TYPE_CLIENT_CONTEXT,
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterClientContextClass, unknown),
                     status_accumulator, NULL,
                     _milter_marshal_ENUM__STRING_POINTER_UINT,
                     MILTER_TYPE_STATUS, 3,
                     G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_UINT);

    signals[UNKNOWN_RESPONSE] =
        g_signal_new("unknown-response",
                     MILTER_TYPE_CLIENT_CONTEXT,
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterClientContextClass, unknown_response),
                     NULL, NULL,
                     _milter_marshal_VOID__ENUM,
                     G_TYPE_NONE, 1, MILTER_TYPE_STATUS);

    signals[HEADER] =
        g_signal_new("header",
                     MILTER_TYPE_CLIENT_CONTEXT,
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterClientContextClass, header),
                     status_accumulator, NULL,
                     _milter_marshal_ENUM__STRING_STRING,
                     MILTER_TYPE_STATUS, 2, G_TYPE_STRING, G_TYPE_STRING);

    signals[HEADER_RESPONSE] =
        g_signal_new("header-response",
                     MILTER_TYPE_CLIENT_CONTEXT,
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterClientContextClass, header_response),
                     NULL, NULL,
                     _milter_marshal_VOID__ENUM,
                     G_TYPE_NONE, 1, MILTER_TYPE_STATUS);

    signals[END_OF_HEADER] =
        g_signal_new("end-of-header",
                     MILTER_TYPE_CLIENT_CONTEXT,
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterClientContextClass, end_of_header),
                     status_accumulator, NULL,
                     _milter_marshal_ENUM__VOID,
                     MILTER_TYPE_STATUS, 0);

    signals[END_OF_HEADER_RESPONSE] =
        g_signal_new("end-of-header-response",
                     MILTER_TYPE_CLIENT_CONTEXT,
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterClientContextClass,
                                     end_of_header_response),
                     NULL, NULL,
                     _milter_marshal_VOID__ENUM,
                     G_TYPE_NONE, 1, MILTER_TYPE_STATUS);

    signals[BODY] =
        g_signal_new("body",
                     MILTER_TYPE_CLIENT_CONTEXT,
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterClientContextClass, body),
                     status_accumulator, NULL,
                     _milter_marshal_ENUM__STRING_UINT,
                     MILTER_TYPE_STATUS, 2, G_TYPE_STRING, G_TYPE_UINT);

    signals[BODY_RESPONSE] =
        g_signal_new("body_response",
                     MILTER_TYPE_CLIENT_CONTEXT,
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterClientContextClass, body_response),
                     NULL, NULL,
                     _milter_marshal_VOID__ENUM,
                     G_TYPE_NONE, 1, MILTER_TYPE_STATUS);

    signals[END_OF_MESSAGE] =
        g_signal_new("end-of-message",
                     MILTER_TYPE_CLIENT_CONTEXT,
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterClientContextClass, end_of_message),
                     status_accumulator, NULL,
                     _milter_marshal_ENUM__VOID,
                     MILTER_TYPE_STATUS, 0);

    signals[END_OF_MESSAGE_RESPONSE] =
        g_signal_new("end-of-message-response",
                     MILTER_TYPE_CLIENT_CONTEXT,
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterClientContextClass,
                                     end_of_message_response),
                     NULL, NULL,
                     _milter_marshal_VOID__ENUM,
                     G_TYPE_NONE, 1, MILTER_TYPE_STATUS);

    signals[QUIT] =
        g_signal_new("quit",
                     MILTER_TYPE_CLIENT_CONTEXT,
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterClientContextClass, quit),
                     status_accumulator, NULL,
                     _milter_marshal_ENUM__VOID,
                     MILTER_TYPE_STATUS, 0);

    signals[QUIT_RESPONSE] =
        g_signal_new("quit-response",
                     MILTER_TYPE_CLIENT_CONTEXT,
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterClientContextClass, quit_response),
                     NULL, NULL,
                     _milter_marshal_VOID__ENUM,
                     G_TYPE_NONE, 1, MILTER_TYPE_STATUS);

    signals[ABORT] =
        g_signal_new("abort",
                     MILTER_TYPE_CLIENT_CONTEXT,
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterClientContextClass, abort),
                     status_accumulator, NULL,
                     _milter_marshal_ENUM__VOID,
                     MILTER_TYPE_STATUS, 0);

    signals[ABORT_RESPONSE] =
        g_signal_new("abort-response",
                     MILTER_TYPE_CLIENT_CONTEXT,
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterClientContextClass, abort_response),
                     NULL, NULL,
                     _milter_marshal_VOID__ENUM,
                     G_TYPE_NONE, 1, MILTER_TYPE_STATUS);

    signals[DEFINE_MACRO] =
        g_signal_new("define-macro",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterClientContextClass, define_macro),
                     NULL, NULL,
                     _milter_marshal_VOID__ENUM_POINTER,
                     G_TYPE_NONE, 2, MILTER_TYPE_COMMAND, G_TYPE_POINTER);

    signals[MTA_TIMEOUT] =
        g_signal_new("mta-timeout",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterClientContextClass, mta_timeout),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__VOID,
                     G_TYPE_NONE, 0);

    g_type_class_add_private(gobject_class, sizeof(MilterClientContextPrivate));
}

static void
milter_client_context_init (MilterClientContext *context)
{
    MilterClientContextPrivate *priv;

    priv = MILTER_CLIENT_CONTEXT_GET_PRIVATE(context);
    priv->private_data = NULL;
    priv->private_data_destroy = NULL;
    priv->reply_code = 0;
    priv->extended_reply_code = NULL;
    priv->reply_message = NULL;
    priv->mta_timeout = 7210;
    priv->timeout_id = 0;
}

static void
disable_timeout (MilterClientContext *context)
{
    MilterClientContextPrivate *priv;

    priv = MILTER_CLIENT_CONTEXT_GET_PRIVATE(context);

    if (priv->timeout_id > 0) {
        g_source_remove(priv->timeout_id);
        priv->timeout_id = 0;
    }
}

static void
dispose (GObject *object)
{
    MilterClientContextPrivate *priv;

    priv = MILTER_CLIENT_CONTEXT_GET_PRIVATE(object);

    disable_timeout(MILTER_CLIENT_CONTEXT(object));

    if (priv->private_data) {
        if (priv->private_data_destroy)
            priv->private_data_destroy(priv->private_data);
        priv->private_data = NULL;
    }
    priv->private_data_destroy = NULL;

    if (priv->extended_reply_code) {
        g_free(priv->extended_reply_code);
        priv->extended_reply_code = NULL;
    }

    if (priv->reply_message) {
        g_free(priv->reply_message);
        priv->reply_message = NULL;
    }

    G_OBJECT_CLASS(milter_client_context_parent_class)->dispose(object);
}

static void
set_property (GObject      *object,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
    MilterClientContextPrivate *priv;

    priv = MILTER_CLIENT_CONTEXT_GET_PRIVATE(object);
    switch (prop_id) {
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
    MilterClientContextPrivate *priv;

    priv = MILTER_CLIENT_CONTEXT_GET_PRIVATE(object);
    switch (prop_id) {
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

GQuark
milter_client_context_error_quark (void)
{
    return g_quark_from_static_string("milter-client-context-error-quark");
}

MilterClientContext *
milter_client_context_new (void)
{
    return g_object_new(MILTER_TYPE_CLIENT_CONTEXT, NULL);
}

gpointer
milter_client_context_get_private_data (MilterClientContext *context)
{
    MilterClientContextPrivate *priv;

    priv = MILTER_CLIENT_CONTEXT_GET_PRIVATE(context);
    return priv->private_data;
}

void
milter_client_context_set_private_data (MilterClientContext *context,
                                        gpointer data,
                                        GDestroyNotify destroy)
{
    MilterClientContextPrivate *priv;

    priv = MILTER_CLIENT_CONTEXT_GET_PRIVATE(context);
    if (priv->private_data && priv->private_data_destroy) {
        priv->private_data_destroy(priv->private_data);
    }
    priv->private_data = data;
    priv->private_data_destroy = destroy;
}

static const gchar *
extended_code_parse_class (const gchar *extended_code,
                           const gchar *current_point,
                           guint *klass, GError **error)
{
    gint i;

    for (i = 0; current_point[i]; i++) {
        if (current_point[i] == '.') {
            if (i == 0) {
                g_set_error(error,
                            MILTER_CLIENT_CONTEXT_ERROR,
                            MILTER_CLIENT_CONTEXT_ERROR_INVALID_CODE,
                            "class of extended status code is missing: <%s>",
                            extended_code);
                return NULL;
            }
            *klass = atoi(current_point);
            return current_point + i + 1;
        } else if (i >= 1) {
            g_set_error(error,
                        MILTER_CLIENT_CONTEXT_ERROR,
                        MILTER_CLIENT_CONTEXT_ERROR_INVALID_CODE,
                        "class of extended status code should be '2', "
                        "'4' or '5': <%s>", extended_code);
            return NULL;
        } else if (!(current_point[i] == '2' ||
                     current_point[i] == '4' ||
                     current_point[i] == '5')) {
            g_set_error(error,
                        MILTER_CLIENT_CONTEXT_ERROR,
                        MILTER_CLIENT_CONTEXT_ERROR_INVALID_CODE,
                        "class of extended status code should be '2', "
                        "'4' or '5': <%s>: <%c>",
                        extended_code, current_point[i]);
            return NULL;
        }
    }

    if (i == 0) {
        g_set_error(error,
                    MILTER_CLIENT_CONTEXT_ERROR,
                    MILTER_CLIENT_CONTEXT_ERROR_INVALID_CODE,
                    "class of extended status code is missing: <%s>",
                    extended_code);
        return NULL;
    } else {
        return current_point + i;
    }
}

static const gchar *
extended_code_parse_subject (const gchar *extended_code,
                             const gchar *current_point,
                             guint *subject, GError **error)
{
    gint i;

    for (i = 0; current_point[i]; i++) {
        if (current_point[i] == '.') {
            if (i == 0) {
                g_set_error(error,
                            MILTER_CLIENT_CONTEXT_ERROR,
                            MILTER_CLIENT_CONTEXT_ERROR_INVALID_CODE,
                            "subject of extended status code is missing: <%s>",
                            extended_code);
                return NULL;
            }
            *subject = atoi(current_point);
            return current_point + i + 1;
        } else if (i >= 3) {
            gchar subject_component[5];

            strncpy(subject_component, current_point, 4);
            subject_component[4] = '\0';
            g_set_error(error,
                        MILTER_CLIENT_CONTEXT_ERROR,
                        MILTER_CLIENT_CONTEXT_ERROR_INVALID_CODE,
                        "subject of extended status code should be "
                        "less than 1000: <%s>: <%s>",
                        subject_component, extended_code);
            return NULL;
        } else if (!g_ascii_isdigit(current_point[i])) {
            g_set_error(error,
                        MILTER_CLIENT_CONTEXT_ERROR,
                        MILTER_CLIENT_CONTEXT_ERROR_INVALID_CODE,
                        "subject of extended status code should be digit: "
                        "<%s>: <%c>",
                        extended_code, current_point[i]);
            return NULL;
        }
    }

    if (i == 0) {
        g_set_error(error,
                    MILTER_CLIENT_CONTEXT_ERROR,
                    MILTER_CLIENT_CONTEXT_ERROR_INVALID_CODE,
                    "subject of extended status code is missing: <%s>",
                    extended_code);
        return NULL;
    } else {
        return current_point + i;
    }
}

static const gchar *
extended_code_parse_detail (const gchar *extended_code,
                            const gchar *current_point,
                            guint *detail, GError **error)
{
    gint i;

    for (i = 0; current_point[i]; i++) {
        if (i >= 3) {
            gchar detail_component[5];

            strncpy(detail_component, current_point, 4);
            detail_component[4] = '\0';
            g_set_error(error,
                        MILTER_CLIENT_CONTEXT_ERROR,
                        MILTER_CLIENT_CONTEXT_ERROR_INVALID_CODE,
                        "detail of extended status code should be "
                        "less than 1000: <%s>: <%s>",
                        detail_component, extended_code);
                return NULL;
        } else if (!g_ascii_isdigit(current_point[i])) {
            g_set_error(error,
                        MILTER_CLIENT_CONTEXT_ERROR,
                        MILTER_CLIENT_CONTEXT_ERROR_INVALID_CODE,
                        "detail of extended status code should be digit: "
                        "<%c>: <%s>",
                        current_point[i], extended_code);
            return NULL;
        }
    }

    if (i == 0) {
        g_set_error(error,
                    MILTER_CLIENT_CONTEXT_ERROR,
                    MILTER_CLIENT_CONTEXT_ERROR_INVALID_CODE,
                    "detail of extended status code is missing: <%s>",
                    extended_code);
        return NULL;
    } else {
        return current_point + i;
    }
}

static gboolean
extended_code_parse (const gchar *extended_code,
                     guint *klass, guint *subject, guint *detail,
                     GError **error)
{
    const gchar *current_point;

    current_point = extended_code;
    current_point = extended_code_parse_class(extended_code, current_point,
                                              klass, error);
    if (!current_point)
        return FALSE;
    if (*current_point == '\0') {
        g_set_error(error,
                    MILTER_CLIENT_CONTEXT_ERROR,
                    MILTER_CLIENT_CONTEXT_ERROR_INVALID_CODE,
                    "subject of extended status code is missing: <%s>",
                    extended_code);
        return FALSE;
    }

    current_point = extended_code_parse_subject(extended_code, current_point,
                                                subject, error);
    if (!current_point)
        return FALSE;
    if (*current_point == '\0') {
        g_set_error(error,
                    MILTER_CLIENT_CONTEXT_ERROR,
                    MILTER_CLIENT_CONTEXT_ERROR_INVALID_CODE,
                    "detail of extended status code is missing: <%s>",
                    extended_code);
        return FALSE;
    }

    current_point = extended_code_parse_detail(extended_code, current_point,
                                               detail, error);
    if (!current_point)
        return FALSE;

    return TRUE;
}

gboolean
milter_client_context_set_reply (MilterClientContext *context,
                                 guint code,
                                 const gchar *extended_code,
                                 const gchar *message,
                                 GError **error)
{
    MilterClientContextPrivate *priv;

    priv = MILTER_CLIENT_CONTEXT_GET_PRIVATE(context);

    if (!(400 <= code && code < 600)) {
        g_set_error(error,
                    MILTER_CLIENT_CONTEXT_ERROR,
                    MILTER_CLIENT_CONTEXT_ERROR_INVALID_CODE,
                    "return code should be 4XX or 5XX: <%u>", code);
        return FALSE;
    }

    if (extended_code) {
        guint klass = 0, subject, detail;

        if (!extended_code_parse(extended_code, &klass, &subject, &detail,
                                 error))
            return FALSE;

        if (klass != code / 100) {
            g_set_error(error,
                        MILTER_CLIENT_CONTEXT_ERROR,
                        MILTER_CLIENT_CONTEXT_ERROR_INVALID_CODE,
                        "extended code should use "
                        "the same class of status code: <%u>:<%s>",
                        code, extended_code);
            return FALSE;
        }
    }

    priv->reply_code = code;
    if (priv->extended_reply_code)
        g_free(priv->extended_reply_code);
    priv->extended_reply_code = g_strdup(extended_code);
    if (priv->reply_message)
        g_free(priv->reply_message);
    priv->reply_message = g_strdup(message);

    return TRUE;
}

gchar *
milter_client_context_format_reply (MilterClientContext *context)
{
    MilterClientContextPrivate *priv;

    priv = MILTER_CLIENT_CONTEXT_GET_PRIVATE(context);

    return milter_utils_format_reply_code(priv->reply_code,
                                          priv->extended_reply_code,
                                          priv->reply_message);
}

static gboolean
cb_mta_timeout (gpointer data)
{
    g_signal_emit(data, signals[MTA_TIMEOUT], 0);

    return FALSE;
}

static gboolean
write_packet (MilterClientContext *context, gchar *packet, gsize packet_size)
{
    MilterClientContextPrivate *priv;
    gboolean success;
    GError *agent_error = NULL;

    if (!packet)
        return FALSE;

    priv = MILTER_CLIENT_CONTEXT_GET_PRIVATE(context);

    priv->timeout_id = g_timeout_add_seconds(priv->mta_timeout,
                                             cb_mta_timeout,
                                             context);
    success = milter_agent_write_packet(MILTER_AGENT(context),
                                        packet, packet_size,
                                        &agent_error);
    g_free(packet);
    if (agent_error) {
        GError *error = NULL;
        milter_utils_set_error_with_sub_error(&error,
                                              MILTER_CLIENT_CONTEXT_ERROR,
                                              MILTER_CLIENT_CONTEXT_ERROR_IO_ERROR,
                                              agent_error,
                                              "Failed to write to MTA");
        milter_error("%s", error->message);
        milter_error_emittable_emit(MILTER_ERROR_EMITTABLE(context),
                                    error);
        g_error_free(error);
    }

    return success;
}

gboolean
milter_client_context_negotiate_reply (MilterClientContext *context,
                                       MilterOption         *option,
                                       MilterMacrosRequests *macros_requests)
{
    gchar *packet = NULL;
    gsize packet_size;
    MilterEncoder *encoder;

    milter_info("sending NEGOTIATE: "
                "version = %d, action = %04X, step = %06X",
                milter_option_get_version(option),
                milter_option_get_action(option),
                milter_option_get_step(option));

    encoder = milter_agent_get_encoder(MILTER_AGENT(context));
    milter_reply_encoder_encode_negotiate(MILTER_REPLY_ENCODER(encoder),
                                          &packet, &packet_size,
                                          option, macros_requests);
    return write_packet(context, packet, packet_size);
}

gboolean
milter_client_context_add_header (MilterClientContext *context,
                                  const gchar *name, const gchar *value)
{
    gchar *packet = NULL;
    gsize packet_size;
    MilterEncoder *encoder;

    milter_info("sending ADD HEADER: <%s>=<%s>", name, value);
    encoder = milter_agent_get_encoder(MILTER_AGENT(context));
    milter_reply_encoder_encode_add_header(MILTER_REPLY_ENCODER(encoder),
                                           &packet, &packet_size,
                                           name, value);
    return write_packet(context, packet, packet_size);
}

gboolean
milter_client_context_insert_header (MilterClientContext *context,
                                     guint32 index,
                                     const gchar *name, const gchar *value)
{
    gchar *packet = NULL;
    gsize packet_size;
    MilterEncoder *encoder;

    milter_info("sending INSERT_HEADER: [%u] <%s>=<%s>", index, name, value);

    encoder = milter_agent_get_encoder(MILTER_AGENT(context));
    milter_reply_encoder_encode_insert_header(MILTER_REPLY_ENCODER(encoder),
                                              &packet, &packet_size,
                                              index, name, value);
    return write_packet(context, packet, packet_size);
}

gboolean
milter_client_context_change_header (MilterClientContext *context,
                                     const gchar *name, guint32 index,
                                     const gchar *value)
{
    gchar *packet = NULL;
    gsize packet_size;
    MilterEncoder *encoder;

    milter_info("sending CHANGE_HEADER: <%s>[%u]=<%s>", name, index, value);

    encoder = milter_agent_get_encoder(MILTER_AGENT(context));
    milter_reply_encoder_encode_change_header(MILTER_REPLY_ENCODER(encoder),
                                              &packet, &packet_size,
                                              name, index, value);
    return write_packet(context, packet, packet_size);
}

gboolean
milter_client_context_delete_header (MilterClientContext *context,
                                     const gchar *name, guint32 index)
{
    gchar *packet = NULL;
    gsize packet_size;
    MilterEncoder *encoder;

    milter_info("sending DELETE_HEADER(CHANGE_HEADER): <%s>[%u]", name, index);

    encoder = milter_agent_get_encoder(MILTER_AGENT(context));
    milter_reply_encoder_encode_delete_header(MILTER_REPLY_ENCODER(encoder),
                                              &packet, &packet_size,
                                              name, index);
    return write_packet(context, packet, packet_size);
}

gboolean
milter_client_context_change_from (MilterClientContext *context,
                                   const gchar *from,
                                   const gchar *parameters)
{
    MilterEncoder *encoder;
    gchar *packet = NULL;
    gsize packet_size;

    milter_info("sending CHANGE_FROM: <%s>=<%s>", from, parameters);

    encoder = milter_agent_get_encoder(MILTER_AGENT(context));
    milter_reply_encoder_encode_change_from(MILTER_REPLY_ENCODER(encoder),
                                            &packet, &packet_size,
                                            from, parameters);
    return write_packet(context, packet, packet_size);
}

gboolean
milter_client_context_add_recipient (MilterClientContext *context,
                                     const gchar *recipient,
                                     const gchar *parameters)
{
    MilterEncoder *encoder;
    gchar *packet = NULL;
    gsize packet_size;

    milter_info("sending ADD_RECIPIENT: <%s>=<%s>", recipient, parameters);

    encoder = milter_agent_get_encoder(MILTER_AGENT(context));
    milter_reply_encoder_encode_add_recipient(MILTER_REPLY_ENCODER(encoder),
                                              &packet, &packet_size,
                                              recipient, parameters);
    return write_packet(context, packet, packet_size);
}

gboolean
milter_client_context_delete_recipient (MilterClientContext *context,
                                        const gchar *recipient)
{
    MilterEncoder *encoder;
    gchar *packet = NULL;
    gsize packet_size;

    milter_info("sending DELETE_RECIPIENT: <%s>", recipient);

    encoder = milter_agent_get_encoder(MILTER_AGENT(context));
    milter_reply_encoder_encode_delete_recipient(MILTER_REPLY_ENCODER(encoder),
                                                 &packet, &packet_size,
                                                 recipient);
    return write_packet(context, packet, packet_size);
}

gboolean
milter_client_context_replace_body (MilterClientContext *context,
                                    const gchar *body, gsize body_size)
{
    gsize rest_size, packed_size = 0;
    MilterEncoder *encoder;
    MilterReplyEncoder *reply_encoder;

    milter_info("sending REPLACE_BODY: body_size = %" G_GSIZE_FORMAT, body_size);

    encoder = milter_agent_get_encoder(MILTER_AGENT(context));
    reply_encoder = MILTER_REPLY_ENCODER(encoder);
    for (rest_size = body_size; rest_size > 0; rest_size -= packed_size) {
        gchar *packet = NULL;
        gsize packet_size;
        gsize offset;

        offset = body_size - rest_size;
        milter_reply_encoder_encode_replace_body(reply_encoder,
                                                 &packet, &packet_size,
                                                 body + offset,
                                                 rest_size,
                                                 &packed_size);
        if (packed_size == 0)
            return FALSE;

        if (!write_packet(context, packet, packet_size))
            return FALSE;
    }

    return TRUE;
}

gboolean
milter_client_context_progress (MilterClientContext *context)
{
    MilterEncoder *encoder;
    gchar *packet = NULL;
    gsize packet_size;

    milter_info("sending PROGRESS");

    encoder = milter_agent_get_encoder(MILTER_AGENT(context));
    milter_reply_encoder_encode_progress(MILTER_REPLY_ENCODER(encoder),
                                         &packet, &packet_size);

    return write_packet(context, packet, packet_size);
}

gboolean
milter_client_context_quarantine (MilterClientContext *context,
                                  const gchar *reason)
{
    MilterEncoder *encoder;
    gchar *packet = NULL;
    gsize packet_size;

    milter_info("sending QUARANTINE: <%s>", reason);

    encoder = milter_agent_get_encoder(MILTER_AGENT(context));
    milter_reply_encoder_encode_quarantine(MILTER_REPLY_ENCODER(encoder),
                                           &packet, &packet_size,
                                           reason);

    return write_packet(context, packet, packet_size);
}

static gboolean
reply (MilterClientContext *context, MilterStatus status)
{
    gchar *packet = NULL;
    gsize packet_size;
    MilterEncoder *encoder;
    MilterReplyEncoder *reply_encoder;

    encoder = milter_agent_get_encoder(MILTER_AGENT(context));
    reply_encoder = MILTER_REPLY_ENCODER(encoder);

    switch (status) {
      case MILTER_STATUS_CONTINUE:
        milter_reply_encoder_encode_continue(reply_encoder,
                                             &packet, &packet_size);
        break;
      case MILTER_STATUS_REJECT:
        {
            gchar *code;

            code = milter_client_context_format_reply(context);
            if (code) {
                milter_reply_encoder_encode_reply_code(reply_encoder,
                                                       &packet, &packet_size,
                                                       code);
                g_free(code);
            } else {
                milter_reply_encoder_encode_reject(reply_encoder,
                                                   &packet, &packet_size);
            }
        }
        break;
      case MILTER_STATUS_TEMPORARY_FAILURE:
        milter_reply_encoder_encode_temporary_failure(reply_encoder,
                                                      &packet, &packet_size);
        break;
      case MILTER_STATUS_ACCEPT:
        milter_reply_encoder_encode_accept(reply_encoder, &packet, &packet_size);
        break;
      case MILTER_STATUS_DISCARD:
        milter_reply_encoder_encode_discard(reply_encoder,
                                            &packet, &packet_size);
        break;
      default:
        break;
    }

    if (!packet)
        return TRUE;

    return write_packet(context, packet, packet_size);
}

static MilterStatus
default_negotiate (MilterClientContext *context, MilterOption *option)
{
    return MILTER_STATUS_NOT_CHANGE;
}

static MilterStatus
default_connect (MilterClientContext *context, const gchar *host_name,
                 struct sockaddr *address, socklen_t address_length)
{
    return MILTER_STATUS_NOT_CHANGE;
}

static MilterStatus
default_helo (MilterClientContext *context, const gchar *fqdn)
{
    return MILTER_STATUS_NOT_CHANGE;
}

static MilterStatus
default_envelope_from (MilterClientContext *context, const gchar *from)
{
    return MILTER_STATUS_NOT_CHANGE;
}

static MilterStatus
default_envelope_recipient (MilterClientContext *context, const gchar *recipient)
{
    return MILTER_STATUS_NOT_CHANGE;
}

static MilterStatus
default_data (MilterClientContext *context)
{
    return MILTER_STATUS_NOT_CHANGE;
}

static MilterStatus
default_header (MilterClientContext *context, const gchar *name, const gchar *value)
{
    return MILTER_STATUS_NOT_CHANGE;
}

static MilterStatus
default_end_of_header (MilterClientContext *context)
{
    return MILTER_STATUS_NOT_CHANGE;
}

static MilterStatus
default_body (MilterClientContext *context, const gchar *chunk, gsize size)
{
    return MILTER_STATUS_NOT_CHANGE;
}

static MilterStatus
default_end_of_message (MilterClientContext *context)
{
    return MILTER_STATUS_NOT_CHANGE;
}

static MilterStatus
default_quit (MilterClientContext *context)
{
    return MILTER_STATUS_NOT_CHANGE;
}

static MilterStatus
default_abort (MilterClientContext *context)
{
    return MILTER_STATUS_NOT_CHANGE;
}

static void
set_macro_context (MilterClientContext *context, MilterCommand macro_context)
{
    MilterProtocolAgent *agent;

    agent = MILTER_PROTOCOL_AGENT(context);
    milter_protocol_agent_set_macro_context(agent, macro_context);
}

static void
reset_macro_context (MilterClientContext *context, MilterCommand macro_context)
{
    MilterProtocolAgent *agent;

    agent = MILTER_PROTOCOL_AGENT(context);
    milter_protocol_agent_clear_macros(agent, macro_context);
    milter_protocol_agent_set_macro_context(agent, MILTER_COMMAND_UNKNOWN);
}

static void
negotiate_response (MilterClientContext *context,
                    MilterOption *option, MilterStatus status)
{
    gchar *packet;
    gsize packet_size;
    MilterEncoder *encoder;

    if (status == MILTER_STATUS_DEFAULT)
        status = MILTER_STATUS_ALL_OPTIONS;

    switch (status) {
      case MILTER_STATUS_ALL_OPTIONS:
        milter_option_set_step(option, 0);
      case MILTER_STATUS_CONTINUE:
        encoder = milter_agent_get_encoder(MILTER_AGENT(context));

        milter_reply_encoder_encode_negotiate(MILTER_REPLY_ENCODER(encoder),
                                              &packet, &packet_size,
                                              option, NULL);
        write_packet(MILTER_CLIENT_CONTEXT(context), packet, packet_size);
        break;
      case MILTER_STATUS_REJECT:
        encoder = milter_agent_get_encoder(MILTER_AGENT(context));
        milter_reply_encoder_encode_reject(MILTER_REPLY_ENCODER(encoder),
                                           &packet, &packet_size);
        write_packet(MILTER_CLIENT_CONTEXT(context), packet, packet_size);
        break;
      default:
        break;
    }
}

static void
connect_response (MilterClientContext *context, MilterStatus status)
{
    if (status == MILTER_STATUS_DEFAULT)
        status = MILTER_STATUS_CONTINUE;

    reply(context, status);
    reset_macro_context(context, MILTER_COMMAND_CONNECT);
}

static void
helo_response (MilterClientContext *context, MilterStatus status)
{
    if (status == MILTER_STATUS_DEFAULT)
        status = MILTER_STATUS_CONTINUE;

    reply(context, status);
    reset_macro_context(context, MILTER_COMMAND_HELO);
}

static void
envelope_from_response (MilterClientContext *context, MilterStatus status)
{
    if (status == MILTER_STATUS_DEFAULT)
        status = MILTER_STATUS_CONTINUE;

    reply(MILTER_CLIENT_CONTEXT(context), status);
    reset_macro_context(context, MILTER_COMMAND_ENVELOPE_FROM);
}

static void
envelope_recipient_response (MilterClientContext *context, MilterStatus status)
{
    if (status == MILTER_STATUS_DEFAULT)
        status = MILTER_STATUS_CONTINUE;

    reply(MILTER_CLIENT_CONTEXT(context), status);
    reset_macro_context(context, MILTER_COMMAND_ENVELOPE_RECIPIENT);
}

static void
data_response (MilterClientContext *context, MilterStatus status)
{
    if (status == MILTER_STATUS_DEFAULT)
        status = MILTER_STATUS_CONTINUE;

    reply(MILTER_CLIENT_CONTEXT(context), status);
}

static void
header_response (MilterClientContext *context, MilterStatus status)
{
    if (status == MILTER_STATUS_DEFAULT)
        status = MILTER_STATUS_CONTINUE;

    reply(MILTER_CLIENT_CONTEXT(context), status);
    reset_macro_context(context, MILTER_COMMAND_HEADER);
}

static void
end_of_header_response (MilterClientContext *context, MilterStatus status)
{
    if (status == MILTER_STATUS_DEFAULT)
        status = MILTER_STATUS_CONTINUE;

    reply(MILTER_CLIENT_CONTEXT(context), status);
    reset_macro_context(context, MILTER_COMMAND_END_OF_HEADER);
}

static void
body_response (MilterClientContext *context, MilterStatus status)
{
    if (status == MILTER_STATUS_DEFAULT)
        status = MILTER_STATUS_CONTINUE;

    reply(MILTER_CLIENT_CONTEXT(context), status);
    reset_macro_context(context, MILTER_COMMAND_BODY);
}

static void
end_of_message_response (MilterClientContext *context, MilterStatus status)
{
    if (status == MILTER_STATUS_DEFAULT)
        status = MILTER_STATUS_CONTINUE;

    reply(MILTER_CLIENT_CONTEXT(context), status);
    reset_macro_context(context, MILTER_COMMAND_END_OF_MESSAGE);
}

static void
quit_response (MilterClientContext *context, MilterStatus status)
{
    if (status == MILTER_STATUS_DEFAULT)
        status = MILTER_STATUS_CONTINUE;
}

static void
abort_response (MilterClientContext *context, MilterStatus status)
{
    if (status == MILTER_STATUS_DEFAULT)
        status = MILTER_STATUS_CONTINUE;
}

static void
cb_decoder_negotiate (MilterDecoder *decoder, MilterOption *option,
                      gpointer user_data)
{
    MilterStatus status = MILTER_STATUS_NOT_CHANGE;
    MilterClientContext *context = MILTER_CLIENT_CONTEXT(user_data);

    disable_timeout(context);
    g_signal_emit(context, signals[NEGOTIATE], 0, option, &status);
    if (status == MILTER_STATUS_PROGRESS)
        return;
    g_signal_emit(context, signals[NEGOTIATE_RESPONSE], 0,  option, status);
}

static void
cb_decoder_define_macro (MilterDecoder *decoder, MilterCommand macro_context,
                         GHashTable *macros, gpointer user_data)
{
    MilterClientContext *context = user_data;

    disable_timeout(context);
    milter_protocol_agent_set_macros_hash_table(MILTER_PROTOCOL_AGENT(context),
                                                macro_context, macros);
    g_signal_emit(context, signals[DEFINE_MACRO], 0, macro_context, macros);
}

static void
cb_decoder_connect (MilterDecoder *decoder, const gchar *host_name,
                    struct sockaddr *address, socklen_t address_length,
                    gpointer user_data)
{
    MilterClientContext *context = user_data;
    MilterStatus status;

    disable_timeout(context);
    set_macro_context(context, MILTER_COMMAND_CONNECT);
    g_signal_emit(context, signals[CONNECT], 0,
                  host_name, address, address_length, &status);
    if (status == MILTER_STATUS_PROGRESS)
        return;
    g_signal_emit(context, signals[CONNECT_RESPONSE], 0, status);
}

static void
cb_decoder_helo (MilterDecoder *decoder, const gchar *fqdn, gpointer user_data)
{
    MilterClientContext *context = user_data;
    MilterStatus status;

    disable_timeout(context);
    set_macro_context(context, MILTER_COMMAND_HELO);
    g_signal_emit(context, signals[HELO], 0, fqdn, &status);
    if (status == MILTER_STATUS_PROGRESS)
        return;
    g_signal_emit(context, signals[HELO_RESPONSE], 0, status);
}

static void
cb_decoder_envelope_from (MilterDecoder *decoder,
                          const gchar *from, gpointer user_data)
{
    MilterClientContext *context = user_data;
    MilterStatus status;

    disable_timeout(context);
    set_macro_context(context, MILTER_COMMAND_ENVELOPE_FROM);
    g_signal_emit(context, signals[ENVELOPE_FROM], 0, from, &status);
    if (status == MILTER_STATUS_PROGRESS)
        return;
    g_signal_emit(context, signals[ENVELOPE_FROM_RESPONSE], 0, status);
}

static void
cb_decoder_envelope_recipient (MilterDecoder *decoder,
                               const gchar *to, gpointer user_data)
{
    MilterClientContext *context = user_data;
    MilterStatus status;

    disable_timeout(context);
    set_macro_context(context, MILTER_COMMAND_ENVELOPE_RECIPIENT);
    g_signal_emit(context, signals[ENVELOPE_RECIPIENT], 0, to, &status);
    if (status == MILTER_STATUS_PROGRESS)
        return;
    g_signal_emit(context, signals[ENVELOPE_RECIPIENT_RESPONSE], 0, status);
}

static void
cb_decoder_data (MilterDecoder *decoder, gpointer user_data)
{
    MilterClientContext *context = user_data;
    MilterStatus status;

    disable_timeout(context);
    g_signal_emit(context, signals[DATA], 0, &status);
    if (status == MILTER_STATUS_PROGRESS)
        return;
    g_signal_emit(context, signals[DATA_RESPONSE], 0, status);
}

static void
cb_decoder_header (MilterDecoder *decoder,
                   const gchar *name, const gchar *value,
                   gpointer user_data)
{
    MilterClientContext *context = user_data;
    MilterStatus status;

    disable_timeout(context);
    set_macro_context(context, MILTER_COMMAND_HEADER);
    g_signal_emit(context, signals[HEADER], 0, name, value, &status);
    if (status == MILTER_STATUS_PROGRESS)
        return;
    g_signal_emit(context, signals[HEADER_RESPONSE], 0, status);
}

static void
cb_decoder_end_of_header (MilterDecoder *decoder, gpointer user_data)
{
    MilterClientContext *context = user_data;
    MilterStatus status;

    disable_timeout(context);
    set_macro_context(context, MILTER_COMMAND_END_OF_HEADER);
    g_signal_emit(context, signals[END_OF_HEADER], 0, &status);
    if (status == MILTER_STATUS_PROGRESS)
        return;
    g_signal_emit(context, signals[END_OF_HEADER_RESPONSE], 0, status);
}

static void
cb_decoder_body (MilterDecoder *decoder, const gchar *chunk, gsize chunk_size,
                 gpointer user_data)
{
    MilterClientContext *context = user_data;
    MilterStatus status;

    disable_timeout(context);
    set_macro_context(context, MILTER_COMMAND_BODY);
    g_signal_emit(context, signals[BODY], 0, chunk, chunk_size, &status);
    if (status == MILTER_STATUS_PROGRESS)
        return;
    g_signal_emit(context, signals[BODY_RESPONSE], 0, status);
}

static void
cb_decoder_end_of_message (MilterDecoder *decoder, const gchar *chunk,
                           gsize chunk_size, gpointer user_data)
{
    MilterClientContext *context = user_data;
    MilterStatus status;

    disable_timeout(context);
    if (chunk && chunk_size > 0) {
        set_macro_context(context, MILTER_COMMAND_BODY);
        g_signal_emit(context, signals[BODY], 0, chunk, chunk_size, &status);
        if (status != MILTER_STATUS_PROGRESS)
            g_signal_emit(context, signals[BODY_RESPONSE], 0, status);
    }

    set_macro_context(context, MILTER_COMMAND_END_OF_MESSAGE);
    g_signal_emit(context, signals[END_OF_MESSAGE], 0, &status);
    if (status == MILTER_STATUS_PROGRESS)
        return;
    g_signal_emit(context, signals[END_OF_MESSAGE_RESPONSE], 0, status);
}

static void
cb_decoder_quit (MilterDecoder *decoder, gpointer user_data)
{
    MilterClientContext *context = MILTER_CLIENT_CONTEXT(user_data);
    MilterStatus status;

    disable_timeout(context);
    g_signal_emit(context, signals[QUIT], 0, &status);
    if (status == MILTER_STATUS_PROGRESS)
        return;
    g_signal_emit(context, signals[QUIT_RESPONSE], 0, status);
}

static void
cb_decoder_abort (MilterDecoder *decoder, gpointer user_data)
{
    MilterClientContext *context = MILTER_CLIENT_CONTEXT(user_data);
    MilterStatus status;

    disable_timeout(context);
    g_signal_emit(context, signals[ABORT], 0, &status);
    if (status == MILTER_STATUS_PROGRESS)
        return;
    g_signal_emit(context, signals[ABORT_RESPONSE], 0, status);
}

static MilterDecoder *
decoder_new (MilterAgent *agent)
{
    MilterDecoder *decoder;

    decoder = milter_command_decoder_new();

#define CONNECT(name)                                                   \
    g_signal_connect(decoder, #name, G_CALLBACK(cb_decoder_ ## name),   \
                     agent)

    CONNECT(negotiate);
    CONNECT(define_macro);
    CONNECT(connect);
    CONNECT(helo);
    CONNECT(envelope_from);
    CONNECT(envelope_recipient);
    CONNECT(data);
    CONNECT(header);
    CONNECT(end_of_header);
    CONNECT(body);
    CONNECT(end_of_message);
    CONNECT(quit);
    CONNECT(abort);

#undef CONNECT

    return decoder;
}

static MilterEncoder *
encoder_new (MilterAgent *agent)
{
    return milter_reply_encoder_new();
}

static gboolean
status_accumulator (GSignalInvocationHint *hint,
                    GValue *return_accumulator,
                    const GValue *context_return,
                    gpointer data)
{
    MilterStatus status;

    status = g_value_get_enum(context_return);
    if (status != MILTER_STATUS_NOT_CHANGE)
        g_value_set_enum(return_accumulator, status);
    status = g_value_get_enum(return_accumulator);

    return status == MILTER_STATUS_DEFAULT ||
        status == MILTER_STATUS_CONTINUE ||
        status == MILTER_STATUS_ALL_OPTIONS;
}

gboolean
milter_client_context_feed (MilterClientContext *context,
                            const gchar *chunk, gsize size,
                            GError **error)
{
    MilterDecoder *decoder;

    decoder = milter_agent_get_decoder(MILTER_AGENT(context));
    return milter_decoder_decode(decoder, chunk, size, error);
}

void
milter_client_context_set_mta_timeout (MilterClientContext *context,
                                       guint timeout)
{
    MILTER_CLIENT_CONTEXT_GET_PRIVATE(context)->mta_timeout = timeout;
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
