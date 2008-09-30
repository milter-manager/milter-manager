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
        *address_size = sizeof(address_unix);
    } else if (g_str_has_prefix(spec, "inet:")) {
        struct sockaddr_in *address_inet;
        struct in_addr ip_address;
        const gchar *port;
        gint i, port_number;

        port = colon + 1;
        ip_address.s_addr = INADDR_ANY;
        for (i = 0; port[i]; i++) {
            if (port[i] == '@') {
                const gchar *host;

                host = port + i + 1;
                if (host[0] == '[') {
                    gsize host_size;
                    gchar *host_address;
                    gint result;

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
                    result = inet_aton(host_address, &ip_address);
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
                    hints.ai_family = PF_INET;
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
                    ip_address =
                        ((struct sockaddr_in *)(results->ai_addr))->sin_addr;
                    freeaddrinfo(results);
                }
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
        port_number = atoi(port);
        if (port_number > 65535) {
            g_set_error(error,
                        MILTER_UTILS_ERROR,
                        MILTER_UTILS_ERROR_INVALID_FORMAT,
                        "port number should be less than 65536: <%s>: <%d>",
                        spec, port_number);
            return FALSE;
        }

        address_inet = g_new(struct sockaddr_in, 1);
        address_inet->sin_family = AF_INET;
        address_inet->sin_port = htons(port_number);
        address_inet->sin_addr = ip_address;

        *address = (struct sockaddr *)address_inet;
        *address_size = sizeof(address_inet);
    } else {
        return FALSE;
    }

    return TRUE;
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
