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

#include <milter-core.h>
#include "milter-client.h"
#include "milter-client-handler.h"
#include "milter-client-enum-types.h"
#include "milter-client-marshalers.h"

#define MILTER_CLIENT_HANDLER_GET_PRIVATE(obj)                          \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj),                                 \
                                 MILTER_CLIENT_TYPE_HANDLER,            \
                                 MilterClientHandlerPrivate))

typedef struct _MilterClientHandlerPrivate	MilterClientHandlerPrivate;
struct _MilterClientHandlerPrivate
{
    MilterDecoder *decoder;
    MilterEncoder *encoder;
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

G_DEFINE_TYPE(MilterClientHandler, milter_client_handler, G_TYPE_OBJECT);

static void dispose        (GObject         *object);
static void set_property   (GObject         *object,
                            guint            prop_id,
                            const GValue    *value,
                            GParamSpec      *pspec);
static void get_property   (GObject         *object,
                            guint            prop_id,
                            GValue          *value,
                            GParamSpec      *pspec);

static MilterClientStatus cb_option_negotiation (MilterClientHandler *handler,
                                                 MilterOption        *option);
static MilterClientStatus cb_connect            (MilterClientHandler *handler,
                                                 const gchar         *host_name,
                                                 struct sockaddr     *address,
                                                 socklen_t            address_length);
static MilterClientStatus cb_helo               (MilterClientHandler *handler,
                                                 const gchar         *fqdn);
static MilterClientStatus cb_envelope_from      (MilterClientHandler *handler,
                                                 const gchar         *from);
static MilterClientStatus cb_envelope_receipt   (MilterClientHandler *handler,
                                                 const gchar         *receipt);
static MilterClientStatus cb_data               (MilterClientHandler *handler);
static MilterClientStatus cb_header             (MilterClientHandler *handler,
                                                 const gchar         *name,
                                                 const gchar         *value);
static MilterClientStatus cb_end_of_header      (MilterClientHandler *handler);
static MilterClientStatus cb_body               (MilterClientHandler *handler,
                                                 const guchar        *chunk,
                                                 gsize                size);
static MilterClientStatus cb_end_of_message     (MilterClientHandler *handler);
static MilterClientStatus cb_close              (MilterClientHandler *handler);
static MilterClientStatus cb_abort              (MilterClientHandler *handler);

static gboolean    status_accumulator  (GSignalInvocationHint *hint,
                                        GValue *return_accumulator,
                                        const GValue *handler_return,
                                        gpointer data);

static void        setup_decoder       (MilterClientHandler *handler,
                                        MilterDecoder       *decoder);

static void
milter_client_handler_class_init (MilterClientHandlerClass *klass)
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
                     G_SIGNAL_RUN_FIRST |
                     G_SIGNAL_RUN_LAST |
                     G_SIGNAL_RUN_CLEANUP,
                     G_STRUCT_OFFSET(MilterClientHandlerClass,
                                     option_negotiation),
                     status_accumulator, NULL,
                     _milter_client_marshal_ENUM__OBJECT,
                     MILTER_TYPE_CLIENT_STATUS, 1, MILTER_TYPE_OPTION);

    signals[CONNECT] =
        g_signal_new("connect",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_FIRST |
                     G_SIGNAL_RUN_LAST |
                     G_SIGNAL_RUN_CLEANUP,
                     G_STRUCT_OFFSET(MilterClientHandlerClass, connect),
                     status_accumulator, NULL,
                     _milter_client_marshal_ENUM__STRING_POINTER_UINT,
                     MILTER_TYPE_CLIENT_STATUS, 3,
                     G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_UINT);

    signals[HELO] =
        g_signal_new("helo",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_FIRST |
                     G_SIGNAL_RUN_LAST |
                     G_SIGNAL_RUN_CLEANUP,
                     G_STRUCT_OFFSET(MilterClientHandlerClass, helo),
                     status_accumulator, NULL,
                     _milter_client_marshal_ENUM__STRING_POINTER_UINT,
                     MILTER_TYPE_CLIENT_STATUS, 3,
                     G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_UINT);

    signals[ENVELOPE_FROM] =
        g_signal_new("envelope-from",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_FIRST |
                     G_SIGNAL_RUN_LAST |
                     G_SIGNAL_RUN_CLEANUP,
                     G_STRUCT_OFFSET(MilterClientHandlerClass, envelope_from),
                     status_accumulator, NULL,
                     _milter_client_marshal_ENUM__STRING_POINTER_UINT,
                     MILTER_TYPE_CLIENT_STATUS, 3,
                     G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_UINT);

    signals[ENVELOPE_RECEIPT] =
        g_signal_new("envelope-receipt",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_FIRST |
                     G_SIGNAL_RUN_LAST |
                     G_SIGNAL_RUN_CLEANUP,
                     G_STRUCT_OFFSET(MilterClientHandlerClass, envelope_receipt),
                     status_accumulator, NULL,
                     _milter_client_marshal_ENUM__STRING_POINTER_UINT,
                     MILTER_TYPE_CLIENT_STATUS, 3,
                     G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_UINT);

    signals[DATA] =
        g_signal_new("data",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_FIRST |
                     G_SIGNAL_RUN_LAST |
                     G_SIGNAL_RUN_CLEANUP,
                     G_STRUCT_OFFSET(MilterClientHandlerClass, data),
                     status_accumulator, NULL,
                     _milter_client_marshal_ENUM__STRING_POINTER_UINT,
                     MILTER_TYPE_CLIENT_STATUS, 3,
                     G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_UINT);

    signals[UNKNOWN] =
        g_signal_new("unknown",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_FIRST |
                     G_SIGNAL_RUN_LAST |
                     G_SIGNAL_RUN_CLEANUP,
                     G_STRUCT_OFFSET(MilterClientHandlerClass, unknown),
                     status_accumulator, NULL,
                     _milter_client_marshal_ENUM__STRING_POINTER_UINT,
                     MILTER_TYPE_CLIENT_STATUS, 3,
                     G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_UINT);

    signals[HEADER] =
        g_signal_new("header",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_FIRST |
                     G_SIGNAL_RUN_LAST |
                     G_SIGNAL_RUN_CLEANUP,
                     G_STRUCT_OFFSET(MilterClientHandlerClass, header),
                     status_accumulator, NULL,
                     _milter_client_marshal_ENUM__STRING_POINTER_UINT,
                     MILTER_TYPE_CLIENT_STATUS, 3,
                     G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_UINT);

    signals[END_OF_HEADER] =
        g_signal_new("end-of-header",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_FIRST |
                     G_SIGNAL_RUN_LAST |
                     G_SIGNAL_RUN_CLEANUP,
                     G_STRUCT_OFFSET(MilterClientHandlerClass, end_of_header),
                     status_accumulator, NULL,
                     _milter_client_marshal_ENUM__STRING_POINTER_UINT,
                     MILTER_TYPE_CLIENT_STATUS, 3,
                     G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_UINT);

    signals[BODY] =
        g_signal_new("body",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_FIRST |
                     G_SIGNAL_RUN_LAST |
                     G_SIGNAL_RUN_CLEANUP,
                     G_STRUCT_OFFSET(MilterClientHandlerClass, body),
                     status_accumulator, NULL,
                     _milter_client_marshal_ENUM__STRING_POINTER_UINT,
                     MILTER_TYPE_CLIENT_STATUS, 3,
                     G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_UINT);

    signals[END_OF_MESSAGE] =
        g_signal_new("end-of-message",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_FIRST |
                     G_SIGNAL_RUN_LAST |
                     G_SIGNAL_RUN_CLEANUP,
                     G_STRUCT_OFFSET(MilterClientHandlerClass, end_of_message),
                     status_accumulator, NULL,
                     _milter_client_marshal_ENUM__STRING_POINTER_UINT,
                     MILTER_TYPE_CLIENT_STATUS, 3,
                     G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_UINT);

    signals[CLOSE] =
        g_signal_new("close",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_FIRST |
                     G_SIGNAL_RUN_LAST |
                     G_SIGNAL_RUN_CLEANUP,
                     G_STRUCT_OFFSET(MilterClientHandlerClass, close),
                     status_accumulator, NULL,
                     _milter_client_marshal_ENUM__STRING_POINTER_UINT,
                     MILTER_TYPE_CLIENT_STATUS, 3,
                     G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_UINT);

    signals[ABORT] =
        g_signal_new("abort",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_FIRST |
                     G_SIGNAL_RUN_LAST |
                     G_SIGNAL_RUN_CLEANUP,
                     G_STRUCT_OFFSET(MilterClientHandlerClass, abort),
                     status_accumulator, NULL,
                     _milter_client_marshal_ENUM__STRING_POINTER_UINT,
                     MILTER_TYPE_CLIENT_STATUS, 3,
                     G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_UINT);

    g_type_class_add_private(gobject_class, sizeof(MilterClientHandlerPrivate));
}

static void
milter_client_handler_init (MilterClientHandler *handler)
{
    MilterClientHandlerPrivate *priv;

    priv = MILTER_CLIENT_HANDLER_GET_PRIVATE(handler);
    priv->decoder = milter_decoder_new();
    priv->encoder = milter_encoder_new();
    setup_decoder(handler, priv->decoder);
}

static void
dispose (GObject *object)
{
    MilterClientHandlerPrivate *priv;

    priv = MILTER_CLIENT_HANDLER_GET_PRIVATE(object);

    if (priv->decoder) {
        g_object_unref(priv->decoder);
        priv->decoder = NULL;
    }

    if (priv->encoder) {
        g_object_unref(priv->encoder);
        priv->encoder = NULL;
    }

    G_OBJECT_CLASS(milter_client_handler_parent_class)->dispose(object);
}

static void
set_property (GObject      *object,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
    MilterClientHandlerPrivate *priv;

    priv = MILTER_CLIENT_HANDLER_GET_PRIVATE(object);
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
    MilterClientHandlerPrivate *priv;

    priv = MILTER_CLIENT_HANDLER_GET_PRIVATE(object);
    switch (prop_id) {
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

MilterClientHandler *
milter_client_handler_new (void)
{
    return g_object_new(MILTER_CLIENT_TYPE_HANDLER,
                        NULL);
}

gboolean
milter_client_handler_feed (MilterClientHandler *handler,
                            const gchar *chunk, gsize size,
                            GError **error)
{
    MilterClientHandlerPrivate *priv;

    priv = MILTER_CLIENT_HANDLER_GET_PRIVATE(handler);
    return milter_decoder_decode(priv->decoder, chunk, size, error);
}

static gboolean
status_accumulator (GSignalInvocationHint *hint,
                   GValue *return_accumulator,
                   const GValue *handler_return,
                   gpointer data)
{
    MilterClientStatus status;

    status = g_value_get_enum(handler_return);
    g_value_set_enum(return_accumulator, status);

    return status == MILTER_CLIENT_STATUS_CONTINUE;
}

static void
cb_decoder_connect (MilterDecoder *decoder, const gchar *host_name,
                    struct sockaddr *address, socklen_t address_length,
                    gpointer user_data)
{
    MilterClientHandler *handler = user_data;
    MilterClientStatus status = MILTER_CLIENT_STATUS_CONTINUE;

    g_signal_emit(handler, signals[CONNECT], 0,
                  host_name, address, address_length, &status);
}

static void
setup_decoder (MilterClientHandler *handler, MilterDecoder *decoder)
{
    g_signal_connect(decoder, "connect",
                     G_CALLBACK(cb_decoder_connect), handler);
}


static MilterClientStatus
cb_option_negotiation (MilterClientHandler *handler,
                       MilterOption        *option)
{
    return MILTER_CLIENT_STATUS_CONTINUE;
}

static MilterClientStatus
cb_connect (MilterClientHandler *handler,
            const gchar         *host_name,
            struct sockaddr     *address,
            socklen_t            address_length)
{
    return MILTER_CLIENT_STATUS_CONTINUE;
}

static MilterClientStatus
cb_helo (MilterClientHandler *handler,
         const gchar         *fqdn)
{
    return MILTER_CLIENT_STATUS_CONTINUE;
}

static MilterClientStatus
cb_envelope_from (MilterClientHandler *handler,
                  const gchar         *from)
{
    return MILTER_CLIENT_STATUS_CONTINUE;
}

static MilterClientStatus
cb_envelope_receipt (MilterClientHandler *handler,
                     const gchar         *receipt)
{
    return MILTER_CLIENT_STATUS_CONTINUE;
}

static MilterClientStatus
cb_data (MilterClientHandler *handler)
{
    return MILTER_CLIENT_STATUS_CONTINUE;
}

static MilterClientStatus
cb_header (MilterClientHandler *handler,
           const gchar         *name,
           const gchar         *value)
{
    return MILTER_CLIENT_STATUS_CONTINUE;
}

static MilterClientStatus
cb_end_of_header (MilterClientHandler *handler)
{
    return MILTER_CLIENT_STATUS_CONTINUE;
}

static MilterClientStatus
cb_body (MilterClientHandler *handler,
         const guchar        *chunk,
         gsize                size)
{
    return MILTER_CLIENT_STATUS_CONTINUE;
}

static MilterClientStatus
cb_end_of_message (MilterClientHandler *handler)
{
    return MILTER_CLIENT_STATUS_CONTINUE;
}

static MilterClientStatus
cb_close (MilterClientHandler *handler)
{
    return MILTER_CLIENT_STATUS_CONTINUE;
}

static MilterClientStatus
cb_abort (MilterClientHandler *handler)
{
    return MILTER_CLIENT_STATUS_CONTINUE;
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
