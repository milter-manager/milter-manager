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
