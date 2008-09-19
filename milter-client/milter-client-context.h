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

#include <sys/types.h>
#include <sys/socket.h>

#include <glib-object.h>

#include <milter-core.h>

G_BEGIN_DECLS

#define MILTER_CLIENT_TYPE_CONTEXT            (milter_client_context_get_type())
#define MILTER_CLIENT_CONTEXT(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), MILTER_CLIENT_TYPE_CONTEXT, MilterClientContext))
#define MILTER_CLIENT_CONTEXT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), MILTER_CLIENT_TYPE_CONTEXT, MilterClientContextClass))
#define MILTER_CLIENT_IS_CONTEXT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), MILTER_CLIENT_TYPE_CONTEXT))
#define MILTER_CLIENT_IS_CONTEXT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), MILTER_CLIENT_TYPE_CONTEXT))
#define MILTER_CLIENT_CONTEXT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), MILTER_CLIENT_TYPE_CONTEXT, MilterClientContextClass))

typedef struct _MilterClientContext         MilterClientContext;
typedef struct _MilterClientContextClass    MilterClientContextClass;

typedef enum
{
    MILTER_CLIENT_STATUS_CONTINUE,
    MILTER_CLIENT_STATUS_REJECT,
    MILTER_CLIENT_STATUS_DISCARD,
    MILTER_CLIENT_STATUS_ACCEPT,
    MILTER_CLIENT_STATUS_TEMP_FAILURE,
    MILTER_CLIENT_STATUS_NO_REPLY,
    MILTER_CLIENT_STATUS_SKIP,
    MILTER_CLIENT_STATUS_ALL_OPTIONS
} MilterClientStatus;

struct _MilterClientContext
{
    GObject object;
};

struct _MilterClientContextClass
{
    GObjectClass parent_class;

    MilterClientStatus (*option_negotiation) (MilterClientContext *context,
                                              MilterOption        *option);
    MilterClientStatus (*connect)            (MilterClientContext *context,
                                              const gchar         *host_name,
                                              struct sockaddr     *address,
                                              socklen_t            address_length);
    MilterClientStatus (*helo)               (MilterClientContext *context,
                                              const gchar         *fqdn);
    MilterClientStatus (*envelope_from)      (MilterClientContext *context,
                                              const gchar         *from);
    MilterClientStatus (*envelope_receipt)   (MilterClientContext *context,
                                              const gchar         *receipt);
    MilterClientStatus (*data)               (MilterClientContext *context);
    MilterClientStatus (*unknown)            (MilterClientContext *context,
                                              const gchar         *command);
    MilterClientStatus (*header)             (MilterClientContext *context,
                                              const gchar         *name,
                                              const gchar         *value);
    MilterClientStatus (*end_of_header)      (MilterClientContext *context);
    MilterClientStatus (*body)               (MilterClientContext *context,
                                              const guchar        *chunk,
                                              gsize                size);
    MilterClientStatus (*end_of_message)     (MilterClientContext *context);
    MilterClientStatus (*close)              (MilterClientContext *context);
    MilterClientStatus (*abort)              (MilterClientContext *context);
};

GType                milter_client_context_get_type          (void) G_GNUC_CONST;

MilterClientContext *milter_client_context_new               (void);

gboolean             milter_client_context_feed              (MilterClientContext *context,
                                                              const gchar *chunk,
                                                              gsize size,
                                                              GError **error);

const gchar         *milter_client_context_get_macro         (MilterClientContext *context,
                                                              const gchar *name);
GHashTable          *milter_client_context_get_macros        (MilterClientContext *context);


G_END_DECLS

#endif /* __MILTER_CLIENT_CONTEXT_H__ */

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
