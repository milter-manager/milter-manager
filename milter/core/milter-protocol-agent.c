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

#include "milter-protocol-agent.h"
#include "milter-marshalers.h"
#include "milter-enum-types.h"
#include "milter-utils.h"
#include "milter-logger.h"

#define MILTER_PROTOCOL_AGENT_GET_PRIVATE(obj)                          \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj),                                 \
                                 MILTER_TYPE_PROTOCOL_AGENT,            \
                                 MilterProtocolAgentPrivate))

typedef struct _MilterProtocolAgentPrivate	MilterProtocolAgentPrivate;
struct _MilterProtocolAgentPrivate
{
    GHashTable *macros;
    GHashTable *available_macros;
    MilterCommand macro_context;
    MilterMacrosRequests *macros_requests;
};

static MilterCommand macro_search_order[] = {
    MILTER_COMMAND_END_OF_MESSAGE,
    MILTER_COMMAND_BODY,
    MILTER_COMMAND_END_OF_HEADER,
    MILTER_COMMAND_HEADER,
    MILTER_COMMAND_DATA,
    MILTER_COMMAND_ENVELOPE_RECIPIENT,
    MILTER_COMMAND_ENVELOPE_FROM,
    MILTER_COMMAND_HELO,
    MILTER_COMMAND_CONNECT,
    0,
};

G_DEFINE_ABSTRACT_TYPE(MilterProtocolAgent, milter_protocol_agent,
                       MILTER_TYPE_AGENT)


static void dispose        (GObject         *object);

static void
milter_protocol_agent_class_init (MilterProtocolAgentClass *klass)
{
    GObjectClass *gobject_class;
    MilterAgentClass *agent_class;

    gobject_class = G_OBJECT_CLASS(klass);
    agent_class = MILTER_AGENT_CLASS(klass);

    gobject_class->dispose      = dispose;

    g_type_class_add_private(gobject_class, sizeof(MilterProtocolAgentPrivate));
}

static void
milter_protocol_agent_init (MilterProtocolAgent *agent)
{
    MilterProtocolAgentPrivate *priv;

    priv = MILTER_PROTOCOL_AGENT_GET_PRIVATE(agent);
    priv->macros = g_hash_table_new_full(g_direct_hash, g_direct_equal,
                                         NULL,
                                         (GDestroyNotify)g_hash_table_unref);
    priv->available_macros = NULL;
    priv->macro_context = MILTER_COMMAND_UNKNOWN;
    priv->macros_requests = NULL;
}

static void
clear_available_macros (MilterProtocolAgentPrivate *priv)
{
    if (priv->available_macros) {
        g_hash_table_unref(priv->available_macros);
        priv->available_macros = NULL;
    }
}

static void
dispose (GObject *object)
{
    MilterProtocolAgentPrivate *priv;

    priv = MILTER_PROTOCOL_AGENT_GET_PRIVATE(object);

    if (priv->macros) {
        g_hash_table_unref(priv->macros);
        priv->macros = NULL;
    }

    clear_available_macros(priv);

    if (priv->macros_requests) {
        g_object_unref(priv->macros_requests);
        priv->macros_requests = NULL;
    }

    G_OBJECT_CLASS(milter_protocol_agent_parent_class)->dispose(object);
}

const gchar *
milter_protocol_agent_get_macro (MilterProtocolAgent *agent, const gchar *name)
{
    GHashTable *available_macros;
    gchar *value;

    if (!name)
        return NULL;

    available_macros = milter_protocol_agent_get_available_macros(agent);
    value = g_hash_table_lookup(available_macros, name);
    if (!value) {
        gchar *unbracket_name = NULL;

        if (g_str_has_prefix(name, "{") && g_str_has_suffix(name, "}")) {
            unbracket_name = g_strndup(name + 1, strlen(name) - 2);
            value = g_hash_table_lookup(available_macros, unbracket_name);
            g_free(unbracket_name);
        }
    }

    return value;
}

GHashTable *
milter_protocol_agent_get_available_macros (MilterProtocolAgent *agent)
{
    MilterProtocolAgentPrivate *priv;
    gboolean found_current_context = FALSE;
    gint i;

    priv = MILTER_PROTOCOL_AGENT_GET_PRIVATE(agent);
    if (priv->available_macros)
        return priv->available_macros;

    priv->available_macros = g_hash_table_new_full(g_str_hash, g_str_equal,
                                                   g_free, g_free);
    for (i = 0; macro_search_order[i] != 0; i++) {
        GHashTable *macros;
        MilterCommand context;

        context = macro_search_order[i];
        if (!found_current_context) {
            if (context != priv->macro_context)
                continue;
            found_current_context = TRUE;
        }

        macros = g_hash_table_lookup(priv->macros, GINT_TO_POINTER(context));
        if (!macros)
            continue;

        milter_utils_merge_hash_string_string(priv->available_macros, macros);
    }

    return priv->available_macros;

}

GHashTable *
milter_protocol_agent_get_macros (MilterProtocolAgent *agent)
{
    MilterProtocolAgentPrivate *priv;

    priv = MILTER_PROTOCOL_AGENT_GET_PRIVATE(agent);
    return g_hash_table_lookup(priv->macros,
                               GINT_TO_POINTER(priv->macro_context));
}

void
milter_protocol_agent_clear_macros (MilterProtocolAgent *agent,
                                    MilterCommand macro_context)
{
    MilterProtocolAgentPrivate *priv;

    priv = MILTER_PROTOCOL_AGENT_GET_PRIVATE(agent);
    g_hash_table_remove(priv->macros, GINT_TO_POINTER(macro_context));
    clear_available_macros(priv);
}

static void
update_macro (gpointer key, gpointer value, gpointer user_data)
{
    GHashTable *macros = user_data;

    g_hash_table_replace(macros, g_strdup(key), g_strdup(value));
}

void
milter_protocol_agent_set_macro_context (MilterProtocolAgent *agent,
                                         MilterCommand macro_context)
{
    MilterProtocolAgentPrivate *priv;

    priv = MILTER_PROTOCOL_AGENT_GET_PRIVATE(agent);
    priv->macro_context = macro_context;
    clear_available_macros(priv);
}

static void
milter_protocol_agent_set_macros_valist (MilterProtocolAgent *agent,
                                         MilterCommand macro_context,
                                         const gchar *macro_name,
                                         va_list var_args)
{
    const gchar *name;

    name = macro_name;
    while (name) {
        const gchar *value;
        value = va_arg(var_args, gchar*);
        if (!value)
            break;
        milter_protocol_agent_set_macro(agent, macro_context, name, value);
        name = va_arg(var_args, gchar*);
    }
}

void
milter_protocol_agent_set_macros (MilterProtocolAgent *agent,
                                  MilterCommand macro_context,
                                  const gchar *macro_name,
                                  ...)
{
    va_list var_args;

    va_start(var_args, macro_name);
    milter_protocol_agent_set_macros_valist(agent, macro_context, macro_name,
                                            var_args);
    va_end(var_args);
}

void
milter_protocol_agent_set_macros_hash_table (MilterProtocolAgent *agent,
                                             MilterCommand macro_context,
                                             GHashTable *macros)
{
    MilterProtocolAgentPrivate *priv;
    GHashTable *copied_macros;

    priv = MILTER_PROTOCOL_AGENT_GET_PRIVATE(agent);
    copied_macros = g_hash_table_new_full(g_str_hash, g_str_equal,
                                          g_free, g_free);
    g_hash_table_insert(priv->macros,
                        GINT_TO_POINTER(macro_context),
                        copied_macros);
    g_hash_table_foreach(macros, update_macro, copied_macros);
}

void
milter_protocol_agent_set_macro (MilterProtocolAgent *agent,
                                 MilterCommand  macro_context,
                                 const gchar   *macro_name,
                                 const gchar   *macro_value)
{
    MilterProtocolAgentPrivate *priv;
    GHashTable *macros;

    priv = MILTER_PROTOCOL_AGENT_GET_PRIVATE(agent);

    macros = g_hash_table_lookup(priv->macros, GINT_TO_POINTER(macro_context));
    if (!macros) {
        macros = g_hash_table_new_full(g_str_hash, g_str_equal,
                                       g_free, g_free);
        g_hash_table_insert(priv->macros,
                            GINT_TO_POINTER(macro_context),
                            macros);
    }

    g_hash_table_replace(macros,
                         g_strdup(macro_name), g_strdup(macro_value));
    clear_available_macros(priv);
}

void
milter_protocol_agent_set_macros_requests (MilterProtocolAgent *agent,
                                           MilterMacrosRequests *macros_requests)
{
    MilterProtocolAgentPrivate *priv;

    priv = MILTER_PROTOCOL_AGENT_GET_PRIVATE(agent);
    if (priv->macros_requests) {
        g_object_unref(priv->macros_requests);
    }

    priv->macros_requests = macros_requests;
    if (macros_requests)
        g_object_ref(priv->macros_requests);
}

MilterMacrosRequests *
milter_protocol_agent_get_macros_requests(MilterProtocolAgent *agent)
{
    return MILTER_PROTOCOL_AGENT_GET_PRIVATE(agent)->macros_requests;
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
