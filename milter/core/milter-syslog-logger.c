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

#include <syslog.h>

#include "milter-syslog-logger.h"

#define INTERESTING_LEVEL_KEY "syslog"

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
    gchar *facility;
};

enum
{
    PROP_0,
    PROP_IDENTITY,
    PROP_TARGET_LEVEL,
    PROP_FACILITY
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
                               "Identity",
                               "The identity of syslog",
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

    spec = g_param_spec_string("facility",
                               NULL,
                               NULL,
                               NULL,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
    g_object_class_install_property(gobject_class, PROP_FACILITY, spec);

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
    else if (milter_log_level & MILTER_LOG_LEVEL_TRACE)
        return LOG_DEBUG;
    else if (milter_log_level & MILTER_LOG_LEVEL_STATISTICS)
        return LOG_NOTICE;
    else if (milter_log_level & MILTER_LOG_LEVEL_PROFILE)
        return LOG_DEBUG;
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
        target_level = milter_logger_get_interesting_level(logger);
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
    if (g_ascii_strcasecmp(facility, "auth") == 0 ||
        g_ascii_strcasecmp(facility, "authpriv") == 0) {
#ifdef LOG_AUTHPRIV
        return LOG_AUTHPRIV;
#else
        return LOG_AUTH;
#endif
    } else if (g_ascii_strcasecmp(facility, "cron") == 0) {
        return LOG_CRON;
    } else if (g_ascii_strcasecmp(facility, "daemon") == 0) {
        return LOG_DAEMON;
#ifdef LOG_FTP
    } else if (g_ascii_strcasecmp(facility, "ftp") == 0) {
        return LOG_FTP;
#endif
    } else if (g_ascii_strcasecmp(facility, "kern") == 0) {
        return LOG_KERN;
    } else if (g_ascii_strcasecmp(facility, "local0") == 0) {
        return LOG_LOCAL0;
    } else if (g_ascii_strcasecmp(facility, "local1") == 0) {
        return LOG_LOCAL1;
    } else if (g_ascii_strcasecmp(facility, "local2") == 0) {
        return LOG_LOCAL2;
    } else if (g_ascii_strcasecmp(facility, "local3") == 0) {
        return LOG_LOCAL3;
    } else if (g_ascii_strcasecmp(facility, "local4") == 0) {
        return LOG_LOCAL4;
    } else if (g_ascii_strcasecmp(facility, "local5") == 0) {
        return LOG_LOCAL5;
    } else if (g_ascii_strcasecmp(facility, "local6") == 0) {
        return LOG_LOCAL6;
    } else if (g_ascii_strcasecmp(facility, "local7") == 0) {
        return LOG_LOCAL7;
    } else if (g_ascii_strcasecmp(facility, "lpr") == 0) {
        return LOG_LPR;
    } else if (g_ascii_strcasecmp(facility, "mail") == 0) {
        return LOG_MAIL;
    } else if (g_ascii_strcasecmp(facility, "news") == 0) {
        return LOG_NEWS;
    } else if (g_ascii_strcasecmp(facility, "user") == 0) {
        return LOG_USER;
    } else if (g_ascii_strcasecmp(facility, "uucp") == 0) {
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
    if (facility_env) {
        facility = resolve_syslog_facility(facility_env);
    } else if (priv->facility) {
        facility = resolve_syslog_facility(priv->facility);
    }
    openlog(priv->identity, LOG_PID, facility);
    g_object_ref(priv->logger);
    g_signal_connect(priv->logger, "log", G_CALLBACK(cb_log), priv);
}

static void
teardown_logger (MilterSyslogLoggerPrivate *priv)
{
    milter_logger_set_interesting_level(priv->logger,
                                        INTERESTING_LEVEL_KEY,
                                        MILTER_LOG_LEVEL_DEFAULT);
    g_signal_handlers_disconnect_by_func(priv->logger,
                                         G_CALLBACK(cb_log), priv);
    g_object_unref(priv->logger);
    closelog();
}

static GObject *
constructor (GType type, guint n_props, GObjectConstructParam *props)
{
    GObject *object;
    GObjectClass *klass;
    MilterSyslogLogger *syslog_logger;
    MilterSyslogLoggerPrivate *priv;
    const gchar *log_level_env;

    klass = G_OBJECT_CLASS(milter_syslog_logger_parent_class);
    object = klass->constructor(type, n_props, props);
    syslog_logger = MILTER_SYSLOG_LOGGER(object);

    priv = MILTER_SYSLOG_LOGGER_GET_PRIVATE(syslog_logger);
    priv->logger = milter_logger();
    log_level_env = g_getenv("MILTER_LOG_SYSLOG_LEVEL");
    if (log_level_env) {
        GError *error = NULL;
        if (!milter_syslog_logger_set_target_level_by_string(syslog_logger,
                                                             log_level_env,
                                                             &error)) {
            milter_warning("[syslog-logger][level][set][warning] %s",
                           error->message);
            g_error_free(error);
        }
    } else {
        MilterLogLevelFlags log_level;
        log_level = milter_logger_get_target_level(priv->logger);
        milter_syslog_logger_set_target_level(syslog_logger, log_level);
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
    priv->facility = NULL;
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

    if (priv->facility) {
        g_free(priv->facility);
        priv->facility = NULL;
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
    case PROP_FACILITY:
        if (priv->facility)
            g_free(priv->facility);
        priv->facility = g_strdup(g_value_get_string(value));
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
    case PROP_FACILITY:
        g_value_set_string(value, priv->facility);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

MilterSyslogLogger *
milter_syslog_logger_new (const gchar *identity, const gchar *facility)
{
    return g_object_new(MILTER_TYPE_SYSLOG_LOGGER,
                        "identity", identity,
                        "facility", facility,
                        NULL);
}

MilterLogLevelFlags
milter_syslog_logger_get_target_level (MilterSyslogLogger *logger)
{
    return MILTER_SYSLOG_LOGGER_GET_PRIVATE(logger)->target_level;
}


void
milter_syslog_logger_set_target_level (MilterSyslogLogger *logger,
                                       MilterLogLevelFlags level)
{
    MilterSyslogLoggerPrivate *priv;

    priv = MILTER_SYSLOG_LOGGER_GET_PRIVATE(logger);
    priv->target_level = level;
    milter_logger_set_interesting_level(priv->logger,
                                        INTERESTING_LEVEL_KEY,
                                        level);
}

gboolean
milter_syslog_logger_set_target_level_by_string (MilterSyslogLogger *logger,
                                                 const gchar *level_name,
                                                 GError **error)
{
    MilterLogLevelFlags level;

    if (level_name) {
        GError *local_error = NULL;
        MilterLogLevelFlags current_level;
        current_level = milter_syslog_logger_get_target_level(logger);
        level = milter_log_level_flags_from_string(level_name,
                                                   current_level,
                                                   &local_error);
        if (local_error) {
            g_propagate_error(error, local_error);
            return FALSE;
        }
    } else {
        level = MILTER_LOG_LEVEL_DEFAULT;
    }
    milter_syslog_logger_set_target_level(logger, level);
    return TRUE;
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
