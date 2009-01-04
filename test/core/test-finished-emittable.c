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

#include <gcutter.h>

#include <milter/core/milter-finished-emittable.h>

void test_emit (void);

static MilterFinishedEmittable *parent;

typedef struct _TestFinished TestFinished;
typedef struct _TestFinishedClass TestFinishedClass;
struct _TestFinished
{
    GObject       object;
};

struct _TestFinishedClass
{
    GObjectClass parent_class;
};

static GType test_finished_get_type (void);
static void emittable_init (MilterFinishedEmittable *emittable);

G_DEFINE_TYPE_WITH_CODE(TestFinished, test_finished, G_TYPE_OBJECT,
                        G_IMPLEMENT_INTERFACE(MILTER_TYPE_FINISHED_EMITTABLE, emittable_init))

static void
test_finished_class_init (TestFinishedClass *klass)
{
}

static void
test_finished_init (TestFinished *test_finished)
{
}

static void
emittable_init (MilterFinishedEmittable *emittable)
{
    parent = g_type_interface_peek_parent(emittable);
}

static TestFinished *test_finished;
static gboolean finished;

static void
cb_finished (MilterFinishedEmittable *emittable)
{
    finished = TRUE;
}

void
setup (void)
{
    test_finished = g_object_new(test_finished_get_type(), NULL);
    g_signal_connect(test_finished, "finished",
                     G_CALLBACK(cb_finished), NULL);

    finished = FALSE;
}

void
teardown (void)
{
    if (test_finished)
        g_object_unref(test_finished);
}

void
test_emit (void)
{
    milter_finished_emittable_emit(MILTER_FINISHED_EMITTABLE(test_finished));
    cut_assert_true(finished);
}
/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
