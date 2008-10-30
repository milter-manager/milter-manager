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

#ifdef HAVE_CONFIG_H
#  include "../../config.h"
#endif /* HAVE_CONFIG_H */

#include "milter-manager-module.h"
#include "milter-manager-configuration.h"
#include "milter-manager-children.h"

#define MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(obj)                   \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj),                                 \
                                 MILTER_TYPE_MANAGER_CONFIGURATION,     \
                                 MilterManagerConfigurationPrivate))

typedef struct _MilterManagerConfigurationPrivate MilterManagerConfigurationPrivate;
struct _MilterManagerConfigurationPrivate
{
    GList *eggs;
    gboolean privilege_mode;
    MilterStatus return_status;
};

enum
{
    PROP_0,
    PROP_PRIVILEGE_MODE
};

static GList *configurations = NULL;
static gchar *configuration_module_dir = NULL;

G_DEFINE_TYPE(MilterManagerConfiguration, milter_manager_configuration,
              G_TYPE_OBJECT);

static void dispose        (GObject         *object);
static void set_property   (GObject         *object,
                            guint            prop_id,
                            const GValue    *value,
                            GParamSpec      *pspec);
static void get_property   (GObject         *object,
                            guint            prop_id,
                            GValue          *value,
                            GParamSpec      *pspec);

static void
milter_manager_configuration_class_init (MilterManagerConfigurationClass *klass)
{
    GObjectClass *gobject_class;
    GParamSpec *spec;

    gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->dispose      = dispose;
    gobject_class->set_property = set_property;
    gobject_class->get_property = get_property;

    spec = g_param_spec_boolean("privilege-mode",
                                "Privilege mode",
                                "Whether MilterManager runs on privilege mode",
                                FALSE,
                                G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
    g_object_class_install_property(gobject_class, PROP_PRIVILEGE_MODE, spec);

    g_type_class_add_private(gobject_class,
                             sizeof(MilterManagerConfigurationPrivate));
}

static void
milter_manager_configuration_init (MilterManagerConfiguration *configuration)
{
    MilterManagerConfigurationPrivate *priv;

    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration);
    priv->eggs = NULL;
    priv->privilege_mode = FALSE;
    priv->return_status = MILTER_STATUS_CONTINUE;
}

static void
dispose (GObject *object)
{
    MilterManagerConfigurationPrivate *priv;

    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(object);

    if (priv->eggs) {
        g_list_foreach(priv->eggs, (GFunc)g_object_unref, NULL);
        g_list_free(priv->eggs);
        priv->eggs = NULL;
    }

    G_OBJECT_CLASS(milter_manager_configuration_parent_class)->dispose(object);
}

static void
set_property (GObject      *object,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
    MilterManagerConfigurationPrivate *priv;

    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(object);
    switch (prop_id) {
      case PROP_PRIVILEGE_MODE:
        priv->privilege_mode = g_value_get_boolean(value);
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
    MilterManagerConfigurationPrivate *priv;

    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(object);
    switch (prop_id) {
      case PROP_PRIVILEGE_MODE:
        g_value_set_boolean(value, priv->privilege_mode);
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static const gchar *
_milter_manager_configuration_module_dir (void)
{
    const gchar *dir;

    if (configuration_module_dir)
        return configuration_module_dir;

    dir = g_getenv("MILTER_MANAGER_CONFIGURATION_MODULE_DIR");
    if (dir)
        return dir;

    return CONFIGURATION_MODULE_DIR;
}

static MilterManagerModule *
milter_manager_configuration_load_module (const gchar *name)
{
    gchar *module_name;
    MilterManagerModule *module;

    module_name = g_strdup_printf("milter-manager-%s-configuration", name);
    module = milter_manager_module_find(configurations, module_name);
    if (!module) {
        const gchar *module_dir;

        module_dir = _milter_manager_configuration_module_dir();
        module = milter_manager_module_load_module(module_dir, module_name);
        if (module) {
            if (g_type_module_use(G_TYPE_MODULE(module))) {
                configurations = g_list_prepend(configurations, module);
            }
        }
    }

    g_free(module_name);
    return module;
}

void
_milter_manager_configuration_init (void)
{
}

static void
milter_manager_configuration_set_default_module_dir (const gchar *dir)
{
    if (configuration_module_dir)
        g_free(configuration_module_dir);
    configuration_module_dir = NULL;

    if (dir)
        configuration_module_dir = g_strdup(dir);
}

static void
milter_manager_configuration_unload (void)
{
    g_list_foreach(configurations, (GFunc)milter_manager_module_unload, NULL);
    g_list_free(configurations);
    configurations = NULL;
}

void
_milter_manager_configuration_quit (void)
{
    milter_manager_configuration_unload();
    milter_manager_configuration_set_default_module_dir(NULL);
}

MilterManagerConfiguration *
milter_manager_configuration_new (const gchar *first_property,
                                  ...)
{
    MilterManagerModule *module;
    GObject *configuration;
    va_list var_args;

    module = milter_manager_configuration_load_module("ruby");
    g_return_val_if_fail(module != NULL, NULL);

    va_start(var_args, first_property);
    configuration = milter_manager_module_instantiate(module,
                                                      first_property, var_args);
    va_end(var_args);

    return MILTER_MANAGER_CONFIGURATION(configuration);
}

void
milter_manager_configuration_add_load_path (MilterManagerConfiguration *configuration,
                                            const gchar *path)
{
    MilterManagerConfigurationClass *configuration_class;

    configuration_class = MILTER_MANAGER_CONFIGURATION_GET_CLASS(configuration);
    if (configuration_class->add_load_path)
        configuration_class->add_load_path(configuration, path);
}

void
milter_manager_configuration_load (MilterManagerConfiguration *configuration,
                                   const gchar *file_name)
{
    MilterManagerConfigurationClass *configuration_class;

    configuration_class = MILTER_MANAGER_CONFIGURATION_GET_CLASS(configuration);
    if (configuration_class->load)
        configuration_class->load(configuration, file_name);
}

gboolean
milter_manager_configuration_is_privilege_mode (MilterManagerConfiguration *configuration)
{
    return MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration)->privilege_mode;
}

void
milter_manager_configuration_add_egg (MilterManagerConfiguration *configuration,
                                      MilterManagerEgg         *egg)
{
    MilterManagerConfigurationPrivate *priv;

    g_return_if_fail(egg != NULL);

    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration);

    g_object_ref(egg);
    priv->eggs = g_list_append(priv->eggs, egg);
}

MilterManagerChildren *
milter_manager_configuration_create_children (MilterManagerConfiguration *configuration)
{
    GList *node;
    MilterManagerChildren *children;
    MilterManagerConfigurationPrivate *priv;

    children = milter_manager_children_new(configuration);
    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration);

    for (node = priv->eggs; node; node = g_list_next(node)) {
        MilterManagerChild *child;
        MilterManagerEgg *egg = node->data;

        child = milter_manager_egg_hatch(egg);
        if (child) {
            milter_manager_children_add_child(children, child);
            g_object_unref(child);
        }
    }

    return children;
}

MilterStatus
milter_manager_configuration_get_return_status_if_filter_unavailable
                                     (MilterManagerConfiguration *configuration)
{
    return MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration)->return_status;
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
