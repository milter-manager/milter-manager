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

static ID id_equal;

static VALUE
ipv4_initialize (VALUE self, VALUE address, VALUE port)
{
    rb_iv_set(self, "@address", address);
    rb_iv_set(self, "@port", port);

    return Qnil;
}

static VALUE
ipv4_pack (VALUE self)
{
    VALUE address, port;
    struct sockaddr_in socket_address;

    MEMZERO(&socket_address, struct sockaddr_in, 1);

    address = rb_iv_get(self, "@address");
    port = rb_iv_get(self, "@port");

    socket_address.sin_family = AF_INET;
    socket_address.sin_port = htons(NUM2UINT(port));

    if (inet_pton(AF_INET, RVAL2CSTR(address), &(socket_address.sin_addr)) < 0)
        rb_sys_fail("fail to pack IPv4 address");

    return rb_str_new((char *)(&socket_address), sizeof(socket_address));
}

static VALUE
ipv4_equal (VALUE self, VALUE other)
{
    if (!RVAL2CBOOL(rb_obj_is_kind_of(other, rb_cMilterSocketAddressIPv4)))
        return Qfalse;

    return rb_funcall(rb_ary_new3(2,
                                  rb_iv_get(self, "@address"),
                                  rb_iv_get(self, "@port")),
                      id_equal, 1,
                      rb_ary_new3(2,
                                  rb_iv_get(other, "@address"),
                                  rb_iv_get(other, "@port")));
}

static VALUE
ipv4_to_s (VALUE self)
{
    VALUE argv[3];

    argv[1] = rb_iv_get(self, "@port");
    argv[2] = rb_iv_get(self, "@address");
    if (NIL_P(argv[2])) {
	argv[0] = rb_str_new2("inet:%d");
        return rb_f_sprintf(2, argv);
    } else {
	argv[0] = rb_str_new2("inet:%d@[%s]");
        return rb_f_sprintf(3, argv);
    }
}

static VALUE
ipv6_initialize (VALUE self, VALUE address, VALUE port)
{
    rb_iv_set(self, "@address", address);
    rb_iv_set(self, "@port", port);

    return Qnil;
}

static VALUE
ipv6_pack (VALUE self)
{
    VALUE address, port;
    struct sockaddr_in6 socket_address;

    address = rb_iv_get(self, "@address");
    port = rb_iv_get(self, "@port");

    MEMZERO(&socket_address, struct sockaddr_in6, 1);

    socket_address.sin6_family = AF_INET6;
    socket_address.sin6_port = htons(NUM2UINT(port));

    if (inet_pton(AF_INET6, RVAL2CSTR(address), &(socket_address.sin6_addr)) < 0)
        rb_sys_fail("fail to pack IPv6 address");

    return rb_str_new((char *)(&socket_address), sizeof(socket_address));
}

static VALUE
ipv6_equal (VALUE self, VALUE other)
{
    if (!RVAL2CBOOL(rb_obj_is_kind_of(other, rb_cMilterSocketAddressIPv6)))
        return Qfalse;

    return rb_funcall(rb_ary_new3(2,
                                  rb_iv_get(self, "@address"),
                                  rb_iv_get(self, "@port")),
                      id_equal, 1,
                      rb_ary_new3(2,
                                  rb_iv_get(other, "@address"),
                                  rb_iv_get(other, "@port")));
}

static VALUE
ipv6_to_s (VALUE self)
{
    VALUE argv[3];

    argv[1] = rb_iv_get(self, "@port");
    argv[2] = rb_iv_get(self, "@address");
    if (NIL_P(argv[2])) {
	argv[0] = rb_str_new2("inet6:%d");
        return rb_f_sprintf(2, argv);
    } else {
	argv[0] = rb_str_new2("inet6:%d@[%s]");
        return rb_f_sprintf(3, argv);
    }
}

static VALUE
unix_initialize (VALUE self, VALUE path)
{
    rb_iv_set(self, "@path", path);

    return Qnil;
}

static VALUE
unix_pack (VALUE self)
{
    VALUE path;
    struct sockaddr_un address;
    int path_length;

    path = rb_iv_get(self, "@path");

    MEMZERO(&address, struct sockaddr_un, 1);

    address.sun_family = AF_UNIX;
    path_length = RSTRING_LEN(path);
    if (path_length > sizeof(address.sun_path))
        path_length = sizeof(address.sun_path);
    strncpy(address.sun_path, RVAL2CSTR(path), path_length);
    address.sun_path[path_length] = '\0';

    return rb_str_new((char *)(&address), sizeof(address));
}

static VALUE
unix_equal (VALUE self, VALUE other)
{
    if (!RVAL2CBOOL(rb_obj_is_kind_of(other, rb_cMilterSocketAddressUnix)))
        return Qfalse;

    return rb_funcall(rb_iv_get(self, "@path"),
                      id_equal, 1,
                      rb_iv_get(other, "@path"));
}

static VALUE
unix_to_s (VALUE self)
{
    VALUE argv[2];

    argv[0] = rb_str_new2("unix:%s");
    argv[1] = rb_iv_get(self, "@path");
    return rb_f_sprintf(2, argv);
}

static VALUE
unknown_equal (VALUE self, VALUE other)
{
    return RVAL2CBOOL(rb_obj_is_kind_of(other, rb_cMilterSocketAddressUnknown));
}

static VALUE
unknown_to_s (VALUE self)
{
    return rb_str_new2("unknown");
}

VALUE rb_cMilterSocketAddressIPv4, rb_cMilterSocketAddressIPv6;
VALUE rb_cMilterSocketAddressUnix, rb_cMilterSocketAddressUnknown;

void
Init_milter_socket_address (void)
{
    VALUE rb_mMilterSocketAddress;

    id_equal = rb_intern("==");

    rb_mMilterSocketAddress =
        rb_define_module_under(rb_mMilter, "SocketAddress");

    rb_cMilterSocketAddressIPv4 =
        rb_define_class_under(rb_mMilterSocketAddress, "IPv4", rb_cObject);
    rb_cMilterSocketAddressIPv6 =
        rb_define_class_under(rb_mMilterSocketAddress, "IPv6", rb_cObject);
    rb_cMilterSocketAddressUnix =
        rb_define_class_under(rb_mMilterSocketAddress, "Unix", rb_cObject);
    rb_cMilterSocketAddressUnknown =
        rb_define_class_under(rb_mMilterSocketAddress, "Unknown", rb_cObject);

    rb_define_attr(rb_cMilterSocketAddressIPv4, "address", TRUE, TRUE);
    rb_define_attr(rb_cMilterSocketAddressIPv4, "port", TRUE, TRUE);
    rb_define_method(rb_cMilterSocketAddressIPv4, "initialize",
                     ipv4_initialize, 2);
    rb_define_method(rb_cMilterSocketAddressIPv4, "pack", ipv4_pack, 0);
    rb_define_method(rb_cMilterSocketAddressIPv4, "==", ipv4_equal, 1);
    rb_define_method(rb_cMilterSocketAddressIPv4, "to_s", ipv4_to_s, 0);

    rb_define_attr(rb_cMilterSocketAddressIPv6, "address", TRUE, TRUE);
    rb_define_attr(rb_cMilterSocketAddressIPv6, "port", TRUE, TRUE);
    rb_define_method(rb_cMilterSocketAddressIPv6, "initialize",
                     ipv6_initialize, 2);
    rb_define_method(rb_cMilterSocketAddressIPv6, "pack", ipv6_pack, 0);
    rb_define_method(rb_cMilterSocketAddressIPv6, "==", ipv6_equal, 1);
    rb_define_method(rb_cMilterSocketAddressIPv6, "to_s", ipv6_to_s, 0);

    rb_define_attr(rb_cMilterSocketAddressUnix, "path", TRUE, TRUE);
    rb_define_method(rb_cMilterSocketAddressUnix, "initialize",
                     unix_initialize, 1);
    rb_define_method(rb_cMilterSocketAddressUnix, "pack", unix_pack, 0);
    rb_define_method(rb_cMilterSocketAddressUnix, "==", unix_equal, 1);
    rb_define_method(rb_cMilterSocketAddressUnix, "to_s", unix_to_s, 0);

    rb_define_method(rb_cMilterSocketAddressUnknown, "==", unknown_equal, 1);
    rb_define_method(rb_cMilterSocketAddressUnknown, "to_s", unknown_to_s, 0);
}
