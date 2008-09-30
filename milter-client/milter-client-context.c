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
#  include "../config.h"
#endif /* HAVE_CONFIG_H */

#include <string.h>
#include <stdlib.h>

#include <milter-core.h>
#include "milter-client.h"
#include "milter-client-context.h"
#include "milter-client-enum-types.h"
#include "milter-client-marshalers.h"

#define MILTER_CLIENT_CONTEXT_GET_PRIVATE(obj)                          \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj),                                 \
                                 MILTER_CLIENT_TYPE_CONTEXT,            \
                                 MilterClientContextPrivate))

typedef struct _MilterClientContextPrivate	MilterClientContextPrivate;
struct _MilterClientContextPrivate
{
    MilterDecoder *decoder;
    MilterEncoder *encoder;
    GHashTable *macros;
    MilterCommand macro_context;
    gpointer private_data;
    GDestroyNotify private_data_destroy;
    guint reply_code;
    gchar *extended_reply_code;
    gchar *reply_message;
    MilterWriter *writer;
};

enum
{
    OPTION_NEGOTIATION,
    CONNECT,
    HELO,
    ENVELOPE_FROM,
    ENVELOPE_RECEIPT,
    DATA,
    UNKNOWN,
    HEADER,
    END_OF_HEADER,
    BODY,
    END_OF_MESSAGE,
    CLOSE,
    ABORT,
    LAST_SIGNAL
};

static gint signals[LAST_SIGNAL] = {0};

G_DEFINE_TYPE(MilterClientContext, milter_client_context, G_TYPE_OBJECT);

static void dispose        (GObject         *object);
static void set_property   (GObject         *object,
                            guint            prop_id,
                            const GValue    *value,
                            GParamSpec      *pspec);
static void get_property   (GObject         *object,
                            guint            prop_id,
                            GValue          *value,
                            GParamSpec      *pspec);

static MilterClientStatus cb_connect            (MilterClientContext *context,
                                                 const gchar         *host_name,
                                                 struct sockaddr     *address,
                                                 socklen_t            address_length);
static MilterClientStatus cb_option_negotiation (MilterClientContext *context,
                                                 MilterOption        *option);
static MilterClientStatus cb_helo               (MilterClientContext *context,
                                                 const gchar         *fqdn);
static MilterClientStatus cb_envelope_from      (MilterClientContext *context,
                                                 const gchar         *from);
static MilterClientStatus cb_envelope_receipt   (MilterClientContext *context,
                                                 const gchar         *receipt);
static MilterClientStatus cb_data               (MilterClientContext *context);
static MilterClientStatus cb_header             (MilterClientContext *context,
                                                 const gchar         *name,
                                                 const gchar         *value);
static MilterClientStatus cb_end_of_header      (MilterClientContext *context);
static MilterClientStatus cb_body               (MilterClientContext *context,
                                                 const guchar        *chunk,
                                                 gsize                size);
static MilterClientStatus cb_end_of_message     (MilterClientContext *context);
static MilterClientStatus cb_close              (MilterClientContext *context);
static MilterClientStatus cb_abort              (MilterClientContext *context);

static gboolean    status_accumulator  (GSignalInvocationHint *hint,
                                        GValue *return_accumulator,
                                        const GValue *context_return,
                                        gpointer data);

static void        setup_decoder       (MilterClientContext *context,
                                        MilterDecoder       *decoder);

static void
milter_client_context_class_init (MilterClientContextClass *klass)
{
    GObjectClass *gobject_class;

    gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->dispose      = dispose;
    gobject_class->set_property = set_property;
    gobject_class->get_property = get_property;

    klass->option_negotiation = cb_option_negotiation;
    klass->connect = cb_connect;
    klass->helo = cb_helo;
    klass->envelope_from = cb_envelope_from;
    klass->envelope_receipt = cb_envelope_receipt;
    klass->data = cb_data;
    klass->header = cb_header;
    klass->end_of_header = cb_end_of_header;
    klass->body = cb_body;
    klass->end_of_message = cb_end_of_message;
    klass->close = cb_close;
    klass->abort = cb_abort;

    signals[OPTION_NEGOTIATION] =
        g_signal_new("option-negotiation",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterClientContextClass,
                                     option_negotiation),
                     status_accumulator, NULL,
                     _milter_client_marshal_ENUM__OBJECT,
                     MILTER_TYPE_CLIENT_STATUS, 1, MILTER_TYPE_OPTION);

    signals[CONNECT] =
        g_signal_new("connect",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterClientContextClass, connect),
                     status_accumulator, NULL,
                     _milter_client_marshal_ENUM__STRING_POINTER_UINT,
                     MILTER_TYPE_CLIENT_STATUS, 3,
                     G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_UINT);

    signals[HELO] =
        g_signal_new("helo",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterClientContextClass, helo),
                     status_accumulator, NULL,
                     _milter_client_marshal_ENUM__STRING,
                     MILTER_TYPE_CLIENT_STATUS, 1, G_TYPE_STRING);

    signals[ENVELOPE_FROM] =
        g_signal_new("envelope-from",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterClientContextClass, envelope_from),
                     status_accumulator, NULL,
                     _milter_client_marshal_ENUM__STRING,
                     MILTER_TYPE_CLIENT_STATUS, 1, G_TYPE_STRING);

    signals[ENVELOPE_RECEIPT] =
        g_signal_new("envelope-receipt",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterClientContextClass, envelope_receipt),
                     status_accumulator, NULL,
                     _milter_client_marshal_ENUM__STRING,
                     MILTER_TYPE_CLIENT_STATUS, 1, G_TYPE_STRING);

    signals[DATA] =
        g_signal_new("data",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterClientContextClass, data),
                     status_accumulator, NULL,
                     _milter_client_marshal_ENUM__STRING_POINTER_UINT,
                     MILTER_TYPE_CLIENT_STATUS, 3,
                     G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_UINT);

    signals[UNKNOWN] =
        g_signal_new("unknown",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterClientContextClass, unknown),
                     status_accumulator, NULL,
                     _milter_client_marshal_ENUM__STRING_POINTER_UINT,
                     MILTER_TYPE_CLIENT_STATUS, 3,
                     G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_UINT);

    signals[HEADER] =
        g_signal_new("header",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterClientContextClass, header),
                     status_accumulator, NULL,
                     _milter_client_marshal_ENUM__STRING_STRING,
                     MILTER_TYPE_CLIENT_STATUS, 2, G_TYPE_STRING, G_TYPE_STRING);

    signals[END_OF_HEADER] =
        g_signal_new("end-of-header",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterClientContextClass, end_of_header),
                     status_accumulator, NULL,
                     _milter_client_marshal_ENUM__VOID,
                     MILTER_TYPE_CLIENT_STATUS, 0);

    signals[BODY] =
        g_signal_new("body",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterClientContextClass, body),
                     status_accumulator, NULL,
                     _milter_client_marshal_ENUM__STRING_UINT,
                     MILTER_TYPE_CLIENT_STATUS, 2, G_TYPE_STRING, G_TYPE_UINT);

    signals[END_OF_MESSAGE] =
        g_signal_new("end-of-message",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterClientContextClass, end_of_message),
                     status_accumulator, NULL,
                     _milter_client_marshal_ENUM__VOID,
                     MILTER_TYPE_CLIENT_STATUS, 0);

    signals[CLOSE] =
        g_signal_new("close",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterClientContextClass, close),
                     status_accumulator, NULL,
                     _milter_client_marshal_ENUM__VOID,
                     MILTER_TYPE_CLIENT_STATUS, 0);

    signals[ABORT] =
        g_signal_new("abort",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterClientContextClass, abort),
                     status_accumulator, NULL,
                     _milter_client_marshal_ENUM__VOID,
                     MILTER_TYPE_CLIENT_STATUS, 0);

    g_type_class_add_private(gobject_class, sizeof(MilterClientContextPrivate));
}

static void
milter_client_context_init (MilterClientContext *context)
{
    MilterClientContextPrivate *priv;

    priv = MILTER_CLIENT_CONTEXT_GET_PRIVATE(context);
    priv->decoder = milter_decoder_new();
    priv->encoder = milter_encoder_new();
    priv->macros = g_hash_table_new_full(g_direct_hash, g_direct_equal,
                                         NULL,
                                         (GDestroyNotify)g_hash_table_unref);
    priv->macro_context = MILTER_COMMAND_UNKNOWN;
    priv->private_data = NULL;
    priv->private_data_destroy = NULL;
    priv->reply_code = 0;
    priv->extended_reply_code = NULL;
    priv->reply_message = NULL;
    priv->writer = NULL;
    setup_decoder(context, priv->decoder);
}

static void
dispose (GObject *object)
{
    MilterClientContextPrivate *priv;

    priv = MILTER_CLIENT_CONTEXT_GET_PRIVATE(object);

    if (priv->decoder) {
        g_object_unref(priv->decoder);
        priv->decoder = NULL;
    }

    if (priv->encoder) {
        g_object_unref(priv->encoder);
        priv->encoder = NULL;
    }

    if (priv->macros) {
        g_hash_table_unref(priv->macros);
        priv->macros = NULL;
    }

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

    if (priv->writer) {
        g_object_unref(priv->writer);
        priv->writer = NULL;
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
    return g_object_new(MILTER_CLIENT_TYPE_CONTEXT,
                        NULL);
}

gboolean
milter_client_context_feed (MilterClientContext *context,
                            const gchar *chunk, gsize size,
                            GError **error)
{
    MilterClientContextPrivate *priv;

    priv = MILTER_CLIENT_CONTEXT_GET_PRIVATE(context);
    return milter_decoder_decode(priv->decoder, chunk, size, error);
}

static void
clear_macros (MilterClientContext *context, MilterCommand macro_context)
{
    MilterClientContextPrivate *priv;

    priv = MILTER_CLIENT_CONTEXT_GET_PRIVATE(context);
    g_hash_table_remove(priv->macros, GINT_TO_POINTER(macro_context));
}

static void
update_macro (gpointer key, gpointer value, gpointer user_data)
{
    GHashTable *macros = user_data;

    g_hash_table_replace(macros, g_strdup(key), g_strdup(value));
}

static void
set_macros (MilterClientContext *context, MilterCommand macro_context,
            GHashTable *macros)
{
    MilterClientContextPrivate *priv;
    GHashTable *copied_macros;

    priv = MILTER_CLIENT_CONTEXT_GET_PRIVATE(context);
    copied_macros = g_hash_table_new_full(g_str_hash, g_str_equal,
                                          g_free, g_free);
    g_hash_table_insert(priv->macros,
                        GINT_TO_POINTER(macro_context),
                        copied_macros);
    g_hash_table_foreach(macros, update_macro, copied_macros);
}

const gchar *
milter_client_context_get_macro (MilterClientContext *context, const gchar *name)
{
    GHashTable *macros;
    const gchar *value;

    macros = milter_client_context_get_macros(context);
    if (!macros)
        return NULL;

    value = g_hash_table_lookup(macros, name);
    if (!value && g_str_has_prefix(name, "{") && g_str_has_suffix(name, "}")) {
        gchar *unbracket_name;

        unbracket_name = g_strndup(name + 1, strlen(name) - 2);
        value = g_hash_table_lookup(macros, unbracket_name);
        g_free(unbracket_name);
    }
    return value;
}

GHashTable *
milter_client_context_get_macros (MilterClientContext *context)
{
    MilterClientContextPrivate *priv;

    priv = MILTER_CLIENT_CONTEXT_GET_PRIVATE(context);
    return g_hash_table_lookup(priv->macros,
                               GINT_TO_POINTER(priv->macro_context));
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
    GString *reply;
    gchar **messages;

    priv = MILTER_CLIENT_CONTEXT_GET_PRIVATE(context);

    if (priv->reply_code == 0)
        return NULL;

    if (priv->reply_message)
        messages = g_strsplit(priv->reply_message, "\n", -1);
    else
        messages = g_strsplit("\n", "\n", -1);

    reply = g_string_new("");
    for (; *messages; messages++) {
        gchar *message = *messages;
        gsize message_size;

        g_string_append_printf(reply, "%3u", priv->reply_code);
        if (priv->extended_reply_code)
            g_string_append_printf(reply, " %s", priv->extended_reply_code);

        message_size = strlen(message);
        if (message_size > 0) {
            if (message[message_size - 1] == '\r')
                message[message_size - 1] = '\0';
            g_string_append_printf(reply, " %s", message);
        }
        if (*(messages + 1))
            g_string_append(reply, "\r\n");
    }

    return g_string_free(reply, FALSE);
}

void
milter_client_context_set_writer (MilterClientContext *context,
                                  MilterWriter *writer)
{
    MilterClientContextPrivate *priv;

    priv = MILTER_CLIENT_CONTEXT_GET_PRIVATE(context);

    if (priv->writer)
        g_object_unref(priv->writer);

    priv->writer = writer;
    if (priv->writer)
        g_object_ref(priv->writer);
}

static gboolean
status_accumulator (GSignalInvocationHint *hint,
                    GValue *return_accumulator,
                    const GValue *context_return,
                    gpointer data)
{
    MilterClientStatus status;

    status = g_value_get_enum(context_return);
    if (status != MILTER_CLIENT_STATUS_NOT_CHANGE)
        g_value_set_enum(return_accumulator, status);
    status = g_value_get_enum(return_accumulator);

    return status == MILTER_CLIENT_STATUS_DEFAULT ||
        status == MILTER_CLIENT_STATUS_CONTINUE ||
        status == MILTER_CLIENT_STATUS_ALL_OPTIONS;
}

/* FIXME: should pass GError **error */
static gboolean
write_packet (MilterClientContext *context,
              const gchar *packet, gsize packet_size)
{
    MilterClientContextPrivate *priv;
    gboolean success;
    GError *error = NULL;

    priv = MILTER_CLIENT_CONTEXT_GET_PRIVATE(context);

    if (!priv->writer)
        return TRUE;

    success = milter_writer_write(priv->writer, packet, packet_size,
                                  NULL, &error);
    if (!success) {
        g_print("error: %s\n", error->message);
        g_error_free(error);
    }

    return success;
}

static gboolean
reply (MilterClientContext *context, MilterClientStatus status)
{
    MilterClientContextPrivate *priv;
    gchar *packet = NULL;
    gsize packet_size;

    priv = MILTER_CLIENT_CONTEXT_GET_PRIVATE(context);
    switch (status) {
      case MILTER_CLIENT_STATUS_CONTINUE:
        milter_encoder_encode_reply_continue(priv->encoder,
                                             &packet, &packet_size);
        break;
      case MILTER_CLIENT_STATUS_REJECT:
        {
            gchar *code;

            code = milter_client_context_format_reply(context);
            if (code) {
                milter_encoder_encode_reply_reply_code(priv->encoder,
                                                       &packet, &packet_size,
                                                       code);
                g_free(code);
            } else {
                milter_encoder_encode_reply_reject(priv->encoder,
                                                   &packet, &packet_size);
            }
        }
        break;
      case MILTER_CLIENT_STATUS_TEMPORARY_FAILURE:
        milter_encoder_encode_reply_temporary_failure(priv->encoder,
                                                      &packet, &packet_size);
        break;
      case MILTER_CLIENT_STATUS_ACCEPT:
        milter_encoder_encode_reply_accept(priv->encoder, &packet, &packet_size);
        break;
      case MILTER_CLIENT_STATUS_DISCARD:
        milter_encoder_encode_reply_discard(priv->encoder,
                                            &packet, &packet_size);
        break;
      default:
        break;
    }

    if (packet) {
        write_packet(context, packet, packet_size);
        g_free(packet);
    }

    return TRUE;
}

static void
cb_decoder_option_negotiation (MilterDecoder *decoder, MilterOption *option,
                               gpointer user_data)
{
    MilterClientContext *context = user_data;
    MilterClientStatus status;
    MilterClientContextPrivate *priv;
    gchar *packet;
    gsize packet_size;

    g_signal_emit(context, signals[OPTION_NEGOTIATION], 0, option, &status);
    if (status == MILTER_CLIENT_STATUS_DEFAULT)
        status = MILTER_CLIENT_STATUS_ALL_OPTIONS;

    switch (status) {
      case MILTER_CLIENT_STATUS_ALL_OPTIONS:
        milter_option_set_step(option, 0);
      case MILTER_CLIENT_STATUS_CONTINUE:
        priv = MILTER_CLIENT_CONTEXT_GET_PRIVATE(context);
        milter_encoder_encode_option_negotiation(priv->encoder,
                                                 &packet, &packet_size,
                                                 option);
        write_packet(context, packet, packet_size);
        g_free(packet);
        break;
      case MILTER_CLIENT_STATUS_REJECT:
        break;
      default:
        break;
    }
}

static void
cb_decoder_define_macro (MilterDecoder *decoder, MilterCommand macro_context,
                         GHashTable *macros, gpointer user_data)
{
    MilterClientContext *context = user_data;

    set_macros(context, macro_context, macros);
}

static void
cb_decoder_connect (MilterDecoder *decoder, const gchar *host_name,
                    struct sockaddr *address, socklen_t address_length,
                    gpointer user_data)
{
    MilterClientContext *context = user_data;
    MilterClientStatus status;
    MilterClientContextPrivate *priv;

    priv = MILTER_CLIENT_CONTEXT_GET_PRIVATE(context);
    priv->macro_context = MILTER_COMMAND_CONNECT;
    g_signal_emit(context, signals[CONNECT], 0,
                  host_name, address, address_length, &status);
    if (status == MILTER_CLIENT_STATUS_DEFAULT)
        status = MILTER_CLIENT_STATUS_CONTINUE;
    clear_macros(context, priv->macro_context);
    priv->macro_context = MILTER_COMMAND_UNKNOWN;

    reply(context, status);
}

static void
cb_decoder_helo (MilterDecoder *decoder, const gchar *fqdn, gpointer user_data)
{
    MilterClientContext *context = user_data;
    MilterClientStatus status;
    MilterClientContextPrivate *priv;

    priv = MILTER_CLIENT_CONTEXT_GET_PRIVATE(context);
    priv->macro_context = MILTER_COMMAND_HELO;
    g_signal_emit(context, signals[HELO], 0, fqdn, &status);
    if (status == MILTER_CLIENT_STATUS_DEFAULT)
        status = MILTER_CLIENT_STATUS_CONTINUE;
    clear_macros(context, priv->macro_context);
    priv->macro_context = MILTER_COMMAND_UNKNOWN;

    reply(context, status);
}

static void
cb_decoder_mail (MilterDecoder *decoder, const gchar *from, gpointer user_data)
{
    MilterClientContext *context = user_data;
    MilterClientContextPrivate *priv;
    MilterClientStatus status;

    priv = MILTER_CLIENT_CONTEXT_GET_PRIVATE(context);
    priv->macro_context = MILTER_COMMAND_MAIL;
    g_signal_emit(context, signals[ENVELOPE_FROM], 0, from, &status);
    if (status == MILTER_CLIENT_STATUS_DEFAULT)
        status = MILTER_CLIENT_STATUS_CONTINUE;
    clear_macros(context, priv->macro_context);
    priv->macro_context = MILTER_COMMAND_UNKNOWN;

    reply(context, status);
}

static void
cb_decoder_rcpt (MilterDecoder *decoder, const gchar *to, gpointer user_data)
{
    MilterClientContext *context = user_data;
    MilterClientContextPrivate *priv;
    MilterClientStatus status;

    priv = MILTER_CLIENT_CONTEXT_GET_PRIVATE(context);
    priv->macro_context = MILTER_COMMAND_RCPT;
    g_signal_emit(context, signals[ENVELOPE_RECEIPT], 0, to, &status);
    if (status == MILTER_CLIENT_STATUS_DEFAULT)
        status = MILTER_CLIENT_STATUS_CONTINUE;
    clear_macros(context, priv->macro_context);
    priv->macro_context = MILTER_COMMAND_UNKNOWN;

    reply(context, status);
}

static void
cb_decoder_header (MilterDecoder *decoder,
                   const gchar *name, const gchar *value,
                   gpointer user_data)
{
    MilterClientContext *context = user_data;
    MilterClientContextPrivate *priv;
    MilterClientStatus status;

    priv = MILTER_CLIENT_CONTEXT_GET_PRIVATE(context);
    priv->macro_context = MILTER_COMMAND_HEADER;
    g_signal_emit(context, signals[HEADER], 0, name, value, &status);
    if (status == MILTER_CLIENT_STATUS_DEFAULT)
        status = MILTER_CLIENT_STATUS_CONTINUE;
    clear_macros(context, priv->macro_context);
    priv->macro_context = MILTER_COMMAND_UNKNOWN;

    reply(context, status);
}

static void
cb_decoder_end_of_header (MilterDecoder *decoder, gpointer user_data)
{
    MilterClientContext *context = user_data;
    MilterClientContextPrivate *priv;
    MilterClientStatus status;

    priv = MILTER_CLIENT_CONTEXT_GET_PRIVATE(context);
    priv->macro_context = MILTER_COMMAND_END_OF_HEADER;
    g_signal_emit(context, signals[END_OF_HEADER], 0, &status);
    if (status == MILTER_CLIENT_STATUS_DEFAULT)
        status = MILTER_CLIENT_STATUS_CONTINUE;
    clear_macros(context, priv->macro_context);
    priv->macro_context = MILTER_COMMAND_UNKNOWN;

    reply(context, status);
}

static void
cb_decoder_body (MilterDecoder *decoder, const gchar *chunk, gsize chunk_size,
                 gpointer user_data)
{
    MilterClientContext *context = user_data;
    MilterClientContextPrivate *priv;
    MilterClientStatus status;

    priv = MILTER_CLIENT_CONTEXT_GET_PRIVATE(context);
    priv->macro_context = MILTER_COMMAND_BODY;
    g_signal_emit(context, signals[BODY], 0, chunk, chunk_size, &status);
    if (status == MILTER_CLIENT_STATUS_DEFAULT)
        status = MILTER_CLIENT_STATUS_CONTINUE;
    clear_macros(context, priv->macro_context);
    priv->macro_context = MILTER_COMMAND_UNKNOWN;

    reply(context, status);
}

static void
cb_decoder_end_of_message (MilterDecoder *decoder, gpointer user_data)
{
    MilterClientContext *context = user_data;
    MilterClientContextPrivate *priv;
    MilterClientStatus status;

    priv = MILTER_CLIENT_CONTEXT_GET_PRIVATE(context);
    priv->macro_context = MILTER_COMMAND_END_OF_MESSAGE;
    g_signal_emit(context, signals[END_OF_MESSAGE], 0, &status);
    if (status == MILTER_CLIENT_STATUS_DEFAULT)
        status = MILTER_CLIENT_STATUS_CONTINUE;
    clear_macros(context, priv->macro_context);
    priv->macro_context = MILTER_COMMAND_UNKNOWN;

    reply(context, status);
}

static void
cb_decoder_quit (MilterDecoder *decoder, gpointer user_data)
{
    MilterClientContext *context = user_data;
    MilterClientStatus status;

    g_signal_emit(context, signals[CLOSE], 0, &status);
    if (status == MILTER_CLIENT_STATUS_DEFAULT)
        status = MILTER_CLIENT_STATUS_CONTINUE;

    reply(context, status);
}

static void
cb_decoder_abort (MilterDecoder *decoder, gpointer user_data)
{
    MilterClientContext *context = user_data;
    MilterClientStatus status;

    g_signal_emit(context, signals[ABORT], 0, &status);
    if (status == MILTER_CLIENT_STATUS_DEFAULT)
        status = MILTER_CLIENT_STATUS_CONTINUE;

    reply(context, status);
}


static void
setup_decoder (MilterClientContext *context, MilterDecoder *decoder)
{
#define CONNECT(name)                                                   \
    g_signal_connect(decoder, #name, G_CALLBACK(cb_decoder_ ## name),   \
                     context)

    CONNECT(option_negotiation);
    CONNECT(define_macro);
    CONNECT(connect);
    CONNECT(helo);
    CONNECT(mail);
    CONNECT(rcpt);
    CONNECT(header);
    CONNECT(end_of_header);
    CONNECT(body);
    CONNECT(end_of_message);
    CONNECT(quit);
    CONNECT(abort);

#undef CONNECT
}


static MilterClientStatus
cb_option_negotiation (MilterClientContext *context,
                       MilterOption        *option)
{
    return MILTER_CLIENT_STATUS_NOT_CHANGE;
}

static MilterClientStatus
cb_connect (MilterClientContext *context,
            const gchar         *host_name,
            struct sockaddr     *address,
            socklen_t            address_length)
{
    return MILTER_CLIENT_STATUS_NOT_CHANGE;
}

static MilterClientStatus
cb_helo (MilterClientContext *context,
         const gchar         *fqdn)
{
    return MILTER_CLIENT_STATUS_NOT_CHANGE;
}

static MilterClientStatus
cb_envelope_from (MilterClientContext *context,
                  const gchar         *from)
{
    return MILTER_CLIENT_STATUS_NOT_CHANGE;
}

static MilterClientStatus
cb_envelope_receipt (MilterClientContext *context,
                     const gchar         *receipt)
{
    return MILTER_CLIENT_STATUS_NOT_CHANGE;
}

static MilterClientStatus
cb_data (MilterClientContext *context)
{
    return MILTER_CLIENT_STATUS_NOT_CHANGE;
}

static MilterClientStatus
cb_header (MilterClientContext *context,
           const gchar         *name,
           const gchar         *value)
{
    return MILTER_CLIENT_STATUS_NOT_CHANGE;
}

static MilterClientStatus
cb_end_of_header (MilterClientContext *context)
{
    return MILTER_CLIENT_STATUS_NOT_CHANGE;
}

static MilterClientStatus
cb_body (MilterClientContext *context,
         const guchar        *chunk,
         gsize                size)
{
    return MILTER_CLIENT_STATUS_NOT_CHANGE;
}

static MilterClientStatus
cb_end_of_message (MilterClientContext *context)
{
    return MILTER_CLIENT_STATUS_NOT_CHANGE;
}

static MilterClientStatus
cb_close (MilterClientContext *context)
{
    return MILTER_CLIENT_STATUS_NOT_CHANGE;
}

static MilterClientStatus
cb_abort (MilterClientContext *context)
{
    return MILTER_CLIENT_STATUS_NOT_CHANGE;
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
