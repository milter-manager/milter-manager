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

#ifndef __MILTER_MANAGER_TEST_CLIENT_H__
#define __MILTER_MANAGER_TEST_CLIENT_H__

#include <gcutter.h>

#define shutdown inet_shutdown
#include <milter/core.h>
#undef shutdown

G_BEGIN_DECLS

#define MILTER_TYPE_MANAGER_TEST_CLIENT            (milter_manager_test_client_get_type())
#define MILTER_MANAGER_TEST_CLIENT(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), MILTER_TYPE_MANAGER_TEST_CLIENT, MilterManagerTestClient))
#define MILTER_MANAGER_TEST_CLIENT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), MILTER_TYPE_MANAGER_TEST_CLIENT, MilterManagerTestClientClass))
#define MILTER_MANAGER_IS_TEST_CLIENT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), MILTER_TYPE_MANAGER_TEST_CLIENT))
#define MILTER_MANAGER_IS_TEST_CLIENT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), MILTER_TYPE_MANAGER_TEST_CLIENT))
#define MILTER_MANAGER_TEST_CLIENT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), MILTER_TYPE_MANAGER_TEST_CLIENT, MilterManagerTestClientClass))

typedef struct _MilterManagerTestClient         MilterManagerTestClient;
typedef struct _MilterManagerTestClientClass    MilterManagerTestClientClass;

typedef guint (*MilterManagerTestClientGetNReceived) (MilterManagerTestClient *client);
typedef const gchar *(*MilterManagerTestClientGetString) (MilterManagerTestClient *client);

struct _MilterManagerTestClient
{
    GObject object;
};

struct _MilterManagerTestClientClass
{
    GObjectClass parent_class;
};

GType                    milter_manager_test_client_get_type (void) G_GNUC_CONST;

MilterManagerTestClient *milter_manager_test_client_new
                                              (guint port,
                                               const gchar *name,
                                               MilterEventLoop *loop);

const gchar             *milter_manager_test_client_get_name
                                              (MilterManagerTestClient *client);
void                     milter_manager_test_client_set_arguments
                                              (MilterManagerTestClient *client,
                                               GArray *arguments);
void                     milter_manager_test_client_set_argument_strings
                                              (MilterManagerTestClient *client,
                                               const gchar **argument_strings);
const GArray            *milter_manager_test_client_get_command
                                              (MilterManagerTestClient *client);

gboolean                 milter_manager_test_client_run
                                              (MilterManagerTestClient *client,
                                               GError **error);

guint                    milter_manager_test_client_get_n_negotiate_received
                                              (MilterManagerTestClient *client);
MilterOption            *milter_manager_test_client_get_negotiate_option
                                              (MilterManagerTestClient *client);
guint                    milter_manager_test_client_get_n_define_macro_received
                                              (MilterManagerTestClient *client);
const GHashTable        *milter_manager_test_client_get_macros
                                              (MilterManagerTestClient *client);
guint                    milter_manager_test_client_get_n_connect_received
                                              (MilterManagerTestClient *client);
const gchar             *milter_manager_test_client_get_connect_host
                                              (MilterManagerTestClient *client);
const gchar             *milter_manager_test_client_get_connect_address
                                              (MilterManagerTestClient *client);
guint                    milter_manager_test_client_get_n_helo_received
                                              (MilterManagerTestClient *client);
const gchar             *milter_manager_test_client_get_helo_fqdn
                                              (MilterManagerTestClient *client);
guint                    milter_manager_test_client_get_n_envelope_from_received
                                              (MilterManagerTestClient *client);
const gchar             *milter_manager_test_client_get_envelope_from
                                              (MilterManagerTestClient *client);
guint                    milter_manager_test_client_get_n_envelope_recipient_received
                                              (MilterManagerTestClient *client);
const gchar             *milter_manager_test_client_get_envelope_recipient
                                              (MilterManagerTestClient *client);
guint                    milter_manager_test_client_get_n_data_received
                                              (MilterManagerTestClient *client);
guint                    milter_manager_test_client_get_n_header_received
                                              (MilterManagerTestClient *client);
const gchar             *milter_manager_test_client_get_header_name
                                              (MilterManagerTestClient *client);
const gchar             *milter_manager_test_client_get_header_value
                                              (MilterManagerTestClient *client);
guint                    milter_manager_test_client_get_n_end_of_header_received
                                              (MilterManagerTestClient *client);
guint                    milter_manager_test_client_get_n_body_received
                                              (MilterManagerTestClient *client);
const gchar             *milter_manager_test_client_get_body_chunk
                                              (MilterManagerTestClient *client);
guint                    milter_manager_test_client_get_n_end_of_message_received
                                              (MilterManagerTestClient *client);
const gchar             *milter_manager_test_client_get_end_of_message_chunk
                                              (MilterManagerTestClient *client);
guint                    milter_manager_test_client_get_n_quit_received
                                              (MilterManagerTestClient *client);
guint                    milter_manager_test_client_get_n_abort_received
                                              (MilterManagerTestClient *client);
guint                    milter_manager_test_client_get_n_unknown_received
                                              (MilterManagerTestClient *client);
const gchar             *milter_manager_test_client_get_unknown_command
                                              (MilterManagerTestClient *client);

gboolean                 milter_manager_test_client_is_reaped
                                              (MilterManagerTestClient *client);

void                     milter_manager_test_client_clear_data
                                              (MilterManagerTestClient *client);
void                     milter_manager_test_client_wait_reply
                                              (MilterManagerTestClient *client,
                                               MilterManagerTestClientGetNReceived getter);
void                     milter_manager_test_client_assert_nothing_output
                                              (MilterManagerTestClient *client);


MilterManagerTestClient *milter_manager_test_clients_find
                                              (const GList *clients,
                                               const gchar *name);
void                     milter_manager_test_clients_wait_reply
                                              (const GList *clients,
                                               MilterManagerTestClientGetNReceived getter);
void                     milter_manager_test_clients_wait_n_replies
                                              (const GList *clients,
                                               MilterManagerTestClientGetNReceived getter,
                                               guint n_replies);
void                     milter_manager_test_clients_wait_n_alive
                                              (const GList *clients,
                                               guint n_alive);
guint                    milter_manager_test_clients_collect_n_received
                                              (const GList *clients,
                                               MilterManagerTestClientGetNReceived getter);
guint                    milter_manager_test_clients_collect_n_alive
                                              (const GList *clients);
const GList             *milter_manager_test_clients_collect_strings
                                              (const GList *clients,
                                               MilterManagerTestClientGetString getter);
const GList             *milter_manager_test_clients_collect_pairs
                                              (const GList *clients,
                                               MilterManagerTestClientGetString name_getter,
                                               MilterManagerTestClientGetString value_getter);
const GList             *milter_manager_test_clients_collect_negotiate_options
                                              (const GList *clients);
const GList             *milter_manager_test_clients_collect_macros
                                              (const GList *clients,
                                               MilterCommand command);
const GList             *milter_manager_test_clients_collect_all_macros
                                              (const GList *clients);

G_END_DECLS

#endif /* __MILTER_MANAGER_TEST_CLIENT_H__ */

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
