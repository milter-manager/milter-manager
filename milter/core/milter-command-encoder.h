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

#ifndef __MILTER_COMMAND_ENCODER_H__
#define __MILTER_COMMAND_ENCODER_H__

#include <sys/types.h>
#include <sys/socket.h>

#include <glib-object.h>

#include <milter/core/milter-encoder.h>
#include <milter/core/milter-protocol.h>
#include <milter/core/milter-option.h>
#include <milter/core/milter-macros-requests.h>

G_BEGIN_DECLS

#define MILTER_TYPE_COMMAND_ENCODER            (milter_command_encoder_get_type())
#define MILTER_COMMAND_ENCODER(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), MILTER_TYPE_COMMAND_ENCODER, MilterCommandEncoder))
#define MILTER_COMMAND_ENCODER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), MILTER_TYPE_COMMAND_ENCODER, MilterCommandEncoderClass))
#define MILTER_IS_COMMAND_ENCODER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), MILTER_TYPE_COMMAND_ENCODER))
#define MILTER_IS_COMMAND_ENCODER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), MILTER_TYPE_COMMAND_ENCODER))
#define MILTER_COMMAND_ENCODER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), MILTER_TYPE_COMMAND_ENCODER, MilterCommandEncoderClass))

typedef struct _MilterCommandEncoder         MilterCommandEncoder;
typedef struct _MilterCommandEncoderClass    MilterCommandEncoderClass;

struct _MilterCommandEncoder
{
    MilterEncoder object;
};

struct _MilterCommandEncoderClass
{
    MilterEncoderClass parent_class;
};

GType            milter_command_encoder_get_type       (void) G_GNUC_CONST;

MilterEncoder   *milter_command_encoder_new            (void);

void             milter_command_encoder_encode_negotiate
                                            (MilterCommandEncoder *encoder,
                                             const gchar         **packet,
                                             gsize                *packet_size,
                                             MilterOption         *option);
void             milter_command_encoder_encode_define_macro
                                            (MilterCommandEncoder *encoder,
                                             const gchar         **packet,
                                             gsize                *packet_size,
                                             MilterCommand         context,
                                             GHashTable           *macros);
void             milter_command_encoder_encode_connect
                                            (MilterCommandEncoder *encoder,
                                             const gchar         **packet,
                                             gsize                *packet_size,
                                             const gchar          *host_name,
                                             const struct sockaddr *address,
                                             socklen_t             address_size);
void             milter_command_encoder_encode_helo
                                            (MilterCommandEncoder *encoder,
                                             const gchar         **packet,
                                             gsize                *packet_size,
                                             const gchar          *fqdn);
void             milter_command_encoder_encode_envelope_from
                                            (MilterCommandEncoder *encoder,
                                             const gchar         **packet,
                                             gsize                *packet_size,
                                             const gchar          *from);
void             milter_command_encoder_encode_envelope_recipient
                                            (MilterCommandEncoder *encoder,
                                             const gchar         **packet,
                                             gsize                *packet_size,
                                             const gchar          *to);
void             milter_command_encoder_encode_data
                                            (MilterCommandEncoder *encoder,
                                             const gchar         **packet,
                                             gsize                *packet_size);
void             milter_command_encoder_encode_header
                                            (MilterCommandEncoder *encoder,
                                             const gchar         **packet,
                                             gsize                *packet_size,
                                             const gchar          *name,
                                             const gchar          *value);
void             milter_command_encoder_encode_end_of_header
                                            (MilterCommandEncoder *encoder,
                                             const gchar         **packet,
                                             gsize                *packet_size);
void             milter_command_encoder_encode_body
                                            (MilterCommandEncoder *encoder,
                                             const gchar         **packet,
                                             gsize                *packet_size,
                                             const gchar          *chunk,
                                             gsize                 size,
                                             gsize                *packed_size);
void             milter_command_encoder_encode_end_of_message
                                            (MilterCommandEncoder *encoder,
                                             const gchar         **packet,
                                             gsize                *packet_size,
                                             const gchar          *chunk,
                                             gsize                 size);
void             milter_command_encoder_encode_abort
                                            (MilterCommandEncoder *encoder,
                                             const gchar         **packet,
                                             gsize                *packet_size);
void             milter_command_encoder_encode_quit
                                            (MilterCommandEncoder *encoder,
                                             const gchar         **packet,
                                             gsize                *packet_size);
void             milter_command_encoder_encode_unknown
                                            (MilterCommandEncoder *encoder,
                                             const gchar         **packet,
                                             gsize                *packet_size,
                                             const gchar          *command);

G_END_DECLS

#endif /* __MILTER_COMMAND_ENCODER_H__ */

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
