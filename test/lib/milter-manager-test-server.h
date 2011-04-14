/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Copyright (C) 2008-2011  Kouhei Sutou <kou@clear-code.com>
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

#ifndef __MILTER_MANAGER_TEST_SERVER_H__
#define __MILTER_MANAGER_TEST_SERVER_H__

#define shutdown inet_shutdown
#include <sys/types.h>
#include <sys/socket.h>

#include <glib-object.h>

#include <milter/server.h>
#undef shutdown

G_BEGIN_DECLS

#define MILTER_TYPE_MANAGER_TEST_SERVER            (milter_manager_test_server_get_type())
#define MILTER_MANAGER_TEST_SERVER(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), MILTER_TYPE_MANAGER_TEST_SERVER, MilterManagerTestServer))
#define MILTER_MANAGER_TEST_SERVER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), MILTER_TYPE_MANAGER_TEST_SERVER, MilterManagerTestServerClass))
#define MILTER_MANAGER_IS_TEST_SERVER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), MILTER_TYPE_MANAGER_TEST_SERVER))
#define MILTER_MANAGER_IS_TEST_SERVER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), MILTER_TYPE_MANAGER_TEST_SERVER))
#define MILTER_MANAGER_TEST_SERVER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), MILTER_TYPE_MANAGER_TEST_SERVER, MilterManagerTestServerClass))

typedef struct _MilterManagerTestServer         MilterManagerTestServer;
typedef struct _MilterManagerTestServerClass    MilterManagerTestServerClass;

struct _MilterManagerTestServer
{
    MilterServerContext object;
};

struct _MilterManagerTestServerClass
{
    MilterServerContextClass parent_class;
};

GQuark                milter_manager_test_server_error_quark (void);

GType                 milter_manager_test_server_get_type    (void) G_GNUC_CONST;

MilterManagerTestServer *milter_manager_test_server_new      (void);

void                  milter_manager_test_server_append_header
                                                             (MilterManagerTestServer *server,
                                                             const gchar *name,
                                                             const gchar *value);

guint                 milter_manager_test_server_get_n_add_headers
                                                            (MilterManagerTestServer *server);
guint                 milter_manager_test_server_get_n_insert_headers
                                                            (MilterManagerTestServer *server);
guint                 milter_manager_test_server_get_n_change_headers
                                                            (MilterManagerTestServer *server);
guint                 milter_manager_test_server_get_n_delete_headers
                                                            (MilterManagerTestServer *server);
guint                 milter_manager_test_server_get_n_change_froms
                                                            (MilterManagerTestServer *server);
guint                 milter_manager_test_server_get_n_add_recipients
                                                            (MilterManagerTestServer *server);
guint                 milter_manager_test_server_get_n_delete_recipients
                                                            (MilterManagerTestServer *server);
guint                 milter_manager_test_server_get_n_replace_bodies
                                                            (MilterManagerTestServer *server);
guint                 milter_manager_test_server_get_n_progresses
                                                            (MilterManagerTestServer *server);
guint                 milter_manager_test_server_get_n_quarantines
                                                            (MilterManagerTestServer *server);
guint                 milter_manager_test_server_get_n_reply_codes
                                                            (MilterManagerTestServer *server);

MilterHeaders        *milter_manager_test_server_get_headers
                                                            (MilterManagerTestServer *server);
const GList          *milter_manager_test_server_get_received_add_recipients
                                                            (MilterManagerTestServer *server);
const GList          *milter_manager_test_server_get_received_delete_recipients
                                                            (MilterManagerTestServer *server);
const GList          *milter_manager_test_server_get_received_change_froms
                                                            (MilterManagerTestServer *server);
const GList          *milter_manager_test_server_get_received_replace_bodies
                                                            (MilterManagerTestServer *server);
const GList          *milter_manager_test_server_get_received_quarantine_reasons
                                                            (MilterManagerTestServer *server);
const GList          *milter_manager_test_server_get_received_reply_codes
                                                            (MilterManagerTestServer *server);
const gchar          *milter_manager_test_server_get_last_error
                                                            (MilterManagerTestServer *server);

#endif /* __MILTER_MANAGER_TEST_SERVER_H__ */

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
