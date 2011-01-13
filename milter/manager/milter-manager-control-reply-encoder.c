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

#include "milter-manager-control-reply-encoder.h"

G_DEFINE_TYPE(MilterManagerControlReplyEncoder,
              milter_manager_control_reply_encoder,
              MILTER_TYPE_MANAGER_REPLY_ENCODER);

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
                                                     const gchar **packet,
                                                     gsize *packet_size)
{
    MilterManagerReplyEncoder *reply_encoder = MILTER_MANAGER_REPLY_ENCODER(encoder);

    milter_manager_reply_encoder_encode_success(reply_encoder, packet, packet_size);
}

void
milter_manager_control_reply_encoder_encode_failure (MilterManagerControlReplyEncoder *encoder,
                                                     const gchar **packet,
                                                     gsize *packet_size,
                                                     const gchar *message)
{
    MilterManagerReplyEncoder *reply_encoder = MILTER_MANAGER_REPLY_ENCODER(encoder);

    milter_manager_reply_encoder_encode_failure(reply_encoder, packet, packet_size, message);
}


void
milter_manager_control_reply_encoder_encode_error (MilterManagerControlReplyEncoder *encoder,
                                                   const gchar **packet,
                                                   gsize *packet_size,
                                                   const gchar *message)
{
    MilterManagerReplyEncoder *reply_encoder = MILTER_MANAGER_REPLY_ENCODER(encoder);

    milter_manager_reply_encoder_encode_error(reply_encoder, packet, packet_size, message);
}


void
milter_manager_control_reply_encoder_encode_configuration (
    MilterManagerControlReplyEncoder *encoder,
    const gchar **packet, gsize *packet_size,
    const gchar *configuration, gsize configuration_size)
{
    MilterEncoder *base_encoder;
    GString *buffer;

    base_encoder = MILTER_ENCODER(encoder);
    milter_encoder_clear_buffer(base_encoder);
    buffer = milter_encoder_get_buffer(base_encoder);

    g_string_append(buffer, MILTER_MANAGER_CONTROL_REPLY_CONFIGURATION);
    g_string_append_c(buffer, '\0');
    g_string_append_len(buffer, configuration, configuration_size);
    milter_encoder_pack(base_encoder, packet, packet_size);
}

void
milter_manager_control_reply_encoder_encode_status (
    MilterManagerControlReplyEncoder *encoder,
    const gchar **packet, gsize *packet_size,
    const gchar *status, gsize status_size)
{
    MilterEncoder *base_encoder;
    GString *buffer;

    base_encoder = MILTER_ENCODER(encoder);
    milter_encoder_clear_buffer(base_encoder);
    buffer = milter_encoder_get_buffer(base_encoder);

    g_string_append(buffer, MILTER_MANAGER_CONTROL_REPLY_STATUS);
    g_string_append_c(buffer, '\0');
    g_string_append_len(buffer, status, status_size);
    milter_encoder_pack(base_encoder, packet, packet_size);
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
