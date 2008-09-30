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

#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "milter-utils.h"

GQuark
milter_utils_error_quark (void)
{
    return g_quark_from_static_string("milter-utils-error-quark");
}

static gboolean
parse_connection_spec_host (const gchar *spec, const gchar *host,
                            gpointer address, gint protocol_family,
                            GError **error)
{
    if (host[0] == '[') {
        gsize host_size;
        gchar *host_address;
        gint result, address_family;

        host++;
        host_size = strlen(host);
        if (host[host_size - 1] != ']') {
            g_set_error(error,
                        MILTER_UTILS_ERROR,
                        MILTER_UTILS_ERROR_INVALID_FORMAT,
                        "']' is missing: <%s>: <%s>",
                        spec, host - 1);
            return FALSE;
        }
        host_address = g_strndup(host, host_size - 1);

        if (protocol_family == PF_INET)
            address_family = AF_INET;
        else
            address_family = AF_INET6;
        result = inet_pton(address_family, host_address, address);
        if (result == 0) {
            g_set_error(error,
                        MILTER_UTILS_ERROR,
                        MILTER_UTILS_ERROR_INVALID_FORMAT,
                        "invalid host address: <%s>: <%s>",
                        spec, host_address);
        }
        g_free(host_address);
        if (result == 0)
            return FALSE;
    } else {
        struct addrinfo hints;
        struct addrinfo *results;
        gint status;

        memset(&hints, 0, sizeof(hints));
        hints.ai_family = protocol_family;
        hints.ai_socktype = SOCK_STREAM;

        status = getaddrinfo(host, NULL, &hints, &results);
        if (status != 0) {
            g_set_error(error,
                        MILTER_UTILS_ERROR,
                        MILTER_UTILS_ERROR_INVALID_FORMAT,
                        "invalid host address: <%s>: <%s>: <%s>",
                        spec, host, gai_strerror(status));
            return FALSE;
        }

        if (protocol_family == PF_INET) {
            memcpy(address,
                   &(((struct sockaddr_in *)(results->ai_addr))->sin_addr),
                   sizeof(struct in_addr));
        } else {
            memcpy(address,
                   &(((struct sockaddr_in6 *)(results->ai_addr))->sin6_addr),
                   sizeof(struct in6_addr));
        }
        freeaddrinfo(results);
    }

    return TRUE;
}

static gboolean
parse_connection_spec_port_and_host (const gchar *spec, const gchar *port,
                                     gint *port_number,
                                     gpointer address, gint protocol_family,
                                     GError **error)
{
    struct in_addr *ip_address;
    struct in6_addr *ipv6_address;
    gint i;

    if (protocol_family == PF_INET) {
        ip_address = address;
        ip_address->s_addr = INADDR_ANY;
    } else {
        ipv6_address = address;
        *ipv6_address = in6addr_any;
    }

    for (i = 0; port[i]; i++) {
        if (port[i] == '@') {
            if (!parse_connection_spec_host(spec, port + i + 1,
                                            address, protocol_family, error))
                return FALSE;
            break;
        } else if (!g_ascii_isdigit(port[i])) {
            g_set_error(error,
                        MILTER_UTILS_ERROR,
                        MILTER_UTILS_ERROR_INVALID_FORMAT,
                        "port number has invalid character: <%s>: <%c>",
                        spec, port[i]);
            return FALSE;
        }
    }

    *port_number = atoi(port);
    if (*port_number > 65535) {
        g_set_error(error,
                    MILTER_UTILS_ERROR,
                    MILTER_UTILS_ERROR_INVALID_FORMAT,
                    "port number should be less than 65536: <%s>: <%d>",
                    spec, *port_number);
        return FALSE;
    }

    return TRUE;
}

gboolean
milter_utils_parse_connection_spec (const gchar      *spec,
                                    struct sockaddr **address,
                                    socklen_t        *address_size,
                                    GError          **error)
{
    const gchar *colon;

    colon = strstr(spec, ":");
    if (!colon) {
        g_set_error(error,
                    MILTER_UTILS_ERROR,
                    MILTER_UTILS_ERROR_INVALID_FORMAT,
                    "spec doesn't have colon: <%s>", spec);
        return FALSE;
    }

    if (g_str_has_prefix(spec, "unix:")) {
        struct sockaddr_un *address_unix;

        address_unix = g_new(struct sockaddr_un, 1);
        address_unix->sun_family = AF_UNIX;
        strcpy(address_unix->sun_path, colon + 1);

        *address = (struct sockaddr *)address_unix;
        *address_size = sizeof(*address_unix);
    } else if (g_str_has_prefix(spec, "inet:")) {
        struct sockaddr_in *address_inet;
        struct in_addr ip_address;
        gint port_number;

        if (!parse_connection_spec_port_and_host(spec, colon + 1,
                                                 &port_number,
                                                 &ip_address, PF_INET,
                                                 error))
            return FALSE;

        address_inet = g_new(struct sockaddr_in, 1);
        address_inet->sin_family = AF_INET;
        address_inet->sin_port = htons(port_number);
        address_inet->sin_addr = ip_address;

        *address = (struct sockaddr *)address_inet;
        *address_size = sizeof(*address_inet);
    } else if (g_str_has_prefix(spec, "inet6:")) {
        struct sockaddr_in6 *address_inet6;
        struct in6_addr ipv6_address;
        gint port_number;

        if (!parse_connection_spec_port_and_host(spec, colon + 1,
                                                 &port_number,
                                                 &ipv6_address, PF_INET6,
                                                 error))
            return FALSE;

        address_inet6 = g_new(struct sockaddr_in6, 1);
        address_inet6->sin6_family = AF_INET6;
        address_inet6->sin6_port = htons(port_number);
        address_inet6->sin6_addr = ipv6_address;

        *address = (struct sockaddr *)address_inet6;
        *address_size = sizeof(*address_inet6);
    } else {
        gchar *protocol;

        protocol = g_strndup(spec, colon - spec);
        g_set_error(error,
                    MILTER_UTILS_ERROR,
                    MILTER_UTILS_ERROR_INVALID_FORMAT,
                    "protocol must be 'unix', 'inet' or 'inet6': <%s>: <%s>",
                    spec, protocol);
        g_free(protocol);
        return FALSE;
    }

    return TRUE;
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
