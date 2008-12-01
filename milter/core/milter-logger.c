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

#ifdef HAVE_CONFIG_H
#  include "../../config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include "milter-logger.h"
#include "milter-utils.h"
#include "milter-marshalers.h"
#include "milter-enum-types.h"

enum
{
    LOG,
    LAST_SIGNAL
};

static gint signals[LAST_SIGNAL] = {0};

static MilterLogger *singleton_milter_logger = NULL;

G_DEFINE_TYPE(MilterLogger, milter_logger, G_TYPE_OBJECT);

static void dispose        (GObject         *object);

static void
milter_logger_class_init (MilterLoggerClass *klass)
{
    GObjectClass *gobject_class;

    gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->dispose      = dispose;

    signals[LOG] =
        g_signal_new("log",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterLoggerClass, log),
                     NULL, NULL,
                     _milter_marshal_VOID__STRING_FLAGS_STRING_UINT_STRING_POINTER_STRING,
                     G_TYPE_NONE, 7,
                     G_TYPE_STRING, MILTER_TYPE_LOG_LEVEL_FLAGS, G_TYPE_STRING,
                     G_TYPE_UINT, G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_STRING);
}

static void
milter_logger_init (MilterLogger *logger)
{
}

static void
dispose (GObject *object)
{
    G_OBJECT_CLASS(milter_logger_parent_class)->dispose(object);
}

MilterLogLevelFlags
milter_log_level_flags_from_string (const gchar *level_name)
{
    return milter_utils_flags_from_string(MILTER_TYPE_LOG_LEVEL_FLAGS,
                                          level_name);
}

static void
cb_log (MilterLogger *logger, const gchar *domain,
        MilterLogLevelFlags level, const gchar *file, guint line,
        const gchar *function, GTimeVal *time_value, const gchar *message,
        gpointer user_data)
{
    const gchar *target_level_flags;
    MilterLogLevelFlags target_level;

    target_level_flags = g_getenv("MILTER_LOG_LEVEL");
    target_level = milter_log_level_flags_from_string(target_level_flags);
    if (level & target_level) {
        GString *log;
        GFlagsClass *flags_class;
        gchar *time_string;

        log = g_string_new(NULL);

        flags_class = g_type_class_ref(MILTER_TYPE_LOG_LEVEL_FLAGS);
        if (flags_class) {
            if (level & flags_class->mask) {
                guint i;
                for (i = 0; i < flags_class->n_values; i++) {
                    GFlagsValue *value = flags_class->values + i;
                    if (level & value->value)
                        g_string_append_printf(log, "[%s]", value->value_nick);
                }
            }
            g_type_class_unref(flags_class);
        }

        if (domain)
            g_string_append_printf(log, "[%s]", domain);
        time_string = g_time_val_to_iso8601(time_value);
        g_string_append_printf(log, " [%s]", time_string);
        g_free(time_string);
        g_string_append(log, " ");
        if (file)
            g_string_append_printf(log, "%s:%d: %s(): ", file, line, function);
        g_string_append(log, message);
        g_string_append(log, "\n");
        g_print("%s", log->str);
        g_string_free(log, TRUE);
    }
}

MilterLogger *
milter_logger (void)
{
    if (!singleton_milter_logger) {
        singleton_milter_logger = milter_logger_new();
        g_signal_connect(singleton_milter_logger, "log",
                         G_CALLBACK(cb_log), NULL);
    }

    return singleton_milter_logger;
}

MilterLogger *
milter_logger_new (void)
{
    return g_object_new(MILTER_TYPE_LOGGER,
                        NULL);
}

void
milter_logger_log (MilterLogger *logger,
                   const gchar *domain, MilterLogLevelFlags level,
                   const gchar *file, guint line, const gchar *function,
                   const gchar *format, ...)
{
    va_list args;

    va_start(args, format);
    milter_logger_log_va_list(logger, domain, level, file, line, function,
                              format, args);
    va_end(args);
}

void
milter_logger_log_va_list (MilterLogger *logger,
                           const gchar *domain, MilterLogLevelFlags level,
                           const gchar *file, guint line, const gchar *function,
                           const gchar *format, va_list args)
{
    GTimeVal time_value;
    gchar *message;

    g_get_current_time(&time_value);

    message = g_strdup_vprintf(format, args);
    g_signal_emit(logger, signals[LOG], 0, domain, level, file, line, function, &time_value, message);
    g_free(message);
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
