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
#include <sys/un.h>
#include <milter-core/milter-utils.h>
#undef shutdown

void test_parse_connection_spec_unix (void);

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

void
test_parse_connection_spec_unix (void)
{
    struct sockaddr_un *address;

    milter_utils_parse_connection_spec("unix:/tmp/xxx.sock",
                                       &actual_address, &actual_address_size,
                                       &actual_error);
    gcut_assert_error(actual_error);
    cut_assert_equal_uint(sizeof(address), actual_address_size);

    address = (struct sockaddr_un *)actual_address;
    cut_assert_equal_int(AF_UNIX, address->sun_family);
    cut_assert_equal_string("/tmp/xxx.sock", address->sun_path);
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
