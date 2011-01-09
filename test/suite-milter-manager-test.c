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
 *  along with this library.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <milter/manager.h>

void milter_manager_test_warmup (void);
void milter_manager_test_cooldown (void);

void
milter_manager_test_warmup (void)
{
    static int argc;
    static char *argv[] = {"milter-manager-test"};
    char **argvp;

    argc = sizeof(argv) / sizeof(*argv);
    argvp = argv;
    milter_manager_init(&argc, &argvp);
    if (!g_getenv("MILTER_LOG_LEVEL"))
        milter_set_log_level(MILTER_LOG_LEVEL_NONE);
}

void
milter_manager_test_cooldown (void)
{
    milter_manager_quit();
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
