/* -*- c-file-style: "ruby" -*- */
/*
 *  Copyright (C) 2008-2009  Kouhei Sutou <kou@clear-code.com>
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

#ifndef __RB_MILTER_H__
#define __RB_MILTER_H__

#include <rbgobject.h>
#include <milter/core.h>
#include <milter/client.h>
#include <milter/server.h>

#include <rb-milter-compat.h>

VALUE rb_mMilter;
VALUE rb_mMilterCore;
VALUE rb_mMilterClient;
VALUE rb_mMilterServer;
VALUE rb_eMilterError;
VALUE rb_cMilterSocketAddressIPv4;
VALUE rb_cMilterSocketAddressIPv6;
VALUE rb_cMilterSocketAddressUnix;
VALUE rb_cMilterSocketAddressUnknown;

#define CSTR2RVAL_SIZE_FREE(string, size)		\
    (rb_milter_cstr2rval_size_free(string, size))

VALUE rb_milter_cstr2rval_size_free(gchar *string, gsize size);

#ifndef G_DEF_CLASS_WITH_GC_FUNC
#  define G_DEF_CLASS_WITH_GC_FUNC(gtype, name, module, mark, free) \
    G_DEF_CLASS2(gtype, name, module, mark, free)
#endif

#endif
