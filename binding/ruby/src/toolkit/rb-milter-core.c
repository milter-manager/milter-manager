/* -*- c-file-style: "ruby" -*- */

#include "rb-milter-core-private.h"

void
Init_milter_core (void)
{
    rb_mMilterCore = rb_define_module_under(rb_mMilter, "Core");

    Init_milter_socket_address();
    Init_milter_utils();
    Init_milter_connection();
    Init_milter_protocol();
    Init_milter_option();
    Init_milter_macros_requests();
    Init_milter_encoder();
    Init_milter_command_encoder();
    Init_milter_reply_encoder();
    Init_milter_decoder();
}
