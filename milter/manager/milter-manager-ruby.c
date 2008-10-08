/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "milter-manager-ruby-internal.h"

#ifdef HAVE_CONFIG_H
#  include "../../config.h"
#endif /* HAVE_CONFIG_H */

#include <signal.h>

#include "milter-manager-ruby.h"

static void
ruby_init_without_signal_change (void)
{
    RETSIGTYPE (*sigint_handler)_((int));
#ifdef SIGHUP
    RETSIGTYPE (*sighup_handler)_((int));
#endif
#ifdef SIGQUIT
    RETSIGTYPE (*sigquit_handler)_((int));
#endif
#ifdef SIGTERM
    RETSIGTYPE (*sigterm_handler)_((int));
#endif
#ifdef SIGSEGV
    RETSIGTYPE (*sigsegv_handler)_((int));
#endif

    sigint_handler = signal(SIGINT, SIG_DFL);
#ifdef SIGHUP
    sighup_handler = signal(SIGHUP, SIG_DFL);
#endif
#ifdef SIGQUIT
    sigquit_handler = signal(SIGQUIT, SIG_DFL);
#endif
#ifdef SIGTERM
    sigterm_handler = signal(SIGTERM, SIG_DFL);
#endif
#ifdef SIGSEGV
    sigsegv_handler = signal(SIGSEGV, SIG_DFL);
#endif

    ruby_init();

    signal(SIGINT, sigint_handler);
#ifdef SIGHUP
    signal(SIGHUP, sighup_handler);
#endif
#ifdef SIGQUIT
    signal(SIGQUIT, sigquit_handler);
#endif
#ifdef SIGTERM
    signal(SIGTERM, sigterm_handler);
#endif
#ifdef SIGSEGV
    signal(SIGSEGV, sigsegv_handler);
#endif
}

extern VALUE rb_load_path;

static void
load_libraries (void)
{
    rb_funcall(Qnil, rb_intern("require"), 1, rb_str_new2("glib2"));
    rb_funcall(Qnil, rb_intern("require"), 1, rb_str_new2("milter"));
    Init_milter_manager();
    rb_funcall(Qnil, rb_intern("require"), 1, rb_str_new2("milter/manager"));
}

void
milter_manager_ruby_init (int *argc, char ***argv)
{
    ruby_init_without_signal_change();
    ruby_script((*argv)[0]);
    ruby_set_argv(*argc, *argv);
    rb_argv0 = rb_gv_get("$PROGRAM_NAME");
    ruby_init_loadpath();
    load_libraries();
}

static void
ruby_cleanup_without_signal_change (int exit_code)
{
    RETSIGTYPE (*sigint_handler)_((int));

    sigint_handler = signal(SIGINT, SIG_DFL);
    ruby_cleanup(exit_code);
    signal(SIGINT, sigint_handler);
}

void
milter_manager_ruby_quit (void)
{
    ruby_cleanup_without_signal_change(0);
}

void
milter_manager_ruby_load (MilterManagerConfiguration *configuration)
{
    rb_funcall(rb_const_get(rb_mMilterManager, rb_intern("ConfigurationLoader")),
               rb_intern("load"), 2,
               Qnil, /*FIXME: GOBJ2RVAL(configuration) */
               rb_str_new2("/tmp/milter-manager.conf"));
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
