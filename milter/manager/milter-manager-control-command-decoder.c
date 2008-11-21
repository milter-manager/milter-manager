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

#include "milter-manager-control-command-decoder.h"
#include "milter-manager-enum-types.h"
#include "milter-manager-marshalers.h"

enum
{
    SET_CONFIGURATION,
    GET_CONFIGURATION,
    RELOAD,
    STOP_CHILD,
    GET_CHILDREN_INFO,
    LAST_SIGNAL
};

static gint signals[LAST_SIGNAL] = {0};

G_DEFINE_TYPE(MilterManagerControlCommandDecoder,
              milter_manager_control_command_decoder,
              MILTER_TYPE_DECODER);

static void dispose        (GObject         *object);

static gboolean decode_command  (MilterDecoder *decoder,
                                 gchar          command,
                                 GError       **error);

static void
milter_manager_control_command_decoder_class_init (MilterManagerControlCommandDecoderClass *klass)
{
    GObjectClass *gobject_class;
    MilterDecoderClass *decoder_class;

    gobject_class = G_OBJECT_CLASS(klass);
    decoder_class = MILTER_DECODER_CLASS(klass);

    gobject_class->dispose      = dispose;

    decoder_class->decode_command = decode_command;

    signals[SET_CONFIGURATION] =
        g_signal_new("set-configuration",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterManagerControlCommandDecoderClass,
                                     set_configuration),
                     NULL, NULL,
#if GLIB_SIZEOF_SIZE_T == 8
                     _milter_manager_marshal_VOID__STRING_UINT64,
                     G_TYPE_NONE, 2, G_TYPE_STRING, G_TYPE_UINT64
#else
                     _milter_manager_marshal_VOID__STRING_UINT,
                     G_TYPE_NONE, 2, G_TYPE_STRING, G_TYPE_UINT
#endif
            );

    signals[GET_CONFIGURATION] =
        g_signal_new("get-configuration",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterManagerControlCommandDecoderClass,
                                     get_configuration),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__VOID,
                     G_TYPE_NONE, 0);

    signals[RELOAD] =
        g_signal_new("reload",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterManagerControlCommandDecoderClass,
                                     reload),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__VOID,
                     G_TYPE_NONE, 0);

    signals[STOP_CHILD] =
        g_signal_new("stop-child",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterManagerControlCommandDecoderClass,
                                     stop_child),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__STRING,
                     G_TYPE_NONE, 1, G_TYPE_STRING);

    signals[GET_CHILDREN_INFO] =
        g_signal_new("get-children-info",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterManagerControlCommandDecoderClass,
                                     get_children_info),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__VOID,
                     G_TYPE_NONE, 0);

}

static void
milter_manager_control_command_decoder_init (MilterManagerControlCommandDecoder *decoder)
{
}

static void
dispose (GObject *object)
{
    G_OBJECT_CLASS(milter_manager_control_command_decoder_parent_class)->dispose(object);
}

GQuark
milter_manager_control_command_decoder_error_quark (void)
{
    return g_quark_from_static_string("milter-manager-control-reply-decoder-error-quark");
}

MilterDecoder *
milter_manager_control_command_decoder_new (void)
{
    return g_object_new(MILTER_TYPE_MANAGER_CONTROL_COMMAND_DECODER,
                        NULL);
}

static gboolean
decode_command_set_configuration (MilterDecoder *decoder, GError **error)
{
    const gchar *buffer;
    gint32 command_length;

    command_length = milter_decoder_get_command_length(decoder);
    buffer = milter_decoder_get_buffer(decoder);
    g_signal_emit(decoder, signals[SET_CONFIGURATION], 0,
                  buffer + 1, command_length - 1);
    return TRUE;
}

static gboolean
decode_command_get_configuration (MilterDecoder *decoder, GError **error)
{
    /* FIXME */
    return TRUE;
}

static gboolean
decode_command_reload (MilterDecoder *decoder, GError **error)
{
    const gchar *buffer;
    gint32 command_length;

    buffer = milter_decoder_get_buffer(decoder);
    command_length = milter_decoder_get_command_length(decoder);
    if (!milter_decoder_check_command_length(
            buffer + 1, command_length - 1, 0,
            MILTER_DECODER_COMPARE_EXACT, error,
            "reload command"))
        return FALSE;

    g_signal_emit(decoder, signals[RELOAD], 0);

    return TRUE;
}

static gboolean
decode_command (MilterDecoder *decoder, gchar command, GError **error)
{
    gboolean success = TRUE;

    switch (command) {
      case MILTER_MANAGER_CONTROL_COMMAND_SET_CONFIGURATION:
        success = decode_command_set_configuration(decoder, error);
        break;
      case MILTER_MANAGER_CONTROL_COMMAND_GET_CONFIGURATION:
        success = decode_command_get_configuration(decoder, error);
        break;
      case MILTER_MANAGER_CONTROL_COMMAND_RELOAD:
        success = decode_command_reload(decoder, error);
        break;
/*
      case MILTER_MANAGER_CONTROL_COMMAND_STOP_CHILD:
        success = decode_command_stop_child(decoder, error);
        break;
      case MILTER_MANAGER_CONTROL_COMMAND_GET_CHILDREN_INFO:
        success = decode_command_get_children_info(decoder, error);
        break;
*/
      default:
        g_set_error(error,
                    MILTER_MANAGER_CONTROL_COMMAND_DECODER_ERROR,
                    MILTER_MANAGER_CONTROL_COMMAND_DECODER_ERROR_UNEXPECTED_COMMAND,
                    "unexpected command was received: %c", command);
        success = FALSE;
        break;
    }

    return success;
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
