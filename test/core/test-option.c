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
#undef shutdown

void test_new (void);

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
    option = milter_option_new_empty();
    cut_assert_equal_int(6, milter_option_get_version(option));
    cut_assert_equal_int(MILTER_ACTION_NONE, milter_option_get_action(option));
    cut_assert_equal_int(0, milter_option_get_step(option));
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
