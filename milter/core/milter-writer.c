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
    MilterEventLoop *loop;
    GString *buffer;
    gsize flush_point;
    gboolean writing;
    guint write_watch_id;
    guint flush_watch_id;
    guint error_watch_id;
    guint tag;
};

enum
{
    PROP_0,
    PROP_IO_CHANNEL,
    PROP_TAG
};

enum
{
    FLUSHED,
    LAST_SIGNAL
};

static gint signals[LAST_SIGNAL] = {0};

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

    spec = g_param_spec_uint("tag",
                             "Tag",
                             "The tag of the reader",
                             0, G_MAXUINT, 0,
                             G_PARAM_READABLE);
    g_object_class_install_property(gobject_class, PROP_TAG, spec);

    /**
     * MilterWriter::flushed:
     * @writer: the writer that received the signal.
     *
     * This signal is emitted on flush.
     */
    signals[FLUSHED] =
        g_signal_new("flushed",
                     MILTER_TYPE_WRITER,
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterWriterClass, flushed),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__VOID,
                     G_TYPE_NONE, 0);

    g_type_class_add_private(gobject_class, sizeof(MilterWriterPrivate));
}

static void
milter_writer_init (MilterWriter *writer)
{
    MilterWriterPrivate *priv;

    priv = MILTER_WRITER_GET_PRIVATE(writer);
    priv->io_channel = NULL;
    priv->loop = NULL;
    priv->buffer = g_string_new(NULL);
    priv->flush_point = 0;
    priv->writing = FALSE;
    priv->write_watch_id = 0;
    priv->flush_watch_id = 0;
    priv->error_watch_id = 0;
    priv->tag = 0;
}

static void
clear_write_watch_id (MilterWriterPrivate *priv)
{
    if (priv->write_watch_id) {
        milter_event_loop_remove(priv->loop, priv->write_watch_id);
        priv->write_watch_id = 0;
    }
}

static void
clear_flush_watch_id (MilterWriterPrivate *priv)
{
    if (priv->flush_watch_id) {
        milter_event_loop_remove(priv->loop, priv->flush_watch_id);
        priv->flush_watch_id = 0;
    }
}

static void
clear_watch_id (MilterWriterPrivate *priv)
{
    clear_write_watch_id(priv);
    clear_flush_watch_id(priv);

    if (priv->error_watch_id) {
        milter_event_loop_remove(priv->loop, priv->error_watch_id);
        priv->error_watch_id = 0;
    }

    if (priv->loop) {
        g_object_unref(priv->loop);
        priv->loop = NULL;
    }
}

static void
dispose (GObject *object)
{
    MilterWriterPrivate *priv;

    priv = MILTER_WRITER_GET_PRIVATE(object);

    milter_trace("[%u] [writer][dispose]", priv->tag);

    clear_watch_id(priv);

    if (priv->io_channel) {
        g_io_channel_unref(priv->io_channel);
        priv->io_channel = NULL;
    }

    if (priv->buffer) {
        if (priv->buffer->len > 0) {
            milter_debug("[%u] [writer][dispose][buffer][unwritten] "
                         "<%" G_GSIZE_FORMAT ">",
                         priv->tag, priv->buffer->len);
        }
        g_string_free(priv->buffer, TRUE);
        priv->buffer = NULL;
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
    case PROP_TAG:
        milter_writer_set_tag(MILTER_WRITER(object), g_value_get_uint(value));
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
    case PROP_TAG:
        g_value_set_uint(value, priv->tag);
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

MilterWriter *
milter_writer_io_channel_new (GIOChannel *channel)
{
    return g_object_new(MILTER_TYPE_WRITER,
                        "io-channel", channel,
                        NULL);
}

static gboolean
flush_watch_func (GIOChannel *channel, GIOCondition condition, gpointer data)
{
    MilterWriter *writer = data;
    MilterWriterPrivate *priv;
    gboolean keep_callback = FALSE;
    GIOStatus status;
    GError *channel_error = NULL;

    priv = MILTER_WRITER_GET_PRIVATE(writer);

    milter_trace("[%u] [writer][flush-callback] [%u]",
                 priv->tag, priv->flush_watch_id);

    status = g_io_channel_flush(priv->io_channel, &channel_error);
    if (channel_error) {
        GError *error = NULL;

        keep_callback = FALSE;
        milter_utils_set_error_with_sub_error(
            &error,
            MILTER_WRITER_ERROR,
            MILTER_WRITER_ERROR_IO_ERROR,
            channel_error,
            "failed to flush");
        milter_error("[%u] [writer][flush-callback][error] [%u] %s",
                     priv->tag,
                     priv->flush_watch_id,
                     error->message);
        milter_error_emittable_emit(MILTER_ERROR_EMITTABLE(writer), error);
        g_error_free(error);
    } else {
        if (status == G_IO_STATUS_AGAIN) {
            keep_callback = TRUE;
        } else {
            g_signal_emit(writer, signals[FLUSHED], 0);
        }
    }

    if (!keep_callback) {
        milter_trace("[%u] [writer][flush-callback][finish] [%u]",
                     priv->tag, priv->flush_watch_id);
        priv->flush_watch_id = 0;
    }

    return keep_callback;
}

static void
request_flush (MilterWriter *writer)
{
    MilterWriterPrivate *priv;

    priv = MILTER_WRITER_GET_PRIVATE(writer);
    if (priv->flush_watch_id == 0) {
        priv->flush_watch_id =
            milter_event_loop_watch_io(priv->loop,
                                       priv->io_channel,
                                       G_IO_OUT,
                                       flush_watch_func, writer);
        milter_trace("[%u] [writer][flush-callback][registered] <%u>",
                     priv->tag, priv->flush_watch_id);
    } else {
        milter_trace("[%u] [writer][flush-callback][register][reuse] [%u]",
                     priv->tag, priv->flush_watch_id);
    }
}

static gboolean
write_watch_func (GIOChannel *channel, GIOCondition condition, gpointer data)
{
    MilterWriter *writer = data;
    MilterWriterPrivate *priv;
    gboolean keep_callback = TRUE;

    priv = MILTER_WRITER_GET_PRIVATE(writer);

    milter_trace("[%u] [writer][write-callback] [%u] "
                 "buffered: <%" G_GSIZE_FORMAT ">",
                 priv->tag, priv->write_watch_id, priv->buffer->len);

    if (priv->buffer->len == 0) {
        keep_callback = FALSE;
        milter_trace("[%u] [writer][write-callback][empty] [%u] "
                     "stop write watch because buffer is empty",
                     priv->tag, priv->write_watch_id);
    } else {
        gsize written_size = 0;
        GError *channel_error = NULL;

        priv->writing = TRUE;
        g_io_channel_write_chars(priv->io_channel,
                                 priv->buffer->str,
                                 priv->buffer->len,
                                 &written_size,
                                 &channel_error);
        priv->writing = FALSE;

        if (written_size == 0) {
            milter_trace("[%u] [writer][write-callback][unwritten] [%u] "
                         "no buffered chunks are written: "
                         "rest: <%" G_GSIZE_FORMAT ">",
                         priv->tag, priv->write_watch_id, priv->buffer->len);
        } else {
            gboolean need_flush = FALSE;

            if (priv->flush_point > 0) {
                if (priv->flush_point <= written_size) {
                    need_flush = TRUE;
                    priv->flush_point = 0;
                } else {
                    priv->flush_point -= written_size;
                }
            }
            g_string_erase(priv->buffer, 0, written_size);
            milter_trace("[%u] [writer][write-callback][wrote] [%u] "
                         "written: <%" G_GSIZE_FORMAT "> "
                         "rest: <%" G_GSIZE_FORMAT "> "
                         "need-flush: <%s>",
                         priv->tag,
                         priv->write_watch_id,
                         written_size,
                         priv->buffer->len,
                         need_flush ? "true" : "false");
            if (need_flush && priv->loop) {
                request_flush(writer);
            }
        }

        if (channel_error) {
            GError *error = NULL;

            keep_callback = FALSE;
            milter_utils_set_error_with_sub_error(
                &error,
                MILTER_WRITER_ERROR,
                MILTER_WRITER_ERROR_IO_ERROR,
                channel_error,
                "failed to write");
            milter_error("[%u] [writer][write-callback][error] [%u] %s",
                         priv->tag,
                         priv->write_watch_id,
                         error->message);
            milter_error_emittable_emit(MILTER_ERROR_EMITTABLE(writer), error);
            g_error_free(error);
        }
    }

    if (!keep_callback) {
        milter_trace("[%u] [writer][write-callback][finish] [%u]",
                     priv->tag, priv->write_watch_id);
        priv->write_watch_id = 0;
    }

    return keep_callback;
}

gboolean
milter_writer_write (MilterWriter *writer, const gchar *chunk, gsize chunk_size,
                     GError **error)
{
    MilterWriterPrivate *priv;

    priv = MILTER_WRITER_GET_PRIVATE(writer);

    if (!priv->io_channel) {
        const gchar *message = "no write channel";
        g_set_error(error,
                    MILTER_WRITER_ERROR, MILTER_WRITER_ERROR_NO_CHANNEL,
                    "%s", message);
        milter_error("[%u] [writer][write][error] %s",
                     priv->tag, message);
        return FALSE;
    }

    if (!priv->loop) {
        const gchar *message = "can't write to not started or shutdown channel";
        g_set_error(error,
                    MILTER_WRITER_ERROR, MILTER_WRITER_ERROR_NOT_READY,
                    "%s", message);
        milter_error("[%u] [writer][write][error] %s",
                     priv->tag, message);
        return FALSE;
    }

    if (chunk_size == 0) {
        milter_debug("[%u] [writer][write][empty] "
                     "ignore empty chunk write request",
                     priv->tag);
        return TRUE;
    }

    g_string_append_len(priv->buffer, chunk, chunk_size);
    if (priv->write_watch_id == 0) {
        priv->write_watch_id =
            milter_event_loop_watch_io(priv->loop,
                                       priv->io_channel,
                                       G_IO_OUT,
                                       write_watch_func, writer);
        milter_trace("[%u] [writer][write-callback][registered] [%u]",
                     priv->tag, priv->write_watch_id);
    } else {
        milter_trace("[%u] [writer][write-callback][register][reuse] [%u]",
                     priv->tag, priv->write_watch_id);
    }

    return TRUE;
}

gboolean
milter_writer_flush (MilterWriter *writer, GError **error)
{
    MilterWriterPrivate *priv;

    priv = MILTER_WRITER_GET_PRIVATE(writer);

    if (!priv->io_channel) {
        const gchar *message = "no write channel";
        g_set_error(error,
                    MILTER_WRITER_ERROR, MILTER_WRITER_ERROR_NO_CHANNEL,
                    "%s", message);
        milter_error("[%u] [writer][flush][error] %s",
                     priv->tag, message);
        return FALSE;
    }

    if (!priv->loop) {
        const gchar *message = "can't flush to not started or shutdown channel";
        g_set_error(error,
                    MILTER_WRITER_ERROR, MILTER_WRITER_ERROR_NOT_READY,
                    "%s", message);
        milter_error("[%u] [writer][flush][error] %s",
                     priv->tag, message);
        return FALSE;
    }

    if (priv->write_watch_id > 0) {
        priv->flush_point = priv->buffer->len;
        milter_trace("[%u] [writer][flush][flush-point][set] [%u] "
                     "<%" G_GSIZE_FORMAT ">",
                     priv->tag,
                     priv->write_watch_id,
                     priv->flush_point);
    } else {
        request_flush(writer);
    }

    return TRUE;
}

static gboolean
error_watch_func (GIOChannel *channel, GIOCondition condition, gpointer data)
{
    MilterWriterPrivate *priv;
    MilterWriter *writer = data;
    gboolean keep_callback = FALSE;
    gchar *message;
    GError *error = NULL;

    priv = MILTER_WRITER_GET_PRIVATE(writer);

    message = milter_utils_inspect_io_condition_error(condition);
    g_set_error(&error,
                MILTER_WRITER_ERROR,
                MILTER_WRITER_ERROR_IO_ERROR,
                "%s", message);
    milter_error("[%u] [writer][error-callback][error] %s", priv->tag, message);
    milter_error_emittable_emit(MILTER_ERROR_EMITTABLE(writer), error);
    g_error_free(error);
    g_free(message);
    keep_callback = FALSE;

    if (!keep_callback) {
        milter_trace("[%u] [writer][error-callback][finish]", priv->tag);
        priv->error_watch_id = 0;
        clear_write_watch_id(priv);
        clear_flush_watch_id(priv);
        milter_finished_emittable_emit(MILTER_FINISHED_EMITTABLE(writer));
    }

    return keep_callback;
}

static void
watch_io_channel (MilterWriter *writer, MilterEventLoop *loop)
{
    MilterWriterPrivate *priv;

    priv = MILTER_WRITER_GET_PRIVATE(writer);

    priv->error_watch_id =
        milter_event_loop_watch_io(loop,
                                   priv->io_channel,
                                   G_IO_ERR | G_IO_HUP | G_IO_NVAL,
                                   error_watch_func, writer);
    if (priv->error_watch_id > 0) {
        g_object_ref(loop);
        if (priv->loop) {
            g_object_unref(priv->loop);
        }
        priv->loop = loop;
    } else {
        milter_error("[%u] [writer][watch][fail] TODO: raise error",
                     priv->tag);
    }

    milter_trace("[%u] [writer][watch] %u", priv->tag, priv->error_watch_id);
}

void
milter_writer_start (MilterWriter *writer, MilterEventLoop *loop)
{
    MilterWriterPrivate *priv;

    priv = MILTER_WRITER_GET_PRIVATE(writer);
    if (priv->io_channel && priv->error_watch_id == 0) {
        watch_io_channel(writer, loop);
    }
}

gboolean
milter_writer_is_watching (MilterWriter *writer)
{
    return MILTER_WRITER_GET_PRIVATE(writer)->error_watch_id > 0;
}

static void
flush_buffer_on_shutdown (MilterWriter *writer)
{
    MilterWriterPrivate *priv;
    gsize written_size = 0;
    GError *channel_error = NULL;

    priv = MILTER_WRITER_GET_PRIVATE(writer);

    milter_trace("[%u] [writer][shutdown][flush-buffer] "
                 "<%" G_GSIZE_FORMAT ">",
                 priv->tag, priv->buffer->len);

    if (priv->buffer->len == 0) {
        milter_trace("[%u] [writer][shutdown][flush-buffer][skip] "
                     "no buffered data",
                     priv->tag);
        return;
    }

    if (priv->writing) {
        milter_trace("[%u] [writer][shutdown][flush-buffer][skip] writing",
                     priv->tag);
        return;
    }

    g_io_channel_write_chars(priv->io_channel,
                             priv->buffer->str,
                             priv->buffer->len,
                             &written_size,
                             &channel_error);

    if (written_size == 0) {
        milter_trace("[%u] [writer][shutdown][flush-buffer][unwritten] "
                     "no buffered chunks are written: "
                     "rest: <%" G_GSIZE_FORMAT ">",
                     priv->tag, priv->buffer->len);
    } else {
        g_string_erase(priv->buffer, 0, written_size);
        milter_trace("[%u] [writer][shutdown][flush-buffer][wrote] "
                     "written: <%" G_GSIZE_FORMAT "> "
                     "rest: <%" G_GSIZE_FORMAT ">",
                     priv->tag,
                     written_size,
                     priv->buffer->len);
    }

    if (channel_error) {
        GError *error = NULL;

        milter_utils_set_error_with_sub_error(
            &error,
            MILTER_WRITER_ERROR,
            MILTER_WRITER_ERROR_IO_ERROR,
            channel_error,
            "failed to flush buffer on shutdown");
        milter_error("[%u] [writer][shutdown][flush-buffer][write][error] %s",
                     priv->tag, error->message);
        milter_error_emittable_emit(MILTER_ERROR_EMITTABLE(writer), error);
        g_error_free(error);
    } else {
        g_io_channel_flush(priv->io_channel, &channel_error);
        if (channel_error) {
            GError *error = NULL;

            milter_utils_set_error_with_sub_error(
                &error,
                MILTER_WRITER_ERROR,
                MILTER_WRITER_ERROR_IO_ERROR,
                channel_error,
                "failed to flush written data in buffer on shutdown");
            milter_error("[%u] [writer][shutdown][flush-buffer][flush][error] "
                         "%s",
                         priv->tag, error->message);
            milter_error_emittable_emit(MILTER_ERROR_EMITTABLE(writer), error);
            g_error_free(error);
        }
        g_signal_emit(writer, signals[FLUSHED], 0);
    }
}

void
milter_writer_shutdown (MilterWriter *writer)
{
    MilterWriterPrivate *priv;

    priv = MILTER_WRITER_GET_PRIVATE(writer);

    if (priv->error_watch_id == 0) {
        milter_debug("[%u] [writer][shutdown][not-started]", priv->tag);
        return;
    }

    clear_watch_id(priv);

    if (priv->io_channel) {
        flush_buffer_on_shutdown(writer);
        milter_trace("[%u] [writer][shutdown][unref]", priv->tag);
        g_io_channel_unref(priv->io_channel);
        priv->io_channel = NULL;
    }
}

guint
milter_writer_get_tag (MilterWriter *writer)
{
    return MILTER_WRITER_GET_PRIVATE(writer)->tag;
}

void
milter_writer_set_tag (MilterWriter *writer, guint tag)
{
    MilterWriterPrivate *priv;

    priv = MILTER_WRITER_GET_PRIVATE(writer);
    priv->tag = tag;
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
