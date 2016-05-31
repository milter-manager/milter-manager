/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Copyright (C) 2008-2016  Kouhei Sutou <kou@cozmixng.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <stdlib.h>

#include <milter/manager.h>

int
main (int argc, char **argv)
{
    gboolean success;

    {
        gpointer stack_address;
        milter_manager_set_stack_address(&stack_address);
        milter_manager_init(&argc, &argv);
        success = milter_manager_main();
        milter_manager_quit();
    }

    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
