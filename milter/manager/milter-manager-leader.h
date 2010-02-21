/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Copyright (C) 2008-2010  Kouhei Sutou <kou@clear-code.com>
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

#ifndef __MILTER_MANAGER_LEADER_H__
#define __MILTER_MANAGER_LEADER_H__

#include <glib-object.h>

#include <milter/client.h>
#include <milter/server.h>
#include <milter/manager/milter-manager-configuration.h>

G_BEGIN_DECLS

#define MILTER_MANAGER_LEADER_ERROR           (milter_manager_leader_error_quark())

#define MILTER_TYPE_MANAGER_LEADER            (milter_manager_leader_get_type())
#define MILTER_MANAGER_LEADER(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), MILTER_TYPE_MANAGER_LEADER, MilterManagerLeader))
#define MILTER_MANAGER_LEADER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), MILTER_TYPE_MANAGER_LEADER, MilterManagerLeaderClass))
#define MILTER_MANAGER_IS_LEADER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), MILTER_TYPE_MANAGER_LEADER))
#define MILTER_MANAGER_IS_LEADER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), MILTER_TYPE_MANAGER_LEADER))
#define MILTER_MANAGER_LEADER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), MILTER_TYPE_MANAGER_LEADER, MilterManagerLeaderClass))

typedef enum
{
    MILTER_MANAGER_LEADER_ERROR_INVALID_STATE
} MilterManagerLeaderError;

typedef struct _MilterManagerLeaderClass    MilterManagerLeaderClass;

struct _MilterManagerLeader
{
    GObject object;
};

struct _MilterManagerLeaderClass
{
    GObjectClass parent_class;

    gboolean (*connection_check) (MilterManagerLeader *leader);
};

typedef enum
{
    MILTER_MANAGER_LEADER_STATE_INVALID,
    MILTER_MANAGER_LEADER_STATE_START,
    MILTER_MANAGER_LEADER_STATE_NEGOTIATE,
    MILTER_MANAGER_LEADER_STATE_NEGOTIATE_REPLIED,
    MILTER_MANAGER_LEADER_STATE_CONNECT,
    MILTER_MANAGER_LEADER_STATE_CONNECT_REPLIED,
    MILTER_MANAGER_LEADER_STATE_HELO,
    MILTER_MANAGER_LEADER_STATE_HELO_REPLIED,
    MILTER_MANAGER_LEADER_STATE_ENVELOPE_FROM,
    MILTER_MANAGER_LEADER_STATE_ENVELOPE_FROM_REPLIED,
    MILTER_MANAGER_LEADER_STATE_ENVELOPE_RECIPIENT,
    MILTER_MANAGER_LEADER_STATE_ENVELOPE_RECIPIENT_REPLIED,
    MILTER_MANAGER_LEADER_STATE_DATA,
    MILTER_MANAGER_LEADER_STATE_DATA_REPLIED,
    MILTER_MANAGER_LEADER_STATE_UNKNOWN,
    MILTER_MANAGER_LEADER_STATE_UNKNOWN_REPLIED,
    MILTER_MANAGER_LEADER_STATE_HEADER,
    MILTER_MANAGER_LEADER_STATE_HEADER_REPLIED,
    MILTER_MANAGER_LEADER_STATE_END_OF_HEADER,
    MILTER_MANAGER_LEADER_STATE_END_OF_HEADER_REPLIED,
    MILTER_MANAGER_LEADER_STATE_BODY,
    MILTER_MANAGER_LEADER_STATE_BODY_REPLIED,
    MILTER_MANAGER_LEADER_STATE_END_OF_MESSAGE,
    MILTER_MANAGER_LEADER_STATE_END_OF_MESSAGE_REPLIED,
    MILTER_MANAGER_LEADER_STATE_QUIT,
    MILTER_MANAGER_LEADER_STATE_QUIT_REPLIED,
    MILTER_MANAGER_LEADER_STATE_ABORT,
    MILTER_MANAGER_LEADER_STATE_ABORT_REPLIED
} MilterManagerLeaderState;

GQuark                milter_manager_leader_error_quark (void);

GType                 milter_manager_leader_get_type    (void) G_GNUC_CONST;

MilterManagerLeader  *milter_manager_leader_new
                                          (MilterManagerConfiguration *configuration,
                                           MilterClientContext        *client_context);
MilterManagerConfiguration *
                      milter_manager_leader_get_configuration
                                          (MilterManagerLeader *leader);

void                  milter_manager_leader_set_launcher_channel
                                          (MilterManagerLeader *leader,
                                           GIOChannel *read_channel,
                                           GIOChannel *write_channel);

gboolean              milter_manager_leader_check_connection
                                          (MilterManagerLeader *leader);

MilterStatus          milter_manager_leader_negotiate
                                          (MilterManagerLeader *leader,
                                           MilterOption        *option,
                                           MilterMacrosRequests *macros_requests);
MilterStatus          milter_manager_leader_connect
                                          (MilterManagerLeader *leader,
                                           const gchar          *host_name,
                                           struct sockaddr      *address,
                                           socklen_t             address_length);
MilterStatus          milter_manager_leader_helo
                                          (MilterManagerLeader *leader,
                                           const gchar          *fqdn);
MilterStatus          milter_manager_leader_envelope_from
                                          (MilterManagerLeader *leader,
                                           const gchar          *from);
MilterStatus          milter_manager_leader_envelope_recipient
                                          (MilterManagerLeader *leader,
                                           const gchar          *recipient);
MilterStatus          milter_manager_leader_data
                                          (MilterManagerLeader *leader);
MilterStatus          milter_manager_leader_unknown
                                          (MilterManagerLeader *leader,
                                           const gchar          *command);
MilterStatus          milter_manager_leader_header
                                          (MilterManagerLeader *leader,
                                           const gchar          *name,
                                           const gchar          *value);
MilterStatus          milter_manager_leader_end_of_header
                                          (MilterManagerLeader *leader);
MilterStatus          milter_manager_leader_body
                                          (MilterManagerLeader *leader,
                                           const gchar             *chunk,
                                           gsize                    size);
MilterStatus          milter_manager_leader_end_of_message
                                          (MilterManagerLeader *leader,
                                           const gchar             *chunk,
                                           gsize                    size);
MilterStatus          milter_manager_leader_quit
                                          (MilterManagerLeader *leader);
MilterStatus          milter_manager_leader_abort
                                          (MilterManagerLeader *leader);
void                  milter_manager_leader_timeout
                                          (MilterManagerLeader *leader);
void                  milter_manager_leader_define_macro
                                          (MilterManagerLeader *leader,
                                           MilterCommand command,
                                           GHashTable *macros);

guint                 milter_manager_leader_get_tag
                                          (MilterManagerLeader *leader);
void                  milter_manager_leader_set_tag
                                          (MilterManagerLeader *leader,
                                           guint                tag);

MilterManagerChildren *milter_manager_leader_get_children
                                          (MilterManagerLeader *leader);

G_END_DECLS

#endif /* __MILTER_MANAGER_LEADER_H__ */

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
