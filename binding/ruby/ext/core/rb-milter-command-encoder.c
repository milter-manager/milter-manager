/* -*- c-file-style: "ruby" -*- */
/*
 *  Copyright (C) 2008-2013  Kouhei Sutou <kou@clear-code.com>
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

#define SELF(self) RVAL2COMMAND_ENCODER(self)

static ID id_pack;

static VALUE
initialize (VALUE self)
{
    G_INITIALIZE(self, milter_command_encoder_new());
    return Qnil;
}

static VALUE
encode_negotiate (VALUE self, VALUE option)
{
    const gchar *packet;
    gsize packet_size;

    milter_command_encoder_encode_negotiate(SELF(self),
					    &packet, &packet_size,
					    RVAL2OPTION(option));

    return CSTR2RVAL_SIZE(packet, packet_size);
}

static VALUE
encode_define_macro (VALUE self, VALUE rb_context, VALUE rb_macros)
{
    const gchar *packet;
    gsize packet_size;
    GHashTable *macros;

    macros = RVAL2MACROS(rb_macros);
    milter_command_encoder_encode_define_macro(SELF(self),
					       &packet, &packet_size,
					       RVAL2COMMAND(rb_context),
					       macros);
    g_hash_table_destroy(macros);

    return CSTR2RVAL_SIZE(packet, packet_size);
}

static VALUE
encode_connect (VALUE self, VALUE host_name, VALUE address)
{
    const gchar *packet;
    gsize packet_size;
    VALUE packed_address;
    const struct sockaddr *socket_address;

    if (RVAL2CBOOL(rb_obj_is_kind_of(address, rb_cString)))
	packed_address = address;
    else
	packed_address = rb_funcall(address, id_pack, 0);
    socket_address = (const struct sockaddr *)RSTRING_PTR(packed_address);
    milter_command_encoder_encode_connect(SELF(self), &packet, &packet_size,
					  RVAL2CSTR(host_name),
					  socket_address,
					  RSTRING_LEN(packed_address));

    return CSTR2RVAL_SIZE(packet, packet_size);
}

static VALUE
encode_helo (VALUE self, VALUE fqdn)
{
    const gchar *packet;
    gsize packet_size;

    milter_command_encoder_encode_helo(SELF(self),
				       &packet, &packet_size,
				       RVAL2CSTR(fqdn));

    return CSTR2RVAL_SIZE(packet, packet_size);
}

static VALUE
encode_envelope_from (VALUE self, VALUE to)
{
    const gchar *packet;
    gsize packet_size;

    milter_command_encoder_encode_envelope_from(SELF(self),
						&packet, &packet_size,
						RVAL2CSTR(to));

    return CSTR2RVAL_SIZE(packet, packet_size);
}

static VALUE
encode_envelope_recipient (VALUE self, VALUE from)
{
    const gchar *packet;
    gsize packet_size;

    milter_command_encoder_encode_envelope_recipient(SELF(self),
						     &packet, &packet_size,
						     RVAL2CSTR(from));

    return CSTR2RVAL_SIZE(packet, packet_size);
}

static VALUE
encode_data (VALUE self)
{
    const gchar *packet;
    gsize packet_size;

    milter_command_encoder_encode_data(SELF(self), &packet, &packet_size);

    return CSTR2RVAL_SIZE(packet, packet_size);
}

static VALUE
encode_header (VALUE self, VALUE name, VALUE value)
{
    const gchar *packet;
    gsize packet_size;

    milter_command_encoder_encode_header(SELF(self),
					 &packet, &packet_size,
					 RVAL2CSTR(name),
					 RVAL2CSTR(value));

    return CSTR2RVAL_SIZE(packet, packet_size);
}

static VALUE
encode_end_of_header (VALUE self)
{
    const gchar *packet;
    gsize packet_size;

    milter_command_encoder_encode_end_of_header(SELF(self),
						&packet, &packet_size);

    return CSTR2RVAL_SIZE(packet, packet_size);
}

static VALUE
encode_body (VALUE self, VALUE chunk)
{
    const gchar *packet;
    gsize packet_size, packed_size;

    milter_command_encoder_encode_body(SELF(self),
				       &packet, &packet_size,
				       RSTRING_PTR(chunk),
				       RSTRING_LEN(chunk),
				       &packed_size);

    return rb_ary_new3(2,
                       CSTR2RVAL_SIZE(packet, packet_size),
                       UINT2NUM(packed_size));
}

static VALUE
encode_end_of_message (int argc, VALUE *argv, VALUE self)
{
    VALUE rb_chunk;
    const gchar *packet, *chunk;
    gsize packet_size, chunk_size;

    rb_scan_args(argc, argv, "01", &rb_chunk);

    if (NIL_P(rb_chunk)) {
        chunk = NULL;
        chunk_size = 0;
    } else {
        chunk = RSTRING_PTR(rb_chunk);
        chunk_size = RSTRING_LEN(rb_chunk);
    }

    milter_command_encoder_encode_end_of_message(SELF(self),
						 &packet, &packet_size,
						 chunk, chunk_size);

    return CSTR2RVAL_SIZE(packet, packet_size);
}

static VALUE
encode_abort (VALUE self)
{
    const gchar *packet;
    gsize packet_size;

    milter_command_encoder_encode_abort(SELF(self), &packet, &packet_size);

    return CSTR2RVAL_SIZE(packet, packet_size);
}

static VALUE
encode_quit (VALUE self)
{
    const gchar *packet;
    gsize packet_size;

    milter_command_encoder_encode_quit(SELF(self), &packet, &packet_size);

    return CSTR2RVAL_SIZE(packet, packet_size);
}

static VALUE
encode_unknown (VALUE self, VALUE command)
{
    const gchar *packet;
    gsize packet_size;

    milter_command_encoder_encode_unknown(SELF(self),
					  &packet, &packet_size,
					  RVAL2CSTR(command));

    return CSTR2RVAL_SIZE(packet, packet_size);
}

void
Init_milter_command_encoder (void)
{
    VALUE rb_cMilterCommandEncoder;

    id_pack = rb_intern("pack");

    rb_cMilterCommandEncoder =
	G_DEF_CLASS(MILTER_TYPE_COMMAND_ENCODER, "CommandEncoder", rb_mMilter);

    rb_define_method(rb_cMilterCommandEncoder, "initialize",
                     initialize, 0);

    rb_define_method(rb_cMilterCommandEncoder, "encode_negotiate",
                     encode_negotiate, 1);
    rb_define_method(rb_cMilterCommandEncoder, "encode_define_macro",
                     encode_define_macro, 2);
    rb_define_method(rb_cMilterCommandEncoder, "encode_connect",
                     encode_connect, 2);
    rb_define_method(rb_cMilterCommandEncoder, "encode_helo",
                     encode_helo, 1);
    rb_define_method(rb_cMilterCommandEncoder, "encode_envelope_from",
                     encode_envelope_from, 1);
    rb_define_method(rb_cMilterCommandEncoder, "encode_envelope_recipient",
                     encode_envelope_recipient, 1);
    rb_define_method(rb_cMilterCommandEncoder, "encode_data",
                     encode_data, 0);
    rb_define_method(rb_cMilterCommandEncoder, "encode_header",
                     encode_header, 2);
    rb_define_method(rb_cMilterCommandEncoder, "encode_end_of_header",
                     encode_end_of_header, 0);
    rb_define_method(rb_cMilterCommandEncoder, "encode_body",
                     encode_body, 1);
    rb_define_method(rb_cMilterCommandEncoder, "encode_end_of_message",
                     encode_end_of_message, -1);
    rb_define_method(rb_cMilterCommandEncoder, "encode_abort",
                     encode_abort, 0);
    rb_define_method(rb_cMilterCommandEncoder, "encode_quit",
                     encode_quit, 0);
    rb_define_method(rb_cMilterCommandEncoder, "encode_unknown",
                     encode_unknown, 1);

    G_DEF_SETTERS(rb_cMilterCommandEncoder);
}
