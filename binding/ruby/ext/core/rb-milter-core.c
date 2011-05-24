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

VALUE rb_mMilter, rb_eMilterError;

VALUE
rb_milter_cstr2rval_size_free (gchar *string, gsize size)
{
    VALUE rb_string;

    rb_string = rb_str_new(string, size);
    g_free(string);

    return rb_string;
}

void
Init_milter_core (void)
{
    VALUE version;

    rb_mMilter = rb_define_module("Milter");
    rb_eMilterError = rb_define_class_under(rb_mMilter, "Error",
					    rb_eStandardError);

    version = rb_ary_new3(3,
			  INT2NUM(MILTER_TOOLKIT_VERSION_MAJOR),
			  INT2NUM(MILTER_TOOLKIT_VERSION_MINOR),
			  INT2NUM(MILTER_TOOLKIT_VERSION_MICRO));
    rb_obj_freeze(version);
    rb_define_const(rb_mMilter, "VERSION", version);

    rb_define_const(rb_mMilter, "CHUNK_SIZE", UINT2NUM(MILTER_CHUNK_SIZE));

    milter_init();

    Init_milter_logger();
    Init_milter_memory_profile();
    Init_milter_socket_address();
    Init_milter_utils();
    Init_milter_connection();
    Init_milter_protocol();
    Init_milter_option();
    Init_milter_macros_requests();
    Init_milter_encoder();
    Init_milter_command_encoder();
    Init_milter_reply_encoder();
    Init_milter_decoder();
    Init_milter_error_emittable();
    Init_milter_agent();
    Init_milter_protocol_agent();
    Init_milter_event_loop();
}
