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

#define MILTER_TYPE_MANAGER_RUBY_CONFIGURATION            (milter_manager_ruby_configuration_get_type())
#define MILTER_MANAGER_RUBY_CONFIGURATION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MILTER_TYPE_MANAGER_RUBY_CONFIGURATION, MilterManagerRubyConfiguration))
#define MILTER_MANAGER_RUBY_CONFIGURATION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MILTER_TYPE_MANAGER_RUBY_CONFIGURATION, MilterManagerRubyConfigurationClass))
#define MILTER_MANAGER_IS_RUBY_CONFIGURATION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MILTER_TYPE_MANAGER_RUBY_CONFIGURATION))
#define MILTER_MANAGER_IS_RUBY_CONFIGURATION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MILTER_TYPE_MANAGER_RUBY_CONFIGURATION))
#define MILTER_MANAGER_RUBY_CONFIGURATION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), MILTER_TYPE_MANAGER_RUBY_CONFIGURATION, MilterManagerRubyConfigurationClass))

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

static gboolean real_load         (MilterManagerConfiguration *configuration,
                                   const gchar                *file_name,
                                   GError                    **error);
static gboolean real_load_custom  (MilterManagerConfiguration *configuration,
                                   const gchar                *file_name,
                                   GError                    **error);

static gpointer milter_manager_ruby_configuration_parent_class = NULL;
static GType    milter_manager_ruby_configuration_type_id = 0;

static GType
milter_manager_ruby_configuration_get_type (void)
{
    return milter_manager_ruby_configuration_type_id;
}

static void
milter_manager_ruby_configuration_class_init (MilterManagerRubyConfigurationClass *klass)
{
    GObjectClass *gobject_class;
    MilterManagerConfigurationClass *configuration_class;

    gobject_class = G_OBJECT_CLASS(klass);
    configuration_class = MILTER_MANAGER_CONFIGURATION_CLASS(klass);

    gobject_class->dispose      = dispose;
    gobject_class->set_property = set_property;
    gobject_class->get_property = get_property;

    configuration_class->load = real_load;
    configuration_class->load_custom = real_load_custom;
}

static void
milter_manager_ruby_configuration_class_finalize (MilterManagerRubyConfigurationClass *klass)
{
}

static void
milter_manager_ruby_configuration_init (MilterManagerRubyConfiguration *configuration)
{
}

static void
milter_manager_ruby_configuration_class_intern_init (gpointer klass)
{
    milter_manager_ruby_configuration_parent_class = g_type_class_peek_parent(klass);
    milter_manager_ruby_configuration_class_init((MilterManagerRubyConfigurationClass*)klass);
}

static void
milter_manager_ruby_configuration_register_type (GTypeModule *type_module)
{
    GType g_define_type_id;
    const GTypeInfo g_define_type_info = {
        sizeof (MilterManagerRubyConfigurationClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) milter_manager_ruby_configuration_class_intern_init,
        (GClassFinalizeFunc) milter_manager_ruby_configuration_class_finalize,
        NULL,   /* class_data */
        sizeof (MilterManagerRubyConfiguration),
        0,      /* n_preallocs */
        (GInstanceInitFunc) milter_manager_ruby_configuration_init,
        NULL    /* value_table */
    };
    
    milter_manager_ruby_configuration_type_id =
        g_type_module_register_type(type_module,
                                    MILTER_TYPE_MANAGER_CONFIGURATION,
                                    "MilterManagerRubyConfiguration",
                                    &g_define_type_info,
                                    (GTypeFlags) 0);
    g_define_type_id = milter_manager_ruby_configuration_type_id;
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
rb_funcall_protect (GError **g_error, VALUE receiver, ID name, guint argc, ...)
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
        GString *error_message;
        VALUE message, class_name, backtrace;
        long i;

        error_message = g_string_new(NULL);
        message = rb_funcall(error, rb_intern("message"), 0);
        class_name = rb_funcall(CLASS_OF(error), rb_intern("to_s"), 0);
        g_string_append_printf(error_message, "%s (%s)\n",
                               RVAL2CSTR(message), RVAL2CSTR(class_name));
        backtrace = rb_funcall(error, rb_intern("backtrace"), 0);
        for (i = 0; i < RARRAY(backtrace)->len; i++) {
            g_string_append_printf(error_message, "%s\n",
                                   RVAL2CSTR(RARRAY(backtrace)->ptr[i]));
        }
        g_set_error(g_error,
                    MILTER_MANAGER_CONFIGURATION_ERROR,
                    MILTER_MANAGER_CONFIGURATION_ERROR_UNKNOWN,
                    "unknown error is occurred: <%s>", error_message->str);
        g_string_free(error_message, TRUE);
    }

    return result;
}


static void
load_libraries (void)
{
    VALUE milter, milter_manager;
    GError *error = NULL;

    rb_funcall_protect(&error,
                       Qnil, rb_intern("require"),
                       1, rb_str_new2("milter/manager"));

    if (error) {
        milter_error("%s", error->message);
        g_error_free(error);
    }

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

    milter_manager_ruby_configuration_register_type(type_module);
    if (milter_manager_ruby_configuration_type_id)
        registered_types =
            g_list_prepend(registered_types,
                           (gchar *)g_type_name(milter_manager_ruby_configuration_type_id));

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
    return g_object_new_valist(MILTER_TYPE_MANAGER_RUBY_CONFIGURATION,
                               first_property, var_args);
}

static void
dispose (GObject *object)
{
    MilterManagerRubyConfiguration *configuration;

    configuration = MILTER_MANAGER_RUBY_CONFIGURATION(object);

    G_OBJECT_CLASS(milter_manager_ruby_configuration_parent_class)->dispose(object);
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

static gboolean
load (MilterManagerConfiguration *_configuration, ID method_name,
      const gchar *file_name, GError **error)
{
    MilterManagerRubyConfiguration *configuration;
    GError *local_error = NULL;
    gboolean success;

    configuration = MILTER_MANAGER_RUBY_CONFIGURATION(_configuration);
    rb_funcall_protect(&local_error,
                       rb_mMilterManagerConfigurationLoader,
                       method_name, 2,
                       GOBJ2RVAL(configuration),
                       rb_str_new2(file_name));

    if (local_error) {
        success = FALSE;
        if (!error)
            milter_error("%s", local_error->message);
        g_propagate_error(error, local_error);
    } else {
        success = TRUE;
    }

    return success;
}

static gboolean
real_load (MilterManagerConfiguration *_configuration, const gchar *file_name,
           GError **error)
{
    return load(_configuration, rb_intern("load"), file_name, error);
}

static gboolean
real_load_custom (MilterManagerConfiguration *_configuration,
                  const gchar *file_name, GError **error)
{
    return load(_configuration, rb_intern("load_custom"), file_name, error);
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
