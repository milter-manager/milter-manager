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
 *  along with this library.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef __MILTER_LOGGER_H__
#define __MILTER_LOGGER_H__

#include <glib-object.h>

#include <milter/core/milter-protocol.h>
#include <milter/core/milter-option.h>

G_BEGIN_DECLS

#define milter_log(level, format, ...)          \
    (milter_logger_log(milter_logger(),         \
                       MILTER_LOG_DOMAIN,       \
                       level,                   \
                       __FILE__,                \
                       __LINE__,                \
                       __PRETTY_FUNCTION__,     \
                       format, ## __VA_ARGS__))

#define milter_error(format, ...)                               \
    milter_log(MILTER_LOG_LEVEL_ERROR, format, ## __VA_ARGS__)
#define milter_critical(format, ...)                            \
    milter_log(MILTER_LOG_LEVEL_CRITICAL, format, ## __VA_ARGS__)
#define milter_message(format, ...)                             \
    milter_log(MILTER_LOG_LEVEL_MESSAGE, format, ## __VA_ARGS__)
#define milter_warning(format, ...)                             \
    milter_log(MILTER_LOG_LEVEL_WARNING, format, ## __VA_ARGS__)
#define milter_debug(format, ...)                               \
    milter_log(MILTER_LOG_LEVEL_DEBUG, format, ## __VA_ARGS__)
#define milter_info(format, ...)                                \
    milter_log(MILTER_LOG_LEVEL_INFO, format, ## __VA_ARGS__)
#define milter_statistics(format, ...)                                  \
    milter_log(MILTER_LOG_LEVEL_STATISTICS, format, ## __VA_ARGS__)

#define MILTER_TYPE_LOGGER            (milter_logger_get_type())
#define MILTER_LOGGER(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), MILTER_TYPE_LOGGER, MilterLogger))
#define MILTER_LOGGER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), MILTER_TYPE_LOGGER, MilterLoggerClass))
#define MILTER_IS_LOGGER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), MILTER_TYPE_LOGGER))
#define MILTER_IS_LOGGER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), MILTER_TYPE_LOGGER))
#define MILTER_LOGGER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), MILTER_TYPE_LOGGER, MilterLoggerClass))

typedef enum
{
    MILTER_LOG_LEVEL_CRITICAL   = 1 << 0,
    MILTER_LOG_LEVEL_ERROR      = 1 << 1,
    MILTER_LOG_LEVEL_WARNING    = 1 << 2,
    MILTER_LOG_LEVEL_MESSAGE    = 1 << 3,
    MILTER_LOG_LEVEL_INFO       = 1 << 4,
    MILTER_LOG_LEVEL_DEBUG      = 1 << 5,
    MILTER_LOG_LEVEL_STATISTICS = 1 << 6
} MilterLogLevelFlags;

#define MILTER_LOG_LEVEL_ALL (MILTER_LOG_LEVEL_CRITICAL |       \
                              MILTER_LOG_LEVEL_ERROR |          \
                              MILTER_LOG_LEVEL_WARNING |        \
                              MILTER_LOG_LEVEL_MESSAGE |        \
                              MILTER_LOG_LEVEL_INFO |           \
                              MILTER_LOG_LEVEL_DEBUG |          \
                              MILTER_LOG_LEVEL_STATISTICS)

typedef enum
{
    MILTER_LOG_ITEM_DEFAULT     = 0,
    MILTER_LOG_ITEM_DOMAIN      = 1 << 0,
    MILTER_LOG_ITEM_LEVEL       = 1 << 1,
    MILTER_LOG_ITEM_LOCATION    = 1 << 2,
    MILTER_LOG_ITEM_TIME        = 1 << 3
} MilterLogItemFlags;

#define MILTER_LOG_ITEM_ALL (MILTER_LOG_ITEM_DOMAIN |   \
                             MILTER_LOG_ITEM_LEVEL |    \
                             MILTER_LOG_ITEM_LOCATION | \
                             MILTER_LOG_ITEM_TIME)

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

MilterLogLevelFlags milter_log_level_flags_from_string (const gchar *level_name);

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
void             milter_logger_set_target_level
                                              (MilterLogger        *logger,
                                               MilterLogLevelFlags  level);

G_END_DECLS

#endif /* __MILTER_LOGGER_H__ */

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
