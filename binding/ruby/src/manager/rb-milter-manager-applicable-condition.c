/* -*- c-file-style: "ruby" -*- */

#include "rb-milter-manager-private.h"

#define SELF(self) (MILTER_MANAGER_APPLICABLE_CONDITION(RVAL2GOBJ(self)))

static VALUE
initialize (VALUE self, VALUE name)
{
    G_INITIALIZE(self, milter_manager_applicable_condition_new(RVAL2CSTR(name)));
    return Qnil;
}

static VALUE
merge (VALUE self, VALUE other)
{
    MilterManagerApplicableCondition *condition, *other_condition;

    condition = SELF(self);
    other_condition = SELF(other);
    milter_manager_applicable_condition_merge(condition, other_condition);
    return Qnil;
}

void
Init_milter_manager_applicable_condition (void)
{
    VALUE rb_cMilterManagerApplicableCondition;

    rb_cMilterManagerApplicableCondition =
	G_DEF_CLASS(MILTER_TYPE_MANAGER_APPLICABLE_CONDITION,
                    "ApplicableCondition", rb_mMilterManager);

    rb_define_method(rb_cMilterManagerApplicableCondition,
                     "initialize", initialize, 1);
    rb_define_method(rb_cMilterManagerApplicableCondition,
                     "merge", merge, 1);

    G_DEF_SETTERS(rb_cMilterManagerApplicableCondition);
}
