/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Copyright (C) 2008-2011  Kouhei Sutou <kou@clear-code.com>
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

#ifdef HAVE_CONFIG_H
#  include "../../config.h"
#endif /* HAVE_CONFIG_H */

#include "milter-headers.h"
#include "milter-utils.h"

#define MILTER_HEADERS_GET_PRIVATE(obj)                   \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj),                   \
                                 MILTER_TYPE_HEADERS,     \
                                 MilterHeadersPrivate))

typedef struct _MilterHeadersPrivate MilterHeadersPrivate;
struct _MilterHeadersPrivate
{
    GList *header_list;
};

enum
{
    PROP_0
};

G_DEFINE_TYPE(MilterHeaders, milter_headers, G_TYPE_OBJECT);

static void dispose        (GObject         *object);
static void set_property   (GObject         *object,
                            guint            prop_id,
                            const GValue    *value,
                            GParamSpec      *pspec);
static void get_property   (GObject         *object,
                            guint            prop_id,
                            GValue          *value,
                            GParamSpec      *pspec);

static void milter_header_change_value
                           (MilterHeader *header,
                            const gchar *new_value);

static void
milter_headers_class_init (MilterHeadersClass *klass)
{
    GObjectClass *gobject_class;

    gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->dispose      = dispose;
    gobject_class->set_property = set_property;
    gobject_class->get_property = get_property;

    g_type_class_add_private(gobject_class,
                             sizeof(MilterHeadersPrivate));
}

static void
milter_headers_init (MilterHeaders *headers)
{
    MilterHeadersPrivate *priv;

    priv = MILTER_HEADERS_GET_PRIVATE(headers);
    priv->header_list = NULL;
}

static void
dispose (GObject *object)
{
    MilterHeadersPrivate *priv;

    priv = MILTER_HEADERS_GET_PRIVATE(object);

    if (priv->header_list) {
        g_list_foreach(priv->header_list, (GFunc)milter_header_free, NULL);
        g_list_free(priv->header_list);
        priv->header_list = NULL;
    }

    G_OBJECT_CLASS(milter_headers_parent_class)->dispose(object);
}

static void
set_property (GObject      *object,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
    switch (prop_id) {
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
    switch (prop_id) {
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

MilterHeaders *
milter_headers_new (void)
{
    return g_object_new(MILTER_TYPE_HEADERS, NULL);
}

MilterHeaders *
milter_headers_copy (MilterHeaders *headers)
{
    MilterHeaders *copied_headers;
    GList *node;
    MilterHeadersPrivate *priv;

    priv = MILTER_HEADERS_GET_PRIVATE(headers);

    copied_headers = milter_headers_new();
    for (node = priv->header_list; node; node = g_list_next(node)) {
        MilterHeader *header = node->data;

        milter_headers_append_header(copied_headers, header->name, header->value);
    }

    return copied_headers;
}

const GList *
milter_headers_get_list (MilterHeaders *headers)
{
    return MILTER_HEADERS_GET_PRIVATE(headers)->header_list;
}

static gboolean
string_equal (const gchar *string1, const gchar *string2)
{
    if (string1 == string2)
        return TRUE;

    if (string1 == NULL || string2 == NULL)
        return FALSE;

    return milter_utils_strcmp0(string1, string2) == 0;
}

MilterHeader *
milter_headers_find (MilterHeaders *headers,
                     MilterHeader *header)
{
    MilterHeadersPrivate *priv;
    GList *found_node;

    priv = MILTER_HEADERS_GET_PRIVATE(headers);
    found_node = g_list_find_custom(priv->header_list,
                                    header,
                                    (GCompareFunc)milter_header_compare);
    if (!found_node)
        return NULL;
    return (MilterHeader *)(found_node->data);
}

static gint
milter_header_name_compare (MilterHeader *header1,
                            const gchar *name)
{
    return milter_utils_strcmp0(header1->name, name);
}

MilterHeader *
milter_headers_lookup_by_name (MilterHeaders *headers,
                               const gchar *name)
{
    MilterHeadersPrivate *priv;
    GList *found_node;

    priv = MILTER_HEADERS_GET_PRIVATE(headers);

    found_node = g_list_find_custom(priv->header_list,
                                    name,
                                    (GCompareFunc)milter_header_name_compare);
    if (!found_node)
        return NULL;
    return (MilterHeader*)found_node->data;
}

MilterHeader *
milter_headers_get_nth_header (MilterHeaders *headers,
                               guint index)
{
    MilterHeadersPrivate *priv;
    gpointer nth_data;

    if (index < 1)
        return NULL;

    priv = MILTER_HEADERS_GET_PRIVATE(headers);
    nth_data = g_list_nth_data(priv->header_list, index - 1);
    if (!nth_data)
        return NULL;

    return (MilterHeader*)nth_data;
}

gint
milter_headers_index_in_same_header_name (MilterHeaders *headers,
                                          MilterHeader *target)
{
    MilterHeadersPrivate *priv;
    GList *node;
    guint found_count = 0;

    priv = MILTER_HEADERS_GET_PRIVATE(headers);

    for (node = priv->header_list; node; node = g_list_next(node)) {
        MilterHeader *header = node->data;

        if (!string_equal(header->name, target->name))
            continue;

        found_count++;

        if (string_equal(header->value, target->value))
            return found_count;
    }
    return -1;
}

gboolean
milter_headers_remove (MilterHeaders *headers,
                       MilterHeader *header)
{
    MilterHeadersPrivate *priv;
    MilterHeader *found_header;

    priv = MILTER_HEADERS_GET_PRIVATE(headers);

    found_header = milter_headers_find(headers, header);
    if (!found_header)
        return FALSE;

    priv->header_list = g_list_remove(priv->header_list, found_header);
    milter_header_free(found_header);

    return TRUE;
}

gboolean
milter_headers_add_header (MilterHeaders *headers,
                           const gchar *name,
                           const gchar *value)
{
    MilterHeadersPrivate *priv;
    GList *node, *same_name_header = NULL;

    priv = MILTER_HEADERS_GET_PRIVATE(headers);

    for (node = priv->header_list; node; node = g_list_next(node)) {
        MilterHeader *header = node->data;
        if (g_ascii_strcasecmp(header->name, name) == 0) {
            same_name_header = node;
            break;
        }
    }

    if (same_name_header) {
        priv->header_list = g_list_insert_before(priv->header_list,
                                                 same_name_header,
                                                 milter_header_new(name, value));
    } else {
        priv->header_list = g_list_append(priv->header_list,
                                          milter_header_new(name, value));
    }

    return TRUE;
}

gboolean
milter_headers_append_header (MilterHeaders *headers,
                              const gchar *name,
                              const gchar *value)
{
    MilterHeadersPrivate *priv;

    priv = MILTER_HEADERS_GET_PRIVATE(headers);
    priv->header_list = g_list_append(priv->header_list,
                                      milter_header_new(name, value));

    return TRUE;
}

gboolean
milter_headers_insert_header (MilterHeaders *headers,
                              guint position,
                              const gchar *name,
                              const gchar *value)
{
    MilterHeadersPrivate *priv;

    priv = MILTER_HEADERS_GET_PRIVATE(headers);

    priv->header_list = g_list_insert(priv->header_list,
                                      milter_header_new(name, value),
                                      position);

    return TRUE;
}

static MilterHeader *
milter_headers_lookup_by_name_with_index (MilterHeaders *headers,
                                          const gchar *name,
                                          guint index)
{
    MilterHeadersPrivate *priv;
    GList *node;
    guint found_count = 0;

    priv = MILTER_HEADERS_GET_PRIVATE(headers);

    for (node = priv->header_list; node; node = g_list_next(node)) {
        MilterHeader *header = node->data;

        if (!string_equal(header->name, name))
            continue;

        found_count++;

        if (found_count == index)
            return header;
    }

    return NULL;
}

gboolean
milter_headers_change_header (MilterHeaders *headers,
                              const gchar *name,
                              guint index,
                              const gchar *value)
{
    MilterHeader *header;

    if (!value)
        return milter_headers_delete_header(headers, name, index);

    header = milter_headers_lookup_by_name_with_index(headers, name, index);
    if (header)
        milter_header_change_value(header, value);
    else
        milter_headers_add_header(headers, name, value);

    return TRUE;
}

gboolean
milter_headers_delete_header (MilterHeaders *headers,
                              const gchar *name,
                              guint index)
{
    MilterHeader *header;
    MilterHeadersPrivate *priv;

    header = milter_headers_lookup_by_name_with_index(headers, name, index);
    if (!header)
        return FALSE;

    priv = MILTER_HEADERS_GET_PRIVATE(headers);
    priv->header_list = g_list_remove(priv->header_list, header);
    milter_header_free(header);

    return TRUE;
}

guint
milter_headers_length (MilterHeaders *headers)
{
    MilterHeadersPrivate *priv;

    priv = MILTER_HEADERS_GET_PRIVATE(headers);
    return g_list_length(priv->header_list);
}

MilterHeader *
milter_header_new (const gchar *name, const gchar *value)
{
    MilterHeader *header;

    header = g_new0(MilterHeader, 1);
    header->name = g_strdup(name);
    header->value = g_strdup(value);

    return header;
}

void
milter_header_free (MilterHeader *header)
{
    g_free(header->name);
    g_free(header->value);

    g_free(header);
}

void
milter_header_inspect (GString *string,
                       gconstpointer data,
                       gpointer user_data)
{
    const MilterHeader *header = data;

    g_string_append_printf(string, "<<%s>=<%s>>", header->name, header->value);
}

gint
milter_header_compare (MilterHeader *header_a,
                       MilterHeader *header_b)
{
    if (milter_utils_strcmp0(header_a->name, header_b->name))
        return milter_utils_strcmp0(header_a->name, header_b->name);

    return milter_utils_strcmp0(header_a->value, header_b->value);
}

gboolean
milter_header_equal (gconstpointer data1, gconstpointer data2)
{
    const MilterHeader *header1 = data1;
    const MilterHeader *header2 = data2;

    return string_equal(header1->name, header2->name) &&
        string_equal(header1->value, header2->value);
}

static void
milter_header_change_value (MilterHeader *header,
                            const gchar *new_value)
{
    if (header->value)
        g_free(header->value);
    header->value = g_strdup(new_value);
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
