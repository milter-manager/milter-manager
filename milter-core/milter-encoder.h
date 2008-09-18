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

#ifndef __MILTER_ENCODER_H__
#define __MILTER_ENCODER_H__

#include <sys/types.h>
#include <sys/socket.h>

#include <glib-object.h>

#include <milter-core/milter-protocol.h>
#include <milter-core/milter-option.h>

G_BEGIN_DECLS

#define MILTER_TYPE_ENCODER            (milter_encoder_get_type())
#define MILTER_ENCODER(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), MILTER_TYPE_ENCODER, MilterEncoder))
#define MILTER_ENCODER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), MILTER_TYPE_ENCODER, MilterEncoderClass))
#define MILTER_IS_ENCODER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), MILTER_TYPE_ENCODER))
#define MILTER_IS_ENCODER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), MILTER_TYPE_ENCODER))
#define MILTER_ENCODER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), MILTER_TYPE_ENCODER, MilterEncoderClass))

typedef struct _MilterEncoder         MilterEncoder;
typedef struct _MilterEncoderClass    MilterEncoderClass;

struct _MilterEncoder
{
    GObject object;
};

struct _MilterEncoderClass
{
    GObjectClass parent_class;
};

GType            milter_encoder_get_type       (void) G_GNUC_CONST;

MilterEncoder   *milter_encoder_new            (void);

void             milter_encoder_encode_option_negotiation
                                               (MilterEncoder     *encoder,
                                                gchar            **packet,
                                                gsize             *packet_size,
                                                MilterOption      *option);
void             milter_encoder_encode_define_macro
                                               (MilterEncoder     *encoder,
                                                gchar            **packet,
                                                gsize             *packet_size,
                                                MilterCommand      context,
                                                GHashTable        *macros);
void             milter_encoder_encode_connect (MilterEncoder     *encoder,
                                                gchar            **packet,
                                                gsize             *packet_size,
                                                const gchar       *host_name,
                                                const struct sockaddr *address,
                                                socklen_t         address_size);
void             milter_encoder_encode_helo    (MilterEncoder     *encoder,
                                                gchar            **packet,
                                                gsize             *packet_size,
                                                const gchar       *fqdn);
void             milter_encoder_encode_mail    (MilterEncoder     *encoder,
                                                gchar            **packet,
                                                gsize             *packet_size,
                                                const gchar       *from);
void             milter_encoder_encode_rcpt    (MilterEncoder     *encoder,
                                                gchar            **packet,
                                                gsize             *packet_size,
                                                const gchar       *to);
void             milter_encoder_encode_header  (MilterEncoder     *encoder,
                                                gchar            **packet,
                                                gsize             *packet_size,
                                                const gchar       *name,
                                                const gchar       *value);
void             milter_encoder_encode_end_of_header
                                               (MilterEncoder     *encoder,
                                                gchar            **packet,
                                                gsize             *packet_size);
void             milter_encoder_encode_body    (MilterEncoder     *encoder,
                                                gchar            **packet,
                                                gsize             *packet_size,
                                                const gchar       *chunk,
                                                gsize              size);
void             milter_encoder_encode_end_of_message
                                               (MilterEncoder     *encoder,
                                                gchar            **packet,
                                                gsize             *packet_size,
                                                const gchar       *chunk,
                                                gsize              size);
void             milter_encoder_encode_abort   (MilterEncoder     *encoder,
                                                gchar            **packet,
                                                gsize             *packet_size);
void             milter_encoder_encode_quit    (MilterEncoder     *encoder,
                                                gchar            **packet,
                                                gsize             *packet_size);
void             milter_encoder_encode_unknown (MilterEncoder     *encoder,
                                                gchar            **packet,
                                                gsize             *packet_size,
                                                const gchar       *command);

G_END_DECLS

#endif /* __MILTER_ENCODER_H__ */

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
