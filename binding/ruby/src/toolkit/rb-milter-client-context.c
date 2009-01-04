/* -*- c-file-style: "ruby" -*- */
/*
 *  Copyright (C) 2008  Kouhei Sutou <kou@cozmixng.org>
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
