/* -*- c-file-style: "ruby" -*- */
/*
 *  Copyright (C) 2008-2011  Kouhei Sutou <kou@clear-code.com>
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
initialize (int argc, VALUE *argv, VALUE self)
{
    VALUE client;
    MilterClientContext *context;

    rb_scan_args(argc, argv, "01", &client);

    context = milter_client_context_new(RVAL2GOBJ(client));
    G_INITIALIZE(self, context);

    return Qnil;
}

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
quarantine (VALUE self, VALUE reason)
{
    return CBOOL2RVAL(milter_client_context_quarantine(SELF(self), StringValueCStr(reason)));
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

    return self;
}

static VALUE
insert_header (VALUE self, VALUE index, VALUE name, VALUE value)
{
    GError *error = NULL;

    if (!milter_client_context_insert_header(SELF(self),
					     NUM2UINT(index),
					     RVAL2CSTR(name),
					     RVAL2CSTR(value),
					     &error))
        RAISE_GERROR(error);

    return self;
}

static VALUE
change_header (VALUE self, VALUE name, VALUE index, VALUE value)
{
    GError *error = NULL;

    if (!milter_client_context_change_header(SELF(self),
                                             RVAL2CSTR(name),
                                             NUM2UINT(index),
                                             RVAL2CSTR(value),
                                             &error))
        RAISE_GERROR(error);

    return self;
}

static VALUE
delete_header (VALUE self, VALUE name, VALUE index)
{
    GError *error = NULL;

    if (!milter_client_context_delete_header(SELF(self),
                                             RVAL2CSTR(name),
                                             NUM2UINT(index),
                                             &error))
        RAISE_GERROR(error);

    return self;
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

    return self;
}

static VALUE
format_reply (VALUE self)
{
    return CSTR2RVAL_FREE(milter_client_context_format_reply(SELF(self)));
}

static VALUE
change_from (int argc, VALUE *argv, VALUE self)
{
    VALUE rb_from, rb_parameters;
    const gchar *from, *parameters = NULL;
    GError *error = NULL;

    rb_scan_args(argc, argv, "11", &rb_from, &rb_parameters);
    from = RVAL2CSTR(rb_from);
    if (!NIL_P(rb_parameters))
	parameters = RVAL2CSTR(rb_parameters);
    if (!milter_client_context_change_from(SELF(self), from, parameters, &error))
        RAISE_GERROR(error);

    return self;
}

static VALUE
add_recipient (int argc, VALUE *argv, VALUE self)
{
    VALUE rb_recipient, rb_parameters;
    const gchar *recipient, *parameters = NULL;
    GError *error = NULL;

    rb_scan_args(argc, argv, "11", &rb_recipient, &rb_parameters);
    recipient = RVAL2CSTR(rb_recipient);
    if (!NIL_P(rb_parameters))
	parameters = RVAL2CSTR(rb_parameters);
    if (!milter_client_context_add_recipient(SELF(self),
					     recipient, parameters,
					     &error))
	RAISE_GERROR(error);

    return self;
}

static VALUE
delete_recipient (VALUE self, VALUE recipient)
{
    GError *error = NULL;

    if (!milter_client_context_delete_recipient(SELF(self),
						RVAL2CSTR(recipient),
						&error))
	RAISE_GERROR(error);

    return self;
}

static VALUE
replace_body (VALUE self, VALUE rb_chunk)
{
    GError *error = NULL;
    const gchar *chunk;

    chunk = RVAL2CSTR(rb_chunk);
    if (!milter_client_context_replace_body(SELF(self),
					    chunk, RSTRING_LEN(rb_chunk),
					    &error))
	RAISE_GERROR(error);

    return self;
}

static VALUE
get_socket_address (VALUE self)
{
    MilterGenericSocketAddress *address;
    socklen_t address_length;

    address = milter_client_context_get_socket_address(SELF(self));
    if (!address)
	return Qnil;

    switch (address->address.base.sa_family) {
      case AF_UNIX:
	address_length = sizeof(struct sockaddr_un);
	break;
      case AF_INET:
	address_length = sizeof(struct sockaddr_in);
	break;
      case AF_INET6:
	address_length = sizeof(struct sockaddr_in6);
	break;
      default:
	return Qnil;
	break;
    }

    return ADDRESS2RVAL(&(address->address.base), address_length);
}

static VALUE
get_n_processing_sessions (VALUE self)
{
    guint n_processing_sessions;

    n_processing_sessions =
	milter_client_context_get_n_processing_sessions(SELF(self));
    return UINT2NUM(n_processing_sessions);
}

static VALUE
set_packet_buffer_size (VALUE self, VALUE size)
{
    milter_client_context_set_packet_buffer_size(SELF(self), NUM2UINT(size));
    return self;
}

static VALUE
get_packet_buffer_size (VALUE self)
{
    guint packet_buffer_size;

    packet_buffer_size =
	milter_client_context_get_packet_buffer_size(SELF(self));
    return UINT2NUM(packet_buffer_size);
}

void
Init_milter_client_context (void)
{
    VALUE rb_cMilterClientContext;

    rb_cMilterClientContext = G_DEF_CLASS(MILTER_TYPE_CLIENT_CONTEXT,
                                          "ClientContext", rb_mMilter);
    G_DEF_ERROR2(MILTER_CLIENT_CONTEXT_ERROR,
		 "ClientContextError", rb_mMilter, rb_eMilterError);

    G_DEF_CLASS(MILTER_TYPE_CLIENT_CONTEXT_STATE, "ClientContextState",
		rb_mMilter);
    G_DEF_CONSTANTS(rb_cMilterClientContext, MILTER_TYPE_CLIENT_CONTEXT_STATE,
                    "MILTER_CLIENT_CONTEXT_");

    rb_define_method(rb_cMilterClientContext, "initialize", initialize, -1);
    rb_define_method(rb_cMilterClientContext, "feed", feed, 1);
    rb_define_method(rb_cMilterClientContext, "progress", progress, 0);
    rb_define_method(rb_cMilterClientContext, "quarantine", quarantine, 1);
    rb_define_method(rb_cMilterClientContext, "add_header", add_header, 2);
    rb_define_method(rb_cMilterClientContext, "insert_header", insert_header, 3);
    rb_define_method(rb_cMilterClientContext, "change_header", change_header, 3);
    rb_define_method(rb_cMilterClientContext, "delete_header", delete_header, 2);
    rb_define_method(rb_cMilterClientContext, "set_reply", set_reply, 3);
    rb_define_method(rb_cMilterClientContext, "format_reply", format_reply, 0);
    rb_define_method(rb_cMilterClientContext, "change_from", change_from, -1);
    rb_define_method(rb_cMilterClientContext, "add_recipient",
		     add_recipient, -1);
    rb_define_method(rb_cMilterClientContext, "delete_recipient",
		     delete_recipient, 1);
    rb_define_method(rb_cMilterClientContext, "replace_body", replace_body, 1);
    rb_define_method(rb_cMilterClientContext, "socket_address",
		     get_socket_address, 0);
    rb_define_method(rb_cMilterClientContext, "n_processing_sessions",
		     get_n_processing_sessions, 0);
    rb_define_method(rb_cMilterClientContext, "set_packet_buffer_size",
		     set_packet_buffer_size, 0);
    rb_define_method(rb_cMilterClientContext, "packet_buffer_size",
		     get_packet_buffer_size, 0);

    G_DEF_SIGNAL_FUNC(rb_cMilterClientContext, "connect",
		      rb_milter__connect_signal_convert);
    G_DEF_SIGNAL_FUNC(rb_cMilterClientContext, "helo",
                      rb_milter__helo_signal_convert);
    G_DEF_SIGNAL_FUNC(rb_cMilterClientContext, "envelope-from",
                      rb_milter__envelope_from_signal_convert);
    G_DEF_SIGNAL_FUNC(rb_cMilterClientContext, "envelope-recipient",
                      rb_milter__envelope_recipient_signal_convert);
    G_DEF_SIGNAL_FUNC(rb_cMilterClientContext, "unknown",
                      rb_milter__unknown_signal_convert);
    G_DEF_SIGNAL_FUNC(rb_cMilterClientContext, "header",
                      rb_milter__header_signal_convert);
    G_DEF_SIGNAL_FUNC(rb_cMilterClientContext, "body",
		      rb_milter__body_signal_convert);
    G_DEF_SIGNAL_FUNC(rb_cMilterClientContext, "end-of-message",
		      rb_milter__end_of_message_signal_convert);

    G_DEF_SETTERS(rb_cMilterClientContext);
}

