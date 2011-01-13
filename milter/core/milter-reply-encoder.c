/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Copyright (C) 2008-2011  Kouhei Sutou <kou@clear-code.com>
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

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>

#include <glib.h>

#include "milter-reply-encoder.h"
#include "milter-logger.h"
#include "milter-enum-types.h"
#include "milter-marshalers.h"
#include "milter-macros-requests.h"
#include "milter-utils.h"

G_DEFINE_TYPE(MilterReplyEncoder, milter_reply_encoder, MILTER_TYPE_ENCODER);

static void dispose        (GObject         *object);

static void
milter_reply_encoder_class_init (MilterReplyEncoderClass *klass)
{
    GObjectClass *gobject_class;

    gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->dispose      = dispose;
}

static void
milter_reply_encoder_init (MilterReplyEncoder *encoder)
{
}

static void
dispose (GObject *object)
{
    G_OBJECT_CLASS(milter_reply_encoder_parent_class)->dispose(object);
}

MilterEncoder *
milter_reply_encoder_new (void)
{
    return g_object_new(MILTER_TYPE_REPLY_ENCODER, NULL);
}

static void
encode_macros_request (gpointer key, gpointer value, gpointer user_data)
{
    MilterCommand command = GPOINTER_TO_INT(key);
    GList *symbols = value;
    GString *buffer = user_data;
    MilterMacroStage stage, normalized_stage;
    gchar encoded_stage[sizeof(guint32)];

    stage = milter_utils_command_to_macro_stage(command);
    if (stage == -1) {
        /* should emit 'error' signal? */
        milter_error("[reply-encoder][error][macro] unknown command: '%c'",
                     command);
        return;
    }

    normalized_stage = g_htonl(stage);
    memcpy(encoded_stage, &normalized_stage, sizeof(encoded_stage));
    g_string_append_len(buffer, encoded_stage, sizeof(encoded_stage));

    g_string_append(buffer, symbols->data);
    for (symbols = g_list_next(symbols);
         symbols;
         symbols = g_list_next(symbols)) {
        g_string_append_c(buffer, ' ');
        g_string_append(buffer, symbols->data);
    }
    g_string_append_c(buffer, '\0');
}

static void
encode_macros_requests (GString *buffer, MilterMacrosRequests *requests)
{
    milter_macros_requests_foreach(requests, encode_macros_request, buffer);
}

void
milter_reply_encoder_encode_negotiate (MilterReplyEncoder *encoder,
                                       const gchar **packet,
                                       gsize *packet_size,
                                       MilterOption *option,
                                       MilterMacrosRequests *macros_requests)
{
    MilterEncoder *base_encoder;
    GString *buffer;

    base_encoder = MILTER_ENCODER(encoder);
    milter_encoder_encode_negotiate(base_encoder, option);
    if (macros_requests) {
        buffer = milter_encoder_get_buffer(base_encoder);
        encode_macros_requests(buffer, macros_requests);
    }
    milter_encoder_pack(base_encoder, packet, packet_size);
}

void
milter_reply_encoder_encode_continue (MilterReplyEncoder *encoder,
                                      const gchar **packet,
                                      gsize *packet_size)
{
    MilterEncoder *base_encoder;
    GString *buffer;

    base_encoder = MILTER_ENCODER(encoder);
    milter_encoder_clear_buffer(base_encoder);
    buffer = milter_encoder_get_buffer(base_encoder);


    g_string_append_c(buffer, MILTER_REPLY_CONTINUE);
    milter_encoder_pack(base_encoder, packet, packet_size);
}

void
milter_reply_encoder_encode_reply_code (MilterReplyEncoder *encoder,
                                        const gchar **packet,
                                        gsize *packet_size,
                                        const gchar *code)
{
    MilterEncoder *base_encoder;
    GString *buffer;

    if (!code) {
        *packet = NULL;
        *packet_size = 0;
        return;
    }

    base_encoder = MILTER_ENCODER(encoder);
    milter_encoder_clear_buffer(base_encoder);
    buffer = milter_encoder_get_buffer(base_encoder);


    g_string_append_c(buffer, MILTER_REPLY_REPLY_CODE);
    g_string_append(buffer, code);
    g_string_append_c(buffer, '\0');
    milter_encoder_pack(base_encoder, packet, packet_size);
}

void
milter_reply_encoder_encode_temporary_failure (MilterReplyEncoder *encoder,
                                               const gchar **packet,
                                               gsize *packet_size)
{
    MilterEncoder *base_encoder;
    GString *buffer;

    base_encoder = MILTER_ENCODER(encoder);
    milter_encoder_clear_buffer(base_encoder);
    buffer = milter_encoder_get_buffer(base_encoder);


    g_string_append_c(buffer, MILTER_REPLY_TEMPORARY_FAILURE);
    milter_encoder_pack(base_encoder, packet, packet_size);
}

void
milter_reply_encoder_encode_reject (MilterReplyEncoder *encoder,
                                    const gchar **packet,
                                    gsize *packet_size)
{
    MilterEncoder *base_encoder;
    GString *buffer;

    base_encoder = MILTER_ENCODER(encoder);
    milter_encoder_clear_buffer(base_encoder);
    buffer = milter_encoder_get_buffer(base_encoder);


    g_string_append_c(buffer, MILTER_REPLY_REJECT);
    milter_encoder_pack(base_encoder, packet, packet_size);
}

void
milter_reply_encoder_encode_accept (MilterReplyEncoder *encoder,
                                    const gchar **packet,
                                    gsize *packet_size)
{
    MilterEncoder *base_encoder;
    GString *buffer;

    base_encoder = MILTER_ENCODER(encoder);
    milter_encoder_clear_buffer(base_encoder);
    buffer = milter_encoder_get_buffer(base_encoder);


    g_string_append_c(buffer, MILTER_REPLY_ACCEPT);
    milter_encoder_pack(base_encoder, packet, packet_size);
}

void
milter_reply_encoder_encode_discard (MilterReplyEncoder *encoder,
                                     const gchar **packet,
                                     gsize *packet_size)
{
    MilterEncoder *base_encoder;
    GString *buffer;

    base_encoder = MILTER_ENCODER(encoder);
    milter_encoder_clear_buffer(base_encoder);
    buffer = milter_encoder_get_buffer(base_encoder);


    g_string_append_c(buffer, MILTER_REPLY_DISCARD);
    milter_encoder_pack(base_encoder, packet, packet_size);
}

static void
encode_reply_header (MilterReplyEncoder *encoder,
                     const gchar **packet, gsize *packet_size,
                     MilterReply reply,
                     const gchar *name, const gchar *value, guint32 index)
{
    MilterEncoder *base_encoder;
    GString *buffer;

    if (name == NULL || name[0] == '\0') {
        *packet = NULL;
        *packet_size = 0;
        return;
    }

    base_encoder = MILTER_ENCODER(encoder);
    milter_encoder_clear_buffer(base_encoder);
    buffer = milter_encoder_get_buffer(base_encoder);


    g_string_append_c(buffer, reply);
    if (reply != MILTER_REPLY_ADD_HEADER) {
        guint32 normalized_index;
        gchar encoded_index[sizeof(guint32)];

        normalized_index = g_htonl(index);
        memcpy(encoded_index, &normalized_index, sizeof(encoded_index));
        g_string_append_len(buffer, encoded_index, sizeof(encoded_index));
    }
    g_string_append(buffer, name);
    g_string_append_c(buffer, '\0');
    if (value)
        g_string_append(buffer, value);
    g_string_append_c(buffer, '\0');
    milter_encoder_pack(base_encoder, packet, packet_size);
}

void
milter_reply_encoder_encode_add_header (MilterReplyEncoder *encoder,
                                        const gchar **packet,
                                        gsize *packet_size,
                                        const gchar *name,
                                        const gchar *value)
{
    encode_reply_header(encoder, packet, packet_size,
                        MILTER_REPLY_ADD_HEADER,
                        name, value,
                        0);
}

void
milter_reply_encoder_encode_insert_header (MilterReplyEncoder *encoder,
                                           const gchar **packet,
                                           gsize *packet_size,
                                           guint32 index,
                                           const gchar *name,
                                           const gchar *value)
{
    encode_reply_header(encoder, packet, packet_size,
                        MILTER_REPLY_INSERT_HEADER,
                        name, value,
                        index);
}

void
milter_reply_encoder_encode_change_header (MilterReplyEncoder *encoder,
                                           const gchar **packet,
                                           gsize *packet_size,
                                           const gchar *name,
                                           guint32 index,
                                           const gchar *value)
{
    encode_reply_header(encoder, packet, packet_size,
                        MILTER_REPLY_CHANGE_HEADER,
                        name, value,
                        index);
}

void
milter_reply_encoder_encode_delete_header (MilterReplyEncoder *encoder,
                                           const gchar **packet,
                                           gsize *packet_size,
                                           const gchar *name,
                                           guint32 index)
{
    encode_reply_header(encoder, packet, packet_size,
                        MILTER_REPLY_CHANGE_HEADER,
                        name, NULL,
                        index);
}

void
milter_reply_encoder_encode_change_from (MilterReplyEncoder *encoder,
                                         const gchar **packet,
                                         gsize *packet_size,
                                         const gchar *from,
                                         const gchar *parameters)
{
    MilterEncoder *base_encoder;
    GString *buffer;

    if (from == NULL || from[0] == '\0') {
        *packet = NULL;
        *packet_size = 0;
        return;
    }

    base_encoder = MILTER_ENCODER(encoder);
    milter_encoder_clear_buffer(base_encoder);
    buffer = milter_encoder_get_buffer(base_encoder);

    g_string_append_c(buffer, MILTER_REPLY_CHANGE_FROM);
    g_string_append(buffer, from);
    g_string_append_c(buffer, '\0');
    if (parameters && parameters != '\0') {
        g_string_append(buffer, parameters);
        g_string_append_c(buffer, '\0');
    }
    milter_encoder_pack(base_encoder, packet, packet_size);
}

void
milter_reply_encoder_encode_add_recipient (MilterReplyEncoder *encoder,
                                           const gchar **packet,
                                           gsize *packet_size,
                                           const gchar *recipient,
                                           const gchar *parameters)
{
    MilterEncoder *base_encoder;
    GString *buffer;
    MilterReply reply;
    gboolean have_parameter;

    if (recipient == NULL || recipient[0] == '\0') {
        *packet = NULL;
        *packet_size = 0;
        return;
    }

    base_encoder = MILTER_ENCODER(encoder);
    milter_encoder_clear_buffer(base_encoder);
    buffer = milter_encoder_get_buffer(base_encoder);

    have_parameter = parameters && parameters[0] != '\0';
    if (have_parameter)
        reply = MILTER_REPLY_ADD_RECIPIENT_WITH_PARAMETERS;
    else
        reply = MILTER_REPLY_ADD_RECIPIENT;
    g_string_append_c(buffer, reply);
    g_string_append(buffer, recipient);
    g_string_append_c(buffer, '\0');
    if (have_parameter) {
        g_string_append(buffer, parameters);
        g_string_append_c(buffer, '\0');
    }
    milter_encoder_pack(base_encoder, packet, packet_size);
}

void
milter_reply_encoder_encode_delete_recipient (MilterReplyEncoder *encoder,
                                              const gchar **packet,
                                              gsize *packet_size,
                                              const gchar *recipient)
{
    MilterEncoder *base_encoder;
    GString *buffer;

    if (recipient == NULL || recipient[0] == '\0') {
        *packet = NULL;
        *packet_size = 0;
        return;
    }

    base_encoder = MILTER_ENCODER(encoder);
    milter_encoder_clear_buffer(base_encoder);
    buffer = milter_encoder_get_buffer(base_encoder);

    g_string_append_c(buffer, MILTER_REPLY_DELETE_RECIPIENT);
    g_string_append(buffer, recipient);
    g_string_append_c(buffer, '\0');
    milter_encoder_pack(base_encoder, packet, packet_size);
}

void
milter_reply_encoder_encode_replace_body (MilterReplyEncoder *encoder,
                                          const gchar **packet,
                                          gsize *packet_size,
                                          const gchar *body, gsize body_size,
                                          gsize *packed_size)
{
    MilterEncoder *base_encoder;
    GString *buffer;

    if (body_size <= 0 || (body == NULL && body_size > 0)) {
        *packet = NULL;
        *packet_size = 0;
        *packed_size = 0;
        return;
    }

    base_encoder = MILTER_ENCODER(encoder);
    milter_encoder_clear_buffer(base_encoder);
    buffer = milter_encoder_get_buffer(base_encoder);

    g_string_append_c(buffer, MILTER_REPLY_REPLACE_BODY);
    if (body_size > MILTER_CHUNK_SIZE)
        *packed_size = MILTER_CHUNK_SIZE;
    else
        *packed_size = body_size;
    g_string_append_len(buffer, body, *packed_size);
    milter_encoder_pack(base_encoder, packet, packet_size);
}

void
milter_reply_encoder_encode_progress (MilterReplyEncoder *encoder,
                                      const gchar **packet,
                                      gsize *packet_size)
{
    MilterEncoder *base_encoder;
    GString *buffer;

    base_encoder = MILTER_ENCODER(encoder);
    milter_encoder_clear_buffer(base_encoder);
    buffer = milter_encoder_get_buffer(base_encoder);

    g_string_append_c(buffer, MILTER_REPLY_PROGRESS);
    milter_encoder_pack(base_encoder, packet, packet_size);
}

void
milter_reply_encoder_encode_quarantine (MilterReplyEncoder *encoder,
                                        const gchar **packet,
                                        gsize *packet_size,
                                        const gchar *reason)
{
    MilterEncoder *base_encoder;
    GString *buffer;

    if (reason == NULL || reason[0] == '\0') {
        *packet = NULL;
        *packet_size = 0;
        return;
    }

    base_encoder = MILTER_ENCODER(encoder);
    milter_encoder_clear_buffer(base_encoder);
    buffer = milter_encoder_get_buffer(base_encoder);

    g_string_append_c(buffer, MILTER_REPLY_QUARANTINE);
    g_string_append(buffer, reason);
    g_string_append_c(buffer, '\0');
    milter_encoder_pack(base_encoder, packet, packet_size);
}

void
milter_reply_encoder_encode_connection_failure (MilterReplyEncoder *encoder,
                                                const gchar **packet,
                                                gsize *packet_size)
{
    MilterEncoder *base_encoder;
    GString *buffer;

    base_encoder = MILTER_ENCODER(encoder);
    milter_encoder_clear_buffer(base_encoder);
    buffer = milter_encoder_get_buffer(base_encoder);


    g_string_append_c(buffer, MILTER_REPLY_CONNECTION_FAILURE);
    milter_encoder_pack(base_encoder, packet, packet_size);
}

void
milter_reply_encoder_encode_shutdown (MilterReplyEncoder *encoder,
                                      const gchar **packet,
                                      gsize *packet_size)
{
    MilterEncoder *base_encoder;
    GString *buffer;

    base_encoder = MILTER_ENCODER(encoder);
    milter_encoder_clear_buffer(base_encoder);
    buffer = milter_encoder_get_buffer(base_encoder);

    g_string_append_c(buffer, MILTER_REPLY_SHUTDOWN);
    milter_encoder_pack(base_encoder, packet, packet_size);
}

void
milter_reply_encoder_encode_skip (MilterReplyEncoder *encoder,
                                  const gchar **packet,
                                  gsize *packet_size)
{
    MilterEncoder *base_encoder;
    GString *buffer;

    base_encoder = MILTER_ENCODER(encoder);
    milter_encoder_clear_buffer(base_encoder);
    buffer = milter_encoder_get_buffer(base_encoder);

    g_string_append_c(buffer, MILTER_REPLY_SKIP);
    milter_encoder_pack(base_encoder, packet, packet_size);
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
