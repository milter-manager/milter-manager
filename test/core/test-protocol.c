/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Copyright (C) 2009-2011  Kouhei Sutou <kou@clear-code.com>
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

#include <milter/core/milter-enum-types.h>
#include <milter/core/milter-protocol.h>

void test_status_is_pass (void);
void data_compare (void);
void test_compare (gconstpointer data);

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

#define ADD_COMPARE_DATUM(label, expected, status1, status2)    \
    gcut_add_datum(label,                                       \
                   "expected", G_TYPE_INT, expected,            \
                   "status1", MILTER_TYPE_STATUS,               \
                   MILTER_STATUS_ ## status1,                   \
                   "status2", MILTER_TYPE_STATUS,               \
                   MILTER_STATUS_ ## status2,                   \
                   NULL)

static void
data_compare_small (void)
{
#define ADD_DATUM(label, status1, status2)              \
    ADD_COMPARE_DATUM(label, -1, status1, status2)

    ADD_DATUM("default - accept", DEFAULT, ACCEPT);
    ADD_DATUM("default - continue", DEFAULT, ACCEPT);
    ADD_DATUM("default - temporary failure", DEFAULT, TEMPORARY_FAILURE);
    ADD_DATUM("default - reject", DEFAULT, REJECT);
    ADD_DATUM("default - discard", DEFAULT, DISCARD);
    ADD_DATUM("default - skip", DEFAULT, SKIP);

    ADD_DATUM("not change - accept", NOT_CHANGE, ACCEPT);
    ADD_DATUM("not change - continue", NOT_CHANGE, ACCEPT);
    ADD_DATUM("not change - temporary failure", NOT_CHANGE, TEMPORARY_FAILURE);
    ADD_DATUM("not change - reject", NOT_CHANGE, REJECT);
    ADD_DATUM("not change - discard", NOT_CHANGE, DISCARD);
    ADD_DATUM("not change - skip", NOT_CHANGE, SKIP);

    ADD_DATUM("accept - continue", ACCEPT, CONTINUE);
    ADD_DATUM("accept - temporary failure", ACCEPT, TEMPORARY_FAILURE);
    ADD_DATUM("accept - reject", ACCEPT, REJECT);
    ADD_DATUM("accept - discard", ACCEPT, DISCARD);
    ADD_DATUM("accept - skip", ACCEPT, SKIP);

    ADD_DATUM("skip - continue", SKIP, CONTINUE);
    ADD_DATUM("skip - temporary failure", SKIP, TEMPORARY_FAILURE);
    ADD_DATUM("skip - reject", SKIP, REJECT);
    ADD_DATUM("skip - discard", SKIP, DISCARD);

    ADD_DATUM("continue - temporary failure", CONTINUE, TEMPORARY_FAILURE);
    ADD_DATUM("continue - reject", CONTINUE, REJECT);
    ADD_DATUM("continue - discard", CONTINUE, DISCARD);

    ADD_DATUM("temporary failure - reject", TEMPORARY_FAILURE, REJECT);
    ADD_DATUM("temporary failure - discard", TEMPORARY_FAILURE, DISCARD);

    ADD_DATUM("reject - discard", REJECT, DISCARD);

#undef ADD_DATUM
}

static void
data_compare_large (void)
{
#define ADD_DATUM(label, status1, status2) \
    ADD_COMPARE_DATUM(label, 1, status1, status2)

    ADD_DATUM("continue - accept", CONTINUE, ACCEPT);
    ADD_DATUM("continue - skip", CONTINUE, SKIP);

    ADD_DATUM("temporary failure - continue", TEMPORARY_FAILURE, CONTINUE);
    ADD_DATUM("temporary failure - accept", TEMPORARY_FAILURE, ACCEPT);
    ADD_DATUM("temporary failure - skip", TEMPORARY_FAILURE, SKIP);

    ADD_DATUM("reject - continue", REJECT, CONTINUE);
    ADD_DATUM("reject - temporary failure", REJECT, TEMPORARY_FAILURE);
    ADD_DATUM("reject - accept", REJECT, ACCEPT);
    ADD_DATUM("reject - skip", REJECT, SKIP);

    ADD_DATUM("discard - continue", DISCARD, ACCEPT);
    ADD_DATUM("discard - temporary failure", DISCARD, TEMPORARY_FAILURE);
    ADD_DATUM("discard - accept", DISCARD, ACCEPT);
    ADD_DATUM("discard - reject", DISCARD, REJECT);
    ADD_DATUM("discard - skip", DISCARD, SKIP);

    ADD_DATUM("skip - default", SKIP, DEFAULT);
    ADD_DATUM("skip - not change", SKIP, NOT_CHANGE);

#undef ADD_DATUM
}

#undef ADD_COMPARE_DATUM

void
data_compare (void)
{
    data_compare_small();
    data_compare_large();
}

void
test_compare (gconstpointer data)
{
    MilterStatus status1, status2;

    status1 = gcut_data_get_enum(data, "status1");
    status2 = gcut_data_get_enum(data, "status2");
    cut_assert_equal_int(gcut_data_get_int(data, "expected"),
                         milter_status_compare(status1, status2));
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
