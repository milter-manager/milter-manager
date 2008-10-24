/* -*- c-file-style: "ruby" -*- */

#include "rb-milter-manager-private.h"

static VALUE
initialize (VALUE self, VALUE name)
{
    G_INITIALIZE(self, milter_manager_child_new(RVAL2CSTR(name)));
    return Qnil;
}

void
Init_milter_manager_child (void)
{
    VALUE rb_cMilterManagerChild;

    rb_cMilterManagerChild =
	G_DEF_CLASS(MILTER_TYPE_MANAGER_CHILD, "Child", rb_mMilterManager);

    rb_define_method(rb_cMilterManagerChild, "initialize",
                     initialize, 1);
}
