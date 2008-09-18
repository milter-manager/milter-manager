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
#  include "../config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>

#include <glib.h>

#include "milter-decoder.h"
#include "milter-enum-types.h"
#include "milter-marshalers.h"

#define COMMAND_LENGTH_BYTES (4)
#define OPTION_BYTES (4)

#define COMMAND_ABORT              'A'     /* Abort */
#define COMMAND_BODY               'B'     /* Body chunk */
#define COMMAND_CONNECT            'C'     /* Connection information */
#define COMMAND_DEFINE_MACRO       'D'     /* Define macro */
#define COMMAND_END_OF_MESSAGE     'E'     /* final body chunk (End) */
#define COMMAND_HELO               'H'     /* HELO/EHLO */
#define COMMAND_QUIT_NC            'K'     /* QUIT but new connection follows */
#define COMMAND_HEADER             'L'     /* Header */
#define COMMAND_MAIL               'M'     /* MAIL from */
#define COMMAND_END_OF_HEADER      'N'     /* EOH */
#define COMMAND_OPTION_NEGOTIATION 'O'     /* Option negotiation */
#define COMMAND_QUIT               'Q'     /* QUIT */
#define COMMAND_RCPT               'R'     /* RCPT to */
#define COMMAND_DATA               'T'     /* DATA */
#define COMMAND_UNKNOWN            'U'     /* Any unknown command */

#define FAMILY_UNKNOWN             'U'
#define FAMILY_UNIX                'L'
#define FAMILY_INET                '4'
#define FAMILY_INET6               '6'


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
};

typedef enum {
    AT_LEAST,
    EXACT
} CompareType;

typedef enum {
    IN_START,
    IN_COMMAND_LENGTH,
    IN_COMMAND_CONTENT,
    IN_ERROR
} DecodeState;

enum
{
    OPTION_NEGOTIATION,
    DEFINE_MACRO,
    CONNECT,
    HELO,
    MAIL,
    RCPT,
    HEADER,
    END_OF_HEADER,
    BODY,
    END_OF_MESSAGE,
    ABORT,
    QUIT,
    UNKNOWN,
    LAST_SIGNAL
};

static gint signals[LAST_SIGNAL] = {0};

G_DEFINE_TYPE(MilterDecoder, milter_decoder, G_TYPE_OBJECT);

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

    gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->dispose      = dispose;
    gobject_class->set_property = set_property;
    gobject_class->get_property = get_property;

    signals[OPTION_NEGOTIATION] =
        g_signal_new("option-negotiation",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterDecoderClass, option_negotiation),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__OBJECT,
                     G_TYPE_NONE, 1, MILTER_TYPE_OPTION);

    signals[DEFINE_MACRO] =
        g_signal_new("define-macro",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterDecoderClass, define_macro),
                     NULL, NULL,
                     _milter_marshal_VOID__ENUM_POINTER,
                     G_TYPE_NONE, 2, MILTER_TYPE_CONTEXT_TYPE, G_TYPE_POINTER);

    signals[CONNECT] =
        g_signal_new("connect",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterDecoderClass, connect),
                     NULL, NULL,
                     _milter_marshal_VOID__STRING_POINTER_INT,
                     G_TYPE_NONE, 3, G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_INT);

    signals[HELO] =
        g_signal_new("helo",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterDecoderClass, helo),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__STRING,
                     G_TYPE_NONE, 1, G_TYPE_STRING);

    signals[MAIL] =
        g_signal_new("mail",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterDecoderClass, mail),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__STRING,
                     G_TYPE_NONE, 1, G_TYPE_STRING);

    signals[RCPT] =
        g_signal_new("rcpt",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterDecoderClass, rcpt),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__STRING,
                     G_TYPE_NONE, 1, G_TYPE_STRING);

    signals[HEADER] =
        g_signal_new("header",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterDecoderClass, header),
                     NULL, NULL,
                     _milter_marshal_VOID__STRING_STRING,
                     G_TYPE_NONE, 2, G_TYPE_STRING, G_TYPE_STRING);

    signals[END_OF_HEADER] =
        g_signal_new("end-of-header",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterDecoderClass, end_of_header),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__VOID,
                     G_TYPE_NONE, 0);

    signals[BODY] =
        g_signal_new("body",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterDecoderClass, body),
                     NULL, NULL,
#if GLIB_SIZEOF_SIZE_T == 8
                     _milter_marshal_VOID__STRING_UINT64,
                     G_TYPE_NONE, 2, G_TYPE_STRING, G_TYPE_UINT64
#else
                     _milter_marshal_VOID__STRING_UINT,
                     G_TYPE_NONE, 2, G_TYPE_STRING, G_TYPE_UINT
#endif
            );

    signals[END_OF_MESSAGE] =
        g_signal_new("end-of-message",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterDecoderClass, end_of_message),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__VOID,
                     G_TYPE_NONE, 0);

    signals[ABORT] =
        g_signal_new("abort",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterDecoderClass, abort),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__VOID,
                     G_TYPE_NONE, 0);

    signals[QUIT] =
        g_signal_new("quit",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterDecoderClass, quit),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__VOID,
                     G_TYPE_NONE, 0);

    signals[UNKNOWN] =
        g_signal_new("unknown",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterDecoderClass, unknown),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__STRING,
                     G_TYPE_NONE, 1, G_TYPE_STRING);

    g_type_class_add_private(gobject_class, sizeof(MilterDecoderPrivate));
}

static void
milter_decoder_init (MilterDecoder *decoder)
{
    MilterDecoderPrivate *priv = MILTER_DECODER_GET_PRIVATE(decoder);

    priv->state = IN_START;
    priv->buffer = g_string_new(NULL);
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
    MilterDecoderPrivate *priv;

    priv = MILTER_DECODER_GET_PRIVATE(object);
    switch (prop_id) {
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

MilterDecoder *
milter_decoder_new (void)
{
    return g_object_new(MILTER_TYPE_DECODER,
                        NULL);
}

static gboolean
decode_macro_context (gchar macro_context, MilterContextType *context,
                     GError **error)
{
    gboolean success = TRUE;

    switch (macro_context) {
      case COMMAND_CONNECT:
        *context = MILTER_CONTEXT_TYPE_CONNECT;
        break;
      case COMMAND_HELO:
        *context = MILTER_CONTEXT_TYPE_HELO;
        break;
      case COMMAND_MAIL:
        *context = MILTER_CONTEXT_TYPE_MAIL;
        break;
      case COMMAND_RCPT:
        *context = MILTER_CONTEXT_TYPE_RCPT;
        break;
      case COMMAND_HEADER:
        *context = MILTER_CONTEXT_TYPE_HEADER;
        break;
      case COMMAND_END_OF_HEADER:
        *context = MILTER_CONTEXT_TYPE_END_OF_HEADER;
        break;
      case COMMAND_BODY:
        *context = MILTER_CONTEXT_TYPE_BODY;
        break;
      case COMMAND_END_OF_MESSAGE:
        *context = MILTER_CONTEXT_TYPE_END_OF_MESSAGE;
        break;
      default:
        g_set_error(error,
                    MILTER_DECODER_ERROR,
                    MILTER_DECODER_ERROR_UNKNOWN_MACRO_CONTEXT,
                    "unknown macro context: %c", macro_context);
        success = FALSE;
        break;
    }

    return success;
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

static gint
decode_null_terminated_value (const gchar *buffer, gint length,
                             GError **error, const gchar *message)
{
    gint null_character_point;

    null_character_point = find_null_character(buffer, length);
    if (null_character_point <= 0) {
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

static gboolean
check_command_length (const gchar *buffer, gint length, gint expected_length,
                      CompareType compare_type,
                      GError **error, const gchar *target)
{
    GString *message;

    switch (compare_type) {
      case AT_LEAST:
        if (length >= expected_length)
            return TRUE;
        break;
      case EXACT:
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

static gboolean
decode_command_option_negotiation (MilterDecoder *decoder, GError **error)
{
    MilterDecoderPrivate *priv;
    MilterOption *option;
    gsize i;
    guint32 version, action, step;

    priv = MILTER_DECODER_GET_PRIVATE(decoder);

    i = 1;
    if (!check_command_length(priv->buffer->str + i,
                              priv->command_length - i,
                              sizeof(version), AT_LEAST,
                              error, "version on option negotiation command"))
        return FALSE;
    memcpy(&version, priv->buffer->str + i, sizeof(version));
    i += sizeof(version);

    if (!check_command_length(priv->buffer->str + i,
                              priv->command_length - i,
                              sizeof(action), AT_LEAST,
                              error,
                              "action flags on option negotiation command"))
        return FALSE;
    memcpy(&action, priv->buffer->str + i, sizeof(action));
    i += sizeof(action);

    if (!check_command_length(priv->buffer->str + i,
                              priv->command_length - i,
                              sizeof(step), AT_LEAST,
                              error,
                              "step flags on option negotiation command"))
        return FALSE;
    memcpy(&step, priv->buffer->str + i, sizeof(step));
    i += sizeof(step);

    if (!check_command_length(priv->buffer->str + i,
                              priv->command_length - i,
                              0, EXACT, error,
                              "option negotiation command"))
        return FALSE;

    option = milter_option_new(ntohl(version), ntohl(action), ntohl(step));
    g_signal_emit(decoder, signals[OPTION_NEGOTIATION], 0, option);
    g_object_unref(option);

    return TRUE;
}

static GHashTable *
decode_command_macro_definitions (const gchar *buffer, gint length,
                                 GError **error)
{
    GHashTable *macros;
    gint i;
    gboolean success = TRUE;

    macros = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);

    i = 0;
    while (i < length) {
        gint null_character_point;
        const gchar *key, *value;

        null_character_point =
            decode_null_terminated_value(buffer + i, length - i, error,
                                        "name isn't terminated by NULL "
                                        "on define macro command");
        if (null_character_point <= 0) {
            success = FALSE;
            break;
        }
        key = buffer + i;

        i += null_character_point + 1;
        null_character_point =
            decode_null_terminated_value(buffer + i, length - i, error,
                                        "value isn't terminated by NULL "
                                        "on define macro command");
        if (null_character_point <= 0) {
            success = FALSE;
            break;
        }
        value = buffer + i;
        i += null_character_point + 1;

        if (*key) {
            gchar *normalized_key;
            gint key_length;

            key_length = value - key - 1;
            if (key[0] == '{' && key[key_length - 1] == '}')
                normalized_key = g_strndup(key + 1, key_length - 2);
            else
                normalized_key = g_strdup(key);

            g_hash_table_insert(macros, normalized_key, g_strdup(value));
        }
    }

    if (!success) {
        g_hash_table_unref(macros);
        macros = NULL;
    }

    return macros;
}

static gboolean
decode_command_define_macro (MilterDecoder *decoder, GError **error)
{
    MilterDecoderPrivate *priv;
    GHashTable *macros;
    MilterContextType context;
    gboolean success = TRUE;

    priv = MILTER_DECODER_GET_PRIVATE(decoder);
    if (decode_macro_context(priv->buffer->str[1], &context, error)) {
        macros = decode_command_macro_definitions(priv->buffer->str + 1 + 1,
                                                 priv->command_length - 1 - 1,
                                                 error);
        if (macros) {
            g_signal_emit(decoder, signals[DEFINE_MACRO], 0, context, macros);
            g_hash_table_unref(macros);
        } else {
            success = FALSE;
        }
    } else {
        success = FALSE;
    }

    return success;
}

static gboolean
decode_command_connect_inet_address (const gchar *buffer,
                                    struct sockaddr **address,
                                    socklen_t *address_length,
                                    const gchar *host_name,
                                    gchar family, guint port,
                                    GError **error)
{
    struct sockaddr_in *address_in;
    struct in_addr ip_address;

    if (inet_aton(buffer, &ip_address) == 0) {
        g_set_error(error,
                    MILTER_DECODER_ERROR,
                    MILTER_DECODER_ERROR_INVALID_FORMAT,
                    "invalid IPv4 address on connect command: "
                    "<%s>: <%c>: <%u>: <%s>",
                    host_name, family, ntohs(port), buffer);
        return FALSE;
    }

    *address_length = sizeof(struct sockaddr_in);
    address_in = g_malloc(*address_length);
    address_in->sin_family = AF_INET;
    address_in->sin_port = port;
    address_in->sin_addr = ip_address;
    *address = (struct sockaddr *)address_in;

    return TRUE;
}

static gboolean
decode_command_connect_inet6_address (const gchar *buffer,
                                     struct sockaddr **address,
                                     socklen_t *address_length,
                                     const gchar *host_name,
                                     gchar family, guint port,
                                     GError **error)
{
    struct sockaddr_in6 *address_in6;
    struct in6_addr ipv6_address;

    if (inet_pton(AF_INET6, buffer, &ipv6_address) == 0) {
        g_set_error(error,
                    MILTER_DECODER_ERROR,
                    MILTER_DECODER_ERROR_INVALID_FORMAT,
                    "invalid IPv6 address on connect command: "
                    "<%s>: <%c>: <%u>: <%s>",
                    host_name, family, ntohs(port), buffer);
        return FALSE;
    }

    *address_length = sizeof(struct sockaddr_in6);
    address_in6 = g_malloc(*address_length);
    address_in6->sin6_family = AF_INET6;
    address_in6->sin6_port = port;
    address_in6->sin6_addr = ipv6_address;
    *address = (struct sockaddr *)address_in6;

    return TRUE;
}

static gboolean
decode_command_connect_unix_address (const gchar *buffer,
                                    struct sockaddr **address,
                                    socklen_t *address_length,
                                    const gchar *host_name,
                                    gchar family, guint port,
                                    GError **error)
{
    struct sockaddr_un *address_un;

    *address_length = sizeof(struct sockaddr_un);
    address_un = g_malloc(*address_length);
    address_un->sun_family = AF_UNIX;
    strcpy(address_un->sun_path, buffer);
    *address = (struct sockaddr *)address_un;

    return TRUE;
}

static gboolean
decode_command_connect_content (const gchar *buffer, gint length,
                               gchar **host_name,
                               struct sockaddr **address,
                               socklen_t *address_length,
                               GError **error)
{
    gchar family;
    gint i, null_character_point;
    uint16_t port;
    gchar *error_message;
    const gchar *decoded_host_name;

    null_character_point =
        decode_null_terminated_value(buffer, length, error,
                                    "host name isn't terminated by NULL "
                                    "on connect command");
    if (null_character_point <= 0)
        return FALSE;

    decoded_host_name = buffer;
    i = null_character_point + 1;

    family = buffer[i];
    i++;
    switch (family) {
      case FAMILY_INET:
      case FAMILY_INET6:
      case FAMILY_UNIX:
        if (!check_command_length(buffer + i, length - i, sizeof(port), AT_LEAST,
                                  error, "port number on connect command"))
            return FALSE;
        memcpy(&port, buffer + i, sizeof(port));
        i += sizeof(port);
        break;
      default:
        g_set_error(error,
                    MILTER_DECODER_ERROR,
                    MILTER_DECODER_ERROR_UNKNOWN_FAMILY,
                    "unknown family on connect command: <%s>: <%c>",
                    decoded_host_name, family);
        return FALSE;
    }

    error_message = g_strdup_printf("address name isn't terminated by NULL "
                                    "on connect command: <%s>: <%c>: <%u>",
                                    decoded_host_name, family, ntohs(port));
    null_character_point =
        decode_null_terminated_value(buffer + i, length - i, error,
                                    error_message);
    g_free(error_message);
    if (null_character_point <= 0)
        return FALSE;

    switch (family) {
      case FAMILY_INET:
        if (!decode_command_connect_inet_address(buffer + i,
                                                address, address_length,
                                                decoded_host_name, family, port,
                                                error))
            return FALSE;
        break;
      case FAMILY_INET6:
        if (!decode_command_connect_inet6_address(buffer + i,
                                                 address, address_length,
                                                 decoded_host_name, family, port,
                                                 error))
            return FALSE;
        break;
      case FAMILY_UNIX:
        if (!decode_command_connect_unix_address(buffer + i,
                                                address, address_length,
                                                decoded_host_name, family, port,
                                                error))
            return FALSE;
        break;
      default:
        g_set_error(error,
                    MILTER_DECODER_ERROR,
                    MILTER_DECODER_ERROR_UNKNOWN_FAMILY,
                    "unknown family on connect command: <%s>: <%c>: <%u>",
                    decoded_host_name, family, ntohs(port));
        return FALSE;
        break;
    }

    *host_name = g_strdup(decoded_host_name);
    return TRUE;
}

static gboolean
decode_command_connect (MilterDecoder *decoder, GError **error)
{
    MilterDecoderPrivate *priv;
    gchar *host_name;
    struct sockaddr *address;
    socklen_t length;

    priv = MILTER_DECODER_GET_PRIVATE(decoder);

    if (!decode_command_connect_content(priv->buffer->str + 1,
                                       priv->command_length - 1,
                                       &host_name, &address, &length, error))
        return FALSE;

    g_signal_emit(decoder, signals[CONNECT], 0, host_name, address, length);
    g_free(host_name);
    g_free(address);

    return TRUE;
}

static gboolean
decode_command_helo (MilterDecoder *decoder, GError **error)
{
    MilterDecoderPrivate *priv;
    gint null_character_point;

    priv = MILTER_DECODER_GET_PRIVATE(decoder);
    null_character_point =
        decode_null_terminated_value(priv->buffer->str + 1,
                                    priv->command_length - 1,
                                    error,
                                    "FQDN isn't terminated by NULL "
                                    "on HELO command");
    if (null_character_point <= 0)
        return FALSE;

    g_signal_emit(decoder, signals[HELO], 0, priv->buffer->str + 1);

    return TRUE;
}

static gboolean
decode_command_mail (MilterDecoder *decoder, GError **error)
{
    MilterDecoderPrivate *priv;
    gint null_character_point;

    priv = MILTER_DECODER_GET_PRIVATE(decoder);
    null_character_point =
        decode_null_terminated_value(priv->buffer->str + 1,
                                    priv->command_length - 1,
                                    error,
                                    "FROM isn't terminated by NULL "
                                    "on MAIL command");
    if (null_character_point <= 0)
        return FALSE;

    g_signal_emit(decoder, signals[MAIL], 0, priv->buffer->str + 1);

    return TRUE;
}

static gboolean
decode_command_rcpt (MilterDecoder *decoder, GError **error)
{
    MilterDecoderPrivate *priv;
    gint null_character_point;

    priv = MILTER_DECODER_GET_PRIVATE(decoder);
    null_character_point =
        decode_null_terminated_value(priv->buffer->str + 1,
                                    priv->command_length - 1,
                                    error,
                                    "TO isn't terminated by NULL "
                                    "on RCPT command");
    if (null_character_point <= 0)
        return FALSE;

    g_signal_emit(decoder, signals[RCPT], 0, priv->buffer->str + 1);
    return TRUE;
}


static gboolean
decode_command_header_content (const gchar *buffer, gint length,
                              const gchar **name, const gchar **value,
                              GError **error)
{
    gint null_character_point;
    gchar *error_message;

    null_character_point =
        decode_null_terminated_value(buffer, length, error,
                                    "name isn't terminated by NULL "
                                    "on header command");
    if (null_character_point <= 0)
        return FALSE;
    *name = buffer;

    buffer += (null_character_point + 1);
    length -= (null_character_point + 1);
    error_message = g_strdup_printf("value isn't terminated by NULL "
                                    "on header command: <%s>", *name);
    null_character_point =
        decode_null_terminated_value(buffer, length, error, error_message);
    g_free(error_message);
    if (null_character_point <= 0)
        return FALSE;
    *value = buffer;

    return TRUE;
}

static gboolean
decode_command_header (MilterDecoder *decoder, GError **error)
{
    MilterDecoderPrivate *priv;
    const gchar *name = NULL, *value = NULL;

    priv = MILTER_DECODER_GET_PRIVATE(decoder);
    if (!decode_command_header_content(priv->buffer->str + 1,
                                      priv->command_length - 1,
                                      &name, &value, error))
        return FALSE;

    g_signal_emit(decoder, signals[HEADER], 0, name, value);

    return TRUE;
}

static gboolean
decode_command_end_of_header (MilterDecoder *decoder, GError **error)
{
    MilterDecoderPrivate *priv;

    priv = MILTER_DECODER_GET_PRIVATE(decoder);
    if (!check_command_length(priv->buffer->str + 1,
                              priv->command_length - 1, 0, EXACT,
                              error, "END OF HEADER command"))
        return FALSE;

    g_signal_emit(decoder, signals[END_OF_HEADER], 0);

    return TRUE;
}

static gboolean
decode_command_body (MilterDecoder *decoder, GError **error)
{
    MilterDecoderPrivate *priv;

    priv = MILTER_DECODER_GET_PRIVATE(decoder);
    g_signal_emit(decoder, signals[BODY], 0,
                  priv->buffer->str + 1, priv->command_length - 1);
    return TRUE;
}

static gboolean
decode_command_end_of_message (MilterDecoder *decoder, GError **error)
{
    MilterDecoderPrivate *priv;

    priv = MILTER_DECODER_GET_PRIVATE(decoder);
    if (priv->command_length > 1) {
        g_signal_emit(decoder, signals[BODY], 0,
                      priv->buffer->str + 1, priv->command_length - 1);
    }
    g_signal_emit(decoder, signals[END_OF_MESSAGE], 0);

    return TRUE;
}

static gboolean
decode_command_abort (MilterDecoder *decoder, GError **error)
{
    MilterDecoderPrivate *priv;

    priv = MILTER_DECODER_GET_PRIVATE(decoder);
    if (!check_command_length(priv->buffer->str + 1,
                              priv->command_length - 1, 0, EXACT,
                              error, "ABORT command"))
        return FALSE;

    g_signal_emit(decoder, signals[ABORT], 0);

    return TRUE;
}

static gboolean
decode_command_quit (MilterDecoder *decoder, GError **error)
{
    MilterDecoderPrivate *priv;

    priv = MILTER_DECODER_GET_PRIVATE(decoder);
    if (!check_command_length(priv->buffer->str + 1,
                              priv->command_length - 1, 0, EXACT,
                              error, "QUIT command"))
        return FALSE;

    g_signal_emit(decoder, signals[QUIT], 0);

    return TRUE;
}

static gboolean
decode_command_unknown (MilterDecoder *decoder, GError **error)
{
    MilterDecoderPrivate *priv;
    gint null_character_point;

    priv = MILTER_DECODER_GET_PRIVATE(decoder);
    null_character_point =
        decode_null_terminated_value(priv->buffer->str + 1,
                                    priv->command_length - 1,
                                    error,
                                    "command value isn't terminated by NULL "
                                    "on unknown command");
    if (null_character_point <= 0)
        return FALSE;

    g_signal_emit(decoder, signals[UNKNOWN], 0, priv->buffer->str + 1);

    return TRUE;
}

static gboolean
decode_command (MilterDecoder *decoder, GError **error)
{
    MilterDecoderPrivate *priv;
    gboolean success = TRUE;

    priv = MILTER_DECODER_GET_PRIVATE(decoder);
    switch (priv->buffer->str[0]) {
      case COMMAND_OPTION_NEGOTIATION:
        success = decode_command_option_negotiation(decoder, error);
        break;
      case COMMAND_DEFINE_MACRO:
        success = decode_command_define_macro(decoder, error);
        break;
      case COMMAND_CONNECT:
        success = decode_command_connect(decoder, error);
        break;
      case COMMAND_HELO:
        success = decode_command_helo(decoder, error);
        break;
      case COMMAND_MAIL:
        success = decode_command_mail(decoder, error);
        break;
      case COMMAND_RCPT:
        success = decode_command_rcpt(decoder, error);
        break;
      case COMMAND_HEADER:
        success = decode_command_header(decoder, error);
        break;
      case COMMAND_END_OF_HEADER:
        success = decode_command_end_of_header(decoder, error);
        break;
      case COMMAND_BODY:
        success = decode_command_body(decoder, error);
        break;
      case COMMAND_END_OF_MESSAGE:
        success = decode_command_end_of_message(decoder, error);
        break;
      case COMMAND_ABORT:
        success = decode_command_abort(decoder, error);
        break;
      case COMMAND_QUIT:
        success = decode_command_quit(decoder, error);
        break;
      case COMMAND_UNKNOWN:
        success = decode_command_unknown(decoder, error);
        break;
      default:
        g_set_error(error,
                    MILTER_DECODER_ERROR,
                    MILTER_DECODER_ERROR_UNEXPECTED_COMMAND,
                    "unexpected command was received: %c", priv->buffer->str[0]);
        success = FALSE;
        break;
    }

    if (success) {
        priv->state = IN_START;
        g_string_erase(priv->buffer, 0, priv->command_length);
    } else {
        priv->state = IN_ERROR;
    }

    return success;
}

gboolean
milter_decoder_decode (MilterDecoder *decoder, const gchar *text, gsize text_len,
                     GError **error)
{
    MilterDecoderPrivate *priv;
    gboolean loop = TRUE;
    gboolean success = TRUE;

    priv = MILTER_DECODER_GET_PRIVATE(decoder);

    g_return_val_if_fail(priv->state != MILTER_DECODER_ERROR, FALSE);

    if (text_len == 0)
        return TRUE;

    g_string_append_len(priv->buffer, text, text_len);
    while (loop) {
        switch (priv->state) {
          case IN_START:
            if (priv->buffer->len == 0) {
                loop = FALSE;
            } else {
                priv->state = IN_COMMAND_LENGTH;
            }
            break;
          case IN_COMMAND_LENGTH:
            if (priv->buffer->len < COMMAND_LENGTH_BYTES) {
                loop = FALSE;
            } else {
                memcpy(&priv->command_length,
                       priv->buffer->str,
                       COMMAND_LENGTH_BYTES);
                priv->command_length = ntohl(priv->command_length);
                g_string_erase(priv->buffer, 0, COMMAND_LENGTH_BYTES);
                priv->state = IN_COMMAND_CONTENT;
            }
            break;
          case IN_COMMAND_CONTENT:
            if (priv->buffer->len < priv->command_length) {
                loop = FALSE;
            } else {
                success = decode_command(decoder, error);
                if (!success)
                    loop = FALSE;
            }
            break;
          case IN_ERROR:
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

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
