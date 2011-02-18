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

#include "rb-milter-core-private.h"

VALUE
rb_milter_compat_glist2array_string (GList *list)
{
    VALUE array;

    array = rb_ary_new();
    while (list) {
        rb_ary_push(array, CSTR2RVAL(list->data));
        list = g_list_next(list);
    }
    return array;
}

VALUE
rb_milter_compat_gobject2ruby_object_with_unref (gpointer instance)
{
    VALUE ruby_object;

    ruby_object = GOBJ2RVAL(instance);
    g_object_unref(instance);
    return ruby_object;
}

const gchar *
rb_milter_compat_inspect (VALUE object)
{
    VALUE inspected;

    inspected = rb_funcall(object, rb_intern("inspect"), 0);
    return RVAL2CSTR(inspected);
}

const gchar *
rb_milter_compat_ruby_object2string_accept_nil (VALUE string)
{
    if (NIL_P(string))
        return NULL;

    return RVAL2CSTR(string);
}
