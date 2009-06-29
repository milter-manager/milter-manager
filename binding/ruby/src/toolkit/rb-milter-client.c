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

#define SELF(self) RVAL2GOBJ(self)

VALUE rb_cMilterClient;

static VALUE
client_main (VALUE self)
{
    milter_client_main(SELF(self));
    return Qnil;
}

static VALUE
client_shutdown (VALUE self)
{
    milter_client_shutdown(SELF(self));
    return Qnil;
}

void
Init_milter_client (void)
{
    rb_cMilterClient = G_DEF_CLASS(MILTER_TYPE_CLIENT, "Client", rb_mMilter);

    rb_define_const(rb_cMilterClient,
		    "DEFAULT_SUSPEND_TIME_ON_UNACCEPTABLE",
		    UINT2NUM(MILTER_CLIENT_DEFAULT_SUSPEND_TIME_ON_UNACCEPTABLE));

    rb_define_method(rb_cMilterClient, "main", client_main, 0);
    rb_define_method(rb_cMilterClient, "shutdown", client_shutdown, 0);

    Init_milter_client_context();
}
