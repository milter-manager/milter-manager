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

static VALUE rb_cMilterLogLevelFlags;

static VALUE
s_from_string (int argc, VALUE *argv, VALUE self)
{
    VALUE rb_string, rb_base_level;
    const gchar *string;
    MilterLogLevelFlags base_level = MILTER_LOG_LEVEL_DEFAULT;
    MilterLogLevelFlags level;
    GError *error = NULL;

    rb_scan_args(argc, argv, "11", &rb_string, &rb_base_level);
    string = RVAL2CSTR(rb_string);
    if (!NIL_P(rb_base_level)) {
	base_level = RVAL2LOG_LEVEL(rb_base_level);
    }

    level = milter_log_level_flags_from_string(string, base_level, &error);
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

static VALUE
set_target_level (VALUE self, VALUE rb_level)
{
    MilterLogger *logger;

    logger = SELF(self);
    if (RVAL2CBOOL(rb_obj_is_kind_of(rb_level, rb_cMilterLogLevelFlags))) {
	milter_logger_set_target_level(logger, RVAL2LOG_LEVEL(rb_level));
    } else if (RVAL2CBOOL(rb_obj_is_kind_of(rb_level, rb_cNumeric))) {
	milter_logger_set_target_level(logger, NUM2INT(rb_level));
    } else {
	GError *error = NULL;
	const char *level;
	if (NIL_P(rb_level)) {
	    level = NULL;
	} else {
	    rb_level = rb_str_to_str(rb_level);
	    level = StringValueCStr(rb_level);
	}
	if (!milter_logger_set_target_level_by_string(logger, level, &error)) {
	    RAISE_GERROR(error);
	}
    }
    return self;
}

void
Init_milter_logger (void)
{
    VALUE rb_cMilterLogger;
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
			       s_from_string, -1);

    rb_define_singleton_method(rb_cMilterLogger, "default", s_default, 0);

    rb_define_method(rb_cMilterLogger, "log_full", log_full, 6);
    rb_undef_method(rb_cMilterLogger, "set_target_level");
    rb_undef_method(rb_cMilterLogger, "target_level=");
    rb_define_method(rb_cMilterLogger, "set_target_level", set_target_level, 1);

    G_DEF_SETTERS(rb_cMilterLogger);
}
