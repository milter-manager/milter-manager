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
#include "milter-manager-controller.h"

static GList *controllers = NULL;
static gchar *controller_module_dir = NULL;

static const gchar *
_milter_manager_controller_module_dir (void)
{
    const gchar *dir;

    if (controller_module_dir)
        return controller_module_dir;

    dir = g_getenv("MILTER_MANAGER_CONTROLLER_MODULE_DIR");
    if (dir)
        return dir;

    return CONTROLLER_MODULE_DIR;
}

static MilterManagerModule *
milter_manager_controller_load_module (const gchar *name)
{
    gchar *module_name;
    MilterManagerModule *module;

    module_name = g_strdup_printf("milter-manager-%s-controller", name);
    module = milter_manager_module_find(controllers, module_name);
    if (!module) {
        const gchar *module_dir;

        module_dir = _milter_manager_controller_module_dir();
        module = milter_manager_module_load_module(module_dir, module_name);
        if (module) {
            if (g_type_module_use(G_TYPE_MODULE(module))) {
                controllers = g_list_prepend(controllers, module);
            }
        }
    }

    g_free(module_name);
    return module;
}

void
_milter_manager_controller_init (void)
{
}

static void
milter_manager_controller_set_default_module_dir (const gchar *dir)
{
    if (controller_module_dir)
        g_free(controller_module_dir);
    controller_module_dir = NULL;

    if (dir)
        controller_module_dir = g_strdup(dir);
}

static void
milter_manager_controller_unload (void)
{
    g_list_foreach(controllers, (GFunc)milter_manager_module_unload, NULL);
    g_list_free(controllers);
    controllers = NULL;
}

void
_milter_manager_controller_quit (void)
{
    milter_manager_controller_unload();
    milter_manager_controller_set_default_module_dir(NULL);
}

GType
milter_manager_controller_get_type (void)
{
    static GType controller_type = 0;

    if (!controller_type) {
        const GTypeInfo controller_info = {
            sizeof(MilterManagerControllerClass), /* class_size */
            NULL,           	                  /* base_init */
            NULL,			          /* base_finalize */
        };

        controller_type = g_type_register_static(G_TYPE_INTERFACE,
                                                 "MilterManagerController",
                                                 &controller_info, 0);
    }

    return controller_type;
}

MilterManagerController *
milter_manager_controller_new (const gchar *name,
                               const gchar *first_property, ...)
{
    MilterManagerModule *module;
    GObject *controller;
    va_list var_args;

    module = milter_manager_controller_load_module(name);
    g_return_val_if_fail(module != NULL, NULL);

    va_start(var_args, first_property);
    controller = milter_manager_module_instantiate(module,
                                                   first_property, var_args);
    va_end(var_args);

    return MILTER_MANAGER_CONTROLLER(controller);
}

void
milter_manager_controller_add_load_path (MilterManagerController *controller,
                                         const gchar *path)
{
    MilterManagerControllerClass *controller_class;

    controller_class = MILTER_MANAGER_CONTROLLER_GET_CLASS(controller);
    if (controller_class->add_load_path)
        controller_class->add_load_path(controller, path);
}

void
milter_manager_controller_load (MilterManagerController *controller,
                                const gchar *file_name)
{
    MilterManagerControllerClass *controller_class;

    controller_class = MILTER_MANAGER_CONTROLLER_GET_CLASS(controller);
    if (controller_class->load)
        controller_class->load(controller, file_name);
}

MilterStatus
milter_manager_controller_negotiate (MilterManagerController *controller,
                                     MilterOption         *option)
{
    MilterManagerControllerClass *controller_class;

    controller_class = MILTER_MANAGER_CONTROLLER_GET_CLASS(controller);
    if (controller_class->negotiate)
        return controller_class->negotiate(controller, option);
    return MILTER_STATUS_DEFAULT;
}

MilterStatus
milter_manager_controller_connect (MilterManagerController *controller,
                                   const gchar          *host_name,
                                   struct sockaddr      *address,
                                   socklen_t             address_length)
{
    MilterManagerControllerClass *controller_class;

    controller_class = MILTER_MANAGER_CONTROLLER_GET_CLASS(controller);
    if (controller_class->connect)
        controller_class->connect(controller, host_name,
                                  address, address_length);
    return MILTER_STATUS_DEFAULT;
}

MilterStatus
milter_manager_controller_helo (MilterManagerController *controller,
                                const gchar          *fqdn)
{
    MilterManagerControllerClass *controller_class;

    controller_class = MILTER_MANAGER_CONTROLLER_GET_CLASS(controller);
    if (controller_class->helo)
        return controller_class->helo(controller, fqdn);
    return MILTER_STATUS_DEFAULT;
}

MilterStatus
milter_manager_controller_envelope_from (MilterManagerController *controller,
                                         const gchar          *from)
{
    MilterManagerControllerClass *controller_class;

    controller_class = MILTER_MANAGER_CONTROLLER_GET_CLASS(controller);
    if (controller_class->envelope_from)
        return controller_class->envelope_from(controller, from);
    return MILTER_STATUS_DEFAULT;
}

MilterStatus
milter_manager_controller_envelope_receipt (MilterManagerController *controller,
                                            const gchar          *receipt)
{
    MilterManagerControllerClass *controller_class;

    controller_class = MILTER_MANAGER_CONTROLLER_GET_CLASS(controller);
    if (controller_class->envelope_receipt)
        return controller_class->envelope_receipt(controller, receipt);
    return MILTER_STATUS_DEFAULT;
}

MilterStatus
milter_manager_controller_data (MilterManagerController *controller)
{
    MilterManagerControllerClass *controller_class;

    controller_class = MILTER_MANAGER_CONTROLLER_GET_CLASS(controller);
    if (controller_class->data)
        return controller_class->data(controller);
    return MILTER_STATUS_DEFAULT;
}

MilterStatus
milter_manager_controller_unknown (MilterManagerController *controller,
                                   const gchar          *command)
{
    MilterManagerControllerClass *controller_class;

    controller_class = MILTER_MANAGER_CONTROLLER_GET_CLASS(controller);
    if (controller_class->unknown)
        return controller_class->unknown(controller, command);
    return MILTER_STATUS_DEFAULT;
}

MilterStatus
milter_manager_controller_header (MilterManagerController *controller,
                                  const gchar          *name,
                                  const gchar          *value)
{
    MilterManagerControllerClass *controller_class;

    controller_class = MILTER_MANAGER_CONTROLLER_GET_CLASS(controller);
    if (controller_class->header)
        return controller_class->header(controller, name, value);
    return MILTER_STATUS_DEFAULT;
}

MilterStatus
milter_manager_controller_end_of_header (MilterManagerController *controller)
{
    MilterManagerControllerClass *controller_class;

    controller_class = MILTER_MANAGER_CONTROLLER_GET_CLASS(controller);
    if (controller_class->end_of_header)
        return controller_class->end_of_header(controller);
    return MILTER_STATUS_DEFAULT;
}

MilterStatus
milter_manager_controller_body (MilterManagerController *controller,
                                const guchar         *chunk,
                                gsize                 size)
{
    MilterManagerControllerClass *controller_class;

    controller_class = MILTER_MANAGER_CONTROLLER_GET_CLASS(controller);
    if (controller_class->body)
        return controller_class->body(controller, chunk, size);
    return MILTER_STATUS_DEFAULT;
}

MilterStatus
milter_manager_controller_end_of_message (MilterManagerController *controller)
{
    MilterManagerControllerClass *controller_class;

    controller_class = MILTER_MANAGER_CONTROLLER_GET_CLASS(controller);
    if (controller_class->end_of_message)
        return controller_class->end_of_message(controller);
    return MILTER_STATUS_DEFAULT;
}

MilterStatus
milter_manager_controller_quit (MilterManagerController *controller)
{
    MilterManagerControllerClass *controller_class;

    controller_class = MILTER_MANAGER_CONTROLLER_GET_CLASS(controller);
    if (controller_class->quit)
        return controller_class->quit(controller);
    return MILTER_STATUS_DEFAULT;
}

MilterStatus
milter_manager_controller_abort (MilterManagerController *controller)
{
    MilterManagerControllerClass *controller_class;

    controller_class = MILTER_MANAGER_CONTROLLER_GET_CLASS(controller);
    if (controller_class->abort)
        return controller_class->abort(controller);
    return MILTER_STATUS_DEFAULT;
}

void
milter_manager_controller_mta_timeout (MilterManagerController *controller)
{
    MilterManagerControllerClass *controller_class;

    controller_class = MILTER_MANAGER_CONTROLLER_GET_CLASS(controller);
    if (controller_class->mta_timeout)
        controller_class->mta_timeout(controller);
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
