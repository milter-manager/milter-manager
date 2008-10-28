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

#include "milter-manager-children.h"
#include "milter-manager-configuration.h"

#define MILTER_MANAGER_CHILDREN_GET_PRIVATE(obj)                    \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj),                             \
                                 MILTER_TYPE_MANAGER_CHILDREN,      \
                                 MilterManagerChildrenPrivate))

typedef struct _MilterManagerChildrenPrivate	MilterManagerChildrenPrivate;
struct _MilterManagerChildrenPrivate
{
    GList *milters;
    GQueue *reply_queue;
    MilterManagerConfiguration *configuration;
    MilterMacrosRequests *macros_requests;
    MilterOption *option;
    MilterStatus reply_status;
};

enum
{
    PROP_0,
    PROP_CONFIGURATION
};

static MilterErrorEmitable *emitable_parent;
static MilterReplySignals *reply_parent;

static void
reply_init (MilterReplySignals *reply)
{
    reply_parent = g_type_interface_peek_parent(reply);
}

static void
emitable_init (MilterErrorEmitable *emitable)
{
    emitable_parent = g_type_interface_peek_parent(emitable);
}

G_DEFINE_TYPE_WITH_CODE(MilterManagerChildren, milter_manager_children, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(MILTER_TYPE_REPLY_SIGNALS, reply_init)
    G_IMPLEMENT_INTERFACE(MILTER_TYPE_ERROR_EMITABLE, emitable_init))

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
                           (MilterManagerChild *child,
                            gpointer user_data);

static void
milter_manager_children_class_init (MilterManagerChildrenClass *klass)
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

    g_type_class_add_private(gobject_class,
                             sizeof(MilterManagerChildrenPrivate));
}

static void
milter_manager_children_init (MilterManagerChildren *milter)
{
    MilterManagerChildrenPrivate *priv;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(milter);
    priv->configuration = NULL;
    priv->reply_queue = g_queue_new();
    priv->milters = NULL;
    priv->macros_requests = milter_macros_requests_new();
    priv->option = NULL;
    priv->reply_status = MILTER_STATUS_NOT_CHANGE;
}

static void
dispose (GObject *object)
{
    MilterManagerChildrenPrivate *priv;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(object);

    if (priv->reply_queue) {
        g_queue_free(priv->reply_queue);
        priv->reply_queue = NULL;
    }

    if (priv->configuration) {
        g_object_unref(priv->configuration);
        priv->configuration = NULL;
    }

    if (priv->milters) {
        g_list_foreach(priv->milters, (GFunc)teardown_server_context_signals, object);
        g_list_free(priv->milters);
        priv->milters = NULL;
    }

    if (priv->macros_requests) {
        g_object_unref(priv->macros_requests);
        priv->macros_requests = NULL;
    }

    if (priv->option) {
        g_object_unref(priv->option);
        priv->option = NULL;
    }

    G_OBJECT_CLASS(milter_manager_children_parent_class)->dispose(object);
}

static void
set_property (GObject      *object,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
    MilterManagerChildrenPrivate *priv;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(object);
    switch (prop_id) {
      case PROP_CONFIGURATION:
        if (priv->configuration)
            g_object_unref(priv->configuration);
        priv->configuration = g_value_get_object(value);
        if (priv->configuration)
            g_object_ref(priv->configuration);
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
    MilterManagerChildrenPrivate *priv;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(object);
    switch (prop_id) {
      case PROP_CONFIGURATION:
        g_value_set_object(value, priv->configuration);
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

GQuark
milter_manager_children_error_quark (void)
{
    return g_quark_from_static_string("milter-manager-children-error-quark");
}

MilterManagerChildren *
milter_manager_children_new (MilterManagerConfiguration *configuration)
{
    return g_object_new(MILTER_TYPE_MANAGER_CHILDREN,
                        "configuration", configuration,
                        NULL);
}

void
milter_manager_children_add_child (MilterManagerChildren *children, 
                                   MilterManagerChild *child)
{
    MilterManagerChildrenPrivate *priv;

    if (!child)
        return;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    priv->milters = g_list_prepend(priv->milters, g_object_ref(child));
}

GList *
milter_manager_children_get_children (MilterManagerChildren *children)
{
    return MILTER_MANAGER_CHILDREN_GET_PRIVATE(children)->milters;
}

void
milter_manager_children_foreach (MilterManagerChildren *children,
                                 GFunc func, gpointer user_data)
{
    GList *milters;

    milters = milter_manager_children_get_children(children);
    g_list_foreach(milters, func, user_data);
}

static const gchar *
status_to_signal_name (MilterStatus status)
{
    const gchar *signal_name;

    switch (status) {
      case MILTER_STATUS_CONTINUE:
        signal_name = "continue";
        break;
      case MILTER_STATUS_REJECT:
        signal_name = "reject";
        break;
      case MILTER_STATUS_DISCARD:
        signal_name = "discard";
        break;
      case MILTER_STATUS_ACCEPT:
        signal_name = "accept";
        break;
      case MILTER_STATUS_TEMPORARY_FAILURE:
        signal_name = "temporary-failure";
        break;
      case MILTER_STATUS_SKIP:
        signal_name = "skip";
        break;
      case MILTER_STATUS_PROGRESS:
        signal_name = "progress";
        break;
      default:
        signal_name = "continue";
        break;
    }

    return signal_name;
}

static gboolean
status_is_critical (MilterStatus status)
{
    switch (status) {
      case MILTER_STATUS_REJECT:
      case MILTER_STATUS_DISCARD:
      case MILTER_STATUS_TEMPORARY_FAILURE:
        return TRUE;
        break;
      default:
        break;
    }

    return FALSE;
}

static MilterStatus
compile_reply_status (MilterManagerChildren *children,
                      MilterStatus status)
{
    MilterManagerChildrenPrivate *priv;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    switch (status) {
      case MILTER_STATUS_REJECT:
        priv->reply_status = status;
        break;
      case MILTER_STATUS_DISCARD:
        if (priv->reply_status != MILTER_STATUS_REJECT)
            priv->reply_status = status;
        break;
      case MILTER_STATUS_TEMPORARY_FAILURE:
        if (!status_is_critical(priv->reply_status))
            priv->reply_status = status;
        break;
      case MILTER_STATUS_ACCEPT:
        if (!status_is_critical(priv->reply_status))
            priv->reply_status = status;
        break;
      default:
        break;
    }
    priv->reply_status = status; /* FIXME */

    return priv->reply_status;
}

static void
cb_negotiate_reply (MilterServerContext *context, MilterOption *option,
                    MilterMacrosRequests *macros_requests, gpointer user_data)
{
    MilterManagerChildren *children = user_data;
    MilterManagerChildrenPrivate *priv;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    if (macros_requests)
        milter_macros_requests_merge(priv->macros_requests, macros_requests);

    if (!priv->option)
        priv->option = milter_option_copy(option);
    else
        milter_option_merge(priv->option, option);

    g_queue_remove(priv->reply_queue, context);

    if (g_queue_is_empty(priv->reply_queue))
        g_signal_emit_by_name(children, "negotiate-reply",
                              priv->option, priv->macros_requests);
}

static void
remove_child_from_queue (MilterManagerChildren *children,
                         MilterServerContext *context)
{
    MilterManagerChildrenPrivate *priv;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    g_queue_remove(priv->reply_queue, context);

    if (g_queue_is_empty(priv->reply_queue))
        g_signal_emit_by_name(children, status_to_signal_name(priv->reply_status));
}

static void
expire_child (MilterManagerChildren *children,
              MilterServerContext *context)
{
    MilterManagerChildrenPrivate *priv;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    milter_server_context_quit(context);
    teardown_server_context_signals(MILTER_MANAGER_CHILD(context), children);
    priv->milters = g_list_remove(priv->milters, context);
}

static void
cb_continue (MilterServerContext *context, gpointer user_data)
{
    MilterManagerChildren *children = user_data;

    compile_reply_status(children, MILTER_STATUS_CONTINUE);
    remove_child_from_queue(children, context);
}

static void
cb_reply_code (MilterServerContext *context, const gchar *code,
               gpointer user_data)
{
    MilterManagerChildren *children = user_data;
    g_signal_emit_by_name(children, "reply-code", code);
}

static void
cb_temporary_failure (MilterServerContext *context, gpointer user_data)
{
    MilterManagerChildren *children = user_data;
    MilterServerContextState state;

    state = milter_server_context_get_state(context);

    switch (state) {
      case MILTER_SERVER_CONTEXT_STATE_CONNECT:
      case MILTER_SERVER_CONTEXT_STATE_HELO:
      case MILTER_SERVER_CONTEXT_STATE_QUIT:
        expire_child(children, context);
        break;
      default:
        compile_reply_status(children, MILTER_STATUS_TEMPORARY_FAILURE);
        break;
    }
    remove_child_from_queue(children, context);
}

static void
cb_reject (MilterServerContext *context, gpointer user_data)
{
    MilterManagerChildren *children = user_data;
    MilterServerContextState state;

    state = milter_server_context_get_state(context);

    switch (state) {
      case MILTER_SERVER_CONTEXT_STATE_ENVELOPE_RECEIPT:
        compile_reply_status(children, MILTER_STATUS_REJECT);
        break;
      default:
        expire_child(children, context);
        break;
    }

    remove_child_from_queue(children, context);
}

static void
cb_accept (MilterServerContext *context, gpointer user_data)
{
    MilterManagerChildren *children = user_data;

    expire_child(children, context);

    compile_reply_status(children, MILTER_STATUS_ACCEPT);
    remove_child_from_queue(children, context);
}

static void
cb_discard (MilterServerContext *context, gpointer user_data)
{
    MilterManagerChildren *children = user_data;

    expire_child(children, context);

    compile_reply_status(children, MILTER_STATUS_DISCARD);
    remove_child_from_queue(children, context);
}

static void
cb_add_header (MilterServerContext *context,
               const gchar *name, const gchar *value,
               gpointer user_data)
{
    MilterManagerChildren *children = user_data;

    g_signal_emit_by_name(children, "add-header", name, value);
}

static void
cb_insert_header (MilterServerContext *context,
                  guint32 index, const gchar *name, const gchar *value,
                  gpointer user_data)
{
    MilterManagerChildren *children = user_data;

    g_signal_emit_by_name(children, "insert-header", index, name, value);
}

static void
cb_change_header (MilterServerContext *context,
                  const gchar *name, guint32 index, const gchar *value,
                  gpointer user_data)
{
    MilterManagerChildren *children = user_data;

    g_signal_emit_by_name(children, "change-header", name, index, value);
}

static void
cb_change_from (MilterServerContext *context,
                const gchar *from, const gchar *parameters,
                gpointer user_data)
{
    MilterManagerChildren *children = user_data;

    g_signal_emit_by_name(children, "change-from", from, parameters);
}

static void
cb_add_receipt (MilterServerContext *context,
                const gchar *receipt, const gchar *parameters,
                gpointer user_data)
{
    MilterManagerChildren *children = user_data;

    g_signal_emit_by_name(children, "add-receipt", receipt, parameters);
}

static void
cb_delete_receipt (MilterServerContext *context,
                   const gchar *receipt,
                   gpointer user_data)
{
    MilterManagerChildren *children = user_data;

    g_signal_emit_by_name(children, "delete-receipt", receipt);
}

static void
cb_replace_body (MilterServerContext *context,
                 const gchar *chunk, gsize chunk_size,
                 gpointer user_data)
{
    MilterManagerChildren *children = user_data;

    g_signal_emit_by_name(children, "replace-body", chunk, chunk_size);
}

static void
cb_progress (MilterServerContext *context, gpointer user_data)
{
    MilterManagerChildren *children = user_data;

    g_signal_emit_by_name(children, "progress");
}

static void
cb_quarantine (MilterServerContext *context,
               const gchar *reason,
               gpointer user_data)
{
    MilterManagerChildren *children = user_data;

    g_signal_emit_by_name(children, "quarantine");
}

static void
cb_connection_failure (MilterServerContext *context, gpointer user_data)
{
    MilterManagerChildren *children = user_data;

    g_signal_emit_by_name(children, "connection-failure");
}

static void
cb_shutdown (MilterServerContext *context, gpointer user_data)
{
    MilterManagerChildren *children = user_data;

    g_signal_emit_by_name(children, "shutdown");
}

static gboolean
get_all_children_skip_body (MilterManagerChildren *children)
{
    MilterManagerChildrenPrivate *priv;
    GList *milter;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    for (milter = priv->milters; milter; milter = g_list_next(milter)) {
        MilterServerContext *context = MILTER_SERVER_CONTEXT(milter->data);

        if (!milter_server_context_get_skip_body(context))
            return FALSE;
    }

    return TRUE;
}

static void
cb_skip (MilterServerContext *context, gpointer user_data)
{
    MilterManagerChildren *children = user_data;
    MilterServerContextState state;

    state = milter_server_context_get_state(context);

    if (state != MILTER_SERVER_CONTEXT_STATE_BODY) {
        milter_error("SKIP reply is only allowed in body session");
        return;
    }
    if (get_all_children_skip_body(children))
        g_signal_emit_by_name(children, "skip");
}

static void
cb_connection_timeout (MilterServerContext *context, gpointer user_data)
{
    milter_error("connection timeout: FIXME");
}

static void
cb_writing_timeout (MilterServerContext *context, gpointer user_data)
{
    milter_error("writing timeout: FIXME");
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
    MilterManagerChildrenPrivate *priv;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(user_data);

    milter_error("error: FIXME: %s", error->message);
    g_queue_remove(priv->reply_queue, emitable);

    milter_error_emitable_emit_error(MILTER_ERROR_EMITABLE(user_data),
                                     error);
}

static void
setup_server_context_signals (MilterManagerChildren *children,
                              MilterServerContext *server_context)
{
#define CONNECT(name)                                           \
    g_signal_connect(server_context, #name,                     \
                     G_CALLBACK(cb_ ## name), children)

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
    CONNECT(writing_timeout);
    CONNECT(reading_timeout);
    CONNECT(end_of_message_timeout);

    CONNECT(error);
#undef CONNECT
}

static void
teardown_server_context_signals (MilterManagerChild *child,
                                 gpointer user_data)
{
    MilterManagerChildren *children = user_data;

#define DISCONNECT(name)                                                \
    g_signal_handlers_disconnect_by_func(child,                         \
                                         G_CALLBACK(cb_ ## name),       \
                                         children)

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
    DISCONNECT(writing_timeout);
    DISCONNECT(reading_timeout);
    DISCONNECT(end_of_message_timeout);

    DISCONNECT(error);
#undef DISCONNECT
}

static MilterStatus
child_negotiate (MilterManagerChild *child, MilterOption *option, 
                 MilterManagerChildren *children)
{
    MilterManagerChildrenPrivate *priv;
    MilterServerContext *context;
    GError *error = NULL;

    context = MILTER_SERVER_CONTEXT(child);
    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    if (!milter_server_context_establish_connection(context, &error)) {
        MilterStatus status;
        status = MILTER_STATUS_CONTINUE;
        gboolean priviledge;

        status =
            milter_manager_configuration_get_return_status_if_filter_unavailable(priv->configuration);

        priviledge =
            milter_manager_configuration_is_privilege_mode(priv->configuration);
        if (!priviledge ||
            error->code != MILTER_SERVER_CONTEXT_ERROR_CONNECTION_FAILURE) {
            milter_error("Error: %s", error->message);
            milter_error_emitable_emit_error(MILTER_ERROR_EMITABLE(children),
                                             error);
            g_error_free(error);
            return status;
        }
        g_error_free(error);

        milter_manager_child_start(child, &error);
        if (error) {
            milter_error("Error: %s", error->message);
            milter_error_emitable_emit_error(MILTER_ERROR_EMITABLE(children),
                                             error);
            g_error_free(error);
            return status;
        }
        return MILTER_STATUS_PROGRESS;
    }

    g_queue_push_tail(priv->reply_queue, context);
    setup_server_context_signals(children, context);
    milter_server_context_negotiate(context, option);

    return MILTER_STATUS_PROGRESS;
}

static void
init_reply_queue (MilterManagerChildren *children)
{
    MilterManagerChildrenPrivate *priv;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    g_queue_clear(priv->reply_queue);
    priv->reply_status = MILTER_STATUS_CONTINUE;
}

gboolean
milter_manager_children_negotiate (MilterManagerChildren *children,
                                   MilterOption          *option)
{
    GList *child;
    MilterManagerChildrenPrivate *priv;
    gboolean success = TRUE;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    init_reply_queue(children);
    for (child = priv->milters; child; child = g_list_next(child)) {
        MilterStatus status;
        status = child_negotiate(MILTER_MANAGER_CHILD(child->data), option, children);

        if (status != MILTER_STATUS_PROGRESS &&
            status != MILTER_STATUS_CONTINUE) {
            success = FALSE;
        }
    }

    return success;
}

gboolean
milter_manager_children_connect (MilterManagerChildren *children,
                                 const gchar           *host_name,
                                 struct sockaddr       *address,
                                 socklen_t              address_length)
{
    GList *child;
    MilterManagerChildrenPrivate *priv;
    gboolean success = TRUE;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    init_reply_queue(children);
    for (child = priv->milters; child; child = g_list_next(child)) {
        MilterServerContext *context = MILTER_SERVER_CONTEXT(child->data);

        if (milter_server_context_is_enable_step(context, MILTER_STEP_NO_CONNECT))
            continue;

        g_queue_push_tail(priv->reply_queue, context);
        success |= milter_server_context_connect(context,
                                                 host_name,
                                                 address,
                                                 address_length);
    }

    return success;
}

gboolean
milter_manager_children_helo (MilterManagerChildren *children,
                              const gchar           *fqdn)
{
    GList *child;
    MilterManagerChildrenPrivate *priv;
    gboolean success = TRUE;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    init_reply_queue(children);

    for (child = priv->milters; child; child = g_list_next(child)) {
        MilterServerContext *context = MILTER_SERVER_CONTEXT(child->data);

        if (milter_server_context_is_enable_step(context, MILTER_STEP_NO_HELO))
            continue;

        g_queue_push_tail(priv->reply_queue, context);
        success |= milter_server_context_helo(context, fqdn);
    }

    return success;
}

gboolean
milter_manager_children_envelope_from (MilterManagerChildren *children,
                                       const gchar           *from)
{
    GList *child;
    MilterManagerChildrenPrivate *priv;
    gboolean success = TRUE;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    init_reply_queue(children);
    for (child = priv->milters; child; child = g_list_next(child)) {
        MilterServerContext *context = MILTER_SERVER_CONTEXT(child->data);

        if (milter_server_context_is_enable_step(context, MILTER_STEP_NO_MAIL))
            continue;

        g_queue_push_tail(priv->reply_queue, context);
        success |= milter_server_context_envelope_from(context, from);
    }

    return success;
}

gboolean
milter_manager_children_envelope_receipt (MilterManagerChildren *children,
                                          const gchar           *receipt)
{
    GList *child;
    MilterManagerChildrenPrivate *priv;
    gboolean success = TRUE;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    init_reply_queue(children);
    for (child = priv->milters; child; child = g_list_next(child)) {
        MilterServerContext *context = MILTER_SERVER_CONTEXT(child->data);

        if (milter_server_context_is_enable_step(context, MILTER_STEP_NO_RCPT))
            continue;

        g_queue_push_tail(priv->reply_queue, context);
        success |= milter_server_context_envelope_receipt(context, receipt);
    }

    return success;
}

gboolean
milter_manager_children_data (MilterManagerChildren *children)
{
    GList *child;
    MilterManagerChildrenPrivate *priv;
    gboolean success = TRUE;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    init_reply_queue(children);
    for (child = priv->milters; child; child = g_list_next(child)) {
        MilterServerContext *context = MILTER_SERVER_CONTEXT(child->data);

        if (milter_server_context_is_enable_step(context, MILTER_STEP_NO_DATA))
            continue;

        g_queue_push_tail(priv->reply_queue, context);
        success |= milter_server_context_data(context);
    }

    return success;
}

gboolean
milter_manager_children_unknown (MilterManagerChildren *children,
                                 const gchar           *command)
{
    GList *child;
    MilterManagerChildrenPrivate *priv;
    gboolean success = TRUE;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    init_reply_queue(children);
    for (child = priv->milters; child; child = g_list_next(child)) {
        MilterServerContext *context = MILTER_SERVER_CONTEXT(child->data);

        if (milter_server_context_is_enable_step(context, MILTER_STEP_NO_UNKNOWN))
            continue;

        g_queue_push_tail(priv->reply_queue, context);
        success |= milter_server_context_unknown(MILTER_SERVER_CONTEXT(child->data),
                                                 command);
    }

    return success;
}

gboolean
milter_manager_children_header (MilterManagerChildren *children,
                                const gchar           *name,
                                const gchar           *value)
{
    GList *child;
    MilterManagerChildrenPrivate *priv;
    gboolean success = TRUE;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    init_reply_queue(children);
    for (child = priv->milters; child; child = g_list_next(child)) {
        MilterServerContext *context = MILTER_SERVER_CONTEXT(child->data);

        if (milter_server_context_is_enable_step(context, MILTER_STEP_NO_HEADERS))
            continue;

        g_queue_push_tail(priv->reply_queue, context);
        success |= milter_server_context_header(context, name, value);
    }

    return success;
}

gboolean
milter_manager_children_end_of_header (MilterManagerChildren *children)
{
    GList *child;
    MilterManagerChildrenPrivate *priv;
    gboolean success = TRUE;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    init_reply_queue(children);
    for (child = priv->milters; child; child = g_list_next(child)) {
        MilterServerContext *context = MILTER_SERVER_CONTEXT(child->data);

        if (milter_server_context_is_enable_step(context, MILTER_STEP_NO_END_OF_HEADER))
            continue;

        g_queue_push_tail(priv->reply_queue, context);
        success |= milter_server_context_end_of_header(context);
    }

    return success;
}

gboolean
milter_manager_children_body (MilterManagerChildren *children,
                              const gchar           *chunk,
                              gsize                  size)
{
    GList *child;
    MilterManagerChildrenPrivate *priv;
    gboolean success = TRUE;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    init_reply_queue(children);
    for (child = priv->milters; child; child = g_list_next(child)) {
        MilterServerContext *context = MILTER_SERVER_CONTEXT(child->data);

        if (milter_server_context_is_enable_step(context, MILTER_STEP_NO_BODY))
            continue;

        if (milter_server_context_get_skip_body(context))
            continue;

        g_queue_push_tail(priv->reply_queue, context);
        success |= milter_server_context_body(context,
                                              chunk, size);
    }

    return success;
}

gboolean
milter_manager_children_end_of_message (MilterManagerChildren *children,
                                        const gchar           *chunk,
                                        gsize                  size)
{
    GList *child;
    MilterManagerChildrenPrivate *priv;
    gboolean success = TRUE;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    init_reply_queue(children);
    for (child = priv->milters; child; child = g_list_next(child)) {
        MilterServerContext *context = MILTER_SERVER_CONTEXT(child->data);

        g_queue_push_tail(priv->reply_queue, context);
        success |= milter_server_context_end_of_message(context, chunk, size);
    }

    return success;
}

gboolean
milter_manager_children_quit (MilterManagerChildren *children)
{
    GList *child;
    MilterManagerChildrenPrivate *priv;
    gboolean success = TRUE;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    init_reply_queue(children);
    for (child = priv->milters; child; child = g_list_next(child)) {
        MilterServerContext *context = MILTER_SERVER_CONTEXT(child->data);
        g_queue_push_tail(priv->reply_queue, context);
        success |= milter_server_context_quit(context);
    }

    return success;
}

gboolean
milter_manager_children_abort (MilterManagerChildren *children)
{
    GList *child;
    MilterManagerChildrenPrivate *priv;
    gboolean success = TRUE;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    init_reply_queue(children);
    for (child = priv->milters; child; child = g_list_next(child)) {
        MilterServerContext *context = MILTER_SERVER_CONTEXT(child->data);
        g_queue_push_tail(priv->reply_queue, context);
        success |= milter_server_context_abort(context);
    }

    return success;
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
