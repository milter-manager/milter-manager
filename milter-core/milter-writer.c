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

#include "milter-writer.h"

#define MILTER_WRITER_GET_PRIVATE(obj)                  \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj),                 \
                                 MILTER_TYPE_WRITER,    \
                                 MilterWriterPrivate))

typedef gboolean (*WriteFunc) (MilterWriter  *writer,
                               const gchar   *chunk,
                               gsize          chunk_size,
                               gsize         *written_size,
                               GError       **error,
                               gpointer       user_data);

typedef struct _MilterWriterPrivate	MilterWriterPrivate;
struct _MilterWriterPrivate
{
    WriteFunc write_func;
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

G_DEFINE_TYPE(MilterWriter, milter_writer, G_TYPE_OBJECT);

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
milter_writer_class_init (MilterWriterClass *klass)
{
    GObjectClass *gobject_class;
    GParamSpec *spec;

    gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->dispose      = dispose;
    gobject_class->set_property = set_property;
    gobject_class->get_property = get_property;

    spec = g_param_spec_pointer("write-func",
                                "Write function",
                                "The write function of the writer",
                                G_PARAM_READWRITE);
    g_object_class_install_property(gobject_class, PROP_WRITE_FUNC, spec);

    spec = g_param_spec_pointer("destroy-func",
                                "Destroy function",
                                "The destroy function of the writer",
                                G_PARAM_READWRITE);
    g_object_class_install_property(gobject_class, PROP_DESTROY_FUNC, spec);

    spec = g_param_spec_pointer("data",
                                "Data",
                                "The data of the writer",
                                G_PARAM_READWRITE);
    g_object_class_install_property(gobject_class, PROP_DATA, spec);

    g_type_class_add_private(gobject_class, sizeof(MilterWriterPrivate));
}

static void
milter_writer_init (MilterWriter *writer)
{
    MilterWriterPrivate *priv;

    priv = MILTER_WRITER_GET_PRIVATE(writer);
    priv->write_func = NULL;
    priv->destroy_func = NULL;
    priv->data = NULL;
}

static void
dispose (GObject *object)
{
    MilterWriterPrivate *priv;

    priv = MILTER_WRITER_GET_PRIVATE(object);
    if (priv->data && priv->destroy_func) {
        priv->destroy_func(priv->data);
        priv->data = NULL;
    }

    G_OBJECT_CLASS(milter_writer_parent_class)->dispose(object);
}

static void
set_property (GObject      *object,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
    MilterWriterPrivate *priv;

    priv = MILTER_WRITER_GET_PRIVATE(object);
    switch (prop_id) {
      case PROP_WRITE_FUNC:
        priv->write_func = g_value_get_pointer(value);
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
    MilterWriterPrivate *priv;

    priv = MILTER_WRITER_GET_PRIVATE(object);
    switch (prop_id) {
      case PROP_WRITE_FUNC:
        g_value_set_pointer(value, priv->write_func);
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
milter_writer_error_quark (void)
{
    return g_quark_from_static_string("milter-writer-error-quark");
}

static gboolean
write_to_string (MilterWriter *writer, const gchar *chunk, gsize chunk_size,
                 gsize *written_size, GError **error)
{
    MilterWriterPrivate *priv;
    GString *string;

    priv = MILTER_WRITER_GET_PRIVATE(writer);

    if (written_size)
        *written_size = 0;

    if (chunk_size == 0)
        return TRUE;

    string = priv->data;
    if (!string) {
        g_set_error(error,
                    MILTER_WRITER_ERROR, MILTER_WRITER_ERROR_NO_DATA,
                    "GString is NULL");
        return FALSE;
    }

    g_string_append_len(string, chunk, chunk_size);
    if (written_size)
        *written_size = chunk_size;

    return TRUE;
}

MilterWriter *
milter_writer_string_new (GString *string)
{
    return g_object_new(MILTER_TYPE_WRITER,
                        "write-func", write_to_string,
                        "data", string,
                        NULL);
}

static gboolean
write_to_io_channel (MilterWriter *writer, const gchar *chunk, gsize chunk_size,
                     gsize *written_size, GError **error)
{
    MilterWriterPrivate *priv;
    GIOChannel *output;
    gsize rest_size, current_written_size;
    gboolean success = TRUE;

    priv = MILTER_WRITER_GET_PRIVATE(writer);
    output = priv->data;

    if (written_size)
        *written_size = 0;

    if (chunk_size == 0)
        return TRUE;

    if (!output) {
        g_set_error(error,
                    MILTER_WRITER_ERROR, MILTER_WRITER_ERROR_NO_DATA,
                    "GIOChannel is NULL");
        return FALSE;
    }

    for (rest_size = chunk_size;
         rest_size > 0;
         rest_size -= current_written_size) {
        GIOStatus status;

        status = g_io_channel_write_chars(output,
                                          chunk + (chunk_size - rest_size),
                                          rest_size,
                                          &current_written_size,
                                          error);
        if (status == G_IO_STATUS_ERROR) {
            success = FALSE;
            break;
        }
    }
    g_io_channel_flush(output, NULL);
    if (written_size)
        *written_size = chunk_size - rest_size;

    return success;
}

MilterWriter *
milter_writer_io_channel_new (GIOChannel *channel)
{
    g_io_channel_ref(channel);
    return g_object_new(MILTER_TYPE_WRITER,
                        "write-func", write_to_io_channel,
                        "destroy-func", g_io_channel_unref,
                        "data", channel,
                        NULL);
}

gboolean
milter_writer_write (MilterWriter *writer, const gchar *chunk, gsize chunk_size,
                     gsize *written_size, GError **error)
{
    MilterWriterPrivate *priv;

    priv = MILTER_WRITER_GET_PRIVATE(writer);

    if (written_size)
        *written_size = 0;
    if (!priv->write_func) {
        g_set_error(error,
                    MILTER_WRITER_ERROR, MILTER_WRITER_ERROR_NO_WRITE_FUNC,
                    "write func is missing");
        return FALSE;
    }

    return priv->write_func(writer, chunk, chunk_size, written_size, error,
                            priv->data);
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
