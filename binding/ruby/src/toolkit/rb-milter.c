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

#include "rb-milter-private.h"

VALUE
rb_milter_cstr2rval_size_free (gchar *string, gsize size)
{
    VALUE rb_string;

    rb_string = rb_str_new(string, size);
    g_free(string);

    return rb_string;
}

void
Init_milter_toolkit (void)
{
    rb_mMilter = rb_define_module("Milter");
    rb_eMilterError = rb_define_class_under(rb_mMilter, "Error",
					    rb_eStandardError);

    Init_milter_core();
    Init_milter_client();
    Init_milter_server();
}
