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

#ifdef HAVE_CONFIG_H
#  include "../../config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include "milter-writer.h"
#include "milter-logger.h"
#include "milter-utils.h"

#define MILTER_WRITER_GET_PRIVATE(obj)                  \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj),                 \
                                 MILTER_TYPE_WRITER,    \
                                 MilterWriterPrivate))

typedef struct _MilterWriterPrivate	MilterWriterPrivate;
struct _MilterWriterPrivate
{
    GIOChannel *io_channel;
    guint channel_watch_id;
};

enum
{
    PROP_0,
    PROP_IO_CHANNEL
};

MILTER_IMPLEMENT_ERROR_EMITTABLE(error_emittable_init);
MILTER_IMPLEMENT_FINISHED_EMITTABLE(finished_emittable_init);
G_DEFINE_TYPE_WITH_CODE(
    MilterWriter, milter_writer, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(MILTER_TYPE_ERROR_EMITTABLE, error_emittable_init)
    G_IMPLEMENT_INTERFACE(MILTER_TYPE_FINISHED_EMITTABLE, finished_emittable_init))

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
    priv->channel_watch_id = 0;
}

static void
dispose (GObject *object)
{
    MilterWriterPrivate *priv;

    priv = MILTER_WRITER_GET_PRIVATE(object);

    if (priv->channel_watch_id > 0) {
        g_source_remove(priv->channel_watch_id);
        priv->channel_watch_id = 0;
    }

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

gboolean
milter_writer_flush (MilterWriter *writer, GError **error)
{
    MilterWriterPrivate *priv;

    priv = MILTER_WRITER_GET_PRIVATE(writer);

    if (!priv->io_channel) {
        g_set_error(error,
                    MILTER_WRITER_ERROR, MILTER_WRITER_ERROR_NO_CHANNEL,
                    "GIOChannel is NULL");
        return FALSE;
    }

    return g_io_channel_flush(priv->io_channel, error) != G_IO_CHANNEL_ERROR;
}

static gboolean
channel_watch_func (GIOChannel *channel, GIOCondition condition, gpointer data)
{
    MilterWriterPrivate *priv;
    MilterWriter *writer = data;
    gboolean keep_callback = TRUE;

    priv = MILTER_WRITER_GET_PRIVATE(writer);

    if (condition & G_IO_ERR ||
        condition & G_IO_HUP ||
        condition & G_IO_NVAL) {
        gchar *message;
        GError *error = NULL;

        message = milter_utils_inspect_io_condition_error(condition);
        g_set_error(&error,
                    MILTER_WRITER_ERROR,
                    MILTER_WRITER_ERROR_IO_ERROR,
                    "%s", message);
        milter_error("[writer][error] %s", message);
        milter_error_emittable_emit(MILTER_ERROR_EMITTABLE(writer), error);
        g_error_free(error);
        g_free(message);
        keep_callback = FALSE;
    }

    if (!keep_callback) {
        milter_debug("[writer] removing writer watcher");
        priv->channel_watch_id = 0;
        milter_finished_emittable_emit(MILTER_FINISHED_EMITTABLE(writer));
    }

    return keep_callback;
}

static void
watch_io_channel (MilterWriter *writer)
{
    MilterWriterPrivate *priv;

    priv = MILTER_WRITER_GET_PRIVATE(writer);

    priv->channel_watch_id = g_io_add_watch(priv->io_channel,
                                            G_IO_ERR | G_IO_HUP | G_IO_NVAL,
                                            channel_watch_func, writer);
}

void
milter_writer_start (MilterWriter *writer)
{
    MilterWriterPrivate *priv;

    priv = MILTER_WRITER_GET_PRIVATE(writer);
    if (priv->io_channel && priv->channel_watch_id == 0)
        watch_io_channel(writer);
}

gboolean
milter_writer_is_watching (MilterWriter *writer)
{
    return MILTER_WRITER_GET_PRIVATE(writer)->channel_watch_id > 0;
}

void
milter_writer_shutdown (MilterWriter *writer)
{
    MilterWriterPrivate *priv;
    GError *channel_error = NULL;

    priv = MILTER_WRITER_GET_PRIVATE(writer);

    if (priv->channel_watch_id == 0)
        return;

    g_io_channel_shutdown(priv->io_channel, TRUE, &channel_error);
    if (channel_error) {
        GError *error = NULL;

        milter_utils_set_error_with_sub_error(
            &error,
            MILTER_WRITER_ERROR,
            MILTER_WRITER_ERROR_IO_ERROR,
            channel_error,
            "failed to shutdown");
        milter_error("[writer][error][shutdown] %s", error->message);
        milter_error_emittable_emit(MILTER_ERROR_EMITTABLE(writer), error);
        g_error_free(error);
    } else {
        g_source_remove(priv->channel_watch_id);
        priv->channel_watch_id = 0;
    }
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
