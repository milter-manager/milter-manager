/* -*- c-file-style: "ruby" -*- */

#include "rb-milter-server-private.h"

void
Init_milter_server (void)
{
    rb_mMilterServer = rb_define_module_under(rb_mMilter, "Server");
    Init_milter_server_context();
}
