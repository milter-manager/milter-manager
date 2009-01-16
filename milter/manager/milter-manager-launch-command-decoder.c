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

#include "milter-manager-launch-command-decoder.h"
#include "milter-manager-enum-types.h"

enum
{
    LAUNCH,
    LAST_SIGNAL
};

static gint signals[LAST_SIGNAL] = {0};

G_DEFINE_TYPE(MilterManagerLaunchCommandDecoder,
              milter_manager_launch_command_decoder,
              MILTER_TYPE_DECODER);

static void dispose        (GObject         *object);

static gboolean decode     (MilterDecoder *decoder,
                            GError       **error);

static void
milter_manager_launch_command_decoder_class_init (MilterManagerLaunchCommandDecoderClass *klass)
{
    GObjectClass *gobject_class;
    MilterDecoderClass *decoder_class;

    gobject_class = G_OBJECT_CLASS(klass);
    decoder_class = MILTER_DECODER_CLASS(klass);

    gobject_class->dispose      = dispose;

    decoder_class->decode = decode;

    signals[LAUNCH] =
        g_signal_new("launch",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterManagerLaunchCommandDecoderClass,
                                     launch),
                     NULL, NULL,
                     _milter_marshal_VOID__STRING_STRING,
                     G_TYPE_NONE, 2, G_TYPE_STRING, G_TYPE_STRING);
}

static void
milter_manager_launch_command_decoder_init (MilterManagerLaunchCommandDecoder *decoder)
{
}

static void
dispose (GObject *object)
{
    G_OBJECT_CLASS(milter_manager_launch_command_decoder_parent_class)->dispose(object);
}

GQuark
milter_manager_launch_command_decoder_error_quark (void)
{
    return g_quark_from_static_string("milter-manager-launch-reply-decoder-error-quark");
}

MilterDecoder *
milter_manager_launch_command_decoder_new (void)
{
    return g_object_new(MILTER_TYPE_MANAGER_LAUNCH_COMMAND_DECODER,
                        NULL);
}

static gboolean
decode_launch (MilterDecoder *decoder,
                    const gchar *content, gint length,
                    GError **error)
{
    gint null_character_point;
    const gchar *command_line;
    const gchar *user_name = NULL;

    null_character_point =
        milter_decoder_decode_null_terminated_value(content,
                                                    length,
                                                    error,
                                                    "Command line isn't terminated by NULL "
                                                    "on spawn-child command");
    if (null_character_point <= 0)
        return FALSE;

    command_line = content;
    if (null_character_point < length - 1)
        user_name = content + null_character_point + 1;

    milter_debug("[launch-command-decoder][launch] <%s>@<%s>",
                 command_line,
                 user_name ? user_name : "NULL");
    g_signal_emit(decoder, signals[LAUNCH], 0, command_line, user_name);

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
            "launch command isn't terminated by NULL");
    if (null_character_point <= 0)
        return FALSE;

    content = buffer + null_character_point + 1;
    content_length = length - null_character_point - 1;

    if (g_str_equal(buffer, MILTER_MANAGER_LAUNCH_COMMAND_LAUNCH)) {
        success = decode_launch(decoder, content, content_length,
                                     error);
    } else {
        g_set_error(error,
                    MILTER_MANAGER_LAUNCH_COMMAND_DECODER_ERROR,
                    MILTER_MANAGER_LAUNCH_COMMAND_DECODER_ERROR_UNEXPECTED_COMMAND,
                    "unexpected command was received: <%s>", buffer);
        success = FALSE;
    }

    return success;
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
