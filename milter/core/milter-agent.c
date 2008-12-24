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

#include <string.h>
#include <stdlib.h>

#include "milter-agent.h"
#include "milter-marshalers.h"
#include "milter-enum-types.h"
#include "milter-utils.h"
#include "milter-logger.h"

#define MILTER_AGENT_GET_PRIVATE(obj)                          \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj),                        \
                                 MILTER_TYPE_AGENT,            \
                                 MilterAgentPrivate))

typedef struct _MilterAgentPrivate	MilterAgentPrivate;
struct _MilterAgentPrivate
{
    MilterEncoder *encoder;
    MilterDecoder *decoder;
    MilterReader *reader;
    MilterWriter *writer;
};

enum
{
    PROP_0,
    PROP_READER
};

MILTER_IMPLEMENT_ERROR_EMITTABLE(error_emittable_init);
MILTER_IMPLEMENT_FINISHED_EMITTABLE(finished_emittable_init);
G_DEFINE_ABSTRACT_TYPE_WITH_CODE(MilterAgent, milter_agent, G_TYPE_OBJECT,
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
milter_agent_class_init (MilterAgentClass *klass)
{
    GObjectClass *gobject_class;
    GParamSpec *spec;

    gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->constructor  = constructor;
    gobject_class->dispose      = dispose;
    gobject_class->set_property = set_property;
    gobject_class->get_property = get_property;

    klass->decoder_new = NULL;
    klass->encoder_new = NULL;

    spec = g_param_spec_object("milter-reader",
                               "milter reader",
                               "A MilterReader object",
                               MILTER_TYPE_READER,
                               G_PARAM_READWRITE);
    g_object_class_install_property(gobject_class, PROP_READER, spec);

    g_type_class_add_private(gobject_class, sizeof(MilterAgentPrivate));
}

static GObject *
constructor (GType type, guint n_props, GObjectConstructParam *props)
{
    GObject *object;
    MilterAgent *agent;
    GObjectClass *klass;
    MilterAgentClass *agent_class;
    MilterAgentPrivate *priv;

    klass = G_OBJECT_CLASS(milter_agent_parent_class);
    object = klass->constructor(type, n_props, props);

    priv = MILTER_AGENT_GET_PRIVATE(object);

    agent = MILTER_AGENT(object);
    agent_class = MILTER_AGENT_GET_CLASS(object);
    if (agent_class->decoder_new)
        priv->decoder = agent_class->decoder_new(agent);
    if (agent_class->encoder_new)
        priv->encoder = agent_class->encoder_new(agent);

    return object;
}

static void
milter_agent_init (MilterAgent *agent)
{
    MilterAgentPrivate *priv;

    priv = MILTER_AGENT_GET_PRIVATE(agent);
    priv->encoder = NULL;
    priv->decoder = NULL;
    priv->writer = NULL;
    priv->reader = NULL;
}

static void
dispose (GObject *object)
{
    MilterAgentPrivate *priv;

    priv = MILTER_AGENT_GET_PRIVATE(object);

    if (priv->encoder) {
        g_object_unref(priv->encoder);
        priv->encoder = NULL;
    }

    if (priv->decoder) {
        g_object_unref(priv->decoder);
        priv->decoder = NULL;
    }

    if (priv->writer) {
        g_object_unref(priv->writer);
        priv->writer = NULL;
    }

    if (priv->reader) {
        milter_agent_set_reader(MILTER_AGENT(object), NULL);
    }

    G_OBJECT_CLASS(milter_agent_parent_class)->dispose(object);
}

static void
set_property (GObject      *object,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
    MilterAgentPrivate *priv;

    priv = MILTER_AGENT_GET_PRIVATE(object);
    switch (prop_id) {
      case PROP_READER:
        milter_agent_set_reader(MILTER_AGENT(object), g_value_get_object(value));
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
    MilterAgentPrivate *priv;

    priv = MILTER_AGENT_GET_PRIVATE(object);
    switch (prop_id) {
      case PROP_READER:
        g_value_set_object(value, priv->reader);
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static void
cb_reader_flow (MilterReader *reader,
                const gchar *data, gsize data_size,
                gpointer user_data)
{
    MilterAgentPrivate *priv;
    GError *decoder_error = NULL;

    priv = MILTER_AGENT_GET_PRIVATE(user_data);

    milter_decoder_decode(priv->decoder, data, data_size, &decoder_error);

    if (decoder_error) {
        GError *error = NULL;

        milter_utils_set_error_with_sub_error(&error,
                                              MILTER_AGENT_ERROR,
                                              MILTER_AGENT_ERROR_DECODE_ERROR,
                                              decoder_error,
                                              "Decode error");
        milter_error("%s", error->message);
        milter_error_emittable_emit(MILTER_ERROR_EMITTABLE(user_data),
                                    error);
        g_error_free(error);
    }
}

static void
cb_reader_error (MilterReader *reader,
                 GError *reader_error,
                 gpointer user_data)
{
    GError *error = NULL;

    milter_utils_set_error_with_sub_error(&error,
                                          MILTER_AGENT_ERROR,
                                          MILTER_AGENT_ERROR_IO_ERROR,
                                          g_error_copy(reader_error),
                                          "I/O error");
    milter_error("%s", error->message);
    milter_error_emittable_emit(MILTER_ERROR_EMITTABLE(user_data),
                                error);
    g_error_free(error);
}

static void
cb_reader_finished (MilterFinishedEmittable *emittable, gpointer user_data)
{
    milter_finished_emittable_emit(MILTER_FINISHED_EMITTABLE(user_data));
}

void
milter_agent_set_reader (MilterAgent *agent, MilterReader *reader)
{
    MilterAgentPrivate *priv;

    priv = MILTER_AGENT_GET_PRIVATE(agent);

#define DISCONNECT(name)                                                \
    g_signal_handlers_disconnect_by_func(priv->reader,                  \
                                         G_CALLBACK(cb_reader_ ## name),\
                                         agent)

#define CONNECT(name)                                                    \
    g_signal_connect(priv->reader, #name, G_CALLBACK(cb_reader_ ## name),\
                     agent)

    if (priv->reader) {
        DISCONNECT(flow);
        DISCONNECT(error);
        DISCONNECT(finished);
        g_object_unref(priv->reader);
    }

    priv->reader = reader;
    if (priv->reader) {
        CONNECT(flow);
        CONNECT(error);
        CONNECT(finished);
        g_object_ref(priv->reader);
    }

#undef DISCONNECT
#undef CONNECT
}

GQuark
milter_agent_error_quark (void)
{
    return g_quark_from_static_string("milter-agent-error-quark");
}

gboolean
milter_agent_write_packet (MilterAgent *agent,
                             const gchar *packet, gsize packet_size,
                             GError **error)
{
    MilterAgentPrivate *priv;
    gboolean success;

    priv = MILTER_AGENT_GET_PRIVATE(agent);

    if (!priv->writer)
        return TRUE;

    success = milter_writer_write(priv->writer, packet, packet_size,
                                  NULL, error);

    return success;
}

void
milter_agent_set_writer (MilterAgent *agent, MilterWriter *writer)
{
    MilterAgentPrivate *priv;

    priv = MILTER_AGENT_GET_PRIVATE(agent);

    if (priv->writer)
        g_object_unref(priv->writer);

    priv->writer = writer;
    if (priv->writer)
        g_object_ref(priv->writer);
}

MilterEncoder *
milter_agent_get_encoder (MilterAgent *agent)
{
    return MILTER_AGENT_GET_PRIVATE(agent)->encoder;
}

MilterDecoder *
milter_agent_get_decoder (MilterAgent *agent)
{
    return MILTER_AGENT_GET_PRIVATE(agent)->decoder;
}

void
milter_agent_start (MilterAgent *agent)
{
    MilterAgentPrivate *priv;

    priv = MILTER_AGENT_GET_PRIVATE(agent);

    if (priv->reader)
        milter_reader_start(priv->reader);
}

void
milter_agent_shutdown (MilterAgent *agent)
{
    MilterAgentPrivate *priv;

    priv = MILTER_AGENT_GET_PRIVATE(agent);

    if (priv->reader)
        milter_reader_shutdown(priv->reader);
    else
        milter_finished_emittable_emit(MILTER_FINISHED_EMITTABLE(agent));
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
