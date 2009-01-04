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

#include "milter-manager-reply-decoder.h"
#include "milter-manager-enum-types.h"

enum
{
    SUCCESS,
    FAILURE,
    ERROR,
    LAST_SIGNAL
};

static gint signals[LAST_SIGNAL] = {0};

G_DEFINE_TYPE(MilterManagerReplyDecoder,
              milter_manager_reply_decoder,
              MILTER_TYPE_DECODER);

static void dispose        (GObject         *object);

static gboolean decode     (MilterDecoder *decoder,
                            GError       **error);

static void
milter_manager_reply_decoder_class_init (MilterManagerReplyDecoderClass *klass)
{
    GObjectClass *gobject_class;
    MilterDecoderClass *decoder_class;

    gobject_class = G_OBJECT_CLASS(klass);
    decoder_class = MILTER_DECODER_CLASS(klass);

    gobject_class->dispose      = dispose;

    decoder_class->decode = decode;

    signals[SUCCESS] =
        g_signal_new("success",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterManagerReplyDecoderClass,
                                     success),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__VOID,
                     G_TYPE_NONE, 0);

    signals[FAILURE] =
        g_signal_new("failure",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterManagerReplyDecoderClass,
                                     failure),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__STRING,
                     G_TYPE_NONE, 1, G_TYPE_STRING);

    signals[ERROR] =
        g_signal_new("error",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterManagerReplyDecoderClass,
                                     error),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__STRING,
                     G_TYPE_NONE, 1, G_TYPE_STRING);
}

static void
milter_manager_reply_decoder_init (MilterManagerReplyDecoder *decoder)
{
}

static void
dispose (GObject *object)
{
    G_OBJECT_CLASS(milter_manager_reply_decoder_parent_class)->dispose(object);
}

GQuark
milter_manager_reply_decoder_error_quark (void)
{
    return g_quark_from_static_string("milter-manager-reply-decoder-error-quark");
}

MilterDecoder *
milter_manager_reply_decoder_new (void)
{
    return g_object_new(MILTER_TYPE_MANAGER_REPLY_DECODER,
                        NULL);
}

static gboolean
decode_success (MilterDecoder *decoder,
                const gchar *content, gint32 length,
                GError **error)
{
    if (!milter_decoder_check_command_length(
            content, length, 0,
            MILTER_DECODER_COMPARE_EXACT, error, "SUCCESS reply"))
        return FALSE;

    g_signal_emit(decoder, signals[SUCCESS], 0);
    return TRUE;
}

static gboolean
decode_failure (MilterDecoder *decoder,
                const gchar *content, gint32 length,
                GError **error)
{
    g_signal_emit(decoder, signals[FAILURE], 0, content);
    return TRUE;
}

static gboolean
decode_error (MilterDecoder *decoder,
              const gchar *content, gint32 length,
              GError **error)
{
    g_signal_emit(decoder, signals[ERROR], 0, content);
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

    content = buffer + null_character_point + 1;
    content_length = length - null_character_point - 1;
    if (g_str_equal(buffer, MILTER_MANAGER_REPLY_SUCCESS)) {
        success = decode_success(decoder, content, content_length, error);
    } else if (g_str_equal(buffer, MILTER_MANAGER_REPLY_FAILURE)) {
        success = decode_failure(decoder, content, content_length, error);
    } else if (g_str_equal(buffer, MILTER_MANAGER_REPLY_ERROR)) {
        success = decode_error(decoder, content, content_length, error);
    } else {
        g_set_error(error,
                    MILTER_MANAGER_REPLY_DECODER_ERROR,
                    MILTER_MANAGER_REPLY_DECODER_ERROR_UNEXPECTED_REPLY,
                    "unexpected reply was received: <%s>", buffer);
        success = FALSE;
    }

    return success;
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
