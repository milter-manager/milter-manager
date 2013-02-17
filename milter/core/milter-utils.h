/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Copyright (C) 2008-2013  Kouhei Sutou <kou@clear-code.com>
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

#ifndef __MILTER_UTILS_H__
#define __MILTER_UTILS_H__

#include <sys/types.h>
#include <sys/socket.h>

#include <glib-object.h>

#include <milter/core/milter-protocol.h>
#include <milter/core/milter-error-emittable.h>
#include <milter/core/milter-finished-emittable.h>

G_BEGIN_DECLS

#ifndef G_SOURCE_REMOVE
#  define G_SOURCE_REMOVE   FALSE
#endif

#ifndef G_SOURCE_CONTINUE
#  define G_SOURCE_CONTINUE TRUE
#endif

#define MILTER_IMPLEMENT_ERROR_EMITTABLE(init)                          \
static MilterErrorEmittableClass *error_emittable_parent;               \
static void init (MilterErrorEmittableClass *emittable);                \
static void                                                             \
init (MilterErrorEmittableClass *iface)                                 \
{                                                                       \
    error_emittable_parent = g_type_interface_peek_parent(iface);       \
}

#define MILTER_DEFINE_ERROR_EMITTABLE_TYPE(TypeName, type_name, TYPE_PARENT) \
MILTER_IMPLEMENT_ERROR_EMITTABLE(error_emittable_init)                       \
G_DEFINE_TYPE_WITH_CODE(TypeName, type_name, TYPE_PARENT,                    \
    G_IMPLEMENT_INTERFACE(MILTER_TYPE_ERROR_EMITTABLE, error_emittable_init))

#define MILTER_IMPLEMENT_FINISHED_EMITTABLE_WITH_CODE(init, code)       \
static MilterFinishedEmittableClass *finished_emittable_parent;         \
static void init (MilterFinishedEmittableClass *iface);                 \
static void                                                             \
init (MilterFinishedEmittableClass *iface)                              \
{                                                                       \
    finished_emittable_parent = g_type_interface_peek_parent(iface);    \
    code;                                                               \
}

#define MILTER_IMPLEMENT_FINISHED_EMITTABLE(init)                       \
    MILTER_IMPLEMENT_FINISHED_EMITTABLE_WITH_CODE(init, {})

#define MILTER_DEFINE_FINISHED_EMITTABLE_TYPE(TypeName,                     \
                                              type_name,                    \
                                              TYPE_PARENT)                  \
MILTER_IMPLEMENT_FINISHED_EMITTABLE(finished_emittable_init)                \
G_DEFINE_TYPE_WITH_CODE(TypeName, type_name, TYPE_PARENT,                   \
    G_IMPLEMENT_INTERFACE(MILTER_TYPE_FINISHED_EMITTABLE,                   \
                          finished_emittable_init))

#define MILTER_IMPLEMENT_REPLY_SIGNALS(init)            \
static MilterReplySignalsClass *reply_parent;           \
static void init (MilterReplySignalsClass *reply);      \
static void                                             \
init (MilterReplySignalsClass *reply)                   \
{                                                       \
    reply_parent = g_type_interface_peek_parent(reply); \
}

gchar    *milter_utils_inspect_io_condition_error
                                             (GIOCondition condition);
gchar    *milter_utils_inspect_enum          (GType enum_type,
                                              gint  enum_value);
gchar    *milter_utils_get_enum_name         (GType enum_type,
                                              gint  enum_value);
gchar    *milter_utils_get_enum_nick_name    (GType enum_type,
                                              gint  enum_value);
gchar    *milter_utils_inspect_flags         (GType flags_type,
                                              guint flags);
gchar    *milter_utils_get_flags_names       (GType flags_type,
                                              guint flags);
gchar    *milter_utils_inspect_object        (GObject *object);
gchar    *milter_utils_format_reply_code     (guint reply_code,
                                              const gchar *extended_code,
                                              const gchar *message);
gchar    *milter_utils_inspect_hash_string_string
                                             (GHashTable *hash);
void      milter_utils_merge_hash_string_string
                                             (GHashTable *dest,
                                              GHashTable *src);
gchar    *milter_utils_inspect_list_pointer  (const GList *list);

MilterMacroStage milter_utils_command_to_macro_stage
                                             (MilterCommand command);
MilterCommand    milter_utils_macro_stage_to_command
                                             (MilterMacroStage stage);

void             milter_utils_set_error_with_sub_error
                                             (GError **error,
                                              GQuark domain,
                                              gint error_code,
                                              GError *sub_error,
                                              const gchar *format,
                                              ...);

guint            milter_utils_flags_from_string
                                             (GType        flags_type,
                                              const gchar *flags_string,
                                              guint        base_flags,
                                              GError     **error);
gint             milter_utils_enum_from_string
                                             (GType        enum_type,
                                              const gchar *enum_string,
                                              GError     **error);

void             milter_utils_append_indent  (GString *string,
                                              guint    size);
void             milter_utils_xml_append_text_element
                                             (GString *string,
                                              const gchar *name,
                                              const gchar *content,
                                              guint indent);
void             milter_utils_xml_append_boolean_element
                                             (GString *string,
                                              const gchar *name,
                                              gboolean boolean,
                                              guint indent);
void             milter_utils_xml_append_enum_element
                                             (GString *string,
                                              const gchar *name,
                                              GType enum_type,
                                              gint enum_value,
                                              guint indent);
gint             milter_utils_strcmp0        (const gchar *str1,
                                              const gchar *str2);

gboolean         milter_utils_detach_io      (gchar **message);

gboolean         milter_utils_guess_console_color_usability
                                             (void);
gboolean         milter_utils_parse_file_mode(const gchar  *string,
                                              guint        *mode,
                                              gchar       **error_message);
GList           *milter_utils_hash_table_get_keys
                                             (GHashTable *table);

typedef enum {
    MILTER_UTILS_READ_PIPE,
    MILTER_UTILS_WRITE_PIPE
} MilterUtilsPipeMode;


#define MILTER_ENUM_ERROR           (milter_enum_error_quark())
#define MILTER_FLAGS_ERROR          (milter_flags_error_quark())

typedef enum
{
    MILTER_ENUM_ERROR_NULL_NAME,
    MILTER_ENUM_ERROR_UNKNOWN_NAME
} MilterEnumError;

typedef enum
{
    MILTER_FLAGS_ERROR_NULL_NAME,
    MILTER_FLAGS_ERROR_UNKNOWN_NAMES
} MilterFlagsError;

GQuark           milter_enum_error_quark       (void);
GQuark           milter_flags_error_quark      (void);


G_END_DECLS

#endif /* __MILTER_UTILS_H__ */

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
