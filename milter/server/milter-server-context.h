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

#ifndef __MILTER_SERVER_CONTEXT_H__
#define __MILTER_SERVER_CONTEXT_H__

#include <glib-object.h>

#include <milter/server/milter-server-enum-types.h>
#include <milter/client/milter-client-context.h>

G_BEGIN_DECLS

#define MILTER_SERVER_CONTEXT_ERROR           (milter_server_context_error_quark())

#define MILTER_SERVER_CONTEXT_TYPE            (milter_server_context_get_type())
#define MILTER_SERVER_CONTEXT(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), MILTER_SERVER_CONTEXT_TYPE, MilterServerContext))
#define MILTER_SERVER_CONTEXT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), MILTER_SERVER_CONTEXT_TYPE, MilterServerContextClass))
#define MILTER_SERVER_CONTEXT_IS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), MILTER_SERVER_CONTEXT_TYPE))
#define MILTER_SERVER_CONTEXT_IS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), MILTER_SERVER_CONTEXT_TYPE))
#define MILTER_SERVER_CONTEXT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), MILTER_SERVER_CONTEXT_TYPE, MilterServerContextClass))

typedef enum
{
    MILTER_SERVER_CONTEXT_ERROR_INVALID_CODE
} MilterServerContextError;

typedef struct _MilterServerContext         MilterServerContext;
typedef struct _MilterServerContextClass    MilterServerContextClass;

struct _MilterServerContext
{
    GObject object;
};

struct _MilterServerContextClass
{
    GObjectClass parent_class;
};

GQuark               milter_server_context_error_quark (void);

GType                milter_server_context_get_type    (void) G_GNUC_CONST;

MilterServerContext *milter_server_context_new         (void);

MilterStatus         milter_server_context_option_negotiation
                                                       (MilterClientContext *context,
                                                        MilterOption        *option);
MilterStatus         milter_server_context_connect     (MilterServerContext *context,
                                                        const gchar         *host_name,
                                                        struct sockaddr     *address,
                                                        socklen_t            address_length);
MilterStatus         milter_server_context_helo        (MilterServerContext *context,
                                                        const gchar *fqdn);
MilterStatus         milter_server_context_envelope_from
                                                       (MilterServerContext *context,
                                                        const gchar         *from);
MilterStatus         milter_server_context_envelope_receipt
                                                       (MilterServerContext *context,
                                                        const gchar         *receipt);
MilterStatus         milter_server_context_data        (MilterServerContext *context);
MilterStatus         milter_server_context_unknown     (MilterServerContext *context,
                                                        const gchar         *command);
MilterStatus         milter_server_context_header      (MilterServerContext *context,
                                                        const gchar         *name,
                                                        const gchar         *value);
MilterStatus         milter_server_context_end_of_header
                                                       (MilterServerContext *context);
MilterStatus         milter_server_context_body        (MilterServerContext *context,
                                                        const guchar        *chunk,
                                                        gsize                size);
MilterStatus         milter_server_context_end_of_message
                                                       (MilterServerContext *context);
MilterStatus         milter_server_context_close       (MilterServerContext *context);
MilterStatus         milter_server_context_abort       (MilterServerContext *context);

G_END_DECLS

#endif /* __MILTER_SERVER_CONTEXT_H__ */

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
