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

#include "milter-syslog-logger.h"

#define MILTER_SYSLOG_LOGGER_GET_PRIVATE(obj)                 \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj),                           \
                                 MILTER_TYPE_SYSLOG_LOGGER,   \
                                 MilterSyslogLoggerPrivate))

typedef struct _MilterSyslogLoggerPrivate MilterSyslogLoggerPrivate;
struct _MilterSyslogLoggerPrivate
{
    MilterLogger *logger;
    gchar *identity;
};

enum
{
    PROP_0,
    PROP_IDENTITY
};

G_DEFINE_TYPE(MilterSyslogLogger, milter_syslog_logger, G_TYPE_OBJECT)

static GObject *constructor  (GType                  type,
                              guint                  n_props,
                              GObjectConstructParam *props);

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
milter_syslog_logger_class_init (MilterSyslogLoggerClass *klass)
{
    GObjectClass *gobject_class;
    GParamSpec *spec;

    gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->constructor  = constructor;
    gobject_class->dispose      = dispose;
    gobject_class->set_property = set_property;
    gobject_class->get_property = get_property;

    spec = g_param_spec_string("identity",
                               NULL,
                               NULL,
                               NULL,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
    g_object_class_install_property(gobject_class, PROP_IDENTITY, spec);

    g_type_class_add_private(gobject_class,
                             sizeof(MilterSyslogLoggerPrivate));
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
    else if (milter_log_level & MILTER_LOG_LEVEL_STATISTICS)
        return LOG_INFO;
    return LOG_DEBUG;
}

static void
cb_log (MilterLogger *logger, const gchar *domain,
        MilterLogLevelFlags level, const gchar *file, guint line,
        const gchar *function, GTimeVal *time_value, const gchar *message,
        gpointer user_data)
{
    GString *log;
    gint syslog_level;

    log = g_string_new(NULL);

    if (level & MILTER_LOG_LEVEL_STATISTICS) {
        gchar *time_string;
        time_string = g_time_val_to_iso8601(time_value);
        g_string_append_printf(log, "[%s]", time_string);
        g_free(time_string);
    }

    g_string_append(log, message);

    syslog_level = milter_log_level_to_syslog_level(level); 
    syslog(syslog_level, "%s", log->str);

    g_string_free(log, TRUE);
}

static void
setup_logger (MilterLogger *logger, const gchar *identity)
{
    openlog(identity, LOG_PID, LOG_MAIL);
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

static GObject *
constructor (GType type, guint n_props, GObjectConstructParam *props)
{
    GObject *object;
    GObjectClass *klass;
    MilterSyslogLoggerPrivate *priv;

    klass = G_OBJECT_CLASS(milter_syslog_logger_parent_class);
    object = klass->constructor(type, n_props, props);

    priv = MILTER_SYSLOG_LOGGER_GET_PRIVATE(object);
    priv->logger = milter_logger();
    setup_logger(priv->logger, priv->identity);

    return object;
}

static void
milter_syslog_logger_init (MilterSyslogLogger *logger)
{
    MilterSyslogLoggerPrivate *priv;

    priv = MILTER_SYSLOG_LOGGER_GET_PRIVATE(logger);

    priv->identity = NULL;
}

static void
dispose (GObject *object)
{
    MilterSyslogLoggerPrivate *priv;

    priv = MILTER_SYSLOG_LOGGER_GET_PRIVATE(object);

    if (priv->identity) {
        g_free(priv->identity);
        priv->identity = NULL;
    }

    if (priv->logger) {
        teardown_logger(priv->logger);
        priv->logger = NULL;
    }

    G_OBJECT_CLASS(milter_syslog_logger_parent_class)->dispose(object);
}

static void
set_property (GObject      *object,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
    MilterSyslogLoggerPrivate *priv;

    priv = MILTER_SYSLOG_LOGGER_GET_PRIVATE(object);
    switch (prop_id) {
      case PROP_IDENTITY:
        if (priv->identity)
            g_free(priv->identity);
        priv->identity = g_strdup(g_value_get_string(value));
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
    MilterSyslogLoggerPrivate *priv;

    priv = MILTER_SYSLOG_LOGGER_GET_PRIVATE(object);
    switch (prop_id) {
      case PROP_IDENTITY:
        g_value_set_string(value, priv->identity);
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

MilterSyslogLogger *
milter_syslog_logger_new (const gchar *identity)
{
    return g_object_new(MILTER_TYPE_SYSLOG_LOGGER,
                        "identity", identity,
                        NULL);
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
