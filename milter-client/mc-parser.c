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

#include "mc-parser.h"
#include "mc-enum-types.h"
#include "mc-marshalers.h"

#define COMMAND_BYTES_LENGTH (4)
#define OPTION_LENGTH (4)

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


#define MC_PARSER_GET_PRIVATE(obj)                                      \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), MC_TYPE_PARSER, MCParserPrivate))

typedef struct _MCParserPrivate	MCParserPrivate;
struct _MCParserPrivate
{
    gint state;
    GString *buffer;
    gint32 command_bytes;
};

typedef enum {
    IN_START,
    IN_COMMAND_BYTES,
    IN_EOF,
    IN_ERROR
} ParseState;

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
    LAST_SIGNAL
};

static gint signals[LAST_SIGNAL] = {0};

G_DEFINE_TYPE(MCParser, mc_parser, G_TYPE_OBJECT);

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
mc_parser_class_init (MCParserClass *klass)
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
                     G_STRUCT_OFFSET(MCParserClass, option_negotiation),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__VOID,
                     G_TYPE_NONE, 0);

    signals[DEFINE_MACRO] =
        g_signal_new("define-macro",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MCParserClass, define_macro),
                     NULL, NULL,
                     _mc_marshal_VOID__ENUM_POINTER,
                     G_TYPE_NONE, 2, MC_TYPE_CONTEXT_TYPE, G_TYPE_POINTER);

    signals[CONNECT] =
        g_signal_new("connect",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MCParserClass, connect),
                     NULL, NULL,
                     _mc_marshal_VOID__STRING_POINTER_INT,
                     G_TYPE_NONE, 3, G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_INT);

    signals[HELO] =
        g_signal_new("helo",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MCParserClass, helo),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__STRING,
                     G_TYPE_NONE, 1, G_TYPE_STRING);

    signals[MAIL] =
        g_signal_new("mail",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MCParserClass, mail),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__STRING,
                     G_TYPE_NONE, 1, G_TYPE_STRING);

    signals[RCPT] =
        g_signal_new("rcpt",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MCParserClass, rcpt),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__STRING,
                     G_TYPE_NONE, 1, G_TYPE_STRING);

    signals[HEADER] =
        g_signal_new("header",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MCParserClass, header),
                     NULL, NULL,
                     _mc_marshal_VOID__STRING_STRING,
                     G_TYPE_NONE, 2, G_TYPE_STRING, G_TYPE_STRING);

    signals[END_OF_HEADER] =
        g_signal_new("end-of-header",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MCParserClass, end_of_header),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__VOID,
                     G_TYPE_NONE, 0);

    signals[BODY] =
        g_signal_new("body",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MCParserClass, body),
                     NULL, NULL,
#if GLIB_SIZEOF_SIZE_T == 8
                     _mc_marshal_VOID__STRING_UINT64,
                     G_TYPE_NONE, 2, G_TYPE_STRING, G_TYPE_UINT64
#else
                     _mc_marshal_VOID__STRING_UINT,
                     G_TYPE_NONE, 2, G_TYPE_STRING, G_TYPE_UINT
#endif
            );

    signals[END_OF_MESSAGE] =
        g_signal_new("end-of-message",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MCParserClass, end_of_message),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__VOID,
                     G_TYPE_NONE, 0);

    signals[ABORT] =
        g_signal_new("abort",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MCParserClass, abort),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__VOID,
                     G_TYPE_NONE, 0);

    signals[QUIT] =
        g_signal_new("quit",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MCParserClass, quit),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__VOID,
                     G_TYPE_NONE, 0);

    g_type_class_add_private(gobject_class, sizeof(MCParserPrivate));
}

static void
mc_parser_init (MCParser *parser)
{
    MCParserPrivate *priv = MC_PARSER_GET_PRIVATE(parser);

    priv->state = IN_START;
    priv->buffer = g_string_new(NULL);
}

static void
dispose (GObject *object)
{
    MCParserPrivate *priv;

    priv = MC_PARSER_GET_PRIVATE(object);
    if (priv->buffer) {
        g_string_free(priv->buffer, TRUE);
        priv->buffer = NULL;
    }

    G_OBJECT_CLASS(mc_parser_parent_class)->dispose(object);
}

static void
set_property (GObject      *object,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
    MCParserPrivate *priv;

    priv = MC_PARSER_GET_PRIVATE(object);
    switch (prop_id) {
/*       case PROP_RUN_CONTEXT: */
/*         if (priv->run_context) { */
/*             g_object_unref(priv->run_context); */
/*             priv->run_context = NULL; */
/*         } */
/*         priv->run_context = g_value_get_object(value); */
/*         if (priv->run_context) */
/*             g_object_ref(priv->run_context); */
/*         break; */
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
    MCParserPrivate *priv;

    priv = MC_PARSER_GET_PRIVATE(object);
    switch (prop_id) {
/*       case PROP_RUN_CONTEXT: */
/*         g_value_set_object(value, priv->run_context); */
/*         break; */
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

GQuark
mc_parser_error_quark (void)
{
    return g_quark_from_static_string("mc-parser-error-quark");
}

MCParser *
mc_parser_new (void)
{
    return g_object_new(MC_TYPE_PARSER,
                        /* "run-context", run_context, */
                        NULL);
}

static GHashTable *
parse_define_macro (const gchar *buffer, gint length, GError **error)
{
    GHashTable *macros;
    const gchar *last;

    macros = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);

    last = buffer + length;
    while (buffer <= last) {
        const gchar *start, *key, *value;

        start = buffer;
        while (buffer <= last && *buffer != '\0') {
            buffer++;
        }
        key = start;

        buffer++;
        start = buffer;
        while (buffer <= last && *buffer != '\0') {
            buffer++;
        }
        value = start;
        buffer++;

        if (*key) {
            gchar *normalized_key;
            gint key_length;

            key_length = value - key - 1;
            if (key[0] == '{' && key[key_length - 1] == '}')
                normalized_key = g_strndup(key + 1, key_length - 2);
            else
                normalized_key = g_strdup(key);

            g_hash_table_insert(macros,
                                normalized_key,
                                g_strndup(value, buffer - value - 1));
        }
    }

    return macros;
}

static gboolean
parse_macro_context (const gchar macro_context, McContextType *context,
                     GError **error)
{
    gboolean success = TRUE;

    switch (macro_context) {
      case COMMAND_CONNECT:
        *context = MC_CONTEXT_TYPE_CONNECT;
        break;
      case COMMAND_HELO:
        *context = MC_CONTEXT_TYPE_HELO;
        break;
      case COMMAND_MAIL:
        *context = MC_CONTEXT_TYPE_MAIL;
        break;
      case COMMAND_RCPT:
        *context = MC_CONTEXT_TYPE_RCPT;
        break;
      case COMMAND_HEADER:
        *context = MC_CONTEXT_TYPE_HEADER;
        break;
      case COMMAND_END_OF_HEADER:
        *context = MC_CONTEXT_TYPE_END_OF_HEADER;
        break;
      case COMMAND_BODY:
        *context = MC_CONTEXT_TYPE_BODY;
        break;
      case COMMAND_END_OF_MESSAGE:
        *context = MC_CONTEXT_TYPE_END_OF_MESSAGE;
        break;
      default:
        g_set_error(error,
                    MC_PARSER_ERROR,
                    MC_PARSER_ERROR_UNKNOWN_MACRO_CONTEXT,
                    "unknown macro context: %c", macro_context);
        success = FALSE;
        break;
    }

    return success;
}

static gboolean
parse_connect(const gchar *buffer, gint length,
              gchar **host_name,
              struct sockaddr **address, socklen_t *address_length,
              GError **error)
{
    gchar family;
    gint i;
    uint16_t port;
    const gchar *parsed_host_name;

    parsed_host_name = buffer;
    i = 0;
    while (i < length && buffer[i] != '\0')
        i++;

    if (i == length) {
        gchar *terminated_host_name;
        terminated_host_name = g_strndup(parsed_host_name, i + 1);
        terminated_host_name[i + 1] = '\0';
        g_set_error(error,
                    MC_PARSER_ERROR,
                    MC_PARSER_ERROR_CONNECT_HOST_NAME_UNTERMINATED,
                    "host name on connect is unterminated: %s",
                    terminated_host_name);
        g_free(terminated_host_name);
        return FALSE;
    }

    i++;
    family = buffer[i];
    i++;
    if (family != FAMILY_UNKNOWN) {
        if (i + sizeof(port) >= length) {
            g_set_error(error,
                        MC_PARSER_ERROR,
                        MC_PARSER_ERROR_CONNECT_PORT_MISSING,
                        "port number on connect is missing: "
                        "%s: %d (required: >= %lu)",
                        parsed_host_name, length, i + sizeof(port));
            return FALSE;
        }
        memcpy(&port, buffer + i, sizeof(port));
        i += sizeof(port);
    } else {
        g_set_error(error,
                    MC_PARSER_ERROR,
                    MC_PARSER_ERROR_CONNECT_UNKNOWN_FAMILY,
                    "unknown family on connect: %s: %c",
                    parsed_host_name, family);
        return FALSE;
    }

    if (buffer[length] != '\0') {
        g_set_error(error,
                    MC_PARSER_ERROR,
                    MC_PARSER_ERROR_CONNECT_SEPARATOR_MISSING,
                    "NULL separator at the end of connect packet is missing: %s",
                    parsed_host_name);
        return FALSE;
    }

    switch (family) {
      case FAMILY_INET:
        {
            struct sockaddr_in *address_in;
            struct in_addr ip_address;

            if (inet_aton(buffer + i, &ip_address) == -1) {
                g_set_error(error,
                            MC_PARSER_ERROR,
                            MC_PARSER_ERROR_CONNECT_INVALID_INET_ADDRESS,
                            "invalid IPv4 address on connect: %s: %s",
                            parsed_host_name, buffer + i);
                return FALSE;
            }

            *address_length = sizeof(struct sockaddr_in);
            address_in = g_malloc(*address_length);
            address_in->sin_family = AF_INET;
            address_in->sin_port = port;
            address_in->sin_addr = ip_address;
            *address = (struct sockaddr *)address_in;
        }
        break;
      case FAMILY_INET6:
        {
            struct sockaddr_in6 *address_in6;
            struct in6_addr ipv6_address;

            if (inet_pton(AF_INET6, buffer + i, &ipv6_address) == -1) {
                g_set_error(error,
                            MC_PARSER_ERROR,
                            MC_PARSER_ERROR_CONNECT_INVALID_INET6_ADDRESS,
                            "invalid IPv6 address on connect: %s: %s",
                            parsed_host_name, buffer + i);
                return FALSE;
            }

            *address_length = sizeof(struct sockaddr_in6);
            address_in6 = g_malloc(*address_length);
            address_in6->sin6_family = AF_INET6;
            address_in6->sin6_port = port;
            address_in6->sin6_addr = ipv6_address;
            *address = (struct sockaddr *)address_in6;
        }
        break;
      case FAMILY_UNIX:
        {
            struct sockaddr_un *address_un;

            *address_length = sizeof(struct sockaddr_un);
            address_un = g_malloc(*address_length);
            address_un->sun_family = AF_UNIX;
            strcpy(address_un->sun_path, buffer + i);
            *address = (struct sockaddr *)address_un;
        }
        break;
      default:
        g_set_error(error,
                    MC_PARSER_ERROR,
                    MC_PARSER_ERROR_CONNECT_UNKNOWN_FAMILY,
                    "unknown family on connect: %s: %c",
                    parsed_host_name, family);
        return FALSE;
        break;
    }

    *host_name = g_strdup(parsed_host_name);
    return TRUE;
}

static gboolean
parse_header (const gchar *buffer, gint length,
              gchar **name, gchar **value, GError **error)
{
    gint i;

    for (i = 0; i < length && buffer[i] != '\0'; i++) {
        /* do nothing */
    }
    if (i == length) {
        g_set_error(error,
                    MC_PARSER_ERROR,
                    MC_PARSER_ERROR_HEADER_MISSING_NULL,
                    "name terminate NULL is missing");
        return FALSE;
    }

    if (buffer[length] != '\0') {
        g_set_error(error,
                    MC_PARSER_ERROR,
                    MC_PARSER_ERROR_HEADER_MISSING_NULL,
                    "last NULL is missing");
        return FALSE;
    }

    *name = g_strdup(buffer);
    *value = g_strdup(buffer + i + 1);
    return TRUE;
}

static gboolean
parse_command (MCParser *parser, GError **error)
{
    MCParserPrivate *priv;
    gboolean success = TRUE;

    priv = MC_PARSER_GET_PRIVATE(parser);

    switch (priv->buffer->str[0]) {
      case COMMAND_OPTION_NEGOTIATION:
        g_signal_emit(parser, signals[OPTION_NEGOTIATION], 0);
        priv->state = IN_START;
        g_string_erase(priv->buffer, 0, priv->command_bytes);
        break;
      case COMMAND_DEFINE_MACRO:
        {
            GHashTable *macros;
            McContextType context;

            if (parse_macro_context(priv->buffer->str[1], &context, error)) {
                macros = parse_define_macro(priv->buffer->str + 1 + 1,
                                            priv->command_bytes - 1 - 1,
                                            error);
                if (macros) {
                    g_signal_emit(parser, signals[DEFINE_MACRO], 0,
                                  context, macros);
                    g_hash_table_unref(macros);
                    priv->state = IN_START;
                    g_string_erase(priv->buffer, 0, priv->command_bytes);
                } else {
                    success = FALSE;
                }
            } else {
                success = FALSE;
            }
        }
        break;
      case COMMAND_CONNECT:
        {
            gchar *host_name;
            struct sockaddr *address;
            socklen_t length;

            if (parse_connect(priv->buffer->str + 1, priv->command_bytes - 1,
                              &host_name, &address, &length, error)) {
                g_signal_emit(parser, signals[CONNECT], 0,
                              host_name, address, length);
                g_free(host_name);
                g_free(address);
                priv->state = IN_START;
                g_string_erase(priv->buffer, 0, priv->command_bytes);
            } else {
                success = FALSE;
            }
        }
        break;
      case COMMAND_HELO:
        {
            if (priv->buffer->str[priv->command_bytes] == '\0') {
                g_signal_emit(parser, signals[HELO], 0, priv->buffer->str + 1);
                priv->state = IN_START;
                g_string_erase(priv->buffer, 0, priv->command_bytes);
            } else {
                gchar *terminated_fqdn;

                terminated_fqdn = g_strndup(priv->buffer->str + 1,
                                            priv->command_bytes + 1);
                terminated_fqdn[priv->command_bytes + 1] = '\0';
                g_set_error(error,
                            MC_PARSER_ERROR,
                            MC_PARSER_ERROR_HELO_MISSING_NULL,
                            "missing last NULL on HELO: %s",
                            terminated_fqdn);
                g_free(terminated_fqdn);
                success = FALSE;
            }
        }
        break;
      case COMMAND_MAIL:
        {
            if (priv->buffer->str[priv->command_bytes] == '\0') {
                g_signal_emit(parser, signals[MAIL], 0, priv->buffer->str + 1);
                priv->state = IN_START;
                g_string_erase(priv->buffer, 0, priv->command_bytes);
            } else {
                gchar *terminated_from;

                terminated_from = g_strndup(priv->buffer->str + 1,
                                            priv->command_bytes + 1);
                terminated_from[priv->command_bytes + 1] = '\0';
                g_set_error(error,
                            MC_PARSER_ERROR,
                            MC_PARSER_ERROR_MAIL_MISSING_NULL,
                            "missing the last NULL on MAIL: %s",
                            terminated_from);
                g_free(terminated_from);
                success = FALSE;
            }
        }
        break;
      case COMMAND_RCPT:
        {
            if (priv->buffer->str[priv->command_bytes] == '\0') {
                g_signal_emit(parser, signals[RCPT], 0, priv->buffer->str + 1);
                priv->state = IN_START;
                g_string_erase(priv->buffer, 0, priv->command_bytes);
            } else {
                gchar *terminated_to;

                terminated_to = g_strndup(priv->buffer->str + 1,
                                          priv->command_bytes + 1);
                terminated_to[priv->command_bytes + 1] = '\0';
                g_set_error(error,
                            MC_PARSER_ERROR,
                            MC_PARSER_ERROR_RCPT_MISSING_NULL,
                            "missing the last NULL on RCPT: %s",
                            terminated_to);
                g_free(terminated_to);
                success = FALSE;
            }
        }
        break;
      case COMMAND_HEADER:
        {
            gchar *name, *value;
            if (parse_header(priv->buffer->str + 1,
                             priv->command_bytes - 1,
                             &name, &value, error)) {
                g_signal_emit(parser, signals[HEADER], 0, name, value);
                priv->state = IN_START;
                g_string_erase(priv->buffer, 0, priv->command_bytes);
            } else {
                success = FALSE;
            }
        }
        break;
      case COMMAND_END_OF_HEADER:
        {
            if (priv->command_bytes == 1) {
                g_signal_emit(parser, signals[END_OF_HEADER], 0);
                priv->state = IN_START;
                g_string_erase(priv->buffer, 0, priv->command_bytes);
            } else {
                g_set_error(error,
                            MC_PARSER_ERROR,
                            MC_PARSER_ERROR_LONG_COMMAND_LENGTH,
                            "too long command length on EOH: %d: expected: %d",
                            priv->command_bytes, 1);
                success = FALSE;
            }
        }
        break;
      case COMMAND_BODY:
        g_signal_emit(parser, signals[BODY], 0,
                      priv->buffer->str + 1, priv->command_bytes - 1);
        priv->state = IN_START;
        g_string_erase(priv->buffer, 0, priv->command_bytes);
        break;
      case COMMAND_END_OF_MESSAGE:
        if (priv->command_bytes > 1) {
            g_signal_emit(parser, signals[BODY], 0,
                          priv->buffer->str + 1, priv->command_bytes - 1);
        }
        g_signal_emit(parser, signals[END_OF_MESSAGE], 0);
        priv->state = IN_START;
        g_string_erase(priv->buffer, 0, priv->command_bytes);
        break;
      case COMMAND_ABORT:
        if (priv->command_bytes == 1) {
            g_signal_emit(parser, signals[ABORT], 0);
            priv->state = IN_START;
            g_string_erase(priv->buffer, 0, priv->command_bytes);
        } else {
            g_set_error(error,
                        MC_PARSER_ERROR,
                        MC_PARSER_ERROR_LONG_COMMAND_LENGTH,
                        "too long command length on ABORT: %d: expected: %d",
                        priv->command_bytes, 1);
            success = FALSE;
        }
        break;
      case COMMAND_QUIT:
        if (priv->command_bytes == 1) {
            g_signal_emit(parser, signals[QUIT], 0);
            priv->state = IN_START;
            g_string_erase(priv->buffer, 0, priv->command_bytes);
        } else {
            g_set_error(error,
                        MC_PARSER_ERROR,
                        MC_PARSER_ERROR_LONG_COMMAND_LENGTH,
                        "too long command length on QUIT: %d: expected: %d",
                        priv->command_bytes, 1);
            success = FALSE;
        }
        break;
      default:
        success = FALSE;
        break;
    }

    if (!success) {
        priv->state = IN_ERROR;
    }

    return success;
}

gboolean
mc_parser_parse (MCParser *parser, const gchar *text, gsize text_len,
                 GError **error)
{
    MCParserPrivate *priv;
    gboolean loop = TRUE;
    gboolean success = TRUE;

    priv = MC_PARSER_GET_PRIVATE(parser);
    if (priv->state == IN_ERROR) {
        g_set_error(error,
                    MC_PARSER_ERROR,
                    MC_PARSER_ERROR_ALREADY_INVALID,
                    "input is already invalid");
        return FALSE;
    }
    if (priv->state == IN_EOF) {
        /* g_error_set(); */
        return FALSE;
    }

    if (text_len < 0)
        text_len = strlen(text);

    g_string_append_len(priv->buffer, text, text_len);
    while (loop) {
        switch (priv->state) {
          case IN_START:
            if (priv->buffer->len < COMMAND_BYTES_LENGTH) {
                loop = FALSE;
            } else {
                memcpy(&priv->command_bytes,
                       priv->buffer->str,
                       COMMAND_BYTES_LENGTH);
                priv->command_bytes = ntohl(priv->command_bytes);
                g_string_erase(priv->buffer, 0, COMMAND_BYTES_LENGTH);
                priv->state = IN_COMMAND_BYTES;
            }
            break;
          case IN_COMMAND_BYTES:
            if (priv->buffer->len < priv->command_bytes) {
                loop = FALSE;
            } else {
                success = parse_command(parser, error);
            }
            break;
          default:
            loop = FALSE;
            break;
        }
    }

    return success;
}

gboolean
mc_parser_end_parse (MCParser *parser, GError **error)
{
    MCParserPrivate *priv = MC_PARSER_GET_PRIVATE(parser);

    if (priv->state == IN_EOF)
        return FALSE;

    priv->state = IN_EOF;
    return TRUE;
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
