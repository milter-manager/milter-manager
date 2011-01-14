/* -*- c-file-style: "ruby" -*- */
/*
 *  Copyright (C) 2010  Kouhei Sutou <kou@clear-code.com>
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

#define SELF(self) (MILTER_ERROR_EMITTABLE(RVAL2GOBJ(self)))

static VALUE
error_convert (guint num, const GValue *values)
{
    GError *error, *copied_error;
    error = g_value_get_pointer(&values[1]);
    copied_error = g_error_copy(error);
    return rb_ary_new3(2, GVAL2RVAL(&values[0]), GERROR2RVAL(copied_error));
}

void
Init_milter_error_emittable (void)
{
    VALUE rb_mMilterErrorEmittable;

    rb_mMilterErrorEmittable = G_DEF_INTERFACE(MILTER_TYPE_ERROR_EMITTABLE,
                                               "ErrorEmittable", rb_mMilter);

    G_DEF_SIGNAL_FUNC(rb_mMilterErrorEmittable, "error",
                      error_convert);
}
