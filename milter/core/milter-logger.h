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

#ifndef __MILTER_LOGGER_H__
#define __MILTER_LOGGER_H__

#include <glib-object.h>

#include <milter/core/milter-protocol.h>
#include <milter/core/milter-option.h>

G_BEGIN_DECLS

#define milter_log(level, format, ...)                          \
    do {                                                        \
        if (milter_need_log(level)) {                           \
            (milter_logger_log(milter_logger(),                 \
                               MILTER_LOG_DOMAIN,               \
                               (level),                         \
                               __FILE__,                        \
                               __LINE__,                        \
                               G_STRFUNC,                       \
                               format, ## __VA_ARGS__));        \
        }                                                       \
    } while (0)

#define milter_critical(format, ...)                            \
    milter_log(MILTER_LOG_LEVEL_CRITICAL, format, ## __VA_ARGS__)
#define milter_error(format, ...)                               \
    milter_log(MILTER_LOG_LEVEL_ERROR, format, ## __VA_ARGS__)
#define milter_warning(format, ...)                             \
    milter_log(MILTER_LOG_LEVEL_WARNING, format, ## __VA_ARGS__)
#define milter_message(format, ...)                             \
    milter_log(MILTER_LOG_LEVEL_MESSAGE, format, ## __VA_ARGS__)
#define milter_info(format, ...)                                \
    milter_log(MILTER_LOG_LEVEL_INFO, format, ## __VA_ARGS__)
#define milter_debug(format, ...)                               \
    milter_log(MILTER_LOG_LEVEL_DEBUG, format, ## __VA_ARGS__)
#define milter_trace(format, ...)                               \
    milter_log(MILTER_LOG_LEVEL_TRACE, format, ## __VA_ARGS__)
#define milter_statistics(format, ...)                                  \
    milter_log(MILTER_LOG_LEVEL_STATISTICS, format, ## __VA_ARGS__)
#define milter_profile(format, ...)                                  \
    milter_log(MILTER_LOG_LEVEL_PROFILE, format, ## __VA_ARGS__)

#define milter_set_log_level(level)                             \
    milter_logger_set_target_level(milter_logger(), (level))
#define milter_get_log_level()                          \
    milter_logger_get_resolved_target_level(milter_logger())
#define milter_get_interesting_log_level()                      \
    milter_logger_get_interesting_level(milter_logger())

#define milter_need_log(level) \
    (milter_get_interesting_log_level() & (level))
#define milter_need_critical_log() \
    (milter_need_log(MILTER_LOG_LEVEL_CRITICAL))
#define milter_need_error_log() \
    (milter_need_log(MILTER_LOG_LEVEL_ERROR))
#define milter_need_warning_log() \
    (milter_need_log(MILTER_LOG_LEVEL_WARNING))
#define milter_need_message_log() \
    (milter_need_log(MILTER_LOG_LEVEL_MESSAGE))
#define milter_need_info_log() \
    (milter_need_log(MILTER_LOG_LEVEL_INFO))
#define milter_need_debug_log() \
    (milter_need_log(MILTER_LOG_LEVEL_DEBUG))
#define milter_need_trace_log() \
    (milter_need_log(MILTER_LOG_LEVEL_TRACE))
#define milter_need_statistics_log() \
    (milter_need_log(MILTER_LOG_LEVEL_STATISTICS))
#define milter_need_profile_log() \
    (milter_need_log(MILTER_LOG_LEVEL_PROFILE))

#define MILTER_TYPE_LOGGER            (milter_logger_get_type())
#define MILTER_LOGGER(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), MILTER_TYPE_LOGGER, MilterLogger))
#define MILTER_LOGGER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), MILTER_TYPE_LOGGER, MilterLoggerClass))
#define MILTER_IS_LOGGER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), MILTER_TYPE_LOGGER))
#define MILTER_IS_LOGGER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), MILTER_TYPE_LOGGER))
#define MILTER_LOGGER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), MILTER_TYPE_LOGGER, MilterLoggerClass))

typedef enum
{
    MILTER_LOG_LEVEL_DEFAULT    = 0,
    MILTER_LOG_LEVEL_NONE       = 1 << 0,
    MILTER_LOG_LEVEL_CRITICAL   = 1 << 1,
    MILTER_LOG_LEVEL_ERROR      = 1 << 2,
    MILTER_LOG_LEVEL_WARNING    = 1 << 3,
    MILTER_LOG_LEVEL_MESSAGE    = 1 << 4,
    MILTER_LOG_LEVEL_INFO       = 1 << 5,
    MILTER_LOG_LEVEL_DEBUG      = 1 << 6,
    MILTER_LOG_LEVEL_TRACE      = 1 << 7,
    MILTER_LOG_LEVEL_STATISTICS = 1 << 8,
    MILTER_LOG_LEVEL_PROFILE    = 1 << 9
} MilterLogLevelFlags;

#define MILTER_LOG_LEVEL_ALL (MILTER_LOG_LEVEL_CRITICAL |       \
                              MILTER_LOG_LEVEL_ERROR |          \
                              MILTER_LOG_LEVEL_WARNING |        \
                              MILTER_LOG_LEVEL_MESSAGE |        \
                              MILTER_LOG_LEVEL_INFO |           \
                              MILTER_LOG_LEVEL_DEBUG |          \
                              MILTER_LOG_LEVEL_TRACE |          \
                              MILTER_LOG_LEVEL_STATISTICS |     \
                              MILTER_LOG_LEVEL_PROFILE)

#define MILTER_LOG_NULL_SAFE_STRING(string) ((string) ? (string) : "(null)")

typedef enum
{
    MILTER_LOG_ITEM_DEFAULT     = 0,
    MILTER_LOG_ITEM_NONE        = 1 << 0,
    MILTER_LOG_ITEM_DOMAIN      = 1 << 1,
    MILTER_LOG_ITEM_LEVEL       = 1 << 2,
    MILTER_LOG_ITEM_LOCATION    = 1 << 3,
    MILTER_LOG_ITEM_TIME        = 1 << 4,
    MILTER_LOG_ITEM_NAME        = 1 << 5,
    MILTER_LOG_ITEM_PID         = 1 << 6
} MilterLogItemFlags;

#define MILTER_LOG_ITEM_ALL (MILTER_LOG_ITEM_DOMAIN |   \
                             MILTER_LOG_ITEM_LEVEL |    \
                             MILTER_LOG_ITEM_LOCATION | \
                             MILTER_LOG_ITEM_TIME |     \
                             MILTER_LOG_ITEM_NAME |     \
                             MILTER_LOG_ITEM_PID)

typedef enum
{
    MILTER_LOG_COLORIZE_DEFAULT,
    MILTER_LOG_COLORIZE_NONE,
    MILTER_LOG_COLORIZE_CONSOLE
} MilterLogColorize;

typedef struct _MilterLogger         MilterLogger;
typedef struct _MilterLoggerClass    MilterLoggerClass;

struct _MilterLogger
{
    GObject object;
};

struct _MilterLoggerClass
{
    GObjectClass parent_class;

    void (*log) (MilterLogger        *logger,
                 const gchar         *domain,
                 MilterLogLevelFlags  level,
                 const gchar         *file,
                 guint                line,
                 const gchar         *function,
                 GTimeVal            *time_value,
                 const gchar         *message);
};

GQuark           milter_logger_error_quark    (void);

GType            milter_logger_get_type       (void) G_GNUC_CONST;

MilterLogLevelFlags milter_log_level_flags_from_string (const gchar *level_name,
                                                        MilterLogLevelFlags base_flags,
                                                        GError     **error);
MilterLogItemFlags  milter_log_item_flags_from_string  (const gchar *item_name,
                                                        MilterLogItemFlags base_flags,
                                                        GError     **error);

MilterLogger    *milter_logger                (void);
void             milter_logger_default_log_handler
                                              (MilterLogger        *logger,
                                               const gchar         *domain,
                                               MilterLogLevelFlags  level,
                                               const gchar         *file,
                                               guint                line,
                                               const gchar         *function,
                                               GTimeVal            *time_value,
                                               const gchar         *message,
                                               gpointer             user_data);
MilterLogger    *milter_logger_new            (void);
void             milter_logger_log            (MilterLogger        *logger,
                                               const gchar         *domain,
                                               MilterLogLevelFlags  level,
                                               const gchar         *file,
                                               guint                line,
                                               const gchar         *function,
                                               const gchar         *format,
                                               ...) G_GNUC_PRINTF(7, 8);
void             milter_logger_log_va_list    (MilterLogger        *logger,
                                               const gchar         *domain,
                                               MilterLogLevelFlags  level,
                                               const gchar         *file,
                                               guint                line,
                                               const gchar         *function,
                                               const gchar         *format,
                                               va_list              args);
MilterLogLevelFlags
                 milter_logger_get_target_level
                                              (MilterLogger        *logger);
MilterLogLevelFlags
                 milter_logger_get_resolved_target_level
                                              (MilterLogger        *logger);
void             milter_logger_set_target_level
                                              (MilterLogger        *logger,
                                               MilterLogLevelFlags  level);
gboolean         milter_logger_set_target_level_by_string
                                              (MilterLogger        *logger,
                                               const gchar         *level_name,
                                               GError             **error);
void             milter_logger_set_interesting_level
                                              (MilterLogger        *logger,
                                               const gchar         *key,
                                               MilterLogLevelFlags  level);
MilterLogLevelFlags
                 milter_logger_get_interesting_level
                                              (MilterLogger        *logger);
MilterLogItemFlags
                 milter_logger_get_target_item
                                              (MilterLogger        *logger);
void             milter_logger_set_target_item
                                              (MilterLogger        *logger,
                                               MilterLogItemFlags   item);
gboolean         milter_logger_set_target_item_by_string
                                              (MilterLogger        *logger,
                                               const gchar         *item_name,
                                               GError             **error);

void             milter_logger_connect_default_handler
                                              (MilterLogger        *logger);
void             milter_logger_disconnect_default_handler
                                              (MilterLogger        *logger);

#define MILTER_GLIB_LOG_DELEGATE(domain)        \
    g_log_set_handler(domain,                   \
                      G_LOG_LEVEL_MASK |        \
                      G_LOG_FLAG_RECURSION |    \
                      G_LOG_FLAG_FATAL,         \
                      milter_glib_log_handler,  \
                      NULL)

void             milter_glib_log_handler      (const gchar         *log_domain,
                                               GLogLevelFlags       log_level,
                                               const gchar         *message,
                                               gpointer             user_data);

G_END_DECLS

#endif /* __MILTER_LOGGER_H__ */

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
