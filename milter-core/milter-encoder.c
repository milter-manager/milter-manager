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
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>

#include <glib.h>

#include "milter-encoder.h"
#include "milter-enum-types.h"
#include "milter-marshalers.h"

#define MILTER_ENCODER_GET_PRIVATE(obj)                 \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj),                 \
                                 MILTER_TYPE_ENCODER,   \
                                 MilterEncoderPrivate))

typedef struct _MilterEncoderPrivate	MilterEncoderPrivate;
struct _MilterEncoderPrivate
{
    GString *buffer;
};

G_DEFINE_TYPE(MilterEncoder, milter_encoder, G_TYPE_OBJECT);

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
milter_encoder_class_init (MilterEncoderClass *klass)
{
    GObjectClass *gobject_class;

    gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->dispose      = dispose;
    gobject_class->set_property = set_property;
    gobject_class->get_property = get_property;

    g_type_class_add_private(gobject_class, sizeof(MilterEncoderPrivate));
}

static void
milter_encoder_init (MilterEncoder *encoder)
{
    MilterEncoderPrivate *priv;

    priv = MILTER_ENCODER_GET_PRIVATE(encoder);
    priv->buffer = g_string_new(NULL);
}

static void
dispose (GObject *object)
{
    MilterEncoderPrivate *priv;

    priv = MILTER_ENCODER_GET_PRIVATE(object);
    if (priv->buffer) {
        g_string_free(priv->buffer, TRUE);
        priv->buffer = NULL;
    }

    G_OBJECT_CLASS(milter_encoder_parent_class)->dispose(object);
}

static void
set_property (GObject      *object,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
    MilterEncoderPrivate *priv;

    priv = MILTER_ENCODER_GET_PRIVATE(object);
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
    MilterEncoderPrivate *priv;

    priv = MILTER_ENCODER_GET_PRIVATE(object);
    switch (prop_id) {
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

MilterEncoder *
milter_encoder_new (void)
{
    return g_object_new(MILTER_TYPE_ENCODER, NULL);
}

static void
pack (GString *output)
{
    guint32 content_size;
    gchar content_string[sizeof(guint32)];

    content_size = htonl(output->len);
    memcpy(content_string, &content_size, sizeof(content_size));
    g_string_prepend_len(output, content_string, sizeof(content_size));
}

void
milter_encoder_encode_option_negotiation (MilterEncoder *encoder,
                                          gchar **packet, gsize *packet_size,
                                          MilterOption *option)
{
    MilterEncoderPrivate *priv;
    guint32 version, action, step;
    gchar version_string[sizeof(guint32)];
    gchar action_string[sizeof(guint32)];
    gchar step_string[sizeof(guint32)];

    g_return_if_fail (option != NULL);

    priv = MILTER_ENCODER_GET_PRIVATE(encoder);
    g_string_truncate(priv->buffer, 0);

    version = ntohl(milter_option_get_version(option));
    action = ntohl(milter_option_get_action(option));
    step = ntohl(milter_option_get_step(option));

    memcpy(version_string, &version, sizeof(version));
    memcpy(action_string, &action, sizeof(action));
    memcpy(step_string, &step, sizeof(step));

    g_string_append_c(priv->buffer, MILTER_COMMAND_OPTION_NEGOTIATION);
    g_string_append_len(priv->buffer, version_string, sizeof(version));
    g_string_append_len(priv->buffer, action_string, sizeof(action));
    g_string_append_len(priv->buffer, step_string, sizeof(step));

    pack(priv->buffer);

    *packet = g_memdup(priv->buffer->str, priv->buffer->len);
    *packet_size = priv->buffer->len;
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
