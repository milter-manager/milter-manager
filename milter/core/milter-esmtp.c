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
     (14 <= character && character <= 126))

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
                    RETURN_ERROR_WITH_POSITION("invalid quoted character "
                                               "in local part",
                                               argument, index + i);
                }
            } else if (local_part[i] == '"') {
                break;
            } else if (g_ascii_isspace(local_part[i])) {
                break;
            } else if (g_ascii_iscntrl(local_part[i]) ||
                       g_ascii_isgraph(local_part[i])) {
                i++;
            } else {
                break;
            }
        }
        if (local_part[i] != '"')
            RETURN_ERROR_WITH_POSITION("end quote for local part is missing",
                                       argument, index + i);
        i++;
    } else {
        i = -1;
        do {
            i++;
            while (IS_ATOM_TEXT(local_part[i])) {
                i++;
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
        RETURN_ERROR_WITH_POSITION("'@' is missing in mailbox",
                                   argument, index + i);
    i++;

    if (!argument[index + i] || !argument[index + i + 1])
        RETURN_ERROR_WITH_POSITION("domain is missing in mailbox",
                                   argument, index + i);

    if (!parse_domain(argument, index + i, &domain_parsed_position, error))
        return FALSE;
    i += domain_parsed_position;

    *parsed_position = i;
    return TRUE;
}

static gboolean
parse_path (gboolean accept_postmaster_path,
            const gchar *argument, gint index, gint *parsed_position,
            gchar **parsed_path, GError **error)
{
    gint i = 0;
    const gchar *path;
    const gchar postmaster_path[] = "<Postmaster>";

    path = argument + index;
    if (path[i] != '<') {
        *parsed_position = 0;
        return TRUE;
    }

    i++;
    if (path[i] == '>') {
        i++;
    } else if (accept_postmaster_path &&
               g_str_has_prefix(path, postmaster_path)) {
        i = strlen(postmaster_path);
        if (parsed_path)
            *parsed_path = g_strdup(postmaster_path);
    } else {
        gint source_route_parsed_position;
        gint mailbox_parsed_position;

        if (!parse_source_route(argument, index + i,
                                &source_route_parsed_position, error))
            return FALSE;

        i += source_route_parsed_position;
        if (source_route_parsed_position > 0) {
            if (argument[index + i] != ':')
                RETURN_ERROR_WITH_POSITION("separator ':' is missing "
                                           "after source route",
                                           argument, index + i);
            i++;
        }

        if (!parse_mailbox(argument, index + i, &mailbox_parsed_position, error))
            return FALSE;
        i += mailbox_parsed_position;

        if (path[i] != '>')
            RETURN_ERROR_WITH_POSITION("terminate '>' is missing in path",
                                       argument, index + i);
        i++;

        if (parsed_path)
            *parsed_path = g_strndup(path, i);
    }

    *parsed_position = i;
    return TRUE;
}

static gboolean
parse_parameters (const gchar *argument, gint index, gint *parsed_position,
                  GHashTable **parameters, GError **error)
{
    gint local_index = 0;

    while (argument[index + local_index] == ' ') {
        gint i, current_argument_index;
        gint keyword_length, value_length = 0;
        const gchar *keyword, *value = NULL;

        current_argument_index = index + local_index + 1;
        keyword = argument + current_argument_index;
        if (!keyword[0])
            RETURN_ERROR_WITH_POSITION("parameter keyword is missing",
                                       argument, current_argument_index);
        if (!g_ascii_isalnum(keyword[0]))
            RETURN_ERROR_WITH_POSITION("parameter keyword should start with "
                                       "alphabet or digit",
                                       argument, current_argument_index);

        i = 1;
        while (g_ascii_isalnum(keyword[i]) || keyword[i] == '-') {
            i++;
        }
        keyword_length = i;

        if (keyword[i] == '=') {
            gint j = 0;

            value = keyword + i + 1;
            while (g_ascii_isgraph(value[j]) && value[j] != '=') {
                j++;
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

        local_index += i + 1;
    }

    *parsed_position = local_index;
    return TRUE;
}

static void
merge_parameter (gpointer key, gpointer value, gpointer user_data)
{
    GHashTable *destination = user_data;
    g_hash_table_insert(destination, key, value);
}

static void
merge_parameters (GHashTable *destination, GHashTable *source)
{
    g_hash_table_foreach(source, merge_parameter, destination);
}

static gboolean
milter_esmtp_parse_envelope_command_argument (gboolean accept_postmaster_path,
                                              const gchar  *argument,
                                              gchar       **path,
                                              GHashTable  **parameters,
                                              GError      **error)
{
    gint index = 0;
    gint parsed_position;
    gchar *parsed_path = NULL;
    GHashTable *parsed_parameters = NULL;

    if (!argument)
        RETURN_ERROR("argument should not be NULL");

    if (argument[0] != '<')
        RETURN_ERROR_WITH_POSITION("argument should start with '<'",
                                   argument, 0);

    if (!parse_path(accept_postmaster_path, argument, index, &parsed_position,
                    path ? &parsed_path : NULL,
                    error)) {
        return FALSE;
    }
    index += parsed_position;

    if (!parse_parameters(argument, index, &parsed_position,
                          parameters ? &parsed_parameters : NULL,
                          error)) {
        if (parsed_path)
            g_free(parsed_path);
        if (parsed_parameters)
            g_hash_table_unref(parsed_parameters);
        return FALSE;
    }

    index += parsed_position;

    if (argument[index]) {
        if (parsed_path)
            g_free(parsed_path);
        if (parsed_parameters)
            g_hash_table_unref(parsed_parameters);
        RETURN_ERROR_WITH_POSITION("there is a garbage at the last",
                                   argument, index);
    }

    if (parsed_path)
        *path = parsed_path;

    if (parsed_parameters) {
        if (*parameters) {
            merge_parameters(*parameters, parsed_parameters);
            g_hash_table_unref(parsed_parameters);
        } else {
            *parameters = parsed_parameters;
        }
    }

    return TRUE;
}

gboolean
milter_esmtp_parse_mail_from_argument (const gchar  *argument,
                                       gchar       **path,
                                       GHashTable  **parameters,
                                       GError      **error)
{
    return milter_esmtp_parse_envelope_command_argument(FALSE,
                                                        argument,
                                                        path,
                                                        parameters,
                                                        error);
}

gboolean
milter_esmtp_parse_rcpt_to_argument (const gchar  *argument,
                                     gchar       **path,
                                     GHashTable  **parameters,
                                     GError      **error)
{
    return milter_esmtp_parse_envelope_command_argument(TRUE,
                                                        argument,
                                                        path,
                                                        parameters,
                                                        error);
}


/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
