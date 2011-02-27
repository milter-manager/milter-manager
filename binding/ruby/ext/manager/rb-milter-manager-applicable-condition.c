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

#define SELF(self) (MILTER_MANAGER_APPLICABLE_CONDITION(RVAL2GOBJ(self)))

static VALUE
initialize (VALUE self, VALUE name)
{
    G_INITIALIZE(self, milter_manager_applicable_condition_new(RVAL2CSTR(name)));
    return Qnil;
}

static VALUE
merge (VALUE self, VALUE other)
{
    MilterManagerApplicableCondition *condition, *other_condition;

    condition = SELF(self);
    other_condition = SELF(other);
    milter_manager_applicable_condition_merge(condition, other_condition);
    return self;
}

void
Init_milter_manager_applicable_condition (void)
{
    VALUE rb_cMilterManagerApplicableCondition;

    rb_cMilterManagerApplicableCondition =
	G_DEF_CLASS(MILTER_TYPE_MANAGER_APPLICABLE_CONDITION,
                    "ApplicableCondition", rb_mMilterManager);

    rb_define_method(rb_cMilterManagerApplicableCondition,
                     "initialize", initialize, 1);
    rb_define_method(rb_cMilterManagerApplicableCondition,
                     "merge", merge, 1);

    G_DEF_SETTERS(rb_cMilterManagerApplicableCondition);
}
