/* -*- c-file-style: "ruby" -*- */
/*
 *  Copyright (C) 2009  Kouhei Sutou <kou@clear-code.com>
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

#define SELF(self) MILTER_AGENT(RVAL2GOBJ(self))

static VALUE
agent_shutdown (VALUE self)
{
    milter_agent_shutdown(SELF(self));
    return self;
}

void
Init_milter_agent (void)
{
    VALUE rb_cMilterAgent;

    rb_cMilterAgent = G_DEF_CLASS(MILTER_TYPE_AGENT, "Agent", rb_mMilter);

    rb_define_method(rb_cMilterAgent, "shutdown", agent_shutdown, 0);

    G_DEF_SETTERS(rb_cMilterAgent);
}
