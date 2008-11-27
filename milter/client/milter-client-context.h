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

#ifndef __MILTER_CLIENT_CONTEXT_H__
#define __MILTER_CLIENT_CONTEXT_H__

#include <glib-object.h>

#include <milter/core.h>

G_BEGIN_DECLS

#define MILTER_CLIENT_CONTEXT_ERROR           (milter_client_context_error_quark())

#define MILTER_TYPE_CLIENT_CONTEXT            (milter_client_context_get_type())
#define MILTER_CLIENT_CONTEXT(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), MILTER_TYPE_CLIENT_CONTEXT, MilterClientContext))
#define MILTER_CLIENT_CONTEXT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), MILTER_TYPE_CLIENT_CONTEXT, MilterClientContextClass))
#define MILTER_CLIENT_IS_CONTEXT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), MILTER_TYPE_CLIENT_CONTEXT))
#define MILTER_CLIENT_IS_CONTEXT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), MILTER_TYPE_CLIENT_CONTEXT))
#define MILTER_CLIENT_CONTEXT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), MILTER_TYPE_CLIENT_CONTEXT, MilterClientContextClass))

typedef enum
{
    MILTER_CLIENT_CONTEXT_ERROR_INVALID_CODE,
    MILTER_CLIENT_CONTEXT_ERROR_IO_ERROR
} MilterClientContextError;

typedef struct _MilterClientContext         MilterClientContext;
typedef struct _MilterClientContextClass    MilterClientContextClass;

struct _MilterClientContext
{
    MilterProtocolAgent object;
};

struct _MilterClientContextClass
{
    MilterProtocolAgentClass parent_class;

    MilterStatus (*negotiate)          (MilterClientContext *context,
                                        MilterOption  *option);
    void         (*negotiate_response) (MilterClientContext *context,
                                        MilterOption  *option,
                                        MilterStatus   status);
    void         (*define_macro)       (MilterClientContext *context,
                                        MilterCommand command,
                                        GHashTable *macros);
    MilterStatus (*connect)            (MilterClientContext *context,
                                        const gchar   *host_name,
                                        struct sockaddr *address,
                                        socklen_t      address_length);
    void         (*connect_response)   (MilterClientContext *context,
                                        MilterStatus         status);
    MilterStatus (*helo)               (MilterClientContext *context,
                                        const gchar   *fqdn);
    void         (*helo_response)      (MilterClientContext *context,
                                        MilterStatus         status);
    MilterStatus (*envelope_from)      (MilterClientContext *context,
                                        const gchar   *from);
    void         (*envelope_from_response)
                                       (MilterClientContext *context,
                                        MilterStatus         status);
    MilterStatus (*envelope_recipient) (MilterClientContext *context,
                                        const gchar   *recipient);
    void         (*envelope_recipient_response)
                                       (MilterClientContext *context,
                                        MilterStatus         status);
    MilterStatus (*data)               (MilterClientContext *context);
    void         (*data_response)      (MilterClientContext *context,
                                        MilterStatus         status);
    MilterStatus (*unknown)            (MilterClientContext *context,
                                        const gchar   *command);
    void         (*unknown_response)   (MilterClientContext *context,
                                        MilterStatus         status);
    MilterStatus (*header)             (MilterClientContext *context,
                                        const gchar   *name,
                                        const gchar   *value);
    void         (*header_response)    (MilterClientContext *context,
                                        MilterStatus         status);
    MilterStatus (*end_of_header)      (MilterClientContext *context);
    void         (*end_of_header_response)
                                       (MilterClientContext *context,
                                        MilterStatus         status);
    MilterStatus (*body)               (MilterClientContext *context,
                                        const gchar         *chunk,
                                        gsize                size);
    void         (*body_response)      (MilterClientContext *context,
                                        MilterStatus         status);
    MilterStatus (*end_of_message)     (MilterClientContext *context);
    void         (*end_of_message_response)
                                       (MilterClientContext *context,
                                        MilterStatus         status);
    MilterStatus (*quit)               (MilterClientContext *context);
    void         (*quit_response)      (MilterClientContext *context,
                                        MilterStatus         status);
    MilterStatus (*abort)              (MilterClientContext *context);
    void         (*abort_response)     (MilterClientContext *context,
                                        MilterStatus         status);
    void         (*mta_timeout)        (MilterClientContext *context);
};

GQuark               milter_client_context_error_quark       (void);

GType                milter_client_context_get_type          (void) G_GNUC_CONST;

MilterClientContext *milter_client_context_new               (void);

gboolean             milter_client_context_feed              (MilterClientContext *context,
                                                              const gchar *chunk,
                                                              gsize size,
                                                              GError **error);
gpointer             milter_client_context_get_private_data  (MilterClientContext *context);
void                 milter_client_context_set_private_data  (MilterClientContext *context,
                                                              gpointer data,
                                                              GDestroyNotify destroy);

gboolean             milter_client_context_set_reply         (MilterClientContext *context,
                                                              guint code,
                                                              const gchar *extended_code,
                                                              const gchar *message,
                                                              GError **error);
gchar               *milter_client_context_format_reply      (MilterClientContext *context);

gboolean             milter_client_context_negotiate_reply   (MilterClientContext *context,
                                                              MilterOption         *option,
                                                              MilterMacrosRequests *macros_requests);
gboolean             milter_client_context_add_header        (MilterClientContext *context,
                                                              const gchar *name,
                                                              const gchar *value);
gboolean             milter_client_context_insert_header     (MilterClientContext *context,
                                                              guint32      index,
                                                              const gchar *name,
                                                              const gchar *value);
gboolean             milter_client_context_change_header     (MilterClientContext *context,
                                                              const gchar *name,
                                                              guint32      index,
                                                              const gchar *value);
gboolean             milter_client_context_delete_header     (MilterClientContext *context,
                                                              const gchar *name,
                                                              guint32      index);
gboolean             milter_client_context_change_from       (MilterClientContext *context,
                                                              const gchar *from,
                                                              const gchar *parameters);
gboolean             milter_client_context_add_recipient     (MilterClientContext *context,
                                                              const gchar *recipient,
                                                              const gchar *parameters);
gboolean             milter_client_context_delete_recipient  (MilterClientContext *context,
                                                              const gchar *recipient);
gboolean             milter_client_context_replace_body      (MilterClientContext *context,
                                                              const gchar *body,
                                                              gsize        body_size);
gboolean             milter_client_context_progress          (MilterClientContext *context);
gboolean             milter_client_context_quarantine        (MilterClientContext *context,
                                                              const gchar *reason);
void                 milter_client_context_set_mta_timeout   (MilterClientContext *context,
                                                              guint timeout);

G_END_DECLS

#endif /* __MILTER_CLIENT_CONTEXT_H__ */

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
