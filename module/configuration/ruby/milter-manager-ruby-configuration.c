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

#include <milter/manager/milter-manager-ruby-internal.h>

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif /* HAVE_CONFIG_H */

#include <signal.h>
#include <gmodule.h>

#include <milter/manager/milter-manager-configuration.h>
#include <milter/manager/milter-manager-module-impl.h>

#define MILTER_MANAGER_TYPE_RUBY_CONFIGURATION            milter_manager_type_ruby_configuration
#define MILTER_MANAGER_RUBY_CONFIGURATION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MILTER_MANAGER_TYPE_RUBY_CONFIGURATION, MilterManagerRubyConfiguration))
#define MILTER_MANAGER_RUBY_CONFIGURATION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MILTER_MANAGER_TYPE_RUBY_CONFIGURATION, MilterManagerRubyConfigurationClass))
#define MILTER_MANAGER_IS_RUBY_CONFIGURATION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MILTER_MANAGER_TYPE_RUBY_CONFIGURATION))
#define MILTER_MANAGER_IS_RUBY_CONFIGURATION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MILTER_MANAGER_TYPE_RUBY_CONFIGURATION))
#define MILTER_MANAGER_RUBY_CONFIGURATION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), MILTER_MANAGER_TYPE_RUBY_CONFIGURATION, MilterManagerRubyConfigurationClass))

typedef struct _MilterManagerRubyConfiguration MilterManagerRubyConfiguration;
typedef struct _MilterManagerRubyConfigurationClass MilterManagerRubyConfigurationClass;
struct _MilterManagerRubyConfiguration
{
    MilterManagerConfiguration object;
};

struct _MilterManagerRubyConfigurationClass
{
    MilterManagerConfigurationClass parent_class;
};

static GType milter_manager_type_ruby_configuration = 0;
static MilterManagerConfigurationClass *parent_class;

static VALUE rb_mMilterManagerConfigurationLoader = Qnil;

static void dispose        (GObject         *object);
static void set_property   (GObject         *object,
                            guint            prop_id,
                            const GValue    *value,
                            GParamSpec      *pspec);
static void get_property   (GObject         *object,
                            guint            prop_id,
                            GValue          *value,
                            GParamSpec      *pspec);

static void         real_add_load_path    (MilterManagerConfiguration *configuration,
                                           const gchar             *path);
static void         real_load             (MilterManagerConfiguration *configuration,
                                           const gchar             *file_name);


static void
class_init (MilterManagerRubyConfigurationClass *klass)
{
    GObjectClass *gobject_class;

    parent_class = g_type_class_peek_parent(klass);

    gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->dispose      = dispose;
    gobject_class->set_property = set_property;
    gobject_class->get_property = get_property;

    parent_class->add_load_path = real_add_load_path;
    parent_class->load = real_load;
}

static void
init (MilterManagerRubyConfiguration *configuration)
{
}

static void
register_type (GTypeModule *type_module)
{
    static const GTypeInfo info =
        {
            sizeof (MilterManagerRubyConfigurationClass),
            (GBaseInitFunc) NULL,
            (GBaseFinalizeFunc) NULL,
            (GClassInitFunc) class_init,
            NULL,           /* class_finalize */
            NULL,           /* class_data */
            sizeof(MilterManagerRubyConfiguration),
            0,
            (GInstanceInitFunc) init,
        };

    milter_manager_type_ruby_configuration =
        g_type_module_register_type(type_module,
                                    MILTER_MANAGER_TYPE_CONFIGURATION,
                                    "MilterManagerRubyConfiguration",
                                    &info, 0);
}

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

typedef struct _FuncallArguments
{
    VALUE receiver;
    ID name;
    int argc;
    VALUE *argv;
} FuncallArguments;

static VALUE
invoke_rb_funcall2 (VALUE data)
{
    FuncallArguments *arguments = (FuncallArguments *)data;

    return rb_funcall2(arguments->receiver, arguments->name,
                       arguments->argc, arguments->argv);
}

static VALUE
rb_funcall_protect (VALUE receiver, ID name, guint argc, ...)
{
    VALUE *argv;
    va_list args;
    VALUE result, error;
    FuncallArguments arguments_data;
    int state = 0;
    guint i;

    argv = ALLOC_N(VALUE, argc);
    va_start(args, argc);
    for (i = 0; i < argc; i++) {
        argv[i] = va_arg(args, VALUE);
    }
    arguments_data.receiver = receiver;
    arguments_data.name = name;
    arguments_data.argc = argc;
    arguments_data.argv = argv;
    result = rb_protect(invoke_rb_funcall2,
                        (VALUE)&arguments_data,
                        &state);
    va_end(args);

    error = rb_errinfo();
    if (state && !NIL_P(error)) {
        VALUE message, class_name, backtrace;
        long i;

        message = rb_funcall(error, rb_intern("message"), 0);
        class_name = rb_funcall(CLASS_OF(error), rb_intern("to_s"), 0);
        milter_error("%s (%s)", RVAL2CSTR(message), RVAL2CSTR(class_name));
        backtrace = rb_funcall(error, rb_intern("backtrace"), 0);
        for (i = 0; i < RARRAY(backtrace)->len; i++) {
            milter_error("%s", RVAL2CSTR(RARRAY(backtrace)->ptr[i]));
        }
    }

    return result;
}


static void
load_libraries (void)
{
    VALUE milter, milter_manager;

    rb_funcall_protect(Qnil, rb_intern("require"),
                       1, rb_str_new2("milter/manager"));

    milter = rb_const_get(rb_cObject, rb_intern("Milter"));
    milter_manager = rb_const_get(milter, rb_intern("Manager"));
    rb_mMilterManagerConfigurationLoader =
        rb_const_get(milter_manager, rb_intern("ConfigurationLoader"));
}

static void
init_ruby (void)
{
    int argc;
    char *argv[] = {"milter-manager"};

    argc = sizeof(argv) / sizeof(*argv);
    ruby_init_without_signal_change();
    ruby_script(argv[0]);
    ruby_set_argv(argc, argv);
    rb_argv0 = rb_gv_get("$PROGRAM_NAME");
    ruby_incpush(BINDING_LIB_DIR);
    ruby_incpush(BINDING_EXT_DIR);
    ruby_init_loadpath();
    load_libraries();
}

G_MODULE_EXPORT GList *
MILTER_MANAGER_MODULE_IMPL_INIT (GTypeModule *type_module)
{
    GList *registered_types = NULL;

    register_type(type_module);
    if (milter_manager_type_ruby_configuration)
        registered_types =
            g_list_prepend(registered_types,
                           (gchar *)g_type_name(milter_manager_type_ruby_configuration));

    init_ruby();

    return registered_types;
}


static void
ruby_cleanup_without_signal_change (int exit_code)
{
    RETSIGTYPE (*sigint_handler)_((int));

    sigint_handler = signal(SIGINT, SIG_DFL);
    ruby_cleanup(exit_code);
    signal(SIGINT, sigint_handler);
}

G_MODULE_EXPORT void
MILTER_MANAGER_MODULE_IMPL_EXIT (void)
{
    ruby_cleanup_without_signal_change(0);
}

G_MODULE_EXPORT GObject *
MILTER_MANAGER_MODULE_IMPL_INSTANTIATE (const gchar *first_property,
                                        va_list var_args)
{
    return g_object_new_valist(MILTER_MANAGER_TYPE_RUBY_CONFIGURATION,
                               first_property, var_args);
}

static void
dispose (GObject *object)
{
    MilterManagerRubyConfiguration *configuration;

    configuration = MILTER_MANAGER_RUBY_CONFIGURATION(object);

    G_OBJECT_CLASS(parent_class)->dispose(object);
}

static void
set_property (GObject      *object,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
    MilterManagerRubyConfiguration *configuration;

    configuration = MILTER_MANAGER_RUBY_CONFIGURATION(object);
    switch (prop_id) {
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static void
get_property (GObject    *object,
              guint       prop_id,
              GValue     *value,
              GParamSpec *pspec)
{
    MilterManagerRubyConfiguration *configuration;

    configuration = MILTER_MANAGER_RUBY_CONFIGURATION(object);
    switch (prop_id) {
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static void
real_add_load_path (MilterManagerConfiguration *_configuration, const gchar *path)
{
    MilterManagerRubyConfiguration *configuration;

    configuration = MILTER_MANAGER_RUBY_CONFIGURATION(_configuration);
    rb_funcall_protect(rb_mMilterManagerConfigurationLoader,
                       rb_intern("add_load_path"), 1,
                       rb_str_new2(path));
}

static void
real_load (MilterManagerConfiguration *_configuration, const gchar *file_name)
{
    MilterManagerRubyConfiguration *configuration;

    configuration = MILTER_MANAGER_RUBY_CONFIGURATION(_configuration);
    rb_funcall_protect(rb_mMilterManagerConfigurationLoader,
                       rb_intern("load"), 2,
                       GOBJ2RVAL(configuration),
                       rb_str_new2(file_name));
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
