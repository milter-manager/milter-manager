/* -*- c-file-style: "ruby" -*- */
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

#include "rb-milter-core-private.h"

#define SELF(self) RVAL2OPTION(self)

static VALUE
step_flags_merge (VALUE self, VALUE other)
{
    MilterStepFlags merged;

    merged = milter_step_flags_merge(RVAL2STEP_FLAGS(self),
				     RVAL2STEP_FLAGS(other));
    return STEP_FLAGS2RVAL(merged);
}

static VALUE
initialize (int argc, VALUE *argv, VALUE self)
{
    VALUE rb_version, rb_action, rb_step;
    guint32 version = 2;
    MilterActionFlags action = 0;
    MilterStepFlags step = 0;
    MilterOption *option;

    rb_scan_args(argc, argv, "03", &rb_version, &rb_action, &rb_step);

    if (!NIL_P(rb_version))
        version = NUM2UINT(rb_version);
    if (!NIL_P(rb_action))
        action = RVAL2ACTION_FLAGS(rb_action);
    if (!NIL_P(rb_step))
        step = RVAL2STEP_FLAGS(rb_step);

    option = milter_option_new(version, action, step);
    G_INITIALIZE(self, option);

    return Qnil;
}

static VALUE
equal (VALUE self, VALUE other)
{
    MilterOption *self_option, *other_option;

    self_option = SELF(self);
    other_option = SELF(other);

    return CBOOL2RVAL((milter_option_get_version(self_option) ==
                       milter_option_get_version(other_option)) &&
                      (milter_option_get_action(self_option) ==
                       milter_option_get_action(other_option)) &&
                      (milter_option_get_step(self_option) ==
                       milter_option_get_step(other_option)));
}

static VALUE
add_action (VALUE self, VALUE action)
{
    milter_option_add_action(SELF(self), RVAL2ACTION_FLAGS(action));
    return self;
}

static VALUE
remove_step (VALUE self, VALUE step)
{
    milter_option_remove_step(SELF(self), RVAL2STEP_FLAGS(step));
    return self;
}

void
Init_milter_option (void)
{
    VALUE rb_cMilterStepFlags;
    VALUE rb_cMilterOption;

    rb_cMilterOption = G_DEF_CLASS(MILTER_TYPE_OPTION, "Option", rb_mMilter);

    G_DEF_CLASS(MILTER_TYPE_ACTION_FLAGS, "ActionFlags", rb_mMilter);
    rb_cMilterStepFlags =
	G_DEF_CLASS(MILTER_TYPE_STEP_FLAGS, "StepFlags", rb_mMilter);

    G_DEF_CONSTANTS(rb_mMilter, MILTER_TYPE_ACTION_FLAGS, "MILTER_");
    G_DEF_CONSTANTS(rb_mMilter, MILTER_TYPE_STEP_FLAGS, "MILTER_");

    rb_define_const(rb_cMilterStepFlags, "NO_EVENT_MASK",
		    STEP_FLAGS2RVAL(MILTER_STEP_NO_EVENT_MASK));
    rb_define_const(rb_cMilterStepFlags, "NO_REPLY_MASK",
		    STEP_FLAGS2RVAL(MILTER_STEP_NO_REPLY_MASK));
    rb_define_const(rb_cMilterStepFlags, "NO_MASK",
		    STEP_FLAGS2RVAL(MILTER_STEP_NO_MASK));
    rb_define_const(rb_cMilterStepFlags, "YES_MASK",
		    STEP_FLAGS2RVAL(MILTER_STEP_YES_MASK));

    rb_define_method(rb_cMilterStepFlags, "merge", step_flags_merge, 1);

    rb_define_method(rb_cMilterOption, "initialize", initialize, -1);
    rb_define_method(rb_cMilterOption, "==", equal, 1);
    rb_define_method(rb_cMilterOption, "add_action", add_action, 1);
    rb_define_method(rb_cMilterOption, "remove_step", remove_step, 1);

    G_DEF_SETTERS(rb_cMilterOption);
}
