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

#ifdef HAVE_CONFIG_H
#  include "../../config.h"
#endif /* HAVE_CONFIG_H */

#include "milter-manager-headers.h"

#define MILTER_MANAGER_HEADERS_GET_PRIVATE(obj)                   \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj),                           \
                                 MILTER_TYPE_MANAGER_HEADERS,     \
                                 MilterManagerHeadersPrivate))

typedef struct _MilterManagerHeadersPrivate MilterManagerHeadersPrivate;
struct _MilterManagerHeadersPrivate
{
    GList *header_list;
};

enum
{
    PROP_0
};

G_DEFINE_TYPE(MilterManagerHeaders, milter_manager_headers, G_TYPE_OBJECT);

static void dispose        (GObject         *object);
static void set_property   (GObject         *object,
                            guint            prop_id,
                            const GValue    *value,
                            GParamSpec      *pspec);
static void get_property   (GObject         *object,
                            guint            prop_id,
                            GValue          *value,
                            GParamSpec      *pspec);

static gint milter_manager_header_compare
                           (MilterManagerHeader *header_a,
                            MilterManagerHeader *header_b);
static void milter_manager_header_change_value
                           (MilterManagerHeader *header,
                            const gchar *new_value);

static void
milter_manager_headers_class_init (MilterManagerHeadersClass *klass)
{
    GObjectClass *gobject_class;

    gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->dispose      = dispose;
    gobject_class->set_property = set_property;
    gobject_class->get_property = get_property;

    g_type_class_add_private(gobject_class,
                             sizeof(MilterManagerHeadersPrivate));
}

static void
milter_manager_headers_init (MilterManagerHeaders *headers)
{
    MilterManagerHeadersPrivate *priv;

    priv = MILTER_MANAGER_HEADERS_GET_PRIVATE(headers);
    priv->header_list = NULL;
}

static void
dispose (GObject *object)
{
    MilterManagerHeadersPrivate *priv;

    priv = MILTER_MANAGER_HEADERS_GET_PRIVATE(object);

    if (priv->header_list) {
        g_list_foreach(priv->header_list, (GFunc)milter_manager_header_free, NULL);
        g_list_free(priv->header_list);
        priv->header_list = NULL;
    }

    G_OBJECT_CLASS(milter_manager_headers_parent_class)->dispose(object);
}

static void
set_property (GObject      *object,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
    MilterManagerHeadersPrivate *priv;

    priv = MILTER_MANAGER_HEADERS_GET_PRIVATE(object);
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
    MilterManagerHeadersPrivate *priv;

    priv = MILTER_MANAGER_HEADERS_GET_PRIVATE(object);
    switch (prop_id) {
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

MilterManagerHeaders *
milter_manager_headers_new (void)
{
    return g_object_new(MILTER_TYPE_MANAGER_HEADERS, NULL);
}

MilterManagerHeaders *
milter_manager_headers_copy (MilterManagerHeaders *headers)
{
    MilterManagerHeaders *copied_headers;
    GList *node;
    MilterManagerHeadersPrivate *priv;

    priv = MILTER_MANAGER_HEADERS_GET_PRIVATE(headers);

    copied_headers = milter_manager_headers_new();
    for (node = priv->header_list; node; node = g_list_next(node)) {
        MilterManagerHeader *header = node->data;

        milter_manager_headers_add_header(copied_headers,
                                          header->name,
                                          header->value);
    }

    return copied_headers;
}

const GList *
milter_manager_headers_get_list (MilterManagerHeaders *headers)
{
    return MILTER_MANAGER_HEADERS_GET_PRIVATE(headers)->header_list;
}

static gboolean
string_equal (const gchar *string1, const gchar *string2)
{
    if (string1 == string2)
        return TRUE;

    if (string1 == NULL || string2 == NULL)
        return FALSE;

    return g_strcmp0(string1, string2) == 0;
}

MilterManagerHeader *
milter_manager_headers_find (MilterManagerHeaders *headers,
                             MilterManagerHeader *header)
{
    MilterManagerHeadersPrivate *priv;
    GList *found_node;

    priv = MILTER_MANAGER_HEADERS_GET_PRIVATE(headers);
    found_node = g_list_find_custom(priv->header_list,
                                    header,
                                    (GCompareFunc)milter_manager_header_compare);
    if (!found_node)
        return NULL;
    return (MilterManagerHeader*)found_node->data;
}

static gint
milter_manager_header_name_compare (MilterManagerHeader *header1,
                                    const gchar *name)
{
    return g_strcmp0(header1->name, name);
}

MilterManagerHeader *
milter_manager_headers_lookup_by_name (MilterManagerHeaders *headers,
                                       const gchar *name)
{
    MilterManagerHeadersPrivate *priv;
    GList *found_node;

    priv = MILTER_MANAGER_HEADERS_GET_PRIVATE(headers);

    found_node = g_list_find_custom(priv->header_list,
                                    name,
                                    (GCompareFunc)milter_manager_header_name_compare);
    if (!found_node)
        return NULL;
    return (MilterManagerHeader*)found_node->data;
}

MilterManagerHeader *
milter_manager_headers_get_nth_header (MilterManagerHeaders *headers,
                                       guint index)
{
    MilterManagerHeadersPrivate *priv;
    gpointer nth_data;

    if (index < 1)
        return NULL;

    priv = MILTER_MANAGER_HEADERS_GET_PRIVATE(headers);
    nth_data = g_list_nth_data(priv->header_list, index - 1);
    if (!nth_data)
        return NULL;

    return (MilterManagerHeader*)nth_data;
}

gboolean
milter_manager_headers_add_header(MilterManagerHeaders *headers,
                                  const gchar *name,
                                  const gchar *value)
{
    MilterManagerHeadersPrivate *priv;

    priv = MILTER_MANAGER_HEADERS_GET_PRIVATE(headers);

    priv->header_list = g_list_append(priv->header_list,
                                      milter_manager_header_new(name, value));

    return TRUE;
}

gboolean
milter_manager_headers_insert_header (MilterManagerHeaders *headers,
                                      guint position,
                                      const gchar *name,
                                      const gchar *value)
{
    MilterManagerHeadersPrivate *priv;

    priv = MILTER_MANAGER_HEADERS_GET_PRIVATE(headers);

    priv->header_list = g_list_insert(priv->header_list,
                                      milter_manager_header_new(name, value),
                                      position);

    return TRUE;
}

gboolean
milter_manager_headers_change_header (MilterManagerHeaders *headers,
                                      const gchar *name,
                                      guint index,
                                      const gchar *value)
{
    MilterManagerHeadersPrivate *priv;
    GList *node;
    guint found_count = 0;

    priv = MILTER_MANAGER_HEADERS_GET_PRIVATE(headers);

    for (node = priv->header_list; node; node = g_list_next(node)) {
        MilterManagerHeader *header = node->data;

        if (string_equal(header->name, name))
            found_count++;

        if (found_count == index) {
            milter_manager_header_change_value(header, value);
            return TRUE;
        }
    }

    return FALSE;
}

guint
milter_manager_headers_length (MilterManagerHeaders *headers)
{
    MilterManagerHeadersPrivate *priv;

    priv = MILTER_MANAGER_HEADERS_GET_PRIVATE(headers);
    return g_list_length(priv->header_list);
}

MilterManagerHeader *
milter_manager_header_new (const gchar *name, const gchar *value)
{
    MilterManagerHeader *header;

    header = g_new0(MilterManagerHeader, 1);
    header->name = g_strdup(name);
    header->value = g_strdup(value);

    return header;
}

void
milter_manager_header_free (MilterManagerHeader *header)
{
    g_free(header->name);
    g_free(header->value);

    g_free(header);
}

void
milter_manager_header_inspect (GString *string,
                               gconstpointer data,
                               gpointer user_data)
{
    const MilterManagerHeader *header = data;

    g_string_append_printf(string, "<<%s>=<%s>>", header->name, header->value);
}

static gint
milter_manager_header_compare (MilterManagerHeader *header_a,
                               MilterManagerHeader *header_b)
{
    if (g_strcmp0(header_a->name, header_b->name))
        return g_strcmp0(header_a->name, header_b->name);

    return g_strcmp0(header_a->value, header_b->value);
}

gboolean
milter_manager_header_equal (gconstpointer data1, gconstpointer data2)
{
    const MilterManagerHeader *header1 = data1;
    const MilterManagerHeader *header2 = data2;

    return string_equal(header1->name, header2->name) &&
        string_equal(header1->value, header2->value);
}

static void
milter_manager_header_change_value (MilterManagerHeader *header,
                                    const gchar *new_value)
{
    if (header->value)
        g_free(header->value);
    header->value = g_strdup(new_value);
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
