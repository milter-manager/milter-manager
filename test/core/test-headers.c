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

#include <string.h>

#include <gcutter.h>

#include <milter/core/milter-headers.h>
#include "milter-test-utils.h"

void test_length (void);
void test_get_nth_header (void);
void test_find (void);
void test_lookup_by_name (void);
void test_index_in_same_header_name (void);
void test_copy (void);
void test_remove (void);
void test_add_header (void);
void test_add_header_same_name (void);
void test_append_header (void);
void test_insert_header (void);
void test_change_header (void);
void test_delete_header_with_change_header (void);
void test_delete_header (void);

static MilterHeaders *headers;
static GList *expected_list;

void
setup (void)
{
    headers = milter_headers_new();
    expected_list = NULL;
}

void
teardown (void)
{
    if (headers)
        g_object_unref(headers);
    if (expected_list) {
        g_list_foreach(expected_list, (GFunc)milter_header_free, NULL);
        g_list_free(expected_list);
    }
}

void
test_length (void)
{
    cut_assert_equal_uint(0, milter_headers_length(headers));

    cut_assert_true(milter_headers_add_header(headers,
                                              "First header",
                                              "First header value"));
    cut_assert_true(milter_headers_add_header(headers,
                                              "Second header",
                                              "Second header value"));
    cut_assert_true(milter_headers_add_header(headers,
                                              "Third header",
                                              "Third header value"));
    cut_assert_equal_uint(3, milter_headers_length(headers));
}

void
test_get_nth_header (void)
{
    MilterHeader *expected_header, *actual_header;

    cut_assert_equal_uint(0, milter_headers_length(headers));

    cut_assert_true(milter_headers_add_header(headers,
                                              "First header",
                                              "First header value"));
    cut_assert_true(milter_headers_add_header(headers,
                                              "Second header",
                                              "Second header value"));
    cut_assert_true(milter_headers_add_header(headers,
                                              "Third header",
                                              "Third header value"));

    expected_header = milter_header_new("First header", "First header value");
    expected_list = g_list_append(expected_list, expected_header);
    actual_header = milter_headers_get_nth_header(headers, 1);
    milter_assert_equal_header(expected_header, actual_header);

    expected_header = milter_header_new("Second header", "Second header value");
    expected_list = g_list_append(expected_list, expected_header);
    actual_header = milter_headers_get_nth_header(headers, 2);
    milter_assert_equal_header(expected_header, actual_header);

    expected_header = milter_header_new("Third header", "Third header value");
    expected_list = g_list_append(expected_list, expected_header);
    actual_header = milter_headers_get_nth_header(headers, 3);
    milter_assert_equal_header(expected_header, actual_header);
}

void
test_find (void)
{
    MilterHeader *found_header, *expected_header;

    cut_assert_true(milter_headers_add_header(headers,
                                              "First header",
                                              "First header value"));
    cut_assert_true(milter_headers_add_header(headers,
                                              "Second header",
                                              "Second header value"));
    cut_assert_true(milter_headers_add_header(headers,
                                              "Third header",
                                              "Third header value"));

    expected_header = milter_header_new("First header", "First header value");
    expected_list = g_list_append(expected_list, expected_header);
    found_header = milter_headers_find(headers, expected_header);
    milter_assert_equal_header(expected_header, found_header);

    expected_header = milter_header_new("Second header", "Second header value");
    expected_list = g_list_append(expected_list, expected_header);
    found_header = milter_headers_find(headers, expected_header);
    milter_assert_equal_header(expected_header, found_header);

    expected_header = milter_header_new("Third header", "Third header value");
    expected_list = g_list_append(expected_list, expected_header);
    found_header = milter_headers_find(headers, expected_header);
    milter_assert_equal_header(expected_header, found_header);

    expected_header = milter_header_new("First header", "First header values");
    expected_list = g_list_append(expected_list, expected_header);
    found_header = milter_headers_find(headers, expected_header);
    cut_assert_null(found_header);

    expected_header = milter_header_new("Second header name", "Second header value");
    expected_list = g_list_append(expected_list, expected_header);
    found_header = milter_headers_find(headers, expected_header);
    cut_assert_null(found_header);
}

void
test_lookup_by_name (void)
{
    MilterHeader *actual_header;

    cut_assert_true(milter_headers_add_header(headers,
                                              "First header",
                                              "First header value"));
    cut_assert_true(milter_headers_add_header(headers,
                                              "Second header",
                                              "Second header value"));
    cut_assert_true(milter_headers_add_header(headers,
                                              "Third header",
                                              "Third header value"));

    actual_header = milter_headers_lookup_by_name(headers, "First header");
    cut_assert_equal_string("First header value", actual_header->value);

    actual_header = milter_headers_lookup_by_name(headers, "Second header");
    cut_assert_equal_string("Second header value", actual_header->value);

    actual_header = milter_headers_lookup_by_name(headers, "Third header");
    cut_assert_equal_string("Third header value", actual_header->value);
}

void
test_index_in_same_header_name (void)
{
    MilterHeader *header;

    cut_assert_true(milter_headers_append_header(headers,
                                                 "Test header",
                                                 "First test header value"));
    cut_assert_true(milter_headers_append_header(headers,
                                                 "Test header",
                                                 "Second test header value"));
    cut_assert_true(milter_headers_append_header(headers,
                                                 "Unique header",
                                                 "Unique header value"));
    cut_assert_true(milter_headers_append_header(headers,
                                                 "Test header",
                                                 "Third test header value"));

    header = milter_header_new("Test header", "First test header value");
    expected_list = g_list_append(expected_list, header);
    cut_assert_equal_int(1, milter_headers_index_in_same_header_name(headers, header));

    header = milter_header_new("Test header", "Second test header value");
    expected_list = g_list_append(expected_list, header);
    cut_assert_equal_int(2, milter_headers_index_in_same_header_name(headers, header));

    header = milter_header_new("Test header", "Third test header value");
    expected_list = g_list_append(expected_list, header);
    cut_assert_equal_int(3, milter_headers_index_in_same_header_name(headers, header));

    header = milter_header_new("Test header", "Inexistent");
    expected_list = g_list_append(expected_list, header);
    cut_assert_equal_int(-1, milter_headers_index_in_same_header_name(headers, header));

    header = milter_header_new("Inexistent header", "Inexistent");
    expected_list = g_list_append(expected_list, header);
    cut_assert_equal_int(-1, milter_headers_index_in_same_header_name(headers, header));
}

void
test_copy (void)
{
    MilterHeaders *copied_headers;

    expected_list = g_list_append(expected_list,
                                  milter_header_new("X-Header1", "Value1"));
    expected_list = g_list_append(expected_list,
                                  milter_header_new("X-Header1", "Value2"));
    expected_list = g_list_append(expected_list,
                                  milter_header_new("X-Header2", "Value1"));

    cut_assert_true(milter_headers_append_header(headers,
                                                 "X-Header1", "Value1"));
    cut_assert_true(milter_headers_append_header(headers,
                                                 "X-Header1", "Value2"));
    cut_assert_true(milter_headers_append_header(headers,
                                                 "X-Header2", "Value1"));
    copied_headers = milter_headers_copy(headers);
    gcut_take_object(G_OBJECT(copied_headers));
    gcut_assert_equal_list(
            milter_headers_get_list(headers),
            milter_headers_get_list(copied_headers),
            milter_header_equal,
            (GCutInspectFunction)milter_header_inspect,
            NULL);
}

void
test_remove (void)
{
    MilterHeader *header;

    cut_assert_true(milter_headers_append_header(headers,
                                                 "First header",
                                                 "First header value"));
    cut_assert_true(milter_headers_append_header(headers,
                                                 "Second header",
                                                 "Second header value"));
    cut_assert_true(milter_headers_append_header(headers,
                                                 "Third header",
                                                 "Third header value"));

    header = milter_header_new("First header", "First header value");
    expected_list = g_list_append(expected_list, header);
    cut_assert_true(milter_headers_remove(headers, header));

    cut_assert_false(milter_headers_remove(headers, header));

    header = milter_header_new("First header", "First header values");
    expected_list = g_list_append(expected_list, header);
    cut_assert_false(milter_headers_remove(headers, header));

    header = milter_header_new("Second header", "Second header value");
    expected_list = g_list_append(expected_list, header);
    cut_assert_true(milter_headers_remove(headers, header));

    header = milter_header_new("Third header", "Third header value");
    expected_list = g_list_append(expected_list, header);
    cut_assert_true(milter_headers_remove(headers, header));

    cut_assert_equal_uint(0, milter_headers_length(headers));

    header = milter_header_new("Third header", "Third header value");
    expected_list = g_list_append(expected_list, header);
    cut_assert_false(milter_headers_remove(headers, header));
}

void
test_add_header (void)
{
    expected_list = g_list_append(expected_list,
                                  milter_header_new("First header",
                                                    "First header value"));
    expected_list = g_list_append(expected_list,
                                  milter_header_new("Second header",
                                                    "Second header value"));
    expected_list = g_list_append(expected_list,
                                  milter_header_new("Third header",
                                                    "Third header value"));

    cut_assert_true(milter_headers_add_header(headers,
                                              "First header",
                                              "First header value"));
    cut_assert_true(milter_headers_add_header(headers,
                                              "Second header",
                                              "Second header value"));
    cut_assert_true(milter_headers_add_header(headers,
                                              "Third header",
                                              "Third header value"));
    gcut_assert_equal_list(
            expected_list,
            milter_headers_get_list(headers),
            milter_header_equal,
            (GCutInspectFunction)milter_header_inspect,
            NULL);
}

void
test_add_header_same_name (void)
{
    expected_list = g_list_append(expected_list,
                                  milter_header_new("X-Same",
                                                    "Same header value2"));
    expected_list = g_list_append(expected_list,
                                  milter_header_new("X-Same",
                                                    "Same header value1"));
    expected_list = g_list_append(expected_list,
                                  milter_header_new("X-Different",
                                                    "Different header value"));

    cut_assert_true(milter_headers_add_header(headers,
                                              "X-Same",
                                              "Same header value1"));
    cut_assert_true(milter_headers_add_header(headers,
                                              "X-Different",
                                              "Different header value"));
    cut_assert_true(milter_headers_add_header(headers,
                                              "X-Same",
                                              "Same header value2"));
    gcut_assert_equal_list(
            expected_list,
            milter_headers_get_list(headers),
            milter_header_equal,
            (GCutInspectFunction)milter_header_inspect,
            NULL);
}

void
test_append_header (void)
{
    expected_list = g_list_append(expected_list,
                                  milter_header_new("X-Different",
                                                    "Different header value"));
    expected_list = g_list_append(expected_list,
                                  milter_header_new("X-Same",
                                                    "Same header value1"));
    expected_list = g_list_append(expected_list,
                                  milter_header_new("X-Same",
                                                    "Same header value2"));

    cut_assert_true(milter_headers_append_header(headers,
                                                 "X-Different",
                                                 "Different header value"));
    cut_assert_true(milter_headers_append_header(headers,
                                                 "X-Same",
                                                 "Same header value1"));
    cut_assert_true(milter_headers_append_header(headers,
                                                 "X-Same",
                                                 "Same header value2"));
    gcut_assert_equal_list(
            expected_list,
            milter_headers_get_list(headers),
            milter_header_equal,
            (GCutInspectFunction)milter_header_inspect,
            NULL);
}

void
test_insert_header (void)
{
    expected_list = g_list_append(expected_list,
                                  milter_header_new("First header",
                                                    "First header value"));
    expected_list = g_list_append(expected_list,
                                  milter_header_new("Second header",
                                                    "Second header value"));
    expected_list = g_list_append(expected_list,
                                  milter_header_new("Third header",
                                                    "Third header value"));
    expected_list = g_list_append(expected_list,
                                  milter_header_new("Forth header",
                                                    "Forth header value"));

    cut_assert_true(milter_headers_append_header(headers,
                                                 "First header",
                                                 "First header value"));
    cut_assert_true(milter_headers_append_header(headers,
                                                 "Third header",
                                                 "Third header value"));
    cut_assert_true(milter_headers_append_header(headers,
                                                 "Forth header",
                                                 "Forth header value"));
    cut_assert_true(milter_headers_insert_header(headers,
                                                 1,
                                                 "Second header",
                                                 "Second header value"));
    gcut_assert_equal_list(
            expected_list,
            milter_headers_get_list(headers),
            milter_header_equal,
            (GCutInspectFunction)milter_header_inspect,
            NULL);
}

void
test_change_header (void)
{
    expected_list = g_list_append(expected_list,
                                  milter_header_new("Test header",
                                                    "Added header value"));
    expected_list = g_list_append(expected_list,
                                  milter_header_new("Test header",
                                                    "Test header value"));
    expected_list = g_list_append(expected_list,
                                  milter_header_new("Test header",
                                                    "Test header value"));
    expected_list = g_list_append(expected_list,
                                  milter_header_new("Unique header",
                                                    "Unique header value"));
    expected_list = g_list_append(expected_list,
                                  milter_header_new("Test header",
                                                    "Replaced header value"));

    cut_assert_true(milter_headers_append_header(headers,
                                                 "Test header",
                                                 "Test header value"));
    cut_assert_true(milter_headers_append_header(headers,
                                                 "Test header",
                                                 "Test header value"));
    cut_assert_true(milter_headers_append_header(headers,
                                                 "Unique header",
                                                 "Unique header value"));
    cut_assert_true(milter_headers_append_header(headers,
                                                 "Test header",
                                                 "Test header value"));
    cut_assert_true(milter_headers_change_header(headers,
                                                 "Test header",
                                                 3,
                                                 "Replaced header value"));
    cut_assert_true(milter_headers_change_header(headers,
                                                 "Test header",
                                                 4,
                                                 "Added header value"));
    gcut_assert_equal_list(
            expected_list,
            milter_headers_get_list(headers),
            milter_header_equal,
            (GCutInspectFunction)milter_header_inspect,
            NULL);
}

void
test_delete_header_with_change_header (void)
{
    expected_list = g_list_append(expected_list,
                                  milter_header_new("Test header",
                                                    "First test header value"));
    expected_list = g_list_append(expected_list,
                                  milter_header_new("Unique header",
                                                    "Unique header value"));
    expected_list = g_list_append(expected_list,
                                  milter_header_new("Test header",
                                                    "Third test header value"));

    cut_assert_true(milter_headers_append_header(headers,
                                                 "Test header",
                                                 "First test header value"));
    cut_assert_true(milter_headers_append_header(headers,
                                                 "Test header",
                                                 "Second test header value"));
    cut_assert_true(milter_headers_append_header(headers,
                                                 "Unique header",
                                                 "Unique header value"));
    cut_assert_true(milter_headers_append_header(headers,
                                                 "Test header",
                                                 "Third test header value"));
    cut_assert_true(milter_headers_change_header(headers,
                                                 "Test header",
                                                 2,
                                                 NULL));
    gcut_assert_equal_list(
            expected_list,
            milter_headers_get_list(headers),
            milter_header_equal,
            (GCutInspectFunction)milter_header_inspect,
            NULL);
}


void
test_delete_header (void)
{
    expected_list = g_list_append(expected_list,
                                  milter_header_new("Test header",
                                                    "First test header value"));
    expected_list = g_list_append(expected_list,
                                  milter_header_new("Unique header",
                                                    "Unique header value"));
    expected_list = g_list_append(expected_list,
                                  milter_header_new("Test header",
                                                    "Third test header value"));

    cut_assert_true(milter_headers_append_header(headers,
                                                 "Test header",
                                                 "First test header value"));
    cut_assert_true(milter_headers_append_header(headers,
                                                 "Test header",
                                                 "Second test header value"));
    cut_assert_true(milter_headers_append_header(headers,
                                                 "Unique header",
                                                 "Unique header value"));
    cut_assert_true(milter_headers_append_header(headers,
                                                 "Test header",
                                                 "Third test header value"));
    cut_assert_true(milter_headers_delete_header(headers,
                                                 "Test header",
                                                 2));
    gcut_assert_equal_list(
            expected_list,
            milter_headers_get_list(headers),
            milter_header_equal,
            (GCutInspectFunction)milter_header_inspect,
            NULL);
}


/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
