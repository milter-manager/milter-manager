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

#include "rb-milter-client-private.h"

#define SELF(self) RVAL2GOBJ(self)

VALUE rb_cMilterClient;

static GPid
client_custom_fork (MilterClient *client)
{
#ifdef HAVE_RB_FORK
    int status;
    return (GPid)rb_fork(&status, NULL, NULL, Qnil);
#else
    VALUE pid = rb_funcall2(rb_mKernel, rb_intern("fork"), 0, 0);
    return (GPid)NUM2INT(pid);
#endif
}

static VALUE
client_initialize (VALUE self)
{
    milter_client_set_custom_fork_func(SELF(self), client_custom_fork);
    return Qnil;
}

static VALUE
client_run (VALUE self)
{
    GError *error = NULL;

    if (!milter_client_run(SELF(self), &error))
	RAISE_GERROR(error);

    return Qnil;
}

static VALUE
client_main (VALUE self)
{
    milter_warning("[ruby][client][deprecatd] Milter::Client#run is deprecated. "
		   "Use Milter::Client#run instead.");
    milter_client_main(SELF(self));
    return Qnil;
}

static VALUE
client_run_master (VALUE self)
{
    GError *error = NULL;

    if (!milter_client_run_master(SELF(self), &error))
	RAISE_GERROR(error);

    return Qnil;
}

static VALUE
client_run_worker (VALUE self)
{
    GError *error = NULL;

    if (!milter_client_run_worker(SELF(self), &error))
	RAISE_GERROR(error);

    return Qnil;
}

static VALUE
client_shutdown (VALUE self)
{
    milter_client_shutdown(SELF(self));
    return Qnil;
}

static VALUE
client_start_syslog (VALUE self, VALUE identify)
{
    milter_client_start_syslog(SELF(self), RVAL2CSTR(identify));
    return Qnil;
}

static VALUE
client_listen (VALUE self)
{
    GError *error = NULL;

    if (!milter_client_listen(SELF(self), &error))
	RAISE_GERROR(error);

    return Qnil;
}

static VALUE
client_get_connection_spec (VALUE self)
{
    return CSTR2RVAL(milter_client_get_connection_spec(SELF(self)));
}

static VALUE
client_set_connection_spec (VALUE self, VALUE spec)
{
    GError *error = NULL;

    if (!milter_client_set_connection_spec(SELF(self),
                                           StringValueCStr(spec), &error))
        RAISE_GERROR(error);

    return Qnil;
}

static VALUE
client_get_effective_user (VALUE self)
{
    return CSTR2RVAL(milter_client_get_effective_user(SELF(self)));
}

static VALUE
client_set_effective_user (VALUE self, VALUE user)
{
    milter_client_set_effective_user(SELF(self), RVAL2CSTR_ACCEPT_NIL(user));
    return Qnil;
}

static VALUE
client_get_effective_group (VALUE self)
{
    return CSTR2RVAL(milter_client_get_effective_group(SELF(self)));
}

static VALUE
client_set_effective_group (VALUE self, VALUE group)
{
    milter_client_set_effective_group(SELF(self), RVAL2CSTR_ACCEPT_NIL(group));
    return Qnil;
}

static VALUE
client_get_unix_socket_group (VALUE self)
{
    return CSTR2RVAL(milter_client_get_unix_socket_group(SELF(self)));
}

static VALUE
client_set_unix_socket_group (VALUE self, VALUE group)
{
    milter_client_set_unix_socket_group(SELF(self), RVAL2CSTR_ACCEPT_NIL(group));
    return Qnil;
}

static VALUE
client_get_unix_socket_mode (VALUE self)
{
    return UINT2NUM(milter_client_get_unix_socket_mode(SELF(self)));
}

static VALUE
client_set_unix_socket_mode (VALUE self, VALUE rb_mode)
{
    guint mode;

    if (NIL_P(rb_mode)) {
	mode = 0;
    } else if (RVAL2CBOOL(rb_obj_is_kind_of(rb_mode, rb_cString))) {
	gchar *error_message = NULL;
	if (!milter_utils_parse_file_mode(RVAL2CSTR(rb_mode),
					  &mode, &error_message)) {
	    VALUE rb_error_message;
	    rb_error_message = CSTR2RVAL(error_message);
	    g_free(error_message);
	    rb_raise(rb_eArgError, "%s", RSTRING_PTR(rb_error_message));
	}
    } else {
	mode = NUM2UINT(rb_mode);
    }

    milter_client_set_unix_socket_mode(SELF(self), mode);
    return Qnil;
}

static VALUE
client_get_default_unix_socket_mode (VALUE self)
{
    return UINT2NUM(milter_client_get_default_unix_socket_mode(SELF(self)));
}

static VALUE
client_set_default_unix_socket_mode (VALUE self, VALUE rb_mode)
{
    guint mode;

    if (NIL_P(rb_mode)) {
	mode = 0;
    } else if (RVAL2CBOOL(rb_obj_is_kind_of(rb_mode, rb_cString))) {
	gchar *error_message = NULL;
	if (!milter_utils_parse_file_mode(RVAL2CSTR(rb_mode),
					  &mode, &error_message)) {
	    VALUE rb_error_message;
	    rb_error_message = CSTR2RVAL(error_message);
	    g_free(error_message);
	    rb_raise(rb_eArgError, "%s", RSTRING_PTR(rb_error_message));
	}
    } else {
	mode = NUM2UINT(rb_mode);
    }

    milter_client_set_default_unix_socket_mode(SELF(self), mode);
    return Qnil;
}

static void
mark (gpointer data)
{
    MilterClient *client = data;

    milter_client_processing_context_foreach(client,
					     (GFunc)rbgobj_gc_mark_instance,
					     NULL);
}

VALUE rb_cMilterClient;

void
Init_milter_client (void)
{
    milter_client_init();

    rb_cMilterClient = G_DEF_CLASS_WITH_GC_FUNC(MILTER_TYPE_CLIENT,
						"Client",
						rb_mMilter,
						mark,
						NULL);

    rb_define_const(rb_cMilterClient,
		    "DEFAULT_SUSPEND_TIME_ON_UNACCEPTABLE",
		    UINT2NUM(MILTER_CLIENT_DEFAULT_SUSPEND_TIME_ON_UNACCEPTABLE));
    rb_define_const(rb_cMilterClient,
		    "DEFAULT_MAX_CONNECTIONS",
		    UINT2NUM(MILTER_CLIENT_DEFAULT_MAX_CONNECTIONS));

    rb_define_method(rb_cMilterClient, "initialize", client_initialize, 0);
    rb_define_method(rb_cMilterClient, "run", client_run, 0);
    rb_define_method(rb_cMilterClient, "main", client_main, 0);
    rb_define_method(rb_cMilterClient, "run_master", client_run_master, 0);
    rb_define_method(rb_cMilterClient, "run_worker", client_run_worker, 0);
    rb_define_method(rb_cMilterClient, "shutdown", client_shutdown, 0);
    rb_define_method(rb_cMilterClient, "start_syslog", client_start_syslog, 1);
    rb_define_method(rb_cMilterClient, "listen", client_listen, 0);
    rb_define_method(rb_cMilterClient, "connection_spec",
                     client_get_connection_spec, 0);
    rb_define_method(rb_cMilterClient, "set_connection_spec",
                     client_set_connection_spec, 1);
    rb_define_method(rb_cMilterClient, "effective_user",
                     client_get_effective_user, 0);
    rb_define_method(rb_cMilterClient, "set_effective_user",
                     client_set_effective_user, 1);
    rb_define_method(rb_cMilterClient, "effective_group",
                     client_get_effective_group, 0);
    rb_define_method(rb_cMilterClient, "set_effective_group",
                     client_set_effective_group, 1);
    rb_define_method(rb_cMilterClient, "unix_socket_group",
                     client_get_unix_socket_group, 0);
    rb_define_method(rb_cMilterClient, "set_unix_socket_group",
                     client_set_unix_socket_group, 1);
    rb_define_method(rb_cMilterClient, "unix_socket_mode",
                     client_get_unix_socket_mode, 0);
    rb_define_method(rb_cMilterClient, "set_unix_socket_mode",
                     client_set_unix_socket_mode, 1);
    rb_define_method(rb_cMilterClient, "default_unix_socket_mode",
                     client_get_default_unix_socket_mode, 0);
    rb_define_method(rb_cMilterClient, "set_default_unix_socket_mode",
                     client_set_default_unix_socket_mode, 1);

    G_DEF_SETTERS(rb_cMilterClient);

    Init_milter_client_context();
}
