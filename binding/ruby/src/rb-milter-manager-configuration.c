/* -*- c-file-style: "ruby" -*- */

#include "rb-milter-manager-private.h"

#define SELF(self) (MILTER_MANAGER_CONFIGURATION(RVAL2GOBJ(self)))

static VALUE
add_child (VALUE self, VALUE milter)
{
    milter_manager_configuration_add_child(SELF(self), RVAL2GOBJ(milter));

    return self;
}

void
Init_milter_manager_configuration (void)
{
    VALUE rb_cMilterManagerConfiguration;

    rb_cMilterManagerConfiguration =
	G_DEF_CLASS(MILTER_TYPE_MANAGER_CONFIGURATION,
		    "Configuration",
		    rb_mMilterManager);

    rb_define_method(rb_cMilterManagerConfiguration, "add_child", add_child, 1);
}
