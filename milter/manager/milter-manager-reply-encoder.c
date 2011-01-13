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

#include "milter-manager-reply-encoder.h"

G_DEFINE_TYPE(MilterManagerReplyEncoder,
              milter_manager_reply_encoder,
              MILTER_TYPE_ENCODER);

static void dispose        (GObject         *object);

static void
milter_manager_reply_encoder_class_init (MilterManagerReplyEncoderClass *klass)
{
    GObjectClass *gobject_class;

    gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->dispose      = dispose;
}

static void
milter_manager_reply_encoder_init (MilterManagerReplyEncoder *encoder)
{
}

static void
dispose (GObject *object)
{
    G_OBJECT_CLASS(milter_manager_reply_encoder_parent_class)->dispose(object);
}

MilterEncoder *
milter_manager_reply_encoder_new (void)
{
    return g_object_new(MILTER_TYPE_MANAGER_REPLY_ENCODER, NULL);
}

void
milter_manager_reply_encoder_encode_success (MilterManagerReplyEncoder *encoder,
                                             const gchar **packet,
                                             gsize *packet_size)
{
    MilterEncoder *base_encoder;
    GString *buffer;

    base_encoder = MILTER_ENCODER(encoder);
    milter_encoder_clear_buffer(base_encoder);
    buffer = milter_encoder_get_buffer(base_encoder);

    g_string_append(buffer, MILTER_MANAGER_REPLY_SUCCESS);
    g_string_append_c(buffer, '\0');
    milter_encoder_pack(base_encoder, packet, packet_size);
}

void
milter_manager_reply_encoder_encode_failure (MilterManagerReplyEncoder *encoder,
                                             const gchar **packet,
                                             gsize *packet_size,
                                             const gchar *message)
{
    MilterEncoder *base_encoder;
    GString *buffer;

    base_encoder = MILTER_ENCODER(encoder);
    milter_encoder_clear_buffer(base_encoder);
    buffer = milter_encoder_get_buffer(base_encoder);

    g_string_append(buffer, MILTER_MANAGER_REPLY_FAILURE);
    g_string_append_c(buffer, '\0');
    g_string_append(buffer, message);
    milter_encoder_pack(base_encoder, packet, packet_size);
}

void
milter_manager_reply_encoder_encode_error (MilterManagerReplyEncoder *encoder,
                                           const gchar **packet,
                                           gsize *packet_size,
                                           const gchar *message)
{
    MilterEncoder *base_encoder;
    GString *buffer;

    base_encoder = MILTER_ENCODER(encoder);
    milter_encoder_clear_buffer(base_encoder);
    buffer = milter_encoder_get_buffer(base_encoder);

    g_string_append(buffer, MILTER_MANAGER_REPLY_ERROR);
    g_string_append_c(buffer, '\0');
    g_string_append(buffer, message);
    milter_encoder_pack(base_encoder, packet, packet_size);
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
