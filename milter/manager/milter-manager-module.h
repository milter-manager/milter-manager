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
 *  along with this library.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef __MILTER_MANAGER_MODULE_H__
#define __MILTER_MANAGER_MODULE_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define MILTER_TYPE_MANAGER_MODULE            (milter_manager_module_get_type ())
#define MILTER_MANAGER_MODULE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MILTER_TYPE_MANAGER_MODULE, MilterManagerModule))
#define MILTER_MANAGER_MODULE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MILTER_TYPE_MANAGER_MODULE, MilterManagerModuleClass))
#define MILTER_MANAGER_IS_MODULE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MILTER_TYPE_MANAGER_MODULE))
#define MILTER_MANAGER_IS_MODULE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MILTER_TYPE_MANAGER_MODULE))
#define MILTER_MANAGER_MODULE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), MILTER_TYPE_MANAGER_MODULE, MilterManagerModuleClass))

typedef struct _MilterManagerModule MilterManagerModule;
typedef struct _MilterManagerModuleClass MilterManagerModuleClass;

struct _MilterManagerModule
{
    GTypeModule object;
};

struct _MilterManagerModuleClass
{
    GTypeModuleClass parent_class;
};

GType                milter_manager_module_get_type    (void) G_GNUC_CONST;

MilterManagerModule *milter_manager_module_load_module (const gchar    *base_dir,
                                                        const gchar    *name);
GList               *milter_manager_module_load_modules(const gchar    *base_dir);
GList               *milter_manager_module_load_modules_unique
                                                       (const gchar    *base_dir,
                                                        GList          *modules);
MilterManagerModule *milter_manager_module_find        (GList          *modules,
                                                        const gchar    *name);

GObject             *milter_manager_module_instantiate (MilterManagerModule *module,
                                                        const gchar    *first_property,
                                                        va_list         var_args);
GList               *milter_manager_module_collect_registered_types
                                                       (GList *modules);
GList               *milter_manager_module_collect_names
                                                       (GList *modules);
void                 milter_manager_module_unload      (MilterManagerModule *module);

G_END_DECLS

#endif /* __MILTER_MANAGER_MODULE_H__ */

/*
vi:nowrap:ai:expandtab:sw=4
*/
