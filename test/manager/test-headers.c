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

#include <string.h>

#include <gcutter.h>

#define shutdown inet_shutdown
#include <milter-manager-test-utils.h>
#include <milter/manager/milter-manager-headers.h>
#undef shutdown

void test_length (void);
void test_get_nth_header (void);
void test_find (void);
void test_lookup_by_name (void);
void test_copy (void);
void test_add_header (void);
void test_insert_header (void);
void test_change_header (void);

static MilterManagerHeaders *headers;
static GList *expected_list;

void
setup (void)
{
    headers = milter_manager_headers_new();
    expected_list = NULL;
}

void
teardown (void)
{
    if (headers)
        g_object_unref(headers);
    if (expected_list) {
        g_list_foreach(expected_list, (GFunc)milter_manager_header_free, NULL);
        g_list_free(expected_list);
    }
}

void
test_length (void)
{
    cut_assert_equal_uint(0, milter_manager_headers_length(headers));

    cut_assert_true(milter_manager_headers_add_header(headers,
                                                      "First header",
                                                      "First header value"));
    cut_assert_true(milter_manager_headers_add_header(headers,
                                                      "Second header",
                                                      "Second header value"));
    cut_assert_true(milter_manager_headers_add_header(headers,
                                                      "Third header",
                                                      "Third header value"));
    cut_assert_equal_uint(3, milter_manager_headers_length(headers));
}

static void
milter_manager_assert_equal_header (MilterManagerHeader *expected,
                                    MilterManagerHeader *actual)
{
    cut_assert_equal_string(expected->name, actual->name);
    cut_assert_equal_string(expected->value, actual->value);
}

void
test_get_nth_header (void)
{
    MilterManagerHeader *expected_header, *actual_header;

    cut_assert_equal_uint(0, milter_manager_headers_length(headers));

    cut_assert_true(milter_manager_headers_add_header(headers,
                                                      "First header",
                                                      "First header value"));
    cut_assert_true(milter_manager_headers_add_header(headers,
                                                      "Second header",
                                                      "Second header value"));
    cut_assert_true(milter_manager_headers_add_header(headers,
                                                      "Third header",
                                                      "Third header value"));

    expected_header = milter_manager_header_new("First header", "First header value");
    expected_list = g_list_append(expected_list, expected_header);
    actual_header = milter_manager_headers_get_nth_header(headers, 1);
    milter_manager_assert_equal_header(expected_header, actual_header);

    expected_header = milter_manager_header_new("Second header", "Second header value");
    expected_list = g_list_append(expected_list, expected_header);
    actual_header = milter_manager_headers_get_nth_header(headers, 2);
    milter_manager_assert_equal_header(expected_header, actual_header);

    expected_header = milter_manager_header_new("Third header", "Third header value");
    expected_list = g_list_append(expected_list, expected_header);
    actual_header = milter_manager_headers_get_nth_header(headers, 3);
    milter_manager_assert_equal_header(expected_header, actual_header);
}

void
test_find (void)
{
    MilterManagerHeader *found_header, *expected_header;

    cut_assert_true(milter_manager_headers_add_header(headers,
                                                      "First header",
                                                      "First header value"));
    cut_assert_true(milter_manager_headers_add_header(headers,
                                                      "Second header",
                                                      "Second header value"));
    cut_assert_true(milter_manager_headers_add_header(headers,
                                                      "Third header",
                                                      "Third header value"));

    expected_header = milter_manager_header_new("First header", "First header value");
    expected_list = g_list_append(expected_list, expected_header);
    found_header = milter_manager_headers_find(headers, expected_header);
    milter_manager_assert_equal_header(expected_header, found_header);

    expected_header = milter_manager_header_new("Second header", "Second header value");
    expected_list = g_list_append(expected_list, expected_header);
    found_header = milter_manager_headers_find(headers, expected_header);
    milter_manager_assert_equal_header(expected_header, found_header);

    expected_header = milter_manager_header_new("Third header", "Third header value");
    expected_list = g_list_append(expected_list, expected_header);
    found_header = milter_manager_headers_find(headers, expected_header);
    milter_manager_assert_equal_header(expected_header, found_header);

    expected_header = milter_manager_header_new("First header", "First header values");
    expected_list = g_list_append(expected_list, expected_header);
    found_header = milter_manager_headers_find(headers, expected_header);
    cut_assert_null(found_header);

    expected_header = milter_manager_header_new("Second header name", "Second header value");
    expected_list = g_list_append(expected_list, expected_header);
    found_header = milter_manager_headers_find(headers, expected_header);
    cut_assert_null(found_header);
}

void
test_lookup_by_name (void)
{
    MilterManagerHeader *actual_header;

    cut_assert_true(milter_manager_headers_add_header(headers,
                                                      "First header",
                                                      "First header value"));
    cut_assert_true(milter_manager_headers_add_header(headers,
                                                      "Second header",
                                                      "Second header value"));
    cut_assert_true(milter_manager_headers_add_header(headers,
                                                      "Third header",
                                                      "Third header value"));

    actual_header = milter_manager_headers_lookup_by_name(headers, "First header");
    cut_assert_equal_string("First header value", actual_header->value);

    actual_header = milter_manager_headers_lookup_by_name(headers, "Second header");
    cut_assert_equal_string("Second header value", actual_header->value);

    actual_header = milter_manager_headers_lookup_by_name(headers, "Third header");
    cut_assert_equal_string("Third header value", actual_header->value);
}

void
test_copy (void)
{
    MilterManagerHeaders *copied_headers;

    expected_list = g_list_append(expected_list, 
                                  milter_manager_header_new("First header",
                                                            "First header value"));
    expected_list = g_list_append(expected_list, 
                                  milter_manager_header_new("Second header",
                                                            "Second header value"));
    expected_list = g_list_append(expected_list, 
                                  milter_manager_header_new("Third header",
                                                            "Third header value"));

    cut_assert_true(milter_manager_headers_add_header(headers,
                                                      "First header",
                                                      "First header value"));
    cut_assert_true(milter_manager_headers_add_header(headers,
                                                      "Second header",
                                                      "Second header value"));
    cut_assert_true(milter_manager_headers_add_header(headers,
                                                      "Third header",
                                                      "Third header value"));
    copied_headers = milter_manager_headers_copy(headers);
    gcut_take_object(G_OBJECT(copied_headers));
    gcut_assert_equal_list(
            milter_manager_headers_get_list(headers),
            milter_manager_headers_get_list(copied_headers),
            milter_manager_header_equal,
            (GCutInspectFunc)milter_manager_header_inspect,
            NULL);
}

void
test_add_header (void)
{
    expected_list = g_list_append(expected_list, 
                                  milter_manager_header_new("First header",
                                                            "First header value"));
    expected_list = g_list_append(expected_list, 
                                  milter_manager_header_new("Second header",
                                                            "Second header value"));
    expected_list = g_list_append(expected_list, 
                                  milter_manager_header_new("Third header",
                                                            "Third header value"));

    cut_assert_true(milter_manager_headers_add_header(headers,
                                                      "First header",
                                                      "First header value"));
    cut_assert_true(milter_manager_headers_add_header(headers,
                                                      "Second header",
                                                      "Second header value"));
    cut_assert_true(milter_manager_headers_add_header(headers,
                                                      "Third header",
                                                      "Third header value"));
    gcut_assert_equal_list(
            expected_list,
            milter_manager_headers_get_list(headers),
            milter_manager_header_equal,
            (GCutInspectFunc)milter_manager_header_inspect,
            NULL);
}

void
test_insert_header (void)
{
    expected_list = g_list_append(expected_list, 
                                  milter_manager_header_new("First header",
                                                            "First header value"));
    expected_list = g_list_append(expected_list, 
                                  milter_manager_header_new("Second header",
                                                            "Second header value"));
    expected_list = g_list_append(expected_list, 
                                  milter_manager_header_new("Third header",
                                                            "Third header value"));
    expected_list = g_list_append(expected_list, 
                                  milter_manager_header_new("Forth header",
                                                            "Forth header value"));

    cut_assert_true(milter_manager_headers_add_header(headers,
                                                      "First header",
                                                      "First header value"));
    cut_assert_true(milter_manager_headers_add_header(headers,
                                                      "Third header",
                                                      "Third header value"));
    cut_assert_true(milter_manager_headers_add_header(headers,
                                                      "Forth header",
                                                      "Forth header value"));
    cut_assert_true(milter_manager_headers_insert_header(headers,
                                                         1,
                                                         "Second header",
                                                         "Second header value"));
    gcut_assert_equal_list(
            expected_list,
            milter_manager_headers_get_list(headers),
            milter_manager_header_equal,
            (GCutInspectFunc)milter_manager_header_inspect,
            NULL);
}

void
test_change_header (void)
{
    expected_list = g_list_append(expected_list, 
                                  milter_manager_header_new("Test header",
                                                            "Test header value"));
    expected_list = g_list_append(expected_list, 
                                  milter_manager_header_new("Test header",
                                                            "Test header value"));
    expected_list = g_list_append(expected_list, 
                                  milter_manager_header_new("Unique header",
                                                            "Unique header value"));
    expected_list = g_list_append(expected_list, 
                                  milter_manager_header_new("Test header",
                                                            "Replaced header value"));

    cut_assert_true(milter_manager_headers_add_header(headers,
                                                      "Test header",
                                                      "Test header value"));
    cut_assert_true(milter_manager_headers_add_header(headers,
                                                      "Test header",
                                                      "Test header value"));
    cut_assert_true(milter_manager_headers_add_header(headers,
                                                      "Unique header",
                                                      "Unique header value"));
    cut_assert_true(milter_manager_headers_add_header(headers,
                                                      "Test header",
                                                      "Test header value"));
    cut_assert_true(milter_manager_headers_change_header(headers,
                                                         "Test header",
                                                         3,
                                                         "Replaced header value"));
    gcut_assert_equal_list(
            expected_list,
            milter_manager_headers_get_list(headers),
            milter_manager_header_equal,
            (GCutInspectFunc)milter_manager_header_inspect,
            NULL);
}


/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
