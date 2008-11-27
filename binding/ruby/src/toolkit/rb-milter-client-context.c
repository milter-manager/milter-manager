/* -*- c-file-style: "ruby" -*- */

#include "rb-milter-client-private.h"

#define SELF(self) (MILTER_CLIENT_CONTEXT(RVAL2GOBJ(self)))

static VALUE
feed (VALUE self, VALUE chunk)
{
    GError *error = NULL;

    if (!milter_client_context_feed(SELF(self),
                                    RSTRING_PTR(chunk), RSTRING_LEN(chunk),
                                    &error))
	RAISE_GERROR(error);

    return self;
}

void
Init_milter_client_context (void)
{
    VALUE rb_cMilterClientContext;

    rb_cMilterClientContext = G_DEF_CLASS(MILTER_TYPE_CLIENT_CONTEXT,
                                          "ClientContext", rb_mMilter);
    G_DEF_ERROR2(MILTER_CLIENT_CONTEXT_ERROR,
		 "ClientContextError", rb_mMilter, rb_eMilterError);

    rb_define_method(rb_cMilterClientContext, "feed", feed, 1);

    G_DEF_SETTERS(rb_cMilterClientContext);
}
