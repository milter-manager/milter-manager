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

#ifndef __MILTER_CLIENT_HANDLER_H__
#define __MILTER_CLIENT_HANDLER_H__

#include <sys/types.h>
#include <sys/socket.h>

#include <glib-object.h>

#include <milter-core.h>

G_BEGIN_DECLS

#define MILTER_CLIENT_TYPE_HANDLER            (milter_client_handler_get_type())
#define MILTER_CLIENT_HANDLER(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), MILTER_CLIENT_TYPE_HANDLER, MilterClientHandler))
#define MILTER_CLIENT_HANDLER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), MILTER_CLIENT_TYPE_HANDLER, MilterClientHandlerClass))
#define MILTER_CLIENT_IS_HANDLER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), MILTER_CLIENT_TYPE_HANDLER))
#define MILTER_CLIENT_IS_HANDLER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), MILTER_CLIENT_TYPE_HANDLER))
#define MILTER_CLIENT_HANDLER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), MILTER_CLIENT_TYPE_HANDLER, MilterClientHandlerClass))

typedef struct _MilterClientHandler         MilterClientHandler;
typedef struct _MilterClientHandlerClass    MilterClientHandlerClass;

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

struct _MilterClientHandler
{
    GObject object;
};

struct _MilterClientHandlerClass
{
    GObjectClass parent_class;

    MilterClientStatus (*option_negotiation) (MilterClientHandler *handler,
                                              MilterOption        *option);
    MilterClientStatus (*connect)            (MilterClientHandler *handler,
                                              const gchar         *host_name,
                                              struct sockaddr     *address,
                                              socklen_t            address_length);
    MilterClientStatus (*helo)               (MilterClientHandler *handler,
                                              const gchar         *fqdn);
    MilterClientStatus (*envelope_from)      (MilterClientHandler *handler,
                                              const gchar         *from);
    MilterClientStatus (*envelope_receipt)   (MilterClientHandler *handler,
                                              const gchar         *receipt);
    MilterClientStatus (*data)               (MilterClientHandler *handler);
    MilterClientStatus (*unknown)            (MilterClientHandler *handler,
                                              const gchar         *command);
    MilterClientStatus (*header)             (MilterClientHandler *handler,
                                              const gchar         *name,
                                              const gchar         *value);
    MilterClientStatus (*end_of_header)      (MilterClientHandler *handler);
    MilterClientStatus (*body)               (MilterClientHandler *handler,
                                              const guchar        *chunk,
                                              gsize                size);
    MilterClientStatus (*end_of_message)     (MilterClientHandler *handler);
    MilterClientStatus (*close)              (MilterClientHandler *handler);
    MilterClientStatus (*abort)              (MilterClientHandler *handler);
};

GType                milter_client_handler_get_type          (void) G_GNUC_CONST;

MilterClientHandler *milter_client_handler_new               (void);

gboolean             milter_client_handler_feed              (MilterClientHandler *handler,
                                                              const gchar *chunk,
                                                              gsize size,
                                                              GError **error);

G_END_DECLS

#endif /* __MILTER_CLIENT_HANDLER_H__ */

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
