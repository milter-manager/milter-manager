/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Copyright (C) 2008-2009  Kouhei Sutou <kou@clear-code.com>
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

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <glib.h>

#include "milter-command-decoder.h"
#include "milter-logger.h"
#include "milter-enum-types.h"
#include "milter-marshalers.h"

enum
{
    NEGOTIATE,
    DEFINE_MACRO,
    CONNECT,
    HELO,
    ENVELOPE_FROM,
    ENVELOPE_RECIPIENT,
    DATA,
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

G_DEFINE_TYPE(MilterCommandDecoder, milter_command_decoder, MILTER_TYPE_DECODER);

static void dispose        (GObject         *object);
static void set_property   (GObject         *object,
                            guint            prop_id,
                            const GValue    *value,
                            GParamSpec      *pspec);
static void get_property   (GObject         *object,
                            guint            prop_id,
                            GValue          *value,
                            GParamSpec      *pspec);

static gboolean decode     (MilterDecoder *decoder,
                            GError       **error);

static void
milter_command_decoder_class_init (MilterCommandDecoderClass *klass)
{
    GObjectClass *gobject_class;
    MilterDecoderClass *decoder_class;

    gobject_class = G_OBJECT_CLASS(klass);
    decoder_class = MILTER_DECODER_CLASS(klass);

    gobject_class->dispose      = dispose;
    gobject_class->set_property = set_property;
    gobject_class->get_property = get_property;

    decoder_class->decode = decode;

    signals[NEGOTIATE] =
        g_signal_new("negotiate",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterCommandDecoderClass, negotiate),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__OBJECT,
                     G_TYPE_NONE, 1, MILTER_TYPE_OPTION);

    signals[DEFINE_MACRO] =
        g_signal_new("define-macro",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterCommandDecoderClass, define_macro),
                     NULL, NULL,
                     _milter_marshal_VOID__ENUM_POINTER,
                     G_TYPE_NONE, 2, MILTER_TYPE_COMMAND, G_TYPE_POINTER);

    signals[CONNECT] =
        g_signal_new("connect",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterCommandDecoderClass, connect),
                     NULL, NULL,
                     _milter_marshal_VOID__STRING_POINTER_UINT,
                     G_TYPE_NONE, 3, G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_UINT);

    signals[HELO] =
        g_signal_new("helo",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterCommandDecoderClass, helo),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__STRING,
                     G_TYPE_NONE, 1, G_TYPE_STRING);

    signals[ENVELOPE_FROM] =
        g_signal_new("envelope-from",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterCommandDecoderClass, envelope_from),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__STRING,
                     G_TYPE_NONE, 1, G_TYPE_STRING);

    signals[ENVELOPE_RECIPIENT] =
        g_signal_new("envelope-recipient",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterCommandDecoderClass,
                                     envelope_recipient),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__STRING,
                     G_TYPE_NONE, 1, G_TYPE_STRING);

    signals[DATA] =
        g_signal_new("data",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterCommandDecoderClass, data),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__VOID,
                     G_TYPE_NONE, 0);

    signals[HEADER] =
        g_signal_new("header",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterCommandDecoderClass, header),
                     NULL, NULL,
                     _milter_marshal_VOID__STRING_STRING,
                     G_TYPE_NONE, 2, G_TYPE_STRING, G_TYPE_STRING);

    signals[END_OF_HEADER] =
        g_signal_new("end-of-header",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterCommandDecoderClass, end_of_header),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__VOID,
                     G_TYPE_NONE, 0);

    signals[BODY] =
        g_signal_new("body",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterCommandDecoderClass, body),
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
                     G_STRUCT_OFFSET(MilterCommandDecoderClass, end_of_message),
                     NULL, NULL,
#if GLIB_SIZEOF_SIZE_T == 8
                     _milter_marshal_VOID__STRING_UINT64,
                     G_TYPE_NONE, 2, G_TYPE_STRING, G_TYPE_UINT64
#else
                     _milter_marshal_VOID__STRING_UINT,
                     G_TYPE_NONE, 2, G_TYPE_STRING, G_TYPE_UINT
#endif
                     );

    signals[ABORT] =
        g_signal_new("abort",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterCommandDecoderClass, abort),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__VOID,
                     G_TYPE_NONE, 0);

    signals[QUIT] =
        g_signal_new("quit",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterCommandDecoderClass, quit),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__VOID,
                     G_TYPE_NONE, 0);

    signals[UNKNOWN] =
        g_signal_new("unknown",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterCommandDecoderClass, unknown),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__STRING,
                     G_TYPE_NONE, 1, G_TYPE_STRING);
}

static void
milter_command_decoder_init (MilterCommandDecoder *decoder)
{
}

static void
dispose (GObject *object)
{
    G_OBJECT_CLASS(milter_command_decoder_parent_class)->dispose(object);
}

static void
set_property (GObject      *object,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
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
    switch (prop_id) {
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

GQuark
milter_command_decoder_error_quark (void)
{
    return g_quark_from_static_string("milter-command-decoder-error-quark");
}

MilterDecoder *
milter_command_decoder_new (void)
{
    return MILTER_DECODER(g_object_new(MILTER_TYPE_COMMAND_DECODER,
                                       NULL));
}

static gboolean
check_macro_context (MilterCommand macro_context, GError **error)
{
    gboolean success = TRUE;

    switch (macro_context) {
      case MILTER_COMMAND_CONNECT:
      case MILTER_COMMAND_HELO:
      case MILTER_COMMAND_ENVELOPE_FROM:
      case MILTER_COMMAND_ENVELOPE_RECIPIENT:
      case MILTER_COMMAND_DATA:
      case MILTER_COMMAND_HEADER:
      case MILTER_COMMAND_END_OF_HEADER:
      case MILTER_COMMAND_BODY:
      case MILTER_COMMAND_END_OF_MESSAGE:
      case MILTER_COMMAND_UNKNOWN:
        break;
      default:
        g_set_error(error,
                    MILTER_COMMAND_DECODER_ERROR,
                    MILTER_COMMAND_DECODER_ERROR_UNKNOWN_MACRO_CONTEXT,
                    "unknown macro context: %c", macro_context);
        success = FALSE;
        break;
    }

    return success;
}

static GHashTable *
decode_define_macros (const gchar *buffer,
                      gint length,
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
            milter_decoder_decode_null_terminated_value(
                buffer + i, length - i, error,
                "name isn't terminated by NULL on define macro command");
        if (null_character_point <= 0) {
            success = FALSE;
            break;
        }
        key = buffer + i;

        i += null_character_point + 1;
        null_character_point =
            milter_decoder_decode_null_terminated_value(
                buffer + i, length - i, error,
                "value isn't terminated by NULL on define macro command");
        if (null_character_point < 0) {
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
decode_define_macro (MilterDecoder *decoder, GError **error)
{
    GHashTable *macros;
    MilterCommand context;
    const gchar *buffer;
    gint32 command_length;

    command_length = milter_decoder_get_command_length(decoder);
    buffer = milter_decoder_get_buffer(decoder);

    if (!milter_decoder_check_command_length(buffer + 1,
                                             strlen(buffer) - 1,
                                             1,
                                             MILTER_DECODER_COMPARE_AT_LEAST,
                                             error,
                                             "macro context "
                                             "on define macro command"))
        return FALSE;

    if (!check_macro_context(buffer[1], error))
        return FALSE;
    context = buffer[1];

    macros = decode_define_macros(buffer + 1 + 1,
                                  command_length - 1 - 1,
                                  error);
    if (!macros)
        return FALSE;

    milter_debug("[%u] [command-decoder][define-macro] <%c>",
                 milter_decoder_get_tag(decoder),
                 context);

    g_signal_emit(decoder, signals[DEFINE_MACRO], 0, context, macros);
    g_hash_table_unref(macros);

    return TRUE;
}

static gboolean
decode_connect_inet_address (const gchar *buffer,
                             struct sockaddr **address,
                             socklen_t *address_length,
                             const gchar *host_name,
                             gchar family, guint port,
                             GError **error)
{
    struct sockaddr_in *address_in;
    struct in_addr ip_address;

    if (inet_pton(AF_INET, buffer, &ip_address) == 0) {
        g_set_error(error,
                    MILTER_COMMAND_DECODER_ERROR,
                    MILTER_COMMAND_DECODER_ERROR_INVALID_FORMAT,
                    "invalid IPv4 address on connect command: "
                    "<%s>: <%c>: <%u>: <%s>",
                    host_name, family, g_ntohs(port), buffer);
        return FALSE;
    }

    *address_length = sizeof(struct sockaddr_in);
    address_in = g_malloc(*address_length);
    memset(address_in, 0, *address_length);
    address_in->sin_family = AF_INET;
    address_in->sin_port = port;
    address_in->sin_addr = ip_address;
    *address = (struct sockaddr *)address_in;

    return TRUE;
}

static gboolean
decode_connect_inet6_address (const gchar *buffer,
                              struct sockaddr **address,
                              socklen_t *address_length,
                              const gchar *host_name,
                              gchar family, guint port,
                              GError **error)
{
    struct sockaddr_in6 *address_in6;
    struct in6_addr ipv6_address;
    const gchar ipv6_prefix[] = "IPv6:";
    size_t address_offset = 0;

    if (g_str_has_prefix(buffer, ipv6_prefix)) {
        address_offset = strlen(ipv6_prefix);
    }

    if (inet_pton(AF_INET6, buffer + address_offset, &ipv6_address) == 0) {
        g_set_error(error,
                    MILTER_COMMAND_DECODER_ERROR,
                    MILTER_COMMAND_DECODER_ERROR_INVALID_FORMAT,
                    "invalid IPv6 address on connect command: "
                    "<%s>: <%c>: <%u>: <%s>",
                    host_name, family, g_ntohs(port), buffer);
        return FALSE;
    }

    *address_length = sizeof(struct sockaddr_in6);
    address_in6 = g_malloc(*address_length);
    memset(address_in6, 0, *address_length);
    address_in6->sin6_family = AF_INET6;
    address_in6->sin6_port = port;
    address_in6->sin6_addr = ipv6_address;
    *address = (struct sockaddr *)address_in6;

    return TRUE;
}

static gboolean
decode_connect_unix_address (const gchar *buffer,
                             struct sockaddr **address,
                             socklen_t *address_length,
                             const gchar *host_name,
                             gchar family, guint port,
                             GError **error)
{
    struct sockaddr_un *address_un;

    *address_length = sizeof(struct sockaddr_un);
    address_un = g_malloc(*address_length);
    memset(address_un, 0, *address_length);
    address_un->sun_family = AF_UNIX;
    strcpy(address_un->sun_path, buffer);
    *address = (struct sockaddr *)address_un;

    return TRUE;
}

static gboolean
decode_connect_content (const gchar *buffer, gint length,
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
        milter_decoder_decode_null_terminated_value(buffer, length, error,
                                    "host name isn't terminated by NULL "
                                    "on connect command");
    if (null_character_point <= 0)
        return FALSE;

    decoded_host_name = buffer;
    i = null_character_point + 1;

    family = buffer[i];
    i++;
    switch (family) {
    case MILTER_SOCKET_FAMILY_INET:
    case MILTER_SOCKET_FAMILY_INET6:
    case MILTER_SOCKET_FAMILY_UNIX:
        if (!milter_decoder_check_command_length(
                buffer + i, length - i, sizeof(port),
                MILTER_DECODER_COMPARE_AT_LEAST, error,
                "port number on connect command"))
            return FALSE;
        memcpy(&port, buffer + i, sizeof(port));
        i += sizeof(port);
        break;
    case MILTER_SOCKET_FAMILY_UNKNOWN:
        if (!milter_decoder_check_command_length(
                buffer + i, length - i, 0,
                MILTER_DECODER_COMPARE_EXACT, error,
                "unknown family on connect command"))
            return FALSE;
        break;
    default:
        g_set_error(error,
                    MILTER_COMMAND_DECODER_ERROR,
                    MILTER_COMMAND_DECODER_ERROR_UNKNOWN_SOCKET_FAMILY,
                    "unexpected family on connect command: <%s>: <%c>",
                    decoded_host_name, family);
        return FALSE;
    }

    if (family != MILTER_SOCKET_FAMILY_UNKNOWN) {
        error_message =
            g_strdup_printf("address name isn't terminated by NULL "
                            "on connect command: <%s>: <%c>: <%u>",
                            decoded_host_name, family, g_ntohs(port));
        null_character_point =
            milter_decoder_decode_null_terminated_value(buffer + i,
                                                        length - i,
                                                        error,
                                                        error_message);
        g_free(error_message);
        if (null_character_point <= 0)
            return FALSE;
    }

    switch (family) {
    case MILTER_SOCKET_FAMILY_INET:
        if (!decode_connect_inet_address(buffer + i,
                                         address, address_length,
                                         decoded_host_name, family, port,
                                         error))
            return FALSE;
        break;
    case MILTER_SOCKET_FAMILY_INET6:
        if (!decode_connect_inet6_address(buffer + i,
                                          address, address_length,
                                          decoded_host_name,
                                          family, port,
                                          error))
            return FALSE;
        break;
    case MILTER_SOCKET_FAMILY_UNIX:
        if (!decode_connect_unix_address(buffer + i,
                                         address, address_length,
                                         decoded_host_name, family, port,
                                         error))
            return FALSE;
        break;
    case MILTER_SOCKET_FAMILY_UNKNOWN:
        *address_length = sizeof(struct sockaddr);
        *address = g_malloc0(*address_length);
        (*address)->sa_family = AF_UNSPEC;
        break;
    default:
        g_set_error(error,
                    MILTER_COMMAND_DECODER_ERROR,
                    MILTER_COMMAND_DECODER_ERROR_UNKNOWN_SOCKET_FAMILY,
                    "unexpected family on connect command: <%s>: <%c>: <%u>",
                    decoded_host_name, family, g_ntohs(port));
        return FALSE;
        break;
    }

    *host_name = g_strdup(decoded_host_name);
    return TRUE;
}

static gboolean
decode_connect (MilterDecoder *decoder, GError **error)
{
    gchar *host_name;
    struct sockaddr *address;
    socklen_t length;
    const gchar *buffer;
    gint32 command_length;

    command_length = milter_decoder_get_command_length(decoder);
    buffer = milter_decoder_get_buffer(decoder);

    if (!decode_connect_content(buffer + 1,
                                command_length - 1,
                                &host_name, &address, &length, error))
        return FALSE;

    milter_debug("[%u] [command-decoder][connect] <%s>",
                 milter_decoder_get_tag(decoder),
                 host_name);

    g_signal_emit(decoder, signals[CONNECT], 0, host_name, address, length);
    g_free(host_name);
    g_free(address);

    return TRUE;
}

static gboolean
decode_helo (MilterDecoder *decoder, GError **error)
{
    gint null_character_point;
    const gchar *buffer;
    gint32 command_length;

    command_length = milter_decoder_get_command_length(decoder);
    buffer = milter_decoder_get_buffer(decoder);

    null_character_point =
        milter_decoder_decode_null_terminated_value(
            buffer + 1, command_length - 1, error,
            "FQDN isn't terminated by NULL on HELO command");
    if (null_character_point <= 0)
        return FALSE;

    milter_debug("[%u] [command-decoder][helo] <%s>",
                 milter_decoder_get_tag(decoder),
                 buffer + 1);

    g_signal_emit(decoder, signals[HELO], 0, buffer + 1);

    return TRUE;
}

static gboolean
decode_envelope_from (MilterDecoder *decoder, GError **error)
{
    gint null_character_point;
    const gchar *buffer;
    gint32 command_length;

    command_length = milter_decoder_get_command_length(decoder);
    buffer = milter_decoder_get_buffer(decoder);

    null_character_point =
        milter_decoder_decode_null_terminated_value(
            buffer + 1, command_length - 1, error,
            "FROM isn't terminated by NULL on MAIL command");
    if (null_character_point <= 0)
        return FALSE;

    milter_debug("[%u] [command-decoder][envelope-from] <%s>",
                 milter_decoder_get_tag(decoder),
                 buffer + 1);

    g_signal_emit(decoder, signals[ENVELOPE_FROM], 0, buffer + 1);

    return TRUE;
}

static gboolean
decode_envelope_recipient (MilterDecoder *decoder, GError **error)
{
    gint null_character_point;
    const gchar *buffer;
    gint32 command_length;

    command_length = milter_decoder_get_command_length(decoder);
    buffer = milter_decoder_get_buffer(decoder);

    null_character_point =
        milter_decoder_decode_null_terminated_value(
            buffer + 1, command_length - 1, error,
            "TO isn't terminated by NULL on RCPT command");
    if (null_character_point <= 0)
        return FALSE;

    milter_debug("[%u] [command-decoder][envelope-recipient] <%s>",
                 milter_decoder_get_tag(decoder),
                 buffer + 1);

    g_signal_emit(decoder, signals[ENVELOPE_RECIPIENT], 0, buffer + 1);
    return TRUE;
}

static gboolean
decode_data (MilterDecoder *decoder, GError **error)
{
    const gchar *buffer;
    gint32 command_length;

    command_length = milter_decoder_get_command_length(decoder);
    buffer = milter_decoder_get_buffer(decoder);

    if (!milter_decoder_check_command_length(
            buffer + 1, command_length - 1, 0,
            MILTER_DECODER_COMPARE_EXACT, error,
            "DATA command"))
        return FALSE;

    milter_debug("[%u] [command-decoder][data]",
                 milter_decoder_get_tag(decoder));

    g_signal_emit(decoder, signals[DATA], 0);

    return TRUE;
}

static gboolean
decode_header (MilterDecoder *decoder, GError **error)
{
    const gchar *name = NULL, *value = NULL;
    const gchar *buffer;
    gint32 command_length;
    gboolean decoded;

    command_length = milter_decoder_get_command_length(decoder);
    buffer = milter_decoder_get_buffer(decoder);

    decoded = milter_decoder_decode_header_content(buffer + 1,
                                                   command_length - 1,
                                                   &name, &value, error);

    if (!decoded)
        return FALSE;

    milter_debug("[%u] [command-decoder][header] <%s>=<%s>",
                 milter_decoder_get_tag(decoder),
                 name, value);

    g_signal_emit(decoder, signals[HEADER], 0, name, value);

    return TRUE;
}

static gboolean
decode_end_of_header (MilterDecoder *decoder, GError **error)
{
    const gchar *buffer;
    gint32 command_length;

    command_length = milter_decoder_get_command_length(decoder);
    buffer = milter_decoder_get_buffer(decoder);

    if (!milter_decoder_check_command_length(
            buffer + 1, command_length - 1, 0,
            MILTER_DECODER_COMPARE_EXACT, error,
            "END OF HEADER command"))
        return FALSE;

    milter_debug("[%u] [command-decoder][end-of-header]",
                 milter_decoder_get_tag(decoder));

    g_signal_emit(decoder, signals[END_OF_HEADER], 0);

    return TRUE;
}

static gboolean
decode_body (MilterDecoder *decoder, GError **error)
{
    const gchar *buffer;
    gint32 command_length;

    command_length = milter_decoder_get_command_length(decoder);
    buffer = milter_decoder_get_buffer(decoder);

    milter_debug("[%u] [command-decoder][body] <%d>",
                 milter_decoder_get_tag(decoder),
                 command_length - 1);

    g_signal_emit(decoder, signals[BODY], 0,
                  buffer + 1, command_length - 1);
    return TRUE;
}

static gboolean
decode_end_of_message (MilterDecoder *decoder, GError **error)
{
    const gchar *chunk;
    gsize chunk_size;
    gint32 command_length;

    command_length = milter_decoder_get_command_length(decoder);

    if (command_length > 1) {
        chunk = milter_decoder_get_buffer(decoder) + 1;
        chunk_size = command_length - 1;
    } else {
        chunk = NULL;
        chunk_size = 0;
    }

    milter_debug("[%u] [command-decoder][end-of-message] <%" G_GSIZE_FORMAT ">",
                 milter_decoder_get_tag(decoder),
                 chunk_size);

    g_signal_emit(decoder, signals[END_OF_MESSAGE], 0, chunk, chunk_size);

    return TRUE;
}

static gboolean
decode_abort (MilterDecoder *decoder, GError **error)
{
    const gchar *buffer;
    gint32 command_length;

    command_length = milter_decoder_get_command_length(decoder);
    buffer = milter_decoder_get_buffer(decoder);

    if (!milter_decoder_check_command_length(
            buffer + 1, command_length - 1, 0,
            MILTER_DECODER_COMPARE_EXACT, error,
            "ABORT command"))
        return FALSE;

    milter_debug("[%u] [command-decoder][abort]",
                 milter_decoder_get_tag(decoder));

    g_signal_emit(decoder, signals[ABORT], 0);

    return TRUE;
}

static gboolean
decode_quit (MilterDecoder *decoder, GError **error)
{
    const gchar *buffer;
    gint32 command_length;

    command_length = milter_decoder_get_command_length(decoder);
    buffer = milter_decoder_get_buffer(decoder);

    if (!milter_decoder_check_command_length(
            buffer + 1, command_length - 1, 0,
            MILTER_DECODER_COMPARE_EXACT, error,
            "QUIT command"))
        return FALSE;

    milter_debug("[%u] [command-decoder][quit]",
                 milter_decoder_get_tag(decoder));

    g_signal_emit(decoder, signals[QUIT], 0);

    return TRUE;
}

static gboolean
decode_unknown (MilterDecoder *decoder, GError **error)
{
    gint null_character_point;
    const gchar *buffer;
    gint32 command_length;

    command_length = milter_decoder_get_command_length(decoder);
    buffer = milter_decoder_get_buffer(decoder);

    null_character_point =
        milter_decoder_decode_null_terminated_value(
            buffer + 1, command_length - 1, error,
            "command value isn't terminated by NULL on unknown command");
    if (null_character_point <= 0)
        return FALSE;

    milter_debug("[%u] [command-decoder][unknown] <%s>",
                 milter_decoder_get_tag(decoder),
                 buffer + 1);

    g_signal_emit(decoder, signals[UNKNOWN], 0, buffer + 1);

    return TRUE;
}

static gboolean
decode_negotiate (MilterDecoder *decoder, GError **error)
{
    const gchar *buffer;
    gint32 command_length;
    MilterOption *option;
    gint processed_length;

    command_length = milter_decoder_get_command_length(decoder);
    buffer = milter_decoder_get_buffer(decoder);

    option = milter_decoder_decode_negotiate(buffer,
                                             command_length,
                                             &processed_length,
                                             error);
    if (!option)
        return FALSE;

    if (!milter_decoder_check_command_length(
            buffer + processed_length, command_length - processed_length, 0,
            MILTER_DECODER_COMPARE_EXACT, error,
            "option negotiation command")) {
        g_object_unref(option);
        return FALSE;
    }

    milter_debug("[%u] [command-decoder][negotiate]",
                 milter_decoder_get_tag(decoder));

    g_signal_emit(decoder, signals[NEGOTIATE], 0, option);
    g_object_unref(option);

    return TRUE;
}

static gboolean
decode (MilterDecoder *decoder, GError **error)
{
    gboolean success = TRUE;
    gchar command;

    command = milter_decoder_get_buffer(decoder)[0];
    switch (command) {
    case MILTER_COMMAND_NEGOTIATE:
        success = decode_negotiate(decoder, error);
        break;
    case MILTER_COMMAND_DEFINE_MACRO:
        success = decode_define_macro(decoder, error);
        break;
    case MILTER_COMMAND_CONNECT:
        success = decode_connect(decoder, error);
        break;
    case MILTER_COMMAND_HELO:
        success = decode_helo(decoder, error);
        break;
    case MILTER_COMMAND_ENVELOPE_FROM:
        success = decode_envelope_from(decoder, error);
        break;
    case MILTER_COMMAND_ENVELOPE_RECIPIENT:
        success = decode_envelope_recipient(decoder, error);
        break;
    case MILTER_COMMAND_DATA:
        success = decode_data(decoder, error);
        break;
    case MILTER_COMMAND_HEADER:
        success = decode_header(decoder, error);
        break;
    case MILTER_COMMAND_END_OF_HEADER:
        success = decode_end_of_header(decoder, error);
        break;
    case MILTER_COMMAND_BODY:
        success = decode_body(decoder, error);
        break;
    case MILTER_COMMAND_END_OF_MESSAGE:
        success = decode_end_of_message(decoder, error);
        break;
    case MILTER_COMMAND_ABORT:
        success = decode_abort(decoder, error);
        break;
    case MILTER_COMMAND_QUIT:
        success = decode_quit(decoder, error);
        break;
    case MILTER_COMMAND_UNKNOWN:
        success = decode_unknown(decoder, error);
        break;
    default:
        g_set_error(error,
                    MILTER_DECODER_ERROR,
                    MILTER_DECODER_ERROR_UNEXPECTED_COMMAND,
                    "unexpected command was received: %c", command);
        success = FALSE;
        break;
    }

    return success;
}


/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
