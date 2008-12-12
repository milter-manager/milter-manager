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
find_egg (VALUE self, VALUE name)
{
    MilterManagerEgg *egg;

    egg = milter_manager_configuration_find_egg(SELF(self), RVAL2CSTR(name));
    return GOBJ2RVAL(egg);
}

static VALUE
get_eggs (VALUE self)
{
    const GList *eggs;

    eggs = milter_manager_configuration_get_eggs(SELF(self));
    return GLIST2ARY((GList *)eggs);
}

static VALUE
remove_egg (VALUE self, VALUE name_or_egg)
{
    if (RVAL2CBOOL(rb_obj_is_kind_of(name_or_egg, rb_cString))) {
	const gchar *name;

	name = RVAL2CSTR(name_or_egg);
	milter_manager_configuration_remove_egg_by_name(SELF(self), name);
    } else {
	MilterManagerEgg *egg;

	egg = RVAL2GOBJ(name_or_egg);
	milter_manager_configuration_remove_egg(SELF(self), egg);
    }

    return Qnil;
}

static VALUE
clear_eggs (VALUE self)
{
    milter_manager_configuration_clear_eggs(SELF(self));
    return Qnil;
}

static VALUE
add_applicable_condition (VALUE self, VALUE condition)
{
    milter_manager_configuration_add_applicable_condition(SELF(self),
							  RVAL2GOBJ(condition));
    return Qnil;
}

static VALUE
find_applicable_condition (VALUE self, VALUE name)
{
    MilterManagerApplicableCondition *condition;

    condition =
	milter_manager_configuration_find_applicable_condition(SELF(self),
							       RVAL2CSTR(name));
    return GOBJ2RVAL(condition);
}

static VALUE
get_applicable_conditions (VALUE self)
{
    const GList *conditions;

    conditions = milter_manager_configuration_get_applicable_conditions(SELF(self));
    return GLIST2ARY((GList *)conditions);
}

static VALUE
remove_applicable_condition (VALUE self, VALUE name_or_condition)
{
    if (RVAL2CBOOL(rb_obj_is_kind_of(name_or_condition, rb_cString))) {
	const gchar *name;

	name = RVAL2CSTR(name_or_condition);
	milter_manager_configuration_remove_applicable_condition_by_name(SELF(self),
									 name);
    } else {
	MilterManagerApplicableCondition *condition;

	condition = RVAL2GOBJ(name_or_condition);
	milter_manager_configuration_remove_applicable_condition(SELF(self),
								 condition);
    }

    return Qnil;
}

static VALUE
clear_applicable_conditions (VALUE self)
{
    milter_manager_configuration_clear_applicable_conditions(SELF(self));
    return Qnil;
}

static VALUE
append_load_path (VALUE self, VALUE path)
{
    milter_manager_configuration_append_load_path(SELF(self), RVAL2CSTR(path));
    return Qnil;
}

static VALUE
prepend_load_path (VALUE self, VALUE path)
{
    milter_manager_configuration_prepend_load_path(SELF(self), RVAL2CSTR(path));
    return Qnil;
}

static VALUE
clear_load_paths (VALUE self)
{
    milter_manager_configuration_clear_load_paths(SELF(self));
    return Qnil;
}

static VALUE
get_load_paths (VALUE self)
{
    const GList *load_paths;
    load_paths = milter_manager_configuration_get_load_paths(SELF(self));
    return GLIST2ARY_STR((GList *)load_paths);
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
    rb_define_method(rb_cMilterManagerConfiguration, "find_egg", find_egg, 1);
    rb_define_method(rb_cMilterManagerConfiguration, "eggs", get_eggs, 0);
    rb_define_method(rb_cMilterManagerConfiguration,
		     "remove_egg", remove_egg, 1);
    rb_define_method(rb_cMilterManagerConfiguration,
		     "clear_eggs", clear_eggs, 0);

    rb_define_method(rb_cMilterManagerConfiguration,
		     "add_applicable_condition", add_applicable_condition, 1);
    rb_define_method(rb_cMilterManagerConfiguration,
		     "find_applicable_condition", find_applicable_condition, 1);
    rb_define_method(rb_cMilterManagerConfiguration,
		     "applicable_conditions", get_applicable_conditions, 0);
    rb_define_method(rb_cMilterManagerConfiguration,
		     "remove_applicable_condition",
		     remove_applicable_condition, 1);
    rb_define_method(rb_cMilterManagerConfiguration,
		     "clear_applicable_conditions",
		     clear_applicable_conditions, 0);

    rb_define_method(rb_cMilterManagerConfiguration,
		     "prepend_load_path", prepend_load_path, 1);
    rb_define_method(rb_cMilterManagerConfiguration,
		     "append_load_path", append_load_path, 1);
    rb_define_method(rb_cMilterManagerConfiguration,
		     "clear_load_paths", clear_load_paths, 0);
    rb_define_method(rb_cMilterManagerConfiguration,
		     "load_paths", get_load_paths, 0);
    rb_define_method(rb_cMilterManagerConfiguration,
		     "create_children", create_children, 0);
    rb_define_method(rb_cMilterManagerConfiguration,
		     "to_xml", to_xml, -1);
}
