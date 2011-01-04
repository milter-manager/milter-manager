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

#ifndef __MILTER_TEST_SERVER_H__
#define __MILTER_TEST_SERVER_H__

#include <glib-object.h>

#include <gcutter.h>
#include <milter/core.h>

G_BEGIN_DECLS

#define MILTER_TEST_TYPE_SERVER            (milter_test_server_get_type())
#define MILTER_TEST_SERVER(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), MILTER_TYPE_TEST_SERVER, MilterTestServer))
#define MILTER_TEST_SERVER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), MILTER_TYPE_TEST_SERVER, MilterTestServerClass))
#define MILTER_TEST_SERVER_IS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), MILTER_TYPE_TEST_SERVER))
#define MILTER_TEST_SERVER_IS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), MILTER_TYPE_TEST_SERVER))
#define MILTER_TEST_SERVER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), MILTER_TYPE_TEST_SERVER, MilterTestServerClass))

typedef struct _MilterTestServer         MilterTestServer;
typedef struct _MilterTestServerClass    MilterTestServerClass;

struct _MilterTestServer
{
    GObject object;
};

struct _MilterTestServerClass
{
    GObjectClass parent_class;
};


#define milter_test_server_write(...)                           \
    cut_trace_with_info_expression(                             \
        milter_test_server_write_with_error_check(__VA_ARGS__), \
        milter_test_server_write(__VA_ARGS__))

GType             milter_test_server_get_type   (void) G_GNUC_CONST;

MilterTestServer *milter_test_server_new        (const gchar       *spec,
                                                 MilterDecoder     *decoder,
                                                 MilterEventLoop   *loop);

void              milter_test_server_write_with_error_check
                                                (MilterTestServer  *server,
                                                 const gchar       *data,
                                                 gsize              data_size);

G_END_DECLS

#endif /* __MILTER_TEST_SERVER_H__ */

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
