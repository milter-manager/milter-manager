/* -*- c-file-style: "ruby" -*- */

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

    return Qnil;
}

static VALUE
end_decode (VALUE self)
{
    GError *error = NULL;

    if (!milter_decoder_end_decode(SELF(self), &error))
        RAISE_GERROR(error);

    return Qnil;
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

static VALUE
body_signal_convert (guint num, const GValue *values)
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
end_of_message_signal_convert (guint num, const GValue *values)
{
    const gchar *chunk;
    gsize chunk_size;
    VALUE rb_chunk;

    chunk = g_value_get_string(&values[1]);
#if GLIB_SIZEOF_SIZE_T == 8
    chunk_size = g_value_get_uint64(&values[2]);
#else
    chunk_size = g_value_get_uint(&values[2]);
#endif

    if (chunk && chunk_size > 0)
	rb_chunk = rb_str_new(chunk, chunk_size);
    else
	rb_chunk = Qnil;

    return rb_ary_new3(2, GVAL2RVAL(&values[0]), rb_chunk);
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
    G_DEF_SIGNAL_FUNC(rb_cMilterCommandDecoder, "body",
		      body_signal_convert);
    G_DEF_SIGNAL_FUNC(rb_cMilterCommandDecoder, "end-of-message",
		      end_of_message_signal_convert);

    G_DEF_SETTERS(rb_cMilterDecoder);
}
