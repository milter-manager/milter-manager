/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Copyright (C) 2008-2010  Kouhei Sutou <kou@clear-code.com>
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

#include <syslog.h>

#include "milter-syslog-logger.h"

#define MILTER_SYSLOG_LOGGER_GET_PRIVATE(obj)                   \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj),                         \
                                 MILTER_TYPE_SYSLOG_LOGGER,     \
                                 MilterSyslogLoggerPrivate))

typedef struct _MilterSyslogLoggerPrivate MilterSyslogLoggerPrivate;
struct _MilterSyslogLoggerPrivate
{
    MilterLogger *logger;
    gchar *identity;
    MilterLogLevelFlags target_level;
};

enum
{
    PROP_0,
    PROP_IDENTITY,
    PROP_TARGET_LEVEL
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

    spec = g_param_spec_flags("target-level",
                              "Target log level",
                              "The target log level",
                              MILTER_TYPE_LOG_LEVEL_FLAGS,
                              MILTER_LOG_LEVEL_DEFAULT,
                              G_PARAM_READWRITE);
    g_object_class_install_property(gobject_class, PROP_TARGET_LEVEL, spec);

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
    MilterSyslogLoggerPrivate *priv = user_data;
    GString *log;
    gint syslog_level;
    MilterLogLevelFlags target_level;

    target_level = priv->target_level;
    if (target_level == MILTER_LOG_LEVEL_DEFAULT)
        target_level = MILTER_LOG_LEVEL_STATISTICS;
    if (!(level & target_level))
        return;

    log = g_string_new(NULL);

    if (level & MILTER_LOG_LEVEL_STATISTICS) {
        g_string_append(log, "[statistics] ");
    }

    g_string_append(log, message);

    syslog_level = milter_log_level_to_syslog_level(level);
    syslog(syslog_level, "%s", log->str);

    g_string_free(log, TRUE);
}

static gint
resolve_syslog_facility (const gchar *facility)
{
    if (g_strcasecmp(facility, "auth") == 0 ||
        g_strcasecmp(facility, "authpriv") == 0) {
        return LOG_AUTHPRIV;
    } else if (g_strcasecmp(facility, "cron") == 0) {
        return LOG_CRON;
    } else if (g_strcasecmp(facility, "daemon") == 0) {
        return LOG_DAEMON;
    } else if (g_strcasecmp(facility, "ftp") == 0) {
        return LOG_FTP;
    } else if (g_strcasecmp(facility, "kern") == 0) {
        return LOG_KERN;
    } else if (g_strcasecmp(facility, "local0") == 0) {
        return LOG_LOCAL0;
    } else if (g_strcasecmp(facility, "local1") == 0) {
        return LOG_LOCAL1;
    } else if (g_strcasecmp(facility, "local2") == 0) {
        return LOG_LOCAL2;
    } else if (g_strcasecmp(facility, "local3") == 0) {
        return LOG_LOCAL3;
    } else if (g_strcasecmp(facility, "local4") == 0) {
        return LOG_LOCAL4;
    } else if (g_strcasecmp(facility, "local5") == 0) {
        return LOG_LOCAL5;
    } else if (g_strcasecmp(facility, "local6") == 0) {
        return LOG_LOCAL6;
    } else if (g_strcasecmp(facility, "local7") == 0) {
        return LOG_LOCAL7;
    } else if (g_strcasecmp(facility, "lpr") == 0) {
        return LOG_LPR;
    } else if (g_strcasecmp(facility, "mail") == 0) {
        return LOG_MAIL;
    } else if (g_strcasecmp(facility, "news") == 0) {
        return LOG_NEWS;
    } else if (g_strcasecmp(facility, "user") == 0) {
        return LOG_USER;
    } else if (g_strcasecmp(facility, "uucp") == 0) {
        return LOG_UUCP;
    }

    return LOG_MAIL;
}

static void
setup_logger (MilterSyslogLoggerPrivate *priv)
{
    gint facility = LOG_MAIL;
    const gchar *facility_env;

    facility_env = g_getenv("MILTER_LOG_SYSLOG_FACILITY");
    if (facility_env)
        facility = resolve_syslog_facility(facility_env);
    openlog(priv->identity, LOG_PID, facility);
    g_object_ref(priv->logger);
    g_signal_connect(priv->logger, "log", G_CALLBACK(cb_log), priv);
}

static void
teardown_logger (MilterSyslogLoggerPrivate *priv)
{
    g_object_unref(priv->logger);
    g_signal_handlers_disconnect_by_func(priv->logger,
                                         G_CALLBACK(cb_log), priv);
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
    if (priv->target_level == MILTER_LOG_LEVEL_DEFAULT) {
        const gchar *level_env;
        level_env = g_getenv("MILTER_LOG_SYSLOG_LEVEL");
        if (level_env) {
            priv->target_level = milter_log_level_flags_from_string(level_env);
        }
    }
    setup_logger(priv);

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
        teardown_logger(priv);
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
    MilterSyslogLoggerPrivate *priv;

    priv = MILTER_SYSLOG_LOGGER_GET_PRIVATE(object);
    switch (prop_id) {
    case PROP_IDENTITY:
        g_value_set_string(value, priv->identity);
        break;
    case PROP_TARGET_LEVEL:
        g_value_set_flags(value, priv->target_level);
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
