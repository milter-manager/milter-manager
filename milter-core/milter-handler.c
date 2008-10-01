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

#include <string.h>
#include <stdlib.h>

#include <milter-core.h>
#include "milter-handler.h"

#define MILTER_HANDLER_GET_PRIVATE(obj)                          \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj),                          \
                                 MILTER_TYPE_HANDLER,            \
                                 MilterHandlerPrivate))

typedef struct _MilterHandlerPrivate	MilterHandlerPrivate;
struct _MilterHandlerPrivate
{
    MilterDecoder *decoder;
    MilterEncoder *encoder;
    MilterWriter *writer;
};

G_DEFINE_ABSTRACT_TYPE(MilterHandler, milter_handler, G_TYPE_OBJECT);

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
milter_handler_class_init (MilterHandlerClass *klass)
{
    GObjectClass *gobject_class;

    gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->dispose      = dispose;
    gobject_class->set_property = set_property;
    gobject_class->get_property = get_property;

    g_type_class_add_private(gobject_class, sizeof(MilterHandlerPrivate));
}

static void
milter_handler_init (MilterHandler *handler)
{
    MilterHandlerPrivate *priv;

    priv = MILTER_HANDLER_GET_PRIVATE(handler);
    priv->decoder = milter_decoder_new();
    priv->encoder = milter_encoder_new();
    priv->writer = NULL;
}

static void
dispose (GObject *object)
{
    MilterHandlerPrivate *priv;

    priv = MILTER_HANDLER_GET_PRIVATE(object);

    if (priv->decoder) {
        g_object_unref(priv->decoder);
        priv->decoder = NULL;
    }

    if (priv->encoder) {
        g_object_unref(priv->encoder);
        priv->encoder = NULL;
    }

    if (priv->writer) {
        g_object_unref(priv->writer);
        priv->writer = NULL;
    }

    G_OBJECT_CLASS(milter_handler_parent_class)->dispose(object);
}

static void
set_property (GObject      *object,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
    MilterHandlerPrivate *priv;

    priv = MILTER_HANDLER_GET_PRIVATE(object);
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
    MilterHandlerPrivate *priv;

    priv = MILTER_HANDLER_GET_PRIVATE(object);
    switch (prop_id) {
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

GQuark
milter_handler_error_quark (void)
{
    return g_quark_from_static_string("milter-handler-error-quark");
}

gboolean
milter_handler_feed (MilterHandler *handler,
                     const gchar *chunk, gsize size,
                     GError **error)
{
    MilterHandlerPrivate *priv;

    priv = MILTER_HANDLER_GET_PRIVATE(handler);
    return milter_decoder_decode(priv->decoder, chunk, size, error);
}

gboolean
milter_handler_write_packet (MilterHandler *handler,
                             const gchar *packet, gsize packet_size,
                             GError **error)
{
    MilterHandlerPrivate *priv;
    gboolean success;

    priv = MILTER_HANDLER_GET_PRIVATE(handler);

    if (!priv->writer)
        return TRUE;

    success = milter_writer_write(priv->writer, packet, packet_size,
                                  NULL, error);

    return success;
}

void
milter_handler_set_writer (MilterHandler *handler,
                           MilterWriter *writer)
{
    MilterHandlerPrivate *priv;

    priv = MILTER_HANDLER_GET_PRIVATE(handler);

    if (priv->writer)
        g_object_unref(priv->writer);

    priv->writer = writer;
    if (priv->writer)
        g_object_ref(priv->writer);
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
