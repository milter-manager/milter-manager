/* -*- c-file-style: "ruby" -*- */
/*
 *  Copyright (C) 2008-2022  Sutou Kouhei <kou@clear-code.com>
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

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <errno.h>

#include "rb-milter-core-private.h"

static VALUE
rval2macro (VALUE rb_data,
            VALUE user_data,
            int argc,
            const VALUE *argv,
            VALUE block)
{
    VALUE rb_key, rb_value;
    gchar *key, *value;
    GHashTable *macros = (GHashTable *)user_data;

    rb_key = RARRAY_PTR(rb_data)[0];
    rb_value = RARRAY_PTR(rb_data)[1];

    key = g_strdup(RVAL2CSTR(rb_key));
    value = g_strdup(RVAL2CSTR(rb_value));

    g_hash_table_insert(macros, key, value);

    return Qnil;
}

GHashTable *
rb_milter__rval2macros (VALUE rb_macros)
{
    GHashTable *macros = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
    ID id_each;
    CONST_ID(id_each, "each");
    rb_block_call(rb_macros, id_each, 0, NULL, rval2macro, (VALUE)macros);
    return macros;
}

static void
macro2rval (gpointer key, gpointer value, gpointer user_data)
{
    VALUE rb_macros = (VALUE)user_data;

    rb_hash_aset(rb_macros, CSTR2RVAL(key), CSTR2RVAL(value));
}

VALUE
rb_milter__macros2rval (GHashTable *macros)
{
    VALUE rb_macros;

    if (!macros)
	return Qnil;

    rb_macros = rb_hash_new();
    g_hash_table_foreach(macros, macro2rval, (gpointer)rb_macros);
    return rb_macros;
}

static VALUE
rb_milter__address2rval_raw (VALUE rb_address)
{
    ID id_resolve;
    CONST_ID(id_resolve, "resolve");
    return rb_funcall(rb_const_get(rb_mMilter, rb_intern("SocketAddress")),
                      id_resolve,
                      1,
                      rb_address);
}

VALUE
rb_milter__address2rval (struct sockaddr *address, socklen_t address_length)
{
    VALUE rb_address = rb_str_new((gchar *)address, address_length);
    return rb_milter__address2rval_raw(rb_address);
}

VALUE
rb_milter__address2rval_free (struct sockaddr *address,
                              socklen_t address_length)
{
    VALUE rb_address = rb_str_new((gchar *)address, address_length);
    g_free(address);
    return rb_milter__address2rval_raw(rb_address);
}

VALUE
rb_milter__connect_signal_convert (guint num, const GValue *values)
{
    struct sockaddr *address;
    socklen_t address_length;
    VALUE rb_address = Qnil;

    address = g_value_get_pointer(&values[2]);
    address_length = g_value_get_uint(&values[3]);
    rb_address = ADDRESS2RVAL(address, address_length);

    return rb_ary_new3(3,
		       GVAL2RVAL(&values[0]),
		       GVAL2RVAL(&values[1]),
		       rb_address);
}

VALUE
rb_milter__helo_signal_convert(guint num, const GValue *values)
{
    return rb_ary_new3(2,
                       GVAL2RVAL(&values[0]),
                       rb_str_new2(g_value_get_string(&values[1])));
}

VALUE
rb_milter__envelope_from_signal_convert(guint num, const GValue *values)
{
    return rb_ary_new3(2,
                       GVAL2RVAL(&values[0]),
                       rb_str_new2(g_value_get_string(&values[1])));
}

VALUE
rb_milter__envelope_recipient_signal_convert(guint num, const GValue *values)
{
    return rb_ary_new3(2,
                       GVAL2RVAL(&values[0]),
                       rb_str_new2(g_value_get_string(&values[1])));
}

VALUE
rb_milter__unknown_signal_convert(guint num, const GValue *values)
{
    return rb_ary_new3(2,
                       GVAL2RVAL(&values[0]),
                       rb_str_new2(g_value_get_string(&values[1])));
}

VALUE
rb_milter__header_signal_convert(guint num, const GValue *values)
{
    return rb_ary_new3(3,
                       GVAL2RVAL(&values[0]),
                       rb_str_new2(g_value_get_string(&values[1])),
                       rb_str_new2(g_value_get_string(&values[2])));
}

VALUE
rb_milter__body_signal_convert (guint num, const GValue *values)
{
    return rb_ary_new3(2,
		       GVAL2RVAL(&values[0]),
		       rb_str_new(g_value_get_string(&values[1]),
#if GLIB_SIZEOF_SIZE_T == 8
				  g_value_get_uint64(&values[2])
#else
				  g_value_get_uint(&values[2])
#endif
			   ));
}

VALUE
rb_milter__end_of_message_signal_convert (guint num, const GValue *values)
{
    VALUE rb_chunk = Qnil;
    const gchar *chunk;
    gsize chunk_size;

    chunk = g_value_get_string(&values[1]);
#if GLIB_SIZEOF_SIZE_T == 8
    chunk_size = g_value_get_uint64(&values[2]);
#else
    chunk_size = g_value_get_uint(&values[2]);
#endif

    if (chunk && chunk_size > 0)
        rb_chunk = rb_str_new(chunk, chunk_size);

    return rb_ary_new3(2, GVAL2RVAL(&values[0]), rb_chunk);
}

const gchar *
rb_milter__inspect (VALUE object)
{
    VALUE inspected;

    inspected = rb_funcall(object, rb_intern("inspect"), 0);
    return StringValueCStr(inspected);
}

void
rb_milter__scan_options (VALUE options, ...)
{
    VALUE available_keys;
    const gchar *key;
    VALUE *value;
    va_list args;

    if (NIL_P(options))
        options = rb_hash_new();
    else
        options = rb_funcall(options, rb_intern("dup"), 0);

    Check_Type(options, T_HASH);

    available_keys = rb_ary_new();
    va_start(args, options);
    key = va_arg(args, const gchar *);
    while (key) {
        VALUE rb_key;
        value = va_arg(args, VALUE *);

        rb_key = ID2SYM(rb_intern(key));
        rb_ary_push(available_keys, rb_key);
        *value = rb_funcall(options, rb_intern("delete"), 1, rb_key);

        key = va_arg(args, const gchar *);
    }
    va_end(args);

    if (RVAL2CBOOL(rb_funcall(options, rb_intern("empty?"), 0)))
        return;

    rb_raise(rb_eArgError,
             "unexpected key(s) exist: %s: available keys: %s",
             rb_milter__inspect(rb_funcall(options, rb_intern("keys"), 0)),
             rb_milter__inspect(available_keys));
}
