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

static VALUE
initialize (VALUE self, VALUE name)
{
    G_INITIALIZE(self, milter_manager_child_new(RVAL2CSTR(name)));
    return Qnil;
}

void
Init_milter_manager_child (void)
{
    VALUE rb_cMilterManagerChild;

    rb_cMilterManagerChild =
	G_DEF_CLASS(MILTER_TYPE_MANAGER_CHILD, "Child", rb_mMilterManager);

    rb_define_method(rb_cMilterManagerChild, "initialize",
                     initialize, 1);
}
