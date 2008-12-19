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

#include "milter-manager-leader.h"
#include "milter-manager-enum-types.h"
#include "milter-manager-children.h"

#define MILTER_MANAGER_LEADER_GET_PRIVATE(obj)                   \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj),                          \
                                 MILTER_TYPE_MANAGER_LEADER,     \
                                 MilterManagerLeaderPrivate))

typedef struct _MilterManagerLeaderPrivate MilterManagerLeaderPrivate;
struct _MilterManagerLeaderPrivate
{
    MilterManagerConfiguration *configuration;
    MilterClientContext *client_context;
    MilterManagerChildren *children;
    MilterManagerLeaderState state;
    gboolean sent_end_of_message;
    GIOChannel *launcher_read_channel;
    GIOChannel *launcher_write_channel;
};

enum
{
    PROP_0,
    PROP_CONFIGURATION,
    PROP_CLIENT_CONTEXT
};

MILTER_IMPLEMENT_ERROR_EMITTABLE(error_emittable_init);
MILTER_IMPLEMENT_FINISHED_EMITTABLE(finished_emittable_init);
G_DEFINE_TYPE_WITH_CODE(MilterManagerLeader, milter_manager_leader, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(MILTER_TYPE_ERROR_EMITTABLE, error_emittable_init)
    G_IMPLEMENT_INTERFACE(MILTER_TYPE_FINISHED_EMITTABLE, finished_emittable_init))

static void dispose        (GObject         *object);
static void set_property   (GObject         *object,
                            guint            prop_id,
                            const GValue    *value,
                            GParamSpec      *pspec);
static void get_property   (GObject         *object,
                            guint            prop_id,
                            GValue          *value,
                            GParamSpec      *pspec);
static void teardown_children_signals
                           (MilterManagerLeader *leader,
                            MilterManagerChildren *children);

static void
milter_manager_leader_class_init (MilterManagerLeaderClass *klass)
{
    GObjectClass *gobject_class;
    GParamSpec *spec;

    gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->dispose      = dispose;
    gobject_class->set_property = set_property;
    gobject_class->get_property = get_property;

    spec = g_param_spec_object("configuration",
                               "Configuration",
                               "The configuration of the milter leader",
                               MILTER_TYPE_MANAGER_CONFIGURATION,
                               G_PARAM_READWRITE);
    g_object_class_install_property(gobject_class, PROP_CONFIGURATION, spec);

    spec = g_param_spec_object("client-context",
                               "Client context",
                               "The client context of the milter leader",
                               MILTER_TYPE_CLIENT_CONTEXT,
                               G_PARAM_READWRITE);
    g_object_class_install_property(gobject_class, PROP_CLIENT_CONTEXT, spec);

    g_type_class_add_private(gobject_class,
                             sizeof(MilterManagerLeaderPrivate));
}

static void
milter_manager_leader_init (MilterManagerLeader *leader)
{
    MilterManagerLeaderPrivate *priv;

    priv = MILTER_MANAGER_LEADER_GET_PRIVATE(leader);
    priv->configuration = NULL;
    priv->client_context = NULL;
    priv->children = NULL;
    priv->state = MILTER_SERVER_CONTEXT_STATE_START;
    priv->sent_end_of_message = FALSE;
    priv->launcher_read_channel = NULL;
    priv->launcher_write_channel = NULL;
}

static void
dispose (GObject *object)
{
    MilterManagerLeader *leader;
    MilterManagerLeaderPrivate *priv;

    leader = MILTER_MANAGER_LEADER(object);
    priv = MILTER_MANAGER_LEADER_GET_PRIVATE(object);

    if (priv->configuration) {
        g_object_unref(priv->configuration);
        priv->configuration = NULL;
    }

    if (priv->client_context) {
        g_object_unref(priv->client_context);
        priv->client_context = NULL;
    }

    if (priv->children) {
        teardown_children_signals(leader, priv->children);
        g_object_unref(priv->children);
        priv->children = NULL;
    }
    milter_manager_leader_set_launcher_channel(leader, NULL, NULL);

    G_OBJECT_CLASS(milter_manager_leader_parent_class)->dispose(object);
}

static void
set_property (GObject      *object,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
    MilterManagerLeaderPrivate *priv;

    priv = MILTER_MANAGER_LEADER_GET_PRIVATE(object);
    switch (prop_id) {
      case PROP_CONFIGURATION:
        if (priv->configuration)
            g_object_unref(priv->configuration);
        priv->configuration = g_value_get_object(value);
        if (priv->configuration)
            g_object_ref(priv->configuration);
        break;
      case PROP_CLIENT_CONTEXT:
        if (priv->client_context)
            g_object_unref(priv->client_context);
        priv->client_context = g_value_get_object(value);
        if (priv->client_context)
            g_object_ref(priv->client_context);
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
    MilterManagerLeaderPrivate *priv;

    priv = MILTER_MANAGER_LEADER_GET_PRIVATE(object);
    switch (prop_id) {
      case PROP_CONFIGURATION:
        g_value_set_object(value, priv->configuration);
        break;
      case PROP_CLIENT_CONTEXT:
        g_value_set_object(value, priv->client_context);
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

MilterManagerLeader *
milter_manager_leader_new (MilterManagerConfiguration *configuration,
                           MilterClientContext *client_context)
{
    return g_object_new(MILTER_TYPE_MANAGER_LEADER,
                        "configuration", configuration,
                        "client-context", client_context,
                        NULL);
}

static const gchar *
state_to_response_signal_name (MilterManagerLeaderState state)
{
    const gchar *signal_name;

    switch (state) {
      case MILTER_MANAGER_LEADER_STATE_NEGOTIATE:
        signal_name = "negotiate-response";
        break;
      case MILTER_MANAGER_LEADER_STATE_CONNECT:
        signal_name = "connect-response";
        break;
      case MILTER_MANAGER_LEADER_STATE_HELO:
        signal_name = "helo-response";
        break;
      case MILTER_MANAGER_LEADER_STATE_ENVELOPE_FROM:
        signal_name = "envelope-from-response";
        break;
      case MILTER_MANAGER_LEADER_STATE_ENVELOPE_RECIPIENT:
        signal_name = "envelope-recipient-response";
        break;
      case MILTER_MANAGER_LEADER_STATE_DATA:
        signal_name = "data-response";
        break;
      case MILTER_MANAGER_LEADER_STATE_UNKNOWN:
        signal_name = "unknown-response";
        break;
      case MILTER_MANAGER_LEADER_STATE_HEADER:
        signal_name = "header-response";
        break;
      case MILTER_MANAGER_LEADER_STATE_END_OF_HEADER:
        signal_name = "end-of-header-response";
        break;
      case MILTER_MANAGER_LEADER_STATE_BODY:
      case MILTER_MANAGER_LEADER_STATE_BODY_REPLIED:
        signal_name = "body-response";
        break;
      case MILTER_MANAGER_LEADER_STATE_END_OF_MESSAGE:
        signal_name = "end-of-message-response";
        break;
      case MILTER_MANAGER_LEADER_STATE_QUIT:
        signal_name = "quit-response";
        break;
      case MILTER_MANAGER_LEADER_STATE_ABORT:
        signal_name = "abort-response";
        break;
      default:
        signal_name = "error-invalid-state";
        break;
    }

    return signal_name;
}

static MilterManagerLeaderState
next_state (MilterManagerLeader *leader,
            MilterManagerLeaderState state)
{
    MilterManagerLeaderState next_state;

    switch (state) {
      case MILTER_MANAGER_LEADER_STATE_NEGOTIATE:
        next_state = MILTER_MANAGER_LEADER_STATE_NEGOTIATE_REPLIED;
        break;
      case MILTER_MANAGER_LEADER_STATE_CONNECT:
        next_state = MILTER_MANAGER_LEADER_STATE_CONNECT_REPLIED;
        break;
      case MILTER_MANAGER_LEADER_STATE_HELO:
        next_state = MILTER_MANAGER_LEADER_STATE_HELO_REPLIED;
        break;
      case MILTER_MANAGER_LEADER_STATE_ENVELOPE_FROM:
        next_state = MILTER_MANAGER_LEADER_STATE_ENVELOPE_FROM_REPLIED;
        break;
      case MILTER_MANAGER_LEADER_STATE_ENVELOPE_RECIPIENT:
        next_state = MILTER_MANAGER_LEADER_STATE_ENVELOPE_RECIPIENT_REPLIED;
        break;
      case MILTER_MANAGER_LEADER_STATE_DATA:
        next_state = MILTER_MANAGER_LEADER_STATE_DATA_REPLIED;
        break;
      case MILTER_MANAGER_LEADER_STATE_UNKNOWN:
        next_state = MILTER_MANAGER_LEADER_STATE_UNKNOWN_REPLIED;
        break;
      case MILTER_MANAGER_LEADER_STATE_HEADER:
        next_state = MILTER_MANAGER_LEADER_STATE_HEADER_REPLIED;
        break;
      case MILTER_MANAGER_LEADER_STATE_END_OF_HEADER:
        next_state = MILTER_MANAGER_LEADER_STATE_END_OF_HEADER_REPLIED;
        break;
      case MILTER_MANAGER_LEADER_STATE_BODY:
      case MILTER_MANAGER_LEADER_STATE_BODY_REPLIED:
      {
        MilterManagerLeaderPrivate *priv;
        priv = MILTER_MANAGER_LEADER_GET_PRIVATE(leader);
        if (!priv->sent_end_of_message)
            next_state = MILTER_MANAGER_LEADER_STATE_BODY_REPLIED;
        else
            next_state = MILTER_MANAGER_LEADER_STATE_END_OF_MESSAGE;
        break;
      }
      case MILTER_MANAGER_LEADER_STATE_END_OF_MESSAGE:
        next_state = MILTER_MANAGER_LEADER_STATE_END_OF_MESSAGE_REPLIED;
        break;
      case MILTER_MANAGER_LEADER_STATE_QUIT:
        next_state = MILTER_MANAGER_LEADER_STATE_QUIT_REPLIED;
        break;
      case MILTER_MANAGER_LEADER_STATE_ABORT:
        next_state = MILTER_MANAGER_LEADER_STATE_ABORT_REPLIED;
        break;
      default:
        {
            gchar *inspected_state;

            inspected_state =
                milter_utils_inspect_enum(MILTER_TYPE_MANAGER_LEADER_STATE,
                                          state);
            g_print("ERROR! - invalid state: %s|FIXME\n", inspected_state);
            g_free(inspected_state);
        }
        next_state = MILTER_MANAGER_LEADER_STATE_INVALID;
        break;
    }

    return next_state;
}


static void
cb_negotiate_reply (MilterServerContext *context, MilterOption *option,
                    MilterMacrosRequests *macros_requests, gpointer user_data)
{
    MilterManagerLeader *leader = user_data;
    MilterManagerLeaderPrivate *priv;

    priv = MILTER_MANAGER_LEADER_GET_PRIVATE(leader);

    g_signal_emit_by_name(priv->client_context, "negotiate-response",
                          option, macros_requests, MILTER_STATUS_CONTINUE);
}

static void
reply (MilterManagerLeader *leader, MilterStatus status)
{
    MilterManagerLeaderPrivate *priv;

    priv = MILTER_MANAGER_LEADER_GET_PRIVATE(leader);
    if (priv->state == MILTER_MANAGER_LEADER_STATE_NEGOTIATE) {
        /* FIXME: should pass option and macros requests. */
        g_signal_emit_by_name(priv->client_context, "negotiate-response",
                              NULL, NULL, status);
    } else {
        g_signal_emit_by_name(priv->client_context,
                              state_to_response_signal_name(priv->state),
                              status);
    }
    priv->state = next_state(leader, priv->state);
}

static void
cb_continue (MilterServerContext *context, gpointer user_data)
{
    MilterManagerLeader *leader = user_data;
    reply(leader, MILTER_STATUS_CONTINUE);
}

static void
cb_reply_code (MilterServerContext *context,
               guint code,
               const gchar *extended_code,
               const gchar *message,
               gpointer user_data)
{
    MilterManagerLeader *leader = user_data;
    MilterManagerLeaderPrivate *priv;
    GError *error = NULL;

    priv = MILTER_MANAGER_LEADER_GET_PRIVATE(leader);
    milter_client_context_set_reply(priv->client_context,
                                    code, extended_code, message,
                                    &error);
    
    if (error) {
        milter_error("%s", error->message);
        milter_error_emittable_emit(MILTER_ERROR_EMITTABLE(leader),
                                    error);
        g_error_free(error);
        return;
    }

    reply(leader, MILTER_STATUS_REJECT);
}

static void
cb_temporary_failure (MilterServerContext *context, gpointer user_data)
{
    MilterManagerLeader *leader = user_data;
    reply(leader, MILTER_STATUS_TEMPORARY_FAILURE);
}

static void
cb_reject (MilterServerContext *context, gpointer user_data)
{
    MilterManagerLeader *leader = user_data;
    reply(leader, MILTER_STATUS_REJECT);
}

static void
cb_accept (MilterServerContext *context, gpointer user_data)
{
    MilterManagerLeader *leader = user_data;
    reply(leader, MILTER_STATUS_ACCEPT);
}

static void
cb_discard (MilterServerContext *context, gpointer user_data)
{
    MilterManagerLeader *leader = user_data;
    reply(leader, MILTER_STATUS_DISCARD);
}

static void
cb_add_header (MilterServerContext *context,
               const gchar *name, const gchar *value,
               gpointer user_data)
{
    MilterManagerLeader *leader = user_data;
    MilterManagerLeaderPrivate *priv;

    priv = MILTER_MANAGER_LEADER_GET_PRIVATE(leader);
    milter_client_context_add_header(priv->client_context, name, value);
}

static void
cb_insert_header (MilterServerContext *context,
                  guint32 index, const gchar *name, const gchar *value,
                  gpointer user_data)
{
    MilterManagerLeader *leader = user_data;
    MilterManagerLeaderPrivate *priv;

    priv = MILTER_MANAGER_LEADER_GET_PRIVATE(leader);
    milter_client_context_insert_header(priv->client_context,
                                        index, name, value);
}

static void
cb_change_header (MilterServerContext *context,
                  const gchar *name, guint32 index, const gchar *value,
                  gpointer user_data)
{
    MilterManagerLeader *leader = user_data;
    MilterManagerLeaderPrivate *priv;

    priv = MILTER_MANAGER_LEADER_GET_PRIVATE(leader);
    milter_client_context_change_header(priv->client_context,
                                        name, index, value);
}

static void
cb_delete_header (MilterServerContext *context,
                  const gchar *name, guint32 index,
                  gpointer user_data)
{
    MilterManagerLeader *leader = user_data;
    MilterManagerLeaderPrivate *priv;

    priv = MILTER_MANAGER_LEADER_GET_PRIVATE(leader);
    milter_client_context_delete_header(priv->client_context, name, index);
}

static void
cb_change_from (MilterServerContext *context,
                const gchar *from, const gchar *parameters,
                gpointer user_data)
{
    MilterManagerLeader *leader = user_data;
    MilterManagerLeaderPrivate *priv;

    priv = MILTER_MANAGER_LEADER_GET_PRIVATE(leader);
    milter_client_context_change_from(priv->client_context, from, parameters);
}

static void
cb_add_recipient (MilterServerContext *context,
                  const gchar *recipient, const gchar *parameters,
                  gpointer user_data)
{
    MilterManagerLeader *leader = user_data;
    MilterManagerLeaderPrivate *priv;

    priv = MILTER_MANAGER_LEADER_GET_PRIVATE(leader);
    milter_client_context_add_recipient(priv->client_context,
                                        recipient, parameters);
}

static void
cb_delete_recipient (MilterServerContext *context,
                     const gchar *recipient,
                     gpointer user_data)
{
    MilterManagerLeader *leader = user_data;
    MilterManagerLeaderPrivate *priv;

    priv = MILTER_MANAGER_LEADER_GET_PRIVATE(leader);
    milter_client_context_delete_recipient(priv->client_context, recipient);
}

static void
cb_replace_body (MilterServerContext *context,
                 const gchar *chunk, gsize chunk_size,
                 gpointer user_data)
{
    MilterManagerLeader *leader = user_data;
    MilterManagerLeaderPrivate *priv;

    priv = MILTER_MANAGER_LEADER_GET_PRIVATE(leader);
    milter_client_context_replace_body(priv->client_context, chunk, chunk_size);
}

static void
cb_progress (MilterServerContext *context, gpointer user_data)
{
    MilterManagerLeader *leader = user_data;
    MilterManagerLeaderPrivate *priv;

    priv = MILTER_MANAGER_LEADER_GET_PRIVATE(leader);
    milter_client_context_progress(priv->client_context);
}

static void
cb_quarantine (MilterServerContext *context,
               const gchar *reason,
               gpointer user_data)
{
    MilterManagerLeader *leader = user_data;
    MilterManagerLeaderPrivate *priv;

    priv = MILTER_MANAGER_LEADER_GET_PRIVATE(leader);
    milter_client_context_quarantine(priv->client_context, reason);
}

static void
cb_connection_failure (MilterServerContext *context, gpointer user_data)
{
    milter_debug(PACKAGE_STRING "could'nt handle CONNECTION-FAILURE reply yet.");
}

static void
cb_shutdown (MilterServerContext *context, gpointer user_data)
{
    milter_debug(PACKAGE_STRING "could'nt handle SHUTDOWN reply yet.");
}

static void
cb_skip (MilterServerContext *context, gpointer user_data)
{
    MilterManagerLeader *leader = user_data;
    reply(leader, MILTER_STATUS_SKIP);
}

static void
cb_error (MilterErrorEmittable *emittable, GError *error, gpointer user_data)
{
    MilterManagerLeader *leader = user_data;

    milter_error("%s", error->message);
    milter_error_emittable_emit(MILTER_ERROR_EMITTABLE(leader),
                                error);

    /* If all milters do not response on negotiation, send "reject" */
    if (error->domain != MILTER_MANAGER_CHILDREN_ERROR)
        return;

    if (error->code == MILTER_MANAGER_CHILDREN_ERROR_NO_NEGOTIATION_RESPONSE)
        reply(leader, MILTER_STATUS_REJECT);
    else
        reply(leader, MILTER_STATUS_TEMPORARY_FAILURE);
}

static void
cb_finished (MilterFinishedEmittable *emittable, gpointer user_data)
{
    milter_finished_emittable_emit(MILTER_FINISHED_EMITTABLE(user_data));
}

static void
setup_children_signals (MilterManagerLeader *leader,
                        MilterManagerChildren *children)
{
#define CONNECT(name)                                           \
    g_signal_connect(children, #name,                           \
                     G_CALLBACK(cb_ ## name), leader)

    CONNECT(negotiate_reply);
    CONNECT(continue);
    CONNECT(reply_code);
    CONNECT(temporary_failure);
    CONNECT(reject);
    CONNECT(accept);
    CONNECT(discard);
    CONNECT(add_header);
    CONNECT(insert_header);
    CONNECT(change_header);
    CONNECT(delete_header);
    CONNECT(change_from);
    CONNECT(add_recipient);
    CONNECT(delete_recipient);
    CONNECT(replace_body);
    CONNECT(progress);
    CONNECT(quarantine);
    CONNECT(connection_failure);
    CONNECT(shutdown);
    CONNECT(skip);

    CONNECT(error);
    CONNECT(finished);
#undef CONNECT
}

static void
teardown_children_signals (MilterManagerLeader *leader,
                           MilterManagerChildren *children)
{

#define DISCONNECT(name)                                                \
    g_signal_handlers_disconnect_by_func(children,                      \
                                         G_CALLBACK(cb_ ## name),       \
                                         leader)

    DISCONNECT(negotiate_reply);
    DISCONNECT(continue);
    DISCONNECT(reply_code);
    DISCONNECT(temporary_failure);
    DISCONNECT(reject);
    DISCONNECT(accept);
    DISCONNECT(discard);
    DISCONNECT(add_header);
    DISCONNECT(insert_header);
    DISCONNECT(change_header);
    DISCONNECT(delete_header);
    DISCONNECT(change_from);
    DISCONNECT(add_recipient);
    DISCONNECT(delete_recipient);
    DISCONNECT(replace_body);
    DISCONNECT(progress);
    DISCONNECT(quarantine);
    DISCONNECT(connection_failure);
    DISCONNECT(shutdown);
    DISCONNECT(skip);

    DISCONNECT(error);
    DISCONNECT(finished);
#undef DISCONNECT
}

MilterStatus
milter_manager_leader_negotiate (MilterManagerLeader *leader,
                                 MilterOption *option,
                                 MilterMacrosRequests *macros_requests)
{
    MilterManagerLeaderPrivate *priv;

    priv = MILTER_MANAGER_LEADER_GET_PRIVATE(leader);
    priv->state = MILTER_MANAGER_LEADER_STATE_NEGOTIATE;

    priv->children =
        milter_manager_configuration_create_children(priv->configuration);

    if (!priv->children)
        return MILTER_STATUS_NOT_CHANGE;

    setup_children_signals(leader, priv->children);
    milter_manager_children_set_launcher_channel(priv->children,
                                                 priv->launcher_read_channel,
                                                 priv->launcher_write_channel);

    if (milter_manager_children_negotiate(priv->children, option,
                                          macros_requests))
        return MILTER_STATUS_PROGRESS;
    else
        return MILTER_STATUS_NOT_CHANGE; /* FIXME: reject or accept */
}

MilterStatus
milter_manager_leader_connect (MilterManagerLeader *leader,
                               const gchar         *host_name,
                               struct sockaddr     *address,
                               socklen_t            address_length)
{
    MilterManagerLeaderPrivate *priv;

    priv = MILTER_MANAGER_LEADER_GET_PRIVATE(leader);
    priv->state = MILTER_MANAGER_LEADER_STATE_CONNECT;

    if (!priv->children)
        return MILTER_STATUS_NOT_CHANGE;

    if (milter_manager_children_connect(priv->children, host_name,
                                        address, address_length))
        return MILTER_STATUS_PROGRESS;
    else
        return MILTER_STATUS_NOT_CHANGE; /* FIXME: reject or accept */
}

MilterStatus
milter_manager_leader_helo (MilterManagerLeader *leader, const gchar *fqdn)
{
    MilterManagerLeaderPrivate *priv;

    priv = MILTER_MANAGER_LEADER_GET_PRIVATE(leader);
    priv->state = MILTER_MANAGER_LEADER_STATE_HELO;

    if (!priv->children)
        return MILTER_STATUS_NOT_CHANGE;

    if (milter_manager_children_helo(priv->children, fqdn))
        return MILTER_STATUS_PROGRESS;
    else
        return MILTER_STATUS_NOT_CHANGE; /* FIXME: reject or accept */
}

MilterStatus
milter_manager_leader_envelope_from (MilterManagerLeader *leader,
                                     const gchar *from)
{
    MilterManagerLeaderPrivate *priv;

    priv = MILTER_MANAGER_LEADER_GET_PRIVATE(leader);
    priv->state = MILTER_MANAGER_LEADER_STATE_ENVELOPE_FROM;

    if (!priv->children)
        return MILTER_STATUS_NOT_CHANGE;

    if (milter_manager_children_envelope_from(priv->children, from))
        return MILTER_STATUS_PROGRESS;
    else
        return MILTER_STATUS_NOT_CHANGE; /* FIXME: reject or accept */
}

MilterStatus
milter_manager_leader_envelope_recipient (MilterManagerLeader *leader,
                                          const gchar *recipient)
{
    MilterManagerLeaderPrivate *priv;

    priv = MILTER_MANAGER_LEADER_GET_PRIVATE(leader);
    priv->state = MILTER_MANAGER_LEADER_STATE_ENVELOPE_RECIPIENT;

    if (!priv->children)
        return MILTER_STATUS_NOT_CHANGE;

    if (milter_manager_children_envelope_recipient(priv->children, recipient))
        return MILTER_STATUS_PROGRESS;
    else
        return MILTER_STATUS_NOT_CHANGE; /* FIXME: reject or accept */
}

MilterStatus
milter_manager_leader_data (MilterManagerLeader *leader)
{
    MilterManagerLeaderPrivate *priv;

    priv = MILTER_MANAGER_LEADER_GET_PRIVATE(leader);
    priv->state = MILTER_MANAGER_LEADER_STATE_DATA;

    if (!priv->children)
        return MILTER_STATUS_NOT_CHANGE;

    if (milter_manager_children_data(priv->children))
        return MILTER_STATUS_PROGRESS;
    else
        return MILTER_STATUS_NOT_CHANGE; /* FIXME: reject or accept */
}

MilterStatus
milter_manager_leader_unknown (MilterManagerLeader *leader,
                               const gchar *command)
{
    MilterManagerLeaderPrivate *priv;

    priv = MILTER_MANAGER_LEADER_GET_PRIVATE(leader);
    priv->state = MILTER_MANAGER_LEADER_STATE_UNKNOWN;

    if (!priv->children)
        return MILTER_STATUS_NOT_CHANGE;

    if (milter_manager_children_unknown(priv->children, command))
        return MILTER_STATUS_PROGRESS;
    else
        return MILTER_STATUS_NOT_CHANGE; /* FIXME: reject or accept */
}

MilterStatus
milter_manager_leader_header (MilterManagerLeader *leader,
                              const gchar *name, const gchar *value)
{
    MilterManagerLeaderPrivate *priv;

    priv = MILTER_MANAGER_LEADER_GET_PRIVATE(leader);
    priv->state = MILTER_MANAGER_LEADER_STATE_HEADER;

    if (!priv->children)
        return MILTER_STATUS_NOT_CHANGE;

    if (milter_manager_children_header(priv->children, name, value))
        return MILTER_STATUS_PROGRESS;
    else
        return MILTER_STATUS_NOT_CHANGE; /* FIXME: reject or accept */
}

MilterStatus
milter_manager_leader_end_of_header (MilterManagerLeader *leader)
{
    MilterManagerLeaderPrivate *priv;

    priv = MILTER_MANAGER_LEADER_GET_PRIVATE(leader);
    priv->state = MILTER_MANAGER_LEADER_STATE_END_OF_HEADER;

    if (!priv->children)
        return MILTER_STATUS_NOT_CHANGE;

    if (milter_manager_children_end_of_header(priv->children))
        return MILTER_STATUS_PROGRESS;
    else
        return MILTER_STATUS_NOT_CHANGE; /* FIXME: reject or accept */
}

MilterStatus
milter_manager_leader_body (MilterManagerLeader *leader,
                            const gchar *chunk, gsize size)
{
    MilterManagerLeaderPrivate *priv;

    priv = MILTER_MANAGER_LEADER_GET_PRIVATE(leader);
    priv->state = MILTER_MANAGER_LEADER_STATE_BODY;

    if (!priv->children)
        return MILTER_STATUS_NOT_CHANGE;

    if (milter_manager_children_body(priv->children, chunk, size))
        return MILTER_STATUS_PROGRESS;
    else
        return MILTER_STATUS_NOT_CHANGE; /* FIXME: reject or accept */
}

static gboolean
is_replied_state (MilterManagerLeaderState state)
{
    switch (state) {
      case MILTER_MANAGER_LEADER_STATE_NEGOTIATE:
      case MILTER_MANAGER_LEADER_STATE_CONNECT:
      case MILTER_MANAGER_LEADER_STATE_HELO:
      case MILTER_MANAGER_LEADER_STATE_ENVELOPE_FROM:
      case MILTER_MANAGER_LEADER_STATE_ENVELOPE_RECIPIENT:
      case MILTER_MANAGER_LEADER_STATE_DATA:
      case MILTER_MANAGER_LEADER_STATE_UNKNOWN:
      case MILTER_MANAGER_LEADER_STATE_HEADER:
      case MILTER_MANAGER_LEADER_STATE_END_OF_HEADER:
      case MILTER_MANAGER_LEADER_STATE_BODY:
      case MILTER_MANAGER_LEADER_STATE_END_OF_MESSAGE:
      case MILTER_MANAGER_LEADER_STATE_QUIT:
      case MILTER_MANAGER_LEADER_STATE_ABORT:
        return FALSE;
      default:
        return TRUE;
        break;
    }
}

MilterStatus
milter_manager_leader_end_of_message (MilterManagerLeader *leader,
                                      const gchar         *chunk,
                                      gsize                size)
{
    MilterManagerLeaderPrivate *priv;

    priv = MILTER_MANAGER_LEADER_GET_PRIVATE(leader);
    if (is_replied_state(priv->state)) {
        priv->state = MILTER_MANAGER_LEADER_STATE_END_OF_MESSAGE;
    } else {
        priv->sent_end_of_message = TRUE;
    }

    if (!priv->children)
        return MILTER_STATUS_NOT_CHANGE;

    if (milter_manager_children_end_of_message(priv->children, chunk, size))
        return MILTER_STATUS_PROGRESS;
    else
        return MILTER_STATUS_NOT_CHANGE; /* FIXME: reject or accept */
}

MilterStatus
milter_manager_leader_quit (MilterManagerLeader *leader)
{
    MilterManagerLeaderPrivate *priv;

    priv = MILTER_MANAGER_LEADER_GET_PRIVATE(leader);
    priv->state = MILTER_MANAGER_LEADER_STATE_QUIT;

    if (!priv->children)
        return MILTER_STATUS_NOT_CHANGE;

    if (milter_manager_children_quit(priv->children))
        return MILTER_STATUS_PROGRESS;
    else
        return MILTER_STATUS_NOT_CHANGE; /* FIXME: reject or accept */
}

MilterStatus
milter_manager_leader_abort (MilterManagerLeader *leader)
{
    MilterManagerLeaderPrivate *priv;

    priv = MILTER_MANAGER_LEADER_GET_PRIVATE(leader);
    priv->state = MILTER_MANAGER_LEADER_STATE_ABORT;

    if (!priv->children)
        return MILTER_STATUS_NOT_CHANGE;

    if (milter_manager_children_abort(priv->children))
        return MILTER_STATUS_PROGRESS;
    else
        return MILTER_STATUS_NOT_CHANGE; /* FIXME: reject or accept */
}

void
milter_manager_leader_define_macro (MilterManagerLeader *leader,
                                    MilterCommand command,
                                    GHashTable *macros)
{
    MilterManagerLeaderPrivate *priv;

    priv = MILTER_MANAGER_LEADER_GET_PRIVATE(leader);

    if (!priv->children)
        return;

    milter_manager_children_define_macro(priv->children,
                                         command, macros);
}

void
milter_manager_leader_timeout (MilterManagerLeader *leader)
{
    milter_manager_leader_quit(leader);
}

void
milter_manager_leader_set_launcher_channel (MilterManagerLeader *leader,
                                            GIOChannel *read_channel,
                                            GIOChannel *write_channel)
{
    MilterManagerLeaderPrivate *priv;

    priv = MILTER_MANAGER_LEADER_GET_PRIVATE(leader);

    if (priv->launcher_write_channel)
        g_io_channel_unref(priv->launcher_write_channel);
    priv->launcher_write_channel = write_channel;
    if (priv->launcher_write_channel)
        g_io_channel_ref(priv->launcher_write_channel);

    if (priv->launcher_read_channel)
        g_io_channel_unref(priv->launcher_read_channel);
    priv->launcher_read_channel = read_channel;
    if (priv->launcher_read_channel)
        g_io_channel_ref(priv->launcher_read_channel);
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
