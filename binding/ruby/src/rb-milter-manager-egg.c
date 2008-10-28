/* -*- c-file-style: "ruby" -*- */

#include "rb-milter-manager-private.h"

static VALUE
initialize (VALUE self, VALUE name)
{
    G_INITIALIZE(self, milter_manager_egg_new(RVAL2CSTR(name)));
    return Qnil;
}

void
Init_milter_manager_egg (void)
{
    VALUE rb_cMilterManagerEgg;

    rb_cMilterManagerEgg =
	G_DEF_CLASS(MILTER_TYPE_MANAGER_EGG, "Egg", rb_mMilterManager);

    rb_define_method(rb_cMilterManagerEgg, "initialize", initialize, 1);
}
