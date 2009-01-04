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

#include <milter/core/milter-error-emittable.h>

void test_emit (void);

static MilterErrorEmittable *parent;

typedef struct _TestError TestError;
typedef struct _TestErrorClass TestErrorClass;
struct _TestError
{
    GObject       object;
};

struct _TestErrorClass
{
    GObjectClass parent_class;
};

static GType test_error_get_type (void);
static void emittable_init (MilterErrorEmittable *emittable);

G_DEFINE_TYPE_WITH_CODE(TestError, test_error, G_TYPE_OBJECT,
                        G_IMPLEMENT_INTERFACE(MILTER_TYPE_ERROR_EMITTABLE, emittable_init))

static void
test_error_class_init (TestErrorClass *klass)
{
}

static void
test_error_init (TestError *test_error)
{
}

static void
emittable_init (MilterErrorEmittable *emittable)
{
    parent = g_type_interface_peek_parent(emittable);
}

static GError *expected_error;
static GError *actual_error;
static TestError *test_error;

static void
cb_error (MilterErrorEmittable *emittable, GError *error)
{
    actual_error = g_error_copy(error);
}

void
setup (void)
{
    test_error = g_object_new(test_error_get_type(), NULL);
    g_signal_connect(test_error, "error",
                     G_CALLBACK(cb_error), NULL);

    expected_error = NULL;
    actual_error = NULL;
}

void
teardown (void)
{
    if (test_error)
        g_object_unref(test_error);

    if (expected_error)
        g_error_free(expected_error);
    if (actual_error)
        g_error_free(actual_error);
}

void
test_emit (void)
{
    expected_error = g_error_new(G_FILE_ERROR,
                                 G_FILE_ERROR_IO,
                                 "test error");
    milter_error_emittable_emit(MILTER_ERROR_EMITTABLE(test_error),
                                expected_error);
    gcut_assert_equal_error(expected_error, actual_error);
}
/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
