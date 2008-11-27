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

#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include "milter-writer.h"

#define MILTER_WRITER_GET_PRIVATE(obj)                  \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj),                 \
                                 MILTER_TYPE_WRITER,    \
                                 MilterWriterPrivate))

typedef struct _MilterWriterPrivate	MilterWriterPrivate;
struct _MilterWriterPrivate
{
    GIOChannel *io_channel;
};

enum
{
    PROP_0,
    PROP_IO_CHANNEL
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


    spec = g_param_spec_pointer("io-channel",
                                "GIOChannel object",
                                "The GIOChannel object",
                                G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    g_object_class_install_property(gobject_class, PROP_IO_CHANNEL, spec);

    g_type_class_add_private(gobject_class, sizeof(MilterWriterPrivate));
}

static void
milter_writer_init (MilterWriter *writer)
{
    MilterWriterPrivate *priv;

    priv = MILTER_WRITER_GET_PRIVATE(writer);
    priv->io_channel = NULL;
}

static void
dispose (GObject *object)
{
    MilterWriterPrivate *priv;

    priv = MILTER_WRITER_GET_PRIVATE(object);
    if (priv->io_channel) {
        g_io_channel_unref(priv->io_channel);
        priv->io_channel = NULL;
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
      case PROP_IO_CHANNEL:
        priv->io_channel = g_value_get_pointer(value);
        if (priv->io_channel)
            g_io_channel_ref(priv->io_channel);
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
      case PROP_IO_CHANNEL:
        g_value_set_pointer(value, priv->io_channel);
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

#define BUFFER_SIZE 4096
static gboolean
write_to_io_channel (MilterWriter *writer, const gchar *chunk, gsize chunk_size,
                     gsize *written_size, GError **error)
{
    MilterWriterPrivate *priv;
    gsize rest_size, current_written_size;
    gboolean success = TRUE;

    priv = MILTER_WRITER_GET_PRIVATE(writer);

    if (chunk_size == 0)
        return TRUE;

    rest_size = chunk_size;

    while (rest_size > 0) {
        gsize bytes = MIN(BUFFER_SIZE, rest_size);
        GIOStatus status;

        status = g_io_channel_write_chars(priv->io_channel,
                                          chunk, bytes,
                                          &current_written_size,
                                          error);

        rest_size -= current_written_size;
        chunk += current_written_size;

        if (status == G_IO_STATUS_ERROR) {
            success = FALSE;
            break;
        }

        g_io_channel_flush(priv->io_channel, NULL);
    }

    if (written_size)
        *written_size = chunk_size - rest_size;

    return success;
}

MilterWriter *
milter_writer_io_channel_new (GIOChannel *channel)
{
    return g_object_new(MILTER_TYPE_WRITER,
                        "io-channel", channel,
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
    if (!priv->io_channel) {
        g_set_error(error,
                    MILTER_WRITER_ERROR, MILTER_WRITER_ERROR_NO_CHANNEL,
                    "GIOChannel is NULL");
        return FALSE;
    }

    return write_to_io_channel(writer, chunk, chunk_size, written_size, error);
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
