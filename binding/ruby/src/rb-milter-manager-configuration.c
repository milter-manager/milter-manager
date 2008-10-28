/* -*- c-file-style: "ruby" -*- */

#include "rb-milter-manager-private.h"

#define SELF(self) (MILTER_MANAGER_CONFIGURATION(RVAL2GOBJ(self)))

static VALUE
add_egg (VALUE self, VALUE egg)
{
    milter_manager_configuration_add_egg(SELF(self), RVAL2GOBJ(egg));

    return self;
}

static VALUE
create_children (VALUE self)
{
    MilterManagerChildren *children;

    children = milter_manager_configuration_create_children(SELF(self));
    return GOBJ2RVAL_UNREF(children);
}

void
Init_milter_manager_configuration (void)
{
    VALUE rb_cMilterManagerConfiguration;

    rb_cMilterManagerConfiguration =
	G_DEF_CLASS(MILTER_TYPE_MANAGER_CONFIGURATION,
		    "Configuration",
		    rb_mMilterManager);

    rb_define_method(rb_cMilterManagerConfiguration, "add_egg", add_egg, 1);
    rb_define_method(rb_cMilterManagerConfiguration,
		     "create_children", create_children, 0);
}
