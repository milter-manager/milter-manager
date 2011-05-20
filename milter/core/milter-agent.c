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
    MilterEventLoop *event_loop;
    MilterEncoder *encoder;
    MilterDecoder *decoder;
    MilterReader *reader;
    MilterWriter *writer;
    gboolean finished;
    guint tag;
    GTimer *timer;
    gboolean shutting_down;
};

enum
{
    PROP_0,
    PROP_READER,
    PROP_DECODER,
    PROP_TAG,
    PROP_ELAPSED,
    PROP_EVENT_LOOP
};

enum
{
    FLUSHED,
    LAST_SIGNAL
};

static gint signals[LAST_SIGNAL] = {0};

static GStaticMutex auto_tag_mutex = G_STATIC_MUTEX_INIT;
static guint auto_tag = 0;

static void         finished           (MilterFinishedEmittable *emittable);

MILTER_IMPLEMENT_ERROR_EMITTABLE(error_emittable_init);
MILTER_IMPLEMENT_FINISHED_EMITTABLE_WITH_CODE(finished_emittable_init,
                                              iface->finished = finished)
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

static gboolean flush      (MilterAgent     *agent,
                            GError         **error);

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
    klass->flush = flush;

    spec = g_param_spec_object("reader",
                               "Reader",
                               "The reader of the agent",
                               MILTER_TYPE_READER,
                               G_PARAM_READWRITE);
    g_object_class_install_property(gobject_class, PROP_READER, spec);

    spec = g_param_spec_object("decoder",
                               "Decoder",
                               "The decoder of the agent",
                               MILTER_TYPE_DECODER,
                               G_PARAM_READABLE);
    g_object_class_install_property(gobject_class, PROP_DECODER, spec);

    spec = g_param_spec_uint("tag",
                             "Tag",
                             "The tag of the agent",
                             0, G_MAXUINT, 0,
                             G_PARAM_READWRITE);
    g_object_class_install_property(gobject_class, PROP_TAG, spec);

    spec = g_param_spec_double("elapsed",
                               "Elapsed",
                               "The elapsed time of the agent",
                               0.0, G_MAXDOUBLE, 0.0,
                               G_PARAM_READABLE);
    g_object_class_install_property(gobject_class, PROP_ELAPSED, spec);

    spec = g_param_spec_object("event-loop",
                               "Event Loop",
                               "The event loop of the agent",
                               MILTER_TYPE_EVENT_LOOP,
                               G_PARAM_READWRITE);
    g_object_class_install_property(gobject_class, PROP_EVENT_LOOP, spec);

    /**
     * MilterAgent::flushed:
     * @agent: the agent that received the signal.
     *
     * This signal is emitted when writer of the agent
     * flushes requested data.
     */
    signals[FLUSHED] =
        g_signal_new("flushed",
                     MILTER_TYPE_AGENT,
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterAgentClass, flushed),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__VOID,
                     G_TYPE_NONE, 0);

    g_type_class_add_private(gobject_class, sizeof(MilterAgentPrivate));
}

static void
apply_tag (MilterAgentPrivate *priv)
{
    if (priv->encoder)
        milter_encoder_set_tag(priv->encoder, priv->tag);
    if (priv->decoder)
        milter_decoder_set_tag(priv->decoder, priv->tag);
    if (priv->writer)
        milter_writer_set_tag(priv->writer, priv->tag);
    if (priv->reader)
        milter_reader_set_tag(priv->reader, priv->tag);
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
    if (priv->tag == 0) {
        g_static_mutex_lock(&auto_tag_mutex);
        priv->tag = auto_tag++;
        if (priv->tag == 0)
            priv->tag = auto_tag++;
        g_static_mutex_unlock(&auto_tag_mutex);
    }
    apply_tag(priv);

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
    priv->tag = 0;
    priv->finished = FALSE;
    priv->timer = NULL;
    priv->event_loop = NULL;
    priv->shutting_down = FALSE;
}

static void
dispose (GObject *object)
{
    MilterAgent *agent;
    MilterAgentPrivate *priv;

    agent = MILTER_AGENT(object);
    priv = MILTER_AGENT_GET_PRIVATE(agent);

    if (priv->timer) {
        g_timer_destroy(priv->timer);
        priv->timer = NULL;
    }

    milter_agent_set_reader(agent, NULL);
    milter_agent_set_writer(agent, NULL);

    if (priv->decoder) {
        g_object_unref(priv->decoder);
        priv->decoder = NULL;
    }

    if (priv->encoder) {
        g_object_unref(priv->encoder);
        priv->encoder = NULL;
    }

    if (priv->event_loop) {
        g_object_unref(priv->event_loop);
        priv->event_loop = NULL;
    }

    G_OBJECT_CLASS(milter_agent_parent_class)->dispose(object);
}

static void
set_property (GObject      *object,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
    MilterAgent *agent;

    agent = MILTER_AGENT(object);
    switch (prop_id) {
    case PROP_READER:
        milter_agent_set_reader(agent, g_value_get_object(value));
        break;
    case PROP_TAG:
        milter_agent_set_tag(agent, g_value_get_uint(value));
        break;
    case PROP_EVENT_LOOP:
        milter_agent_set_event_loop(agent, g_value_get_object(value));
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
    MilterAgent *agent;
    MilterAgentPrivate *priv;

    agent = MILTER_AGENT(object);
    priv = MILTER_AGENT_GET_PRIVATE(agent);
    switch (prop_id) {
    case PROP_READER:
        g_value_set_object(value, priv->reader);
        break;
    case PROP_DECODER:
        g_value_set_object(value, priv->decoder);
        break;
    case PROP_TAG:
        g_value_set_uint(value, priv->tag);
        break;
    case PROP_ELAPSED:
        g_value_set_double(value, milter_agent_get_elapsed(agent));
        break;
    case PROP_EVENT_LOOP:
        g_value_set_object(value, milter_agent_get_event_loop(agent));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static gboolean
flush (MilterAgent *agent, GError **error)
{
    MilterAgentPrivate *priv;
    gboolean success;

    priv = MILTER_AGENT_GET_PRIVATE(agent);

    if (!priv->writer)
        return TRUE;

    success = milter_writer_flush(priv->writer, error);

    return success;
}

static void
finished (MilterFinishedEmittable *emittable)
{
    MilterAgent *agent;
    MilterAgentPrivate *priv;

    agent = MILTER_AGENT(emittable);
    priv = MILTER_AGENT_GET_PRIVATE(agent);

    if (priv->timer)
        g_timer_stop(priv->timer);

    priv->finished = TRUE;
    if (priv->reader)
        milter_reader_shutdown(priv->reader);
    if (priv->writer)
        milter_writer_shutdown(priv->writer);
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
        milter_error("[%u] [agent][error][decode] %s",
                     priv->tag, error->message);
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
    MilterAgent *agent = user_data;
    MilterAgentPrivate *priv;

    priv = MILTER_AGENT_GET_PRIVATE(agent);
    milter_utils_set_error_with_sub_error(&error,
                                          MILTER_AGENT_ERROR,
                                          MILTER_AGENT_ERROR_IO_ERROR,
                                          g_error_copy(reader_error),
                                          "Input error");
    milter_error("[%u] [agent][error][reader] %s", priv->tag, error->message);
    milter_error_emittable_emit(MILTER_ERROR_EMITTABLE(user_data),
                                error);
    g_error_free(error);
}

static void
cb_reader_finished (MilterFinishedEmittable *emittable, gpointer user_data)
{
    MilterAgent *agent = user_data;
    MilterAgentPrivate *priv;

    priv = MILTER_AGENT_GET_PRIVATE(agent);
    milter_agent_set_reader(agent, NULL);
    if (!priv->finished)
        milter_finished_emittable_emit(MILTER_FINISHED_EMITTABLE(agent));
}

void
milter_agent_set_reader (MilterAgent *agent, MilterReader *reader)
{
    MilterAgentPrivate *priv;

    priv = MILTER_AGENT_GET_PRIVATE(agent);

    if (priv->reader) {
#define DISCONNECT(name)                                                \
        g_signal_handlers_disconnect_by_func(priv->reader,              \
                                             G_CALLBACK(cb_reader_ ## name), \
                                             agent)

        DISCONNECT(flow);
        DISCONNECT(error);
        DISCONNECT(finished);
#undef DISCONNECT

        g_object_unref(priv->reader);
    }

    priv->reader = reader;
    if (priv->reader) {
        g_object_ref(priv->reader);

#define CONNECT(name)                                                   \
        g_signal_connect(priv->reader, #name, G_CALLBACK(cb_reader_ ## name), \
                         agent)
        CONNECT(flow);
        CONNECT(error);
        CONNECT(finished);
#undef CONNECT

        milter_reader_set_tag(priv->reader, priv->tag);
    }
}

GQuark
milter_agent_error_quark (void)
{
    return g_quark_from_static_string("milter-agent-error-quark");
}

static void
cb_writer_flushed (MilterWriter *writer, gpointer user_data)
{
    MilterAgent *agent = user_data;

    if (milter_need_trace_log()) {
        MilterAgentPrivate *priv;

        priv = MILTER_AGENT_GET_PRIVATE(agent);
        milter_trace("[%u] [agent][writer] flushed", priv->tag);
    }
    g_signal_emit(agent, signals[FLUSHED], 0);
}

static void
cb_writer_error (MilterWriter *writer,
                 GError *writer_error,
                 gpointer user_data)
{
    GError *error = NULL;
    MilterAgent *agent = user_data;
    MilterAgentPrivate *priv;

    priv = MILTER_AGENT_GET_PRIVATE(agent);
    milter_utils_set_error_with_sub_error(&error,
                                          MILTER_AGENT_ERROR,
                                          MILTER_AGENT_ERROR_IO_ERROR,
                                          g_error_copy(writer_error),
                                          "Output error");
    milter_error("[%u] [agent][error][writer] %s",
                 priv->tag, error->message);
    milter_error_emittable_emit(MILTER_ERROR_EMITTABLE(user_data),
                                error);
    g_error_free(error);
}

static void
cb_writer_finished (MilterFinishedEmittable *emittable, gpointer user_data)
{
    MilterAgent *agent = user_data;

    milter_agent_set_writer(agent, NULL);
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

    success = milter_writer_write(priv->writer, packet, packet_size, error);
    if (success) {
        success = milter_agent_flush(agent, error);
    }

    return success;
}

gboolean
milter_agent_flush (MilterAgent *agent, GError **error)
{
    MilterAgentClass *agent_class;

    agent_class = MILTER_AGENT_GET_CLASS(agent);
    return agent_class->flush(agent, error);
}

void
milter_agent_set_writer (MilterAgent *agent, MilterWriter *writer)
{
    MilterAgentPrivate *priv;

    priv = MILTER_AGENT_GET_PRIVATE(agent);

    if (priv->writer) {
        if (milter_writer_is_watching(priv->writer)) {
            GError *error = NULL;
            if (!milter_agent_flush(agent, &error)) {
                milter_error("[%u] [agent][error][set-writer][auto-flush] %s",
                             priv->tag, error->message);
                g_error_free(error);
            }
        }

#define DISCONNECT(name)                                                \
        g_signal_handlers_disconnect_by_func(priv->writer,              \
                                             G_CALLBACK(cb_writer_ ## name), \
                                             agent)
        DISCONNECT(flushed);
        DISCONNECT(error);
        DISCONNECT(finished);
#undef DISCONNECT

        g_object_unref(priv->writer);
    }

    priv->writer = writer;
    if (priv->writer) {
        g_object_ref(priv->writer);

#define CONNECT(name)                                                   \
        g_signal_connect(priv->writer, #name,                           \
                         G_CALLBACK(cb_writer_ ## name),                \
                         agent)
        CONNECT(flushed);
        CONNECT(error);
        CONNECT(finished);
#undef CONNECT

        milter_writer_set_tag(priv->writer, priv->tag);
    }
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

gboolean
milter_agent_start (MilterAgent *agent, GError **error)
{
    MilterAgentPrivate *priv;

    priv = MILTER_AGENT_GET_PRIVATE(agent);

    if (!priv->event_loop) {
        g_set_error(error,
                    MILTER_AGENT_ERROR,
                    MILTER_AGENT_ERROR_NO_EVENT_LOOP_ERROR,
                    "agent has no event loop");
        return FALSE;
    }

    if (priv->timer) {
        g_timer_start(priv->timer);
    } else {
        priv->timer = g_timer_new();
    }

    if (priv->reader)
        milter_reader_start(priv->reader, priv->event_loop);
    if (priv->writer)
        milter_writer_start(priv->writer, priv->event_loop);

    return TRUE;
}

void
milter_agent_shutdown (MilterAgent *agent)
{
    MilterAgentPrivate *priv;
    gboolean have_reader = FALSE;

    priv = MILTER_AGENT_GET_PRIVATE(agent);

    milter_trace("[%u] [agent][shutdown]", priv->tag);

    if (priv->shutting_down)
        return;

    priv->shutting_down = TRUE;

    if (priv->reader) {
        have_reader = TRUE;
        milter_trace("[%u] [agent][shutdown][reader]", priv->tag);
        milter_reader_shutdown(priv->reader);
    }

    if (priv->writer) {
        milter_trace("[%u] [agent][shutdown][writer]", priv->tag);
        milter_writer_shutdown(priv->writer);
    }

    if (!have_reader && !priv->finished) {
        milter_finished_emittable_emit(MILTER_FINISHED_EMITTABLE(agent));
    }
    priv->shutting_down = FALSE;
}

guint
milter_agent_get_tag (MilterAgent *agent)
{
    return MILTER_AGENT_GET_PRIVATE(agent)->tag;
}

void
milter_agent_set_tag (MilterAgent *agent, guint tag)
{
    MilterAgentPrivate *priv;

    priv = MILTER_AGENT_GET_PRIVATE(agent);
    priv->tag = tag;
    apply_tag(priv);
}

MilterEventLoop *
milter_agent_get_event_loop (MilterAgent *agent)
{
    return MILTER_AGENT_GET_PRIVATE(agent)->event_loop;
}

void
milter_agent_set_event_loop (MilterAgent *agent, MilterEventLoop *loop)
{
    MilterAgentPrivate *priv;

    priv = MILTER_AGENT_GET_PRIVATE(agent);
    if (loop == priv->event_loop)
        return;

    if (priv->event_loop)
        g_object_unref(priv->event_loop);
    priv->event_loop = loop;
    if (priv->event_loop)
        g_object_ref(priv->event_loop);
}

gdouble
milter_agent_get_elapsed (MilterAgent *agent)
{
    MilterAgentPrivate *priv;

    priv = MILTER_AGENT_GET_PRIVATE(agent);
    if (priv->timer) {
        return g_timer_elapsed(priv->timer, NULL);
    } else {
        return 0.0;
    }
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
