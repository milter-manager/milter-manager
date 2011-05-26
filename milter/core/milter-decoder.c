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
#  include <config.h>
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>

#include <glib.h>

#include "milter-logger.h"
#include "milter-decoder.h"
#include "milter-enum-types.h"
#include "milter-marshalers.h"

#define COMMAND_LENGTH_BYTES (sizeof(guint32))

#define MILTER_DECODER_GET_PRIVATE(obj)                 \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj),                 \
                                 MILTER_TYPE_DECODER,   \
                                 MilterDecoderPrivate))

typedef struct _MilterDecoderPrivate	MilterDecoderPrivate;
struct _MilterDecoderPrivate
{
    gint state;
    GString *buffer;
    gint32 command_length;
    guint tag;
};

enum
{
    PROP_0,
    PROP_TAG
};

typedef enum {
    IN_START,
    IN_COMMAND_LENGTH,
    IN_COMMAND_CONTENT,
    IN_ERROR
} DecodeState;

enum
{
    DECODE,
    LAST_SIGNAL
};

static gint signals[LAST_SIGNAL] = {0};

G_DEFINE_ABSTRACT_TYPE(MilterDecoder, milter_decoder, G_TYPE_OBJECT);

static void dispose        (GObject         *object);
static void set_property   (GObject         *object,
                            guint            prop_id,
                            const GValue    *value,
                            GParamSpec      *pspec);
static void get_property   (GObject         *object,
                            guint            prop_id,
                            GValue          *value,
                            GParamSpec      *pspec);

static void
milter_decoder_class_init (MilterDecoderClass *klass)
{
    GObjectClass *gobject_class;
    GParamSpec *spec;

    gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->dispose      = dispose;
    gobject_class->set_property = set_property;
    gobject_class->get_property = get_property;

    spec = g_param_spec_uint("tag",
                             "Tag",
                             "The tag of the decoder",
                             0, G_MAXUINT, 0,
                             G_PARAM_READABLE);
    g_object_class_install_property(gobject_class, PROP_TAG, spec);

    signals[DECODE] =
        g_signal_new("decode",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterDecoderClass, decode),
                     NULL, NULL,
                     _milter_marshal_BOOLEAN__POINTER,
                     G_TYPE_BOOLEAN, 1, G_TYPE_POINTER);

    g_type_class_add_private(gobject_class, sizeof(MilterDecoderPrivate));
}

static void
milter_decoder_init (MilterDecoder *decoder)
{
    MilterDecoderPrivate *priv = MILTER_DECODER_GET_PRIVATE(decoder);

    priv->state = IN_START;
    priv->buffer = g_string_new(NULL);
    priv->tag = 0;
}

static void
dispose (GObject *object)
{
    MilterDecoderPrivate *priv;

    priv = MILTER_DECODER_GET_PRIVATE(object);
    if (priv->buffer) {
        g_string_free(priv->buffer, TRUE);
        priv->buffer = NULL;
    }

    G_OBJECT_CLASS(milter_decoder_parent_class)->dispose(object);
}

static void
set_property (GObject      *object,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
    switch (prop_id) {
    case PROP_TAG:
        milter_decoder_set_tag(MILTER_DECODER(object), g_value_get_uint(value));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static void
get_property (GObject    *object,
              guint       prop_id,
              GValue     *value,
              GParamSpec *pspec)
{
    MilterDecoderPrivate *priv;

    priv = MILTER_DECODER_GET_PRIVATE(object);
    switch (prop_id) {
    case PROP_TAG:
        g_value_set_uint(value, priv->tag);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

GQuark
milter_decoder_error_quark (void)
{
    return g_quark_from_static_string("milter-decoder-error-quark");
}

#define set_missing_null_error(error, message, ...)     \
    g_set_error(error,                                  \
                MILTER_DECODER_ERROR,                    \
                MILTER_DECODER_ERROR_MISSING_NULL,       \
                message, ## __VA_ARGS__)

static gint
find_null_character (const gchar *buffer, gint length)
{
    gint i;

    for (i = 0; i < length; i++) {
        if (buffer[i] == '\0')
            return i;
    }

    return -1;
}

gint
milter_decoder_decode_null_terminated_value (const gchar *buffer,
                                             gint length,
                                             GError **error,
                                             const gchar *message)
{
    gint null_character_point;

    null_character_point = find_null_character(buffer, length);
    if (null_character_point < 0) {
        gchar *terminated_value;

        terminated_value = g_strndup(buffer, length + 1);
        terminated_value[length + 1] = '\0';
        set_missing_null_error(error, "%s: <%s>", message, terminated_value);
        g_free(terminated_value);
    }

    return null_character_point;
}

static void
append_inspected_bytes (GString *message, const gchar *buffer, gsize length)
{
    gsize i;
    gboolean have_unprintable_byte = FALSE;

    if (length == 0)
        return;

    g_string_append(message, ":");
    for (i = 0; i < length; i++) {
        g_string_append_printf(message, " 0x%02x", buffer[i]);
        if (buffer[i] < 0x20 || 0x7e < buffer[i])
            have_unprintable_byte = TRUE;
    }
    if (!have_unprintable_byte)
        g_string_append_printf(message, " (%s)", buffer);
}

static void
append_need_more_bytes_for_decoding_message (GString *message,
                                             const gchar *buffer,
                                             gsize length, gsize required_length,
                                             const gchar *decoding_target)
{
    gsize needed_length;

    needed_length = required_length - length;
    g_string_append_printf(message,
                           "need more %" G_GSIZE_FORMAT " byte%s "
                           "for decoding %s",
                           needed_length,
                           needed_length > 1 ? "s" : "",
                           decoding_target);
    append_inspected_bytes(message, buffer, length);
}

static void
append_needless_bytes_were_received_message (GString *message,
                                             const gchar *buffer,
                                             gsize needless_bytes,
                                             const gchar *on_target)
{
    g_return_if_fail(needless_bytes > 0);
    g_string_append_printf(message,
                           "needless %" G_GSIZE_FORMAT " byte%s received "
                           "on %s",
                           needless_bytes,
                           needless_bytes > 1 ? "s were" : " was",
                           on_target);
    append_inspected_bytes(message, buffer, needless_bytes);
}

gboolean
milter_decoder_check_command_length (const gchar *buffer, gint length,
                                     gint expected_length,
                                     MilterDecoderCompareType compare_type,
                                     GError **error, const gchar *target)
{
    GString *message;

    switch (compare_type) {
      case MILTER_DECODER_COMPARE_AT_LEAST:
        if (length >= expected_length)
            return TRUE;
        break;
      case MILTER_DECODER_COMPARE_EXACT:
        if (length == expected_length)
            return TRUE;
        break;
    }

    message = g_string_new(NULL);
    if (length > expected_length) {
        append_needless_bytes_were_received_message(message,
                                                    buffer,
                                                    length - expected_length,
                                                    target);
        g_set_error(error,
                    MILTER_DECODER_ERROR,
                    MILTER_DECODER_ERROR_LONG_COMMAND_LENGTH,
                    "%s", message->str);
    } else {
        append_need_more_bytes_for_decoding_message(message,
                                                    buffer, length,
                                                    expected_length,
                                                    target);
        g_set_error(error,
                    MILTER_DECODER_ERROR,
                    MILTER_DECODER_ERROR_SHORT_COMMAND_LENGTH,
                    "%s", message->str);
    }
    g_string_free(message, TRUE);

    return FALSE;
}

gboolean
milter_decoder_decode_header_content (const gchar *buffer, gint length,
                                      const gchar **name, const gchar **value,
                                      GError **error)
{
    gint null_character_point;
    gchar *error_message;

    null_character_point =
        milter_decoder_decode_null_terminated_value(buffer, length, error,
                                                    "name isn't terminated by NULL "
                                                    "on header");
    if (null_character_point <= 0)
        return FALSE;
    *name = buffer;

    buffer += (null_character_point + 1);
    length -= (null_character_point + 1);
    error_message = g_strdup_printf("value isn't terminated by NULL "
                                    "on header: <%s>", *name);
    null_character_point =
        milter_decoder_decode_null_terminated_value(buffer, length, error, error_message);
    g_free(error_message);
    if (null_character_point < 0)
        return FALSE;
    *value = buffer;

    return TRUE;
}

gboolean
milter_decoder_decode (MilterDecoder *decoder, const gchar *chunk, gsize size,
                       GError **error)
{
    MilterDecoderPrivate *priv;
    gboolean loop = TRUE;
    gboolean success = TRUE;

    priv = MILTER_DECODER_GET_PRIVATE(decoder);

    g_return_val_if_fail(priv->state != MILTER_DECODER_ERROR, FALSE);

    if (size == 0)
        return TRUE;

    milter_trace("[%u] [decoder][decode] "
                 "<%" G_GSIZE_FORMAT "> "
                 "(%" G_GSIZE_FORMAT ")",
                 priv->tag, size,
                 priv->buffer->len);
    g_string_append_len(priv->buffer, chunk, size);
    while (loop) {
        switch (priv->state) {
        case IN_START:
            milter_trace("[%u] [decoder][decode][start]", priv->tag);
            if (priv->buffer->len == 0) {
                loop = FALSE;
            } else {
                priv->state = IN_COMMAND_LENGTH;
            }
            break;
        case IN_COMMAND_LENGTH:
            if (priv->buffer->len < COMMAND_LENGTH_BYTES) {
                milter_trace("[%u] [decoder][decode][length][need-more]",
                             priv->tag);
                loop = FALSE;
            } else {
                memcpy(&priv->command_length,
                       priv->buffer->str,
                       COMMAND_LENGTH_BYTES);
                priv->command_length = g_ntohl(priv->command_length);
                milter_trace("[%u] [decoder][decode][length] <%d>",
                             priv->tag, priv->command_length);
                g_string_erase(priv->buffer, 0, COMMAND_LENGTH_BYTES);
                priv->state = IN_COMMAND_CONTENT;
            }
            break;
        case IN_COMMAND_CONTENT:
            if (priv->buffer->len < priv->command_length) {
                milter_trace("[%u] [decoder][decode][content][need-more] "
                             "<%" G_GSIZE_FORMAT ">/<%d>",
                             priv->tag,
                             priv->buffer->len, priv->command_length);
                loop = FALSE;
            } else {
                milter_trace("[%u] [decoder][decode][content][fill] "
                             "<%d> (%" G_GSIZE_FORMAT ")",
                             priv->tag, priv->command_length, priv->buffer->len);
                g_signal_emit(decoder, signals[DECODE], 0, error, &success);
                if (success) {
                    priv->state = IN_START;
                    g_string_erase(priv->buffer, 0, priv->command_length);
                } else {
                    priv->state = IN_ERROR;
                    loop = FALSE;
                }
            }
            break;
        case IN_ERROR:
            milter_error("[%u] [decoder][decode][error] "
                         "<%d> (%" G_GSIZE_FORMAT ")",
                         priv->tag, priv->command_length, priv->buffer->len);
            loop = FALSE;
            break;
        }
    }

    return success;
}

static void
set_unexpected_end_error (GError **error, MilterDecoderPrivate *priv,
                          gsize required_length, const gchar *decoding_target)
{
    GString *message;

    message = g_string_new("stream is ended unexpectedly: ");
    append_need_more_bytes_for_decoding_message(message,
                                                priv->buffer->str,
                                                priv->buffer->len,
                                                required_length,
                                                decoding_target);
    g_set_error(error,
                MILTER_DECODER_ERROR,
                MILTER_DECODER_ERROR_UNEXPECTED_END,
                "%s", message->str);
    g_string_free(message, TRUE);

    priv->state = IN_ERROR;
}

gboolean
milter_decoder_end_decode (MilterDecoder *decoder, GError **error)
{
    MilterDecoderPrivate *priv;

    priv = MILTER_DECODER_GET_PRIVATE(decoder);

    g_return_val_if_fail(priv->state != IN_ERROR, FALSE);

    switch (priv->state) {
      case IN_START:
        break;
      case IN_COMMAND_LENGTH:
        set_unexpected_end_error(error, priv,
                                 COMMAND_LENGTH_BYTES, "command length");
        break;
      case IN_COMMAND_CONTENT:
        set_unexpected_end_error(error, priv,
                                 priv->command_length, "command content");
        break;
      case IN_ERROR:
        break;
    }

    return priv->state != IN_ERROR;
}

const gchar *
milter_decoder_get_buffer (MilterDecoder *decoder)
{
    return MILTER_DECODER_GET_PRIVATE(decoder)->buffer->str;
}

gint32
milter_decoder_get_command_length (MilterDecoder *decoder)
{
    return MILTER_DECODER_GET_PRIVATE(decoder)->command_length;
}

MilterOption *
milter_decoder_decode_negotiate (const gchar *buffer,
                                 gint length,
                                 gint *processed_length,
                                 GError **error)
{
    gsize i;
    guint32 version, action, step;

    *processed_length = 0;

    i = 1;
    if (!milter_decoder_check_command_length(
            buffer + i, length - i, sizeof(version),
            MILTER_DECODER_COMPARE_AT_LEAST, error,
            "version on option negotiation command")) {
        return NULL;
    }
    memcpy(&version, buffer + i, sizeof(version));
    i += sizeof(version);

    if (!milter_decoder_check_command_length(
            buffer + i, length - i, sizeof(action),
            MILTER_DECODER_COMPARE_AT_LEAST, error,
            "action flags on option negotiation command")) {
        return NULL;
    }
    memcpy(&action, buffer + i, sizeof(action));
    i += sizeof(action);

    if (!milter_decoder_check_command_length(
            buffer + i, length - i, sizeof(step),
            MILTER_DECODER_COMPARE_AT_LEAST, error,
            "step flags on option negotiation command")) {
        return NULL;
    }
    memcpy(&step, buffer + i, sizeof(step));
    i += sizeof(step);

    *processed_length = i;

    return milter_option_new(g_ntohl(version), g_ntohl(action), g_ntohl(step));
}

guint
milter_decoder_get_tag (MilterDecoder *decoder)
{
    return MILTER_DECODER_GET_PRIVATE(decoder)->tag;
}

void
milter_decoder_set_tag (MilterDecoder *decoder, guint tag)
{
    MILTER_DECODER_GET_PRIVATE(decoder)->tag = tag;
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
