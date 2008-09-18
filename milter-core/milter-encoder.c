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
#  include "../config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>

#include <glib.h>

#include "milter-encoder.h"
#include "milter-enum-types.h"
#include "milter-marshalers.h"

#define MILTER_ENCODER_GET_PRIVATE(obj)                 \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj),                 \
                                 MILTER_TYPE_ENCODER,   \
                                 MilterEncoderPrivate))

typedef struct _MilterEncoderPrivate	MilterEncoderPrivate;
struct _MilterEncoderPrivate
{
    GString *buffer;
};

G_DEFINE_TYPE(MilterEncoder, milter_encoder, G_TYPE_OBJECT);

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
milter_encoder_class_init (MilterEncoderClass *klass)
{
    GObjectClass *gobject_class;

    gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->dispose      = dispose;
    gobject_class->set_property = set_property;
    gobject_class->get_property = get_property;

    g_type_class_add_private(gobject_class, sizeof(MilterEncoderPrivate));
}

static void
milter_encoder_init (MilterEncoder *encoder)
{
    MilterEncoderPrivate *priv;

    priv = MILTER_ENCODER_GET_PRIVATE(encoder);
    priv->buffer = g_string_new(NULL);
}

static void
dispose (GObject *object)
{
    MilterEncoderPrivate *priv;

    priv = MILTER_ENCODER_GET_PRIVATE(object);
    if (priv->buffer) {
        g_string_free(priv->buffer, TRUE);
        priv->buffer = NULL;
    }

    G_OBJECT_CLASS(milter_encoder_parent_class)->dispose(object);
}

static void
set_property (GObject      *object,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
    MilterEncoderPrivate *priv;

    priv = MILTER_ENCODER_GET_PRIVATE(object);
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
    MilterEncoderPrivate *priv;

    priv = MILTER_ENCODER_GET_PRIVATE(object);
    switch (prop_id) {
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

MilterEncoder *
milter_encoder_new (void)
{
    return g_object_new(MILTER_TYPE_ENCODER, NULL);
}

static void
pack (GString *output)
{
    guint32 content_size;
    gchar content_string[sizeof(guint32)];

    content_size = htonl(output->len);
    memcpy(content_string, &content_size, sizeof(content_size));
    g_string_prepend_len(output, content_string, sizeof(content_size));
}

void
milter_encoder_encode_option_negotiation (MilterEncoder *encoder,
                                          gchar **packet, gsize *packet_size,
                                          MilterOption *option)
{
    MilterEncoderPrivate *priv;

    priv = MILTER_ENCODER_GET_PRIVATE(encoder);
    g_string_truncate(priv->buffer, 0);

    g_string_append_c(priv->buffer, MILTER_COMMAND_OPTION_NEGOTIATION);
    if (option) {
        guint32 version, action, step;
        gchar version_string[sizeof(guint32)];
        gchar action_string[sizeof(guint32)];
        gchar step_string[sizeof(guint32)];

        version = ntohl(milter_option_get_version(option));
        action = ntohl(milter_option_get_action(option));
        step = ntohl(milter_option_get_step(option));

        memcpy(version_string, &version, sizeof(version));
        memcpy(action_string, &action, sizeof(action));
        memcpy(step_string, &step, sizeof(step));

        g_string_append_len(priv->buffer, version_string, sizeof(version));
        g_string_append_len(priv->buffer, action_string, sizeof(action));
        g_string_append_len(priv->buffer, step_string, sizeof(step));
    }
    pack(priv->buffer);

    *packet = g_memdup(priv->buffer->str, priv->buffer->len);
    *packet_size = priv->buffer->len;
}

static void
encode_each_macro (gpointer key, gpointer value, gpointer user_data)
{
    GString *buffer = user_data;

    g_string_append(buffer, key);
    g_string_append_c(buffer, '\0');
    g_string_append(buffer, value);
    g_string_append_c(buffer, '\0');
}

void
milter_encoder_encode_define_macro (MilterEncoder *encoder,
                                    gchar **packet, gsize *packet_size,
                                    MilterCommand context, GHashTable *macros)
{
    MilterEncoderPrivate *priv;

    priv = MILTER_ENCODER_GET_PRIVATE(encoder);
    g_string_truncate(priv->buffer, 0);

    g_string_append_c(priv->buffer, MILTER_COMMAND_DEFINE_MACRO);
    g_string_append_c(priv->buffer, context);
    if (macros)
        g_hash_table_foreach(macros, encode_each_macro, priv->buffer);
    pack(priv->buffer);

    *packet = g_memdup(priv->buffer->str, priv->buffer->len);
    *packet_size = priv->buffer->len;
}

static void
encode_connect_inet (GString *buffer, const struct sockaddr_in *address)
{
    gchar port_string[sizeof(guint16)];
    gchar inet_address[INET_ADDRSTRLEN];

    memcpy(port_string, &(address->sin_port), sizeof(port_string));
    inet_ntop(AF_INET, &(address->sin_addr), inet_address, sizeof(inet_address));

    g_string_append_c(buffer, MILTER_SOCKET_FAMILY_INET);
    g_string_append_len(buffer, port_string, sizeof(port_string));
    g_string_append(buffer, inet_address);
}

static void
encode_connect_inet6 (GString *buffer, const struct sockaddr_in6 *address)
{
    gchar port_string[sizeof(guint16)];
    gchar inet6_address[INET6_ADDRSTRLEN];

    memcpy(port_string, &(address->sin6_port), sizeof(port_string));
    inet_ntop(AF_INET6, &(address->sin6_addr),
              inet6_address, sizeof(inet6_address));

    g_string_append_c(buffer, MILTER_SOCKET_FAMILY_INET6);
    g_string_append_len(buffer, port_string, sizeof(port_string));
    g_string_append(buffer, inet6_address);
}

static void
encode_connect_unix (GString *buffer, const struct sockaddr_un *address)
{
    gchar port_string[sizeof(guint16)];

    memset(port_string, '\0', sizeof(port_string));

    g_string_append_c(buffer, MILTER_SOCKET_FAMILY_UNIX);
    g_string_append_len(buffer, port_string, sizeof(port_string));
    g_string_append(buffer, address->sun_path);
}

void
milter_encoder_encode_connect (MilterEncoder *encoder,
                               gchar **packet, gsize *packet_size,
                               const gchar *host_name,
                               const struct sockaddr *address,
                               socklen_t address_size)
{
    MilterEncoderPrivate *priv;

    priv = MILTER_ENCODER_GET_PRIVATE(encoder);
    g_string_truncate(priv->buffer, 0);

    g_string_append_c(priv->buffer, MILTER_COMMAND_CONNECT);
    g_string_append(priv->buffer, host_name);
    g_string_append_c(priv->buffer, '\0');
    switch (address->sa_family) {
      case AF_INET:
        encode_connect_inet(priv->buffer, (const struct sockaddr_in *)address);
        break;
      case AF_INET6:
        encode_connect_inet6(priv->buffer, (const struct sockaddr_in6 *)address);
        break;
      case AF_UNIX:
        encode_connect_unix(priv->buffer, (const struct sockaddr_un *)address);
        break;
      default:
        *packet = NULL;
        *packet_size = 0;
        return;
    }
    g_string_append_c(priv->buffer, '\0');
    pack(priv->buffer);

    *packet = g_memdup(priv->buffer->str, priv->buffer->len);
    *packet_size = priv->buffer->len;
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
