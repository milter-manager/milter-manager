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

#ifndef __MC_PARSER_H__
#define __MC_PARSER_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define MC_PARSER_ERROR           (mc_parser_error_quark())

#define MC_TYPE_PARSER            (mc_parser_get_type ())
#define MC_PARSER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MC_TYPE_PARSER, MCParser))
#define MC_PARSER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MC_TYPE_PARSER, MCParserClass))
#define MC_IS_PARSER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MC_TYPE_PARSER))
#define MC_IS_PARSER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MC_TYPE_PARSER))
#define MC_PARSER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), MC_TYPE_PARSER, MCParserClass))

typedef enum
{
    MC_PARSER_ERROR_SHORT_COMMAND_LENGTH,
    MC_PARSER_ERROR_UNKNOWN_MACRO_CONTEXT
} MCParserError;

typedef enum
{
    MC_CONTEXT_TYPE_CONNECT,
    MC_CONTEXT_TYPE_HELO,
    MC_CONTEXT_TYPE_MAIL
} McContextType;

typedef struct _MCParser         MCParser;
typedef struct _MCParserClass    MCParserClass;

struct _MCParser
{
    GObject object;
};

struct _MCParserClass
{
    GObjectClass parent_class;

    void (*abort)               (MCParser *parser);
    void (*connect)             (MCParser *parser);
    void (*define_macro)        (MCParser *parser,
                                 McContextType context,
                                 GHashTable *macros);
    void (*option_negotiation)  (MCParser *parser);
};

GQuark           mc_parser_error_quark       (void);

GType            mc_parser_get_type          (void) G_GNUC_CONST;

MCParser        *mc_parser_new               (void);

gboolean         mc_parser_parse             (MCParser        *parser,
                                              const gchar     *text,
                                              gsize            text_len,
                                              GError         **error);
gboolean         mc_parser_end_parse         (MCParser        *parser,
                                              GError         **error);

G_END_DECLS

#endif /* __MC_PARSER_H__ */

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
