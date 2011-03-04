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

#ifndef __MILTER_SYSLOG_LOGGER_H__
#define __MILTER_SYSLOG_LOGGER_H__

#include <glib-object.h>

#include <milter/core.h>

G_BEGIN_DECLS

#define MILTER_TYPE_SYSLOG_LOGGER            (milter_syslog_logger_get_type())
#define MILTER_SYSLOG_LOGGER(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), MILTER_TYPE_SYSLOG_LOGGER, MilterSyslogLogger))
#define MILTER_SYSLOG_LOGGER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), MILTER_TYPE_SYSLOG_LOGGER, MilterSyslogLoggerClass))
#define MILTER_SYSLOG_LOGGER_IS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), MILTER_TYPE_SYSLOG_LOGGER))
#define MILTER_SYSLOG_LOGGER_IS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), MILTER_TYPE_SYSLOG_LOGGER))
#define MILTER_SYSLOG_LOGGER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), MILTER_TYPE_SYSLOG_LOGGER, MilterSyslogLoggerClass))

typedef struct _MilterSyslogLogger         MilterSyslogLogger;
typedef struct _MilterSyslogLoggerClass    MilterSyslogLoggerClass;

struct _MilterSyslogLogger
{
    GObject object;
};

struct _MilterSyslogLoggerClass
{
    GObjectClass parent_class;
};

GType               milter_syslog_logger_get_type (void) G_GNUC_CONST;

MilterSyslogLogger *milter_syslog_logger_new      (const gchar *identity,
                                                   const gchar *facility);

MilterLogLevelFlags milter_syslog_logger_get_target_level
                                              (MilterSyslogLogger  *logger);
void                milter_syslog_logger_set_target_level
                                              (MilterSyslogLogger  *logger,
                                               MilterLogLevelFlags  level);
gboolean            milter_syslog_logger_set_target_level_by_string
                                              (MilterSyslogLogger  *logger,
                                               const gchar         *level_name,
                                               GError             **error);

G_END_DECLS

#endif /* __MILTER_SYSLOG_LOGGER_H__ */

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
