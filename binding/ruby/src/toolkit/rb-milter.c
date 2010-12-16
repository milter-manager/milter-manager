/* -*- c-file-style: "ruby" -*- */
/*
 *  Copyright (C) 2008-2010  Kouhei Sutou <kou@clear-code.com>
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

#include "rb-milter-private.h"

VALUE
rb_milter_cstr2rval_size_free (gchar *string, gsize size)
{
    VALUE rb_string;

    rb_string = rb_str_new(string, size);
    g_free(string);

    return rb_string;
}

VALUE rb_mMilter, rb_eMilterError;

void
Init_milter_toolkit (void)
{
    VALUE cMilterToolkitVersion;

    rb_mMilter = rb_define_module("Milter");
    rb_eMilterError = rb_define_class_under(rb_mMilter, "Error",
					    rb_eStandardError);

    cMilterToolkitVersion = rb_ary_new3(3,
					INT2NUM(MILTER_TOOLKIT_VERSION_MAJOR),
					INT2NUM(MILTER_TOOLKIT_VERSION_MINOR),
					INT2NUM(MILTER_TOOLKIT_VERSION_MICRO));
    rb_obj_freeze(cMilterToolkitVersion);
    rb_define_const(rb_mMilter, "TOOLKIT_VERSION", cMilterToolkitVersion);

    Init_milter_core();
    Init_milter_client();
    Init_milter_server();
}
