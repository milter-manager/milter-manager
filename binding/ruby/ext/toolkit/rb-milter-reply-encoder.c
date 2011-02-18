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

#include "rb-milter-core-private.h"

#define SELF(self) RVAL2REPLY_ENCODER(self)

static VALUE
initialize (VALUE self)
{
    G_INITIALIZE(self, milter_reply_encoder_new());
    return Qnil;
}

static VALUE
encode_negotiate (int argc, VALUE *argv, VALUE self)
{
    VALUE option, macros_requests;
    const gchar *packet;
    gsize packet_size;

    rb_scan_args(argc, argv, "11", &option, &macros_requests);
    milter_reply_encoder_encode_negotiate(SELF(self),
					  &packet, &packet_size,
					  RVAL2OPTION(option),
					  RVAL2MACROS_REQUESTS(macros_requests));

    return CSTR2RVAL_SIZE(packet, packet_size);
}

static VALUE
encode_continue (VALUE self)
{
    const gchar *packet;
    gsize packet_size;

    milter_reply_encoder_encode_continue(SELF(self),
					 &packet, &packet_size);

    return CSTR2RVAL_SIZE(packet, packet_size);
}

static VALUE
encode_reply_code (VALUE self, VALUE code)
{
    const gchar *packet;
    gsize packet_size;

    milter_reply_encoder_encode_reply_code(SELF(self),
					   &packet, &packet_size,
					   RVAL2CSTR(code));

    return CSTR2RVAL_SIZE(packet, packet_size);
}

static VALUE
encode_temporary_failure (VALUE self)
{
    const gchar *packet;
    gsize packet_size;

    milter_reply_encoder_encode_temporary_failure(SELF(self),
						  &packet, &packet_size);

    return CSTR2RVAL_SIZE(packet, packet_size);
}

static VALUE
encode_reject (VALUE self)
{
    const gchar *packet;
    gsize packet_size;

    milter_reply_encoder_encode_reject(SELF(self),
				       &packet, &packet_size);

    return CSTR2RVAL_SIZE(packet, packet_size);
}

static VALUE
encode_accept (VALUE self)
{
    const gchar *packet;
    gsize packet_size;

    milter_reply_encoder_encode_accept(SELF(self),
				       &packet, &packet_size);

    return CSTR2RVAL_SIZE(packet, packet_size);
}

static VALUE
encode_discard (VALUE self)
{
    const gchar *packet;
    gsize packet_size;

    milter_reply_encoder_encode_discard(SELF(self),
					&packet, &packet_size);

    return CSTR2RVAL_SIZE(packet, packet_size);
}

static VALUE
encode_add_header (VALUE self, VALUE name, VALUE value)
{
    const gchar *packet;
    gsize packet_size;

    milter_reply_encoder_encode_add_header(SELF(self),
					   &packet, &packet_size,
					   RVAL2CSTR(name),
					   RVAL2CSTR(value));

    return CSTR2RVAL_SIZE(packet, packet_size);
}

static VALUE
encode_insert_header (VALUE self, VALUE index, VALUE name, VALUE value)
{
    const gchar *packet;
    gsize packet_size;

    milter_reply_encoder_encode_insert_header(SELF(self),
					      &packet, &packet_size,
					      NUM2UINT(index),
					      RVAL2CSTR(name),
					      RVAL2CSTR(value));

    return CSTR2RVAL_SIZE(packet, packet_size);
}

static VALUE
encode_change_header (VALUE self, VALUE name, VALUE index, VALUE value)
{
    const gchar *packet;
    gsize packet_size;

    milter_reply_encoder_encode_change_header(SELF(self),
					      &packet, &packet_size,
					      RVAL2CSTR(name),
					      NUM2UINT(index),
					      RVAL2CSTR(value));

    return CSTR2RVAL_SIZE(packet, packet_size);
}

static VALUE
encode_delete_header (VALUE self, VALUE name, VALUE index)
{
    const gchar *packet;
    gsize packet_size;

    milter_reply_encoder_encode_delete_header(SELF(self),
					      &packet, &packet_size,
					      RVAL2CSTR(name),
					      NUM2UINT(index));

    return CSTR2RVAL_SIZE(packet, packet_size);
}

static VALUE
encode_change_from (int argc, VALUE *argv, VALUE self)
{
    VALUE from, parameters;
    const gchar *packet;
    gsize packet_size;

    rb_scan_args(argc, argv, "11", &from, &parameters);
    milter_reply_encoder_encode_change_from(SELF(self),
					    &packet, &packet_size,
					    RVAL2CSTR(from),
					    RVAL2CSTR_ACCEPT_NIL(parameters));

    return CSTR2RVAL_SIZE(packet, packet_size);
}

static VALUE
encode_add_recipient (int argc, VALUE *argv, VALUE self)
{
    VALUE to, parameters;
    const gchar *packet;
    gsize packet_size;

    rb_scan_args(argc, argv, "11", &to, &parameters);
    milter_reply_encoder_encode_add_recipient(SELF(self),
					      &packet, &packet_size,
					      RVAL2CSTR(to),
					      RVAL2CSTR_ACCEPT_NIL(parameters));

    return CSTR2RVAL_SIZE(packet, packet_size);
}

static VALUE
encode_delete_recipient (VALUE self, VALUE to)
{
    const gchar *packet;
    gsize packet_size;

    milter_reply_encoder_encode_delete_recipient(SELF(self),
						 &packet, &packet_size,
						 RVAL2CSTR(to));

    return CSTR2RVAL_SIZE(packet, packet_size);
}

static VALUE
encode_replace_body (VALUE self, VALUE chunk)
{
    const gchar *packet;
    gsize packet_size, packed_size;

    if (!RVAL2CBOOL(rb_obj_is_kind_of(chunk, rb_cString)))
	rb_raise(rb_eArgError,
		 "chunk should be a String: %s",
		 RBG_INSPECT(chunk));
    milter_reply_encoder_encode_replace_body(SELF(self),
					     &packet, &packet_size,
					     RSTRING_PTR(chunk),
					     RSTRING_LEN(chunk),
					     &packed_size);

    return rb_ary_new3(2,
		       CSTR2RVAL_SIZE(packet, packet_size),
		       UINT2NUM(packed_size));
}

static VALUE
encode_progress (VALUE self)
{
    const gchar *packet;
    gsize packet_size;

    milter_reply_encoder_encode_progress(SELF(self),
					 &packet, &packet_size);

    return CSTR2RVAL_SIZE(packet, packet_size);
}

static VALUE
encode_quarantine (VALUE self, VALUE reason)
{
    const gchar *packet;
    gsize packet_size;

    milter_reply_encoder_encode_quarantine(SELF(self),
					   &packet, &packet_size,
					   RVAL2CSTR(reason));

    return CSTR2RVAL_SIZE(packet, packet_size);
}

static VALUE
encode_connection_failure (VALUE self)
{
    const gchar *packet;
    gsize packet_size;

    milter_reply_encoder_encode_connection_failure(SELF(self),
						   &packet, &packet_size);

    return CSTR2RVAL_SIZE(packet, packet_size);
}

static VALUE
encode_shutdown (VALUE self)
{
    const gchar *packet;
    gsize packet_size;

    milter_reply_encoder_encode_shutdown(SELF(self),
					 &packet, &packet_size);

    return CSTR2RVAL_SIZE(packet, packet_size);
}

static VALUE
encode_skip (VALUE self)
{
    const gchar *packet;
    gsize packet_size;

    milter_reply_encoder_encode_skip(SELF(self),
				     &packet, &packet_size);

    return CSTR2RVAL_SIZE(packet, packet_size);
}

void
Init_milter_reply_encoder (void)
{
    VALUE rb_cMilterReplyEncoder;

    rb_cMilterReplyEncoder =
	G_DEF_CLASS(MILTER_TYPE_REPLY_ENCODER, "ReplyEncoder", rb_mMilter);

    rb_define_method(rb_cMilterReplyEncoder, "initialize",
                     initialize, 0);

    rb_define_method(rb_cMilterReplyEncoder, "encode_negotiate",
                     encode_negotiate, -1);
    rb_define_method(rb_cMilterReplyEncoder, "encode_continue",
                     encode_continue, 0);
    rb_define_method(rb_cMilterReplyEncoder, "encode_reply_code",
                     encode_reply_code, 1);
    rb_define_method(rb_cMilterReplyEncoder, "encode_temporary_failure",
                     encode_temporary_failure, 0);
    rb_define_method(rb_cMilterReplyEncoder, "encode_reject",
                     encode_reject, 0);
    rb_define_method(rb_cMilterReplyEncoder, "encode_accept",
                     encode_accept, 0);
    rb_define_method(rb_cMilterReplyEncoder, "encode_discard",
                     encode_discard, 0);
    rb_define_method(rb_cMilterReplyEncoder, "encode_add_header",
                     encode_add_header, 2);
    rb_define_method(rb_cMilterReplyEncoder, "encode_insert_header",
                     encode_insert_header, 3);
    rb_define_method(rb_cMilterReplyEncoder, "encode_change_header",
                     encode_change_header, 3);
    rb_define_method(rb_cMilterReplyEncoder, "encode_delete_header",
                     encode_delete_header, 2);
    rb_define_method(rb_cMilterReplyEncoder, "encode_change_from",
                     encode_change_from, -1);
    rb_define_method(rb_cMilterReplyEncoder, "encode_add_recipient",
                     encode_add_recipient, -1);
    rb_define_method(rb_cMilterReplyEncoder, "encode_delete_recipient",
                     encode_delete_recipient, 1);
    rb_define_method(rb_cMilterReplyEncoder, "encode_replace_body",
                     encode_replace_body, 1);
    rb_define_method(rb_cMilterReplyEncoder, "encode_progress",
                     encode_progress, 0);
    rb_define_method(rb_cMilterReplyEncoder, "encode_quarantine",
                     encode_quarantine, 1);
    rb_define_method(rb_cMilterReplyEncoder, "encode_connection_failure",
                     encode_connection_failure, 0);
    rb_define_method(rb_cMilterReplyEncoder, "encode_shutdown",
                     encode_shutdown, 0);
    rb_define_method(rb_cMilterReplyEncoder, "encode_skip",
                     encode_skip, 0);

    G_DEF_SETTERS(rb_cMilterReplyEncoder);
}
