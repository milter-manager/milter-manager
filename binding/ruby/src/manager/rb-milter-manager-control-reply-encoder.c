/* -*- c-file-style: "ruby" -*- */

#include "rb-milter-manager-private.h"

#define SELF(self) (MILTER_MANAGER_CONTROL_REPLY_ENCODER(RVAL2GOBJ(self)))


static VALUE
initialize (VALUE self)
{
    G_INITIALIZE(self, milter_manager_control_reply_encoder_new());
    return Qnil;
}

static VALUE
encode_success (VALUE self)
{
    gchar *packet;
    gsize packet_size;

    milter_manager_control_reply_encoder_encode_success(SELF(self),
                                                        &packet, &packet_size);
    return CSTR2RVAL_SIZE_FREE(packet, packet_size);
}

static VALUE
encode_failure (VALUE self, VALUE message)
{
    gchar *packet;
    gsize packet_size;

    milter_manager_control_reply_encoder_encode_failure(SELF(self),
                                                        &packet, &packet_size,
                                                        RVAL2CSTR(message));
    return CSTR2RVAL_SIZE_FREE(packet, packet_size);
}

static VALUE
encode_error (VALUE self, VALUE message)
{
    gchar *packet;
    gsize packet_size;

    milter_manager_control_reply_encoder_encode_error(SELF(self),
                                                      &packet, &packet_size,
                                                      RVAL2CSTR(message));
    return CSTR2RVAL_SIZE_FREE(packet, packet_size);
}

static VALUE
encode_configuration (VALUE self, VALUE configuration)
{
    gchar *packet;
    gsize packet_size;

    milter_manager_control_reply_encoder_encode_configuration(
        SELF(self),
        &packet, &packet_size,
        RSTRING_PTR(configuration), RSTRING_LEN(configuration));
    return CSTR2RVAL_SIZE_FREE(packet, packet_size);
}

void
Init_milter_manager_control_reply_encoder (void)
{
    VALUE rb_cMilterManagerControlReplyEncoder;

    rb_cMilterManagerControlReplyEncoder =
	G_DEF_CLASS(MILTER_TYPE_MANAGER_CONTROL_REPLY_ENCODER,
                    "ControlReplyEncoder", rb_mMilterManager);

    rb_define_method(rb_cMilterManagerControlReplyEncoder,
                     "initialize", initialize, 0);

    rb_define_method(rb_cMilterManagerControlReplyEncoder,
                     "encode_success", encode_success, 0);
    rb_define_method(rb_cMilterManagerControlReplyEncoder,
                     "encode_failure", encode_failure, 1);
    rb_define_method(rb_cMilterManagerControlReplyEncoder,
                     "encode_error", encode_error, 1);
    rb_define_method(rb_cMilterManagerControlReplyEncoder,
                     "encode_configuration", encode_configuration, 1);

    G_DEF_SETTERS(rb_cMilterManagerControlReplyEncoder);
}
