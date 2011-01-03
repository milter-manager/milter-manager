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

#ifndef __MILTER_MANAGER_CHILDREN_H__
#define __MILTER_MANAGER_CHILDREN_H__

#include <glib-object.h>

#include <milter/manager/milter-manager-objects.h>
#include <milter/manager/milter-manager-child.h>
#include <milter/core/milter-reply-signals.h>

G_BEGIN_DECLS

#define MILTER_MANAGER_CHILDREN_ERROR           (milter_manager_children_error_quark())

#define MILTER_TYPE_MANAGER_CHILDREN            (milter_manager_children_get_type())
#define MILTER_MANAGER_CHILDREN(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), MILTER_TYPE_MANAGER_CHILDREN, MilterManagerChildren))
#define MILTER_MANAGER_CHILDREN_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), MILTER_TYPE_MANAGER_CHILDREN, MilterManagerChildrenClass))
#define MILTER_MANAGER_IS_CHILDREN(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), MILTER_TYPE_MANAGER_CHILDREN))
#define MILTER_MANAGER_IS_CHILDREN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), MILTER_TYPE_MANAGER_CHILDREN))
#define MILTER_MANAGER_CHILDREN_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), MILTER_TYPE_MANAGER_CHILDREN, MilterManagerChildrenClass))

typedef enum
{
    MILTER_MANAGER_CHILDREN_ERROR_MILTER_EXIT,
    MILTER_MANAGER_CHILDREN_ERROR_NO_NEGOTIATION,
    MILTER_MANAGER_CHILDREN_ERROR_NO_DEMAND_COMMAND,
    MILTER_MANAGER_CHILDREN_ERROR_NO_ALIVE_MILTER,
    MILTER_MANAGER_CHILDREN_ERROR_NO_MESSAGE_PROCESSING_MILTER
} MilterManagerChildrenError;

typedef struct _MilterManagerChildrenClass    MilterManagerChildrenClass;

struct _MilterManagerChildren
{
    GObject object;
};

struct _MilterManagerChildrenClass
{
    GObjectClass parent_class;
};

GQuark                 milter_manager_children_error_quark (void);

GType                  milter_manager_children_get_type    (void) G_GNUC_CONST;

MilterManagerChildren *milter_manager_children_new         (MilterManagerConfiguration *configuration,
                                                            MilterEventLoop            *event_loop);

void                   milter_manager_children_add_child   (MilterManagerChildren *children,
                                                            MilterManagerChild    *child);
guint                  milter_manager_children_length      (MilterManagerChildren *children);
GList                 *milter_manager_children_get_children(MilterManagerChildren *children);
void                   milter_manager_children_foreach     (MilterManagerChildren *children,
                                                            GFunc                  func,
                                                            gpointer               user_data);

gboolean               milter_manager_children_negotiate   (MilterManagerChildren *children,
                                                            MilterOption          *option,
                                                            MilterMacrosRequests  *macros_requests);
gboolean               milter_manager_children_define_macro(MilterManagerChildren *children,
                                                            MilterCommand          command,
                                                            GHashTable            *macro);
gboolean               milter_manager_children_connect     (MilterManagerChildren *children,
                                                            const gchar           *host_name,
                                                            struct sockaddr       *address,
                                                            socklen_t              address_length);
gboolean               milter_manager_children_helo        (MilterManagerChildren *children,
                                                            const gchar           *fqdn);
gboolean               milter_manager_children_envelope_from
                                                           (MilterManagerChildren *children,
                                                            const gchar           *from);
gboolean               milter_manager_children_envelope_recipient
                                                           (MilterManagerChildren *children,
                                                            const gchar           *recipient);
gboolean               milter_manager_children_data        (MilterManagerChildren *children);
gboolean               milter_manager_children_unknown     (MilterManagerChildren *children,
                                                            const gchar           *command);
gboolean               milter_manager_children_header      (MilterManagerChildren *children,
                                                            const gchar           *name,
                                                            const gchar           *value);
gboolean               milter_manager_children_end_of_header
                                                           (MilterManagerChildren *children);
gboolean               milter_manager_children_body        (MilterManagerChildren *children,
                                                            const gchar           *chunk,
                                                            gsize                  size);
gboolean               milter_manager_children_end_of_message
                                                           (MilterManagerChildren *children,
                                                            const gchar           *chunk,
                                                            gsize                  size);
gboolean               milter_manager_children_quit        (MilterManagerChildren *children);
gboolean               milter_manager_children_abort       (MilterManagerChildren *children);

void                   milter_manager_children_set_retry_connect_time
                                                           (MilterManagerChildren *children,
                                                            gdouble time);

/* private */
gboolean               milter_manager_children_is_important_status
                                                           (MilterManagerChildren *children,
                                                            MilterServerContextState state,
                                                            MilterStatus status);
void                   milter_manager_children_set_status  (MilterManagerChildren *children,
                                                            MilterServerContextState state,
                                                            MilterStatus status);
MilterServerContextState
                       milter_manager_children_get_processing_state
                                                           (MilterManagerChildren *children);
void                   milter_manager_children_set_launcher_channel
                                                           (MilterManagerChildren *children,
                                                            GIOChannel *read_channel,
                                                            GIOChannel *write_channel);

guint                  milter_manager_children_get_tag     (MilterManagerChildren *children);
void                   milter_manager_children_set_tag     (MilterManagerChildren *children,
                                                            guint                  tag);


gboolean               milter_manager_children_get_smtp_client_address
                                                           (MilterManagerChildren *children,
                                                            struct sockaddr       **address,
                                                            socklen_t             *address_length);


MilterOption          *milter_manager_children_get_option   (MilterManagerChildren *children);

gboolean               milter_manager_children_is_waiting_reply
                                                            (MilterManagerChildren *children);


#endif /* __MILTER_MANAGER_CHILDREN_H__ */

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
