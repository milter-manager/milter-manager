/* -*- c-file-style: "ruby" -*- */

#include "rb-milter-core-private.h"

void
Init_milter_encoder (void)
{
    VALUE rb_cMilterEncoder;

    rb_cMilterEncoder = G_DEF_CLASS(MILTER_TYPE_ENCODER, "Encoder", rb_mMilter);

    G_DEF_SETTERS(rb_cMilterEncoder);
}
