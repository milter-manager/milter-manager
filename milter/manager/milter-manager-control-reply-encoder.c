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

#include "milter-manager-control-reply-encoder.h"

G_DEFINE_TYPE(MilterManagerControlReplyEncoder,
              milter_manager_control_reply_encoder,
              MILTER_TYPE_ENCODER);

static void dispose        (GObject         *object);

static void
milter_manager_control_reply_encoder_class_init (MilterManagerControlReplyEncoderClass *klass)
{
    GObjectClass *gobject_class;

    gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->dispose      = dispose;
}

static void
milter_manager_control_reply_encoder_init (MilterManagerControlReplyEncoder *encoder)
{
}

static void
dispose (GObject *object)
{
    G_OBJECT_CLASS(milter_manager_control_reply_encoder_parent_class)->dispose(object);
}

MilterEncoder *
milter_manager_control_reply_encoder_new (void)
{
    return g_object_new(MILTER_TYPE_MANAGER_CONTROL_REPLY_ENCODER, NULL);
}

void
milter_manager_control_reply_encoder_encode_success (MilterManagerControlReplyEncoder *encoder,
                                                     gchar **packet,
                                                     gsize *packet_size)
{
    MilterEncoder *_encoder;
    GString *buffer;

    _encoder = MILTER_ENCODER(encoder);
    buffer = milter_encoder_get_buffer(_encoder);

    g_string_append_c(buffer, MILTER_MANAGER_CONTROL_REPLY_SUCCESS);
    milter_encoder_pack(_encoder, packet, packet_size);
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
