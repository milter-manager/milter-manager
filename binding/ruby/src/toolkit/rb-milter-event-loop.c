/* -*- c-file-style: "ruby" -*- */
/*
 *  Copyright (C) 2011  Kouhei Sutou <kou@clear-code.com>
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

#include "rb-milter-core-private.h"
#ifdef RUBY_UBF_IO
#  define USE_BLOCKING_REGION 1
#else
#  define USE_BLOCKING_REGION 0
#endif
#if !USE_BLOCKING_REGION
#  include <rubysig.h>
#endif

#define SELF(self) RVAL2EVENT_LOOP(self)

typedef struct _CallbackContext CallbackContext;
struct _CallbackContext
{
    VALUE event_loop;
    VALUE callback;
};

static CallbackContext *
callback_context_new (VALUE event_loop, VALUE callback)
{
    CallbackContext *context;
    VALUE callbacks;

    context = g_new(CallbackContext, 1);
    context->event_loop = event_loop;
    context->callback = callback;

    callbacks = rb_iv_get(context->event_loop, "callbacks");
    if (NIL_P(callbacks)) {
	callbacks = rb_ary_new();
	rb_iv_set(context->event_loop, "callbacks", callbacks);
    }
    rb_ary_push(callbacks, callback);

    return context;
}

static void
callback_context_free (CallbackContext *context)
{
    VALUE callback, callbacks;
    VALUE *callbacks_raw;
    long i, length;

    callback = context->callback;
    callbacks = rb_iv_get(context->event_loop, "callbacks");
    callbacks_raw = RARRAY_PTR(callbacks);
    length = RARRAY_LEN(callbacks);
    for (i = 0; i < length; i++) {
	if (rb_equal(callbacks_raw[i], callback)) {
	    rb_ary_delete_at(callbacks, i);
	    break;
	}
    }
    g_free(context);
}

static gboolean
cb_timeout (gpointer user_data)
{
    CallbackContext *context = user_data;

    return RVAL2CBOOL(rb_funcall(context->callback, rb_intern("call"), 0));
}

static void
cb_timeout_notify (gpointer user_data)
{
    CallbackContext *context = user_data;
    callback_context_free(context);
}

static VALUE
rb_loop_add_timeout (int argc, VALUE *argv, VALUE self)
{
    VALUE rb_interval, rb_priority, rb_options, rb_block;
    CallbackContext *context;
    gdouble interval;
    gint priority = G_PRIORITY_DEFAULT;
    guint tag;

    rb_scan_args(argc, argv, "11&", &rb_interval, &rb_options, &rb_block);

    interval = NUM2DBL(rb_interval);

    rb_milter__scan_options(rb_options,
			    "priority", &rb_priority,
			    NULL);

    if (!NIL_P(rb_priority))
	priority = NUM2INT(rb_priority);

    if (NIL_P(rb_block))
	rb_raise(rb_eArgError, "timeout block is missing");

    context = callback_context_new(self, rb_block);
    tag = milter_event_loop_add_timeout_full(SELF(self),
					     priority,
					     interval,
					     cb_timeout,
					     context,
					     cb_timeout_notify);

    return UINT2NUM(tag);
}

static VALUE
rb_loop_iterate (int argc, VALUE *argv, VALUE self)
{
    VALUE rb_may_block, rb_options;
    gboolean may_block, event_dispatched;

    rb_scan_args(argc, argv, "01", &rb_options);
    rb_milter__scan_options(rb_options,
			    "may_block", &rb_may_block,
			    NULL);
    may_block = RVAL2CBOOL(rb_may_block);
    event_dispatched = milter_event_loop_iterate(SELF(self), may_block);
    return CBOOL2RVAL(event_dispatched);
}

static VALUE
rb_loop_remove (VALUE self, VALUE tag)
{
    milter_event_loop_remove(SELF(self), NUM2UINT(tag));
    return Qnil;
}

#if USE_BLOCKING_REGION
static VALUE
rb_loop_run_block (void *arg)
{
    MilterEventLoop *loop = arg;
    milter_event_loop_iterate(loop, TRUE);
    return Qnil;
}

static void
rb_loop_run_unblock (void *arg)
{
    MilterEventLoop *loop = arg;
    milter_event_loop_quit(loop);
}
#endif

static void
rb_loop_run (MilterEventLoop *loop)
{
    while (milter_event_loop_is_running(loop)) {
#if USE_BLOCKING_REGION
	rb_thread_blocking_region(rb_loop_run_block, loop,
				  rb_loop_run_unblock, loop);
#else
	TRAP_BEG;
	milter_event_loop_iterate(loop, TRUE);
	TRAP_END;
#endif
    }
}

static VALUE
glib_setup (VALUE self)
{
    return Qnil;
}

static VALUE
glib_initialize (int argc, VALUE *argv, VALUE self)
{
    VALUE main_context;
    MilterEventLoop *event_loop;

    rb_scan_args(argc, argv, "01", &main_context);

    event_loop = milter_glib_event_loop_new(RVAL2GOBJ(main_context));
    G_INITIALIZE(self, event_loop);
    glib_setup(self);

    return Qnil;
}

static VALUE
libev_setup (VALUE self)
{
    milter_event_loop_set_custom_run_func(SELF(self), rb_loop_run);
    return self;
}

static VALUE
libev_initialize (VALUE self)
{
    MilterEventLoop *event_loop;

    event_loop = milter_libev_event_loop_new();
    G_INITIALIZE(self, event_loop);
    libev_setup(self);

    return Qnil;
}

void
Init_milter_event_loop (void)
{
    VALUE rb_cMilterEventLoop, rb_cMilterGLibEventLoop, rb_cMilterLibevEventLoop;

    rb_cMilterEventLoop = G_DEF_CLASS(MILTER_TYPE_EVENT_LOOP,
                                      "EventLoop", rb_mMilter);

    rb_define_method(rb_cMilterEventLoop, "add_timeout",
		     rb_loop_add_timeout, -1);
    rb_define_method(rb_cMilterEventLoop, "iterate", rb_loop_iterate, -1);
    rb_define_method(rb_cMilterEventLoop, "remove", rb_loop_remove, 1);

    G_DEF_SETTERS(rb_cMilterEventLoop);

    rb_cMilterGLibEventLoop = G_DEF_CLASS(MILTER_TYPE_GLIB_EVENT_LOOP,
                                          "GLibEventLoop", rb_mMilter);

    rb_define_method(rb_cMilterGLibEventLoop, "initialize", glib_initialize, -1);
    rb_define_method(rb_cMilterGLibEventLoop, "setup", glib_setup, 0);

    G_DEF_SETTERS(rb_cMilterGLibEventLoop);

    rb_cMilterLibevEventLoop = G_DEF_CLASS(MILTER_TYPE_LIBEV_EVENT_LOOP,
                                          "LibevEventLoop", rb_mMilter);

    rb_define_method(rb_cMilterLibevEventLoop, "initialize", libev_initialize, 0);
    rb_define_method(rb_cMilterLibevEventLoop, "setup", libev_setup, 0);

    G_DEF_SETTERS(rb_cMilterGLibEventLoop);
}
