/* -*- c-file-style: "ruby" -*- */
/*
 *  Copyright (C) 2009-2011  Kouhei Sutou <kou@clear-code.com>
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

#include "rb-milter-core-private.h"

#define SELF(self) MILTER_LOGGER(RVAL2GOBJ(self))
#define LOG_LEVEL2RVAL(level) GFLAGS2RVAL(level, MILTER_TYPE_LOG_LEVEL_FLAGS)
#define RVAL2LOG_LEVEL(level) RVAL2GFLAGS(level, MILTER_TYPE_LOG_LEVEL_FLAGS)
#define LOG_ITEM2RVAL(item) GFLAGS2RVAL(item, MILTER_TYPE_LOG_ITEM_FLAGS)

static VALUE
s_from_string (VALUE self, VALUE string)
{
    MilterLogLevelFlags level;
    GError *error = NULL;

    level = milter_log_level_flags_from_string(RVAL2CSTR(string), &error);
    if (error)
	RAISE_GERROR(error);
    return LOG_LEVEL2RVAL(level);
}

static VALUE
s_default (VALUE self)
{
    return GOBJ2RVAL(milter_logger());
}

static VALUE
log_full (VALUE self, VALUE domain, VALUE level, VALUE file, VALUE line,
	  VALUE function, VALUE message)
{
    milter_logger_log(SELF(self),
		      RVAL2CSTR(domain),
		      RVAL2LOG_LEVEL(level),
		      RVAL2CSTR(file),
		      NUM2UINT(line),
		      RVAL2CSTR(function),
		      "%s",
		      RVAL2CSTR(message));
    return self;
}

void
Init_milter_logger (void)
{
    VALUE rb_cMilterLogger;
    VALUE rb_cMilterLogLevelFlags;
    VALUE rb_cMilterLogItemFlags;

    rb_cMilterLogger = G_DEF_CLASS(MILTER_TYPE_LOGGER, "Logger", rb_mMilter);

    rb_cMilterLogLevelFlags =
	G_DEF_CLASS(MILTER_TYPE_LOG_LEVEL_FLAGS, "LogLevelFlags", rb_mMilter);
    rb_cMilterLogItemFlags =
	G_DEF_CLASS(MILTER_TYPE_LOG_ITEM_FLAGS, "LogItemFlags", rb_mMilter);
    G_DEF_CLASS(MILTER_TYPE_LOG_COLORIZE, "LogColorize", rb_mMilter);

    G_DEF_CONSTANTS(rb_mMilter, MILTER_TYPE_LOG_LEVEL_FLAGS, "MILTER_");
    G_DEF_CONSTANTS(rb_mMilter, MILTER_TYPE_LOG_ITEM_FLAGS, "MILTER_");
    G_DEF_CONSTANTS(rb_mMilter, MILTER_TYPE_LOG_COLORIZE, "MILTER_");

    rb_define_const(rb_cMilterLogLevelFlags,
		    "ALL",
		    LOG_LEVEL2RVAL(MILTER_LOG_LEVEL_ALL));
    rb_define_const(rb_mMilter,
		    "LOG_LEVEL_ALL",
		    LOG_LEVEL2RVAL(MILTER_LOG_LEVEL_ALL));
    rb_define_const(rb_cMilterLogItemFlags,
		    "ALL",
		    LOG_ITEM2RVAL(MILTER_LOG_ITEM_ALL));
    rb_define_const(rb_mMilter,
		    "LOG_ITEM_ALL",
		    LOG_ITEM2RVAL(MILTER_LOG_ITEM_ALL));

    rb_define_singleton_method(rb_cMilterLogLevelFlags, "from_string",
			       s_from_string, 1);

    rb_define_singleton_method(rb_cMilterLogger, "default", s_default, 0);

    rb_define_method(rb_cMilterLogger, "log_full", log_full, 6);

    G_DEF_SETTERS(rb_cMilterLogger);
}
