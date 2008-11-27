/* -*- c-file-style: "ruby" -*- */

#include "rb-milter-client-private.h"

void
Init_milter_client (void)
{
    rb_mMilterClient = rb_define_module_under(rb_mMilter, "Client");

    Init_milter_client_context();
}
