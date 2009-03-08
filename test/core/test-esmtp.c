/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Copyright (C) 2009  Kouhei Sutou <kou@cozmixng.org>
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

#include <milter/core/milter-esmtp.h>
#include <milter-test-utils.h>

#include <gcutter.h>

void test_parse_mail_from_argument (void);

static gchar *actual_path;
static GHashTable *actual_parameters;

static GError *expected_error;
static GError *actual_error;

void
setup (void)
{
    actual_path = NULL;
    actual_parameters = NULL;

    expected_error = NULL;
    actual_error = NULL;
}

void
teardown (void)
{
    if (actual_path)
        g_free(actual_path);
    if (actual_parameters)
        g_hash_table_unref(actual_parameters);

    if (expected_error)
        g_error_free(expected_error);
    if (actual_error)
        g_error_free(actual_error);
}

void
test_parse_mail_from_argument (void)
{
    cut_assert_true(milter_esmtp_parse_mail_from_argument("<>",
                                                          &actual_path,
                                                          &actual_parameters,
                                                          &actual_error));
    gcut_assert_equal_error(NULL, actual_error);
    cut_assert_equal_string(NULL, actual_path);
    gcut_assert_equal_hash_table_string_string(NULL, actual_parameters);
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
