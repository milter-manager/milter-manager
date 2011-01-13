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

#ifndef __MILTER_REPLY_ENCODER_H__
#define __MILTER_REPLY_ENCODER_H__

#include <sys/types.h>
#include <sys/socket.h>

#include <glib-object.h>

#include <milter/core/milter-encoder.h>
#include <milter/core/milter-protocol.h>
#include <milter/core/milter-option.h>
#include <milter/core/milter-macros-requests.h>

G_BEGIN_DECLS

#define MILTER_TYPE_REPLY_ENCODER            (milter_reply_encoder_get_type())
#define MILTER_REPLY_ENCODER(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), MILTER_TYPE_REPLY_ENCODER, MilterReplyEncoder))
#define MILTER_REPLY_ENCODER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), MILTER_TYPE_REPLY_ENCODER, MilterReplyEncoderClass))
#define MILTER_IS_REPLY_ENCODER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), MILTER_TYPE_REPLY_ENCODER))
#define MILTER_IS_REPLY_ENCODER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), MILTER_TYPE_REPLY_ENCODER))
#define MILTER_REPLY_ENCODER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), MILTER_TYPE_REPLY_ENCODER, MilterReplyEncoderClass))

typedef struct _MilterReplyEncoder         MilterReplyEncoder;
typedef struct _MilterReplyEncoderClass    MilterReplyEncoderClass;

struct _MilterReplyEncoder
{
    MilterEncoder object;
};

struct _MilterReplyEncoderClass
{
    MilterEncoderClass parent_class;
};

GType            milter_reply_encoder_get_type (void) G_GNUC_CONST;

MilterEncoder   *milter_reply_encoder_new      (void);

void             milter_reply_encoder_encode_negotiate
                                               (MilterReplyEncoder *encoder,
                                                const gchar       **packet,
                                                gsize              *packet_size,
                                                MilterOption       *option,
                                                MilterMacrosRequests *macros_requests);
void             milter_reply_encoder_encode_continue
                                               (MilterReplyEncoder *encoder,
                                                const gchar       **packet,
                                                gsize              *packet_size);
void             milter_reply_encoder_encode_reply_code
                                               (MilterReplyEncoder *encoder,
                                                const gchar       **packet,
                                                gsize              *packet_size,
                                                const gchar        *code);
void             milter_reply_encoder_encode_temporary_failure
                                               (MilterReplyEncoder *encoder,
                                                const gchar       **packet,
                                                gsize              *packet_size);
void             milter_reply_encoder_encode_reject
                                               (MilterReplyEncoder *encoder,
                                                const gchar       **packet,
                                                gsize              *packet_size);
void             milter_reply_encoder_encode_accept
                                               (MilterReplyEncoder *encoder,
                                                const gchar       **packet,
                                                gsize              *packet_size);
void             milter_reply_encoder_encode_discard
                                               (MilterReplyEncoder *encoder,
                                                const gchar       **packet,
                                                gsize              *packet_size);
void             milter_reply_encoder_encode_add_header
                                               (MilterReplyEncoder *encoder,
                                                const gchar       **packet,
                                                gsize              *packet_size,
                                                const gchar        *name,
                                                const gchar        *value);
void             milter_reply_encoder_encode_insert_header
                                               (MilterReplyEncoder *encoder,
                                                const gchar       **packet,
                                                gsize              *packet_size,
                                                guint32             index,
                                                const gchar        *name,
                                                const gchar        *value);
void             milter_reply_encoder_encode_change_header
                                               (MilterReplyEncoder *encoder,
                                                const gchar       **packet,
                                                gsize              *packet_size,
                                                const gchar        *name,
                                                guint32             index,
                                                const gchar        *value);
void             milter_reply_encoder_encode_delete_header
                                               (MilterReplyEncoder *encoder,
                                                const gchar       **packet,
                                                gsize              *packet_size,
                                                const gchar        *name,
                                                guint32             index);
void             milter_reply_encoder_encode_change_from
                                               (MilterReplyEncoder *encoder,
                                                const gchar       **packet,
                                                gsize              *packet_size,
                                                const gchar        *from,
                                                const gchar        *parameters);
void             milter_reply_encoder_encode_add_recipient
                                               (MilterReplyEncoder *encoder,
                                                const gchar       **packet,
                                                gsize              *packet_size,
                                                const gchar        *recipient,
                                                const gchar        *parameters);
void             milter_reply_encoder_encode_delete_recipient
                                               (MilterReplyEncoder *encoder,
                                                const gchar       **packet,
                                                gsize              *packet_size,
                                                const gchar        *recipient);
void             milter_reply_encoder_encode_replace_body
                                               (MilterReplyEncoder *encoder,
                                                const gchar       **packet,
                                                gsize              *packet_size,
                                                const gchar        *body,
                                                gsize               body_size,
                                                gsize              *packed_size);
void             milter_reply_encoder_encode_progress
                                               (MilterReplyEncoder *encoder,
                                                const gchar       **packet,
                                                gsize              *packet_size);
void             milter_reply_encoder_encode_quarantine
                                               (MilterReplyEncoder *encoder,
                                                const gchar       **packet,
                                                gsize              *packet_size,
                                                const gchar        *reason);
void             milter_reply_encoder_encode_connection_failure
                                               (MilterReplyEncoder *encoder,
                                                const gchar       **packet,
                                                gsize              *packet_size);
void             milter_reply_encoder_encode_shutdown
                                               (MilterReplyEncoder *encoder,
                                                const gchar       **packet,
                                                gsize              *packet_size);
void             milter_reply_encoder_encode_skip
                                               (MilterReplyEncoder *encoder,
                                                const gchar       **packet,
                                                gsize              *packet_size);

G_END_DECLS

#endif /* __MILTER_REPLY_ENCODER_H__ */

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
