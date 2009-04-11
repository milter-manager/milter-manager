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

#ifndef __MILTER_ERROR_EMITTABLE_H__
#define __MILTER_ERROR_EMITTABLE_H__

#include <glib-object.h>

G_BEGIN_DECLS

/**
 * SECTION: milter-error-emittable
 * @title: MilterErrorEmittable
 * @short_description: An interface for "error" signal.
 *
 * The %MilterErrorEmittable interface provides
 * #MilterErrorEmittable::error
 * signal. #MilterErrorEmittable::error signal will be
 * emitted on error.
 */

#define MILTER_TYPE_ERROR_EMITTABLE             (milter_error_emittable_get_type ())
#define MILTER_ERROR_EMITTABLE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), MILTER_TYPE_ERROR_EMITTABLE, MilterErrorEmittable))
#define MILTER_ERROR_EMITTABLE_CLASS(vtable)    (G_TYPE_CHECK_CLASS_CAST ((vtable), MILTER_TYPE_ERROR_EMITTABLE, MilterErrorEmittableClass))
#define MILTER_IS_ERROR_EMITTABLE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MILTER_TYPE_ERROR_EMITTABLE))
#define MILTER_IS_ERROR_EMITTABLE_CLASS(vtable) (G_TYPE_CHECK_CLASS_TYPE ((vtable), MILTER_TYPE_ERROR_EMITTABLE))
#define MILTER_ERROR_EMITTABLE_GET_CLASS(inst)  (G_TYPE_INSTANCE_GET_INTERFACE ((inst), MILTER_TYPE_ERROR_EMITTABLE, MilterErrorEmittableClass))

typedef struct _MilterErrorEmittable         MilterErrorEmittable;
typedef struct _MilterErrorEmittableClass    MilterErrorEmittableClass;

struct _MilterErrorEmittableClass
{
    GTypeInterface base_iface;

    void (*error) (MilterErrorEmittable *emittable, GError *error);
};

GType    milter_error_emittable_get_type (void) G_GNUC_CONST;

/**
 * milter_error_emittable_emit:
 * @emittable: a %MilterErrorEmittable.
 * @error: a %GError describes error details.
 *
 * Emits #MilterErrorEmittable::error signal.
 */
void     milter_error_emittable_emit     (MilterErrorEmittable *emittable,
                                          GError *error);

G_END_DECLS

#endif /* __MILTER_ERROR_EMITTABLE_H__ */

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
