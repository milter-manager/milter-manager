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

#ifndef __MILTER_PARSER_H__
#define __MILTER_PARSER_H__

#include <sys/types.h>
#include <sys/socket.h>

#include <glib-object.h>

G_BEGIN_DECLS

#define MILTER_PARSER_ERROR           (milter_parser_error_quark())

#define MILTER_TYPE_PARSER            (milter_parser_get_type())
#define MILTER_PARSER(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), MILTER_TYPE_PARSER, MilterParser))
#define MILTER_PARSER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), MILTER_TYPE_PARSER, MilterParserClass))
#define MILTER_IS_PARSER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), MILTER_TYPE_PARSER))
#define MILTER_IS_PARSER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), MILTER_TYPE_PARSER))
#define MILTER_PARSER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), MILTER_TYPE_PARSER, MilterParserClass))

typedef enum
{
    MILTER_PARSER_ERROR_SHORT_COMMAND_LENGTH,
    MILTER_PARSER_ERROR_LONG_COMMAND_LENGTH,
    MILTER_PARSER_ERROR_UNKNOWN_MACRO_CONTEXT,
    MILTER_PARSER_ERROR_UNEXPECTED_END,
    MILTER_PARSER_ERROR_UNKNOWN_COMMAND,
    MILTER_PARSER_ERROR_MISSING_NULL,
    MILTER_PARSER_ERROR_CONNECT_MISSING_PORT,
    MILTER_PARSER_ERROR_CONNECT_SEPARATOR_MISSING,
    MILTER_PARSER_ERROR_CONNECT_INVALID_INET_ADDRESS,
    MILTER_PARSER_ERROR_CONNECT_INVALID_INET6_ADDRESS,
    MILTER_PARSER_ERROR_CONNECT_UNKNOWN_FAMILY,
    MILTER_PARSER_ERROR_HELO_MISSING_NULL,
    MILTER_PARSER_ERROR_MAIL_MISSING_NULL,
    MILTER_PARSER_ERROR_RCPT_MISSING_NULL,
    MILTER_PARSER_ERROR_HEADER_MISSING_NULL,
    MILTER_PARSER_ERROR_UNKNOWN_MISSING_NULL
} MilterParserError;

typedef enum
{
    MILTER_CONTEXT_TYPE_CONNECT,
    MILTER_CONTEXT_TYPE_HELO,
    MILTER_CONTEXT_TYPE_MAIL,
    MILTER_CONTEXT_TYPE_RCPT,
    MILTER_CONTEXT_TYPE_HEADER,
    MILTER_CONTEXT_TYPE_END_OF_HEADER,
    MILTER_CONTEXT_TYPE_BODY,
    MILTER_CONTEXT_TYPE_END_OF_MESSAGE,
    MILTER_CONTEXT_TYPE_ABORT,
    MILTER_CONTEXT_TYPE_QUIT,
    MILTER_CONTEXT_TYPE_UNKNOWN
} MilterContextType;

typedef struct _MilterParser         MilterParser;
typedef struct _MilterParserClass    MilterParserClass;

struct _MilterParser
{
    GObject object;
};

struct _MilterParserClass
{
    GObjectClass parent_class;

    void (*option_negotiation)  (MilterParser *parser);
    void (*define_macro)        (MilterParser *parser,
                                 MilterContextType context,
                                 GHashTable *macros);
    void (*connect)             (MilterParser *parser,
                                 const gchar *host_name,
                                 const struct sockaddr *address,
                                 socklen_t address_length);
    void (*helo)                (MilterParser *parser,
                                 const gchar *fqdn);
    void (*mail)                (MilterParser *parser,
                                 const gchar *from);
    void (*rcpt)                (MilterParser *parser,
                                 const gchar *to);
    void (*header)              (MilterParser *parser,
                                 const gchar *name,
                                 const gchar *value);
    void (*end_of_header)       (MilterParser *parser);
    void (*body)                (MilterParser *parser,
                                 const gchar *chunk,
                                 gsize length);
    void (*end_of_message)      (MilterParser *parser);
    void (*abort)               (MilterParser *parser);
    void (*quit)                (MilterParser *parser);
    void (*unknown)             (MilterParser *parser,
                                 const gchar *command);
};

GQuark           milter_parser_error_quark       (void);

GType            milter_parser_get_type          (void) G_GNUC_CONST;

MilterParser    *milter_parser_new               (void);

gboolean         milter_parser_parse             (MilterParser    *parser,
                                                  const gchar     *text,
                                                  gsize            text_len,
                                                  GError         **error);
gboolean         milter_parser_end_parse         (MilterParser    *parser,
                                                  GError         **error);

G_END_DECLS

#endif /* __MILTER_PARSER_H__ */

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
