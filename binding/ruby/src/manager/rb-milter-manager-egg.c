/* -*- c-file-style: "ruby" -*- */

#include "rb-milter-manager-private.h"

#define SELF(self) (MILTER_MANAGER_EGG(RVAL2GOBJ(self)))

static VALUE rb_cGString;

static void
rb_gstring_free (GString *string)
{
    g_string_free(string, TRUE);
}

static VALUE
rb_gstring_allocate (VALUE klass)
{
  return Data_Wrap_Struct(klass, NULL, rb_gstring_free, NULL);
}

static VALUE
rb_gstring_initialize (VALUE self)
{
    DATA_PTR(self) = g_string_new(NULL);
    return Qnil;
}

static VALUE
rb_gstring_write (VALUE self, VALUE content)
{
    GString *string;

    string = DATA_PTR(self);
    g_string_append_len(string, RSTRING_PTR(content), RSTRING_LEN(content));
    return LL2NUM(RSTRING_LEN(content));
}

static VALUE
rb_gstring_to_s (VALUE self)
{
    GString *string;

    string = DATA_PTR(self);
    return rb_str_new(string->str, string->len);
}

static VALUE
signal_to_xml (guint num, const GValue *values)
{
    return rb_ary_new3(3,
		       GVAL2RVAL(&values[0]),
		       Data_Wrap_Struct(rb_cGString,
					NULL,
					NULL,
					g_value_get_pointer(&values[1])),
		       UINT2NUM(g_value_get_uint(&values[2])));
}

static VALUE
initialize (VALUE self, VALUE name)
{
    G_INITIALIZE(self, milter_manager_egg_new(RVAL2CSTR(name)));
    return Qnil;
}

static VALUE
set_connection_spec (VALUE self, VALUE spec)
{
    GError *error = NULL;

    if (!milter_manager_egg_set_connection_spec(SELF(self),
						RVAL2CSTR(spec),
						&error))
	RAISE_GERROR(error);

    return Qnil;
}

static VALUE
to_xml (int argc, VALUE *argv, VALUE self)
{
    VALUE rb_indent;
    VALUE xml;
    GString *string;
    guint indent = 0;

    rb_scan_args(argc, argv, "01", &rb_indent);

    string = g_string_new(NULL);
    if (!NIL_P(rb_indent))
	indent = NUM2UINT(rb_indent);

    milter_manager_egg_to_xml_string(SELF(self), string, indent);
    xml = rb_str_new(string->str, string->len);
    g_string_free(string, TRUE);

    return xml;
}

void
Init_milter_manager_egg (void)
{
    VALUE rb_cMilterManagerEgg;

    rb_cGString = rb_define_class_under(rb_mMilterManager, "GString", rb_cIO);
    rb_define_alloc_func(rb_cGString, rb_gstring_allocate);
    rb_define_method(rb_cGString, "initialize", rb_gstring_initialize, 0);
    rb_define_method(rb_cGString, "write", rb_gstring_write, 1);
    rb_define_method(rb_cGString, "to_s", rb_gstring_to_s, 0);

    rb_cMilterManagerEgg =
	G_DEF_CLASS(MILTER_TYPE_MANAGER_EGG, "Egg", rb_mMilterManager);

    G_DEF_SIGNAL_FUNC(rb_cMilterManagerEgg,
                      "to-xml", signal_to_xml);

    rb_define_method(rb_cMilterManagerEgg, "initialize", initialize, 1);

    rb_define_method(rb_cMilterManagerEgg, "set_connection_spec",
		     set_connection_spec, 1);
    rb_define_method(rb_cMilterManagerEgg, "to_xml", to_xml, -1);

    G_DEF_SETTERS(rb_cMilterManagerEgg);
}
