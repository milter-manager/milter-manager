/* -*- c-file-style: "ruby" -*- */
/*
 *  Copyright (C) 2008-2011  Kouhei Sutou <kou@clear-code.com>
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

static VALUE
compare (VALUE self, VALUE other)
{
    return INT2NUM(milter_status_compare(RVAL2STATUS(self),
					 RVAL2STATUS(other)));
}

void
Init_milter_protocol (void)
{
    VALUE rb_cMilterStatus;

    rb_cMilterStatus = G_DEF_CLASS(MILTER_TYPE_STATUS, "Status", rb_mMilter);
    G_DEF_CONSTANTS(rb_mMilter, MILTER_TYPE_STATUS, "MILTER_");

    rb_include_module(rb_cMilterStatus, rb_mComparable);

    rb_define_method(rb_cMilterStatus, "<=>", compare, 1);

    G_DEF_CLASS(MILTER_TYPE_COMMAND, "Command", rb_mMilter);
    G_DEF_CONSTANTS(rb_mMilter, MILTER_TYPE_COMMAND, "MILTER_");
}
