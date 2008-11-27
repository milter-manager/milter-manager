/* -*- c-file-style: "ruby" -*- */

#include "rb-milter-core-private.h"

void
Init_milter_protocol (void)
{
    G_DEF_CLASS(MILTER_TYPE_STATUS, "Status", rb_mMilter);
    G_DEF_CONSTANTS(rb_mMilter, MILTER_TYPE_STATUS, "MILTER_");

    G_DEF_CLASS(MILTER_TYPE_COMMAND, "Command", rb_mMilter);
    G_DEF_CONSTANTS(rb_mMilter, MILTER_TYPE_COMMAND, "MILTER_");
}
