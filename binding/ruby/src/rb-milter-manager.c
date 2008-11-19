/* -*- c-file-style: "ruby" -*- */

#include "rb-milter-manager-private.h"

VALUE rb_mMilterManager;

void
Init_milter_manager (void)
{
    rb_mMilterManager = rb_define_module_under(rb_mMilter, "Manager");
    Init_milter_manager_configuration();
    Init_milter_manager_child();
    Init_milter_manager_egg();
    Init_milter_manager_children();
    Init_milter_manager_control_command_encoder();
    Init_milter_manager_control_reply_encoder();
    Init_milter_manager_control_decoder();
}
