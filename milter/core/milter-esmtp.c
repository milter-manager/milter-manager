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
  RFC 2822 (Internet Message Format) 3.2.4. Atom:

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

/*
  RFC 2822 (Internet Message Format) 3.2.1. Primitive Tokens:

  text = %d1-9 /         ; Characters excluding CR and LF
         %d11 /
         %d12 /
         %d14-127 /
         obs-text
 */
#define IS_TEXT(character)                      \
    ((1 <= character && character <= 9) ||      \
     character == 11 ||                         \
     character == 12 ||                         \
     (14 <= character && character <= 127))

#define RETURN_ERROR(...) G_STMT_START                  \
{                                                       \
    g_set_error(error,                                  \
                MILTER_ESMTP_ERROR,                     \
                MILTER_ESMTP_ERROR_INVALID_FORMAT,      \
                __VA_ARGS__);                           \
    return FALSE;                                       \
} G_STMT_END

#define RETURN_ERROR_WITH_POSITION(message, argument, index) G_STMT_START \
{                                                                       \
    GString *argument_with_error_position;                              \
                                                                        \
    argument_with_error_position = g_string_new_len(argument, index);   \
    g_string_append(argument_with_error_position, "|@|");               \
    g_string_append(argument_with_error_position, argument + index);    \
    g_set_error(error,                                                  \
                MILTER_ESMTP_ERROR,                                     \
                MILTER_ESMTP_ERROR_INVALID_FORMAT,                      \
                message ": <%s>",                                       \
                argument_with_error_position->str);                     \
    g_string_free(argument_with_error_position, TRUE);                  \
    return FALSE;                                                       \
} G_STMT_END

static gboolean
parse_domain (const gchar *argument, gint index, gint *parsed_position,
              GError **error)
{
    gint i;
    const gchar *domain;

    domain = argument + index;
    if (domain[0] == '[') {
        i = 1;
        while (TRUE) {
            if (domain[i] == '[' || domain[i] == ']') {
                break;
            } else if (domain[i] == '\\') {
                i++;
                if (IS_TEXT(domain[i])) {
                    i++;
                } else {
                    RETURN_ERROR_WITH_POSITION("invalid quoted character "
                                               "in domain",
                                               argument, index + i);
                }
            } else if (g_ascii_isspace(domain[i])) {
                break;
            } else if (g_ascii_iscntrl(domain[i]) ||
                       g_ascii_isgraph(domain[i])) {
                i++;
            } else {
                break;
            }
        }
        if (domain[i] != ']')
            RETURN_ERROR_WITH_POSITION("terminate ']' is missing in domain",
                                       argument, index + i);
        i++;
    } else {
        i = 0;
        if (!g_ascii_isalnum(domain[i]))
            RETURN_ERROR_WITH_POSITION("domain should start with "
                                       "alphabet or digit",
                                       argument, index + i);
        do {
            i++;
            while (g_ascii_isalnum(domain[i]) || domain[i] == '-') {
                i++;
            }
        } while (domain[i] == '.');
    }

    *parsed_position = i;
    return TRUE;
}

static gboolean
parse_source_route (const gchar *argument, gint index, gint *parsed_position,
                    GError **error)
{
    gint i, domain_parsed_position;
    const gchar *source_route;

    source_route = argument + index;
    i = -1;
    do {
        i++;

        if (source_route[i] != '@')
            break;
        i++;

        if (!parse_domain(argument, index + i, &domain_parsed_position, error))
            return FALSE;
        i += domain_parsed_position;
    } while (source_route[i] == ',');

    *parsed_position = i;
    return TRUE;
}

static gboolean
parse_local_part (const gchar *argument, gint index, gint *parsed_position,
                  GError **error)
{
    gint i;
    const gchar *local_part;

    local_part = argument + index;
    if (local_part[0] == '"') {
        i = 1;
        while (TRUE) {
            if (local_part[i] == '\\') {
                i++;
                if (IS_TEXT(local_part[i])) {
                    i++;
                } else {
                    RETURN_ERROR("invalid quoted character: <%s>: <0x%x>",
                                 argument, local_part[i]);
                }
            } else if (local_part[i] == '"') {
                break;
            } else if (g_ascii_isgraph(local_part[i])) {
                i++;
            } else {
                break;
            }
        }
        if (local_part[i] != '"')
            RETURN_ERROR("end quote for local part is missing: <%s>",
                         argument);
        i++;
    } else {
        i = -1;
        do {
            i++;
            for (; local_part[i] && IS_ATOM_TEXT(local_part[i]); i++) {
            }
        } while (local_part[i] == '.');
    }

    *parsed_position = i;
    return TRUE;
}

static gboolean
parse_mailbox (const gchar *argument, gint index, gint *parsed_position,
               GError **error)
{
    gint i = 0;
    gint local_part_parsed_position, domain_parsed_position;

    if (!parse_local_part(argument, index, &local_part_parsed_position, error))
        return FALSE;
    i += local_part_parsed_position;

    if (argument[index + i] != '@')
        RETURN_ERROR_WITH_POSITION("'@' is missing in path",
                                   argument, index + i);
    i++;

    if (!argument[index + i])
        RETURN_ERROR_WITH_POSITION("domain is missing in path",
                                   argument, index + i);

    if (!parse_domain(argument, index + i, &domain_parsed_position, error))
        return FALSE;
    i += domain_parsed_position;

    *parsed_position = i;
    return TRUE;
}

gboolean
milter_esmtp_parse_mail_from_argument (const gchar  *argument,
                                       gchar       **path,
                                       GHashTable  **parameters,
                                       GError      **error)
{
    gint index = 0;

    if (!argument)
        RETURN_ERROR("argument should not be NULL");

    if (argument[0] != '<')
        RETURN_ERROR_WITH_POSITION("argument should start with '<'",
                                   argument, 0);

    index++;
    if (argument[1] == '>') {
        index++;
    } else {
        gint parsed_position = 0;

        if (!parse_source_route(argument, index, &parsed_position, error))
            return FALSE;
        if (parsed_position > 0) {
            if (argument[index + parsed_position] != ':')
                RETURN_ERROR_WITH_POSITION("separator ':' is missing "
                                           "after source route",
                                           argument, index + parsed_position);
            parsed_position++;
        }
        index += parsed_position;

        if (!parse_mailbox(argument, index, &parsed_position, error))
            return FALSE;
        index += parsed_position;

        if (argument[index] != '>')
            RETURN_ERROR_WITH_POSITION("terminate '>' is missing in path",
                                       argument, index);
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
