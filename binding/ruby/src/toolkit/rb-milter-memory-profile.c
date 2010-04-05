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

static VALUE
s_report (VALUE klass)
{
    milter_memory_profile_report();
    return Qnil;
}

void
Init_milter_memory_profile (void)
{
    VALUE rb_mMilterMemoryProfile;

    rb_mMilterMemoryProfile =
        rb_define_module_under(rb_mMilter, "MemoryProfile");

    rb_define_singleton_method(rb_mMilterMemoryProfile, "report",
			       s_report, 0);
}
