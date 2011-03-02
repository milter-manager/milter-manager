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

#define SELF(self) (DATA_PTR(self))

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
    SELF(self) = g_string_new(NULL);
    return Qnil;
}

static VALUE
rb_gstring_write (VALUE self, VALUE content)
{
    GString *string;

    string = SELF(self);
    StringValue(content);
    g_string_append_len(string, RSTRING_PTR(content), RSTRING_LEN(content));
    return LL2NUM(RSTRING_LEN(content));
}

static VALUE
rb_gstring_to_s (VALUE self)
{
    GString *string;

    string = SELF(self);
    return rb_str_new(string->str, string->len);
}

VALUE
rb_milter_manager_gstring_handle_to_xml_signal (guint num, const GValue *values)
{
    return rb_ary_new3(3,
		       GVAL2RVAL(&values[0]),
		       Data_Wrap_Struct(rb_cGString,
					NULL,
					NULL,
					g_value_get_pointer(&values[1])),
		       UINT2NUM(g_value_get_uint(&values[2])));
}

void
Init_milter_manager_gstring (void)
{
    rb_cGString = rb_define_class_under(rb_mMilterManager, "GString", rb_cIO);
    rb_define_alloc_func(rb_cGString, rb_gstring_allocate);
    rb_define_method(rb_cGString, "initialize", rb_gstring_initialize, 0);
    rb_define_method(rb_cGString, "write", rb_gstring_write, 1);
    rb_define_method(rb_cGString, "to_s", rb_gstring_to_s, 0);
}
