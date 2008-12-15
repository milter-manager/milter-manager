/* -*- c-file-style: "ruby" -*- */

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

VALUE rb_milter_compat_glist2array_string (GList *list);
VALUE rb_milter_compat_gobject2ruby_object_with_unref (gpointer instance);

const gchar *rb_milter_compat_inspect (VALUE object);
const gchar *rb_milter_compat_ruby_object2string_accept_nil (VALUE string);

#endif
