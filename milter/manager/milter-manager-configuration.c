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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <glib/gstdio.h>

#include "milter-manager-module.h"
#include "milter-manager-configuration.h"
#include "milter-manager-children.h"
#include <milter/core/milter-marshalers.h>

#define MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(obj)                   \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj),                                 \
                                 MILTER_TYPE_MANAGER_CONFIGURATION,     \
                                 MilterManagerConfigurationPrivate))

typedef struct _MilterManagerConfigurationPrivate MilterManagerConfigurationPrivate;
struct _MilterManagerConfigurationPrivate
{
    GList *load_paths;
    GList *eggs;
    gboolean privilege_mode;
    gchar *control_connection_spec;
    gchar *manager_connection_spec;
    MilterStatus return_status;
};

enum
{
    PROP_0,
    PROP_PRIVILEGE_MODE,
    PROP_CONTROL_CONNECTION_SPEC,
    PROP_MANAGER_CONNECTION_SPEC,
    PROP_RETURN_STATUS,
};

enum
{
    TO_XML,
    LAST_SIGNAL
};

static gint signals[LAST_SIGNAL] = {0};

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

    spec = g_param_spec_string("control-connection-spec",
                               "Control connection spec",
                               "The control connection spec "
                               "of the milter-manager",
                               NULL,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
    g_object_class_install_property(gobject_class, PROP_CONTROL_CONNECTION_SPEC,
                                    spec);

    spec = g_param_spec_string("manager-connection-spec",
                               "Manager connection spec",
                               "The manager connection spec "
                               "of the milter-manager",
                               NULL,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
    g_object_class_install_property(gobject_class, PROP_MANAGER_CONNECTION_SPEC,
                                    spec);

    spec = g_param_spec_enum("return-status",
                             "Return status",
                             "The return status of the milter-manager",
                             MILTER_TYPE_STATUS,
                             MILTER_STATUS_CONTINUE,
                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
    g_object_class_install_property(gobject_class, PROP_RETURN_STATUS, spec);

    signals[TO_XML] =
        g_signal_new("to-xml",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterManagerConfigurationClass, to_xml),
                     NULL, NULL,
                     _milter_marshal_VOID__POINTER_UINT,
                     G_TYPE_NONE, 2, G_TYPE_POINTER, G_TYPE_UINT);

    g_type_class_add_private(gobject_class,
                             sizeof(MilterManagerConfigurationPrivate));
}

static void
milter_manager_configuration_init (MilterManagerConfiguration *configuration)
{
    MilterManagerConfigurationPrivate *priv;
    const gchar *config_dir_env;

    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration);
    priv->load_paths = NULL;
    priv->eggs = NULL;
    priv->control_connection_spec = NULL;
    priv->manager_connection_spec = NULL;

    config_dir_env = g_getenv("MILTER_MANAGER_CONFIG_DIR");
    if (config_dir_env)
        priv->load_paths = g_list_append(priv->load_paths,
                                         g_strdup(config_dir_env));
    priv->load_paths = g_list_append(priv->load_paths, g_strdup(CONFIG_DIR));

    milter_manager_configuration_clear(configuration);
}

static void
dispose (GObject *object)
{
    MilterManagerConfiguration *configuration;
    MilterManagerConfigurationPrivate *priv;

    configuration = MILTER_MANAGER_CONFIGURATION(object);
    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration);

    milter_manager_configuration_clear_load_paths(configuration);
    milter_manager_configuration_clear(configuration);

    G_OBJECT_CLASS(milter_manager_configuration_parent_class)->dispose(object);
}

static void
set_property (GObject      *object,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
    MilterManagerConfiguration *config;

    config = MILTER_MANAGER_CONFIGURATION(object);
    switch (prop_id) {
      case PROP_PRIVILEGE_MODE:
        milter_manager_configuration_set_privilege_mode(
            config, g_value_get_boolean(value));
        break;
      case PROP_CONTROL_CONNECTION_SPEC:
        milter_manager_configuration_set_control_connection_spec(
            config, g_value_get_string(value));
        break;
      case PROP_MANAGER_CONNECTION_SPEC:
        milter_manager_configuration_set_manager_connection_spec(
            config, g_value_get_string(value));
        break;
      case PROP_RETURN_STATUS:
        milter_manager_configuration_set_return_status_if_filter_unavailable(
            config, g_value_get_enum(value));
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
      case PROP_CONTROL_CONNECTION_SPEC:
        g_value_set_string(value, priv->control_connection_spec);
        break;
      case PROP_MANAGER_CONNECTION_SPEC:
        g_value_set_string(value, priv->manager_connection_spec);
        break;
      case PROP_RETURN_STATUS:
        g_value_set_enum(value, priv->return_status);
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

GQuark
milter_manager_configuration_error_quark (void)
{
    return g_quark_from_static_string("milter-manager-configuration-error-quark");
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
milter_manager_configuration_append_load_path (MilterManagerConfiguration *configuration,
                                               const gchar *path)
{
    MilterManagerConfigurationPrivate *priv;

    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration);
    priv->load_paths = g_list_append(priv->load_paths, g_strdup(path));
}

void
milter_manager_configuration_prepend_load_path (MilterManagerConfiguration *configuration,
                                                const gchar *path)
{
    MilterManagerConfigurationPrivate *priv;

    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration);
    priv->load_paths = g_list_prepend(priv->load_paths, g_strdup(path));
}

void
milter_manager_configuration_clear_load_paths (MilterManagerConfiguration *configuration)
{
    MilterManagerConfigurationPrivate *priv;

    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration);
    if (priv->load_paths) {
        g_list_foreach(priv->load_paths, (GFunc)g_free, NULL);
        g_list_free(priv->load_paths);
        priv->load_paths = NULL;
    }
}

static gchar *
find_file (MilterManagerConfiguration *configuration,
           const gchar *file_name, GError **error)
{
    MilterManagerConfigurationPrivate *priv;
    gchar *full_path = NULL;
    GList *node;

    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration);
    for (node = priv->load_paths; node; node = g_list_next(node)) {
        const gchar *dir = node->data;

        full_path = g_build_filename(dir, file_name, NULL);
        if (g_file_test(full_path, G_FILE_TEST_IS_REGULAR)) {
            break;
        } else {
            g_free(full_path);
            full_path = NULL;
        }
    }

    if (!full_path) {
        GString *inspected_load_paths;

        inspected_load_paths = g_string_new("[");
        for (node = priv->load_paths; node; node = g_list_next(node)) {
            const gchar *dir = node->data;

            g_string_append_printf(inspected_load_paths, "\"%s\"", dir);
            if (g_list_next(node))
                g_string_append(inspected_load_paths, ", ");
        }
        g_string_append(inspected_load_paths, "]");
        g_set_error(error,
                    MILTER_MANAGER_CONFIGURATION_ERROR,
                    MILTER_MANAGER_CONFIGURATION_ERROR_NOT_EXIST,
                    "path %s doesn't exist in load path (%s)",
                    file_name, inspected_load_paths->str);
        g_string_free(inspected_load_paths, TRUE);
    }

    return full_path;
}

gboolean
milter_manager_configuration_load (MilterManagerConfiguration *configuration,
                                   const gchar *file_name, GError **error)
{
    MilterManagerConfigurationClass *configuration_class;

    configuration_class = MILTER_MANAGER_CONFIGURATION_GET_CLASS(configuration);
    if (configuration_class->load) {
        gboolean success;
        gchar *full_path;

        full_path = find_file(configuration, file_name, error);
        if (!full_path)
            return FALSE;

        success = configuration_class->load(configuration, full_path, error);
        g_free(full_path);
        return success;
    } else {
        g_set_error(error,
                    MILTER_MANAGER_CONFIGURATION_ERROR,
                    MILTER_MANAGER_CONFIGURATION_ERROR_NOT_IMPLEMENTED,
                    "load isn't implemented");
        return FALSE;
    }
}

gboolean
milter_manager_configuration_load_custom (MilterManagerConfiguration *configuration,
                                          const gchar *file_name, GError **error)
{
    MilterManagerConfigurationClass *configuration_class;

    configuration_class = MILTER_MANAGER_CONFIGURATION_GET_CLASS(configuration);
    if (configuration_class->load_custom) {
        gboolean success;
        gchar *full_path;

        full_path = find_file(configuration, file_name, error);
        if (!full_path)
            return FALSE;

        success = configuration_class->load_custom(configuration,
                                                   full_path,
                                                   error);
        g_free(full_path);
        return success;
    } else {
        g_set_error(error,
                    MILTER_MANAGER_CONFIGURATION_ERROR,
                    MILTER_MANAGER_CONFIGURATION_ERROR_NOT_IMPLEMENTED,
                    "load_custom isn't implemented");
        return FALSE;
    }
}

gboolean
milter_manager_configuration_load_if_exist (MilterManagerConfiguration *configuration,
                                            const gchar *file_name,
                                            GError **error)
{
    GError *local_error = NULL;

    if (milter_manager_configuration_load(configuration, file_name,
                                          &local_error))
        return TRUE;

    if (local_error->code == MILTER_MANAGER_CONFIGURATION_ERROR_NOT_EXIST) {
        g_error_free(local_error);
        return TRUE;
    }

    g_propagate_error(error, local_error);
    return FALSE;
}

gboolean
milter_manager_configuration_load_custom_if_exist (MilterManagerConfiguration *configuration,
                                                   const gchar *file_name,
                                                   GError **error)
{
    GError *local_error = NULL;

    if (milter_manager_configuration_load_custom(configuration, file_name,
                                                 &local_error))
        return TRUE;

    if (local_error->code == MILTER_MANAGER_CONFIGURATION_ERROR_NOT_EXIST) {
        g_error_free(local_error);
        return TRUE;
    }

    g_propagate_error(error, local_error);
    return FALSE;
}

void
milter_manager_configuration_reload (MilterManagerConfiguration *configuration)
{
    GError *error = NULL;

    milter_manager_configuration_clear(configuration);
    if (!milter_manager_configuration_load(configuration, CONFIG_FILE_NAME,
                                           &error)) {
        milter_error("failed to load config file: <%s>: %s",
                     CONFIG_FILE_NAME, error->message);
        g_error_free(error);
        return;
    }

    if (!milter_manager_configuration_load_custom_if_exist(
            configuration, CUSTOM_CONFIG_FILE_NAME, &error)) {
        milter_error("failed to load custom config file: <%s>: %s",
                     CUSTOM_CONFIG_FILE_NAME, error->message);
        g_error_free(error);
    }
}

gboolean
milter_manager_configuration_save_custom (MilterManagerConfiguration *configuration,
                                          const gchar                *content,
                                          gssize                      size,
                                          GError                    **error)
{
    MilterManagerConfigurationPrivate *priv;
    GError *local_error = NULL;
    GList *node;
    gboolean success = FALSE;
    GString *inspected_tried_paths;

    inspected_tried_paths = g_string_new("[");
    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration);
    for (node = priv->load_paths; node; node = g_list_next(node)) {
        const gchar *dir = node->data;
        gchar *full_path;

        full_path = g_build_filename(dir, CUSTOM_CONFIG_FILE_NAME, NULL);
        g_string_append_printf(inspected_tried_paths, "\"%s\"", full_path);
        if (g_file_set_contents(full_path, content, size, &local_error)) {
            success = TRUE;
        } else {
            g_string_append_printf(inspected_tried_paths, "[%s]",
                                   local_error->message);
            g_error_free(local_error);
            local_error = NULL;
            if (g_list_next(node))
                g_string_append(inspected_tried_paths, ", ");
        }
        g_free(full_path);
        if (success)
            break;
    }
    g_string_append(inspected_tried_paths, "]");

    if (!success) {
        g_set_error(error,
                    MILTER_MANAGER_CONFIGURATION_ERROR,
                    MILTER_MANAGER_CONFIGURATION_ERROR_SAVE,
                    "can't find writable custom config file in %s",
                    inspected_tried_paths->str);
        g_string_free(inspected_tried_paths, TRUE);
    }

    return success;
}

gboolean
milter_manager_configuration_is_privilege_mode (MilterManagerConfiguration *configuration)
{
    return MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration)->privilege_mode;
}

void
milter_manager_configuration_set_privilege_mode (MilterManagerConfiguration *configuration,
                                                 gboolean mode)
{
    MilterManagerConfigurationPrivate *priv;

    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration);
    priv->privilege_mode = mode;
}

const gchar *
milter_manager_configuration_get_control_connection_spec (MilterManagerConfiguration *configuration)
{
    return MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration)->control_connection_spec;
}

void
milter_manager_configuration_set_control_connection_spec (MilterManagerConfiguration *configuration,
                                                          const gchar *spec)
{
    MilterManagerConfigurationPrivate *priv;

    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration);
    if (priv->control_connection_spec)
        g_free(priv->control_connection_spec);
    priv->control_connection_spec = g_strdup(spec);
}

const gchar *
milter_manager_configuration_get_manager_connection_spec (MilterManagerConfiguration *configuration)
{
    return MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration)->manager_connection_spec;
}

void
milter_manager_configuration_set_manager_connection_spec (MilterManagerConfiguration *configuration,
                                                          const gchar *spec)
{
    MilterManagerConfigurationPrivate *priv;

    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration);
    if (priv->manager_connection_spec)
        g_free(priv->manager_connection_spec);
    priv->manager_connection_spec = g_strdup(spec);
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

void
milter_manager_configuration_set_return_status_if_filter_unavailable
                                     (MilterManagerConfiguration *configuration,
                                      MilterStatus status)
{
    MilterManagerConfigurationPrivate *priv;

    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration);
    priv->return_status = status;
}

void
milter_manager_configuration_clear (MilterManagerConfiguration *configuration)
{
    MilterManagerConfigurationPrivate *priv;

    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration);

    if (priv->eggs) {
        g_list_foreach(priv->eggs, (GFunc)g_object_unref, NULL);
        g_list_free(priv->eggs);
        priv->eggs = NULL;
    }

    if (priv->control_connection_spec) {
        g_free(priv->control_connection_spec);
        priv->control_connection_spec = NULL;
    }

    if (priv->manager_connection_spec) {
        g_free(priv->manager_connection_spec);
        priv->manager_connection_spec = NULL;
    }

    priv->privilege_mode = FALSE;
    priv->return_status = MILTER_STATUS_CONTINUE;
}

gchar *
milter_manager_configuration_to_xml (MilterManagerConfiguration *configuration)
{
    GString *string;

    string = g_string_new(NULL);
    milter_manager_configuration_to_xml_string(configuration, string, 0);
    return g_string_free(string, FALSE);
}

void
milter_manager_configuration_to_xml_string (MilterManagerConfiguration *configuration,
                                            GString *string, guint indent)
{
    MilterManagerConfigurationPrivate *priv;
    gchar *full_path;

    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration);

    milter_utils_append_indent(string, indent);
    g_string_append(string, "<configuration>\n");

    full_path = find_file(configuration, CUSTOM_CONFIG_FILE_NAME, NULL);
    if (full_path) {
        struct stat status;
        if (g_stat(full_path, &status) == 0) {
            gchar *content;

            content = g_strdup_printf("%ld", status.st_mtime);
            milter_utils_xml_append_text_element(string,
                                                 "last-modified", content,
                                                 indent + 2);
        }
        g_free(full_path);
    }

    if (priv->eggs) {
        GList *node;

        milter_utils_append_indent(string, indent + 2);
        g_string_append(string, "<milters>\n");
        for (node = priv->eggs; node; node = g_list_next(node)) {
            MilterManagerEgg *egg = node->data;
            milter_manager_egg_to_xml_string(egg, string, indent + 2 + 2);
        }
        milter_utils_append_indent(string, indent + 2);
        g_string_append(string, "</milters>\n");
    }

    g_signal_emit(configuration, signals[TO_XML], 0, string, indent + 2);

    milter_utils_append_indent(string, indent);
    g_string_append(string, "</configuration>\n");
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
