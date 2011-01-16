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

#ifndef __MILTER_AGENT_H__
#define __MILTER_AGENT_H__

#include <glib-object.h>

#include <milter/core/milter-protocol.h>
#include <milter/core/milter-option.h>
#include <milter/core/milter-writer.h>
#include <milter/core/milter-reader.h>
#include <milter/core/milter-encoder.h>
#include <milter/core/milter-decoder.h>
#include <milter/core/milter-macros-requests.h>
#include <milter/core/milter-finished-emittable.h>
#include <milter/core/milter-event-loop.h>

G_BEGIN_DECLS

#define MILTER_AGENT_ERROR           (milter_agent_error_quark())

#define MILTER_TYPE_AGENT            (milter_agent_get_type())
#define MILTER_AGENT(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), MILTER_TYPE_AGENT, MilterAgent))
#define MILTER_AGENT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), MILTER_TYPE_AGENT, MilterAgentClass))
#define MILTER_IS_AGENT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), MILTER_TYPE_AGENT))
#define MILTER_IS_AGENT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), MILTER_TYPE_AGENT))
#define MILTER_AGENT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), MILTER_TYPE_AGENT, MilterAgentClass))

typedef enum
{
    MILTER_AGENT_ERROR_IO_ERROR,
    MILTER_AGENT_ERROR_DECODE_ERROR,
    MILTER_AGENT_ERROR_NO_EVENT_LOOP_ERROR
} MilterAgentError;

typedef struct _MilterAgent         MilterAgent;
typedef struct _MilterAgentClass    MilterAgentClass;

struct _MilterAgent
{
    GObject object;
};

struct _MilterAgentClass
{
    GObjectClass parent_class;

    MilterDecoder *(*decoder_new)   (MilterAgent *agent);
    MilterEncoder *(*encoder_new)   (MilterAgent *agent);
    gboolean       (*flush)         (MilterAgent *agent,
                                     GError     **error);

    void           (*flushed)       (MilterAgent *agent);
};

GQuark               milter_agent_error_quark       (void);

GType                milter_agent_get_type          (void) G_GNUC_CONST;

MilterEncoder       *milter_agent_get_encoder       (MilterAgent *agent);
MilterDecoder       *milter_agent_get_decoder       (MilterAgent *agent);

void                 milter_agent_set_writer        (MilterAgent *agent,
                                                     MilterWriter *writer);
void                 milter_agent_set_reader        (MilterAgent *agent,
                                                     MilterReader *reader);
gboolean             milter_agent_write_packet      (MilterAgent *agent,
                                                     const char *packet,
                                                     gsize packet_size,
                                                     GError **error);
gboolean             milter_agent_flush             (MilterAgent *agent,
                                                     GError **error);

gboolean             milter_agent_start             (MilterAgent *agent,
                                                     GError     **error);
void                 milter_agent_shutdown          (MilterAgent *agent);

guint                milter_agent_get_tag           (MilterAgent *agent);
void                 milter_agent_set_tag           (MilterAgent *agent,
                                                     guint        tag);

MilterEventLoop     *milter_agent_get_event_loop    (MilterAgent *agent);
void                 milter_agent_set_event_loop    (MilterAgent *agent,
                                                     MilterEventLoop *loop);

gdouble              milter_agent_get_elapsed       (MilterAgent *agent);

G_END_DECLS

#endif /* __MILTER_AGENT_H__ */

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
