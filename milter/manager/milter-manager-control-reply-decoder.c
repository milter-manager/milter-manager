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
 *  along with this library.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifdef HAVE_CONFIG_H
#  include "../../config.h"
#endif /* HAVE_CONFIG_H */

#include <milter/core/milter-marshalers.h>

#include "milter-manager-control-reply-decoder.h"
#include "milter-manager-enum-types.h"

enum
{
    CONFIGURATION,
    LAST_SIGNAL
};

static gint signals[LAST_SIGNAL] = {0};

G_DEFINE_TYPE(MilterManagerControlReplyDecoder,
              milter_manager_control_reply_decoder,
              MILTER_TYPE_MANAGER_REPLY_DECODER);

static void dispose        (GObject         *object);

static gboolean decode     (MilterDecoder *decoder,
                            GError       **error);

static void
milter_manager_control_reply_decoder_class_init (MilterManagerControlReplyDecoderClass *klass)
{
    GObjectClass *gobject_class;
    MilterDecoderClass *decoder_class;

    gobject_class = G_OBJECT_CLASS(klass);
    decoder_class = MILTER_DECODER_CLASS(klass);

    gobject_class->dispose      = dispose;

    decoder_class->decode = decode;

    signals[CONFIGURATION] =
        g_signal_new("configuration",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterManagerControlReplyDecoderClass,
                                     configuration),
                     NULL, NULL,
 #if GLIB_SIZEOF_SIZE_T == 8
                     _milter_marshal_VOID__STRING_UINT64,
                     G_TYPE_NONE, 2, G_TYPE_STRING, G_TYPE_UINT64
#else
                     _milter_marshal_VOID__STRING_UINT,
                     G_TYPE_NONE, 2, G_TYPE_STRING, G_TYPE_UINT
#endif
            );
}

static void
milter_manager_control_reply_decoder_init (MilterManagerControlReplyDecoder *decoder)
{
}

static void
dispose (GObject *object)
{
    G_OBJECT_CLASS(milter_manager_control_reply_decoder_parent_class)->dispose(object);
}

GQuark
milter_manager_control_reply_decoder_error_quark (void)
{
    return g_quark_from_static_string("milter-manager-control-reply-decoder-error-quark");
}

MilterDecoder *
milter_manager_control_reply_decoder_new (void)
{
    return g_object_new(MILTER_TYPE_MANAGER_CONTROL_REPLY_DECODER,
                        NULL);
}

static gboolean
decode_configuration (MilterDecoder *decoder,
                      const gchar *content, gint32 length,
                      GError **error)
{
    g_signal_emit(decoder, signals[CONFIGURATION], 0, content, length);
    return TRUE;
}

static gboolean
decode (MilterDecoder *decoder, GError **error)
{
    gboolean success = TRUE;
    gint null_character_point;
    const gchar *buffer;
    gint32 length;
    const gchar *content;
    gint32 content_length;

    buffer = milter_decoder_get_buffer(decoder);
    length = milter_decoder_get_command_length(decoder);
    null_character_point =
        milter_decoder_decode_null_terminated_value(
            buffer, length, error,
            "control command isn't terminated by NULL");
    if (null_character_point <= 0)
        return FALSE;

    if (MILTER_DECODER_CLASS(milter_manager_control_reply_decoder_parent_class)->decode(decoder, error))
        return TRUE;

    g_clear_error(error);

    content = buffer + null_character_point + 1;
    content_length = length - null_character_point - 1;
    if (g_str_equal(buffer, MILTER_MANAGER_CONTROL_REPLY_CONFIGURATION)) {
        success = decode_configuration(decoder, content, content_length, error);
    } else {
        g_set_error(error,
                    MILTER_MANAGER_CONTROL_REPLY_DECODER_ERROR,
                    MILTER_MANAGER_CONTROL_REPLY_DECODER_ERROR_UNEXPECTED_REPLY,
                    "unexpected reply was received: <%s>", buffer);
        success = FALSE;
    }

    return success;
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
