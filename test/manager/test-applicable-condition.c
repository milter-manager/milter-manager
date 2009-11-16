/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Copyright (C) 2008-2009  Kouhei Sutou <kou@clear-code.com>
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

#include <milter/manager/milter-manager-applicable-condition.h>

#include <milter-manager-test-utils.h>

#include <gcutter.h>

void test_new (void);
void test_name (void);
void test_description (void);
void test_data (void);
void test_merge (void);

static MilterManagerApplicableCondition *condition;
static MilterManagerApplicableCondition *merged_condition;

void
setup (void)
{
    condition = NULL;
    merged_condition = NULL;
}

void
teardown (void)
{
    if (condition)
        g_object_unref(condition);
    if (merged_condition)
        g_object_unref(merged_condition);
}

void
test_new (void)
{
    const gchar name[] = "S25R";

    condition = milter_manager_applicable_condition_new(name);
    cut_assert_equal_string(
        name,
        milter_manager_applicable_condition_get_name(condition));
    cut_assert_equal_string(
        NULL,
        milter_manager_applicable_condition_get_description(condition));
}

void
test_name (void)
{
    const gchar name[] = "S25R";
    const gchar new_name[] = "S25R (new)";

    condition = milter_manager_applicable_condition_new(name);
    cut_assert_equal_string(
        name,
        milter_manager_applicable_condition_get_name(condition));
    milter_manager_applicable_condition_set_name(condition, new_name);
    cut_assert_equal_string(
        new_name,
        milter_manager_applicable_condition_get_name(condition));
}

void
test_description (void)
{
    const gchar description[] = "Selective SMTP Rejection";

    condition = milter_manager_applicable_condition_new("S25R");
    cut_assert_equal_string(
        NULL,
        milter_manager_applicable_condition_get_description(condition));
    milter_manager_applicable_condition_set_description(condition, description);
    cut_assert_equal_string(
        description,
        milter_manager_applicable_condition_get_description(condition));
}

void
test_data (void)
{
    const gchar data[] =
        "- /example\\.com$/"
        "- 192.168.1.100\n";

    condition = milter_manager_applicable_condition_new("Whitelist");
    cut_assert_equal_string(
        NULL,
        milter_manager_applicable_condition_get_data(condition));
    milter_manager_applicable_condition_set_data(condition, data);
    cut_assert_equal_string(
        data,
        milter_manager_applicable_condition_get_data(condition));
}

void
test_merge (void)
{
    condition = milter_manager_applicable_condition_new("S25R");
    merged_condition = milter_manager_applicable_condition_new("Merged");

    cut_assert_equal_string(
        NULL,
        milter_manager_applicable_condition_get_description(merged_condition));
    cut_assert_equal_string(
        NULL,
        milter_manager_applicable_condition_get_data(merged_condition));
    milter_manager_applicable_condition_merge(merged_condition, condition);
    cut_assert_equal_string(
        "Merged",
        milter_manager_applicable_condition_get_name(merged_condition));
    cut_assert_equal_string(
        NULL,
        milter_manager_applicable_condition_get_description(merged_condition));
    cut_assert_equal_string(
        NULL,
        milter_manager_applicable_condition_get_data(merged_condition));

    milter_manager_applicable_condition_set_description(
        condition, "Selective SMTP Rejection");
    milter_manager_applicable_condition_set_data(
        condition, "DATA");
    milter_manager_applicable_condition_merge(merged_condition, condition);
    cut_assert_equal_string(
        "Selective SMTP Rejection",
        milter_manager_applicable_condition_get_description(merged_condition));
    cut_assert_equal_string(
        "DATA",
        milter_manager_applicable_condition_get_data(merged_condition));

    milter_manager_applicable_condition_set_description(merged_condition,
                                                        "Description");
    milter_manager_applicable_condition_set_data(merged_condition,
                                                 "New DATA");
    milter_manager_applicable_condition_set_description(condition, NULL);
    milter_manager_applicable_condition_set_data(condition, NULL);
    milter_manager_applicable_condition_merge(merged_condition, condition);
    cut_assert_equal_string(
        "Description",
        milter_manager_applicable_condition_get_description(merged_condition));
    cut_assert_equal_string(
        "New DATA",
        milter_manager_applicable_condition_get_data(merged_condition));
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
