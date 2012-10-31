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

#include "rb-milter-core-private.h"

#define SELF(self) RVAL2DECODER(self)

static VALUE
decode (VALUE self, VALUE chunk)
{
    GError *error = NULL;

    if (!RVAL2CBOOL(rb_obj_is_kind_of(chunk, rb_cString)))
        rb_raise(rb_eArgError,
                 "chunk should be a String: <%s>", RBG_INSPECT(chunk));
    if (!milter_decoder_decode(SELF(self),
                               RSTRING_PTR(chunk), RSTRING_LEN(chunk),
                               &error))
        RAISE_GERROR(error);

    return self;
}

static VALUE
end_decode (VALUE self)
{
    GError *error = NULL;

    if (!milter_decoder_end_decode(SELF(self), &error))
        RAISE_GERROR(error);

    return self;
}

static VALUE
replace_body_signal_convert (guint num, const GValue *values)
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
define_macro_signal_convert (guint num, const GValue *values)
{
    return rb_ary_new3(3,
		       GVAL2RVAL(&values[0]),
		       GVAL2RVAL(&values[1]),
		       MACROS2RVAL(g_value_get_pointer(&values[2])));
}

void
Init_milter_decoder (void)
{
    VALUE rb_cMilterDecoder, rb_cMilterReplyDecoder, rb_cMilterCommandDecoder;

    rb_cMilterDecoder = G_DEF_CLASS(MILTER_TYPE_DECODER, "Decoder", rb_mMilter);
    rb_cMilterReplyDecoder = G_DEF_CLASS(MILTER_TYPE_REPLY_DECODER,
                                         "ReplyDecoder", rb_mMilter);
    rb_cMilterCommandDecoder = G_DEF_CLASS(MILTER_TYPE_COMMAND_DECODER,
                                           "CommandDecoder", rb_mMilter);
    G_DEF_ERROR2(MILTER_TYPE_DECODER_ERROR, "DecoderError",
                 rb_mMilter, rb_eMilterError);
    G_DEF_ERROR2(MILTER_TYPE_COMMAND_DECODER_ERROR, "CommandDecoderError",
                 rb_mMilter, rb_eMilterError);
    G_DEF_ERROR2(MILTER_TYPE_REPLY_DECODER_ERROR, "ReplyDecoderError",
                 rb_mMilter, rb_eMilterError);

    rb_define_method(rb_cMilterDecoder, "decode", decode, 1);
    rb_define_method(rb_cMilterDecoder, "end_decode", end_decode, 0);

    G_DEF_SIGNAL_FUNC(rb_cMilterReplyDecoder, "replace-body",
                      replace_body_signal_convert);

    G_DEF_SIGNAL_FUNC(rb_cMilterCommandDecoder, "define-macro",
                      define_macro_signal_convert);
    G_DEF_SIGNAL_FUNC(rb_cMilterCommandDecoder, "connect",
		      rb_milter__connect_signal_convert);
    G_DEF_SIGNAL_FUNC(rb_cMilterCommandDecoder, "helo",
                      rb_milter__helo_signal_convert);
    G_DEF_SIGNAL_FUNC(rb_cMilterCommandDecoder, "envelope-from",
                      rb_milter__envelope_from_signal_convert);
    G_DEF_SIGNAL_FUNC(rb_cMilterCommandDecoder, "envelope-recipient",
                      rb_milter__envelope_recipient_signal_convert);
    G_DEF_SIGNAL_FUNC(rb_cMilterCommandDecoder, "unknown",
                      rb_milter__unknown_signal_convert);
    G_DEF_SIGNAL_FUNC(rb_cMilterCommandDecoder, "header",
                      rb_milter__header_signal_convert);
    G_DEF_SIGNAL_FUNC(rb_cMilterCommandDecoder, "body",
                      rb_milter__body_signal_convert);
    G_DEF_SIGNAL_FUNC(rb_cMilterCommandDecoder, "end-of-message",
                      rb_milter__end_of_message_signal_convert);

    G_DEF_SETTERS(rb_cMilterDecoder);
}
