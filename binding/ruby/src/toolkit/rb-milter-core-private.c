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

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <errno.h>

#include "rb-milter-core-private.h"

static VALUE
rval2macro (VALUE data, VALUE user_data)
{
    VALUE key, value;
    GHashTable *macros = (GHashTable *)user_data;

    key = RARRAY_PTR(data)[0];
    value = RARRAY_PTR(data)[1];

    g_hash_table_insert(macros, RVAL2CSTR(key), RVAL2CSTR(value));

    return Qnil;
}

GHashTable *
rb_milter__rval2macros (VALUE rb_macros)
{
    GHashTable *macros;

    macros = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
    rb_iterate(rb_each, rb_macros, rval2macro, (VALUE)macros);
    return macros;
}

static void
macro2rval (gpointer key, gpointer value, gpointer user_data)
{
    VALUE rb_macros = (VALUE)user_data;

    rb_hash_aset(rb_macros, CSTR2RVAL(key), CSTR2RVAL(value));
}

VALUE
rb_milter__macros2rval (GHashTable *macros)
{
    VALUE rb_macros;

    if (!macros)
	return Qnil;

    rb_macros = rb_hash_new();
    g_hash_table_foreach(macros, macro2rval, (gpointer)rb_macros);
    return rb_macros;
}

VALUE
rb_milter__address2rval (struct sockaddr *address, socklen_t address_length)
{
    static ID id_new = 0;
    VALUE rb_address = Qnil;

    if (id_new == 0)
	id_new = rb_intern("new");

    switch (address->sa_family) {
      case AF_INET:
	{
	    gchar address_string[INET_ADDRSTRLEN];
	    struct sockaddr_in *address_in;

	    address_in = (struct sockaddr_in *)address;
	    if (inet_ntop(AF_INET, &(address_in->sin_addr),
			  address_string, INET_ADDRSTRLEN)) {
		rb_address = rb_funcall(rb_cMilterSocketAddressIPv4, id_new, 2,
					rb_str_new2(address_string),
					UINT2NUM(ntohs(address_in->sin_port)));
	    } else {
		g_warning("fail to unpack IPv4 address: %s", g_strerror(errno));
		rb_address = rb_str_new((char *)address, address_length);
	    }
	}
	break;
      case AF_INET6:
	{
	    gchar address_string[INET6_ADDRSTRLEN];
	    struct sockaddr_in6 *address_in6;

	    address_in6 = (struct sockaddr_in6 *)address;
	    if (inet_ntop(AF_INET6, &(address_in6->sin6_addr),
			  address_string, INET6_ADDRSTRLEN)) {
		rb_address = rb_funcall(rb_cMilterSocketAddressIPv6, id_new, 2,
					rb_str_new2(address_string),
					UINT2NUM(ntohs(address_in6->sin6_port)));
	    } else {
		g_warning("fail to unpack IPv6 address: %s", g_strerror(errno));
		rb_address = rb_str_new((char *)address, address_length);
	    }
	}
	break;
      case AF_UNIX:
	{
	    struct sockaddr_un *address_un;

	    address_un = (struct sockaddr_un *)address;
	    rb_address = rb_funcall(rb_cMilterSocketAddressUnix, id_new, 1,
				    rb_str_new2(address_un->sun_path));
	}
	break;
      case AF_UNSPEC:
	rb_address = rb_funcall(rb_cMilterSocketAddressUnknown, id_new, 0);
	break;
      default:
	g_warning("unexpected family: %d", address->sa_family);
	rb_address = rb_str_new((char *)address, address_length);
	break;
    }

    return rb_address;
}

VALUE
rb_milter__connect_signal_convert (guint num, const GValue *values)
{
    struct sockaddr *address;
    socklen_t address_length;
    VALUE rb_address = Qnil;

    address = g_value_get_pointer(&values[2]);
    address_length = g_value_get_uint(&values[3]);
    rb_address = ADDRESS2RVAL(address, address_length);

    return rb_ary_new3(3,
		       GVAL2RVAL(&values[0]),
		       GVAL2RVAL(&values[1]),
		       rb_address);
}

VALUE
rb_milter__body_signal_convert (guint num, const GValue *values)
{
    return rb_ary_new3(2,
		       GVAL2RVAL(&values[0]),
		       rb_str_new(g_value_get_string(&values[1]),
#if GLIB_SIZEOF_SIZE_T == 8
				  g_value_get_uint64(&values[2])
#else
				  g_value_get_uint(&values[2])
#endif
			   ));
}

VALUE
rb_milter__end_of_message_signal_convert (guint num, const GValue *values)
{
    VALUE rb_chunk = Qnil;
    const gchar *chunk;
    gsize size;

    chunk = g_value_get_string(&values[1]);
#if GLIB_SIZEOF_SIZE_T == 8
    size = g_value_get_uint64(&values[2]);
#else
    size = g_value_get_uint(&values[2]);
#endif

    if (chunk && size > 0)
	rb_chunk = rb_str_new(chunk, size);

    return rb_ary_new3(2, GVAL2RVAL(&values[0]), rb_chunk);
}
