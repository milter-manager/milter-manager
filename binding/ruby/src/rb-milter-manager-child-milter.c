/* -*- c-file-style: "ruby" -*- */

#include "rb-milter-manager-private.h"

static VALUE
initialize (VALUE self, VALUE name)
{
    G_INITIALIZE(self, milter_manager_child_milter_new(RVAL2CSTR(name)));
    return Qnil;
}

void
Init_milter_manager_child_milter (void)
{
    VALUE rb_cMilterManagerChildMilter;

    rb_cMilterManagerChildMilter =
	G_DEF_CLASS(MILTER_MANAGER_TYPE_CHILD_MILTER,
		    "ChildMilter",
		    rb_mMilterManager);

    rb_define_method(rb_cMilterManagerChildMilter, "initialize",
                     initialize, 1);
}
