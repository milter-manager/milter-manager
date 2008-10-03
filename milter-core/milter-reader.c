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
#  include "../config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include "milter-reader.h"

#define MILTER_READER_GET_PRIVATE(obj)                  \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj),                 \
                                 MILTER_TYPE_READER,    \
                                 MilterReaderPrivate))

typedef gboolean (*ReadFunc) (MilterReader  *reader,
                              gchar        **read_data,
                              gsize         *read_size,
                              GError       **error,
                              gpointer       user_data);

typedef struct _MilterReaderPrivate	MilterReaderPrivate;
struct _MilterReaderPrivate
{
    ReadFunc read_func;
    GDestroyNotify destroy_func;
    gpointer data;
};

enum
{
    PROP_0,
    PROP_WRITE_FUNC,
    PROP_DESTROY_FUNC,
    PROP_DATA
};

G_DEFINE_TYPE(MilterReader, milter_reader, G_TYPE_OBJECT);

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
milter_reader_class_init (MilterReaderClass *klass)
{
    GObjectClass *gobject_class;
    GParamSpec *spec;

    gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->dispose      = dispose;
    gobject_class->set_property = set_property;
    gobject_class->get_property = get_property;

    spec = g_param_spec_pointer("read-func",
                                "Read function",
                                "The read function of the reader",
                                G_PARAM_READWRITE);
    g_object_class_install_property(gobject_class, PROP_WRITE_FUNC, spec);

    spec = g_param_spec_pointer("destroy-func",
                                "Destroy function",
                                "The destroy function of the reader",
                                G_PARAM_READWRITE);
    g_object_class_install_property(gobject_class, PROP_DESTROY_FUNC, spec);

    spec = g_param_spec_pointer("data",
                                "Data",
                                "The data of the reader",
                                G_PARAM_READWRITE);
    g_object_class_install_property(gobject_class, PROP_DATA, spec);

    g_type_class_add_private(gobject_class, sizeof(MilterReaderPrivate));
}

static void
milter_reader_init (MilterReader *reader)
{
    MilterReaderPrivate *priv;

    priv = MILTER_READER_GET_PRIVATE(reader);
    priv->read_func = NULL;
    priv->destroy_func = NULL;
    priv->data = NULL;
}

static void
dispose (GObject *object)
{
    MilterReaderPrivate *priv;

    priv = MILTER_READER_GET_PRIVATE(object);
    if (priv->data && priv->destroy_func) {
        priv->destroy_func(priv->data);
        priv->data = NULL;
    }

    G_OBJECT_CLASS(milter_reader_parent_class)->dispose(object);
}

static void
set_property (GObject      *object,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
    MilterReaderPrivate *priv;

    priv = MILTER_READER_GET_PRIVATE(object);
    switch (prop_id) {
      case PROP_WRITE_FUNC:
        priv->read_func = g_value_get_pointer(value);
        break;
      case PROP_DESTROY_FUNC:
        priv->destroy_func = g_value_get_pointer(value);
        break;
      case PROP_DATA:
        if (priv->data && priv->destroy_func)
            priv->destroy_func(priv->data);
        priv->data = g_value_get_pointer(value);
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
    MilterReaderPrivate *priv;

    priv = MILTER_READER_GET_PRIVATE(object);
    switch (prop_id) {
      case PROP_WRITE_FUNC:
        g_value_set_pointer(value, priv->read_func);
        break;
      case PROP_DESTROY_FUNC:
        g_value_set_pointer(value, priv->destroy_func);
        break;
      case PROP_DATA:
        g_value_set_pointer(value, priv->data);
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

GQuark
milter_reader_error_quark (void)
{
    return g_quark_from_static_string("milter-reader-error-quark");
}

static gboolean
read_from_string (MilterReader *reader,
                  gchar       **read_data,
                  gsize        *read_size,
                  GError      **error)
{
    MilterReaderPrivate *priv;
    GString *string;

    priv = MILTER_READER_GET_PRIVATE(reader);

    if (read_size)
        *read_size = 0;

    string = priv->data;
    if (!string) {
        g_set_error(error,
                    MILTER_READER_ERROR, MILTER_READER_ERROR_NO_DATA,
                    "GString is NULL");
        return FALSE;
    }

    *read_data = g_strdup(string->str);
    if (read_size)
        *read_size = string->len;

    return TRUE;
}

MilterReader *
milter_reader_string_new (GString *string)
{
    return g_object_new(MILTER_TYPE_READER,
                        "read-func", read_from_string,
                        "data", string,
                        NULL);
}

static gboolean
read_from_io_channel (MilterReader *reader,
                      gchar       **read_data,
                      gsize        *read_size,
                      GError      **error)
{
    MilterReaderPrivate *priv;
    GIOChannel *input;
    gboolean success = TRUE;

    priv = MILTER_READER_GET_PRIVATE(reader);
    input = priv->data;

    if (read_size)
        *read_size = 0;

    if (!input) {
        g_set_error(error,
                    MILTER_READER_ERROR, MILTER_READER_ERROR_NO_DATA,
                    "GIOChannel is NULL");
        return FALSE;
    }

    g_io_channel_read_to_end(input, read_data, read_size, error);

    return success;
}

MilterReader *
milter_reader_io_channel_new (GIOChannel *channel)
{
    g_io_channel_ref(channel);
    return g_object_new(MILTER_TYPE_READER,
                        "read-func", read_from_io_channel,
                        "destroy-func", g_io_channel_unref,
                        "data", channel,
                        NULL);
}

gboolean
milter_reader_read (MilterReader *reader, 
                    gchar       **read_data,
                    gsize        *read_size,
                    GError      **error)
{
    MilterReaderPrivate *priv;

    priv = MILTER_READER_GET_PRIVATE(reader);

    if (read_size)
        *read_size = 0;
    if (!priv->read_func) {
        g_set_error(error,
                    MILTER_READER_ERROR, MILTER_READER_ERROR_NO_WRITE_FUNC,
                    "read func is missing");
        return FALSE;
    }

    return priv->read_func(reader, read_data, read_size, error, priv->data);
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
