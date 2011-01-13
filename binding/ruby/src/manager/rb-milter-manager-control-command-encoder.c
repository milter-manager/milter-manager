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

#include "rb-milter-manager-private.h"

#define SELF(self) (MILTER_MANAGER_CONTROL_COMMAND_ENCODER(RVAL2GOBJ(self)))


static VALUE
initialize (VALUE self)
{
    G_INITIALIZE(self, milter_manager_control_command_encoder_new());
    return Qnil;
}

static VALUE
encode_set_configuration (VALUE self, VALUE configuration)
{
    const gchar *packet;
    gsize packet_size;

    milter_manager_control_command_encoder_encode_set_configuration(
        SELF(self),
        &packet, &packet_size,
        RSTRING_PTR(configuration), RSTRING_LEN(configuration));
    return CSTR2RVAL_SIZE(packet, packet_size);
}

void
Init_milter_manager_control_command_encoder (void)
{
    VALUE rb_cMilterManagerControlCommandEncoder;

    rb_cMilterManagerControlCommandEncoder =
	G_DEF_CLASS(MILTER_TYPE_MANAGER_CONTROL_COMMAND_ENCODER,
                    "ControlCommandEncoder", rb_mMilterManager);

    rb_define_method(rb_cMilterManagerControlCommandEncoder,
                     "initialize", initialize, 0);

    rb_define_method(rb_cMilterManagerControlCommandEncoder,
                     "encode_set_configuration", encode_set_configuration, 1);

    G_DEF_SETTERS(rb_cMilterManagerControlCommandEncoder);
}
