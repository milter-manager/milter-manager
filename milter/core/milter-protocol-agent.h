/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Copyright (C) 2008-2022  Sutou Kouhei <kou@clear-code.com>
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

#ifndef __MILTER_PROTOCOL_AGENT_H__
#define __MILTER_PROTOCOL_AGENT_H__

#include <glib-object.h>

#include <milter/core/milter-agent.h>
#include <milter/core/milter-macros-requests.h>

G_BEGIN_DECLS

#define MILTER_TYPE_PROTOCOL_AGENT            (milter_protocol_agent_get_type())
G_DECLARE_DERIVABLE_TYPE(MilterProtocolAgent,
                         milter_protocol_agent,
                         MILTER,
                         PROTOCOL_AGENT,
                         MilterAgent)
struct _MilterProtocolAgentClass
{
    MilterAgentClass parent_class;
};

void                 milter_protocol_agent_set_macro_context
                                                    (MilterProtocolAgent *agent,
                                                     MilterCommand  macro_context);
void                 milter_protocol_agent_set_macros
                                                    (MilterProtocolAgent *agent,
                                                     MilterCommand  macro_context,
                                                     const gchar   *macro_name,
                                                     ...) G_GNUC_NULL_TERMINATED;
void                 milter_protocol_agent_set_macros_hash_table
                                                    (MilterProtocolAgent *agent,
                                                     MilterCommand  macro_context,
                                                     GHashTable    *macros);
void                 milter_protocol_agent_set_macro(MilterProtocolAgent *agent,
                                                     MilterCommand  macro_context,
                                                     const gchar   *macro_name,
                                                     const gchar   *macro_value);
GHashTable          *milter_protocol_agent_get_macros
                                                    (MilterProtocolAgent *agent);
GHashTable          *milter_protocol_agent_get_available_macros
                                                    (MilterProtocolAgent *agent);
const gchar         *milter_protocol_agent_get_macro(MilterProtocolAgent *agent,
                                                     const gchar   *name);
void                 milter_protocol_agent_clear_macros
                                                    (MilterProtocolAgent *agent,
                                                     MilterCommand  macro_context);
void                 milter_protocol_agent_clear_message_related_macros
                                                    (MilterProtocolAgent *agent);

void                 milter_protocol_agent_set_macros_requests
                                                    (MilterProtocolAgent *agent,
                                                     MilterMacrosRequests *requests);
MilterMacrosRequests *milter_protocol_agent_get_macros_requests
                                                    (MilterProtocolAgent *agent);

G_END_DECLS

#endif /* __MILTER_PROTOCOL_AGENT_H__ */

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
