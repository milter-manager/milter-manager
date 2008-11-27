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

#include "milter-reader.h"
#include "milter-logger.h"
#include "milter-utils.h"
#include "milter-marshalers.h"

#define MILTER_READER_GET_PRIVATE(obj)                  \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj),                 \
                                 MILTER_TYPE_READER,    \
                                 MilterReaderPrivate))

typedef struct _MilterReaderPrivate	MilterReaderPrivate;
struct _MilterReaderPrivate
{
    GIOChannel *io_channel;
    guint channel_watch_id;
};

enum
{
    PROP_0,
    PROP_IO_CHANNEL
};

enum
{
    FLOW,
    LAST_SIGNAL
};

static gint signals[LAST_SIGNAL] = {0};

MILTER_IMPLEMENT_ERROR_EMITTABLE(error_emittable_init);
MILTER_IMPLEMENT_FINISHED_EMITTABLE(finished_emittable_init);
G_DEFINE_TYPE_WITH_CODE(MilterReader, milter_reader, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(MILTER_TYPE_ERROR_EMITTABLE, error_emittable_init)
    G_IMPLEMENT_INTERFACE(MILTER_TYPE_FINISHED_EMITTABLE, finished_emittable_init))

static GObject *constructor  (GType                  type,
                              guint                  n_props,
                              GObjectConstructParam *props);

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

    gobject_class->constructor  = constructor;
    gobject_class->dispose      = dispose;
    gobject_class->set_property = set_property;
    gobject_class->get_property = get_property;

    spec = g_param_spec_pointer("io-channel",
                                "GIOChannel object",
                                "The GIOChannel object",
                                G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    g_object_class_install_property(gobject_class, PROP_IO_CHANNEL, spec);

    signals[FLOW] =
        g_signal_new("flow",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterReaderClass, flow),
                     NULL, NULL,
                     _milter_marshal_VOID__POINTER_UINT,
                     G_TYPE_NONE, 2, G_TYPE_POINTER, G_TYPE_UINT);

    g_type_class_add_private(gobject_class, sizeof(MilterReaderPrivate));
}

static void
milter_reader_init (MilterReader *reader)
{
    MilterReaderPrivate *priv;

    priv = MILTER_READER_GET_PRIVATE(reader);
    priv->io_channel = NULL;
    priv->channel_watch_id = 0;
}

#define BUFFER_SIZE 4096
static gboolean
read_from_channel (MilterReader *reader, GIOChannel *channel)
{
    gboolean error_occurred = FALSE;
    gboolean eof = FALSE;

    while (!error_occurred) {
        GIOStatus status;
        gchar stream[BUFFER_SIZE + 1];
        gsize length = 0;
        GError *io_error = NULL;

        status = g_io_channel_read_chars(channel, stream, BUFFER_SIZE,
                                         &length, &io_error);
        if (status == G_IO_STATUS_EOF) {
            eof = TRUE;
        }
        if (io_error) {
            GError *error = NULL;
            milter_utils_set_error_with_sub_error(&error,
                                                  MILTER_READER_ERROR,
                                                  MILTER_READER_ERROR_IO_ERROR,
                                                  io_error,
                                                  "I/O error");

            milter_error_emittable_emit(MILTER_ERROR_EMITTABLE(reader),
                                        error);
            milter_error("error: %s", error->message);
            g_error_free(error);
            error_occurred = TRUE;
            break;
        }

        if (length <= 0)
            break;

        g_signal_emit(reader, signals[FLOW], 0, stream, length);

        if (eof)
            break;
    }

    return !error_occurred && !eof;
}

static gboolean
channel_watch_func (GIOChannel *channel, GIOCondition condition, gpointer data)
{
    MilterReader *reader = data;
    gboolean keep_callback = TRUE;

    if (condition & G_IO_IN ||
        condition & G_IO_PRI) {
        milter_info("reading from io channel...");
        keep_callback = read_from_channel(reader, channel);
    }

    if (condition & G_IO_ERR ||
        condition & G_IO_HUP ||
        condition & G_IO_NVAL) {
        gchar *message;
        GError *error = NULL;

        message = milter_utils_inspect_io_condition_error(condition);
        g_set_error(&error,
                    MILTER_READER_ERROR,
                    MILTER_READER_ERROR_IO_ERROR,
                    "%s", message);
        milter_error_emittable_emit(MILTER_ERROR_EMITTABLE(reader),
                                    error);
        milter_error("error!: %s", message);
        g_error_free(error);
        g_free(message);
        keep_callback = FALSE;
    }

    if (!keep_callback) {
        MilterReaderPrivate *priv;

        milter_info("done.");
        priv = MILTER_READER_GET_PRIVATE(reader);
        priv->channel_watch_id = 0;
        milter_finished_emittable_emit(MILTER_FINISHED_EMITTABLE(reader));
    }

    return keep_callback;
}

static void
watch_io_channel (MilterReader *reader)
{
    MilterReaderPrivate *priv;

    priv = MILTER_READER_GET_PRIVATE(reader);

    priv->channel_watch_id = g_io_add_watch(priv->io_channel,
                                            G_IO_IN | G_IO_PRI |
                                            G_IO_ERR | G_IO_HUP | G_IO_NVAL,
                                            channel_watch_func, reader);
}

static GObject *
constructor (GType type, guint n_props, GObjectConstructParam *props)
{
    GObject *object;
    GObjectClass *klass;
    MilterReaderPrivate *priv;

    klass = G_OBJECT_CLASS(milter_reader_parent_class);
    object = klass->constructor(type, n_props, props);

    priv = MILTER_READER_GET_PRIVATE(object);
    if (priv->io_channel)
        watch_io_channel(MILTER_READER(object));

    return object;
}

static void
dispose (GObject *object)
{
    MilterReaderPrivate *priv;

    priv = MILTER_READER_GET_PRIVATE(object);

    if (priv->channel_watch_id) {
        g_source_remove(priv->channel_watch_id);
        priv->channel_watch_id = 0;
    }

    if (priv->io_channel) {
        g_io_channel_unref(priv->io_channel);
        priv->io_channel = NULL;
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
    MilterReaderPrivate *priv;

    priv = MILTER_READER_GET_PRIVATE(object);
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
milter_reader_error_quark (void)
{
    return g_quark_from_static_string("milter-reader-error-quark");
}

MilterReader *
milter_reader_io_channel_new (GIOChannel *channel)
{
    return g_object_new(MILTER_TYPE_READER,
                        "io-channel", channel,
                        NULL);
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
