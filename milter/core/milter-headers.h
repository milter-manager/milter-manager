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
 *  along with this library.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef __MILTER_HEADERS_H__
#define __MILTER_HEADERS_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define MILTER_TYPE_HEADERS            (milter_headers_get_type())
#define MILTER_HEADERS(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), MILTER_TYPE_HEADERS, MilterHeaders))
#define MILTER_HEADERS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), MILTER_TYPE_HEADERS, MilterHeadersClass))
#define MILTER_IS_HEADERS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), MILTER_TYPE_HEADERS))
#define MILTER_IS_HEADERS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), MILTER_TYPE_HEADERS))
#define MILTER_HEADERS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), MILTER_TYPE_HEADERS, MilterHeadersClass))

typedef struct _MilterHeader
{
    gchar *name;
    gchar *value;
} MilterHeader;

MilterHeader *milter_header_new   (const gchar *name,
                                   const gchar *value);
void          milter_header_free  (MilterHeader *header);
gboolean      milter_header_equal (gconstpointer header1,
                                   gconstpointer header2);
gint          milter_header_compare
                                  (MilterHeader *header_a,
                                   MilterHeader *header_b);
void          milter_header_inspect
                                  (GString *string,
                                   gconstpointer header,
                                   gpointer user_data);

typedef struct _MilterHeaders         MilterHeaders;
typedef struct _MilterHeadersClass    MilterHeadersClass;

struct _MilterHeaders
{
    GObject object;
};

struct _MilterHeadersClass
{
    GObjectClass parent_class;
};

GType          milter_headers_get_type    (void) G_GNUC_CONST;

MilterHeaders *milter_headers_new         (void);
MilterHeaders *milter_headers_copy        (MilterHeaders *headers);
const GList   *milter_headers_get_list    (MilterHeaders *headers);

gboolean       milter_headers_add_header  (MilterHeaders *headers,
                                           const gchar *name,
                                           const gchar *value);
gboolean       milter_headers_append_header
                                          (MilterHeaders *headers,
                                           const gchar *name,
                                           const gchar *value);
gboolean       milter_headers_insert_header
                                          (MilterHeaders *headers,
                                           guint position,
                                           const gchar *name,
                                           const gchar *value);
gboolean       milter_headers_change_header
                                          (MilterHeaders *headers,
                                           const gchar *name,
                                           guint index,
                                           const gchar *value);
gboolean       milter_headers_delete_header
                                          (MilterHeaders *headers,
                                           const gchar *name,
                                           guint index);
guint          milter_headers_length      (MilterHeaders *headers);
MilterHeader  *milter_headers_get_nth_header
                                          (MilterHeaders *headers,
                                           guint index);
MilterHeader  *milter_headers_find        (MilterHeaders *headers,
                                           MilterHeader *header);
MilterHeader  *milter_headers_lookup_by_name
                                          (MilterHeaders *headers,
                                           const gchar *name);
gboolean       milter_headers_remove      (MilterHeaders *headers,
                                           MilterHeader *header);
gint           milter_headers_index_in_same_header_name
                                          (MilterHeaders *headers,
                                           MilterHeader *header);

G_END_DECLS

#endif /* __MILTER_HEADERS_H__ */

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
