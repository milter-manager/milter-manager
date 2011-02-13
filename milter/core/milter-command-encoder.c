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

#ifdef HAVE_CONFIG_H
#  include "../../config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
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
                                         const gchar **packet,
                                         gsize *packet_size,
                                         MilterOption *option)
{
    MilterEncoder *base_encoder;

    base_encoder = MILTER_ENCODER(encoder);
    milter_encoder_encode_negotiate(base_encoder, option);
    milter_encoder_pack(base_encoder, packet, packet_size);
}

static gint
compare_macro_key (gconstpointer a, gconstpointer b)
{
    const gchar *key1 = a;
    const gchar *key2 = b;

    if (key1[0] == '{')
        key1++;
    if (key2[0] == '{')
        key2++;
    return g_utf8_collate(key1, key2);
}

static void
encode_macros (GHashTable *macros, GString *buffer)
{
    GList *keys, *node;

    keys = milter_utils_hash_table_get_keys(macros);
    keys = g_list_sort(keys, compare_macro_key);
    for (node = keys; node; node = g_list_next(node)) {
        gchar *key = node->data;
        gchar *value;

        if (key[0] == '\0')
            continue;
        if (key[0] == '{' || key[1] == '\0') {
            g_string_append(buffer, key);
        } else {
            g_string_append_printf(buffer, "{%s}", key);
        }
        g_string_append_c(buffer, '\0');

        value = g_hash_table_lookup(macros, key);
        if (value)
            g_string_append(buffer, value);
        g_string_append_c(buffer, '\0');
    }
    g_list_free(keys);
}

void
milter_command_encoder_encode_define_macro (MilterCommandEncoder *encoder,
                                            const gchar **packet,
                                            gsize *packet_size,
                                            MilterCommand context,
                                            GHashTable *macros)
{
    MilterEncoder *base_encoder;
    GString *buffer;

    base_encoder = MILTER_ENCODER(encoder);
    milter_encoder_clear_buffer(base_encoder);
    buffer = milter_encoder_get_buffer(base_encoder);

    g_string_append_c(buffer, MILTER_COMMAND_DEFINE_MACRO);
    g_string_append_c(buffer, context);
    if (macros) {
        encode_macros(macros, buffer);
    }
    milter_encoder_pack(base_encoder, packet, packet_size);
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
    g_string_append_printf(buffer, "IPv6:%s", inet6_address);
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

static void
encode_connect_unknown (GString *buffer, const struct sockaddr *address)
{
    g_string_append_c(buffer, MILTER_SOCKET_FAMILY_UNKNOWN);
}

void
milter_command_encoder_encode_connect (MilterCommandEncoder *encoder,
                                       const gchar **packet,
                                       gsize *packet_size,
                                       const gchar *host_name,
                                       const struct sockaddr *address,
                                       socklen_t address_size)
{
    MilterEncoder *base_encoder;
    GString *buffer;
    gboolean need_last_null = TRUE;

    base_encoder = MILTER_ENCODER(encoder);
    milter_encoder_clear_buffer(base_encoder);
    buffer = milter_encoder_get_buffer(base_encoder);

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
        encode_connect_unknown(buffer, address);
        need_last_null = FALSE;
        break;
    }

    if (need_last_null)
        g_string_append_c(buffer, '\0');
    milter_encoder_pack(base_encoder, packet, packet_size);
}

void
milter_command_encoder_encode_helo (MilterCommandEncoder *encoder,
                                    const gchar **packet,
                                    gsize *packet_size,
                                    const gchar *fqdn)
{
    MilterEncoder *base_encoder;
    GString *buffer;

    base_encoder = MILTER_ENCODER(encoder);
    milter_encoder_clear_buffer(base_encoder);
    buffer = milter_encoder_get_buffer(base_encoder);

    g_string_append_c(buffer, MILTER_COMMAND_HELO);
    g_string_append(buffer, fqdn);
    g_string_append_c(buffer, '\0');
    milter_encoder_pack(base_encoder, packet, packet_size);
}

void
milter_command_encoder_encode_envelope_from (MilterCommandEncoder *encoder,
                                             const gchar **packet,
                                             gsize *packet_size,
                                             const gchar *from)
{
    MilterEncoder *base_encoder;
    GString *buffer;

    base_encoder = MILTER_ENCODER(encoder);
    milter_encoder_clear_buffer(base_encoder);
    buffer = milter_encoder_get_buffer(base_encoder);

    g_string_append_c(buffer, MILTER_COMMAND_ENVELOPE_FROM);
    g_string_append(buffer, from);
    g_string_append_c(buffer, '\0');
    milter_encoder_pack(base_encoder, packet, packet_size);
}

void
milter_command_encoder_encode_envelope_recipient (MilterCommandEncoder *encoder,
                                                  const gchar **packet,
                                                  gsize *packet_size,
                                                  const gchar *to)
{
    MilterEncoder *base_encoder;
    GString *buffer;

    base_encoder = MILTER_ENCODER(encoder);
    milter_encoder_clear_buffer(base_encoder);
    buffer = milter_encoder_get_buffer(base_encoder);

    g_string_append_c(buffer, MILTER_COMMAND_ENVELOPE_RECIPIENT);
    g_string_append(buffer, to);
    g_string_append_c(buffer, '\0');
    milter_encoder_pack(base_encoder, packet, packet_size);
}

void
milter_command_encoder_encode_data (MilterCommandEncoder *encoder,
                                    const gchar **packet,
                                    gsize *packet_size)
{
    MilterEncoder *base_encoder;
    GString *buffer;

    base_encoder = MILTER_ENCODER(encoder);
    milter_encoder_clear_buffer(base_encoder);
    buffer = milter_encoder_get_buffer(base_encoder);

    g_string_append_c(buffer, MILTER_COMMAND_DATA);
    milter_encoder_pack(base_encoder, packet, packet_size);
}

void
milter_command_encoder_encode_header (MilterCommandEncoder *encoder,
                                      const gchar **packet,
                                      gsize *packet_size,
                                      const gchar *name, const gchar *value)
{
    MilterEncoder *base_encoder;
    GString *buffer;

    base_encoder = MILTER_ENCODER(encoder);
    milter_encoder_clear_buffer(base_encoder);
    buffer = milter_encoder_get_buffer(base_encoder);

    g_string_append_c(buffer, MILTER_COMMAND_HEADER);
    g_string_append(buffer, name);
    g_string_append_c(buffer, '\0');
    g_string_append(buffer, value);
    g_string_append_c(buffer, '\0');
    milter_encoder_pack(base_encoder, packet, packet_size);
}

void
milter_command_encoder_encode_end_of_header (MilterCommandEncoder *encoder,
                                             const gchar **packet,
                                             gsize *packet_size)
{
    MilterEncoder *base_encoder;
    GString *buffer;

    base_encoder = MILTER_ENCODER(encoder);
    milter_encoder_clear_buffer(base_encoder);
    buffer = milter_encoder_get_buffer(base_encoder);

    g_string_append_c(buffer, MILTER_COMMAND_END_OF_HEADER);
    milter_encoder_pack(base_encoder, packet, packet_size);
}

void
milter_command_encoder_encode_body (MilterCommandEncoder *encoder,
                                    const gchar **packet,
                                    gsize *packet_size,
                                    const gchar *chunk, gsize size,
                                    gsize *packed_size)
{
    MilterEncoder *base_encoder;
    GString *buffer;
    gsize packed_chunk_size;

    base_encoder = MILTER_ENCODER(encoder);
    milter_encoder_clear_buffer(base_encoder);
    buffer = milter_encoder_get_buffer(base_encoder);

    g_string_append_c(buffer, MILTER_COMMAND_BODY);
    if (size > MILTER_CHUNK_SIZE)
        packed_chunk_size = MILTER_CHUNK_SIZE;
    else
        packed_chunk_size = size;
    g_string_append_len(buffer, chunk, packed_chunk_size);
    milter_encoder_pack(base_encoder, packet, packet_size);

    if (packed_size)
        *packed_size = packed_chunk_size;
}

void
milter_command_encoder_encode_end_of_message (MilterCommandEncoder *encoder,
                                              const gchar **packet,
                                              gsize *packet_size,
                                              const gchar *chunk, gsize size)
{
    MilterEncoder *base_encoder;
    GString *buffer;

    base_encoder = MILTER_ENCODER(encoder);
    milter_encoder_clear_buffer(base_encoder);
    buffer = milter_encoder_get_buffer(base_encoder);

    g_string_append_c(buffer, MILTER_COMMAND_END_OF_MESSAGE);
    if (chunk && size > 0)
        g_string_append_len(buffer, chunk, size);
    milter_encoder_pack(base_encoder, packet, packet_size);
}

void
milter_command_encoder_encode_abort (MilterCommandEncoder *encoder,
                                     const gchar **packet,
                                     gsize *packet_size)
{
    MilterEncoder *base_encoder;
    GString *buffer;

    base_encoder = MILTER_ENCODER(encoder);
    milter_encoder_clear_buffer(base_encoder);
    buffer = milter_encoder_get_buffer(base_encoder);

    g_string_append_c(buffer, MILTER_COMMAND_ABORT);
    milter_encoder_pack(base_encoder, packet, packet_size);
}

void
milter_command_encoder_encode_quit (MilterCommandEncoder *encoder,
                                    const gchar **packet,
                                    gsize *packet_size)
{
    MilterEncoder *base_encoder;
    GString *buffer;

    base_encoder = MILTER_ENCODER(encoder);
    milter_encoder_clear_buffer(base_encoder);
    buffer = milter_encoder_get_buffer(base_encoder);

    g_string_append_c(buffer, MILTER_COMMAND_QUIT);
    milter_encoder_pack(base_encoder, packet, packet_size);
}

void
milter_command_encoder_encode_unknown (MilterCommandEncoder *encoder,
                                       const gchar **packet,
                                       gsize *packet_size,
                                       const gchar *command)
{
    MilterEncoder *base_encoder;
    GString *buffer;

    base_encoder = MILTER_ENCODER(encoder);
    milter_encoder_clear_buffer(base_encoder);
    buffer = milter_encoder_get_buffer(base_encoder);

    g_string_append_c(buffer, MILTER_COMMAND_UNKNOWN);
    g_string_append(buffer, command);
    g_string_append_c(buffer, '\0');
    milter_encoder_pack(base_encoder, packet, packet_size);
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
