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
    MilterMacrosRequests *macros_requests;
    MilterOption *option;
};

enum
{
    PROP_0
};

MILTER_DEFINE_REPLY_SIGNALS_TYPE(MilterManagerChildren, milter_manager_children, G_TYPE_OBJECT);

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

    gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->dispose      = dispose;
    gobject_class->set_property = set_property;
    gobject_class->get_property = get_property;

    g_type_class_add_private(gobject_class,
                             sizeof(MilterManagerChildrenPrivate));
}

static void
milter_manager_children_init (MilterManagerChildren *milter)
{
    MilterManagerChildrenPrivate *priv;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(milter);
    priv->milters = NULL;
    priv->macros_requests = milter_macros_requests_new();
    priv->option = NULL;
}

static void
dispose (GObject *object)
{
    MilterManagerChildrenPrivate *priv;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(object);

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
milter_manager_children_new (void)
{
    return g_object_new(MILTER_TYPE_MANAGER_CHILDREN, NULL);
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

    g_signal_emit_by_name(children, "negotiate-reply", option, macros_requests);
}

static void
cb_continue (MilterServerContext *context, gpointer user_data)
{
    MilterManagerChildren *children = user_data;
    g_signal_emit_by_name(children, "continue");
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
    g_signal_emit_by_name(children, "temporary-failure");
}

static void
cb_reject (MilterServerContext *context, gpointer user_data)
{
    MilterManagerChildren *children = user_data;
    g_signal_emit_by_name(children, "reject");
}

static void
cb_accept (MilterServerContext *context, gpointer user_data)
{
    MilterManagerChildren *children = user_data;
    g_signal_emit_by_name(children, "accept");
}

static void
cb_discard (MilterServerContext *context, gpointer user_data)
{
    MilterManagerChildren *children = user_data;
    g_signal_emit_by_name(children, "discard");
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

static void
cb_skip (MilterServerContext *context, gpointer user_data)
{
    MilterManagerChildren *children = user_data;

    g_signal_emit_by_name(children, "skip");
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
    /* MilterManagerChildren *children = user_data; */

    milter_error("error: FIXME: %s", error->message);
/*
    milter_error_emitable_emit_error(MILTER_ERROR_EMITABLE(children),
                                     error);
*/
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
    CONNECT(sending_timeout);
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
    DISCONNECT(sending_timeout);
    DISCONNECT(reading_timeout);
    DISCONNECT(end_of_message_timeout);

    DISCONNECT(error);
#undef DISCONNECT
}

static MilterStatus
child_negotiate (MilterManagerChild *child, MilterOption *option, 
                 MilterManagerChildren *children)
{
    MilterServerContext *context;
    GError *error = NULL;

    context = MILTER_SERVER_CONTEXT(child);

    if (!milter_server_context_establish_connection(context, &error)) {
        MilterStatus status;
        status = MILTER_STATUS_CONTINUE;
#if 0
        gboolean priviledge;
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
#endif
        milter_manager_child_start(child, &error);
        if (error) {
            milter_error("Error: %s", error->message);
            g_error_free(error);
            return status;
        }
        return MILTER_STATUS_PROGRESS;
    }

    setup_server_context_signals(children, context);
    milter_server_context_negotiate(context, option);

    return MILTER_STATUS_PROGRESS;
}

gboolean
milter_manager_children_negotiate (MilterManagerChildren *children,
                                   MilterOption          *option)
{
    GList *child;
    MilterManagerChildrenPrivate *priv;
    gboolean success = TRUE;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    for (child = priv->milters; child; child = g_list_next(child)) {
        success |= child_negotiate(MILTER_MANAGER_CHILD(child->data), option, children);
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

    for (child = priv->milters; child; child = g_list_next(child)) {
        success |= milter_server_context_connect(MILTER_SERVER_CONTEXT(child->data),
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

    for (child = priv->milters; child; child = g_list_next(child)) {
        success |= milter_server_context_helo(MILTER_SERVER_CONTEXT(child->data),
                                              fqdn);
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

    for (child = priv->milters; child; child = g_list_next(child)) {
        success |= milter_server_context_envelope_from(MILTER_SERVER_CONTEXT(child->data),
                                                       from);
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

    for (child = priv->milters; child; child = g_list_next(child)) {
        success |= milter_server_context_envelope_receipt(MILTER_SERVER_CONTEXT(child->data),
                                                          receipt);
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

    for (child = priv->milters; child; child = g_list_next(child)) {
        success |= milter_server_context_data(MILTER_SERVER_CONTEXT(child->data));
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

    for (child = priv->milters; child; child = g_list_next(child)) {
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

    for (child = priv->milters; child; child = g_list_next(child)) {
        success |= milter_server_context_header(MILTER_SERVER_CONTEXT(child->data),
                                                name, value);
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

    for (child = priv->milters; child; child = g_list_next(child)) {
        success |= milter_server_context_end_of_header(MILTER_SERVER_CONTEXT(child->data));
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

    for (child = priv->milters; child; child = g_list_next(child)) {
        success |= milter_server_context_body(MILTER_SERVER_CONTEXT(child->data),
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

    for (child = priv->milters; child; child = g_list_next(child)) {
        success |= milter_server_context_end_of_message(MILTER_SERVER_CONTEXT(child->data),
                                                        chunk, size);
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

    for (child = priv->milters; child; child = g_list_next(child)) {
        success |= milter_server_context_quit(MILTER_SERVER_CONTEXT(child->data));
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

    for (child = priv->milters; child; child = g_list_next(child)) {
        success |= milter_server_context_abort(MILTER_SERVER_CONTEXT(child->data));
    }

    return success;
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
