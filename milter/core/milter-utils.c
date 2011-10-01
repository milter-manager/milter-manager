/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Copyright (C) 2008-2011  Kouhei Sutou <kou@clear-code.com>
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
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <glib/gprintf.h>

#include "milter-utils.h"

gchar *
milter_utils_inspect_io_condition_error (GIOCondition condition)
{
    GArray *messages;
    gchar *message;

    messages = g_array_new(TRUE, TRUE, sizeof(gchar *));
    if (condition & G_IO_ERR) {
        message = "Error";
        g_array_append_val(messages, message);
    }
    if (condition & G_IO_HUP) {
        message = "Hung up";
        g_array_append_val(messages, message);
    }
    if (condition & G_IO_NVAL) {
        message = "Invalid request";
        g_array_append_val(messages, message);
    }
    message = g_strjoinv(" | ", (gchar **)(messages->data));
    g_array_free(messages, TRUE);

    return message;
}

gchar *
milter_utils_inspect_enum (GType enum_type, gint enum_value)
{
    GEnumClass *enum_class;
    GEnumValue *value;
    gchar *inspected;

    enum_class = g_type_class_ref(enum_type);
    if (!enum_class)
        return g_strdup_printf("unknown enum type: %s(%lu)",
                               g_type_name(enum_type), enum_type);

    value = g_enum_get_value(enum_class, enum_value);
    if (value) {
        inspected = g_strdup_printf("#<%s: %s(%s:%d)>",
                                    g_type_name(enum_type),
                                    value->value_nick,
                                    value->value_name,
                                    value->value);
    } else {
        inspected = g_strdup_printf("#<%s: %d>",
                                    g_type_name(enum_type), enum_value);
    }

    g_type_class_unref(enum_class);

    return inspected;
}

gchar *
milter_utils_get_enum_name (GType enum_type, gint enum_value)
{
    GEnumClass *enum_class;
    GEnumValue *value;
    gchar *name;

    enum_class = g_type_class_ref(enum_type);
    if (!enum_class)
        return g_strdup_printf("unknown enum type: %s(%lu)",
                               g_type_name(enum_type), enum_type);

    value = g_enum_get_value(enum_class, enum_value);
    if (value)
        name = g_strdup(value->value_name);
    else
        name = g_strdup_printf("#<%s: %d>",
                               g_type_name(enum_type), enum_value);

    g_type_class_unref(enum_class);

    return name;
}

gchar *
milter_utils_get_enum_nick_name (GType enum_type, gint enum_value)
{
    GEnumClass *enum_class;
    GEnumValue *value;
    gchar *name;

    enum_class = g_type_class_ref(enum_type);
    if (!enum_class)
        return g_strdup_printf("unknown enum type: %s(%lu)",
                               g_type_name(enum_type), enum_type);

    value = g_enum_get_value(enum_class, enum_value);
    if (value)
        name = g_strdup(value->value_nick);
    else
        name = g_strdup_printf("#<%s: %d>",
                               g_type_name(enum_type), enum_value);

    g_type_class_unref(enum_class);

    return name;
}

gchar *
milter_utils_inspect_flags (GType flags_type, guint flags)
{
    GFlagsClass *flags_class;
    guint i;
    GString *inspected;

    flags_class = g_type_class_ref(flags_type);
    if (!flags_class)
        return g_strdup_printf("unknown flags type: %s(%lu)",
                               g_type_name(flags_type), flags_type);


    inspected = g_string_new(NULL);
    g_string_append_printf(inspected, "#<%s", g_type_name(flags_type));
    if (flags > 0) {
        g_string_append(inspected, ":");
        if (flags & flags_class->mask) {
            g_string_append(inspected, " ");
            for (i = 0; i < flags_class->n_values; i++) {
                GFlagsValue *value = flags_class->values + i;
                if (value->value & flags)
                    g_string_append_printf(inspected, "%s|", value->value_nick);
            }
            inspected->str[inspected->len - 1] = ' ';
            for (i = 0; i < flags_class->n_values; i++) {
                GFlagsValue *value = flags_class->values + i;
                if (value->value & flags)
                    g_string_append_printf(inspected, "(%s:0x%x)|",
                                           value->value_name, value->value);
            }
            g_string_truncate(inspected, inspected->len - 1);
        }

        if (flags & ~(flags_class->mask)) {
            g_string_append_printf(inspected,
                                   " (unknown flags: 0x%x)",
                                   flags & ~(flags_class->mask));
        }
    }
    g_string_append(inspected, ">");

    g_type_class_unref(flags_class);

    return g_string_free(inspected, FALSE);
}

gchar *
milter_utils_get_flags_names (GType flags_type, guint flags)
{
    GFlagsClass *flags_class;
    GString *names;

    flags_class = g_type_class_ref(flags_type);
    if (!flags_class)
        return g_strdup_printf("unknown flags type: %s(%lu)",
                               g_type_name(flags_type), flags_type);

    names = g_string_new(NULL);
    if (flags & flags_class->mask) {
        guint i;
        for (i = 0; i < flags_class->n_values; i++) {
            GFlagsValue *value = flags_class->values + i;
            if (value->value & flags) {
                if (names->len > 0)
                    g_string_append(names, "|");
                g_string_append(names, value->value_nick);
            }
        }
    }

    if (flags & ~(flags_class->mask)) {
        if (names->len > 0)
            g_string_append(names, " ");
        g_string_append_printf(names,
                               "(unknown flags: %s:0x%x)",
                               g_type_name(flags_type),
                               flags & ~(flags_class->mask));
    }

    g_type_class_unref(flags_class);

    return g_string_free(names, FALSE);
}

gchar *
milter_utils_inspect_object (GObject *object)
{
    GString *string;
    guint i, n_properties = 0;
    GParamSpec **param_specs = NULL;

    string = g_string_new(NULL);
    g_string_append_printf(string, "#<%s:%p",
                           G_OBJECT_TYPE_NAME(object),
                           object);
    param_specs = g_object_class_list_properties(G_OBJECT_GET_CLASS(object),
                                                 &n_properties);
    for (i = 0; i < n_properties; i++) {
        GParamSpec *spec = param_specs[i];
        GValue value = {0,};
        gchar *value_string;

        if (i > 0)
            g_string_append(string, ",");

        g_value_init(&value, spec->value_type);
        g_object_get_property(object, spec->name, &value);
        if (G_TYPE_IS_ENUM(spec->value_type)) {
            value_string = milter_utils_inspect_enum(spec->value_type,
                                                     g_value_get_enum(&value));
        } else if (G_TYPE_IS_FLAGS(spec->value_type)) {
            value_string = milter_utils_inspect_flags(spec->value_type,
                                                      g_value_get_flags(&value));
        } else if (G_TYPE_IS_OBJECT(spec->value_type)) {
            /* FIXME: circular check */
            value_string =
                milter_utils_inspect_object(g_value_get_object(&value));
        } else {
            value_string = g_strdup_value_contents(&value);
        }
        g_string_append_printf(string, " %s=<%s>", spec->name, value_string);
        g_free(value_string);
        g_value_unset(&value);
    }
    g_free(param_specs);
    g_string_append(string, ">");

    return g_string_free(string, FALSE);
}

MilterMacroStage
milter_utils_command_to_macro_stage (MilterCommand command)
{
    MilterMacroStage stage;

    switch (command) {
      case MILTER_COMMAND_CONNECT:
        stage = MILTER_MACRO_STAGE_CONNECT;
        break;
      case MILTER_COMMAND_HELO:
        stage = MILTER_MACRO_STAGE_HELO;
        break;
      case MILTER_COMMAND_ENVELOPE_FROM:
        stage = MILTER_MACRO_STAGE_ENVELOPE_FROM;
        break;
      case MILTER_COMMAND_ENVELOPE_RECIPIENT:
        stage = MILTER_MACRO_STAGE_ENVELOPE_RECIPIENT;
        break;
      case MILTER_COMMAND_DATA:
        stage = MILTER_MACRO_STAGE_DATA;
        break;
      case MILTER_COMMAND_END_OF_MESSAGE:
        stage = MILTER_MACRO_STAGE_END_OF_MESSAGE;
        break;
      case MILTER_COMMAND_END_OF_HEADER:
        stage = MILTER_MACRO_STAGE_END_OF_HEADER;
        break;
      default:
        stage = -1;
        break;
    }

    return stage;
}

MilterCommand
milter_utils_macro_stage_to_command (MilterMacroStage stage)
{
    MilterCommand command;

    switch (stage) {
      case MILTER_MACRO_STAGE_CONNECT:
        command = MILTER_COMMAND_CONNECT;
        break;
      case MILTER_MACRO_STAGE_HELO:
        command = MILTER_COMMAND_HELO;
        break;
      case MILTER_MACRO_STAGE_ENVELOPE_FROM:
        command = MILTER_COMMAND_ENVELOPE_FROM;
        break;
      case MILTER_MACRO_STAGE_ENVELOPE_RECIPIENT:
        command = MILTER_COMMAND_ENVELOPE_RECIPIENT;
        break;
      case MILTER_MACRO_STAGE_DATA:
        command = MILTER_COMMAND_DATA;
        break;
      case MILTER_MACRO_STAGE_END_OF_MESSAGE:
        command = MILTER_COMMAND_END_OF_MESSAGE;
        break;
      case MILTER_MACRO_STAGE_END_OF_HEADER:
        command = MILTER_COMMAND_END_OF_HEADER;
        break;
      default:
        command = '\0';
        break;
    }

    return command;
}

void
milter_utils_set_error_with_sub_error (GError **error,
                                       GQuark domain,
                                       gint error_code,
                                       GError *sub_error,
                                       const gchar *format,
                                       ...)
{
    GString *message;
    va_list var_args;
    gchar *buf;
    gint len;

    message = g_string_new(NULL);

    va_start(var_args, format);
    len = g_vasprintf(&buf, format, var_args);
    if (len >= 0) {
        g_string_append_len(message, buf, len);
        g_free(buf);
    }
    va_end(var_args);

    if (sub_error) {
        g_string_append_printf(message,
                               ": %s:%d",
                               g_quark_to_string(sub_error->domain),
                               sub_error->code);
        if (sub_error->message)
            g_string_append_printf(message, ": %s", sub_error->message);

        g_error_free(sub_error);
    }

    g_set_error(error,
                domain, error_code,
                "%s", message->str);
    g_string_free(message, TRUE);
}

gchar *
milter_utils_format_reply_code (guint reply_code,
                                const gchar *extended_code,
                                const gchar *reply_message)
{
    GString *reply;
    gchar **messages, **original_messages;

    if (reply_code == 0)
        return NULL;

    if (!reply_message)
        reply_message = "\n";
    messages = g_strsplit(reply_message, "\n", -1);

    original_messages = messages;
    reply = g_string_new("");
    for (; *messages; messages++) {
        gchar *message = *messages;
        gsize message_size;

        g_string_append_printf(reply, "%3u", reply_code);
        if (extended_code)
            g_string_append_printf(reply, " %s", extended_code);

        message_size = strlen(message);
        if (message_size > 0) {
            if (message[message_size - 1] == '\r')
                message[message_size - 1] = '\0';
            g_string_append_printf(reply, " %s", message);
        }
        if (*(messages + 1))
            g_string_append(reply, "\r\n");
    }
    g_strfreev(original_messages);

    return g_string_free(reply, FALSE);
}

static void
inspect_hash_string_string_element (gpointer _key, gpointer _value,
                                    gpointer user_data)
{
    gchar *key = _key;
    gchar *value = _value;
    GString *inspected = user_data;

    if (key)
        g_string_append_printf(inspected, "\"%s\"", key);
    else
        g_string_append(inspected, "NULL");
    g_string_append(inspected, " => ");
    if (key)
        g_string_append_printf(inspected, "\"%s\"", value);
    else
        g_string_append(inspected, "NULL");
    g_string_append(inspected, ", ");
}

gchar *
milter_utils_inspect_hash_string_string (GHashTable *hash)
{
    GString *inspected;

    inspected = g_string_new("{");
    if (g_hash_table_size(hash) > 0) {
        g_hash_table_foreach(hash,
                             inspect_hash_string_string_element,
                             inspected);
        g_string_truncate(inspected, inspected->len - strlen(", "));
    }
    g_string_append(inspected, "}");

    return g_string_free(inspected, FALSE);
}

static void
merge_hash_string_string_element (gpointer _key, gpointer _value,
                                  gpointer user_data)
{
    gchar *key = _key;
    gchar *value = _value;
    GHashTable *dest = user_data;

    g_hash_table_insert(dest, g_strdup(key), g_strdup(value));
}

void
milter_utils_merge_hash_string_string (GHashTable *dest, GHashTable *src)
{
    if (!src)
        return;

    g_hash_table_foreach(src,
                         merge_hash_string_string_element,
                         dest);
}

gchar *
milter_utils_inspect_list_pointer (const GList *list)
{
    const GList *node;
    GString *inspected;

    inspected = g_string_new("[");
    for (node = list; node; node = g_list_next(node)) {
        g_string_append_printf(inspected, "<%p>, ", node->data);
    }
    if (list)
        g_string_truncate(inspected, inspected->len - strlen(", "));
    g_string_append(inspected, "]");

    return g_string_free(inspected, FALSE);
}

GQuark
milter_flags_error_quark (void)
{
    return g_quark_from_static_string("milter-flags-error-quark");
}

guint
milter_utils_flags_from_string (GType        flags_type,
                                const gchar *flags_string,
                                guint        base_flags,
                                GError     **error)
{
    gchar **split_names, **names;
    GFlagsClass *flags_class;
    guint flags = 0;
    gboolean append = TRUE;
    GString *unknown_strings = NULL;

    if (!flags_string) {
        g_set_error(error,
                    MILTER_FLAGS_ERROR,
                    MILTER_FLAGS_ERROR_NULL_NAME,
                    "flags name is NULL (<%s>)",
                    g_type_name(flags_type));
        return 0;
    }

    switch (flags_string[0]) {
    case '+':
        flags = base_flags;
        flags_string++;
        break;
    case '-':
        append = FALSE;
        flags = base_flags;
        flags_string++;
        break;
    default:
        break;
    }

    split_names = g_strsplit(flags_string, "|", 0);
    flags_class = g_type_class_ref(flags_type);
    for (names = split_names; *names; names++) {
        gchar *name = *names;
        gboolean local_append = append;

        switch (name[0]) {
        case '+':
            local_append = TRUE;
            name++;
            break;
        case '-':
            local_append = FALSE;
            name++;
            break;
        default:
            break;
        }

        if (g_str_equal(name, "all")) {
            if (local_append) {
                flags |= flags_class->mask;
            } else {
                flags = 0;
            }
        } else {
            GFlagsValue *value;

            value = g_flags_get_value_by_nick(flags_class, name);
            if (value) {
                if (local_append) {
                    flags |= value->value;
                } else {
                    flags &= ~(value->value);
                }
            } else {
                if (!unknown_strings)
                    unknown_strings = g_string_new(NULL);
                g_string_append_printf(unknown_strings,
                                       "%s<%s>",
                                       unknown_strings->len > 0 ? "|" : "",
                                       name);
            }
        }
    }
    g_type_class_unref(flags_class);
    g_strfreev(split_names);

    if (unknown_strings) {
        g_set_error(error,
                    MILTER_FLAGS_ERROR,
                    MILTER_FLAGS_ERROR_UNKNOWN_NAMES,
                    "unknown flag names: [%s](<%s>)",
                    unknown_strings->str,
                    g_type_name(flags_type));
        g_string_free(unknown_strings, TRUE);
    }

    return flags;
}

GQuark
milter_enum_error_quark (void)
{
    return g_quark_from_static_string("milter-enum-error-quark");
}

gint
milter_utils_enum_from_string (GType        enum_type,
                               const gchar *enum_string,
                               GError     **error)
{
    GEnumClass *enum_class;
    GEnumValue *enum_value;
    gint value = 0;

    if (!enum_string) {
        g_set_error(error,
                    MILTER_ENUM_ERROR,
                    MILTER_ENUM_ERROR_NULL_NAME,
                    "enum name is NULL (<%s>)",
                    g_type_name(enum_type));
        return 0;
    }

    enum_class = g_type_class_ref(enum_type);
    enum_value = g_enum_get_value_by_nick(enum_class, enum_string);
    if (enum_value) {
        value = enum_value->value;
    } else {
        g_set_error(error,
                    MILTER_ENUM_ERROR,
                    MILTER_ENUM_ERROR_UNKNOWN_NAME,
                    "unknown enum name: <%s>(<%s>)",
                    enum_string,
                    g_type_name(enum_type));
    }
    g_type_class_unref(enum_class);

    return value;
}

void
milter_utils_append_indent (GString *string, guint size)
{
    while (size > 0) {
        g_string_append_c(string, ' ');
        size--;
    }
}

void
milter_utils_xml_append_text_element (GString *string,
                                      const gchar *name, const gchar *content,
                                      guint indent)
{
    gchar *escaped_content;

    milter_utils_append_indent(string, indent);
    escaped_content = g_markup_escape_text(content, -1);
    g_string_append_printf(string, "<%s>%s</%s>\n", name, escaped_content, name);
    g_free(escaped_content);
}

void
milter_utils_xml_append_boolean_element (GString *string,
                                         const gchar *name,
                                         gboolean boolean,
                                         guint indent)
{
    milter_utils_xml_append_text_element(string,
                                         name, boolean ? "true" : "false",
                                         indent);
}

void
milter_utils_xml_append_enum_element (GString *string,
                                      const gchar *name,
                                      GType enum_type,
                                      gint enum_value,
                                      guint indent)
{
    gchar *enum_nick_name;

    enum_nick_name = milter_utils_get_enum_nick_name(enum_type, enum_value);
    milter_utils_xml_append_text_element(string,
                                         name, enum_nick_name,
                                         indent);
    g_free(enum_nick_name);
}

gint
milter_utils_strcmp0 (const char *str1,
                      const char *str2)
{
    if (!str1)
        return -(str1 != str2);
    if (!str2)
        return str1 != str2;
    return strcmp(str1, str2);
}

gboolean
milter_utils_detach_io (gchar **message)
{
    gboolean success = FALSE;
    int null_stdin_fd = -1;
    int null_stdout_fd = -1;
    int null_stderr_fd = -1;

    null_stdin_fd = open("/dev/null", O_RDONLY);
    if (null_stdin_fd == -1) {
        *message = g_strdup_printf("failed to open /dev/null for STDIN: %s",
                                   g_strerror(errno));
        goto cleanup;
    }

    null_stdout_fd = open("/dev/null", O_WRONLY);
    if (null_stdout_fd == -1) {
        *message = g_strdup_printf("failed to open /dev/null for STDOUT: %s",
                                   g_strerror(errno));
        goto cleanup;
    }

    null_stderr_fd = open("/dev/null", O_WRONLY);
    if (null_stderr_fd == -1) {
        *message = g_strdup_printf("failed to open /dev/null for STDERR: %s",
                                   g_strerror(errno));
        goto cleanup;
    }

    if (dup2(null_stdin_fd, STDIN_FILENO) == -1) {
        *message = g_strdup_printf("failed to detach STDIN: %s",
                                   g_strerror(errno));
        goto cleanup;
    }

    if (dup2(null_stdout_fd, STDOUT_FILENO) == -1) {
        *message = g_strdup_printf("failed to detach STDOUT: %s",
                                   g_strerror(errno));
        goto cleanup;
    }

    if (dup2(null_stderr_fd, STDERR_FILENO) == -1) {
        *message = g_strdup_printf("failed to detach STDERR: %s",
                                   g_strerror(errno));
        goto cleanup;
    }

    success = TRUE;

cleanup:
    if (null_stdin_fd != -1)
        close(null_stdin_fd);
    if (null_stdout_fd != -1)
        close(null_stdout_fd);
    if (null_stderr_fd != -1)
        close(null_stderr_fd);

    return success;
}

gboolean
milter_utils_guess_console_color_usability (void)
{
    const gchar *term;

    term = g_getenv("TERM");
    if (term && (g_str_has_suffix(term, "term") ||
                 g_str_has_suffix(term, "term-color") ||
                 g_str_equal(term, "screen") ||
                 g_str_equal(term, "linux")))
        return TRUE;

    return FALSE;
}

static gboolean
parse_file_mode_symbol (const gchar *string, guint *mode, gchar **error_message)
{
    guint target = 0;
    guint parsed_mode = 0;
    gint i;

    for (i = 0; string[i]; i++) {
        gboolean break_loop = FALSE;
        switch (string[i]) {
        case 'a':
            target |= 0777;
            break;
        case 'u':
            target |= 0700;
            break;
        case 'g':
            target |= 0070;
            break;
        case 'o':
            target |= 0007;
            break;
        default:
            if (i == 0) {
                if (error_message) {
                    *error_message =
                        g_strdup_printf("invalid target: '%c': <%s>",
                                        string[i], string);
                }
                return FALSE;
            }
            break_loop = TRUE;
            break;
        }
        if (break_loop)
            break;
    }

    if (!string[i]) {
        if (error_message) {
            *error_message = g_strdup_printf("set mode '=' is missing: <%s>",
                                             string);
        }
        return FALSE;
    }

    if (string[i] == '=') {
        i++;
    } else {
        if (error_message) {
            *error_message = g_strdup_printf("set mode must be '=': '%c': <%s>",
                                             string[i], string);
        }
        return FALSE;
    }

    if (!string[i]) {
        if (error_message) {
            *error_message =
                g_strdup_printf("mode 'r', 'w' or 'x' is missing: <%s>",
                                string);
        }
        return FALSE;
    }

    for (; string[i]; i++) {
        switch (string[i]) {
        case 'r':
            parsed_mode |= target & 0444;
            break;
        case 'w':
            parsed_mode |= target & 0222;
            break;
        case 'x':
            parsed_mode |= target & 0111;
            break;
        default:
            if (error_message) {
                *error_message =
                    g_strdup_printf("mode must be 'r', 'w' or 'x': '%c': <%s>",
                                    string[i], string);
            }
            return FALSE;
            break;
        }
    }

    *mode = parsed_mode;
    return TRUE;
}

static gboolean
parse_file_mode_numeric (const gchar *string, guint *mode, gchar **error_message)
{
    guint parsed_mode = 0;
    gint i;

    for (i = 0; string[i]; i++) {
        if (i > 2) {
            if (error_message) {
                *error_message =
                    g_strdup_printf("must be at most 3 digits: <%s>",
                                    string);
            }
            return FALSE;
        }

        if ('0' <= string[i] && string[i] <= '7') {
            parsed_mode <<= 3;
            parsed_mode |= string[i] - '0';
        } else {
            if (error_message) {
                *error_message = g_strdup_printf("must be 0-7: '%c': <%s>",
                                                 string[i], string);
            }
            return FALSE;
        }
    }

    *mode = parsed_mode;
    return TRUE;
}

gboolean
milter_utils_parse_file_mode (const gchar *string,
                              guint *mode, gchar **error_message)
{
    gboolean success = TRUE;

    if (!string[0]) {
        *mode = 0;
    } else if (string[0] == 'a' || string[0] == 'u' ||
               string[0] == 'g' || string[0] == 'o') {
        success = parse_file_mode_symbol(string, mode, error_message);
    } else if ('0' <= string[0] && string[0] <= '7') {
        success = parse_file_mode_numeric(string, mode, error_message);
    } else {
        if (error_message) {
            *error_message = g_strdup_printf("invalid character: '%c': <%s>",
                                             string[0], string);
        }
        success = FALSE;
    }

    return success;
}

#if GLIB_CHECK_VERSION(2, 14, 0)
GList *
milter_utils_hash_table_get_keys (GHashTable *table)
{
    return g_hash_table_get_keys(table);
}
#else
static void
collect_key (gpointer key, gpointer value, gpointer user_data)
{
    GList **keys = (GList **)user_data;

    *keys = g_list_prepend(*keys, key);
}

GList *
milter_utils_hash_table_get_keys (GHashTable *table)
{
    GList *keys = NULL;

    g_hash_table_foreach(table, collect_key, &keys);

    return keys;
}
#endif

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
