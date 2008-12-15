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

#include <ruby.h>
#include <milter/manager.h>

#ifndef RUBY_INIT_STACK
#  define RUBY_INIT_STACK                       \
    VALUE variable_in_this_stack_frame;         \
    extern void Init_stack (VALUE *address);    \
    Init_stack(&variable_in_this_stack_frame)
#endif

int
main (int argc, char **argv)
{
    {
        RUBY_INIT_STACK;
        milter_manager_init(&argc, &argv);
        milter_manager_main();
        milter_manager_quit();
    }

    return 0;
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
