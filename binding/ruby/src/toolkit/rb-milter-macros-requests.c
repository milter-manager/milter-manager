/* -*- c-file-style: "ruby" -*- */

#include "rb-milter-core-private.h"

#define SELF(self) RVAL2MACROS_REQUESTS(self)

static VALUE
initialize (VALUE self)
{
    MilterMacrosRequests *requests;

    requests = milter_macros_requests_new();
    G_INITIALIZE(self, requests);

    return Qnil;
}

void
Init_milter_macros_requests (void)
{
    VALUE rb_cMilterMacrosRequests;

    rb_cMilterMacrosRequests =
        G_DEF_CLASS(MILTER_TYPE_MACROS_REQUESTS, "MacrosRequests", rb_mMilter);

    rb_define_method(rb_cMilterMacrosRequests, "initialize", initialize, 0);

    G_DEF_SETTERS(rb_cMilterMacrosRequests);
}
