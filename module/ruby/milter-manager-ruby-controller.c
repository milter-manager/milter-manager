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
#  include "../../config.h"
#endif /* HAVE_CONFIG_H */

#include <signal.h>
#include <gmodule.h>

#include <milter/manager/milter-manager-controller.h>
#include <milter/manager/milter-manager-module-impl.h>

#define MILTER_MANAGER_TYPE_RUBY_CONTROLLER            milter_manager_type_ruby_controller
#define MILTER_MANAGER_RUBY_CONTROLLER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MILTER_MANAGER_TYPE_RUBY_CONTROLLER, MilterManagerRubyController))
#define MILTER_MANAGER_RUBY_CONTROLLER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MILTER_MANAGER_TYPE_RUBY_CONTROLLER, MilterManagerRubyControllerClass))
#define MILTER_MANAGER_IS_RUBY_CONTROLLER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MILTER_MANAGER_TYPE_RUBY_CONTROLLER))
#define MILTER_MANAGER_IS_RUBY_CONTROLLER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MILTER_MANAGER_TYPE_RUBY_CONTROLLER))
#define MILTER_MANAGER_RUBY_CONTROLLER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), MILTER_MANAGER_TYPE_RUBY_CONTROLLER, MilterManagerRubyControllerClass))

typedef struct _MilterManagerRubyController MilterManagerRubyController;
typedef struct _MilterManagerRubyControllerClass MilterManagerRubyControllerClass;
struct _MilterManagerRubyController
{
    GObject       object;
    MilterManagerConfiguration *configuration;
};

struct _MilterManagerRubyControllerClass
{
    GObjectClass parent_class;
};

enum
{
    PROP_0,
    PROP_CONFIGURATION
};

static GType milter_manager_type_ruby_controller = 0;
static GObjectClass *parent_class;

static void dispose        (GObject         *object);
static void set_property   (GObject         *object,
                            guint            prop_id,
                            const GValue    *value,
                            GParamSpec      *pspec);
static void get_property   (GObject         *object,
                            guint            prop_id,
                            GValue          *value,
                            GParamSpec      *pspec);

static void         real_load             (MilterManagerController *controller,
                                           MilterManagerConfiguration *configuration);
static MilterStatus real_negotiate        (MilterManagerController *controller,
                                           MilterOption            *option);
static MilterStatus real_connect          (MilterManagerController *controller,
                                           const gchar             *host_name,
                                           struct sockaddr         *address,
                                           socklen_t                address_length);
static MilterStatus real_helo             (MilterManagerController *controller,
                                           const gchar *fqdn);
static MilterStatus real_envelope_from    (MilterManagerController *controller,
                                           const gchar             *from);
static MilterStatus real_envelope_receipt (MilterManagerController *controller,
                                           const gchar             *receipt);
static MilterStatus real_data             (MilterManagerController *controller);
static MilterStatus real_unknown          (MilterManagerController *controller,
                                           const gchar             *command);
static MilterStatus real_header           (MilterManagerController *controller,
                                           const gchar             *name,
                                           const gchar             *value);
static MilterStatus real_end_of_header    (MilterManagerController *controller);
static MilterStatus real_body             (MilterManagerController *controller,
                                           const guchar            *chunk,
                                           gsize                    size);
static MilterStatus real_end_of_message   (MilterManagerController *controller);
static MilterStatus real_close            (MilterManagerController *controller);
static MilterStatus real_abort            (MilterManagerController *controller);


static void
class_init (MilterManagerRubyControllerClass *klass)
{
    GObjectClass *gobject_class;
    GParamSpec *spec;

    parent_class = g_type_class_peek_parent(klass);

    gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->dispose      = dispose;
    gobject_class->set_property = set_property;
    gobject_class->get_property = get_property;

    spec = g_param_spec_object("configuration",
                               "Configuration",
                               "The configuration of the milter manager",
                               MILTER_MANAGER_TYPE_CONFIGURATION,
                               G_PARAM_READWRITE);
    g_object_class_install_property(gobject_class, PROP_CONFIGURATION, spec);
}

static void
init (MilterManagerRubyController *controller)
{
    controller->configuration = NULL;
}

static void
controller_init (MilterManagerControllerClass *controller)
{
    controller->load = real_load;
    controller->negotiate = real_negotiate;
    controller->connect = real_connect;
    controller->helo = real_helo;
    controller->envelope_from = real_envelope_from;
    controller->envelope_receipt = real_envelope_receipt;
    controller->data = real_data;
    controller->unknown = real_unknown;
    controller->header = real_header;
    controller->end_of_header = real_end_of_header;
    controller->body = real_body;
    controller->end_of_message = real_end_of_message;
    controller->close = real_close;
    controller->abort = real_abort;
}

static void
register_type (GTypeModule *type_module)
{
    static const GTypeInfo info =
        {
            sizeof (MilterManagerRubyControllerClass),
            (GBaseInitFunc) NULL,
            (GBaseFinalizeFunc) NULL,
            (GClassInitFunc) class_init,
            NULL,           /* class_finalize */
            NULL,           /* class_data */
            sizeof(MilterManagerRubyController),
            0,
            (GInstanceInitFunc) init,
        };

    static const GInterfaceInfo controller_info =
        {
            (GInterfaceInitFunc) controller_init,
            NULL,
            NULL
        };

    milter_manager_type_ruby_controller =
        g_type_module_register_type(type_module,
                                    G_TYPE_OBJECT,
                                    "MilterManagerRubyController",
                                    &info, 0);

    g_type_module_add_interface(type_module,
                                milter_manager_type_ruby_controller,
                                MILTER_MANAGER_TYPE_CONTROLLER,
                                &controller_info);
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

static void
load_libraries (void)
{
    rb_funcall(Qnil, rb_intern("require"), 1, rb_str_new2("milter/manager"));
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
    if (milter_manager_type_ruby_controller)
        registered_types =
            g_list_prepend(registered_types,
                           (gchar *)g_type_name(milter_manager_type_ruby_controller));

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
    return g_object_new_valist(MILTER_MANAGER_TYPE_RUBY_CONTROLLER,
                               first_property, var_args);
}

static void
dispose (GObject *object)
{
    MilterManagerRubyController *controller;

    controller = MILTER_MANAGER_RUBY_CONTROLLER(object);
    if (controller->configuration) {
        g_object_unref(controller->configuration);
        controller->configuration = NULL;
    }

    G_OBJECT_CLASS(parent_class)->dispose(object);
}

static void
set_property (GObject      *object,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
    MilterManagerRubyController *controller;

    controller = MILTER_MANAGER_RUBY_CONTROLLER(object);
    switch (prop_id) {
      case PROP_CONFIGURATION:
        if (controller->configuration)
            g_object_unref(controller->configuration);
        controller->configuration = g_value_get_object(value);
        if (controller->configuration)
            g_object_ref(controller->configuration);
        break;
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
    MilterManagerRubyController *controller;

    controller = MILTER_MANAGER_RUBY_CONTROLLER(object);
    switch (prop_id) {
      case PROP_CONFIGURATION:
        g_value_set_object(value, controller->configuration);
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static void
real_load (MilterManagerController *controller,
           MilterManagerConfiguration *configuration)
{
    rb_funcall(rb_const_get(rb_mMilterManager, rb_intern("ConfigurationLoader")),
               rb_intern("load"), 2,
               GOBJ2RVAL(configuration),
               rb_str_new2("/tmp/milter-manager.conf"));
}

static MilterStatus
real_negotiate (MilterManagerController *controller, MilterOption *option)
{
    rb_p(rb_str_new2("negotiate"));
    return MILTER_STATUS_DEFAULT;
}

static MilterStatus
real_connect (MilterManagerController *controller,
              const gchar             *host_name,
              struct sockaddr         *address,
              socklen_t                address_length)
{
     rb_p(rb_str_new2("connect"));
    return MILTER_STATUS_DEFAULT;
}

static MilterStatus
real_helo (MilterManagerController *controller, const gchar *fqdn)
{
     rb_p(rb_str_new2("helo"));
    return MILTER_STATUS_DEFAULT;
}

static MilterStatus
real_envelope_from (MilterManagerController *controller, const gchar *from)
{
     rb_p(rb_str_new2("envelope-from"));
    return MILTER_STATUS_DEFAULT;
}

static MilterStatus
real_envelope_receipt (MilterManagerController *controller, const gchar *receipt)
{
     rb_p(rb_str_new2("envelope-receipt"));
    return MILTER_STATUS_DEFAULT;
}

static MilterStatus
real_data (MilterManagerController *controller)
{
     rb_p(rb_str_new2("data"));
    return MILTER_STATUS_DEFAULT;
}

static MilterStatus
real_unknown (MilterManagerController *controller, const gchar *command)
{
     rb_p(rb_str_new2("unknown"));
    return MILTER_STATUS_DEFAULT;
}

static MilterStatus
real_header (MilterManagerController *controller,
             const gchar *name, const gchar *value)
{
     rb_p(rb_str_new2("header"));
    return MILTER_STATUS_DEFAULT;
}

static MilterStatus
real_end_of_header (MilterManagerController *controller)
{
     rb_p(rb_str_new2("end-of-header"));
    return MILTER_STATUS_DEFAULT;
}

static MilterStatus
real_body (MilterManagerController *controller, const guchar *chunk, gsize size)
{
     rb_p(rb_str_new2("body"));
    return MILTER_STATUS_DEFAULT;
}

static MilterStatus
real_end_of_message (MilterManagerController *controller)
{
     rb_p(rb_str_new2("end-of-body"));
    return MILTER_STATUS_DEFAULT;
}

static MilterStatus
real_close (MilterManagerController *controller)
{
     rb_p(rb_str_new2("close"));
    return MILTER_STATUS_DEFAULT;
}

static MilterStatus
real_abort (MilterManagerController *controller)
{
     rb_p(rb_str_new2("abort"));
    return MILTER_STATUS_DEFAULT;
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
