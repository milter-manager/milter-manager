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

#ifndef __MILTER_MANAGER_HEADERS_H__
#define __MILTER_MANAGER_HEADERS_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define MILTER_TYPE_MANAGER_HEADERS            (milter_manager_headers_get_type())
#define MILTER_MANAGER_HEADERS(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), MILTER_TYPE_MANAGER_HEADERS, MilterManagerHeaders))
#define MILTER_MANAGER_HEADERS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), MILTER_TYPE_MANAGER_HEADERS, MilterManagerHeadersClass))
#define MILTER_MANAGER_IS_HEADERS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), MILTER_TYPE_MANAGER_HEADERS))
#define MILTER_MANAGER_IS_HEADERS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), MILTER_TYPE_MANAGER_HEADERS))
#define MILTER_MANAGER_HEADERS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), MILTER_TYPE_MANAGER_HEADERS, MilterManagerHeadersClass))

typedef struct _MilterManagerHeader
{
    gchar *name;
    gchar *value;
} MilterManagerHeader;

MilterManagerHeader   *milter_manager_header_new
                                        (const gchar *name,
                                         const gchar *value);
void                   milter_manager_header_free
                                        (MilterManagerHeader *header);
gboolean               milter_manager_header_equal
                                        (gconstpointer header1,
                                         gconstpointer header2);
void                   milter_manager_header_inspect
                                        (GString *string,
                                         gconstpointer header,
                                         gpointer user_data);

typedef struct _MilterManagerHeaders         MilterManagerHeaders;
typedef struct _MilterManagerHeadersClass    MilterManagerHeadersClass;

struct _MilterManagerHeaders
{
    GObject object;
};

struct _MilterManagerHeadersClass
{
    GObjectClass parent_class;
};

GType                 milter_manager_headers_get_type    (void) G_GNUC_CONST;

MilterManagerHeaders *milter_manager_headers_new         (void);
MilterManagerHeaders *milter_manager_headers_copy        (MilterManagerHeaders *headers);
const GList          *milter_manager_headers_get_list    (MilterManagerHeaders *headers);

gboolean              milter_manager_headers_add_header  (MilterManagerHeaders *headers,
                                                          const gchar *name,
                                                          const gchar *value);
gboolean              milter_manager_headers_insert_header
                                                         (MilterManagerHeaders *headers,
                                                          guint position,
                                                          const gchar *name,
                                                          const gchar *value);
gboolean              milter_manager_headers_change_header
                                                         (MilterManagerHeaders *headers,
                                                          const gchar *name,
                                                          guint index,
                                                          const gchar *value);
guint                 milter_manager_headers_length      (MilterManagerHeaders *headers);
MilterManagerHeader  *milter_manager_headers_get_nth_header
                                                         (MilterManagerHeaders *headers,
                                                          guint index);
MilterManagerHeader  *milter_manager_headers_find        (MilterManagerHeaders *headers,
                                                          MilterManagerHeader *header);
MilterManagerHeader  *milter_manager_headers_lookup_by_name
                                                         (MilterManagerHeaders *headers,
                                                          const gchar *name);
                                                                

G_END_DECLS

#endif /* __MILTER_MANAGER_HEADERS_H__ */

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
