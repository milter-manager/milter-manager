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

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>

#include <glib.h>

#include "milter-command-encoder.h"
#include "milter-logger.h"
#include "milter-enum-types.h"
#include "milter-marshalers.h"
#include "milter-macros-requests.h"
#include "milter-utils.h"

G_DEFINE_TYPE(MilterCommandEncoder, milter_command_encoder, MILTER_TYPE_ENCODER);

static void dispose        (GObject         *object);

static void
milter_command_encoder_class_init (MilterCommandEncoderClass *klass)
{
    GObjectClass *gobject_class;

    gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->dispose      = dispose;
}

static void
milter_command_encoder_init (MilterCommandEncoder *encoder)
{
}

static void
dispose (GObject *object)
{
    G_OBJECT_CLASS(milter_command_encoder_parent_class)->dispose(object);
}

MilterEncoder *
milter_command_encoder_new (void)
{
    return g_object_new(MILTER_TYPE_COMMAND_ENCODER, NULL);
}

void
milter_command_encoder_encode_negotiate (MilterCommandEncoder *encoder,
                                         gchar **packet, gsize *packet_size,
                                         MilterOption *option)
{
    MilterEncoder *_encoder;

    _encoder = MILTER_ENCODER(encoder);
    milter_encoder_encode_negotiate(_encoder, option);
    milter_encoder_pack(_encoder, packet, packet_size);
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
milter_command_encoder_encode_define_macro (MilterCommandEncoder *encoder,
                                            gchar **packet, gsize *packet_size,
                                            MilterCommand context,
                                            GHashTable *macros)
{
    GString *buffer;

    buffer = milter_encoder_get_buffer(MILTER_ENCODER(encoder));

    g_string_append_c(buffer, MILTER_COMMAND_DEFINE_MACRO);
    g_string_append_c(buffer, context);
    if (macros)
        g_hash_table_foreach(macros, encode_each_macro, buffer);
    milter_encoder_pack(MILTER_ENCODER(encoder), packet, packet_size);
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
milter_command_encoder_encode_connect (MilterCommandEncoder *encoder,
                                       gchar **packet, gsize *packet_size,
                                       const gchar *host_name,
                                       const struct sockaddr *address,
                                       socklen_t address_size)
{
    GString *buffer;

    buffer = milter_encoder_get_buffer(MILTER_ENCODER(encoder));

    g_string_append_c(buffer, MILTER_COMMAND_CONNECT);
    g_string_append(buffer, host_name);
    g_string_append_c(buffer, '\0');
    switch (address->sa_family) {
      case AF_INET:
        encode_connect_inet(buffer, (const struct sockaddr_in *)address);
        break;
      case AF_INET6:
        encode_connect_inet6(buffer, (const struct sockaddr_in6 *)address);
        break;
      case AF_UNIX:
        encode_connect_unix(buffer, (const struct sockaddr_un *)address);
        break;
      default:
        *packet = NULL;
        *packet_size = 0;
        return;
    }
    g_string_append_c(buffer, '\0');
    milter_encoder_pack(MILTER_ENCODER(encoder), packet, packet_size);
}

void
milter_command_encoder_encode_helo (MilterCommandEncoder *encoder,
                                    gchar **packet, gsize *packet_size,
                                    const gchar *fqdn)
{
    GString *buffer;

    buffer = milter_encoder_get_buffer(MILTER_ENCODER(encoder));

    g_string_append_c(buffer, MILTER_COMMAND_HELO);
    g_string_append(buffer, fqdn);
    g_string_append_c(buffer, '\0');
    milter_encoder_pack(MILTER_ENCODER(encoder), packet, packet_size);
}

void
milter_command_encoder_encode_envelope_from (MilterCommandEncoder *encoder,
                                             gchar **packet, gsize *packet_size,
                                             const gchar *from)
{
    GString *buffer;

    buffer = milter_encoder_get_buffer(MILTER_ENCODER(encoder));

    g_string_append_c(buffer, MILTER_COMMAND_ENVELOPE_FROM);
    g_string_append(buffer, from);
    g_string_append_c(buffer, '\0');
    milter_encoder_pack(MILTER_ENCODER(encoder), packet, packet_size);
}

void
milter_command_encoder_encode_envelope_recipient (MilterCommandEncoder *encoder,
                                                  gchar **packet,
                                                  gsize *packet_size,
                                                  const gchar *to)
{
    GString *buffer;

    buffer = milter_encoder_get_buffer(MILTER_ENCODER(encoder));

    g_string_append_c(buffer, MILTER_COMMAND_ENVELOPE_RECIPIENT);
    g_string_append(buffer, to);
    g_string_append_c(buffer, '\0');
    milter_encoder_pack(MILTER_ENCODER(encoder), packet, packet_size);
}

void
milter_command_encoder_encode_data (MilterCommandEncoder *encoder,
                                    gchar **packet, gsize *packet_size)
{
    GString *buffer;

    buffer = milter_encoder_get_buffer(MILTER_ENCODER(encoder));

    g_string_append_c(buffer, MILTER_COMMAND_DATA);
    milter_encoder_pack(MILTER_ENCODER(encoder), packet, packet_size);
}

void
milter_command_encoder_encode_header (MilterCommandEncoder *encoder,
                                      gchar **packet, gsize *packet_size,
                                      const gchar *name, const gchar *value)
{
    GString *buffer;

    buffer = milter_encoder_get_buffer(MILTER_ENCODER(encoder));

    g_string_append_c(buffer, MILTER_COMMAND_HEADER);
    g_string_append(buffer, name);
    g_string_append_c(buffer, '\0');
    g_string_append(buffer, value);
    g_string_append_c(buffer, '\0');
    milter_encoder_pack(MILTER_ENCODER(encoder), packet, packet_size);
}

void
milter_command_encoder_encode_end_of_header (MilterCommandEncoder *encoder,
                                             gchar **packet, gsize *packet_size)
{
    GString *buffer;

    buffer = milter_encoder_get_buffer(MILTER_ENCODER(encoder));

    g_string_append_c(buffer, MILTER_COMMAND_END_OF_HEADER);
    milter_encoder_pack(MILTER_ENCODER(encoder), packet, packet_size);
}

void
milter_command_encoder_encode_body (MilterCommandEncoder *encoder,
                                    gchar **packet, gsize *packet_size,
                                    const gchar *chunk, gsize size,
                                    gsize *packed_size)
{
    GString *buffer;
    gsize packed_chunk_size;

    buffer = milter_encoder_get_buffer(MILTER_ENCODER(encoder));

    g_string_append_c(buffer, MILTER_COMMAND_BODY);
    if (size > MILTER_CHUNK_SIZE)
        packed_chunk_size = MILTER_CHUNK_SIZE;
    else
        packed_chunk_size = size;
    g_string_append_len(buffer, chunk, packed_chunk_size);
    milter_encoder_pack(MILTER_ENCODER(encoder), packet, packet_size);

    if (packed_size)
        *packed_size = packed_chunk_size;
}

void
milter_command_encoder_encode_end_of_message (MilterCommandEncoder *encoder,
                                              gchar **packet, gsize *packet_size,
                                              const gchar *chunk, gsize size)
{
    GString *buffer;

    buffer = milter_encoder_get_buffer(MILTER_ENCODER(encoder));

    g_string_append_c(buffer, MILTER_COMMAND_END_OF_MESSAGE);
    if (chunk && size > 0)
        g_string_append_len(buffer, chunk, size);
    milter_encoder_pack(MILTER_ENCODER(encoder), packet, packet_size);
}

void
milter_command_encoder_encode_abort (MilterCommandEncoder *encoder,
                                     gchar **packet, gsize *packet_size)
{
    GString *buffer;

    buffer = milter_encoder_get_buffer(MILTER_ENCODER(encoder));

    g_string_append_c(buffer, MILTER_COMMAND_ABORT);
    milter_encoder_pack(MILTER_ENCODER(encoder), packet, packet_size);
}

void
milter_command_encoder_encode_quit (MilterCommandEncoder *encoder,
                                    gchar **packet, gsize *packet_size)
{
    GString *buffer;

    buffer = milter_encoder_get_buffer(MILTER_ENCODER(encoder));

    g_string_append_c(buffer, MILTER_COMMAND_QUIT);
    milter_encoder_pack(MILTER_ENCODER(encoder), packet, packet_size);
}

void
milter_command_encoder_encode_unknown (MilterCommandEncoder *encoder,
                                       gchar **packet, gsize *packet_size,
                                       const gchar *command)
{
    GString *buffer;

    buffer = milter_encoder_get_buffer(MILTER_ENCODER(encoder));

    g_string_append_c(buffer, MILTER_COMMAND_UNKNOWN);
    g_string_append(buffer, command);
    g_string_append_c(buffer, '\0');
    milter_encoder_pack(MILTER_ENCODER(encoder), packet, packet_size);
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
