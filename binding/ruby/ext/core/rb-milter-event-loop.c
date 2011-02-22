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
#  define USE_DIRECT_THREAD_BLOCKING_REGION 1
struct rb_blocking_region_buffer;
struct rb_blocking_region_buffer *rb_thread_blocking_region_begin(void);
void rb_thread_blocking_region_end(struct rb_blocking_region_buffer *buffer);
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

static void
cb_callback_context_notify (gpointer user_data)
{
    CallbackContext *context = user_data;
    callback_context_free(context);
}

static gboolean
cb_watch_io (GIOChannel *channel, GIOCondition condition, gpointer user_data)
{
    CallbackContext *context = user_data;

    return RVAL2CBOOL(rb_funcall(context->callback, rb_intern("call"), 2,
				 BOXED2RVAL(channel, G_TYPE_IO_CHANNEL),
				 UINT2NUM(condition)));
}

static VALUE
rb_loop_watch_io (int argc, VALUE *argv, VALUE self)
{
    VALUE rb_channel, rb_condition, rb_priority, rb_options, rb_block;
    CallbackContext *context;
    GIOChannel *channel;
    GIOCondition condition;
    gint priority = G_PRIORITY_DEFAULT;
    guint tag;

    rb_scan_args(argc, argv, "21&",
		 &rb_channel, &rb_condition, &rb_options, &rb_block);

    channel = RVAL2BOXED(rb_channel, G_TYPE_IO_CHANNEL);
    condition = RVAL2GFLAGS(rb_condition, G_TYPE_IO_CONDITION);

    rb_milter__scan_options(rb_options,
			    "priority", &rb_priority,
			    NULL);

    if (!NIL_P(rb_priority))
	priority = NUM2INT(rb_priority);

    if (NIL_P(rb_block))
	rb_raise(rb_eArgError, "watch IO block is missing");

    context = callback_context_new(self, rb_block);
    tag = milter_event_loop_watch_io_full(SELF(self),
					  priority,
					  channel,
					  condition,
					  cb_watch_io,
					  context,
					  cb_callback_context_notify);

    return UINT2NUM(tag);
}

static VALUE
last_status_set(gint status, GPid pid)
{
#ifdef HAVE_RB_LAST_STATUS_SET
    rb_last_status_set(status, (rb_pid_t)pid);
    return rb_last_status_get();
#else
    extern VALUE rb_last_status;
    VALUE last_status = rb_obj_alloc(rb_path2class("Process::Status"));
    rb_iv_set(last_status, "status", INT2FIX(status));
    rb_iv_set(last_status, "pid", INT2FIX(pid));
    rb_last_status = last_status;
    return last_status;
#endif
}

static void
cb_watch_child (GPid pid, gint status, gpointer user_data)
{
    CallbackContext *context = user_data;

    rb_funcall(context->callback, rb_intern("call"), 1,
	       last_status_set(status, pid));
}

static VALUE
rb_loop_watch_child (int argc, VALUE *argv, VALUE self)
{
    VALUE rb_pid, rb_priority, rb_options, rb_block;
    CallbackContext *context;
    GPid pid;
    gint priority = G_PRIORITY_DEFAULT;
    guint tag;

    rb_scan_args(argc, argv, "11&", &rb_pid, &rb_options, &rb_block);

    pid = (GPid)NUM2INT(rb_pid);

    rb_milter__scan_options(rb_options,
			    "priority", &rb_priority,
			    NULL);

    if (!NIL_P(rb_priority))
	priority = NUM2INT(rb_priority);

    if (NIL_P(rb_block))
	rb_raise(rb_eArgError, "watch child block is missing");

    context = callback_context_new(self, rb_block);
    tag = milter_event_loop_watch_child_full(SELF(self),
					     priority,
					     pid,
					     cb_watch_child,
					     context,
					     cb_callback_context_notify);

    return UINT2NUM(tag);
}

static gboolean
cb_timeout (gpointer user_data)
{
    CallbackContext *context = user_data;

    return RVAL2CBOOL(rb_funcall(context->callback, rb_intern("call"), 0));
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
					     cb_callback_context_notify);

    return UINT2NUM(tag);
}

static gboolean
cb_idle (gpointer user_data)
{
    CallbackContext *context = user_data;

    return RVAL2CBOOL(rb_funcall(context->callback, rb_intern("call"), 0));
}

static VALUE
rb_loop_add_idle (int argc, VALUE *argv, VALUE self)
{
    VALUE rb_priority, rb_options, rb_block;
    CallbackContext *context;
    gint priority = G_PRIORITY_DEFAULT_IDLE;
    guint tag;

    rb_scan_args(argc, argv, "01&", &rb_options, &rb_block);

    rb_milter__scan_options(rb_options,
			    "priority", &rb_priority,
			    NULL);

    if (!NIL_P(rb_priority))
	priority = NUM2INT(rb_priority);

    if (NIL_P(rb_block))
	rb_raise(rb_eArgError, "idle block is missing");

    context = callback_context_new(self, rb_block);
    tag = milter_event_loop_add_idle_full(SELF(self),
					  priority,
					  cb_idle,
					  context,
					  cb_callback_context_notify);

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
rb_loop_run (VALUE self)
{
    milter_event_loop_run(SELF(self));
    return Qnil;
}

static VALUE
rb_loop_quit (VALUE self)
{
    milter_event_loop_quit(SELF(self));
    return Qnil;
}

static VALUE
rb_loop_remove (VALUE self, VALUE tag)
{
    milter_event_loop_remove(SELF(self), NUM2UINT(tag));
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
    rb_milter_event_loop_setup(event_loop);

    return Qnil;
}

static VALUE
libev_s_default (VALUE klass)
{
    VALUE rb_event_loop;
    MilterEventLoop *event_loop;

    event_loop = milter_libev_event_loop_default();
    rb_event_loop = GOBJ2RVAL(event_loop);
    g_object_unref(event_loop);
    rb_milter_event_loop_setup(event_loop);

    return rb_event_loop;
}

static VALUE
libev_initialize (VALUE self)
{
    MilterEventLoop *event_loop;

    event_loop = milter_libev_event_loop_new();
    G_INITIALIZE(self, event_loop);
    rb_milter_event_loop_setup(event_loop);

    return Qnil;
}

#ifdef USE_DIRECT_THREAD_BLOCKING_REGION
typedef struct _ReleaseData
{
    struct rb_blocking_region_buffer *buffer;
} ReleaseData;

static void
cb_release (MilterEventLoop *loop, gpointer user_data)
{
    ReleaseData *data = user_data;
    data->buffer = rb_thread_blocking_region_begin();
}

static void
cb_acquire (MilterEventLoop *loop, gpointer user_data)
{
    ReleaseData *data = user_data;
    rb_thread_blocking_region_end(data->buffer);
}

static void
cb_notify (gpointer user_data)
{
    ReleaseData *data = user_data;
    g_free(data);
}
#endif

void
rb_milter_event_loop_setup (MilterEventLoop *loop)
{
#ifdef USE_DIRECT_THREAD_BLOCKING_REGION
    if (MILTER_IS_LIBEV_EVENT_LOOP(loop)) {
	ReleaseData *data;
	data = g_new0(ReleaseData, 1);
	milter_libev_event_loop_set_release_func(loop,
						 (GFunc)cb_release,
						 (GFunc)cb_acquire,
						 data,
						 cb_notify);
    }
#endif
}

void
Init_milter_event_loop (void)
{
    VALUE rb_cMilterEventLoop, rb_cMilterGLibEventLoop, rb_cMilterLibevEventLoop;

    rb_cMilterEventLoop = G_DEF_CLASS(MILTER_TYPE_EVENT_LOOP,
                                      "EventLoop", rb_mMilter);

    rb_define_method(rb_cMilterEventLoop, "watch_io",
		     rb_loop_watch_io, -1);
    rb_define_method(rb_cMilterEventLoop, "watch_child",
		     rb_loop_watch_child, -1);
    rb_define_method(rb_cMilterEventLoop, "add_timeout",
		     rb_loop_add_timeout, -1);
    rb_define_method(rb_cMilterEventLoop, "add_idle",
		     rb_loop_add_idle, -1);
    rb_define_method(rb_cMilterEventLoop, "iterate", rb_loop_iterate, -1);
    rb_define_method(rb_cMilterEventLoop, "run", rb_loop_run, 0);
    rb_define_method(rb_cMilterEventLoop, "quit", rb_loop_quit, 0);
    rb_define_method(rb_cMilterEventLoop, "remove", rb_loop_remove, 1);

    G_DEF_SETTERS(rb_cMilterEventLoop);

    rb_cMilterGLibEventLoop = G_DEF_CLASS(MILTER_TYPE_GLIB_EVENT_LOOP,
                                          "GLibEventLoop", rb_mMilter);

    rb_define_method(rb_cMilterGLibEventLoop, "initialize", glib_initialize, -1);

    G_DEF_SETTERS(rb_cMilterGLibEventLoop);

    rb_cMilterLibevEventLoop = G_DEF_CLASS(MILTER_TYPE_LIBEV_EVENT_LOOP,
                                          "LibevEventLoop", rb_mMilter);

    rb_define_singleton_method(rb_cMilterLibevEventLoop, "default",
			       libev_s_default, 0);

    rb_define_method(rb_cMilterLibevEventLoop, "initialize",
		     libev_initialize, 0);

    G_DEF_SETTERS(rb_cMilterGLibEventLoop);
}
