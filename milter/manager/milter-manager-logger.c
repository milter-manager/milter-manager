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

#include <syslog.h>

#include "milter-manager-logger.h"

#define MILTER_MANAGER_LOGGER_GET_PRIVATE(obj)                 \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj),                           \
                                 MILTER_TYPE_MANAGER_LOGGER,   \
                                 MilterManagerLoggerPrivate))

typedef struct _MilterManagerLoggerPrivate MilterManagerLoggerPrivate;
struct _MilterManagerLoggerPrivate
{
    MilterLogger *logger;
};

enum
{
    PROP_0
};

G_DEFINE_TYPE(MilterManagerLogger, milter_manager_logger, G_TYPE_OBJECT)

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
milter_manager_logger_class_init (MilterManagerLoggerClass *klass)
{
    GObjectClass *gobject_class;

    gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->dispose      = dispose;
    gobject_class->set_property = set_property;
    gobject_class->get_property = get_property;

    g_type_class_add_private(gobject_class,
                             sizeof(MilterManagerLoggerPrivate));
}

static gint
milter_log_level_to_syslog_level (MilterLogLevelFlags milter_log_level)
{
    if (milter_log_level & MILTER_LOG_LEVEL_CRITICAL)
        return LOG_CRIT;
    else if (milter_log_level & MILTER_LOG_LEVEL_ERROR)
        return LOG_ERR;
    else if (milter_log_level & MILTER_LOG_LEVEL_WARNING)
        return LOG_WARNING;
    else if (milter_log_level & MILTER_LOG_LEVEL_MESSAGE)
        return LOG_NOTICE;
    else if (milter_log_level & MILTER_LOG_LEVEL_INFO)
        return LOG_INFO;
    else if (milter_log_level & MILTER_LOG_LEVEL_DEBUG)
        return LOG_DEBUG;
    return LOG_DEBUG;
}

static void
cb_log (MilterLogger *logger, const gchar *domain,
        MilterLogLevelFlags level, const gchar *file, guint line,
        const gchar *function, GTimeVal *time_value, const gchar *message,
        gpointer user_data)
{
    GString *log;
    gchar *time_string;
    gint syslog_level;

    syslog_level = milter_log_level_to_syslog_level(level); 

    log = g_string_new(NULL);

    time_string = g_time_val_to_iso8601(time_value);
    g_string_append_printf(log, "[%s]", time_string);
    g_free(time_string);

    g_string_append(log, message);
    syslog(syslog_level, "%s", log->str);
    g_string_free(log, TRUE);
}

static void
setup_logger (MilterLogger *logger)
{
    openlog(PACKAGE_NAME, LOG_PID, LOG_MAIL);
    g_object_ref(logger);
    g_signal_connect(logger, "log", G_CALLBACK(cb_log), NULL);
}

static void
teardown_logger (MilterLogger *logger)
{
    g_object_unref(logger);
    g_signal_handlers_disconnect_by_func(logger,
                                         G_CALLBACK(cb_log), NULL);
    closelog();
}

static void
milter_manager_logger_init (MilterManagerLogger *logger)
{
    MilterManagerLoggerPrivate *priv;

    priv = MILTER_MANAGER_LOGGER_GET_PRIVATE(logger);

    priv->logger = milter_logger();
    setup_logger(priv->logger);
}

static void
dispose (GObject *object)
{
    MilterManagerLoggerPrivate *priv;

    priv = MILTER_MANAGER_LOGGER_GET_PRIVATE(object);

    if (priv->logger) {
        teardown_logger(priv->logger);
        priv->logger = NULL;
    }

    G_OBJECT_CLASS(milter_manager_logger_parent_class)->dispose(object);
}

static void
set_property (GObject      *object,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
    MilterManagerLoggerPrivate *priv;

    priv = MILTER_MANAGER_LOGGER_GET_PRIVATE(object);
    switch (prop_id) {
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
    MilterManagerLoggerPrivate *priv;

    priv = MILTER_MANAGER_LOGGER_GET_PRIVATE(object);
    switch (prop_id) {
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

MilterManagerLogger *
milter_manager_logger_new (void)
{
    return g_object_new(MILTER_TYPE_MANAGER_LOGGER, NULL);
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
