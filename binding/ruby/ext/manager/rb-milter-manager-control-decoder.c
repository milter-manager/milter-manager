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

#include "rb-milter-manager-private.h"

static VALUE
initialize_command_decoder (VALUE self)
{
    G_INITIALIZE(self, milter_manager_control_command_decoder_new());
    return Qnil;
}

static VALUE
initialize_reply_decoder (VALUE self)
{
    G_INITIALIZE(self, milter_manager_control_reply_decoder_new());
    return Qnil;
}

static VALUE
command_set_configuration (guint num, const GValue *values)
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

static VALUE
reply_configuration (guint num, const GValue *values)
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

void
Init_milter_manager_control_decoder (void)
{
    VALUE rb_cMilterManagerControlCommandDecoder;
    VALUE rb_cMilterManagerControlReplyDecoder;

    rb_cMilterManagerControlCommandDecoder =
        G_DEF_CLASS(MILTER_TYPE_MANAGER_CONTROL_COMMAND_DECODER,
                    "ControlCommandDecoder", rb_mMilterManager);
    rb_cMilterManagerControlReplyDecoder =
        G_DEF_CLASS(MILTER_TYPE_MANAGER_CONTROL_REPLY_DECODER,
                    "ControlReplyDecoder", rb_mMilterManager);

    G_DEF_ERROR2(MILTER_TYPE_MANAGER_CONTROL_COMMAND_DECODER_ERROR,
                 "ControlCommandDecoderError",
                 rb_mMilterManager, rb_eMilterError);
    G_DEF_ERROR2(MILTER_TYPE_MANAGER_CONTROL_REPLY_DECODER_ERROR,
                 "ControlReplyDecoderError",
                 rb_mMilterManager, rb_eMilterError);

    rb_define_method(rb_cMilterManagerControlCommandDecoder,
                     "initialize", initialize_command_decoder, 0);
    rb_define_method(rb_cMilterManagerControlReplyDecoder,
                     "initialize", initialize_reply_decoder, 0);

    G_DEF_SIGNAL_FUNC(rb_cMilterManagerControlCommandDecoder,
                      "set-configuration", command_set_configuration);
    G_DEF_SIGNAL_FUNC(rb_cMilterManagerControlReplyDecoder,
                      "configuration", reply_configuration);
}
