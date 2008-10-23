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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef __MILTER_MANAGER_CONTROLLER_H__
#define __MILTER_MANAGER_CONTROLLER_H__

#include <glib-object.h>

#include <milter/client.h>
#include <milter/server.h>
#include <milter/manager/milter-manager-configuration.h>

G_BEGIN_DECLS

#define MILTER_MANAGER_TYPE_CONTROLLER            (milter_manager_controller_get_type())
#define MILTER_MANAGER_CONTROLLER(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), MILTER_MANAGER_TYPE_CONTROLLER, MilterManagerController))
#define MILTER_MANAGER_CONTROLLER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), MILTER_MANAGER_TYPE_CONTROLLER, MilterManagerControllerClass))
#define MILTER_MANAGER_IS_CONTROLLER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), MILTER_MANAGER_TYPE_CONTROLLER))
#define MILTER_MANAGER_IS_CONTROLLER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), MILTER_MANAGER_TYPE_CONTROLLER))
#define MILTER_MANAGER_CONTROLLER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), MILTER_MANAGER_TYPE_CONTROLLER, MilterManagerControllerClass))

typedef struct _MilterManagerController         MilterManagerController;
typedef struct _MilterManagerControllerClass    MilterManagerControllerClass;

struct _MilterManagerController
{
    GObject object;
};

struct _MilterManagerControllerClass
{
    GObjectClass parent_class;
};

typedef enum
{
    MILTER_MANAGER_CONTROLLER_STATE_INVALID,
    MILTER_MANAGER_CONTROLLER_STATE_START,
    MILTER_MANAGER_CONTROLLER_STATE_NEGOTIATE,
    MILTER_MANAGER_CONTROLLER_STATE_NEGOTIATE_REPLIED,
    MILTER_MANAGER_CONTROLLER_STATE_CONNECT,
    MILTER_MANAGER_CONTROLLER_STATE_CONNECT_REPLIED,
    MILTER_MANAGER_CONTROLLER_STATE_HELO,
    MILTER_MANAGER_CONTROLLER_STATE_HELO_REPLIED,
    MILTER_MANAGER_CONTROLLER_STATE_ENVELOPE_FROM,
    MILTER_MANAGER_CONTROLLER_STATE_ENVELOPE_FROM_REPLIED,
    MILTER_MANAGER_CONTROLLER_STATE_ENVELOPE_RECEIPT,
    MILTER_MANAGER_CONTROLLER_STATE_ENVELOPE_RECEIPT_REPLIED,
    MILTER_MANAGER_CONTROLLER_STATE_DATA,
    MILTER_MANAGER_CONTROLLER_STATE_DATA_REPLIED,
    MILTER_MANAGER_CONTROLLER_STATE_UNKNOWN,
    MILTER_MANAGER_CONTROLLER_STATE_UNKNOWN_REPLIED,
    MILTER_MANAGER_CONTROLLER_STATE_HEADER,
    MILTER_MANAGER_CONTROLLER_STATE_HEADER_REPLIED,
    MILTER_MANAGER_CONTROLLER_STATE_END_OF_HEADER,
    MILTER_MANAGER_CONTROLLER_STATE_END_OF_HEADER_REPLIED,
    MILTER_MANAGER_CONTROLLER_STATE_BODY,
    MILTER_MANAGER_CONTROLLER_STATE_BODY_REPLIED,
    MILTER_MANAGER_CONTROLLER_STATE_END_OF_MESSAGE,
    MILTER_MANAGER_CONTROLLER_STATE_END_OF_MESSAGE_REPLIED,
    MILTER_MANAGER_CONTROLLER_STATE_QUIT,
    MILTER_MANAGER_CONTROLLER_STATE_QUIT_REPLIED,
    MILTER_MANAGER_CONTROLLER_STATE_ABORT,
    MILTER_MANAGER_CONTROLLER_STATE_ABORT_REPLIED
} MilterManagerControllerState;

GType                 milter_manager_controller_get_type    (void) G_GNUC_CONST;

MilterManagerController *milter_manager_controller_new
                                          (MilterManagerConfiguration *configuration,
                                           MilterClientContext        *client_context);

MilterStatus          milter_manager_controller_negotiate
                                          (MilterManagerController *controller,
                                           MilterOption            *option);
MilterStatus          milter_manager_controller_connect
                                          (MilterManagerController *controller,
                                           const gchar          *host_name,
                                           struct sockaddr      *address,
                                           socklen_t             address_length);
MilterStatus          milter_manager_controller_helo
                                          (MilterManagerController *controller,
                                           const gchar          *fqdn);
MilterStatus          milter_manager_controller_envelope_from
                                          (MilterManagerController *controller,
                                           const gchar          *from);
MilterStatus          milter_manager_controller_envelope_receipt
                                          (MilterManagerController *controller,
                                           const gchar          *receipt);
MilterStatus          milter_manager_controller_data
                                          (MilterManagerController *controller);
MilterStatus          milter_manager_controller_unknown
                                          (MilterManagerController *controller,
                                           const gchar          *command);
MilterStatus          milter_manager_controller_header
                                          (MilterManagerController *controller,
                                           const gchar          *name,
                                           const gchar          *value);
MilterStatus          milter_manager_controller_end_of_header
                                          (MilterManagerController *controller);
MilterStatus          milter_manager_controller_body
                                          (MilterManagerController *controller,
                                           const guchar         *chunk,
                                           gsize                 size);
MilterStatus          milter_manager_controller_end_of_message
                                          (MilterManagerController *controller);
MilterStatus          milter_manager_controller_quit
                                          (MilterManagerController *controller);
MilterStatus          milter_manager_controller_abort
                                          (MilterManagerController *controller);
void                  milter_manager_controller_mta_timeout
                                          (MilterManagerController *controller);

G_END_DECLS

#endif /* __MILTER_MANAGER_CONTROLLER_H__ */

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
