/* -*- c-file-style: "ruby" -*- */

#include "rb-milter-core-private.h"

#define SELF(self) RVAL2OPTION(self)

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

void
Init_milter_option (void)
{
    VALUE rb_cMilterOption;

    rb_cMilterOption = G_DEF_CLASS(MILTER_TYPE_OPTION, "Option", rb_mMilter);

    G_DEF_CLASS(MILTER_TYPE_ACTION_FLAGS, "ActionFlags", rb_mMilter);
    G_DEF_CLASS(MILTER_TYPE_STEP_FLAGS, "StepFlags", rb_mMilter);

    G_DEF_CONSTANTS(rb_mMilter, MILTER_TYPE_ACTION_FLAGS, "MILTER_");
    G_DEF_CONSTANTS(rb_mMilter, MILTER_TYPE_STEP_FLAGS, "MILTER_");

    rb_define_method(rb_cMilterOption, "initialize", initialize, -1);
    rb_define_method(rb_cMilterOption, "==", equal, 1);

    G_DEF_SETTERS(rb_cMilterOption);
}
