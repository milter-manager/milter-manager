/* -*- c-file-style: "ruby" -*- */

#include "rb-milter-manager-private.h"

#define SELF(self) (MILTER_MANAGER_CHILDREN(RVAL2GOBJ(self)))

static VALUE
initialize (VALUE self, VALUE configuration)
{
    G_INITIALIZE(self, milter_manager_children_new(RVAL2GOBJ(configuration)));
    return Qnil;
}

static VALUE
length (VALUE self)
{
    return UINT2NUM(milter_manager_children_length(SELF(self)));
}

void
Init_milter_manager_children (void)
{
    VALUE rb_cMilterManagerChildren;

    rb_cMilterManagerChildren =
	G_DEF_CLASS(MILTER_TYPE_MANAGER_CHILDREN, "Children", rb_mMilterManager);

    rb_define_method(rb_cMilterManagerChildren, "initialize",
                     initialize, 1);
    rb_define_method(rb_cMilterManagerChildren, "length", length, 0);
}
