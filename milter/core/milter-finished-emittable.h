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

#ifndef __MILTER_FINISHED_EMITTABLE_H__
#define __MILTER_FINISHED_EMITTABLE_H__

#include <glib-object.h>

G_BEGIN_DECLS

/**
 * SECTION: milter-finished-emittable
 * @title: MilterFinishedEmittable
 * @short_description: An interface for "finished" signal.
 *
 * The %MilterFinishedEmittable interface provides
 * #MilterFinishedEmittable::finished
 * signal. #MilterFinishedEmittable::finished signal will be
 * emitted on finishing something.
 */

#define MILTER_TYPE_FINISHED_EMITTABLE             (milter_finished_emittable_get_type())
#define MILTER_FINISHED_EMITTABLE(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj), MILTER_TYPE_FINISHED_EMITTABLE, MilterFinishedEmittable))
#define MILTER_FINISHED_EMITTABLE_CLASS(obj)       (G_TYPE_CHECK_CLASS_CAST((obj), MILTER_TYPE_FINISHED_EMITTABLE, MilterFinishedEmittableClass))
#define MILTER_IS_FINISHED_EMITTABLE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj), MILTER_TYPE_FINISHED_EMITTABLE))
#define MILTER_FINISHED_EMITTABLE_GET_IFACE(obj)   (G_TYPE_INSTANCE_GET_INTERFACE((obj), MILTER_TYPE_FINISHED_EMITTABLE, MilterFinishedEmittableClass))

typedef struct _MilterFinishedEmittable         MilterFinishedEmittable;
typedef struct _MilterFinishedEmittableClass    MilterFinishedEmittableClass;

struct _MilterFinishedEmittableClass
{
    GTypeInterface base_iface;

    void (*finished) (MilterFinishedEmittable *emittable);
};

GType    milter_finished_emittable_get_type          (void) G_GNUC_CONST;

/**
 * milter_finished_emittable_emit:
 * @emittable: a %MilterFinishedEmittable.
 *
 * Emits #MilterFinishedEmittable::finished signal.
 */
void     milter_finished_emittable_emit              (MilterFinishedEmittable *emittable);

G_END_DECLS

#endif /* __MILTER_FINISHED_EMITTABLE_H__ */

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
