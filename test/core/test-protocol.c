/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Copyright (C) 2009  Kouhei Sutou <kou@clear-code.com>
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

#include <milter/core/milter-protocol.h>

#include <gcutter.h>

void test_status_is_pass (void);

void
setup (void)
{
}

void
teardown (void)
{
}

void
test_status_is_pass (void)
{
    cut_assert_true(MILTER_STATUS_IS_PASS(MILTER_STATUS_DEFAULT));
    cut_assert_true(MILTER_STATUS_IS_PASS(MILTER_STATUS_NOT_CHANGE));
    cut_assert_true(MILTER_STATUS_IS_PASS(MILTER_STATUS_CONTINUE));
    cut_assert_false(MILTER_STATUS_IS_PASS(MILTER_STATUS_REJECT));
    cut_assert_false(MILTER_STATUS_IS_PASS(MILTER_STATUS_DISCARD));
    cut_assert_false(MILTER_STATUS_IS_PASS(MILTER_STATUS_ACCEPT));
    cut_assert_false(MILTER_STATUS_IS_PASS(MILTER_STATUS_TEMPORARY_FAILURE));
    cut_assert_false(MILTER_STATUS_IS_PASS(MILTER_STATUS_NO_REPLY));
    cut_assert_false(MILTER_STATUS_IS_PASS(MILTER_STATUS_SKIP));
    cut_assert_false(MILTER_STATUS_IS_PASS(MILTER_STATUS_ALL_OPTIONS));
    cut_assert_false(MILTER_STATUS_IS_PASS(MILTER_STATUS_PROGRESS));
    cut_assert_false(MILTER_STATUS_IS_PASS(MILTER_STATUS_ABORT));
    cut_assert_false(MILTER_STATUS_IS_PASS(MILTER_STATUS_QUARANTINE));
    cut_assert_false(MILTER_STATUS_IS_PASS(MILTER_STATUS_STOP));
    cut_assert_false(MILTER_STATUS_IS_PASS(MILTER_STATUS_ERROR));
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
