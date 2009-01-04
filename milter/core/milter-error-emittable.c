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

#include "milter-error-emittable.h"

enum
{
	ERROR,
	LAST_SIGNAL
};

static gint signals[LAST_SIGNAL] = {0};

static void
base_init (gpointer g_class)
{
    static gboolean initialized = FALSE;

    if (initialized) return;

    /**
     * MilterErrorEmittable::error:
     * @emittable: the object that received the signal.
     *
     * This signal is emitted on error.
     */
    signals[ERROR] =
        g_signal_new("error",
                     G_TYPE_FROM_CLASS(g_class),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterErrorEmittableClass, error),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__POINTER,
                     G_TYPE_NONE, 1, G_TYPE_POINTER);

    initialized = TRUE;
}

GType
milter_error_emittable_get_type (void)
{
    static GType error_emittable_type = 0;

    if (!error_emittable_type) {
        const GTypeInfo error_emittable_info = {
            sizeof(MilterErrorEmittableClass), /* class_size */
            base_init,          /* base_init */
            NULL,               /* base_finalize */
        };

        error_emittable_type = g_type_register_static(G_TYPE_INTERFACE,
                                                     "MilterErrorEmittable",
                                                     &error_emittable_info, 0);
    }

    return error_emittable_type;
}

void
milter_error_emittable_emit (MilterErrorEmittable *emittable,
                             GError *error)
{
    g_signal_emit(emittable, signals[ERROR], 0, error);
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
