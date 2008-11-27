/* -*- c-file-style: "ruby" -*- */

#include "rb-milter-core-private.h"

void
Init_milter_utils (void)
{
    VALUE rb_mMilterUtils;

    rb_mMilterUtils = rb_define_module_under(rb_mMilter, "Utils");
}
