/* GObject - GLib Type, Object, Parameter and Signal Library
 * Copyright (C) 1998-1999, 2000-2001 Tim Janik and Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __G_OBJECT_COMPATIBLE_H__
#define __G_OBJECT_COMPATIBLE_H__

#include        <glib.h>

G_BEGIN_DECLS

/**
 * G_DEFINE_BOXED_TYPE:
 * @TypeName: The name of the new type, in Camel case.
 * @type_name: The name of the new type, in lowercase, with words
 *  separated by '_'.
 * @copy_func: the #GBoxedCopyFunc for the new type
 * @free_func: the #GBoxedFreeFunc for the new type
 *
 * A convenience macro for boxed type implementations, which defines a
 * type_name_get_type() function registering the boxed type.
 *
 * Since: 2.26
 */
#define G_DEFINE_BOXED_TYPE(TypeName, type_name, copy_func, free_func) G_DEFINE_BOXED_TYPE_WITH_CODE (TypeName, type_name, copy_func, free_func, {})
/**
 * G_DEFINE_BOXED_TYPE_WITH_CODE:
 * @TypeName: The name of the new type, in Camel case.
 * @type_name: The name of the new type, in lowercase, with words
 *  separated by '_'.
 * @copy_func: the #GBoxedCopyFunc for the new type
 * @free_func: the #GBoxedFreeFunc for the new type
 * @_C_: Custom code that gets inserted in the *_get_type() function.
 *
 * A convenience macro for boxed type implementations.
 * Similar to G_DEFINE_BOXED_TYPE(), but allows to insert custom code into the
 * type_name_get_type() function, e.g. to register value transformations with
 * g_value_register_transform_func().
 *
 * Since: 2.26
 */
#define G_DEFINE_BOXED_TYPE_WITH_CODE(TypeName, type_name, copy_func, free_func, _C_) _G_DEFINE_BOXED_TYPE_BEGIN (TypeName, type_name, copy_func, free_func) {_C_;} _G_DEFINE_TYPE_EXTENDED_END()

#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 7)
#define _G_DEFINE_BOXED_TYPE_BEGIN(TypeName, type_name, copy_func, free_func) \
GType \
type_name##_get_type (void) \
{ \
  static volatile gsize g_define_type_id__volatile = 0; \
  if (g_once_init_enter (&g_define_type_id__volatile))  \
    { \
      GType (* _g_register_boxed) \
        (const gchar *, \
         union \
           { \
             TypeName * (*do_copy_type) (TypeName *); \
             TypeName * (*do_const_copy_type) (const TypeName *); \
             GBoxedCopyFunc do_copy_boxed; \
           } __attribute__((__transparent_union__)), \
         union \
           { \
             void (* do_free_type) (TypeName *); \
             GBoxedFreeFunc do_free_boxed; \
           } __attribute__((__transparent_union__)) \
        ) = g_boxed_type_register_static; \
      GType g_define_type_id = \
        _g_register_boxed (g_intern_static_string (#TypeName), copy_func, free_func); \
      { /* custom code follows */
#else
#define _G_DEFINE_BOXED_TYPE_BEGIN(TypeName, type_name, copy_func, free_func) \
GType \
type_name##_get_type (void) \
{ \
  static volatile gsize g_define_type_id__volatile = 0; \
  if (g_once_init_enter (&g_define_type_id__volatile))  \
    { \
      GType g_define_type_id = \
        g_boxed_type_register_static (g_intern_static_string (#TypeName), \
                                      (GBoxedCopyFunc) copy_func, \
                                      (GBoxedFreeFunc) free_func); \
      { /* custom code follows */
#endif /* __GNUC__ */

G_END_DECLS

#endif /* __G_OBJECT_COMPATIBLE_H__ */
