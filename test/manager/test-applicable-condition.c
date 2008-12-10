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

#include <milter/manager/milter-manager-applicable-condition.h>

#include <milter-manager-test-utils.h>

#include <gcutter.h>

void test_new (void);
void test_name (void);
void test_description (void);

static MilterManagerApplicableCondition *condition;

void
setup (void)
{
    condition = NULL;
}

void
teardown (void)
{
    if (condition)
        g_object_unref(condition);
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


/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
