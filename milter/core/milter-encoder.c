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
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>

#include <glib.h>

#include "milter-encoder.h"
#include "milter-logger.h"
#include "milter-enum-types.h"
#include "milter-marshalers.h"
#include "milter-macros-requests.h"
#include "milter-utils.h"

#define MILTER_ENCODER_GET_PRIVATE(obj)                 \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj),                 \
                                 MILTER_TYPE_ENCODER,   \
                                 MilterEncoderPrivate))

typedef struct _MilterEncoderPrivate	MilterEncoderPrivate;
struct _MilterEncoderPrivate
{
    GString *buffer;
    guint tag;
};

enum
{
    PROP_0,
    PROP_TAG
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
    GParamSpec *spec;

    gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->dispose      = dispose;
    gobject_class->set_property = set_property;
    gobject_class->get_property = get_property;

    spec = g_param_spec_uint("tag",
                             "Tag",
                             "The tag of the encoder",
                             0, G_MAXUINT, 0,
                             G_PARAM_READABLE);
    g_object_class_install_property(gobject_class, PROP_TAG, spec);

    g_type_class_add_private(gobject_class, sizeof(MilterEncoderPrivate));
}

static void
milter_encoder_init (MilterEncoder *encoder)
{
    MilterEncoderPrivate *priv;

    priv = MILTER_ENCODER_GET_PRIVATE(encoder);
    priv->buffer = g_string_new(NULL);
    priv->tag = 0;
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
    switch (prop_id) {
    case PROP_TAG:
        milter_encoder_set_tag(MILTER_ENCODER(object), g_value_get_uint(value));
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
    MilterEncoderPrivate *priv;

    priv = MILTER_ENCODER_GET_PRIVATE(object);
    switch (prop_id) {
    case PROP_TAG:
        g_value_set_uint(value, priv->tag);
        break;
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

GString *
milter_encoder_get_buffer (MilterEncoder *encoder)
{
    return MILTER_ENCODER_GET_PRIVATE(encoder)->buffer;
}

void
milter_encoder_clear_buffer (MilterEncoder *encoder)
{
    MilterEncoderPrivate *priv;

    priv = MILTER_ENCODER_GET_PRIVATE(encoder);
    g_string_truncate(priv->buffer, 0);
}

void
milter_encoder_pack (MilterEncoder *encoder, const gchar **packet,
                     gsize *packet_size)
{
    MilterEncoderPrivate *priv;
    guint32 content_size;
    gchar content_string[sizeof(guint32)];

    priv = MILTER_ENCODER_GET_PRIVATE(encoder);
    content_size = g_htonl(priv->buffer->len);
    memcpy(content_string, &content_size, sizeof(content_size));
    g_string_prepend_len(priv->buffer, content_string, sizeof(content_size));

    *packet = priv->buffer->str;
    *packet_size = priv->buffer->len;
}

void
milter_encoder_encode_negotiate (MilterEncoder *encoder, MilterOption *option)
{
    MilterEncoderPrivate *priv;

    priv = MILTER_ENCODER_GET_PRIVATE(encoder);
    g_string_append_c(priv->buffer, MILTER_COMMAND_NEGOTIATE);
    if (option) {
        guint32 version, action, step;
        gchar version_string[sizeof(guint32)];
        gchar action_string[sizeof(guint32)];
        gchar step_string[sizeof(guint32)];

        version = g_htonl(milter_option_get_version(option));
        action = g_htonl(milter_option_get_action(option));
        step = g_htonl(milter_option_get_step(option));

        memcpy(version_string, &version, sizeof(version));
        memcpy(action_string, &action, sizeof(action));
        memcpy(step_string, &step, sizeof(step));

        g_string_append_len(priv->buffer, version_string, sizeof(version));
        g_string_append_len(priv->buffer, action_string, sizeof(action));
        g_string_append_len(priv->buffer, step_string, sizeof(step));
    }
}

guint
milter_encoder_get_tag (MilterEncoder *encoder)
{
    return MILTER_ENCODER_GET_PRIVATE(encoder)->tag;
}

void
milter_encoder_set_tag (MilterEncoder *encoder, guint tag)
{
    MILTER_ENCODER_GET_PRIVATE(encoder)->tag = tag;
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
