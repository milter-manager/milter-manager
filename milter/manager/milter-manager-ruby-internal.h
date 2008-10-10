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

#ifndef __MILTER_MANAGER_RUBY_INTERNAL_H__
#define __MILTER_MANAGER_RUBY_INTERNAL_H__

#include <rb-milter-manager.h>

#undef PACKAGE_NAME
#undef PACKAGE_TARNAME
#undef PACKAGE_VERSION
#undef PACKAGE_STRING
#undef PACKAGE_BUGREPORT

#ifndef HAVE_RB_ERRINFO
#  define rb_errinfo() (ruby_errinfo)
#endif

#endif /* __MILTER_MANAGER_RUBY_INTERNAL_H__ */

/*
vi:nowrap:ai:expandtab:sw=4
*/
