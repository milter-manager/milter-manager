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
    gint index = 0;

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

    index++;
    if (argument[1] == '>') {
        index++;
    } else if (argument[1] == '"') {
    } else {
        gint i;
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
        index++;
    }

    while (argument[index] == ' ') {
        gint i, keyword_length, value_length;
        const gchar *keyword, *value = NULL;

        keyword = argument + index + 1;
        if (!keyword[0])
            RETURN_ERROR("parameter keyword is missing: <%s>", argument);
        if (!g_ascii_isalnum(keyword[0]))
            RETURN_ERROR("parameter keyword should start with "
                         "alphabet or number: <%s>", argument);

        for (i = 1;
             keyword[i] && (g_ascii_isalnum(keyword[i]) || keyword[i] == '-');
             i++) {
        }
        keyword_length = i;
        if (keyword[i] == '=') {
            gint j;
            value = keyword + i + 1;
            for (j = 0;
                 value[j] && (g_ascii_isgraph(value[j]) && value[j] != '=');
                 j++) {
            }
            value_length = j;
            i += 1 + j;
        }
        if (parameters) {
            if (!*parameters)
                *parameters = g_hash_table_new_full(g_str_hash,
                                                    g_str_equal,
                                                    g_free,
                                                    g_free);
            g_hash_table_insert(*parameters,
                                g_strndup(keyword, keyword_length),
                                value ? g_strndup(value, value_length) : NULL);
        }
        index += i + 1;
    }

    if (argument[index])
        RETURN_ERROR("there is a garbage in the last: <%s>", argument);

    return TRUE;
}


/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
