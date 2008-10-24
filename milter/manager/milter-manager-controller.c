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

#include "milter-manager-controller.h"
#include "milter-manager-enum-types.h"

#define MILTER_MANAGER_CONTROLLER_GET_PRIVATE(obj)                   \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj),                              \
                                 MILTER_TYPE_MANAGER_CONTROLLER,     \
                                 MilterManagerControllerPrivate))

typedef struct _MilterManagerControllerPrivate MilterManagerControllerPrivate;
struct _MilterManagerControllerPrivate
{
    MilterManagerConfiguration *configuration;
    MilterClientContext *client_context;
    GList *children;
    MilterManagerControllerState state;
    MilterMacrosRequests *macros_requests;
};

enum
{
    PROP_0,
    PROP_CONFIGURATION,
    PROP_CLIENT_CONTEXT
};

MILTER_DEFINE_ERROR_EMITABLE_TYPE(MilterManagerController,
                                  milter_manager_controller,
                                  G_TYPE_OBJECT)

static void dispose        (GObject         *object);
static void set_property   (GObject         *object,
                            guint            prop_id,
                            const GValue    *value,
                            GParamSpec      *pspec);
static void get_property   (GObject         *object,
                            guint            prop_id,
                            GValue          *value,
                            GParamSpec      *pspec);

static void teardown_server_context_signals
                           (MilterManagerController *controller,
                            MilterServerContext *server_context);

static void
milter_manager_controller_class_init (MilterManagerControllerClass *klass)
{
    GObjectClass *gobject_class;
    GParamSpec *spec;

    gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->dispose      = dispose;
    gobject_class->set_property = set_property;
    gobject_class->get_property = get_property;

    spec = g_param_spec_object("configuration",
                               "Configuration",
                               "The configuration of the milter controller",
                               MILTER_TYPE_MANAGER_CONFIGURATION,
                               G_PARAM_READWRITE);
    g_object_class_install_property(gobject_class, PROP_CONFIGURATION, spec);

    spec = g_param_spec_object("client-context",
                               "Client context",
                               "The client context of the milter controller",
                               MILTER_TYPE_CLIENT_CONTEXT,
                               G_PARAM_READWRITE);
    g_object_class_install_property(gobject_class, PROP_CLIENT_CONTEXT, spec);

    g_type_class_add_private(gobject_class,
                             sizeof(MilterManagerControllerPrivate));
}

static void
milter_manager_controller_init (MilterManagerController *controller)
{
    MilterManagerControllerPrivate *priv;

    priv = MILTER_MANAGER_CONTROLLER_GET_PRIVATE(controller);
    priv->configuration = NULL;
    priv->client_context = NULL;
    priv->children = NULL;
    priv->macros_requests = milter_macros_requests_new();
    priv->state = MILTER_SERVER_CONTEXT_STATE_START;
}

static void
dispose (GObject *object)
{
    MilterManagerController *controller;
    MilterManagerControllerPrivate *priv;

    controller = MILTER_MANAGER_CONTROLLER(object);
    priv = MILTER_MANAGER_CONTROLLER_GET_PRIVATE(object);

    if (priv->configuration) {
        g_object_unref(priv->configuration);
        priv->configuration = NULL;
    }

    if (priv->client_context) {
        g_object_unref(priv->client_context);
        priv->client_context = NULL;
    }

    if (priv->children) {
        GList *node;
        for (node = priv->children; node; node = g_list_next(node)) {
            MilterServerContext *server_context = node->data;
            teardown_server_context_signals(controller, server_context);
            g_object_unref(server_context);
        }
        g_list_free(priv->children);
        priv->children = NULL;
    }

    if (priv->macros_requests) {
        g_object_unref(priv->macros_requests);
        priv->macros_requests = NULL;
    }

    G_OBJECT_CLASS(milter_manager_controller_parent_class)->dispose(object);
}

static void
set_property (GObject      *object,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
    MilterManagerControllerPrivate *priv;

    priv = MILTER_MANAGER_CONTROLLER_GET_PRIVATE(object);
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
    MilterManagerControllerPrivate *priv;

    priv = MILTER_MANAGER_CONTROLLER_GET_PRIVATE(object);
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

MilterManagerController *
milter_manager_controller_new (MilterManagerConfiguration *configuration,
                               MilterClientContext *client_context)
{
    return g_object_new(MILTER_TYPE_MANAGER_CONTROLLER,
                        "configuration", configuration,
                        "client-context", client_context,
                        NULL);
}

static const gchar *
state_to_response_signal_name (MilterManagerControllerState state)
{
    const gchar *signal_name;

    switch (state) {
      case MILTER_MANAGER_CONTROLLER_STATE_NEGOTIATE:
        signal_name = "negotiate-response";
        break;
      case MILTER_MANAGER_CONTROLLER_STATE_CONNECT:
        signal_name = "connect-response";
        break;
      case MILTER_MANAGER_CONTROLLER_STATE_HELO:
        signal_name = "helo-response";
        break;
      case MILTER_MANAGER_CONTROLLER_STATE_ENVELOPE_FROM:
        signal_name = "envelope-from-response";
        break;
      case MILTER_MANAGER_CONTROLLER_STATE_ENVELOPE_RECEIPT:
        signal_name = "envelope-receipt-response";
        break;
      case MILTER_MANAGER_CONTROLLER_STATE_DATA:
        signal_name = "data-response";
        break;
      case MILTER_MANAGER_CONTROLLER_STATE_UNKNOWN:
        signal_name = "unknown-response";
        break;
      case MILTER_MANAGER_CONTROLLER_STATE_HEADER:
        signal_name = "header-response";
        break;
      case MILTER_MANAGER_CONTROLLER_STATE_END_OF_HEADER:
        signal_name = "end-of-header-response";
        break;
      case MILTER_MANAGER_CONTROLLER_STATE_BODY:
        signal_name = "body-response";
        break;
      case MILTER_MANAGER_CONTROLLER_STATE_END_OF_MESSAGE:
        signal_name = "end-of-message-response";
        break;
      case MILTER_MANAGER_CONTROLLER_STATE_QUIT:
        signal_name = "quit-response";
        break;
      case MILTER_MANAGER_CONTROLLER_STATE_ABORT:
        signal_name = "abort-response";
        break;
      default:
        signal_name = "error-invalid-state";
        break;
    }

    return signal_name;
}

static MilterManagerControllerState
next_state (MilterManagerControllerState state)
{
    MilterManagerControllerState next_state;

    switch (state) {
      case MILTER_MANAGER_CONTROLLER_STATE_NEGOTIATE:
        next_state = MILTER_MANAGER_CONTROLLER_STATE_NEGOTIATE_REPLIED;
        break;
      case MILTER_MANAGER_CONTROLLER_STATE_CONNECT:
        next_state = MILTER_MANAGER_CONTROLLER_STATE_CONNECT_REPLIED;
        break;
      case MILTER_MANAGER_CONTROLLER_STATE_HELO:
        next_state = MILTER_MANAGER_CONTROLLER_STATE_HELO_REPLIED;
        break;
      case MILTER_MANAGER_CONTROLLER_STATE_ENVELOPE_FROM:
        next_state = MILTER_MANAGER_CONTROLLER_STATE_ENVELOPE_FROM_REPLIED;
        break;
      case MILTER_MANAGER_CONTROLLER_STATE_ENVELOPE_RECEIPT:
        next_state = MILTER_MANAGER_CONTROLLER_STATE_ENVELOPE_RECEIPT_REPLIED;
        break;
      case MILTER_MANAGER_CONTROLLER_STATE_DATA:
        next_state = MILTER_MANAGER_CONTROLLER_STATE_DATA_REPLIED;
        break;
      case MILTER_MANAGER_CONTROLLER_STATE_UNKNOWN:
        next_state = MILTER_MANAGER_CONTROLLER_STATE_UNKNOWN_REPLIED;
        break;
      case MILTER_MANAGER_CONTROLLER_STATE_HEADER:
        next_state = MILTER_MANAGER_CONTROLLER_STATE_HEADER_REPLIED;
        break;
      case MILTER_MANAGER_CONTROLLER_STATE_END_OF_HEADER:
        next_state = MILTER_MANAGER_CONTROLLER_STATE_END_OF_HEADER_REPLIED;
        break;
      case MILTER_MANAGER_CONTROLLER_STATE_BODY:
        next_state = MILTER_MANAGER_CONTROLLER_STATE_BODY_REPLIED;
        break;
      case MILTER_MANAGER_CONTROLLER_STATE_END_OF_MESSAGE:
        next_state = MILTER_MANAGER_CONTROLLER_STATE_END_OF_MESSAGE_REPLIED;
        break;
      case MILTER_MANAGER_CONTROLLER_STATE_QUIT:
        next_state = MILTER_MANAGER_CONTROLLER_STATE_QUIT_REPLIED;
        break;
      case MILTER_MANAGER_CONTROLLER_STATE_ABORT:
        next_state = MILTER_MANAGER_CONTROLLER_STATE_ABORT_REPLIED;
        break;
      default:
        {
            gchar *inspected_state;

            inspected_state =
                milter_utils_inspect_enum(MILTER_TYPE_MANAGER_CONTROLLER_STATE,
                                          state);
            g_print("ERROR! - invalid state: %s|FIXME\n", inspected_state);
            g_free(inspected_state);
        }
        next_state = MILTER_MANAGER_CONTROLLER_STATE_INVALID;
        break;
    }

    return next_state;
}


static void
cb_negotiate_reply (MilterServerContext *context, MilterOption *option,
                    MilterMacrosRequests *macros_requests, gpointer user_data)
{
    MilterManagerController *controller = user_data;
    MilterManagerControllerPrivate *priv;

    priv = MILTER_MANAGER_CONTROLLER_GET_PRIVATE(controller);

    if (macros_requests)
        milter_macros_requests_merge(priv->macros_requests, macros_requests);

    g_signal_emit_by_name(priv->client_context, "negotiate-response",
                          0, option, MILTER_STATUS_CONTINUE);
}

static void
reply (MilterManagerController *controller, MilterStatus status)
{
    MilterManagerControllerPrivate *priv;

    priv = MILTER_MANAGER_CONTROLLER_GET_PRIVATE(controller);
    if (priv->state == MILTER_MANAGER_CONTROLLER_STATE_NEGOTIATE) {
        g_signal_emit_by_name(priv->client_context, "negotiate-response",
                              0, NULL, status);
    } else {
        g_signal_emit_by_name(priv->client_context,
                              state_to_response_signal_name(priv->state),
                              0, status);
    }
    priv->state = next_state(priv->state);
}

static void
cb_continue (MilterServerContext *context, gpointer user_data)
{
    MilterManagerController *controller = user_data;
    reply(controller, MILTER_STATUS_CONTINUE);
}

static void
cb_reply_code (MilterServerContext *context, const gchar *code,
               gpointer user_data)
{
    /* MilterManagerController *controller = user_data; */
    /* FIXME */
}

static void
cb_temporary_failure (MilterServerContext *context, gpointer user_data)
{
    MilterManagerController *controller = user_data;
    reply(controller, MILTER_STATUS_TEMPORARY_FAILURE);
}

static void
cb_reject (MilterServerContext *context, gpointer user_data)
{
    MilterManagerController *controller = user_data;
    reply(controller, MILTER_STATUS_REJECT);
}

static void
cb_accept (MilterServerContext *context, gpointer user_data)
{
    MilterManagerController *controller = user_data;
    reply(controller, MILTER_STATUS_ACCEPT);
}

static void
cb_discard (MilterServerContext *context, gpointer user_data)
{
    MilterManagerController *controller = user_data;
    reply(controller, MILTER_STATUS_DISCARD);
}

static void
cb_add_header (MilterServerContext *context,
               const gchar *name, const gchar *value,
               gpointer user_data)
{
    MilterManagerController *controller = user_data;
    MilterManagerControllerPrivate *priv;

    priv = MILTER_MANAGER_CONTROLLER_GET_PRIVATE(controller);
    milter_client_context_add_header(priv->client_context, name, value);
}

static void
cb_insert_header (MilterServerContext *context,
                  guint32 index, const gchar *name, const gchar *value,
                  gpointer user_data)
{
    MilterManagerController *controller = user_data;
    MilterManagerControllerPrivate *priv;

    priv = MILTER_MANAGER_CONTROLLER_GET_PRIVATE(controller);
    milter_client_context_insert_header(priv->client_context,
                                        index, name, value);
}

static void
cb_change_header (MilterServerContext *context,
                  const gchar *name, guint32 index, const gchar *value,
                  gpointer user_data)
{
    MilterManagerController *controller = user_data;
    MilterManagerControllerPrivate *priv;

    priv = MILTER_MANAGER_CONTROLLER_GET_PRIVATE(controller);
    milter_client_context_change_header(priv->client_context,
                                        name, index, value);
}

static void
cb_change_from (MilterServerContext *context,
                const gchar *from, const gchar *parameters,
                gpointer user_data)
{
    MilterManagerController *controller = user_data;
    MilterManagerControllerPrivate *priv;

    priv = MILTER_MANAGER_CONTROLLER_GET_PRIVATE(controller);
    milter_client_context_change_from(priv->client_context, from, parameters);
}

static void
cb_add_receipt (MilterServerContext *context,
                const gchar *receipt, const gchar *parameters,
                gpointer user_data)
{
    MilterManagerController *controller = user_data;
    MilterManagerControllerPrivate *priv;

    priv = MILTER_MANAGER_CONTROLLER_GET_PRIVATE(controller);
    milter_client_context_add_receipt(priv->client_context, receipt, parameters);
}

static void
cb_delete_receipt (MilterServerContext *context,
                   const gchar *receipt,
                   gpointer user_data)
{
    MilterManagerController *controller = user_data;
    MilterManagerControllerPrivate *priv;

    priv = MILTER_MANAGER_CONTROLLER_GET_PRIVATE(controller);
    milter_client_context_delete_receipt(priv->client_context, receipt);
}

static void
cb_replace_body (MilterServerContext *context,
                 const gchar *chunk, gsize chunk_size,
                 gpointer user_data)
{
    MilterManagerController *controller = user_data;
    MilterManagerControllerPrivate *priv;

    priv = MILTER_MANAGER_CONTROLLER_GET_PRIVATE(controller);
    milter_client_context_replace_body(priv->client_context, chunk, chunk_size);
}

static void
cb_progress (MilterServerContext *context, gpointer user_data)
{
    MilterManagerController *controller = user_data;
    MilterManagerControllerPrivate *priv;

    priv = MILTER_MANAGER_CONTROLLER_GET_PRIVATE(controller);
    milter_client_context_progress(priv->client_context);
}

static void
cb_quarantine (MilterServerContext *context,
               const gchar *reason,
               gpointer user_data)
{
    MilterManagerController *controller = user_data;
    MilterManagerControllerPrivate *priv;

    priv = MILTER_MANAGER_CONTROLLER_GET_PRIVATE(controller);
    milter_client_context_quarantine(priv->client_context, reason);
}

static void
cb_connection_failure (MilterServerContext *context, gpointer user_data)
{
    /* FIXME */
}

static void
cb_shutdown (MilterServerContext *context, gpointer user_data)
{
    /* FIXME */
}

static void
cb_skip (MilterServerContext *context, gpointer user_data)
{
    MilterManagerController *controller = user_data;
    reply(controller, MILTER_STATUS_SKIP);
}

static void
cb_connection_timeout (MilterServerContext *context, gpointer user_data)
{
    milter_error("connection timeout: FIXME");
}

static void
cb_sending_timeout (MilterServerContext *context, gpointer user_data)
{
    milter_error("sending timeout: FIXME");
}

static void
cb_reading_timeout (MilterServerContext *context, gpointer user_data)
{
    milter_error("reading timeout: FIXME");
}

static void
cb_end_of_message_timeout (MilterServerContext *context, gpointer user_data)
{
    milter_error("end_of_message timeout: FIXME");
}

static void
cb_error (MilterErrorEmitable *emitable, GError *error, gpointer user_data)
{
    MilterManagerController *controller = user_data;

    milter_error("error: FIXME: %s", error->message);
    milter_error_emitable_emit_error(MILTER_ERROR_EMITABLE(controller),
                                     error);
}

static void
setup_server_context_signals (MilterManagerController *controller,
                              MilterServerContext *server_context)
{
#define CONNECT(name)                                           \
    g_signal_connect(server_context, #name,                     \
                     G_CALLBACK(cb_ ## name), controller)

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
    CONNECT(change_from);
    CONNECT(add_receipt);
    CONNECT(delete_receipt);
    CONNECT(replace_body);
    CONNECT(progress);
    CONNECT(quarantine);
    CONNECT(connection_failure);
    CONNECT(shutdown);
    CONNECT(skip);

    CONNECT(connection_timeout);
    CONNECT(sending_timeout);
    CONNECT(reading_timeout);
    CONNECT(end_of_message_timeout);

    CONNECT(error);
#undef CONNECT
}

static void
teardown_server_context_signals (MilterManagerController *controller,
                                 MilterServerContext *server_context)
{
#define DISCONNECT(name)                                                \
    g_signal_handlers_disconnect_by_func(server_context,                \
                                         G_CALLBACK(cb_ ## name),       \
                                         controller)

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
    DISCONNECT(change_from);
    DISCONNECT(add_receipt);
    DISCONNECT(delete_receipt);
    DISCONNECT(replace_body);
    DISCONNECT(progress);
    DISCONNECT(quarantine);
    DISCONNECT(connection_failure);
    DISCONNECT(shutdown);
    DISCONNECT(skip);

    DISCONNECT(connection_timeout);
    DISCONNECT(sending_timeout);
    DISCONNECT(reading_timeout);
    DISCONNECT(end_of_message_timeout);

    DISCONNECT(error);
#undef DISCONNECT
}

MilterStatus
milter_manager_controller_negotiate (MilterManagerController *controller,
                                     MilterOption *option)
{
    MilterManagerControllerPrivate *priv;
    MilterServerContext *server_context;
    MilterManagerChild *child;
    GError *error = NULL;

    priv = MILTER_MANAGER_CONTROLLER_GET_PRIVATE(controller);
    priv->state = MILTER_MANAGER_CONTROLLER_STATE_NEGOTIATE;

    priv->children =
        milter_manager_configuration_create_children(priv->configuration);

    if (!priv->children)
        return MILTER_STATUS_NOT_CHANGE;

    child = priv->children->data;
    server_context = MILTER_SERVER_CONTEXT(child);
    if (!milter_server_context_establish_connection(server_context, &error)) {
        gboolean priviledge;
        MilterStatus status;

        status =
            milter_manager_configuration_get_return_status_if_filter_unavailable(priv->configuration);

        priviledge =
            milter_manager_configuration_is_privilege_mode(priv->configuration);
        if (!priviledge ||
            error->code != MILTER_SERVER_CONTEXT_ERROR_CONNECTION_FAILURE) {
            milter_error("Error: %s", error->message);
            g_error_free(error);
            return status;
        }
        g_error_free(error);

        milter_manager_child_start(child, &error);
        if (error) {
            milter_error("Error: %s", error->message);
            g_error_free(error);
            return status;
        }
        return MILTER_STATUS_PROGRESS;
    }

    setup_server_context_signals(controller, server_context);
    milter_server_context_negotiate(server_context, option);

    return MILTER_STATUS_PROGRESS;
}

MilterStatus
milter_manager_controller_connect (MilterManagerController *controller,
                                   const gchar             *host_name,
                                   struct sockaddr         *address,
                                   socklen_t                address_length)
{
    MilterManagerControllerPrivate *priv;
    MilterServerContext *context;

    priv = MILTER_MANAGER_CONTROLLER_GET_PRIVATE(controller);
    priv->state = MILTER_MANAGER_CONTROLLER_STATE_CONNECT;

    if (!priv->children)
        return MILTER_STATUS_NOT_CHANGE;
    context = priv->children->data;
    return milter_server_context_connect(context, host_name,
                                         address, address_length);
}

MilterStatus
milter_manager_controller_helo (MilterManagerController *controller, const gchar *fqdn)
{
    MilterManagerControllerPrivate *priv;
    MilterServerContext *context;

    priv = MILTER_MANAGER_CONTROLLER_GET_PRIVATE(controller);
    priv->state = MILTER_MANAGER_CONTROLLER_STATE_HELO;

    if (!priv->children)
        return MILTER_STATUS_NOT_CHANGE;
    context = priv->children->data;
    return milter_server_context_helo(context, fqdn);
}

MilterStatus
milter_manager_controller_envelope_from (MilterManagerController *controller,
                                         const gchar *from)
{
    MilterManagerControllerPrivate *priv;
    MilterServerContext *context;

    priv = MILTER_MANAGER_CONTROLLER_GET_PRIVATE(controller);
    priv->state = MILTER_MANAGER_CONTROLLER_STATE_ENVELOPE_FROM;

    if (!priv->children)
        return MILTER_STATUS_NOT_CHANGE;
    context = priv->children->data;
    return milter_server_context_envelope_from(context, from);
}

MilterStatus
milter_manager_controller_envelope_receipt (MilterManagerController *controller,
                                            const gchar *receipt)
{
    MilterManagerControllerPrivate *priv;
    MilterServerContext *context;

    priv = MILTER_MANAGER_CONTROLLER_GET_PRIVATE(controller);
    priv->state = MILTER_MANAGER_CONTROLLER_STATE_ENVELOPE_RECEIPT;

    if (!priv->children)
        return MILTER_STATUS_NOT_CHANGE;
    context = priv->children->data;
    return milter_server_context_envelope_receipt(context, receipt);
}

MilterStatus
milter_manager_controller_data (MilterManagerController *controller)
{
    MilterManagerControllerPrivate *priv;
    MilterServerContext *context;

    priv = MILTER_MANAGER_CONTROLLER_GET_PRIVATE(controller);
    priv->state = MILTER_MANAGER_CONTROLLER_STATE_DATA;

    if (!priv->children)
        return MILTER_STATUS_NOT_CHANGE;
    context = priv->children->data;
    return milter_server_context_data(context);
}

MilterStatus
milter_manager_controller_unknown (MilterManagerController *controller, const gchar *command)
{
    g_print("unknown");
    return MILTER_STATUS_NOT_CHANGE;
}

MilterStatus
milter_manager_controller_header (MilterManagerController *controller,
        const gchar *name, const gchar *value)
{
    g_print("header");
    return MILTER_STATUS_NOT_CHANGE;
}

MilterStatus
milter_manager_controller_end_of_header (MilterManagerController *controller)
{
    g_print("end-of-header");
    return MILTER_STATUS_NOT_CHANGE;
}

MilterStatus
milter_manager_controller_body (MilterManagerController *controller, const guchar *chunk, gsize size)
{
    g_print("body");
    return MILTER_STATUS_NOT_CHANGE;
}

MilterStatus
milter_manager_controller_end_of_message (MilterManagerController *controller)
{
    g_print("end-of-body");
    return MILTER_STATUS_NOT_CHANGE;
}

MilterStatus
milter_manager_controller_quit (MilterManagerController *controller)
{
    g_print("quit");
    return MILTER_STATUS_NOT_CHANGE;
}

MilterStatus
milter_manager_controller_abort (MilterManagerController *controller)
{
    g_print("abort");
    return MILTER_STATUS_NOT_CHANGE;
}

void
milter_manager_controller_mta_timeout (MilterManagerController *controller)
{
    g_print("mta_timeout");
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
