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

#ifdef HAVE_CONFIG_H
#  include "../../config.h"
#endif /* HAVE_CONFIG_H */

#include <string.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h>

#include <unistd.h>
#include <errno.h>

#include <milter/core.h>
#include "milter-server-context.h"

#define MILTER_SERVER_CONTEXT_GET_PRIVATE(obj)                       \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj),                              \
                                 MILTER_TYPE_SERVER_CONTEXT,         \
                                 MilterServerContextPrivate))

typedef struct _MilterServerContextPrivate	MilterServerContextPrivate;
struct _MilterServerContextPrivate
{
    gint server_context_fd;
    struct sockaddr *address;
    socklen_t address_size;
};

G_DEFINE_TYPE(MilterServerContext, milter_server_context, G_TYPE_OBJECT);

static void dispose        (GObject         *object);
static void set_property   (GObject         *object,
                            guint            prop_id,
                            const GValue    *value,
                            GParamSpec      *pspec);
static void get_property   (GObject         *object,
                            guint            prop_id,
                            GValue          *value,
                            GParamSpec      *pspec);

static void
milter_server_context_class_init (MilterServerContextClass *klass)
{
    GObjectClass *gobject_class;

    gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->dispose      = dispose;
    gobject_class->set_property = set_property;
    gobject_class->get_property = get_property;

    g_type_class_add_private(gobject_class, sizeof(MilterServerContextPrivate));
}

static void
milter_server_context_init (MilterServerContext *server_context)
{
    MilterServerContextPrivate *priv;

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(server_context);
    priv->server_context_fd = -1;
    priv->address = NULL;
    priv->address_size = 0;
}

static void
dispose (GObject *object)
{
    MilterServerContextPrivate *priv;

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(object);

    if (priv->address) {
        g_free(priv->address);
        priv->address = NULL;
    }

    G_OBJECT_CLASS(milter_server_context_parent_class)->dispose(object);
}

static void
set_property (GObject      *object,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
    MilterServerContextPrivate *priv;

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(object);
    switch (prop_id) {
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static void
get_property (GObject    *object,
              guint       prop_id,
              GValue     *value,
              GParamSpec *pspec)
{
    MilterServerContextPrivate *priv;

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(object);
    switch (prop_id) {
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

GQuark
milter_server_context_error_quark (void)
{
    return g_quark_from_static_string("milter-server-context-error-quark");
}

MilterServerContext *
milter_server_context_new (void)
{
    return g_object_new(MILTER_TYPE_SERVER_CONTEXT,
                        NULL);
}

MilterStatus
milter_server_context_negotiation (MilterClientContext *context,
                                   MilterOption        *option)
{
    return MILTER_STATUS_REJECT;
}

MilterStatus
milter_server_context_helo (MilterServerContext *context,
                            const gchar *fqdn)
{
    return MILTER_STATUS_ACCEPT;
}

MilterStatus
milter_server_context_connect (MilterServerContext *context,
                               const gchar         *host_name,
                               struct sockaddr     *address,
                               socklen_t            address_length)
{
    return MILTER_STATUS_REJECT;
}

MilterStatus
milter_server_context_envelope_from (MilterServerContext *context,
                                     const gchar         *from)
{
    return MILTER_STATUS_REJECT;
}

MilterStatus
milter_server_context_envelope_receipt (MilterServerContext *context,
                                        const gchar         *receipt)
{
    return MILTER_STATUS_REJECT;
}

MilterStatus
milter_server_context_data (MilterServerContext *context)
{
    return MILTER_STATUS_REJECT;
}

MilterStatus
milter_server_context_unknown (MilterServerContext *context,
                               const gchar         *command)
{
    return MILTER_STATUS_REJECT;
}

MilterStatus
milter_server_context_header (MilterServerContext *context,
                              const gchar         *name,
                              const gchar         *value)
{
    return MILTER_STATUS_REJECT;
}

MilterStatus
milter_server_context_end_of_header (MilterServerContext *context)
{
    return MILTER_STATUS_REJECT;
}

MilterStatus
milter_server_context_body (MilterServerContext *context,
                            const guchar        *chunk,
                            gsize                size)
{
    return MILTER_STATUS_REJECT;
}

MilterStatus
milter_server_context_end_of_message (MilterServerContext *context)
{
    return MILTER_STATUS_REJECT;
}

MilterStatus
milter_server_context_close (MilterServerContext *context)
{
    return MILTER_STATUS_REJECT;
}


MilterStatus
milter_server_context_abort (MilterServerContext *context)
{
    return MILTER_STATUS_REJECT;
}


/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
