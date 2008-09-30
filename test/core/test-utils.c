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

#include <gcutter.h>

#define shutdown inet_shutdown
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <milter-core/milter-utils.h>
#undef shutdown

void test_parse_connection_spec_unix (void);
void data_parse_connection_spec_inet (void);
void test_parse_connection_spec_inet (gconstpointer data);
void data_parse_connection_spec_inet6 (void);
void test_parse_connection_spec_inet6 (gconstpointer data);

static struct sockaddr *actual_address;
static socklen_t actual_address_size;

static GError *expected_error;
static GError *actual_error;

void
setup (void)
{
    actual_address = NULL;
    actual_address_size = 0;

    expected_error = NULL;
    actual_error = NULL;
}

void
teardown (void)
{
    if (actual_address)
        g_free(actual_address);

    if (expected_error)
        g_error_free(expected_error);
    if (actual_error)
        g_error_free(actual_error);
}

typedef struct _TestData
{
    gchar *spec;
    struct sockaddr *expected_address;
    GError *expected_error;
} TestData;

static struct sockaddr *
sockaddr_un_new (const gchar *path)
{
    struct sockaddr_un *address;

    address = g_new(struct sockaddr_un, 1);
    address->sun_family = AF_UNIX;
    strcpy(address->sun_path, path);

    return (struct sockaddr *)address;
}

static struct sockaddr *
sockaddr_in_new (guint16 port, const gchar *host)
{
    struct sockaddr_in *address;
    struct in_addr ip_address;

    if (host) {
        if (inet_aton(host, &ip_address) == 0)
            cut_error("invalid host address: <%s>", host);
    } else {
        ip_address.s_addr = INADDR_ANY;
    }

    address = g_new(struct sockaddr_in, 1);
    address->sin_family = AF_INET;
    address->sin_port = htons(port);
    address->sin_addr = ip_address;

    return (struct sockaddr *)address;
}

static struct sockaddr *
sockaddr_in6_new (guint16 port, const gchar *host)
{
    struct sockaddr_in6 *address;
    struct in6_addr ipv6_address;

    if (host) {
        if (inet_pton(AF_INET6, host, &ipv6_address) == 0)
            cut_error("invalid host address: <%s>", host);
    } else {
        ipv6_address = in6addr_any;
    }

    address = g_new(struct sockaddr_in6, 1);
    address->sin6_family = AF_INET6;
    address->sin6_port = htons(port);
    address->sin6_addr = ipv6_address;

    return (struct sockaddr *)address;
}

static TestData *
test_data_new (const gchar *spec,
               struct sockaddr *expected_address,
               GError *expected_error)
{
    TestData *data;

    data = g_new0(TestData, 1);

    data->spec = g_strdup(spec);
    data->expected_address = expected_address;
    data->expected_error = expected_error;

    return data;
}

static void
test_data_free (TestData *data)
{
    g_free(data->spec);
    if (data->expected_address)
        g_free(data->expected_address);
    if (data->expected_error)
        g_error_free(data->expected_error);
    g_free(data);
}


void
test_parse_connection_spec_unix (void)
{
    struct sockaddr_un *address;
    GError *error = NULL;

    milter_utils_parse_connection_spec("unix:/tmp/xxx.sock",
                                       &actual_address, &actual_address_size,
                                       &error);
    gcut_assert_error(error);
    cut_assert_equal_uint(sizeof(*address), actual_address_size);

    address = (struct sockaddr_un *)actual_address;
    cut_assert_equal_int(AF_UNIX, address->sun_family);
    cut_assert_equal_string("/tmp/xxx.sock", address->sun_path);
}


void
data_parse_connection_spec_inet (void)
{
    cut_add_data("full",
                 test_data_new("inet:9999@127.0.0.1",
                               sockaddr_in_new(9999, "127.0.0.1"),
                               NULL),
                 test_data_free,
                 "full - host name",
                 test_data_new("inet:9999@localhost",
                               sockaddr_in_new(9999, "127.0.0.1"),
                               NULL),
                 test_data_free,
                 "full - bracket",
                 test_data_new("inet:9999@[127.0.0.1]",
                               sockaddr_in_new(9999, "127.0.0.1"),
                               NULL),
                 test_data_free,
                 "no host",
                 test_data_new("inet:9999",
                               sockaddr_in_new(9999, "0.0.0.0"),
                               NULL),
                 test_data_free);
}

void
test_parse_connection_spec_inet (gconstpointer data)
{
    const TestData *test_data = data;
    struct sockaddr_in *expected_in, *actual_in;
    GError *error = NULL;

    milter_utils_parse_connection_spec(test_data->spec,
                                       &actual_address, &actual_address_size,
                                       &error);
    gcut_assert_error(error);
    cut_assert_equal_uint(sizeof(*expected_in), actual_address_size);

    expected_in = (struct sockaddr_in *)test_data->expected_address;
    actual_in = (struct sockaddr_in *)actual_address;
    cut_assert_equal_int(expected_in->sin_family,
                         actual_in->sin_family);
    cut_assert_equal_uint(ntohs(expected_in->sin_port),
                          ntohs(actual_in->sin_port));
    cut_assert_equal_string(cut_take_strdup(inet_ntoa(expected_in->sin_addr)),
                            cut_take_strdup(inet_ntoa(actual_in->sin_addr)));
}


void
data_parse_connection_spec_inet6 (void)
{
    cut_add_data("full",
                 test_data_new("inet6:9999@::1",
                               sockaddr_in6_new(9999, "::1"),
                               NULL),
                 test_data_free,
                 "full - host name",
                 test_data_new("inet6:9999@localhost",
                               sockaddr_in6_new(9999, "::1"),
                               NULL),
                 test_data_free,
                 "full - bracket",
                 test_data_new("inet6:9999@[::1]",
                               sockaddr_in6_new(9999, "::1"),
                               NULL),
                 test_data_free,
                 "no host",
                 test_data_new("inet6:9999",
                               sockaddr_in6_new(9999, "::0"),
                               NULL),
                 test_data_free);
}

void
test_parse_connection_spec_inet6 (gconstpointer data)
{
    const TestData *test_data = data;
    struct sockaddr_in6 *expected_in6, *actual_in6;
    gchar expected_address_string[INET6_ADDRSTRLEN];
    gchar actual_address_string[INET6_ADDRSTRLEN];
    GError *error = NULL;

    milter_utils_parse_connection_spec(test_data->spec,
                                       &actual_address, &actual_address_size,
                                       &error);
    gcut_assert_error(error);
    cut_assert_equal_uint(sizeof(*expected_in6), actual_address_size);

    expected_in6 = (struct sockaddr_in6 *)test_data->expected_address;
    actual_in6 = (struct sockaddr_in6 *)actual_address;
    cut_assert_equal_int(expected_in6->sin6_family,
                         actual_in6->sin6_family);
    cut_assert_equal_uint(ntohs(expected_in6->sin6_port),
                          ntohs(actual_in6->sin6_port));
    cut_assert_equal_string(inet_ntop(AF_INET6, &(expected_in6->sin6_addr),
                                      expected_address_string,
                                      sizeof(expected_address_string)),
                            inet_ntop(AF_INET6, &(actual_in6->sin6_addr),
                                      actual_address_string,
                                      sizeof(actual_address_string)));
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
