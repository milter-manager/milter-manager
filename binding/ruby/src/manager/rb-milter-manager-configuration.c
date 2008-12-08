/* -*- c-file-style: "ruby" -*- */

#include "rb-milter-manager-private.h"

#define SELF(self) (MILTER_MANAGER_CONFIGURATION(RVAL2GOBJ(self)))

static VALUE
add_egg (VALUE self, VALUE egg)
{
    milter_manager_configuration_add_egg(SELF(self), RVAL2GOBJ(egg));

    return self;
}

static VALUE
create_children (VALUE self)
{
    MilterManagerChildren *children;

    children = milter_manager_configuration_create_children(SELF(self));
    return GOBJ2RVAL_UNREF(children);
}

static VALUE
to_xml (int argc, VALUE *argv, VALUE self)
{
    VALUE rb_indent, rb_xml;
    guint indent = 0;
    GString *xml;

    rb_scan_args(argc, argv, "01", &rb_indent);
    if (!NIL_P(rb_indent))
	indent = NUM2UINT(rb_indent);

    xml = g_string_new(NULL);
    milter_manager_configuration_to_xml_string(SELF(self), xml, indent);
    rb_xml = rb_str_new(xml->str, xml->len);
    g_string_free(xml, TRUE);

    return rb_xml;
}

void
Init_milter_manager_configuration (void)
{
    VALUE rb_cMilterManagerConfiguration;

    rb_cMilterManagerConfiguration =
	G_DEF_CLASS(MILTER_TYPE_MANAGER_CONFIGURATION,
		    "Configuration",
		    rb_mMilterManager);

    G_DEF_SIGNAL_FUNC(rb_cMilterManagerConfiguration,
                      "to-xml", rb_milter_manager_gstring_handle_to_xml_signal);

    rb_define_method(rb_cMilterManagerConfiguration, "add_egg", add_egg, 1);
    rb_define_method(rb_cMilterManagerConfiguration,
		     "create_children", create_children, 0);
    rb_define_method(rb_cMilterManagerConfiguration,
		     "to_xml", to_xml, -1);
}
