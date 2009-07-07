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

#include <milter/core/milter-option.h>
#include <milter/core/milter-enum-types.h>

#include <gcutter.h>

void test_new (void);
void test_new_empty (void);
void test_copy (void);
void test_equal (void);
void test_version (void);
void test_action (void);
void test_step (void);
void test_get_step_convenience (void);
void test_combine (void);
void test_combine_older_version (void);
void test_combine_newer_version (void);
void test_merge (void);
void test_merge_older_version (void);
void test_merge_newer_version (void);
void test_inspect (void);

static MilterOption *option;
static MilterOption *copied_option;
static MilterOption *other_option;

void
setup (void)
{
    option = NULL;
    copied_option = NULL;
    other_option = NULL;
}

void
teardown (void)
{
    if (option)
        g_object_unref(option);
    if (copied_option)
        g_object_unref(copied_option);
    if (other_option)
        g_object_unref(other_option);
}

void
test_new (void)
{
    guint32 version;
    MilterActionFlags action;
    MilterStepFlags step;

    version = 6;
    action = MILTER_ACTION_ADD_HEADERS | MILTER_ACTION_CHANGE_BODY;
    step = MILTER_STEP_NO_ENVELOPE_FROM | MILTER_STEP_NO_REPLY_CONNECT;

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
test_copy (void)
{
    guint32 version;
    MilterActionFlags action;
    MilterStepFlags step;

    version = 6;
    action = MILTER_ACTION_ADD_HEADERS | MILTER_ACTION_CHANGE_BODY;
    step = MILTER_STEP_NO_ENVELOPE_FROM | MILTER_STEP_NO_REPLY_CONNECT;

    option = milter_option_new(version, action, step);
    copied_option = milter_option_copy(option);
    cut_assert_equal_uint(version, milter_option_get_version(copied_option));
    cut_assert_equal_uint(action, milter_option_get_action(copied_option));
    cut_assert_equal_uint(step, milter_option_get_step(copied_option));
}

void
test_equal (void)
{
    guint32 version;
    MilterActionFlags action;
    MilterStepFlags step;

    version = 6;
    action = MILTER_ACTION_ADD_HEADERS | MILTER_ACTION_CHANGE_BODY;
    step = MILTER_STEP_NO_ENVELOPE_FROM | MILTER_STEP_NO_REPLY_CONNECT;

    option = milter_option_new(version, action, step);
    other_option = milter_option_new(version, action, step);
    cut_assert_true(milter_option_equal(option, option));
    cut_assert_true(milter_option_equal(other_option, other_option));
    cut_assert_true(milter_option_equal(option, other_option));

    milter_option_set_version(other_option, version - 1);
    cut_assert_false(milter_option_equal(option, other_option));
    milter_option_set_version(other_option, version);

    milter_option_set_action(other_option,
                             action | MILTER_ACTION_ADD_ENVELOPE_RECIPIENT);
    cut_assert_false(milter_option_equal(option, other_option));
    milter_option_set_action(other_option, action);

    milter_option_set_step(other_option, step | MILTER_STEP_SKIP);
    cut_assert_false(milter_option_equal(option, other_option));
    milter_option_set_step(other_option, step);
}

void
test_version (void)
{
    option = milter_option_new_empty();

    cut_assert_equal_uint(6, milter_option_get_version(option));
    milter_option_set_version(option, 4);
    cut_assert_equal_uint(4, milter_option_get_version(option));
}

void
test_action (void)
{
    option = milter_option_new_empty();

    gcut_assert_equal_flags(MILTER_TYPE_ACTION_FLAGS,
                            MILTER_ACTION_NONE,
                            milter_option_get_action(option));

    milter_option_set_action(option, MILTER_ACTION_ADD_ENVELOPE_RECIPIENT);
    gcut_assert_equal_flags(MILTER_TYPE_ACTION_FLAGS,
                            MILTER_ACTION_ADD_ENVELOPE_RECIPIENT,
                            milter_option_get_action(option));

    milter_option_add_action(option,
                             MILTER_ACTION_DELETE_ENVELOPE_RECIPIENT |
                             MILTER_ACTION_CHANGE_ENVELOPE_FROM);
    gcut_assert_equal_flags(MILTER_TYPE_ACTION_FLAGS,
                            MILTER_ACTION_ADD_ENVELOPE_RECIPIENT |
                            MILTER_ACTION_DELETE_ENVELOPE_RECIPIENT |
                            MILTER_ACTION_CHANGE_ENVELOPE_FROM,
                            milter_option_get_action(option));

    milter_option_remove_action(option, MILTER_ACTION_CHANGE_ENVELOPE_FROM);
    gcut_assert_equal_flags(MILTER_TYPE_ACTION_FLAGS,
                            MILTER_ACTION_ADD_ENVELOPE_RECIPIENT |
                            MILTER_ACTION_DELETE_ENVELOPE_RECIPIENT,
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

void
test_get_step_convenience (void)
{
    option = milter_option_new_empty();

    milter_option_set_step(option,
                           MILTER_STEP_SKIP |
                           MILTER_STEP_HEADER_VALUE_WITH_LEADING_SPACE |
                           MILTER_STEP_NO_HELO |
                           MILTER_STEP_NO_DATA |
                           MILTER_STEP_NO_REPLY_HELO |
                           MILTER_STEP_NO_REPLY_DATA);

    gcut_assert_equal_flags(MILTER_TYPE_STEP_FLAGS,
                            MILTER_STEP_NO_HELO |
                            MILTER_STEP_NO_DATA,
                            milter_option_get_step_no_event(option));
    gcut_assert_equal_flags(MILTER_TYPE_STEP_FLAGS,
                            MILTER_STEP_NO_REPLY_HELO |
                            MILTER_STEP_NO_REPLY_DATA,
                            milter_option_get_step_no_reply(option));
    gcut_assert_equal_flags(MILTER_TYPE_STEP_FLAGS,
                            MILTER_STEP_NO_HELO |
                            MILTER_STEP_NO_DATA |
                            MILTER_STEP_NO_REPLY_HELO |
                            MILTER_STEP_NO_REPLY_DATA,
                            milter_option_get_step_no(option));
    gcut_assert_equal_flags(MILTER_TYPE_STEP_FLAGS,
                            MILTER_STEP_SKIP |
                            MILTER_STEP_HEADER_VALUE_WITH_LEADING_SPACE,
                            milter_option_get_step_yes(option));
}

void
test_combine (void)
{
    guint32 version;
    MilterActionFlags action;
    MilterStepFlags step;

    version = 6;
    action = MILTER_ACTION_ADD_HEADERS | MILTER_ACTION_CHANGE_BODY;
    step = MILTER_STEP_NO_ENVELOPE_FROM | MILTER_STEP_NO_REPLY_CONNECT;

    option = milter_option_new(version, action, step);
    copied_option = milter_option_new(version,
                                      MILTER_ACTION_ADD_HEADERS |
                                      MILTER_ACTION_CHANGE_HEADERS,
                                      MILTER_STEP_NO_ENVELOPE_FROM |
                                      MILTER_STEP_NO_DATA |
                                      MILTER_STEP_NO_REPLY_HELO);

    milter_option_combine(option, copied_option);

    cut_assert_equal_uint(version, milter_option_get_version(option));
    gcut_assert_equal_flags(MILTER_TYPE_ACTION_FLAGS,
                            MILTER_ACTION_ADD_HEADERS,
                            milter_option_get_action(option));
    gcut_assert_equal_flags(MILTER_TYPE_STEP_FLAGS,
                            MILTER_STEP_NO_ENVELOPE_FROM,
                            milter_option_get_step(option));
}

void
test_combine_older_version (void)
{
    guint32 version, older_version;
    MilterActionFlags action;
    MilterStepFlags step;

    version = 6;
    older_version = 4;
    action = MILTER_ACTION_ADD_HEADERS | MILTER_ACTION_CHANGE_BODY;
    step = MILTER_STEP_NO_ENVELOPE_FROM | MILTER_STEP_NO_REPLY_CONNECT;

    option = milter_option_new(version, action, step);
    copied_option = milter_option_new(older_version,
                                      MILTER_ACTION_ADD_HEADERS |
                                      MILTER_ACTION_CHANGE_HEADERS,
                                      MILTER_STEP_NO_ENVELOPE_FROM |
                                      MILTER_STEP_NO_DATA |
                                      MILTER_STEP_NO_REPLY_HELO);

    cut_assert_true(milter_option_combine(option, copied_option));

    cut_assert_equal_uint(older_version, milter_option_get_version(option));
    gcut_assert_equal_flags(MILTER_TYPE_ACTION_FLAGS,
                            MILTER_ACTION_ADD_HEADERS,
                            milter_option_get_action(option));
    gcut_assert_equal_flags(MILTER_TYPE_STEP_FLAGS,
                            MILTER_STEP_NO_ENVELOPE_FROM,
                            milter_option_get_step(option));
}

void
test_combine_newer_version (void)
{
    guint32 version, newer_version;
    MilterActionFlags action;
    MilterStepFlags step;

    version = 4;
    newer_version = 6;
    action = MILTER_ACTION_ADD_HEADERS | MILTER_ACTION_CHANGE_BODY;
    step = MILTER_STEP_NO_ENVELOPE_FROM | MILTER_STEP_NO_REPLY_CONNECT;

    option = milter_option_new(version, action, step);
    copied_option = milter_option_new(newer_version,
                                      MILTER_ACTION_ADD_HEADERS |
                                      MILTER_ACTION_CHANGE_HEADERS,
                                      MILTER_STEP_NO_ENVELOPE_FROM |
                                      MILTER_STEP_NO_DATA |
                                      MILTER_STEP_NO_REPLY_HELO);

    cut_assert_false(milter_option_combine(option, copied_option));

    cut_assert_equal_uint(version, milter_option_get_version(option));
    gcut_assert_equal_flags(MILTER_TYPE_ACTION_FLAGS,
                            action,
                            milter_option_get_action(option));
    gcut_assert_equal_flags(MILTER_TYPE_STEP_FLAGS,
                            step,
                            milter_option_get_step(option));
}

void
test_merge (void)
{
    guint32 version;
    MilterActionFlags action;
    MilterStepFlags step;

    version = 6;
    action = MILTER_ACTION_ADD_HEADERS | MILTER_ACTION_CHANGE_BODY;
    step = MILTER_STEP_NO_ENVELOPE_FROM | MILTER_STEP_NO_REPLY_CONNECT;

    option = milter_option_new(version, action, step);
    copied_option = milter_option_new(version,
                                      MILTER_ACTION_ADD_HEADERS |
                                      MILTER_ACTION_CHANGE_HEADERS,
                                      MILTER_STEP_NO_ENVELOPE_FROM |
                                      MILTER_STEP_NO_DATA |
                                      MILTER_STEP_NO_REPLY_HELO);

    cut_assert_true(milter_option_merge(option, copied_option));

    cut_assert_equal_uint(version, milter_option_get_version(option));
    gcut_assert_equal_flags(MILTER_TYPE_ACTION_FLAGS,
                            MILTER_ACTION_ADD_HEADERS |
                            MILTER_ACTION_CHANGE_BODY |
                            MILTER_ACTION_CHANGE_HEADERS,
                            milter_option_get_action(option));
    gcut_assert_equal_flags(MILTER_TYPE_STEP_FLAGS,
                            MILTER_STEP_NO_ENVELOPE_FROM,
                            milter_option_get_step(option));
}

void
test_merge_older_version (void)
{
    guint32 version;
    MilterActionFlags action;
    MilterStepFlags step;

    version = 6;
    action = MILTER_ACTION_ADD_HEADERS | MILTER_ACTION_CHANGE_BODY;
    step = MILTER_STEP_NO_ENVELOPE_FROM | MILTER_STEP_NO_REPLY_CONNECT;

    option = milter_option_new(version, action, step);
    copied_option = milter_option_new(2,
                                      MILTER_ACTION_ADD_HEADERS |
                                      MILTER_ACTION_CHANGE_HEADERS,
                                      MILTER_STEP_NO_ENVELOPE_FROM |
                                      MILTER_STEP_NO_REPLY_HELO);

    cut_assert_true(milter_option_merge(option, copied_option));

    cut_assert_equal_uint(version, milter_option_get_version(option));
    gcut_assert_equal_flags(MILTER_TYPE_ACTION_FLAGS,
                            MILTER_ACTION_ADD_HEADERS |
                            MILTER_ACTION_CHANGE_BODY |
                            MILTER_ACTION_CHANGE_HEADERS,
                            milter_option_get_action(option));
    gcut_assert_equal_flags(MILTER_TYPE_STEP_FLAGS,
                            MILTER_STEP_NO_ENVELOPE_FROM,
                            milter_option_get_step(option));
}

void
test_merge_newer_version (void)
{
    guint32 version;
    MilterActionFlags action;
    MilterStepFlags step;

    version = 2;
    action = MILTER_ACTION_ADD_HEADERS | MILTER_ACTION_CHANGE_BODY;
    step = MILTER_STEP_NO_ENVELOPE_FROM | MILTER_STEP_NO_REPLY_CONNECT;

    option = milter_option_new(version, action, step);
    copied_option = milter_option_new(6,
                                      MILTER_ACTION_ADD_HEADERS |
                                      MILTER_ACTION_CHANGE_HEADERS,
                                      MILTER_STEP_NO_ENVELOPE_FROM |
                                      MILTER_STEP_NO_REPLY_HELO);

    cut_assert_false(milter_option_merge(option, copied_option));
}

void
test_inspect (void)
{
    guint32 version;
    MilterActionFlags action;
    MilterStepFlags step;

    version = 6;
    action = MILTER_ACTION_ADD_HEADERS | MILTER_ACTION_CHANGE_BODY;
    step = MILTER_STEP_NO_ENVELOPE_FROM | MILTER_STEP_NO_REPLY_CONNECT;

    option = milter_option_new(version, action, step);
    cut_assert_equal_string_with_free(
        "#<MilterOption version=<6> action=<add-headers|change-body> "
        "step=<no-envelope-from|no-reply-connect>>",
        milter_option_inspect(option));
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
