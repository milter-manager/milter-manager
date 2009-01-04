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
 *  along with this library.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <gcutter.h>

#define shutdown inet_shutdown
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <arpa/inet.h>
#undef shutdown

#include <milter/core/milter-macros-requests.h>
#include <milter/core/milter-enum-types.h>
#include "milter-test-utils.h"

void test_symbols_new (void);
void data_merge (void);
void test_merge (gconstpointer data);

static MilterMacrosRequests *requests;
static MilterMacrosRequests *another_requests;
static MilterMacrosRequests *expected_requests;
static GHashTable *expected_symbols_table;

void
setup (void)
{
    requests = NULL;
    another_requests = NULL;
    expected_requests = NULL;

    expected_symbols_table = NULL;
}

void
teardown (void)
{
    if (requests)
        g_object_unref(requests);
    if (another_requests)
        g_object_unref(another_requests);
    if (expected_requests)
        g_object_unref(expected_requests);

    if (expected_symbols_table)
        g_hash_table_unref(expected_symbols_table);
}

void
test_symbols_new (void)
{
    GHashTable *table;

    requests = milter_macros_requests_new();
    g_object_get(requests, "symbols-table", &table, NULL);
    expected_symbols_table = g_hash_table_new(g_int_hash, g_int_equal);
    milter_assert_equal_symbols_table(expected_symbols_table, table);
}

static void
setup_merge_same_command_data (void)
{
    requests = milter_macros_requests_new();
    milter_macros_requests_set_symbols(requests,
                                       MILTER_COMMAND_END_OF_HEADER,
                                       "first-name",
                                       "third-name",
                                       NULL);

    another_requests = milter_macros_requests_new();
    milter_macros_requests_set_symbols(another_requests,
                                       MILTER_COMMAND_END_OF_HEADER,
                                       "first-name",
                                       "second-name",
                                       "forth-name",
                                       NULL);

    expected_requests = milter_macros_requests_new();
    milter_macros_requests_set_symbols(expected_requests,
                                       MILTER_COMMAND_END_OF_HEADER,
                                       "first-name",
                                       "third-name",
                                       "second-name",
                                       "forth-name",
                                       NULL);
}

static void
setup_merge_different_command_data (void)
{
    requests = milter_macros_requests_new();
    milter_macros_requests_set_symbols(requests,
                                       MILTER_COMMAND_END_OF_HEADER,
                                       "first-name",
                                       "third-name",
                                       NULL);

    another_requests = milter_macros_requests_new();
    milter_macros_requests_set_symbols(another_requests,
                                       MILTER_COMMAND_END_OF_MESSAGE,
                                       "first-name",
                                       "second-name",
                                       "forth-name",
                                       NULL);

    expected_requests = milter_macros_requests_new();
    milter_macros_requests_set_symbols(expected_requests,
                                       MILTER_COMMAND_END_OF_HEADER,
                                       "first-name",
                                       "third-name",
                                       NULL);
    milter_macros_requests_set_symbols(expected_requests,
                                       MILTER_COMMAND_END_OF_MESSAGE,
                                       "first-name",
                                       "second-name",
                                       "forth-name",
                                       NULL);
}

static void
setup_merge_same_command_empty_source_data (void)
{
    requests = milter_macros_requests_new();
    milter_macros_requests_set_symbols(requests,
                                       MILTER_COMMAND_END_OF_HEADER,
                                       "first-name",
                                       "third-name",
                                       NULL);

    another_requests = milter_macros_requests_new();

    expected_requests = milter_macros_requests_new();
    milter_macros_requests_set_symbols(expected_requests,
                                       MILTER_COMMAND_END_OF_HEADER,
                                       "first-name",
                                       "third-name",
                                       NULL);
}

static void
setup_merge_same_command_empty_destination_data (void)
{
    requests = milter_macros_requests_new();

    another_requests = milter_macros_requests_new();
    milter_macros_requests_set_symbols(another_requests,
                                       MILTER_COMMAND_END_OF_HEADER,
                                       "first-name",
                                       "second-name",
                                       "forth-name",
                                       NULL);

    expected_requests = milter_macros_requests_new();
    milter_macros_requests_set_symbols(expected_requests,
                                       MILTER_COMMAND_END_OF_HEADER,
                                       "first-name",
                                       "second-name",
                                       "forth-name",
                                       NULL);
}

void
data_merge (void)
{
    cut_add_data("same command", setup_merge_same_command_data, NULL,
                 "same command - empty destination",
                 setup_merge_same_command_empty_destination_data, NULL,
                 "same command - empty source",
                 setup_merge_same_command_empty_source_data, NULL,
                 "different command", setup_merge_different_command_data, NULL);
}

void
test_merge (gconstpointer data)
{
    GCallback setup_func = (GCallback)data;

    setup_func();

    milter_macros_requests_merge(requests, another_requests);
    milter_assert_equal_macros_requests(expected_requests, requests);
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
