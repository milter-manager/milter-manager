/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Copyright (C) 2008  Kouhei Sutou <kou@cozmixng.org>
 *
 *  This library is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your macros_request) any later version.
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

#ifdef HAVE_CONFIG_H
#  include "../../config.h"
#endif /* HAVE_CONFIG_H */

#include <string.h>

#include "milter-macros-requests.h"
#include "milter-enum-types.h"

#define MILTER_MACROS_REQUESTS_GET_PRIVATE(obj)                  \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj),                 \
                                 MILTER_TYPE_MACROS_REQUESTS,    \
                                 MilterMacrosRequestsPrivate))

typedef struct _MilterMacrosRequestsPrivate	MilterMacrosRequestsPrivate;
struct _MilterMacrosRequestsPrivate
{
    GHashTable *symbols_table;
};

enum
{
    PROP_0,
    PROP_SYMBOLS_TABLE
};

G_DEFINE_TYPE(MilterMacrosRequests, milter_macros_requests, G_TYPE_OBJECT);

static void dispose        (GObject         *object);
static void set_property   (GObject         *object,
                            guint            prop_id,
                            const GValue    *value,
                            GParamSpec      *pspec);
static void get_property   (GObject         *object,
                            guint            prop_id,
                            GValue          *value,
                            GParamSpec      *pspec);

static void
milter_macros_requests_class_init (MilterMacrosRequestsClass *klass)
{
    GObjectClass *gobject_class;
    GParamSpec *spec;

    gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->dispose      = dispose;
    gobject_class->set_property = set_property;
    gobject_class->get_property = get_property;

    spec = g_param_spec_pointer("symbols-table",
                                "requested symbols table",
                                "The GHashTable stored "
                                "macro requests symbols in",
                                G_PARAM_READWRITE);
    g_object_class_install_property(gobject_class, PROP_SYMBOLS_TABLE, spec);

    g_type_class_add_private(gobject_class, sizeof(MilterMacrosRequestsPrivate));
}

static void
symbols_free (gpointer data)
{
    GList *symbols = data;

    g_list_foreach(symbols, (GFunc)g_free, NULL);
    g_list_free(symbols);
}

static void
milter_macros_requests_init (MilterMacrosRequests *requests)
{
    MilterMacrosRequestsPrivate *priv;

    priv = MILTER_MACROS_REQUESTS_GET_PRIVATE(requests);

    priv->symbols_table = g_hash_table_new_full(g_direct_hash, g_direct_equal,
                                                NULL, symbols_free);
}

static void
dispose (GObject *object)
{
    MilterMacrosRequestsPrivate *priv;

    priv = MILTER_MACROS_REQUESTS_GET_PRIVATE(object);

    if (priv->symbols_table) {
        g_hash_table_unref(priv->symbols_table);
        priv->symbols_table = NULL;
    }

    G_OBJECT_CLASS(milter_macros_requests_parent_class)->dispose(object);
}

static void
set_property (GObject      *object,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
    MilterMacrosRequestsPrivate *priv;

    priv = MILTER_MACROS_REQUESTS_GET_PRIVATE(object);
    switch (prop_id) {
      case PROP_SYMBOLS_TABLE:
        if (priv->symbols_table)
            g_hash_table_unref(priv->symbols_table);
        priv->symbols_table = g_value_get_pointer(value);
        if (priv->symbols_table)
            g_hash_table_ref(priv->symbols_table);
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static void
get_property (GObject    *object,
              guint       prop_id,
              GValue     *value,
              GParamSpec *pspec)
{
    MilterMacrosRequestsPrivate *priv;

    priv = MILTER_MACROS_REQUESTS_GET_PRIVATE(object);
    switch (prop_id) {
      case PROP_SYMBOLS_TABLE:
        g_value_set_pointer(value, priv->symbols_table);
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

MilterMacrosRequests *
milter_macros_requests_new (void)
{
    return g_object_new(MILTER_TYPE_MACROS_REQUESTS, NULL);
}

void
milter_macros_requests_set_symbols_va_list (MilterMacrosRequests *requests,
                                            MilterCommand command,
                                            const gchar *first_symbol,
                                            va_list var_args)
{
    const gchar *symbol;
    GArray *symbols;

    symbols = g_array_new(TRUE, FALSE, sizeof(gchar *));
    symbol = first_symbol;
    while (symbol) {
        g_array_append_val(symbols, symbol);
        symbol = va_arg(var_args, gchar *);
    }

    milter_macros_requests_set_symbols_string_array(
        requests, command, (const gchar **)(symbols->data));
    g_array_free(symbols, TRUE);
}

void
milter_macros_requests_set_symbols (MilterMacrosRequests *requests,
                                    MilterCommand command,
                                    const gchar *first_symbol, ...)
{
    va_list var_args;

    va_start(var_args, first_symbol);
    milter_macros_requests_set_symbols_va_list(requests, command,
                                               first_symbol, var_args);
    va_end(var_args);
}

void
milter_macros_requests_set_symbols_string_array (MilterMacrosRequests *requests,
                                                 MilterCommand command,
                                                 const gchar **strings)
{
    MilterMacrosRequestsPrivate *priv;
    GList *symbols = NULL;
    gint i;

    for (i = 0; strings[i]; i++) {
        symbols = g_list_append(symbols, g_strdup(strings[i]));
    }

    priv = MILTER_MACROS_REQUESTS_GET_PRIVATE(requests);
    g_hash_table_insert(priv->symbols_table, GINT_TO_POINTER(command), symbols);
}

GList *
milter_macros_requests_get_symbols (MilterMacrosRequests *requests,
                                    MilterCommand command)
{
    GHashTable *symbols_table;

    symbols_table = MILTER_MACROS_REQUESTS_GET_PRIVATE(requests)->symbols_table;
    return g_hash_table_lookup(symbols_table, GINT_TO_POINTER(command));
}

static void
merge_requests (gpointer key, gpointer value, gpointer user_data)
{
    GHashTable *dest = user_data;
    GList *dest_symbols, *copied_symbols, *node;

    dest_symbols = g_hash_table_lookup(dest, key);
    copied_symbols = NULL;
    for (node = dest_symbols; node; node = g_list_next(node)) {
        gchar *symbol = node->data;
        copied_symbols = g_list_append(copied_symbols, g_strdup(symbol));
    }
    for (node = value; node; node = g_list_next(node)) {
        gchar *symbol = node->data;
        if (!g_list_find_custom(copied_symbols, symbol,
                                (GCompareFunc)g_utf8_collate))
            copied_symbols = g_list_append(copied_symbols, g_strdup(symbol));
    }
    g_hash_table_insert(dest, key, copied_symbols);
}

void
milter_macros_requests_merge (MilterMacrosRequests *dest,
                              MilterMacrosRequests *src)
{
    GHashTable *dest_symbols_table;

    dest_symbols_table = MILTER_MACROS_REQUESTS_GET_PRIVATE(dest)->symbols_table;
    milter_macros_requests_foreach(src, merge_requests, dest_symbols_table);
}

void
milter_macros_requests_foreach (MilterMacrosRequests *requests,
                                GHFunc func, gpointer user_data)
{
    GHashTable *symbols_table;

    symbols_table = MILTER_MACROS_REQUESTS_GET_PRIVATE(requests)->symbols_table;
    g_hash_table_foreach(symbols_table, func, user_data);
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
