/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Copyright (C) 2015  Kenji Okimoto <okimoto@clear-code.com>
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

#define SELF(self) MILTER_HEADERS(RVAL2GOBJ(self))
#define MILTER_HEADER(obj, sval)                        \
    Data_Get_Struct((obj), MilterHeader, (sval))
#define RB_MILTER_HEADER(sval)                                          \
    Data_Wrap_Struct(rb_cMilterHeader, NULL,                            \
                     milter_header_free, (sval))

static VALUE rb_cMilterHeader;
static VALUE rb_cMilterHeaders;

static VALUE
rb_milter_header_alloc (VALUE klass)
{
    return Data_Wrap_Struct(klass, NULL, milter_header_free, NULL);
}

static VALUE
rb_milter_header_initialize (VALUE self, VALUE name, VALUE val)
{
    DATA_PTR(self) = milter_header_new(StringValueCStr(name),
                                       StringValueCStr(val));
    return Qnil;
}

static VALUE
rb_milter_header_name (VALUE obj)
{
    MilterHeader *header;
    MILTER_HEADER(obj, header);
    return CSTR2RVAL(header->name);
}

static VALUE
rb_milter_header_value (VALUE obj)
{
    MilterHeader *header;
    MILTER_HEADER(obj, header);
    return CSTR2RVAL(header->value);
}

static VALUE
rb_milter_header_compare (VALUE lhs, VALUE rhs)
{
    gint result;
    MilterHeader *header_lhs;
    MilterHeader *header_rhs;
    MILTER_HEADER(lhs, header_lhs);
    MILTER_HEADER(rhs, header_rhs);

    result = milter_header_compare(header_lhs, header_rhs);
    return INT2NUM(result);
}

static VALUE
rb_milter_header_equal (VALUE lhs, VALUE rhs)
{
    gboolean result;
    MilterHeader *header_lhs;
    MilterHeader *header_rhs;
    MILTER_HEADER(lhs, header_lhs);
    MILTER_HEADER(rhs, header_rhs);

    result = milter_header_equal(header_lhs, header_rhs);
    return CBOOL2RVAL(result);
}

static VALUE
rb_milter_header_inspect (VALUE self)
{
    gchar *str;
    VALUE rb_str;
    MilterHeader *header;
    MILTER_HEADER(self, header);
    str = g_strdup_printf("<<%s>,<%s>>", header->name, header->value);
    rb_str = CSTR2RVAL(str);
    g_free(str);
    return rb_str;
}

static VALUE
rb_milter_headers_dup (VALUE self)
{
    MilterHeaders *headers;
    headers = milter_headers_copy(SELF(self));

    return GOBJ2RVAL(headers);
}

static VALUE
rb_milter_headers_add (VALUE self, VALUE name, VALUE value)
{
    gboolean succeeded;
    succeeded = milter_headers_add_header(SELF(self),
                                          StringValueCStr(name),
                                          StringValueCStr(value));
    return CBOOL2RVAL(succeeded);
}

static VALUE
rb_milter_headers_append (VALUE self, VALUE name, VALUE value)
{
    gboolean succeeded;
    succeeded = milter_headers_append_header(SELF(self),
                                             StringValueCStr(name),
                                             StringValueCStr(value));
    return CBOOL2RVAL(succeeded);
}

static VALUE
rb_milter_headers_insert (VALUE self, VALUE position, VALUE name, VALUE value)
{
    gboolean succeeded;
    succeeded = milter_headers_insert_header(SELF(self),
                                             FIX2INT(position),
                                             StringValueCStr(name),
                                             StringValueCStr(value));
    return CBOOL2RVAL(succeeded);
}

static VALUE
rb_milter_headers_find (VALUE self, VALUE name, VALUE value)
{
    MilterHeader *target_header;
    MilterHeader *found_header;
    MilterHeader *header;
    VALUE rb_header;

    target_header = milter_header_new(StringValueCStr(name),
                                      StringValueCStr(value));
    found_header = milter_headers_find(SELF(self), target_header);
    milter_header_free(target_header);

    if (!found_header)
        return Qnil;

    header = milter_header_new(found_header->name, found_header->value);
    rb_header = RB_MILTER_HEADER(header);

    return rb_header;
}

static VALUE
rb_milter_headers_lookup_by_name (VALUE self, VALUE name)
{
    MilterHeader *found_header;
    MilterHeader *header;
    VALUE rb_header;
    found_header = milter_headers_lookup_by_name(SELF(self),
                                                 StringValueCStr(name));
    if (!found_header)
        return Qnil;

    header = milter_header_new(found_header->name, found_header->value);
    rb_header = RB_MILTER_HEADER(header);

    return rb_header;
}

static VALUE
rb_milter_headers_get_nth (VALUE self, VALUE index)
{
    MilterHeader *found_header;
    MilterHeader *header;
    VALUE rb_header;
    found_header = milter_headers_get_nth_header(SELF(self), FIX2INT(index));

    if (!found_header)
        return Qnil;

    header = milter_header_new(found_header->name, found_header->value);
    rb_header = RB_MILTER_HEADER(header);

    return rb_header;
}

static VALUE
rb_milter_headers_remove (VALUE self, VALUE name, VALUE value)
{
    gboolean succeeded;
    MilterHeader *target_header;
    target_header = milter_header_new(StringValueCStr(name),
                                      StringValueCStr(value));
    succeeded = milter_headers_remove(SELF(self), target_header);
    return CBOOL2RVAL(succeeded);
}

static VALUE
rb_milter_headers_change_header (VALUE self,
                                 VALUE name, VALUE index, VALUE value)
{
    gboolean succeeded;
    succeeded = milter_headers_change_header(SELF(self),
                                             StringValueCStr(name),
                                             FIX2INT(index),
                                             StringValueCStr(value));
    return CBOOL2RVAL(succeeded);
}

static VALUE
rb_milter_headers_delete_header (VALUE self, VALUE name, VALUE index)
{
    gboolean succeeded;
    succeeded = milter_headers_delete_header(SELF(self),
                                             StringValueCStr(name),
                                             FIX2INT(index));
    return CBOOL2RVAL(succeeded);
}

static VALUE
rb_milter_headers_length (VALUE self)
{
    guint length = milter_headers_length(SELF(self));
    return UINT2NUM(length);
}

static VALUE
rb_milter_headers_array (VALUE self)
{
    VALUE rb_array;
    VALUE rb_milter_header;
    guint i;

    rb_array = rb_ary_new();
    for (i = 1; i <= milter_headers_length(SELF(self)); i++) {
        rb_milter_header = rb_milter_headers_get_nth(self, INT2NUM(i));
        rb_ary_push(rb_array, rb_milter_header);
    }

    return rb_array;
}

static VALUE
rb_milter_headers_each (VALUE self)
{
    VALUE rb_milter_header;
    guint i;
    RETURN_ENUMERATOR(self, 0, 0);
    for (i = 1; i <= milter_headers_length(SELF(self)); i++) {
        rb_milter_header = rb_milter_headers_get_nth(self, INT2NUM(i));
        rb_yield(rb_milter_header);
    }
    return self;
}

void
Init_milter_headers (void)
{
    rb_cMilterHeader = rb_define_class_under(rb_mMilter, "Header", rb_cObject);
    rb_define_alloc_func(rb_cMilterHeader, rb_milter_header_alloc);

    rb_define_method(rb_cMilterHeader,
                     "initialize", rb_milter_header_initialize, 2);
    rb_define_method(rb_cMilterHeader, "name", rb_milter_header_name, 0);
    rb_define_method(rb_cMilterHeader, "value", rb_milter_header_value, 0);
    rb_define_method(rb_cMilterHeader, "<=>", rb_milter_header_compare, 1);
    rb_define_method(rb_cMilterHeader, "==", rb_milter_header_equal, 1);
    rb_define_method(rb_cMilterHeader, "inspect", rb_milter_header_inspect, 0);

    rb_cMilterHeaders = G_DEF_CLASS(MILTER_TYPE_HEADERS, "Headers", rb_mMilter);
    rb_include_module(rb_cMilterHeaders, rb_mEnumerable);

    rb_define_method(rb_cMilterHeaders, "dup", rb_milter_headers_dup, 0);
    rb_define_method(rb_cMilterHeaders,
                     "add", rb_milter_headers_add, 2);
    rb_define_method(rb_cMilterHeaders,
                     "append", rb_milter_headers_append, 2);
    rb_define_method(rb_cMilterHeaders,
                     "insert", rb_milter_headers_insert, 3);
    rb_define_method(rb_cMilterHeaders,
                     "find", rb_milter_headers_find, 2);
    rb_define_method(rb_cMilterHeaders,
                     "find_by_name", rb_milter_headers_lookup_by_name, 1);
    rb_define_method(rb_cMilterHeaders,
                     "get_nth", rb_milter_headers_get_nth, 1);
    rb_define_method(rb_cMilterHeaders,
                     "remove", rb_milter_headers_remove, 2);
    rb_define_method(rb_cMilterHeaders,
                     "change", rb_milter_headers_change_header, 3);
    rb_define_method(rb_cMilterHeaders,
                     "delete", rb_milter_headers_delete_header, 2);
    rb_define_method(rb_cMilterHeaders,
                     "length", rb_milter_headers_length, 0);
    rb_define_method(rb_cMilterHeaders,
                     "size", rb_milter_headers_length, 0);
    rb_define_method(rb_cMilterHeaders,
                     "to_a", rb_milter_headers_array, 0);
    rb_define_method(rb_cMilterHeaders,
                     "each", rb_milter_headers_each, 0);

    G_DEF_SETTERS(rb_cMilterHeaders);
}
