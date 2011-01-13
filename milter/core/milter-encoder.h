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

#ifndef __MILTER_ENCODER_H__
#define __MILTER_ENCODER_H__

#include <sys/types.h>
#include <sys/socket.h>

#include <glib-object.h>

#include <milter/core/milter-protocol.h>
#include <milter/core/milter-option.h>
#include <milter/core/milter-macros-requests.h>

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

GString         *milter_encoder_get_buffer     (MilterEncoder     *encoder);
void             milter_encoder_clear_buffer   (MilterEncoder     *encoder);
void             milter_encoder_pack           (MilterEncoder     *encoder,
                                                const gchar      **packet,
                                                gsize             *packet_size);
void             milter_encoder_encode_negotiate
                                               (MilterEncoder    *encoder,
                                                MilterOption     *option);

guint            milter_encoder_get_tag        (MilterEncoder *encoder);
void             milter_encoder_set_tag        (MilterEncoder *encoder,
                                                guint          tag);

G_END_DECLS

#endif /* __MILTER_ENCODER_H__ */

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
