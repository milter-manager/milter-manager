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

#ifdef HAVE_CONFIG_H
#  include "../../config.h"
#endif /* HAVE_CONFIG_H */

#include "milter-finished-emittable.h"

enum
{
	FINISHED,
	LAST_SIGNAL
};

static gint signals[LAST_SIGNAL] = {0};

static void
base_init (gpointer g_class)
{
    static gboolean initialized = FALSE;

    if (initialized) return;

    /**
     * MilterFinishedEmittable::finished:
     * @emittable: the object that received the signal.
     *
     * This signal is emitted on @emittable's work is
     * finished.
     */
    signals[FINISHED] =
        g_signal_new("finished",
                     G_TYPE_FROM_CLASS(g_class),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterFinishedEmittableClass, finished),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__VOID,
                     G_TYPE_NONE, 0);

    initialized = TRUE;
}

GType
milter_finished_emittable_get_type (void)
{
    static GType finished_emittable_type = 0;

    if (!finished_emittable_type) {
        const GTypeInfo finished_emittable_info = {
            sizeof(MilterFinishedEmittableClass), /* class_size */
            base_init,          /* base_init */
            NULL,               /* base_finalize */
        };

        finished_emittable_type = g_type_register_static(G_TYPE_INTERFACE,
                                                     "MilterFinishedEmittable",
                                                     &finished_emittable_info, 0);
    }

    return finished_emittable_type;
}

void
milter_finished_emittable_emit (MilterFinishedEmittable *emittable)
{
    g_signal_emit(emittable, signals[FINISHED], 0);
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
