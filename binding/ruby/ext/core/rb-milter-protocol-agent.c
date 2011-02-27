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

#define SELF(self) (MILTER_PROTOCOL_AGENT(RVAL2GOBJ(self)))

static VALUE
set_macro (VALUE self, VALUE rb_macro_context, VALUE name, VALUE value)
{
    MilterProtocolAgent *agent;
    MilterCommand macro_context;

    agent = SELF(self);
    macro_context = RVAL2GENUM(rb_macro_context, MILTER_TYPE_COMMAND);
    milter_protocol_agent_set_macro(agent, macro_context,
                                    RVAL2CSTR(name),
				    RVAL2CSTR_ACCEPT_NIL(value));
    return self;
}

static void
cb_get_macros (gpointer key, gpointer value, gpointer user_data)
{
    gchar *macro_name = key;
    gchar *macro_value = value;
    VALUE *rb_macros = user_data;

    rb_hash_aset(*rb_macros, CSTR2RVAL(macro_name), CSTR2RVAL(macro_value));
}

static VALUE
get_available_macros (VALUE self)
{
    MilterProtocolAgent *agent;
    GHashTable *available_macros;
    VALUE rb_available_macros;

    agent = SELF(self);
    available_macros = milter_protocol_agent_get_available_macros(agent);
    rb_available_macros = rb_hash_new();

    g_hash_table_foreach(available_macros, cb_get_macros, &rb_available_macros);

    return rb_available_macros;
}

static VALUE
get_macros (VALUE self)
{
    MilterProtocolAgent *agent;
    GHashTable *macros;
    VALUE rb_macros = Qnil;

    agent = SELF(self);
    macros = milter_protocol_agent_get_macros(agent);
    if (macros) {
	rb_macros = rb_hash_new();
	g_hash_table_foreach(macros, cb_get_macros, &rb_macros);
    }

    return rb_macros;
}

void
Init_milter_protocol_agent (void)
{
    VALUE rb_cMilterProtocolAgent;

    rb_cMilterProtocolAgent = G_DEF_CLASS(MILTER_TYPE_PROTOCOL_AGENT,
                                          "ProtocolAgent", rb_mMilter);

    rb_define_method(rb_cMilterProtocolAgent, "set_macro", set_macro, 3);
    rb_define_method(rb_cMilterProtocolAgent, "available_macros",
                     get_available_macros, 0);
    rb_define_method(rb_cMilterProtocolAgent, "macros", get_macros, 0);
}
