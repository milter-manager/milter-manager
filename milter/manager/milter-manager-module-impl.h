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

#ifndef __MILTER_MANAGER_MODULE_IMPL_H__
#define __MILTER_MANAGER_MODULE_IMPL_H__

#include <glib-object.h>

G_BEGIN_DECLS

#include <milter/manager/milter-manager-module.h>

typedef GList   *(*MilterManagerModuleInitFunc)         (GTypeModule *module);
typedef void     (*MilterManagerModuleExitFunc)         (void);
typedef GObject *(*MilterManagerModuleInstantiateFunc)  (const gchar *first_property,
                                                         va_list      var_args);

#define MILTER_MANAGER_MODULE_IMPL_INIT           milter_manager_module_impl_init
#define MILTER_MANAGER_MODULE_IMPL_EXIT           milter_manager_module_impl_exit
#define MILTER_MANAGER_MODULE_IMPL_INSTANTIATE    milter_manager_module_impl_instantiate

GList   *MILTER_MANAGER_MODULE_IMPL_INIT           (GTypeModule  *module);
void     MILTER_MANAGER_MODULE_IMPL_EXIT           (void);
GObject *MILTER_MANAGER_MODULE_IMPL_INSTANTIATE    (const gchar *first_property,
                                                    va_list      var_args);

G_END_DECLS

#endif /* __MILTER_MANAGER_MODULE_IMPL_H__ */

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
