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
#include "milter-client-marshalers.h"

#define MILTER_CLIENT_CONTEXT_GET_PRIVATE(obj)                          \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj),                                 \
                                 MILTER_CLIENT_TYPE_CONTEXT,            \
                                 MilterClientContextPrivate))

typedef struct _MilterClientContextPrivate	MilterClientContextPrivate;
struct _MilterClientContextPrivate
{
    gpointer private_data;
    GDestroyNotify private_data_destroy;
    guint reply_code;
    gchar *extended_reply_code;
    gchar *reply_message;
};

G_DEFINE_TYPE(MilterClientContext, milter_client_context, MILTER_TYPE_HANDLER);

static void dispose        (GObject         *object);
static void set_property   (GObject         *object,
                            guint            prop_id,
                            const GValue    *value,
                            GParamSpec      *pspec);
static void get_property   (GObject         *object,
                            guint            prop_id,
                            GValue          *value,
                            GParamSpec      *pspec);

static MilterStatus connect_response   (MilterHandler *handler,
                                        const gchar   *host_name,
                                        struct sockaddr *address,
                                        socklen_t      address_length,
                                        MilterStatus   status);
static MilterStatus negotiate_response (MilterHandler *handler,
                                        MilterOption  *option,
                                        MilterStatus   status);
static MilterStatus helo_response      (MilterHandler *handler,
                                        const gchar   *fqdn,
                                        MilterStatus   status);
static MilterStatus envelope_from_response
                                       (MilterHandler *handler,
                                        const gchar   *from,
                                        MilterStatus   status);
static MilterStatus envelope_receipt_response
                                       (MilterHandler *handler,
                                        const gchar   *receipt,
                                        MilterStatus   status);
static MilterStatus data_response      (MilterHandler *handler,
                                        MilterStatus   status);
static MilterStatus header_response    (MilterHandler *handler,
                                        const gchar   *name,
                                        const gchar   *value,
                                        MilterStatus   status);
static MilterStatus end_of_header_response
                                       (MilterHandler *handler,
                                        MilterStatus   status);
static MilterStatus body_response      (MilterHandler *handler,
                                        const guchar  *chunk,
                                        gsize          size,
                                        MilterStatus   status);
static MilterStatus end_of_message_response
                                       (MilterHandler *handler,
                                        MilterStatus   status);
static MilterStatus close_response     (MilterHandler *handler,
                                        MilterStatus   status);
static MilterStatus abort_response     (MilterHandler *handler,
                                        MilterStatus   status);

static void
milter_client_context_class_init (MilterClientContextClass *klass)
{
    GObjectClass *gobject_class;
    MilterHandlerClass *handler_class;

    gobject_class = G_OBJECT_CLASS(klass);
    handler_class = MILTER_HANDLER_CLASS(klass);

    gobject_class->dispose      = dispose;
    gobject_class->set_property = set_property;
    gobject_class->get_property = get_property;

    handler_class->negotiate_response = negotiate_response;
    handler_class->connect_response = connect_response;
    handler_class->helo_response = helo_response;
    handler_class->envelope_from_response = envelope_from_response;
    handler_class->envelope_receipt_response = envelope_receipt_response;
    handler_class->data_response = data_response;
    handler_class->header_response = header_response;
    handler_class->end_of_header_response = end_of_header_response;
    handler_class->body_response = body_response;
    handler_class->end_of_message_response = end_of_message_response;
    handler_class->close_response = close_response;
    handler_class->abort_response = abort_response;

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
}

static void
dispose (GObject *object)
{
    MilterClientContextPrivate *priv;

    priv = MILTER_CLIENT_CONTEXT_GET_PRIVATE(object);

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
    return g_object_new(MILTER_CLIENT_TYPE_CONTEXT,
                        NULL);
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

static gboolean
write_packet (MilterClientContext *context, gchar *packet, gsize packet_size)
{
    gboolean success;
    GError *error = NULL;

    if (!packet)
        return FALSE;

    success = milter_handler_write_packet(MILTER_HANDLER(context),
                                          packet, packet_size,
                                          &error);
    g_free(packet);
    if (error) {
        g_print("error: %s\n", error->message);
        g_error_free(error);
    }

    return success;
}

gboolean
milter_client_context_add_header (MilterClientContext *context,
                                  const gchar *name, const gchar *value)
{
    gchar *packet = NULL;
    gsize packet_size;
    MilterEncoder *encoder;

    encoder = milter_handler_get_encoder(MILTER_HANDLER(context));
    milter_encoder_encode_reply_add_header(encoder,
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

    encoder = milter_handler_get_encoder(MILTER_HANDLER(context));
    milter_encoder_encode_reply_insert_header(encoder,
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

    encoder = milter_handler_get_encoder(MILTER_HANDLER(context));
    milter_encoder_encode_reply_change_header(encoder,
                                              &packet, &packet_size,
                                              name, index, value);
    return write_packet(context, packet, packet_size);
}

gboolean
milter_client_context_remove_header (MilterClientContext *context,
                                     const gchar *name, guint32 index)
{
    return milter_client_context_change_header(context, name, index, NULL);
}

gboolean
milter_client_context_change_from (MilterClientContext *context,
                                   const gchar *from,
                                   const gchar *parameters)
{
    MilterEncoder *encoder;
    gchar *packet = NULL;
    gsize packet_size;

    encoder = milter_handler_get_encoder(MILTER_HANDLER(context));
    milter_encoder_encode_reply_change_from(encoder, &packet, &packet_size,
                                            from, parameters);
    return write_packet(context, packet, packet_size);
}

gboolean
milter_client_context_add_receipt (MilterClientContext *context,
                                   const gchar *receipt,
                                   const gchar *parameters)
{
    MilterEncoder *encoder;
    gchar *packet = NULL;
    gsize packet_size;

    encoder = milter_handler_get_encoder(MILTER_HANDLER(context));
    milter_encoder_encode_reply_add_receipt(encoder, &packet, &packet_size,
                                            receipt, parameters);
    return write_packet(context, packet, packet_size);
}

gboolean
milter_client_context_delete_receipt (MilterClientContext *context,
                                      const gchar *receipt)
{
    MilterEncoder *encoder;
    gchar *packet = NULL;
    gsize packet_size;

    encoder = milter_handler_get_encoder(MILTER_HANDLER(context));
    milter_encoder_encode_reply_delete_receipt(encoder, &packet, &packet_size,
                                               receipt);
    return write_packet(context, packet, packet_size);
}

gboolean
milter_client_context_replace_body (MilterClientContext *context,
                                    const gchar *body, gsize body_size)
{
    gsize rest_size, packed_size = 0;
    MilterEncoder *encoder;

    encoder = milter_handler_get_encoder(MILTER_HANDLER(context));
    for (rest_size = body_size; rest_size > 0; rest_size -= packed_size) {
        gchar *packet = NULL;
        gsize packet_size;
        gsize offset;

        offset = body_size - rest_size;
        milter_encoder_encode_reply_replace_body(encoder,
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

    encoder = milter_handler_get_encoder(MILTER_HANDLER(context));
    milter_encoder_encode_reply_progress(encoder, &packet, &packet_size);

    return write_packet(context, packet, packet_size);
}

gboolean
milter_client_context_quarantine (MilterClientContext *context,
                                  const gchar *reason)
{
    MilterEncoder *encoder;
    gchar *packet = NULL;
    gsize packet_size;

    encoder = milter_handler_get_encoder(MILTER_HANDLER(context));
    milter_encoder_encode_reply_quarantine(encoder, &packet, &packet_size,
                                           reason);

    return write_packet(context, packet, packet_size);
}

static gboolean
reply (MilterClientContext *context, MilterStatus status)
{
    gchar *packet = NULL;
    gsize packet_size;
    MilterEncoder *encoder;

    encoder = milter_handler_get_encoder(MILTER_HANDLER(context));

    switch (status) {
      case MILTER_STATUS_CONTINUE:
        milter_encoder_encode_reply_continue(encoder,
                                             &packet, &packet_size);
        break;
      case MILTER_STATUS_REJECT:
        {
            gchar *code;

            code = milter_client_context_format_reply(context);
            if (code) {
                milter_encoder_encode_reply_reply_code(encoder,
                                                       &packet, &packet_size,
                                                       code);
                g_free(code);
            } else {
                milter_encoder_encode_reply_reject(encoder,
                                                   &packet, &packet_size);
            }
        }
        break;
      case MILTER_STATUS_TEMPORARY_FAILURE:
        milter_encoder_encode_reply_temporary_failure(encoder,
                                                      &packet, &packet_size);
        break;
      case MILTER_STATUS_ACCEPT:
        milter_encoder_encode_reply_accept(encoder, &packet, &packet_size);
        break;
      case MILTER_STATUS_DISCARD:
        milter_encoder_encode_reply_discard(encoder,
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
negotiate_response (MilterHandler *handler,
                    MilterOption  *option,
                    MilterStatus   status)
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
        encoder = milter_handler_get_encoder(handler);

        milter_encoder_encode_negotiate(encoder,
                                        &packet, &packet_size,
                                        option);
        write_packet(MILTER_CLIENT_CONTEXT(handler), packet, packet_size);
        break;
      case MILTER_STATUS_REJECT:
        break;
      default:
        break;
    }
    return status;
}

static MilterStatus
connect_response (MilterHandler   *handler,
                  const gchar     *host_name,
                  struct sockaddr *address,
                  socklen_t        address_length,
                  MilterStatus     status)
{
    if (status == MILTER_STATUS_DEFAULT)
        status = MILTER_STATUS_CONTINUE;

    reply(MILTER_CLIENT_CONTEXT(handler), status);
    return status;
}

static MilterStatus
helo_response (MilterHandler *handler,
               const gchar   *fqdn,
               MilterStatus   status)
{
    if (status == MILTER_STATUS_DEFAULT)
        status = MILTER_STATUS_CONTINUE;

    reply(MILTER_CLIENT_CONTEXT(handler), status);
    return status;
}

static MilterStatus
envelope_from_response (MilterHandler *handler,
                        const gchar   *from,
                        MilterStatus   status)
{
    if (status == MILTER_STATUS_DEFAULT)
        status = MILTER_STATUS_CONTINUE;

    reply(MILTER_CLIENT_CONTEXT(handler), status);
    return status;
}

static MilterStatus
envelope_receipt_response (MilterHandler *handler,
                           const gchar   *receipt,
                           MilterStatus   status)
{
    if (status == MILTER_STATUS_DEFAULT)
        status = MILTER_STATUS_CONTINUE;

    reply(MILTER_CLIENT_CONTEXT(handler), status);
    return status;
}

static MilterStatus
data_response (MilterHandler *handler,
               MilterStatus   status)
{
    return MILTER_STATUS_NOT_CHANGE;
}

static MilterStatus
header_response (MilterHandler *handler,
                 const gchar   *name,
                 const gchar   *value,
                 MilterStatus   status)
{
    if (status == MILTER_STATUS_DEFAULT)
        status = MILTER_STATUS_CONTINUE;

    reply(MILTER_CLIENT_CONTEXT(handler), status);
    return status;
}

static MilterStatus
end_of_header_response (MilterHandler *handler,
                        MilterStatus   status)
{
    if (status == MILTER_STATUS_DEFAULT)
        status = MILTER_STATUS_CONTINUE;

    reply(MILTER_CLIENT_CONTEXT(handler), status);
    return status;
}

static MilterStatus
body_response (MilterHandler *handler,
               const guchar  *chunk,
               gsize          size,
               MilterStatus   status)
{
    if (status == MILTER_STATUS_DEFAULT)
        status = MILTER_STATUS_CONTINUE;

    reply(MILTER_CLIENT_CONTEXT(handler), status);
    return status;
}

static MilterStatus
end_of_message_response (MilterHandler *handler,
                         MilterStatus   status)
{
    if (status == MILTER_STATUS_DEFAULT)
        status = MILTER_STATUS_CONTINUE;

    reply(MILTER_CLIENT_CONTEXT(handler), status);
    return status;
}

static MilterStatus
close_response (MilterHandler *handler,
                MilterStatus   status)
{
    if (status == MILTER_STATUS_DEFAULT)
        status = MILTER_STATUS_CONTINUE;
    return status;
}

static MilterStatus
abort_response (MilterHandler *handler,
                MilterStatus   status)
{
    if (status == MILTER_STATUS_DEFAULT)
        status = MILTER_STATUS_CONTINUE;

    return status;
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
