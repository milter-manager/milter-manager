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

#define SELF(self) (MILTER_MANAGER_CONFIGURATION(RVAL2GOBJ(self)))

static VALUE
initialize (VALUE self)
{
    MilterManagerConfiguration *configuration;
    GError *error = NULL;

    configuration = milter_manager_configuration_instantiate(NULL);
    G_INITIALIZE(self, configuration);
    if (!milter_manager_configuration_clear(configuration, &error))
	RAISE_GERROR(error);

    return Qnil;
}

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

    return self;
}

static VALUE
clear_eggs (VALUE self)
{
    milter_manager_configuration_clear_eggs(SELF(self));
    return self;
}

static VALUE
add_applicable_condition (VALUE self, VALUE condition)
{
    milter_manager_configuration_add_applicable_condition(SELF(self),
							  RVAL2GOBJ(condition));
    return self;
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

    return self;
}

static VALUE
clear_applicable_conditions (VALUE self)
{
    milter_manager_configuration_clear_applicable_conditions(SELF(self));
    return self;
}

static VALUE
append_load_path (VALUE self, VALUE path)
{
    milter_manager_configuration_append_load_path(SELF(self), RVAL2CSTR(path));
    return self;
}

static VALUE
prepend_load_path (VALUE self, VALUE path)
{
    milter_manager_configuration_prepend_load_path(SELF(self), RVAL2CSTR(path));
    return self;
}

static VALUE
clear_load_paths (VALUE self)
{
    milter_manager_configuration_clear_load_paths(SELF(self));
    return self;
}

static VALUE
get_load_paths (VALUE self)
{
    const GList *load_paths;
    load_paths = milter_manager_configuration_get_load_paths(SELF(self));
    return GLIST2ARY_STR((GList *)load_paths);
}

static VALUE
setup_children (VALUE self, VALUE rb_children, VALUE rb_client_context)
{
    MilterManagerChildren *children;
    MilterClientContext *client_context;

    children = MILTER_MANAGER_CHILDREN(RVAL2GOBJ(rb_children));
    client_context = MILTER_CLIENT_CONTEXT(RVAL2GOBJ(rb_client_context));
    milter_manager_configuration_setup_children(SELF(self),
						children,
						client_context);
    return self;
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

static VALUE
set_location (VALUE self, VALUE key, VALUE file, VALUE line)
{
    milter_manager_configuration_set_location(SELF(self),
					      RVAL2CSTR(key),
					      RVAL2CSTR_ACCEPT_NIL(file),
					      NUM2INT(line));

    return self;
}

static VALUE
reset_location (VALUE self, VALUE key)
{
    milter_manager_configuration_reset_location(SELF(self),
						RVAL2CSTR(key));

    return self;
}

static VALUE
location_to_ruby_object (gconstpointer location)
{
    VALUE rb_location;

    rb_location = rb_hash_new();
    rb_hash_aset(rb_location,
		 CSTR2RVAL("file"),
		 CSTR2RVAL(g_dataset_get_data(location, "file")));
    rb_hash_aset(rb_location,
		 CSTR2RVAL("line"),
		 INT2NUM(GPOINTER_TO_INT(g_dataset_get_data(location, "line"))));

    return rb_location;
}

static VALUE
get_location (VALUE self, VALUE key)
{
    gconstpointer location;

    location = milter_manager_configuration_get_location(SELF(self),
							 RVAL2CSTR(key));
    if (!location)
	return Qnil;

    return location_to_ruby_object(location);
}

static void
collect_location (gpointer key, gpointer value, gpointer user_data)
{
    gchar *key_ = key;
    VALUE *rb_locations = user_data;

    rb_hash_aset(*rb_locations, CSTR2RVAL(key_), location_to_ruby_object(value));
}

static VALUE
get_locations (VALUE self)
{
    GHashTable *locations;
    VALUE rb_locations;

    locations = milter_manager_configuration_get_locations(SELF(self));
    if (!locations)
	return Qnil;

    rb_locations = rb_hash_new();
    g_hash_table_foreach(locations, collect_location, &rb_locations);
    return rb_locations;
}

static VALUE
reload (VALUE self)
{
    GError *error = NULL;

    if (!milter_manager_configuration_reload(SELF(self), &error)) {
	RAISE_GERROR(error);
    }

    return self;
}

static void
mark (gpointer data)
{
    MilterManagerConfiguration *configuration = data;
    MilterManager *manager;
    const GList *node;

    g_list_foreach((GList *)milter_manager_configuration_get_eggs(configuration),
		   (GFunc)rbgobj_gc_mark_instance, NULL);
    g_list_foreach((GList *)milter_manager_configuration_get_applicable_conditions(configuration),
		   (GFunc)rbgobj_gc_mark_instance, NULL);

    manager = g_object_get_data(G_OBJECT(configuration), "manager");
    if (!manager)
	return;

    for (node = milter_manager_get_leaders(manager);
	 node;
	 node = g_list_next(node)) {
	MilterManagerLeader *leader = node->data;
	MilterManagerChildren *children;

	rbgobj_gc_mark_instance(leader);
	children = milter_manager_leader_get_children(leader);
	if (!children)
	    continue;
	g_list_foreach((GList *)milter_manager_children_get_children(children),
		       (GFunc)rbgobj_gc_mark_instance, NULL);
    }
}

void
Init_milter_manager_configuration (void)
{
    VALUE rb_cMilterManagerConfiguration;

    rb_cMilterManagerConfiguration =
	G_DEF_CLASS_WITH_GC_FUNC(MILTER_TYPE_MANAGER_CONFIGURATION,
				 "Configuration",
				 rb_mMilterManager,
				 mark,
				 NULL);

    G_DEF_SIGNAL_FUNC(rb_cMilterManagerConfiguration,
                      "to-xml", rb_milter_manager_gstring_handle_to_xml_signal);

    rb_define_method(rb_cMilterManagerConfiguration,
		     "initialize", initialize, 0);

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
		     "setup_children", setup_children, 2);
    rb_define_method(rb_cMilterManagerConfiguration,
		     "to_xml", to_xml, -1);
    rb_define_method(rb_cMilterManagerConfiguration,
		     "set_location", set_location, 3);
    rb_define_method(rb_cMilterManagerConfiguration,
		     "reset_location", reset_location, 1);
    rb_define_method(rb_cMilterManagerConfiguration,
		     "get_location", get_location, 1);
    rb_define_method(rb_cMilterManagerConfiguration,
		     "locations", get_locations, 0);

    rb_define_method(rb_cMilterManagerConfiguration,
		     "reload", reload, 0);
}
