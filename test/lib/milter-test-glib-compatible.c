/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Copyright (C) 2011  Kouhei Sutou <kou@clear-code.com>
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

#include "milter-test-glib-compatible.h"

#if !GLIB_CHECK_VERSION(2, 14, 0)
static void
collect_key (gpointer key, gpointer value, gpointer user_data)
{
    GList **keys = (GList **)user_data;

    *keys = g_list_append(*keys, key);
}

GList *
g_hash_table_get_keys (GHashTable *hash_table)
{
    GList *keys = NULL;

    g_hash_table_foreach(hash_table, collect_key, &keys);

    return keys;
}
#endif

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
