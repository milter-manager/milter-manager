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
#include <milter-manager-test-utils.h>
#include <milter/manager/milter-manager-child-milter.h>
#undef shutdown

void test_start (void);

static MilterManagerChildMilter *milter;
static GError *error;

void
setup (void)
{
    milter = milter_manager_child_milter_new("test-milter");
    error = NULL;
}

void
teardown (void)
{
    if (milter)
        g_object_unref(milter);
}

void
test_start (void)
{
    g_object_set(milter,
                 "command", "/bin/echo",
                 NULL);
    milter_manager_child_milter_start(milter, &error);
    gcut_assert_error(error);
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
