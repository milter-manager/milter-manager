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
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <arpa/inet.h>
#undef shutdown

#include <libmilter/libmilter-compatible.h>

void test_private (void);

static SmfiContext *context;

void
setup (void)
{
    context = smfi_context_new();
}

void
teardown (void)
{
    if (context)
        g_object_unref(context);
}

void
test_private (void)
{
    gchar data[] = "private data";

    cut_assert_null(smfi_getpriv(context));
    smfi_setpriv(context, data);
    cut_assert_equal_string(data, smfi_getpriv(context));
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
