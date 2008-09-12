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
#  include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <string.h>
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
#define COMMAND_BODYEOB            'E'     /* final body chunk (End) */
#define COMMAND_HELO               'H'     /* HELO/EHLO */
#define COMMAND_QUIT_NC            'K'     /* QUIT but new connection follows */
#define COMMAND_HEADER             'L'     /* Header */
#define COMMAND_MAIL               'M'     /* MAIL from */
#define COMMAND_EOH                'N'     /* EOH */
#define COMMAND_OPTION_NEGOTIATION 'O'     /* Option negotiation */
#define COMMAND_QUIT               'Q'     /* QUIT */
#define COMMAND_RCPT               'R'     /* RCPT to */
#define COMMAND_DATA               'T'     /* DATA */
#define COMMAND_UNKNOWN            'U'     /* Any unknown command */


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
    IN_EOF
} ParseState;

enum
{
    ABORT,
    CONNECT,
    DEFINE_MACRO,
    OPTION_NEGOTIATION,
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

    signals[ABORT] =
        g_signal_new("abort",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MCParserClass, abort),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__VOID,
                     G_TYPE_NONE, 0);

    signals[CONNECT] =
        g_signal_new("connect",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MCParserClass, connect),
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

    signals[OPTION_NEGOTIATION] =
        g_signal_new("option-negotiation",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MCParserClass, option_negotiation),
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
parse_command (MCParser *parser, GError **error)
{
    MCParserPrivate *priv;
    gboolean success = TRUE;

    priv = MC_PARSER_GET_PRIVATE(parser);

    switch (priv->buffer->str[0]) {
      case COMMAND_ABORT:
        g_signal_emit(parser, signals[ABORT], 0);
        priv->state = IN_START;
        g_string_erase(priv->buffer, 0, priv->command_bytes);
        break;
      case COMMAND_CONNECT:
        g_signal_emit(parser, signals[CONNECT], 0);
        break;
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
                } else {
                    success = FALSE;
                }
            } else {
                success = FALSE;
            }
        }
        priv->state = IN_START;
        g_string_erase(priv->buffer, 0, priv->command_bytes);
        break;
      default:
        success = FALSE;
        break;
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
