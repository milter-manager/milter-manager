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

#define SELF(self) (MILTER_MANAGER_CHILDREN(RVAL2GOBJ(self)))

static VALUE
initialize (VALUE self, VALUE configuration)
{
    G_INITIALIZE(self, milter_manager_children_new(RVAL2GOBJ(configuration)));
    return Qnil;
}

static VALUE
length (VALUE self)
{
    return UINT2NUM(milter_manager_children_length(SELF(self)));
}

void
Init_milter_manager_children (void)
{
    VALUE rb_cMilterManagerChildren;

    rb_cMilterManagerChildren =
	G_DEF_CLASS(MILTER_TYPE_MANAGER_CHILDREN, "Children", rb_mMilterManager);

    rb_define_method(rb_cMilterManagerChildren, "initialize",
                     initialize, 1);
    rb_define_method(rb_cMilterManagerChildren, "length", length, 0);
}
