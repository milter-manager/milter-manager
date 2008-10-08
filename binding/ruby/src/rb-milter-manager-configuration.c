/* -*- c-file-style: "ruby" -*- */

#include "rb-milter-manager-private.h"

void
Init_milter_manager_configuration (void)
{
    rb_mMilterCore = rb_define_module_under(rb_mMilter, "Core");
}
