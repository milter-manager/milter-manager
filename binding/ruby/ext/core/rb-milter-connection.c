/* -*- c-file-style: "ruby" -*- */
/*
 *  Copyright (C) 2008-2022  Sutou Kouhei <kou@clear-code.com>
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

static ID id_new;

static VALUE
parse_spec (VALUE self, VALUE spec)
{
    gint domain;
    struct sockaddr *address;
    socklen_t address_size;
    VALUE rb_address;
    GError *error = NULL;

    if (!milter_connection_parse_spec(RVAL2CSTR(spec),
				      &domain, &address, &address_size,
				      &error))
	RAISE_GERROR(error);

    return ADDRESS2RVAL_FREE(address, address_size);
}

void
Init_milter_connection (void)
{
    VALUE rb_mMilterConnection;

    id_new = rb_intern("new");

    rb_mMilterConnection = rb_define_module_under(rb_mMilter, "Connection");

    G_DEF_ERROR2(MILTER_CONNECTION_ERROR, "ConnectionError",
		 rb_mMilter, rb_eMilterError);

    rb_define_module_function(rb_mMilterConnection, "parse_spec",
			      parse_spec, 1);
}
