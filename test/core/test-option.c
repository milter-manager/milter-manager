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
#include <milter-core/milter-option.h>
#include <milter-core/milter-enum-types.h>
#undef shutdown

void test_new (void);
void test_new_empty (void);
void test_version (void);
void test_action (void);
void test_step (void);

static MilterOption *option;

void
setup (void)
{
    option = NULL;
}

void
teardown (void)
{
    if (option)
        g_object_unref(option);
}

void
test_new (void)
{
    guint32 version;
    MilterActionFlags action;
    MilterStepFlags step;

    version = 8;
    action = MILTER_ACTION_ADD_HEADERS | MILTER_ACTION_CHANGE_BODY;
    step = MILTER_STEP_NO_MAIL | MILTER_STEP_NO_REPLY_CONNECT;

    option = milter_option_new(version, action, step);
    cut_assert_equal_uint(version, milter_option_get_version(option));
    cut_assert_equal_uint(action, milter_option_get_action(option));
    cut_assert_equal_uint(step, milter_option_get_step(option));
}

void
test_new_empty (void)
{
    option = milter_option_new_empty();
    cut_assert_equal_uint(6, milter_option_get_version(option));
    cut_assert_equal_uint(MILTER_ACTION_NONE, milter_option_get_action(option));
    cut_assert_equal_uint(0, milter_option_get_step(option));
}

void
test_version (void)
{
    option = milter_option_new_empty();

    cut_assert_equal_uint(6, milter_option_get_version(option));
    milter_option_set_version(option, 8);
    cut_assert_equal_uint(8, milter_option_get_version(option));
}

void
test_action (void)
{
    option = milter_option_new_empty();

    gcut_assert_equal_flags(MILTER_TYPE_ACTION_FLAGS,
                            MILTER_ACTION_NONE,
                            milter_option_get_action(option));

    milter_option_set_action(option,
                             MILTER_ACTION_ADD_RCPT);
    gcut_assert_equal_flags(MILTER_TYPE_ACTION_FLAGS,
                            MILTER_ACTION_ADD_RCPT,
                            milter_option_get_action(option));

    milter_option_add_action(option,
                             MILTER_ACTION_DELETE_RCPT |
                             MILTER_ACTION_CHANGE_FROM);
    gcut_assert_equal_flags(MILTER_TYPE_ACTION_FLAGS,
                            MILTER_ACTION_ADD_RCPT |
                            MILTER_ACTION_DELETE_RCPT |
                            MILTER_ACTION_CHANGE_FROM,
                            milter_option_get_action(option));

    milter_option_remove_action(option, MILTER_ACTION_CHANGE_FROM);
    gcut_assert_equal_flags(MILTER_TYPE_ACTION_FLAGS,
                            MILTER_ACTION_ADD_RCPT |
                            MILTER_ACTION_DELETE_RCPT,
                            milter_option_get_action(option));

    milter_option_set_action(option,
                             MILTER_ACTION_QUARANTINE |
                             MILTER_ACTION_SET_SYMBOL_LIST);
    gcut_assert_equal_flags(MILTER_TYPE_ACTION_FLAGS,
                            MILTER_ACTION_QUARANTINE |
                            MILTER_ACTION_SET_SYMBOL_LIST,
                            milter_option_get_action(option));
}

void
test_step (void)
{
    option = milter_option_new_empty();

    gcut_assert_equal_flags(MILTER_TYPE_STEP_FLAGS,
                            MILTER_STEP_NONE,
                            milter_option_get_step(option));

    milter_option_set_step(option,
                           MILTER_STEP_SKIP |
                           MILTER_STEP_NO_REPLY_HELO);
    gcut_assert_equal_flags(MILTER_TYPE_STEP_FLAGS,
                            MILTER_STEP_SKIP |
                            MILTER_STEP_NO_REPLY_HELO,
                            milter_option_get_step(option));

    milter_option_add_step(option,
                           MILTER_STEP_NO_CONNECT |
                           MILTER_STEP_NO_HELO);
    gcut_assert_equal_flags(MILTER_TYPE_STEP_FLAGS,
                            MILTER_STEP_SKIP |
                            MILTER_STEP_NO_REPLY_HELO |
                            MILTER_STEP_NO_CONNECT |
                            MILTER_STEP_NO_HELO,
                            milter_option_get_step(option));

    milter_option_remove_step(option, MILTER_STEP_NO_CONNECT);
    gcut_assert_equal_flags(MILTER_TYPE_STEP_FLAGS,
                            MILTER_STEP_SKIP |
                            MILTER_STEP_NO_REPLY_HELO |
                            MILTER_STEP_NO_HELO,
                            milter_option_get_step(option));

    milter_option_set_step(option,
                           MILTER_STEP_NO_REPLY_HELO |
                           MILTER_STEP_NO_HELO);
    gcut_assert_equal_flags(MILTER_TYPE_STEP_FLAGS,
                            MILTER_STEP_NO_REPLY_HELO |
                            MILTER_STEP_NO_HELO,
                            milter_option_get_step(option));
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
