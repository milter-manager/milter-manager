/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Copyright (C) 2009  Kouhei Sutou <kou@cozmixng.org>
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

#include <string.h>

#include "milter-esmtp.h"

GQuark
milter_esmtp_error_quark (void)
{
    return g_quark_from_static_string("milter-esmtp-error-quark");
}

/*
  RFC 2822 (Internet Message Format)  3.2.4. Atom:

  atext = ALPHA / DIGIT / ; Any character except controls,
          "!" / "#" /     ;  SP, and specials.
          "$" / "%" /     ;  Used for atoms
          "&" / "'" /
          "*" / "+" /
          "-" / "/" /
          "=" / "?" /
          "^" / "_" /
          "`" / "{" /
          "|" / "}" /
          "~"
*/
#define IS_ATOM_TEXT(character)                 \
    (g_ascii_isalnum(character) ||              \
     character == '!' ||                        \
     ('#' <= character && character <= '\'') || \
     character == '*' ||                        \
     character == '+' ||                        \
     character == '-' ||                        \
     character == '/' ||                        \
     character == '=' ||                        \
     character == '?' ||                        \
     ('^' <= character && character <= '`') ||  \
     ('{' <= character && character <= '~'))

gboolean
milter_esmtp_parse_mail_from_argument (const gchar  *argument,
                                       gchar       **path,
                                       GHashTable  **parameters,
                                       GError      **error)
{
#define RETURN_ERROR(...) G_STMT_START {                \
        g_set_error(error,                              \
                    MILTER_ESMTP_ERROR,                 \
                    MILTER_ESMTP_ERROR_INVALID_FORMAT,  \
                    __VA_ARGS__);                       \
        return FALSE;                                   \
} G_STMT_END

    if (!argument)
        RETURN_ERROR("argument should not be NULL");

    if (argument[0] != '<')
        RETURN_ERROR("argument should start with '<': <%s>", argument);

    if (argument[1] == '>') {
    } else if (argument[1] == '"') {
    } else {
        gint i, index = 1;
        const gchar *source_route, *local_part, *domain;

        source_route = argument + index;
        i = -1;
        do {
            i++;
            if (source_route[i] != '@')
                break;

            do {
                i++;
                while (source_route[i] && (g_ascii_isalnum(source_route[i]) || source_route[i] == '-')) {
                    i++;
                }
            } while (source_route[i] == '.');
        } while (source_route[i] == ',');
        if (i > 0) {
            if (source_route[i] != ':')
                RETURN_ERROR("separator ':' is missing after source route: <%s>",
                             argument);
            i++;
        }
        index += i;

        local_part = argument + index;
        for (i = 0; local_part[i] && IS_ATOM_TEXT(local_part[i]); i++) {
        }
        index += i;
        if (argument[index] != '@')
            RETURN_ERROR("'@' is missing in path: <%s>", argument);
        index++;
        domain = argument + index;
        if (!domain[0])
            RETURN_ERROR("domain is missing in path: <%s>", argument);
        if (!g_ascii_isalnum(domain[1]))
            RETURN_ERROR("domain should start with alphabet or digit: <%s>",
                         argument);
        i = 0;
        do {
            i++;
            while (domain[i] && (g_ascii_isalnum(domain[i]) || domain[i] == '-')) {
                i++;
            }
        } while (domain[i] == '.');
        index += i;
        if (domain[i] != '>')
            RETURN_ERROR("terminate '>' is missing in path: <%s>", argument);
        if (path)
            *path = g_strndup(argument + 1, index - 1);
    }

    return TRUE;
}


/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
