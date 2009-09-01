/* -*- c-file-style: "ruby" -*- */
/*
 *  Copyright (C) 2008-2009  Kouhei Sutou <kou@clear-code.com>
 *
 *  This library is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this library.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

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

static VALUE
progress (VALUE self)
{
    return CBOOL2RVAL(milter_client_context_progress(SELF(self)));
}

static VALUE
add_header (VALUE self, VALUE name, VALUE value)
{
    GError *error = NULL;

    if (!milter_client_context_add_header(SELF(self),
                                          StringValueCStr(name),
                                          StringValueCStr(value),
                                          &error))
        RAISE_GERROR(error);

    return Qnil;
}

static VALUE
set_reply (VALUE self, VALUE code, VALUE extended_code, VALUE message)
{
    GError *error = NULL;

    if (!milter_client_context_set_reply(SELF(self),
                                         NUM2UINT(code),
                                         StringValueCStr(extended_code),
                                         StringValueCStr(message),
                                         &error))
        RAISE_GERROR(error);

    return Qnil;
}

static VALUE
format_reply (VALUE self)
{
    return CSTR2RVAL_FREE(milter_client_context_format_reply(SELF(self)));
}

void
Init_milter_client_context (void)
{
    VALUE rb_cMilterClientContext;

    rb_cMilterClientContext = G_DEF_CLASS(MILTER_TYPE_CLIENT_CONTEXT,
                                          "ClientContext", rb_mMilter);
    G_DEF_ERROR2(MILTER_CLIENT_CONTEXT_ERROR,
		 "ClientContextError", rb_mMilter, rb_eMilterError);
    
    G_DEF_CLASS(MILTER_TYPE_CLIENT_CONTEXT_STATE, "ClientContextState", rb_mMilter);
    G_DEF_CONSTANTS(rb_cMilterClientContext, MILTER_TYPE_CLIENT_CONTEXT_STATE,
                    "MILTER_CLIENT_CONTEXT_");

    rb_define_method(rb_cMilterClientContext, "feed", feed, 1);
    rb_define_method(rb_cMilterClientContext, "progress", progress, 0);
    rb_define_method(rb_cMilterClientContext, "add_header", add_header, 2);
    rb_define_method(rb_cMilterClientContext, "set_reply", set_reply, 3);
    rb_define_method(rb_cMilterClientContext, "format_reply", format_reply, 0);

    G_DEF_SIGNAL_FUNC(rb_cMilterClientContext, "connect",
		      rb_milter__connect_signal_convert);
    G_DEF_SIGNAL_FUNC(rb_cMilterClientContext, "body",
		      rb_milter__body_signal_convert);
    G_DEF_SIGNAL_FUNC(rb_cMilterClientContext, "end-of-message",
		      rb_milter__end_of_message_signal_convert);

    G_DEF_SETTERS(rb_cMilterClientContext);
}

