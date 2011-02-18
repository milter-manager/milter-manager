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

    switch (address->sa_family) {
      case AF_INET:
	{
	    struct sockaddr_in *address_in = (struct sockaddr_in *)address;
	    gchar ip_address[INET_ADDRSTRLEN];
	    guint16 port;
	    gboolean success;

	    success = (NULL != inet_ntop(AF_INET, &(address_in->sin_addr),
					 ip_address, sizeof(ip_address)));
	    port = ntohs(address_in->sin_port);
	    g_free(address);
	    if (!success)
		rb_sys_fail("failed to convert IP address to string");
	    rb_address = rb_funcall(rb_cMilterSocketAddressIPv4,
				    id_new,
				    2,
				    CSTR2RVAL(ip_address),
				    UINT2NUM(port));
	}
	break;
      case AF_INET6:
	{
	    struct sockaddr_in6 *address_in6 = (struct sockaddr_in6 *)address;
	    gchar ipv6_address[INET6_ADDRSTRLEN];
	    guint16 port;
	    gboolean success;

	    success = (NULL != inet_ntop(AF_INET6, &(address_in6->sin6_addr),
					 ipv6_address, sizeof(ipv6_address)));
	    port = ntohs(address_in6->sin6_port);
	    g_free(address);
	    if (!success)
		rb_sys_fail("failed to convert IPv6 address to string");
	    rb_address = rb_funcall(rb_cMilterSocketAddressIPv6,
				    id_new,
				    2,
				    CSTR2RVAL(ipv6_address),
				    UINT2NUM(port));
	}
	break;
      case AF_UNIX:
	{
	    struct sockaddr_un *address_un = (struct sockaddr_un *)address;
	    VALUE rb_path;

	    rb_path = CSTR2RVAL(address_un->sun_path);
	    g_free(address);
	    rb_address = rb_funcall(rb_cMilterSocketAddressUnix,
				    id_new,
				    1,
				    rb_path);
	}
	break;
      case AF_UNSPEC:
	g_free(address);
	rb_address = rb_funcall(rb_cMilterSocketAddressUnknown, id_new, 0);
	break;
      default:
	rb_address = rb_str_new((gchar *)address, address_size);
	g_free(address);
	break;
    }

    return rb_address;
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
