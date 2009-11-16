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

#ifndef __MILTER_TEST_COMPATIBLE_H__
#define __MILTER_TEST_COMPATIBLE_H__

#include <gcutter.h>

#ifndef cut_message
#  define cut_message(...) __VA_ARGS__
#endif

#ifndef cut_test_with_user_message
#  define cut_test_with_user_message(assertion, set_user_message) (assertion)
#endif

#endif /* __MILTER_TEST_COMPATIBLE_H__ */

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
