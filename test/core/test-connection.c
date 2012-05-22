/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Copyright (C) 2008-2012  Kouhei Sutou <kou@clear-code.com>
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

#include <string.h>
#include <errno.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <glib/gstdio.h>

#include <milter/core/milter-connection.h>
#include <milter-test-utils.h>

#include <gcutter.h>

void test_parse_connection_spec_null (void);
void data_parse_connection_spec_unix (void);
void test_parse_connection_spec_unix (gconstpointer data);
void data_parse_connection_spec_inet (void);
void test_parse_connection_spec_inet (gconstpointer data);
void data_parse_connection_spec_inet6 (void);
void test_parse_connection_spec_inet6 (gconstpointer data);
void data_address_to_spec (void);
void test_address_to_spec (gconstpointer data);
void test_listen (void);
void test_listen_exist_socket (void);
void test_listen_remove_failure (void);
void test_listen_nonexistent_path (void);

static struct sockaddr *actual_address;
static socklen_t actual_address_size;

static GError *expected_error;
static GError *actual_error;

static gchar *tmp_dir;

void
setup (void)
{
    actual_address = NULL;
    actual_address_size = 0;

    expected_error = NULL;
    actual_error = NULL;

    tmp_dir = g_build_filename(milter_test_get_base_dir(),
                               "tmp",
                               NULL);
    cut_remove_path(tmp_dir, NULL);
    if (g_mkdir_with_parents(tmp_dir, 0700) == -1)
        cut_assert_errno();
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

    if (tmp_dir) {
        g_chmod(tmp_dir, 0755);
        cut_remove_path(tmp_dir, NULL);
        g_free(tmp_dir);
    }
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

    address = g_new0(struct sockaddr_un, 1);
    address->sun_family = AF_UNIX;
    memcpy(address->sun_path, path, strlen(path));

    return (struct sockaddr *)address;
}

static struct sockaddr *
sockaddr_in_new (guint16 port, const gchar *host)
{
    struct sockaddr_in *address;
    struct in_addr ip_address;

    if (host) {
        if (inet_pton(AF_INET, host, &ip_address) == 0) {
            cut_error("invalid host address: <%s>", host);
        }
    } else {
        ip_address.s_addr = INADDR_ANY;
    }

    address = g_new0(struct sockaddr_in, 1);
    address->sin_family = AF_INET;
    address->sin_port = g_htons(port);
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

    address = g_new0(struct sockaddr_in6, 1);
    address->sin6_family = AF_INET6;
    address->sin6_port = g_htons(port);
    address->sin6_addr = ipv6_address;

    return (struct sockaddr *)address;
}

static struct sockaddr *
sockaddr_unknown_new (void)
{
    struct sockaddr *address;

    address = g_new0(struct sockaddr, 1);
    address->sa_family = AF_UNSPEC;

    return address;
}

static struct sockaddr *
sockaddr_unexpected_new (int family)
{
    struct sockaddr *address;

    address = g_new0(struct sockaddr, 1);
    address->sa_family = family;

    return address;
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
test_parse_connection_spec_null (void)
{
    gint domain;

    expected_error = g_error_new(MILTER_CONNECTION_ERROR,
                                 MILTER_CONNECTION_ERROR_INVALID_FORMAT,
                                 "spec should not be NULL");
    milter_connection_parse_spec(NULL,
                                 &domain, &actual_address, &actual_address_size,
                                 &actual_error);
    gcut_assert_equal_error(expected_error, actual_error);
}

void
data_parse_connection_spec_unix (void)
{
    cut_add_data("normal",
                 test_data_new("unix:/tmp/xxx.sock",
                               sockaddr_un_new("/tmp/xxx.sock"),
                               NULL),
                 test_data_free,
                 "normal - local",
                 test_data_new("local:/tmp/xxx.sock",
                               sockaddr_un_new("/tmp/xxx.sock"),
                               NULL),
                 test_data_free,
                 "no path",
                 test_data_new("unix:",
                               NULL,
                               g_error_new(
                                   MILTER_CONNECTION_ERROR,
                                   MILTER_CONNECTION_ERROR_INVALID_FORMAT,
                                   "spec doesn't have content: <unix:>")),
                 test_data_free,
                 "no path - local",
                 test_data_new("local:",
                               NULL,
                               g_error_new(
                                   MILTER_CONNECTION_ERROR,
                                   MILTER_CONNECTION_ERROR_INVALID_FORMAT,
                                   "spec doesn't have content: <local:>")),
                 test_data_free,
                 NULL);
}

void
test_parse_connection_spec_unix (gconstpointer data)
{
    const TestData *test_data = data;
    struct sockaddr_un *expected_address;
    struct sockaddr_un *address;
    GError *error = NULL;
    gint domain;

    milter_connection_parse_spec(test_data->spec,
                                 &domain, &actual_address, &actual_address_size,
                                 &error);
    if (test_data->expected_error) {
        actual_error = error;
        gcut_assert_equal_error(test_data->expected_error, error);
        return;
    }

    gcut_assert_error(error);
    cut_assert_equal_int(PF_UNIX, domain);
    cut_assert_equal_uint(sizeof(*address), actual_address_size);

    expected_address = (struct sockaddr_un *)test_data->expected_address;
    address = (struct sockaddr_un *)actual_address;
    cut_assert_equal_int(expected_address->sun_family, address->sun_family);
    cut_assert_equal_string(expected_address->sun_path, address->sun_path);
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
                 test_data_free,
                 "no content",
                 test_data_new("inet:",
                               NULL,
                               g_error_new(
                                   MILTER_CONNECTION_ERROR,
                                   MILTER_CONNECTION_ERROR_INVALID_FORMAT,
                                   "spec doesn't have content: <inet:>")),
                 test_data_free,
                 NULL);
}

void
test_parse_connection_spec_inet (gconstpointer data)
{
    const TestData *test_data = data;
    gint domain;
    struct sockaddr_in *expected_in, *actual_in;
    gchar expected_address_string[INET6_ADDRSTRLEN];
    gchar actual_address_string[INET6_ADDRSTRLEN];
    GError *error = NULL;

    milter_connection_parse_spec(test_data->spec,
                                 &domain, &actual_address, &actual_address_size,
                                 &error);
    if (test_data->expected_error) {
        actual_error = error;
        gcut_assert_equal_error(test_data->expected_error, actual_error);
        return;
    }

    gcut_assert_error(error);
    cut_assert_equal_int(PF_INET, domain);
    cut_assert_equal_uint(sizeof(*expected_in), actual_address_size);

    expected_in = (struct sockaddr_in *)test_data->expected_address;
    actual_in = (struct sockaddr_in *)actual_address;
    cut_assert_equal_int(expected_in->sin_family,
                         actual_in->sin_family);
    cut_assert_equal_uint(g_ntohs(expected_in->sin_port),
                          g_ntohs(actual_in->sin_port));
    cut_assert_equal_string(
        inet_ntop(AF_INET, &(expected_in->sin_addr),
                  expected_address_string, sizeof(expected_address_string)),
        inet_ntop(AF_INET, &(actual_in->sin_addr),
                  actual_address_string, sizeof(actual_address_string)));
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
                 test_data_free,
                 "no content",
                 test_data_new("inet6:",
                               NULL,
                               g_error_new(
                                   MILTER_CONNECTION_ERROR,
                                   MILTER_CONNECTION_ERROR_INVALID_FORMAT,
                                   "spec doesn't have content: <inet6:>")),
                 test_data_free);
}

void
test_parse_connection_spec_inet6 (gconstpointer data)
{
    const TestData *test_data = data;
    gint domain;
    struct sockaddr_in6 *expected_in6, *actual_in6;
    gchar expected_address_string[INET6_ADDRSTRLEN];
    gchar actual_address_string[INET6_ADDRSTRLEN];
    GError *error = NULL;

    milter_connection_parse_spec(test_data->spec,
                                 &domain, &actual_address, &actual_address_size,
                                 &error);
    if (test_data->expected_error) {
        actual_error = error;
        gcut_assert_equal_error(test_data->expected_error, actual_error);
        return;
    }

    gcut_assert_error(error);
    cut_assert_equal_int(PF_INET6, domain);
    cut_assert_equal_uint(sizeof(*expected_in6), actual_address_size);

    expected_in6 = (struct sockaddr_in6 *)test_data->expected_address;
    actual_in6 = (struct sockaddr_in6 *)actual_address;
    cut_assert_equal_int(expected_in6->sin6_family,
                         actual_in6->sin6_family);
    cut_assert_equal_uint(g_ntohs(expected_in6->sin6_port),
                          g_ntohs(actual_in6->sin6_port));
    cut_assert_equal_string(inet_ntop(AF_INET6, &(expected_in6->sin6_addr),
                                      expected_address_string,
                                      sizeof(expected_address_string)),
                            inet_ntop(AF_INET6, &(actual_in6->sin6_addr),
                                      actual_address_string,
                                      sizeof(actual_address_string)));
}

void
data_address_to_spec (void)
{
    cut_add_data("inet6",
                 test_data_new("inet6:9999@[::1]",
                               sockaddr_in6_new(9999, "::1"),
                               NULL),
                 test_data_free,
                 "inet6 no host",
                 test_data_new("inet6:9999@[::]",
                               sockaddr_in6_new(9999, "::0"),
                               NULL),
                 test_data_free,
                 "inet",
                 test_data_new("inet:9999@[127.0.0.1]",
                               sockaddr_in_new(9999, "127.0.0.1"),
                               NULL),
                 test_data_free,
                 "inet no host",
                 test_data_new("inet:9999@[0.0.0.0]",
                               sockaddr_in_new(9999, NULL),
                               NULL),
                 test_data_free,
                 "unix",
                 test_data_new("unix:/tmp/xxx.sock",
                               sockaddr_un_new("/tmp/xxx.sock"),
                               NULL),
                 test_data_free,
                 "unknown",
                 test_data_new("unknown",
                               sockaddr_unknown_new(),
                               NULL),
                 test_data_free,
                 "unexpected",
                 test_data_new("unexpected:100",
                               sockaddr_unexpected_new(100),
                               NULL),
                 test_data_free,
                 "no content",
                 test_data_new(NULL,
                               NULL,
                               NULL),
                 test_data_free,
                 NULL);
}

void
test_address_to_spec (gconstpointer data)
{
    const TestData *test_data = data;
    gchar *actual_address_string;

    actual_address_string =
        milter_connection_address_to_spec(test_data->expected_address);
    cut_take_string(actual_address_string);

    cut_assert_equal_string(test_data->spec, actual_address_string);
}

void
test_listen (void)
{
    const gchar *socket_path;
    const gchar *spec;
    GError *error = NULL;
    struct sockaddr_un *address_un = NULL;

    socket_path = cut_take_printf("%s/milter.sock", tmp_dir);
    spec = cut_take_printf("unix:%s", socket_path);
    cut_assert_true(milter_connection_listen(spec, 5,
                                             &actual_address,
                                             &actual_address_size,
                                             TRUE,
                                             &error));
    gcut_assert_error(error);

    cut_assert_not_null(actual_address);

    address_un = (struct sockaddr_un *)actual_address;
    cut_assert_equal_uint(AF_UNIX, address_un->sun_family);
    cut_assert_equal_string(socket_path, address_un->sun_path);
    cut_assert_equal_int(sizeof(*address_un), actual_address_size);
}

void
test_listen_exist_socket (void)
{
    const gchar *socket_path;
    const gchar *spec;
    GError *error = NULL;
    struct sockaddr_un *address_un = NULL;

    socket_path = cut_take_printf("%s/milter.sock", tmp_dir);
    spec = cut_take_printf("unix:%s", socket_path);
    g_file_set_contents(socket_path, "", 0, &error);
    gcut_assert_error(error);

    expected_error = g_error_new(MILTER_CONNECTION_ERROR,
                                 MILTER_CONNECTION_ERROR_BIND_FAILURE,
                                 "failed to bind(): %s: %s",
                                 spec,
                                 g_strerror(EADDRINUSE));
    cut_assert_false(milter_connection_listen(spec, 5,
                                              &actual_address,
                                              &actual_address_size,
                                              FALSE,
                                              &actual_error));
    gcut_assert_equal_error(expected_error, actual_error);
    cut_assert_null(actual_address);
    cut_assert_equal_uint(0, actual_address_size);

    cut_assert_true(milter_connection_listen(spec, 5,
                                             &actual_address,
                                             &actual_address_size,
                                             TRUE,
                                             &error));
    gcut_assert_error(error);
    cut_assert_not_null(actual_address);

    address_un = (struct sockaddr_un *)actual_address;
    cut_assert_equal_uint(AF_UNIX, address_un->sun_family);
    cut_assert_equal_string(socket_path, address_un->sun_path);
    cut_assert_equal_int(sizeof(*address_un), actual_address_size);
}

void
test_listen_remove_failure (void)
{
    const gchar *socket_path;
    const gchar *spec;
    GError *error = NULL;
    struct sockaddr_un *address_un = NULL;

    socket_path = cut_take_printf("%s/milter.sock", tmp_dir);
    spec = cut_take_printf("unix:%s", socket_path);
    g_file_set_contents(socket_path, "", 0, &error);
    gcut_assert_error(error);
    if (g_chmod(tmp_dir, 0500))
        cut_assert_errno();

    expected_error = g_error_new(MILTER_CONNECTION_ERROR,
                                 MILTER_CONNECTION_ERROR_UNIX_SOCKET,
                                 "can't remove existing socket: <%s>: <%s>: %s",
                                 socket_path,
                                 spec,
                                 g_strerror(EACCES));
    cut_assert_false(milter_connection_listen(spec, 5,
                                              &actual_address,
                                              &actual_address_size,
                                              TRUE,
                                              &actual_error));
    gcut_assert_equal_error(expected_error, actual_error);
    cut_assert_null(actual_address);
    cut_assert_equal_uint(0, actual_address_size);

    if (g_chmod(tmp_dir, 0755))
        cut_assert_errno();
    cut_assert_true(milter_connection_listen(spec, 5,
                                             &actual_address,
                                             &actual_address_size,
                                             TRUE,
                                             &error));
    gcut_assert_error(error);
    cut_assert_not_null(actual_address);

    address_un = (struct sockaddr_un *)actual_address;
    cut_assert_equal_uint(AF_UNIX, address_un->sun_family);
    cut_assert_equal_string(socket_path, address_un->sun_path);
    cut_assert_equal_int(sizeof(*address_un), actual_address_size);
}

void
test_listen_nonexistent_path (void)
{
    const gchar *spec;
    struct sockaddr *address = NULL;
    socklen_t address_size = 0;

    spec = cut_take_printf("unix:%s/nonexistent/milter.sock", tmp_dir);
    expected_error = g_error_new(MILTER_CONNECTION_ERROR,
                                 MILTER_CONNECTION_ERROR_BIND_FAILURE,
                                 "failed to bind(): %s: %s",
                                 spec,
                                 g_strerror(ENOENT));

    cut_assert_false(milter_connection_listen(spec, 5, &address, &address_size,
                                              TRUE, &actual_error));
    gcut_assert_equal_error(expected_error, actual_error);

    cut_assert_null(address);
    cut_assert_equal_int(0, address_size);
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
