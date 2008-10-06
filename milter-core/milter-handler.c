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
#include "milter-handler.h"
#include "milter-marshalers.h"

#define MILTER_HANDLER_GET_PRIVATE(obj)                          \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj),                          \
                                 MILTER_TYPE_HANDLER,            \
                                 MilterHandlerPrivate))

typedef struct _MilterHandlerPrivate	MilterHandlerPrivate;
struct _MilterHandlerPrivate
{
    MilterDecoder *decoder;
    MilterEncoder *encoder;
    MilterWriter *writer;
    GHashTable *macros;
    MilterCommand macro_context;
};

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
    ENVELOPE_RECEIPT,
    ENVELOPE_RECEIPT_RESPONSE,
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
    CLOSE,
    CLOSE_RESPONSE,
    ABORT,
    ABORT_RESPONSE,
    LAST_SIGNAL
};

static gint signals[LAST_SIGNAL] = {0};

G_DEFINE_ABSTRACT_TYPE(MilterHandler, milter_handler, G_TYPE_OBJECT);

static void dispose        (GObject         *object);
static void set_property   (GObject         *object,
                            guint            prop_id,
                            const GValue    *value,
                            GParamSpec      *pspec);
static void get_property   (GObject         *object,
                            guint            prop_id,
                            GValue          *value,
                            GParamSpec      *pspec);

static gboolean    status_accumulator  (GSignalInvocationHint *hint,
                                        GValue *return_accumulator,
                                        const GValue *context_return,
                                        gpointer data);

static void        setup_decoder       (MilterHandler *handler,
                                        MilterDecoder *decoder);

static MilterStatus default_negotiate  (MilterHandler *handler,
                                        MilterOption  *option);
static MilterStatus default_connect    (MilterHandler *handler,
                                        const gchar   *host_name,
                                        struct sockaddr *address,
                                        socklen_t      address_length);
static MilterStatus default_helo       (MilterHandler *handler,
                                        const gchar   *fqdn);
static MilterStatus default_envelope_from
                                       (MilterHandler *handler,
                                        const gchar   *from);
static MilterStatus default_envelope_receipt
                                       (MilterHandler *handler,
                                        const gchar   *receipt);
static MilterStatus default_data       (MilterHandler *handler);
static MilterStatus default_header     (MilterHandler *handler,
                                        const gchar   *name,
                                        const gchar   *value);
static MilterStatus default_end_of_header
                                       (MilterHandler *handler);
static MilterStatus default_body       (MilterHandler *handler,
                                        const guchar  *chunk,
                                        gsize          size);
static MilterStatus default_end_of_message
                                       (MilterHandler *handler);
static MilterStatus default_close      (MilterHandler *handler);
static MilterStatus default_abort      (MilterHandler *handler);

static void
milter_handler_class_init (MilterHandlerClass *klass)
{
    GObjectClass *gobject_class;

    gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->dispose      = dispose;
    gobject_class->set_property = set_property;
    gobject_class->get_property = get_property;

    klass->negotiate = default_negotiate;
    klass->connect = default_connect;
    klass->helo = default_helo;
    klass->envelope_from = default_envelope_from;
    klass->envelope_receipt = default_envelope_receipt;
    klass->data = default_data;
    klass->header = default_header;
    klass->end_of_header = default_end_of_header;
    klass->body = default_body;
    klass->end_of_message = default_end_of_message;
    klass->close = default_close;
    klass->abort = default_abort;

    signals[NEGOTIATE] =
        g_signal_new("negotiate",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterHandlerClass, negotiate),
                     status_accumulator, NULL,
                     _milter_marshal_ENUM__OBJECT,
                     MILTER_TYPE_STATUS, 1, MILTER_TYPE_OPTION);

    signals[NEGOTIATE_RESPONSE] =
        g_signal_new("negotiate-response",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterHandlerClass, negotiate_response),
                     status_accumulator, NULL,
                     _milter_marshal_ENUM__OBJECT_ENUM,
                     MILTER_TYPE_STATUS, 2, MILTER_TYPE_OPTION, MILTER_TYPE_STATUS);

    signals[CONNECT] =
        g_signal_new("connect",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterHandlerClass, connect),
                     status_accumulator, NULL,
                     _milter_marshal_ENUM__STRING_POINTER_UINT,
                     MILTER_TYPE_STATUS, 3,
                     G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_UINT);

    signals[CONNECT_RESPONSE] =
        g_signal_new("connect-response",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterHandlerClass, connect_response),
                     status_accumulator, NULL,
                     _milter_marshal_ENUM__STRING_POINTER_UINT_ENUM,
                     MILTER_TYPE_STATUS, 4,
                     G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_UINT, MILTER_TYPE_STATUS);

    signals[HELO] =
        g_signal_new("helo",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterHandlerClass, helo),
                     status_accumulator, NULL,
                     _milter_marshal_ENUM__STRING,
                     MILTER_TYPE_STATUS, 1, G_TYPE_STRING);

    signals[HELO_RESPONSE] =
        g_signal_new("helo_response",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterHandlerClass, helo_response),
                     status_accumulator, NULL,
                     _milter_marshal_ENUM__STRING_ENUM,
                     MILTER_TYPE_STATUS, 2, G_TYPE_STRING, MILTER_TYPE_STATUS);

    signals[ENVELOPE_FROM] =
        g_signal_new("envelope-from",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterHandlerClass, envelope_from),
                     status_accumulator, NULL,
                     _milter_marshal_ENUM__STRING,
                     MILTER_TYPE_STATUS, 1, G_TYPE_STRING);

    signals[ENVELOPE_FROM_RESPONSE] =
        g_signal_new("envelope-from-response",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterHandlerClass, envelope_from_response),
                     status_accumulator, NULL,
                     _milter_marshal_ENUM__STRING_ENUM,
                     MILTER_TYPE_STATUS, 2, G_TYPE_STRING, MILTER_TYPE_STATUS);

    signals[ENVELOPE_RECEIPT] =
        g_signal_new("envelope-receipt",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterHandlerClass, envelope_receipt),
                     status_accumulator, NULL,
                     _milter_marshal_ENUM__STRING,
                     MILTER_TYPE_STATUS, 1, G_TYPE_STRING);

    signals[ENVELOPE_RECEIPT_RESPONSE] =
        g_signal_new("envelope-receipt-response",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterHandlerClass, envelope_receipt_response),
                     status_accumulator, NULL,
                     _milter_marshal_ENUM__STRING_ENUM,
                     MILTER_TYPE_STATUS, 2, G_TYPE_STRING, MILTER_TYPE_STATUS);

    signals[DATA] =
        g_signal_new("data",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterHandlerClass, data),
                     status_accumulator, NULL,
                     _milter_marshal_ENUM__STRING_POINTER_UINT,
                     MILTER_TYPE_STATUS, 3,
                     G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_UINT);

    signals[DATA_RESPONSE] =
        g_signal_new("data-response",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterHandlerClass, data_response),
                     status_accumulator, NULL,
                     _milter_marshal_ENUM__STRING_POINTER_UINT_ENUM,
                     MILTER_TYPE_STATUS, 4,
                     G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_UINT, MILTER_TYPE_STATUS);

    signals[UNKNOWN] =
        g_signal_new("unknown",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterHandlerClass, unknown),
                     status_accumulator, NULL,
                     _milter_marshal_ENUM__STRING_POINTER_UINT,
                     MILTER_TYPE_STATUS, 3,
                     G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_UINT);

    signals[UNKNOWN_RESPONSE] =
        g_signal_new("unknown-response",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterHandlerClass, unknown_response),
                     status_accumulator, NULL,
                     _milter_marshal_ENUM__STRING_POINTER_UINT_ENUM,
                     MILTER_TYPE_STATUS, 4,
                     G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_UINT, MILTER_TYPE_STATUS);

    signals[HEADER] =
        g_signal_new("header",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterHandlerClass, header),
                     status_accumulator, NULL,
                     _milter_marshal_ENUM__STRING_STRING,
                     MILTER_TYPE_STATUS, 2, G_TYPE_STRING, G_TYPE_STRING);

    signals[HEADER_RESPONSE] =
        g_signal_new("header-response",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterHandlerClass, header_response),
                     status_accumulator, NULL,
                     _milter_marshal_ENUM__STRING_STRING_ENUM,
                     MILTER_TYPE_STATUS, 3, G_TYPE_STRING, G_TYPE_STRING, MILTER_TYPE_STATUS);

    signals[END_OF_HEADER] =
        g_signal_new("end-of-header",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterHandlerClass, end_of_header),
                     status_accumulator, NULL,
                     _milter_marshal_ENUM__VOID,
                     MILTER_TYPE_STATUS, 0);

    signals[END_OF_HEADER_RESPONSE] =
        g_signal_new("end-of-header-response",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterHandlerClass, end_of_header_response),
                     status_accumulator, NULL,
                     _milter_marshal_ENUM__ENUM,
                     MILTER_TYPE_STATUS, 1, MILTER_TYPE_STATUS);

    signals[BODY] =
        g_signal_new("body",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterHandlerClass, body),
                     status_accumulator, NULL,
                     _milter_marshal_ENUM__STRING_UINT,
                     MILTER_TYPE_STATUS, 2, G_TYPE_STRING, G_TYPE_UINT);

    signals[BODY_RESPONSE] =
        g_signal_new("body_response",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterHandlerClass, body_response),
                     status_accumulator, NULL,
                     _milter_marshal_ENUM__STRING_UINT_ENUM,
                     MILTER_TYPE_STATUS,
                     3, G_TYPE_STRING, G_TYPE_UINT, MILTER_TYPE_STATUS);

    signals[END_OF_MESSAGE] =
        g_signal_new("end-of-message",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterHandlerClass, end_of_message),
                     status_accumulator, NULL,
                     _milter_marshal_ENUM__VOID,
                     MILTER_TYPE_STATUS, 0);

    signals[END_OF_MESSAGE_RESPONSE] =
        g_signal_new("end-of-message-response",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterHandlerClass, end_of_message_response),
                     status_accumulator, NULL,
                     _milter_marshal_ENUM__ENUM,
                     MILTER_TYPE_STATUS, 1, MILTER_TYPE_STATUS);

    signals[CLOSE] =
        g_signal_new("close",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterHandlerClass, close),
                     status_accumulator, NULL,
                     _milter_marshal_ENUM__VOID,
                     MILTER_TYPE_STATUS, 0);

    signals[CLOSE_RESPONSE] =
        g_signal_new("close-response",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterHandlerClass, close_response),
                     status_accumulator, NULL,
                     _milter_marshal_ENUM__ENUM,
                     MILTER_TYPE_STATUS, 1, MILTER_TYPE_STATUS);

    signals[ABORT] =
        g_signal_new("abort",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterHandlerClass, abort),
                     status_accumulator, NULL,
                     _milter_marshal_ENUM__VOID,
                     MILTER_TYPE_STATUS, 0);

    signals[ABORT_RESPONSE] =
        g_signal_new("abort-response",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterHandlerClass, abort_response),
                     status_accumulator, NULL,
                     _milter_marshal_ENUM__ENUM,
                     MILTER_TYPE_STATUS, 1, MILTER_TYPE_STATUS);

    g_type_class_add_private(gobject_class, sizeof(MilterHandlerPrivate));
}

static void
milter_handler_init (MilterHandler *handler)
{
    MilterHandlerPrivate *priv;

    priv = MILTER_HANDLER_GET_PRIVATE(handler);
    priv->decoder = milter_decoder_new();
    priv->encoder = milter_encoder_new();
    priv->writer = NULL;
    priv->macros = g_hash_table_new_full(g_direct_hash, g_direct_equal,
                                         NULL,
                                         (GDestroyNotify)g_hash_table_unref);
    priv->macro_context = MILTER_COMMAND_UNKNOWN;

    setup_decoder(handler, priv->decoder);
}

static void
dispose (GObject *object)
{
    MilterHandlerPrivate *priv;

    priv = MILTER_HANDLER_GET_PRIVATE(object);

    if (priv->decoder) {
        g_object_unref(priv->decoder);
        priv->decoder = NULL;
    }

    if (priv->encoder) {
        g_object_unref(priv->encoder);
        priv->encoder = NULL;
    }

    if (priv->writer) {
        g_object_unref(priv->writer);
        priv->writer = NULL;
    }

    if (priv->macros) {
        g_hash_table_unref(priv->macros);
        priv->macros = NULL;
    }

    G_OBJECT_CLASS(milter_handler_parent_class)->dispose(object);
}

static void
set_property (GObject      *object,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
    MilterHandlerPrivate *priv;

    priv = MILTER_HANDLER_GET_PRIVATE(object);
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
    MilterHandlerPrivate *priv;

    priv = MILTER_HANDLER_GET_PRIVATE(object);
    switch (prop_id) {
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
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

static MilterStatus
default_negotiate (MilterHandler *handler, MilterOption *option)
{
    return MILTER_STATUS_NOT_CHANGE;
}

static MilterStatus
default_connect (MilterHandler *handler, const gchar *host_name,
                 struct sockaddr *address, socklen_t address_length)
{
    return MILTER_STATUS_NOT_CHANGE;
}

static MilterStatus
default_helo (MilterHandler *handler, const gchar *fqdn)
{
    return MILTER_STATUS_NOT_CHANGE;
}

static MilterStatus
default_envelope_from (MilterHandler *handler, const gchar *from)
{
    return MILTER_STATUS_NOT_CHANGE;
}

static MilterStatus
default_envelope_receipt (MilterHandler *handler, const gchar *receipt)
{
    return MILTER_STATUS_NOT_CHANGE;
}

static MilterStatus
default_data (MilterHandler *handler)
{
    return MILTER_STATUS_NOT_CHANGE;
}

static MilterStatus
default_header (MilterHandler *handler, const gchar *name, const gchar *value)
{
    return MILTER_STATUS_NOT_CHANGE;
}

static MilterStatus
default_end_of_header (MilterHandler *handler)
{
    return MILTER_STATUS_NOT_CHANGE;
}

static MilterStatus
default_body (MilterHandler *handler, const guchar *chunk, gsize size)
{
    return MILTER_STATUS_NOT_CHANGE;
}

static MilterStatus
default_end_of_message (MilterHandler *handler)
{
    return MILTER_STATUS_NOT_CHANGE;
}

static MilterStatus
default_close (MilterHandler *handler)
{
    return MILTER_STATUS_NOT_CHANGE;
}

static MilterStatus
default_abort (MilterHandler *handler)
{
    return MILTER_STATUS_NOT_CHANGE;
}

GQuark
milter_handler_error_quark (void)
{
    return g_quark_from_static_string("milter-handler-error-quark");
}

gboolean
milter_handler_feed (MilterHandler *handler,
                     const gchar *chunk, gsize size,
                     GError **error)
{
    MilterHandlerPrivate *priv;

    priv = MILTER_HANDLER_GET_PRIVATE(handler);
    return milter_decoder_decode(priv->decoder, chunk, size, error);
}

gboolean
milter_handler_write_packet (MilterHandler *handler,
                             const gchar *packet, gsize packet_size,
                             GError **error)
{
    MilterHandlerPrivate *priv;
    gboolean success;

    priv = MILTER_HANDLER_GET_PRIVATE(handler);

    if (!priv->writer)
        return TRUE;

    success = milter_writer_write(priv->writer, packet, packet_size,
                                  NULL, error);

    return success;
}

void
milter_handler_set_writer (MilterHandler *handler,
                           MilterWriter *writer)
{
    MilterHandlerPrivate *priv;

    priv = MILTER_HANDLER_GET_PRIVATE(handler);

    if (priv->writer)
        g_object_unref(priv->writer);

    priv->writer = writer;
    if (priv->writer)
        g_object_ref(priv->writer);
}

MilterEncoder *
milter_handler_get_encoder (MilterHandler *handler)
{
    return MILTER_HANDLER_GET_PRIVATE(handler)->encoder;
}

MilterDecoder *
milter_handler_get_decoder (MilterHandler *handler)
{
    return MILTER_HANDLER_GET_PRIVATE(handler)->decoder;
}

const gchar *
milter_handler_get_macro (MilterHandler *handler, const gchar *name)
{
    GHashTable *macros;
    const gchar *value;

    macros = milter_handler_get_macros(handler);
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
milter_handler_get_macros (MilterHandler *handler)
{
    MilterHandlerPrivate *priv;

    priv = MILTER_HANDLER_GET_PRIVATE(handler);
    return g_hash_table_lookup(priv->macros,
                               GINT_TO_POINTER(priv->macro_context));
}

void
milter_handler_clear_macros (MilterHandler *handler, MilterCommand macro_context)
{
    MilterHandlerPrivate *priv;

    priv = MILTER_HANDLER_GET_PRIVATE(handler);
    g_hash_table_remove(priv->macros, GINT_TO_POINTER(macro_context));
}

static void
update_macro (gpointer key, gpointer value, gpointer user_data)
{
    GHashTable *macros = user_data;

    g_hash_table_replace(macros, g_strdup(key), g_strdup(value));
}

static void
set_macros (MilterHandler *handler, MilterCommand macro_context,
            GHashTable *macros)
{
    MilterHandlerPrivate *priv;
    GHashTable *copied_macros;

    priv = MILTER_HANDLER_GET_PRIVATE(handler);
    copied_macros = g_hash_table_new_full(g_str_hash, g_str_equal,
                                          g_free, g_free);
    g_hash_table_insert(priv->macros,
                        GINT_TO_POINTER(macro_context),
                        copied_macros);
    g_hash_table_foreach(macros, update_macro, copied_macros);
}


static void
cb_decoder_negotiate (MilterDecoder *decoder, MilterOption *option,
                      gpointer user_data)
{
    MilterHandler *handler = user_data;
    MilterStatus status = MILTER_STATUS_NOT_CHANGE;

    g_signal_emit(handler, signals[NEGOTIATE], 0, option, &status);
    g_signal_emit(handler, signals[NEGOTIATE_RESPONSE], 0,
                  option, status, &status);
}

static void
cb_decoder_define_macro (MilterDecoder *decoder, MilterCommand macro_context,
                         GHashTable *macros, gpointer user_data)
{
    MilterHandler *handler = user_data;

    set_macros(handler, macro_context, macros);
}

static void
cb_decoder_connect (MilterDecoder *decoder, const gchar *host_name,
                    struct sockaddr *address, socklen_t address_length,
                    gpointer user_data)
{
    MilterHandler *handler = user_data;
    MilterStatus status;
    MilterHandlerPrivate *priv;

    priv = MILTER_HANDLER_GET_PRIVATE(handler);
    priv->macro_context = MILTER_COMMAND_CONNECT;
    g_signal_emit(handler, signals[CONNECT], 0,
                  host_name, address, address_length, &status);
    g_signal_emit(handler, signals[CONNECT_RESPONSE], 0,
                  host_name, address, address_length, status, &status);
    milter_handler_clear_macros(handler, MILTER_COMMAND_CONNECT);
    priv->macro_context = MILTER_COMMAND_UNKNOWN;
}

static void
cb_decoder_helo (MilterDecoder *decoder, const gchar *fqdn, gpointer user_data)
{
    MilterHandler *handler = user_data;
    MilterStatus status;
    MilterHandlerPrivate *priv;

    priv = MILTER_HANDLER_GET_PRIVATE(handler);
    priv->macro_context = MILTER_COMMAND_HELO;
    g_signal_emit(handler, signals[HELO], 0, fqdn, &status);
    g_signal_emit(handler, signals[HELO_RESPONSE], 0, fqdn, status, &status);
    milter_handler_clear_macros(handler, MILTER_COMMAND_HELO);
    priv->macro_context = MILTER_COMMAND_UNKNOWN;
}

static void
cb_decoder_mail (MilterDecoder *decoder, const gchar *from, gpointer user_data)
{
    MilterHandler *handler = user_data;
    MilterStatus status;
    MilterHandlerPrivate *priv;

    priv = MILTER_HANDLER_GET_PRIVATE(handler);
    priv->macro_context = MILTER_COMMAND_MAIL;
    g_signal_emit(handler, signals[ENVELOPE_FROM], 0, from, &status);
    g_signal_emit(handler, signals[ENVELOPE_FROM_RESPONSE], 0, from, status, &status);
    milter_handler_clear_macros(handler, MILTER_COMMAND_MAIL);
    priv->macro_context = MILTER_COMMAND_UNKNOWN;
}

static void
cb_decoder_rcpt (MilterDecoder *decoder, const gchar *to, gpointer user_data)
{
    MilterHandler *handler = user_data;
    MilterStatus status;
    MilterHandlerPrivate *priv;

    priv = MILTER_HANDLER_GET_PRIVATE(handler);
    priv->macro_context = MILTER_COMMAND_RCPT;
    g_signal_emit(handler, signals[ENVELOPE_RECEIPT], 0, to, &status);
    g_signal_emit(handler, signals[ENVELOPE_RECEIPT_RESPONSE], 0, to, status, &status);
    milter_handler_clear_macros(handler, MILTER_COMMAND_RCPT);
    priv->macro_context = MILTER_COMMAND_UNKNOWN;
}

static void
cb_decoder_header (MilterDecoder *decoder,
                   const gchar *name, const gchar *value,
                   gpointer user_data)
{
    MilterHandler *handler = user_data;
    MilterStatus status;
    MilterHandlerPrivate *priv;

    priv = MILTER_HANDLER_GET_PRIVATE(handler);
    priv->macro_context = MILTER_COMMAND_HEADER;
    g_signal_emit(handler, signals[HEADER], 0, name, value, &status);
    g_signal_emit(handler, signals[HEADER_RESPONSE], 0, name, value, status, &status);
    milter_handler_clear_macros(handler, MILTER_COMMAND_HEADER);
    priv->macro_context = MILTER_COMMAND_UNKNOWN;
}

static void
cb_decoder_end_of_header (MilterDecoder *decoder, gpointer user_data)
{
    MilterHandler *handler = user_data;
    MilterStatus status;
    MilterHandlerPrivate *priv;

    priv = MILTER_HANDLER_GET_PRIVATE(handler);
    priv->macro_context = MILTER_COMMAND_END_OF_HEADER;
    g_signal_emit(handler, signals[END_OF_HEADER], 0, &status);
    g_signal_emit(handler, signals[END_OF_HEADER_RESPONSE], 0, status, &status);
    milter_handler_clear_macros(handler, MILTER_COMMAND_END_OF_HEADER);
    priv->macro_context = MILTER_COMMAND_UNKNOWN;
}

static void
cb_decoder_body (MilterDecoder *decoder, const gchar *chunk, gsize chunk_size,
                 gpointer user_data)
{
    MilterHandler *handler = user_data;
    MilterHandlerPrivate *priv;
    MilterStatus status;
    MilterHandler *context = MILTER_HANDLER(handler);

    priv = MILTER_HANDLER_GET_PRIVATE(context);
    priv->macro_context = MILTER_COMMAND_BODY;
    g_signal_emit(handler, signals[BODY], 0, chunk, chunk_size, &status);
    g_signal_emit(handler, signals[BODY_RESPONSE], 0,
                  chunk, chunk_size, status, &status);
    milter_handler_clear_macros(MILTER_HANDLER(context), MILTER_COMMAND_BODY);
    priv->macro_context = MILTER_COMMAND_UNKNOWN;
}

static void
cb_decoder_end_of_message (MilterDecoder *decoder, gpointer user_data)
{
    MilterHandler *handler = user_data;
    MilterStatus status;
    MilterHandlerPrivate *priv;

    priv = MILTER_HANDLER_GET_PRIVATE(handler);
    priv->macro_context = MILTER_COMMAND_END_OF_MESSAGE;
    g_signal_emit(handler, signals[END_OF_MESSAGE], 0, &status);
    g_signal_emit(handler, signals[END_OF_MESSAGE_RESPONSE], 0, status, &status);
    milter_handler_clear_macros(handler, MILTER_COMMAND_END_OF_MESSAGE);
    priv->macro_context = MILTER_COMMAND_UNKNOWN;
}

static void
cb_decoder_quit (MilterDecoder *decoder, gpointer user_data)
{
    MilterHandler *handler = user_data;
    MilterStatus status;

    g_signal_emit(handler, signals[CLOSE], 0, &status);
    g_signal_emit(handler, signals[CLOSE_RESPONSE], 0, status, &status);
}

static void
cb_decoder_abort (MilterDecoder *decoder, gpointer user_data)
{
    MilterHandler *handler = user_data;
    MilterStatus status;

    g_signal_emit(handler, signals[ABORT], 0, &status);
    g_signal_emit(handler, signals[ABORT_RESPONSE], 0, status, &status);
}

static void
setup_decoder (MilterHandler *handler, MilterDecoder *decoder)
{
#define CONNECT(name)                                                   \
    g_signal_connect(decoder, #name, G_CALLBACK(cb_decoder_ ## name),   \
                     handler)

    CONNECT(negotiate);
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

gboolean
milter_handler_reply (MilterHandler *handler, MilterStatus status)
{
    if (MILTER_HANDLER_GET_CLASS(handler)->reply)
        return MILTER_HANDLER_GET_CLASS(handler)->reply(handler, status);
    
    return FALSE;
}


/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
