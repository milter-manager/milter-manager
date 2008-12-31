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
#include <unistd.h>

#include <glib.h>

#include "milter-logger.h"
#include "milter-utils.h"
#include "milter-marshalers.h"
#include "milter-enum-types.h"

#define MILTER_LOGGER_GET_PRIVATE(obj)                  \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj),                 \
                                 MILTER_TYPE_LOGGER,    \
                                 MilterLoggerPrivate))

typedef struct _MilterLoggerPrivate	MilterLoggerPrivate;
struct _MilterLoggerPrivate
{
    MilterLogLevelFlags target_level;
};

enum
{
    PROP_0,
    PROP_TARGET_LEVEL
};

enum
{
    LOG,
    LAST_SIGNAL
};

static gint signals[LAST_SIGNAL] = {0};

static MilterLogger *singleton_milter_logger = NULL;

G_DEFINE_TYPE(MilterLogger, milter_logger, G_TYPE_OBJECT);

static void dispose        (GObject         *object);
static void set_property   (GObject         *object,
                            guint            prop_id,
                            const GValue    *value,
                            GParamSpec      *pspec);
static void get_property   (GObject         *object,
                            guint            prop_id,
                            GValue          *value,
                            GParamSpec      *pspec);

static void
milter_logger_class_init (MilterLoggerClass *klass)
{
    GObjectClass *gobject_class;
    GParamSpec *spec;

    gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->dispose      = dispose;
    gobject_class->set_property = set_property;
    gobject_class->get_property = get_property;

    spec = g_param_spec_flags("target-level",
                              "Target log level",
                              "The target log level",
                              MILTER_TYPE_LOG_LEVEL_FLAGS,
                              0,
                              G_PARAM_READWRITE);
    g_object_class_install_property(gobject_class, PROP_TARGET_LEVEL, spec);

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

    g_type_class_add_private(gobject_class, sizeof(MilterLoggerPrivate));
}

static void
milter_logger_init (MilterLogger *logger)
{
    MilterLoggerPrivate *priv;

    priv = MILTER_LOGGER_GET_PRIVATE(logger);
    priv->target_level = 0;
}

static void
dispose (GObject *object)
{
    G_OBJECT_CLASS(milter_logger_parent_class)->dispose(object);
}

static void
set_property (GObject      *object,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
    MilterLoggerPrivate *priv;

    priv = MILTER_LOGGER_GET_PRIVATE(object);
    switch (prop_id) {
    case PROP_TARGET_LEVEL:
        priv->target_level = g_value_get_flags(value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static void
get_property (GObject    *object,
              guint       prop_id,
              GValue     *value,
              GParamSpec *pspec)
{
    MilterLoggerPrivate *priv;

    priv = MILTER_LOGGER_GET_PRIVATE(object);
    switch (prop_id) {
    case PROP_TARGET_LEVEL:
        g_value_set_flags(value, priv->target_level);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

MilterLogLevelFlags
milter_log_level_flags_from_string (const gchar *level_name)
{
    return milter_utils_flags_from_string(MILTER_TYPE_LOG_LEVEL_FLAGS,
                                          level_name);
}

#define BLACK_COLOR "\033[01;30m"
#define RED_COLOR "\033[01;31m"
#define RED_BACK_COLOR "\033[41m"
#define GREEN_COLOR "\033[01;32m"
#define GREEN_BACK_COLOR "\033[01;42m"
#define YELLOW_COLOR "\033[01;33m"
#define YELLOW_BACK_COLOR "\033[01;43m"
#define BLUE_COLOR "\033[01;34m"
#define BLUE_BACK_COLOR "\033[01;44m"
#define MAGENTA_COLOR "\033[01;35m"
#define MAGENTA_BACK_COLOR "\033[01;45m"
#define CYAN_COLOR "\033[01;36m"
#define CYAN_BACK_COLOR "\033[01;46m"
#define WHITE_COLOR "\033[01;37m"
#define WHITE_BACK_COLOR "\033[01;47m"
#define NORMAL_COLOR "\033[00m"

static void
log_message_colorize_console (GString *log,
                              MilterLogLevelFlags level,
                              const gchar *message)
{
    const gchar *color = NULL;

    switch (level) {
      case MILTER_LOG_LEVEL_ERROR:
        color = WHITE_COLOR RED_BACK_COLOR;
        break;
      case MILTER_LOG_LEVEL_CRITICAL:
        color = YELLOW_COLOR RED_BACK_COLOR;
        break;
      case MILTER_LOG_LEVEL_WARNING:
        color = WHITE_COLOR YELLOW_BACK_COLOR;
        break;
      case MILTER_LOG_LEVEL_MESSAGE:
        color = WHITE_COLOR GREEN_BACK_COLOR;
        break;
      case MILTER_LOG_LEVEL_INFO:
        color = WHITE_COLOR CYAN_BACK_COLOR;
        break;
      case MILTER_LOG_LEVEL_DEBUG:
        color = WHITE_COLOR BLUE_BACK_COLOR;
        break;
      case MILTER_LOG_LEVEL_STATISTICS:
        color = BLUE_COLOR WHITE_BACK_COLOR;
        break;
      default:
        color = NULL;
        break;
    }

    if (color)
        g_string_append_printf(log, "%s%s%s", color, message, NORMAL_COLOR);
    else
        g_string_append(log, message);
}

static void
log_message (GString *log, MilterLogLevelFlags level, const gchar *message)
{
    const gchar *colorize_type;
    MilterLogColorize colorize = MILTER_LOG_COLORIZE_DEFAULT;

    colorize_type = g_getenv("MILTER_LOG_COLORIZE");
    if (colorize_type)
        colorize = milter_utils_enum_from_string(MILTER_TYPE_LOG_COLORIZE,
                                                 colorize_type);

    if (colorize == MILTER_LOG_COLORIZE_DEFAULT) {
        const gchar *term;

        term = g_getenv("TERM");
        if (isatty(STDOUT_FILENO) &&
            term && (g_str_has_suffix(term, "term") ||
                     g_str_has_suffix(term, "term-color") ||
                     g_str_equal(term, "screen") ||
                     g_str_equal(term, "linux"))) {
            colorize = MILTER_LOG_COLORIZE_CONSOLE;
        } else {
            colorize = MILTER_LOG_COLORIZE_NONE;
        }
    }

    switch (colorize) {
      case MILTER_LOG_COLORIZE_CONSOLE:
        log_message_colorize_console(log, level, message);
        break;
      default:
        g_string_append(log, message);
        break;
    }
}

void
milter_logger_default_log_handler (MilterLogger *logger, const gchar *domain,
                                   MilterLogLevelFlags level,
                                   const gchar *file, guint line,
                                   const gchar *function,
                                   GTimeVal *time_value, const gchar *message,
                                   gpointer user_data)
{
    MilterLoggerPrivate *priv;
    const gchar *target_level_flags;
    MilterLogLevelFlags target_level;
    const gchar *target_item_flags;
    MilterLogItemFlags target_item = MILTER_LOG_ITEM_DEFAULT;
    GString *log;

    priv = MILTER_LOGGER_GET_PRIVATE(logger);
    target_level = priv->target_level;
    target_level_flags = g_getenv("MILTER_LOG_LEVEL");
    if (target_level_flags)
        target_level = milter_log_level_flags_from_string(target_level_flags);
    if (!(level & target_level))
        return;

    log = g_string_new(NULL);

    target_item_flags = g_getenv("MILTER_LOG_ITEM");
    if (target_item_flags)
        target_item = milter_utils_flags_from_string(MILTER_TYPE_LOG_ITEM_FLAGS,
                                                     target_item_flags);
    if (target_item == MILTER_LOG_ITEM_DEFAULT)
        target_item = MILTER_LOG_ITEM_TIME;

    if (target_item & MILTER_LOG_ITEM_LEVEL) {
        GFlagsClass *flags_class;

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
    }

    if (domain && (target_item & MILTER_LOG_ITEM_DOMAIN))
        g_string_append_printf(log, "[%s]", domain);

    if (target_item & MILTER_LOG_ITEM_TIME) {
        gchar *time_string;

        time_string = g_time_val_to_iso8601(time_value);
        g_string_append_printf(log, "[%s]", time_string);
        g_free(time_string);
    }

    if (file && (target_item & MILTER_LOG_ITEM_LOCATION)) {
        if (log->len > 0)
            g_string_append(log, " ");
        g_string_append_printf(log, "%s:%d: %s(): ", file, line, function);
    } else {
        if (log->len > 0)
            g_string_append(log, ": ");
    }

    log_message(log, level, message);
    g_string_append(log, "\n");
    g_print("%s", log->str);
    g_string_free(log, TRUE);
}

MilterLogger *
milter_logger (void)
{
    if (!singleton_milter_logger) {
        singleton_milter_logger = milter_logger_new();
        g_signal_connect(singleton_milter_logger, "log",
                         G_CALLBACK(milter_logger_default_log_handler), NULL);
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

MilterLogLevelFlags
milter_logger_get_target_level (MilterLogger *logger)
{
    return MILTER_LOGGER_GET_PRIVATE(logger)->target_level;
}


void
milter_logger_set_target_level (MilterLogger *logger,
                                MilterLogLevelFlags level)
{
    MILTER_LOGGER_GET_PRIVATE(logger)->target_level = level;
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
