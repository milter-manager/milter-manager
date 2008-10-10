/* -*- c-file-style: "ruby" -*- */

#include "rb-milter-manager-private.h"

VALUE rb_mMilterManager;

void
Init_milter_manager (void)
{
    rb_mMilterManager = rb_define_module_under(rb_mMilter, "Manager");
    Init_milter_manager_configuration();
    Init_milter_manager_child_milter();
}
