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

#ifndef __MILTER_MANAGER_CONTEXT_H__
#define __MILTER_MANAGER_CONTEXT_H__

#include <sys/types.h>
#include <sys/socket.h>

#include <glib-object.h>

#include <milter-client.h>
#include <milter-server.h>

G_BEGIN_DECLS

#define MILTER_MANAGER_CONTEXT_ERROR           (milter_manager_context_error_quark())

#define MILTER_MANAGER_TYPE_CONTEXT            (milter_manager_context_get_type())
#define MILTER_MANAGER_CONTEXT(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), MILTER_MANAGER_TYPE_CONTEXT, MilterManagerContext))
#define MILTER_MANAGER_CONTEXT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), MILTER_MANAGER_TYPE_CONTEXT, MilterManagerContextClass))
#define MILTER_MANAGER_IS_CONTEXT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), MILTER_MANAGER_TYPE_CONTEXT))
#define MILTER_MANAGER_IS_CONTEXT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), MILTER_MANAGER_TYPE_CONTEXT))
#define MILTER_MANAGER_CONTEXT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), MILTER_MANAGER_TYPE_CONTEXT, MilterManagerContextClass))

typedef enum
{
    MILTER_MANAGER_CONTEXT_ERROR_FIXME
} MilterManagerContextError;

typedef struct _MilterManagerContext         MilterManagerContext;
typedef struct _MilterManagerContextClass    MilterManagerContextClass;

struct _MilterManagerContext
{
    GObject object;
};

struct _MilterManagerContextClass
{
    GObjectClass parent_class;
};

GQuark                milter_manager_context_error_quark (void);

GType                 milter_manager_context_get_type    (void) G_GNUC_CONST;

MilterManagerContext *milter_manager_context_new         (void);

MilterStatus          milter_manager_context_option_negotiate
                                          (MilterManagerContext *context,
                                           MilterOption         *option);
MilterStatus          milter_manager_context_connect
                                          (MilterManagerContext *context,
                                           const gchar          *host_name,
                                           struct sockaddr      *address,
                                           socklen_t             address_length);
MilterStatus          milter_manager_context_helo
                                          (MilterManagerContext *context,
                                           const gchar          *fqdn);
MilterStatus          milter_manager_context_envelope_from
                                          (MilterManagerContext *context,
                                           const gchar          *from);
MilterStatus          milter_manager_context_envelope_receipt
                                          (MilterManagerContext *context,
                                           const gchar          *receipt);
MilterStatus          milter_manager_context_data
                                          (MilterManagerContext *context);
MilterStatus          milter_manager_context_unknown
                                          (MilterManagerContext *context,
                                           const gchar          *command);
MilterStatus          milter_manager_context_header
                                          (MilterManagerContext *context,
                                           const gchar          *name,
                                           const gchar          *value);
MilterStatus          milter_manager_context_end_of_header
                                          (MilterManagerContext *context);
MilterStatus          milter_manager_context_body
                                          (MilterManagerContext *context,
                                           const guchar         *chunk,
                                           gsize                 size);
MilterStatus          milter_manager_context_end_of_message
                                          (MilterManagerContext *context);
MilterStatus          milter_manager_context_close
                                          (MilterManagerContext *context);
MilterStatus          milter_manager_context_abort
                                          (MilterManagerContext *context);

G_END_DECLS

#endif /* __MILTER_MANAGER_CONTEXT_H__ */

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
