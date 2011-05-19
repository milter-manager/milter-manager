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

enum
{
    PROP_0,
    PROP_MACRO_CONTEXT
};

static MilterCommand macro_search_order[] = {
    MILTER_COMMAND_CONNECT,
    MILTER_COMMAND_HELO,
    MILTER_COMMAND_ENVELOPE_FROM,
    MILTER_COMMAND_ENVELOPE_RECIPIENT,
    MILTER_COMMAND_DATA,
    MILTER_COMMAND_HEADER,
    MILTER_COMMAND_END_OF_HEADER,
    MILTER_COMMAND_BODY,
    MILTER_COMMAND_END_OF_MESSAGE,
    0,
};

G_DEFINE_ABSTRACT_TYPE(MilterProtocolAgent, milter_protocol_agent,
                       MILTER_TYPE_AGENT)


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
milter_protocol_agent_class_init (MilterProtocolAgentClass *klass)
{
    GObjectClass *gobject_class;
    GParamSpec *spec;

    gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->dispose      = dispose;
    gobject_class->set_property = set_property;
    gobject_class->get_property = get_property;

    spec = g_param_spec_enum("macro-context",
                             "macro context",
                             "The current macro context",
                             MILTER_TYPE_COMMAND,
                             MILTER_COMMAND_UNKNOWN,
                             G_PARAM_READWRITE);
    g_object_class_install_property(gobject_class, PROP_MACRO_CONTEXT, spec);

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

static void
set_property (GObject      *object,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
    switch (prop_id) {
    case PROP_MACRO_CONTEXT:
        milter_protocol_agent_set_macro_context(MILTER_PROTOCOL_AGENT(object),
                                                g_value_get_enum(value));
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
    MilterProtocolAgentPrivate *priv;

    priv = MILTER_PROTOCOL_AGENT_GET_PRIVATE(object);
    switch (prop_id) {
    case PROP_MACRO_CONTEXT:
        g_value_set_enum(value, priv->macro_context);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
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

        macros = g_hash_table_lookup(priv->macros, GINT_TO_POINTER(context));
        if (macros)
            milter_utils_merge_hash_string_string(priv->available_macros,
                                                  macros);
        if (context == priv->macro_context)
            break;
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

void
milter_protocol_agent_clear_message_related_macros (MilterProtocolAgent *agent)
{
    MilterProtocolAgentPrivate *priv;

    priv = MILTER_PROTOCOL_AGENT_GET_PRIVATE(agent);
#define CLEAR_MACRO(command)                                            \
    g_hash_table_remove(priv->macros,                                   \
                        GINT_TO_POINTER(MILTER_COMMAND_ ## command))

    CLEAR_MACRO(ENVELOPE_FROM);
    CLEAR_MACRO(ENVELOPE_RECIPIENT);
    CLEAR_MACRO(DATA);
    CLEAR_MACRO(HEADER);
    CLEAR_MACRO(END_OF_HEADER);
    CLEAR_MACRO(BODY);
    CLEAR_MACRO(END_OF_MESSAGE);

#undef CLEAR_MACRO
    clear_available_macros(priv);
}

static void
update_macro (GHashTable *macros, const gchar *name, const gchar *value)
{
    if (value) {
        g_hash_table_replace(macros, g_strdup(name), g_strdup(value));
    } else {
        g_hash_table_remove(macros, name);
    }
}

static GHashTable *
ensure_macros (MilterProtocolAgentPrivate *priv, MilterCommand macro_context)
{
    GHashTable *macros;

    macros = g_hash_table_lookup(priv->macros, GINT_TO_POINTER(macro_context));
    if (!macros) {
        macros = g_hash_table_new_full(g_str_hash, g_str_equal,
                                       g_free, g_free);
        g_hash_table_insert(priv->macros,
                            GINT_TO_POINTER(macro_context),
                            macros);
    }
    return macros;
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
    GHashTable *macros;
    MilterProtocolAgentPrivate *priv;

    priv = MILTER_PROTOCOL_AGENT_GET_PRIVATE(agent);
    macros = ensure_macros(priv, macro_context);
    name = macro_name;
    while (name) {
        const gchar *value;
        value = va_arg(var_args, gchar *);
        update_macro(macros, name, value);
        name = va_arg(var_args, gchar *);
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

static void
cb_copy_macro (gpointer key, gpointer value, gpointer user_data)
{
    GHashTable *macros = user_data;
    const gchar *macro_name = key;
    const gchar *macro_value = value;

    if (!macro_value)
        return;

    g_hash_table_replace(macros, g_strdup(macro_name), g_strdup(macro_value));
}

void
milter_protocol_agent_set_macros_hash_table (MilterProtocolAgent *agent,
                                             MilterCommand macro_context,
                                             GHashTable *macros)
{
    MilterProtocolAgentPrivate *priv;
    GHashTable *new_macros;

    priv = MILTER_PROTOCOL_AGENT_GET_PRIVATE(agent);
    new_macros = g_hash_table_new_full(g_str_hash, g_str_equal,
                                       g_free, g_free);
    g_hash_table_insert(priv->macros,
                        GINT_TO_POINTER(macro_context),
                        new_macros);
    g_hash_table_foreach(macros, cb_copy_macro, new_macros);
    clear_available_macros(priv);
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

    macros = ensure_macros(priv, macro_context);
    update_macro(macros, macro_name, macro_value);
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
