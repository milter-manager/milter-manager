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

#ifndef __RB_MILTER_COMPAT_H__
#define __RB_MILTER_COMPAT_H__

#include <rbgobject.h>

#ifndef RSTRING_PTR
#  define RSTRING_PTR(s) (RSTRING(s)->ptr)
#  define RSTRING_LEN(s) (RSTRING(s)->len)
#endif

#ifndef RARRAY_PTR
#  define RARRAY_PTR(s) (RARRAY(s)->ptr)
#  define RARRAY_LEN(s) (RARRAY(s)->len)
#endif

#ifndef GLIST2ARY_STR
#  define GLIST2ARY_STR(list) (rb_milter_compat_glist2array_string(list))
#endif

#ifndef GOBJ2RVAL_UNREF
#  define GOBJ2RVAL_UNREF(object) (rb_milter_compat_gobject2ruby_object_with_unref(object))
#endif

#ifndef RVAL2CSTR_ACCEPT_NIL
#  define RVAL2CSTR_ACCEPT_NIL(string) (rb_milter_compat_ruby_object2string_accept_nil(string))
#endif

#ifndef RBG_INSPECT
#  define RBG_INSPECT(object) (rb_milter_compat_inspect(object))
#endif

#ifndef G_DEF_CLASS_WITH_GC_FUNC
#  define G_DEF_CLASS_WITH_GC_FUNC(gtype, name, module, mark, free) \
    G_DEF_CLASS2(gtype, name, module, mark, free)
#endif

VALUE rb_milter_compat_glist2array_string (GList *list);
VALUE rb_milter_compat_gobject2ruby_object_with_unref (gpointer instance);

const gchar *rb_milter_compat_inspect (VALUE object);
const gchar *rb_milter_compat_ruby_object2string_accept_nil (VALUE string);

#endif
