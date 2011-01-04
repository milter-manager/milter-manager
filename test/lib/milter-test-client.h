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

#ifndef __MILTER_TEST_CLIENT_H__
#define __MILTER_TEST_CLIENT_H__

#include <glib-object.h>

#include <gcutter.h>
#include <milter/core.h>

G_BEGIN_DECLS

#define MILTER_TEST_TYPE_CLIENT            (milter_test_client_get_type())
#define MILTER_TEST_CLIENT(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), MILTER_TYPE_TEST_CLIENT, MilterTestClient))
#define MILTER_TEST_CLIENT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), MILTER_TYPE_TEST_CLIENT, MilterTestClientClass))
#define MILTER_TEST_CLIENT_IS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), MILTER_TYPE_TEST_CLIENT))
#define MILTER_TEST_CLIENT_IS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), MILTER_TYPE_TEST_CLIENT))
#define MILTER_TEST_CLIENT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), MILTER_TYPE_TEST_CLIENT, MilterTestClientClass))

typedef struct _MilterTestClient         MilterTestClient;
typedef struct _MilterTestClientClass    MilterTestClientClass;

struct _MilterTestClient
{
    GObject object;
};

struct _MilterTestClientClass
{
    GObjectClass parent_class;
};


#define milter_test_client_write(...)                           \
    cut_trace_with_info_expression(                             \
        milter_test_client_write_with_error_check(__VA_ARGS__), \
        milter_test_client_write(__VA_ARGS__))

GType             milter_test_client_get_type   (void) G_GNUC_CONST;

MilterTestClient *milter_test_client_new        (const gchar       *spec,
                                                 MilterDecoder     *decoder,
                                                 MilterEventLoop   *loop);

void              milter_test_client_write_with_error_check
                                                (MilterTestClient  *client,
                                                 const gchar       *data,
                                                 gsize              data_size);

G_END_DECLS

#endif /* __MILTER_TEST_CLIENT_H__ */

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
