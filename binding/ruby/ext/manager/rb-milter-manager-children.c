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

#include <rb-milter-core-private.h>
#include "rb-milter-manager-private.h"

#define SELF(self) (MILTER_MANAGER_CHILDREN(RVAL2GOBJ(self)))

static VALUE
initialize (VALUE self, VALUE configuration, VALUE event_loop)
{
    MilterManagerChildren *children;

    children = milter_manager_children_new(RVAL2GOBJ(configuration),
					   RVAL2GOBJ(event_loop));
    G_INITIALIZE(self, children);

    return Qnil;
}

static VALUE
length (VALUE self)
{
    return UINT2NUM(milter_manager_children_length(SELF(self)));
}

static VALUE
add_child (VALUE self, VALUE child)
{
    milter_manager_children_add_child(SELF(self), RVAL2GOBJ(child));
    return self;
}

static VALUE
add_child_less_than (VALUE self, VALUE child)
{
    add_child(self, child);
    return self;
}

static void
cb_each (gpointer data, gpointer user_data)
{
    MilterManagerChild *child = data;
    rb_yield(GOBJ2RVAL(child));
}

static VALUE
each (VALUE self)
{
    milter_manager_children_foreach(SELF(self), cb_each, NULL);
    return self;
}

static VALUE
get_children (VALUE self)
{
    return GLIST2ARY(milter_manager_children_get_children(SELF(self)));
}

static VALUE
get_smtp_client_address (VALUE self)
{
    VALUE rb_address;
    struct sockaddr *address;
    socklen_t address_length;

    if (milter_manager_children_get_smtp_client_address(SELF(self),
							&address,
							&address_length)) {
	rb_address = ADDRESS2RVAL(address, address_length);
	g_free(address);
	return rb_address;
    } else {
	return Qnil;
    }
}

void
Init_milter_manager_children (void)
{
    VALUE rb_cMilterManagerChildren;

    rb_cMilterManagerChildren =
	G_DEF_CLASS(MILTER_TYPE_MANAGER_CHILDREN, "Children", rb_mMilterManager);

    rb_define_method(rb_cMilterManagerChildren, "initialize",
                     initialize, 2);
    rb_define_method(rb_cMilterManagerChildren, "length", length, 0);
    rb_define_method(rb_cMilterManagerChildren, "add", add_child, 1);
    rb_define_method(rb_cMilterManagerChildren, "<<", add_child_less_than, 1);
    rb_define_method(rb_cMilterManagerChildren, "each", each, 0);
    rb_define_method(rb_cMilterManagerChildren, "children", get_children, 0);
    rb_define_method(rb_cMilterManagerChildren, "smtp_client_address",
		     get_smtp_client_address, 0);
}
