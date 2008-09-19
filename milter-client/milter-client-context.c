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

static MilterClientStatus cb_option_negotiation (MilterClientContext *context,
                                                 MilterOption        *option);
static MilterClientStatus cb_connect            (MilterClientContext *context,
                                                 const gchar         *host_name,
                                                 struct sockaddr     *address,
                                                 socklen_t            address_length);
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

    macros = milter_client_context_get_macros(context);
    if (!macros)
        return NULL;

    return g_hash_table_lookup(macros, name);
}

GHashTable *
milter_client_context_get_macros (MilterClientContext *context)
{
    MilterClientContextPrivate *priv;

    priv = MILTER_CLIENT_CONTEXT_GET_PRIVATE(context);
    return g_hash_table_lookup(priv->macros,
                               GINT_TO_POINTER(priv->macro_context));
}

static gboolean
status_accumulator (GSignalInvocationHint *hint,
                   GValue *return_accumulator,
                   const GValue *context_return,
                   gpointer data)
{
    MilterClientStatus status;

    status = g_value_get_enum(context_return);
    g_value_set_enum(return_accumulator, status);

    return status == MILTER_CLIENT_STATUS_CONTINUE;
}


static void
cb_decoder_option_negotiation (MilterDecoder *decoder, MilterOption *option,
                               gpointer user_data)
{
    MilterClientContext *context = user_data;
    MilterClientStatus status = MILTER_CLIENT_STATUS_ALL_OPTIONS;

    g_signal_emit(context, signals[OPTION_NEGOTIATION], 0, option, &status);
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
    MilterClientContextPrivate *priv;
    MilterClientStatus status = MILTER_CLIENT_STATUS_CONTINUE;

    priv = MILTER_CLIENT_CONTEXT_GET_PRIVATE(context);
    priv->macro_context = MILTER_COMMAND_CONNECT;
    g_signal_emit(context, signals[CONNECT], 0,
                  host_name, address, address_length, &status);
    clear_macros(context, priv->macro_context);
    priv->macro_context = MILTER_COMMAND_UNKNOWN;
}

static void
cb_decoder_helo (MilterDecoder *decoder, const gchar *fqdn, gpointer user_data)
{
    MilterClientContext *context = user_data;
    MilterClientContextPrivate *priv;
    MilterClientStatus status = MILTER_CLIENT_STATUS_CONTINUE;

    priv = MILTER_CLIENT_CONTEXT_GET_PRIVATE(context);
    priv->macro_context = MILTER_COMMAND_HELO;
    g_signal_emit(context, signals[HELO], 0, fqdn, &status);
    clear_macros(context, priv->macro_context);
    priv->macro_context = MILTER_COMMAND_UNKNOWN;
}

static void
cb_decoder_mail (MilterDecoder *decoder, const gchar *from, gpointer user_data)
{
    MilterClientContext *context = user_data;
    MilterClientContextPrivate *priv;
    MilterClientStatus status = MILTER_CLIENT_STATUS_CONTINUE;

    priv = MILTER_CLIENT_CONTEXT_GET_PRIVATE(context);
    priv->macro_context = MILTER_COMMAND_MAIL;
    g_signal_emit(context, signals[ENVELOPE_FROM], 0, from, &status);
    clear_macros(context, priv->macro_context);
    priv->macro_context = MILTER_COMMAND_UNKNOWN;
}

static void
cb_decoder_rcpt (MilterDecoder *decoder, const gchar *to, gpointer user_data)
{
    MilterClientContext *context = user_data;
    MilterClientStatus status = MILTER_CLIENT_STATUS_CONTINUE;

    g_signal_emit(context, signals[ENVELOPE_RECEIPT], 0, to, &status);
}

static void
cb_decoder_header (MilterDecoder *decoder,
                   const gchar *name, const gchar *value,
                   gpointer user_data)
{
    MilterClientContext *context = user_data;
    MilterClientStatus status = MILTER_CLIENT_STATUS_CONTINUE;

    g_signal_emit(context, signals[HEADER], 0, name, value, &status);
}

static void
cb_decoder_end_of_header (MilterDecoder *decoder, gpointer user_data)
{
    MilterClientContext *context = user_data;
    MilterClientStatus status = MILTER_CLIENT_STATUS_CONTINUE;

    g_signal_emit(context, signals[END_OF_HEADER], 0, &status);
}

static void
cb_decoder_body (MilterDecoder *decoder, const gchar *chunk, gsize chunk_size,
                 gpointer user_data)
{
    MilterClientContext *context = user_data;
    MilterClientStatus status = MILTER_CLIENT_STATUS_CONTINUE;

    g_signal_emit(context, signals[BODY], 0, chunk, chunk_size, &status);
}

static void
cb_decoder_end_of_message (MilterDecoder *decoder, gpointer user_data)
{
    MilterClientContext *context = user_data;
    MilterClientStatus status = MILTER_CLIENT_STATUS_CONTINUE;

    g_signal_emit(context, signals[END_OF_MESSAGE], 0, &status);
}

static void
cb_decoder_quit (MilterDecoder *decoder, gpointer user_data)
{
    MilterClientContext *context = user_data;
    MilterClientStatus status = MILTER_CLIENT_STATUS_CONTINUE;

    g_signal_emit(context, signals[CLOSE], 0, &status);
}

static void
cb_decoder_abort (MilterDecoder *decoder, gpointer user_data)
{
    MilterClientContext *context = user_data;
    MilterClientStatus status = MILTER_CLIENT_STATUS_CONTINUE;

    g_signal_emit(context, signals[ABORT], 0, &status);
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
    return MILTER_CLIENT_STATUS_CONTINUE;
}

static MilterClientStatus
cb_connect (MilterClientContext *context,
            const gchar         *host_name,
            struct sockaddr     *address,
            socklen_t            address_length)
{
    return MILTER_CLIENT_STATUS_CONTINUE;
}

static MilterClientStatus
cb_helo (MilterClientContext *context,
         const gchar         *fqdn)
{
    return MILTER_CLIENT_STATUS_CONTINUE;
}

static MilterClientStatus
cb_envelope_from (MilterClientContext *context,
                  const gchar         *from)
{
    return MILTER_CLIENT_STATUS_CONTINUE;
}

static MilterClientStatus
cb_envelope_receipt (MilterClientContext *context,
                     const gchar         *receipt)
{
    return MILTER_CLIENT_STATUS_CONTINUE;
}

static MilterClientStatus
cb_data (MilterClientContext *context)
{
    return MILTER_CLIENT_STATUS_CONTINUE;
}

static MilterClientStatus
cb_header (MilterClientContext *context,
           const gchar         *name,
           const gchar         *value)
{
    return MILTER_CLIENT_STATUS_CONTINUE;
}

static MilterClientStatus
cb_end_of_header (MilterClientContext *context)
{
    return MILTER_CLIENT_STATUS_CONTINUE;
}

static MilterClientStatus
cb_body (MilterClientContext *context,
         const guchar        *chunk,
         gsize                size)
{
    return MILTER_CLIENT_STATUS_CONTINUE;
}

static MilterClientStatus
cb_end_of_message (MilterClientContext *context)
{
    return MILTER_CLIENT_STATUS_CONTINUE;
}

static MilterClientStatus
cb_close (MilterClientContext *context)
{
    return MILTER_CLIENT_STATUS_CONTINUE;
}

static MilterClientStatus
cb_abort (MilterClientContext *context)
{
    return MILTER_CLIENT_STATUS_CONTINUE;
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
