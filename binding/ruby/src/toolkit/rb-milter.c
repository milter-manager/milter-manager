/* -*- c-file-style: "ruby" -*- */

#include "rb-milter-private.h"

VALUE
rb_milter_cstr2rval_size_free (gchar *string, gsize size)
{
    VALUE rb_string;

    rb_string = rb_str_new(string, size);
    g_free(string);

    return rb_string;
}

void
Init_milter_toolkit (void)
{
    rb_mMilter = rb_define_module("Milter");
    rb_eMilterError = rb_define_class_under(rb_mMilter, "Error",
					    rb_eStandardError);

    Init_milter_core();
    Init_milter_client();
    Init_milter_server();
}
