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
