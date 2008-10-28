/* -*- c-file-style: "ruby" -*- */

#include "rb-milter-manager-private.h"

#define SELF(self) (MILTER_MANAGER_EGG(RVAL2GOBJ(self)))


static VALUE
initialize (VALUE self, VALUE name)
{
    G_INITIALIZE(self, milter_manager_egg_new(RVAL2CSTR(name)));
    return Qnil;
}

static VALUE
set_connection_spec (VALUE self, VALUE spec)
{
    GError *error = NULL;

    if (!milter_manager_egg_set_connection_spec(SELF(self),
						RVAL2CSTR(spec),
						&error))
	RAISE_GERROR(error);

    return Qnil;
}

void
Init_milter_manager_egg (void)
{
    VALUE rb_cMilterManagerEgg;

    rb_cMilterManagerEgg =
	G_DEF_CLASS(MILTER_TYPE_MANAGER_EGG, "Egg", rb_mMilterManager);

    rb_define_method(rb_cMilterManagerEgg, "initialize", initialize, 1);

    rb_define_method(rb_cMilterManagerEgg, "set_connection_spec",
		     set_connection_spec, 1);

    G_DEF_SETTERS(rb_cMilterManagerEgg);
}
