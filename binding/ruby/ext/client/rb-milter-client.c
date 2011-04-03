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

#ifdef HAVE_CONFIG_H
#  include "../../../../config.h"
#  undef PACKAGE_NAME
#  undef PACKAGE_TARNAME
#  undef PACKAGE_VERSION
#  undef PACKAGE_STRING
#  undef PACKAGE_BUGREPORT
#endif /* HAVE_CONFIG_H */

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
    VALUE pid;

    pid = rb_funcall2(rb_mKernel, rb_intern("fork"), 0, 0);
    if (NIL_P(pid)) {
	return (GPid)0;
    } else {
	return (GPid)NUM2INT(pid);
    }
#endif
}

static void
cb_event_loop_created (MilterClient *client,
		       MilterEventLoop *loop,
		       gpointer user_data)
{
    rb_milter_event_loop_setup(loop);
}

static VALUE
client_initialize (VALUE self)
{
    MilterClient *client;

    client = milter_client_new();
    G_INITIALIZE(self, client);
    milter_client_set_custom_fork_func(client, client_custom_fork);
    g_signal_connect(client, "event-loop-created",
		     G_CALLBACK(cb_event_loop_created), NULL);
    return Qnil;
}

static VALUE
client_run (VALUE self)
{
    GError *error = NULL;

    if (!milter_client_run(SELF(self), &error))
	RAISE_GERROR(error);

    return self;
}

static VALUE
client_main (VALUE self)
{
    milter_warning("[ruby][client][deprecatd] Milter::Client#main is deprecated. "
		   "Use Milter::Client#run instead.");
    milter_client_main(SELF(self));
    return self;
}

static VALUE
client_shutdown (VALUE self)
{
    milter_client_shutdown(SELF(self));
    return self;
}

static VALUE
client_start_syslog (int argc, VALUE *argv, VALUE self)
{
    MilterClient *client;
    VALUE identify, facility;

    rb_scan_args(argc, argv, "11", &identify, &facility);

    client = SELF(self);
    milter_client_set_syslog_identify(client, RVAL2CSTR(identify));
    if (!NIL_P(facility)) {
	milter_client_set_syslog_facility(client, RVAL2CSTR(facility));
    }
    milter_client_start_syslog(client);

    return self;
}

static VALUE
client_stop_syslog (VALUE self)
{
    milter_client_stop_syslog(SELF(self));
    return self;
}

static VALUE
client_syslog_enabled_p (VALUE self)
{
    return CBOOL2RVAL(milter_client_get_syslog_enabled(SELF(self)));
}

static VALUE
client_listen (VALUE self)
{
    GError *error = NULL;

    if (!milter_client_listen(SELF(self), &error))
	RAISE_GERROR(error);

    return self;
}

static VALUE
client_drop_privilege (VALUE self)
{
    GError *error = NULL;

    if (!milter_client_drop_privilege(SELF(self), &error))
	RAISE_GERROR(error);

    return self;
}

static VALUE
client_daemonize (VALUE self)
{
    GError *error = NULL;

    if (!milter_client_daemonize(SELF(self), &error))
	RAISE_GERROR(error);

    return self;
}

static VALUE
client_create_context (VALUE self)
{
    return GOBJ2RVAL_UNREF(milter_client_create_context(SELF(self)));
}

static VALUE
client_set_connection_spec (VALUE self, VALUE spec)
{
    GError *error = NULL;

    if (!milter_client_set_connection_spec(SELF(self),
                                           StringValueCStr(spec), &error))
        RAISE_GERROR(error);

    return self;
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
    return self;
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
    return self;
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
    return self;
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
    return self;
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
    return self;
}

static void
mark (gpointer data)
{
    MilterClient *client = data;

    milter_client_processing_context_foreach(client,
					     (GFunc)rbgobj_gc_mark_instance,
					     NULL);
}

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
    rb_define_method(rb_cMilterClient, "shutdown", client_shutdown, 0);
    rb_define_method(rb_cMilterClient, "start_syslog", client_start_syslog, -1);
    rb_define_method(rb_cMilterClient, "stop_syslog", client_stop_syslog, 0);
    rb_define_method(rb_cMilterClient, "syslog_enabled?",
		     client_syslog_enabled_p, 0);
    rb_define_method(rb_cMilterClient, "listen", client_listen, 0);
    rb_define_method(rb_cMilterClient, "drop_privilege",
		     client_drop_privilege, 0);
    rb_define_method(rb_cMilterClient, "daemonize", client_daemonize, 0);
    rb_define_method(rb_cMilterClient, "create_context",
		     client_create_context, 0);
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

    G_DEF_CLASS(MILTER_TYPE_CLIENT_EVENT_LOOP_BACKEND,
		"ClientEventLoopBackend",
		rb_mMilter);
    G_DEF_CONSTANTS(rb_cMilterClient, MILTER_TYPE_CLIENT_EVENT_LOOP_BACKEND,
		    "MILTER_CLIENT_");

    Init_milter_client_context();
}
