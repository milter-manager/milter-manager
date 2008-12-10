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

#include <glib/gstdio.h>
#include "milter-manager-configuration.h"
#include "milter/core.h"

#define MILTER_MANAGER_CHILDREN_GET_PRIVATE(obj)                    \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj),                             \
                                 MILTER_TYPE_MANAGER_CHILDREN,      \
                                 MilterManagerChildrenPrivate))

typedef struct _MilterManagerChildrenPrivate	MilterManagerChildrenPrivate;
struct _MilterManagerChildrenPrivate
{
    GList *milters;
    GList *quitted_milters;
    GQueue *reply_queue;
    GList *command_waiting_child_queue; /* storing child milters which is waiting for commands after DATA command */
    GList *command_queue; /* storing commands after DATA command */
    GHashTable *try_negotiate_ids;
    MilterManagerConfiguration *configuration;
    MilterMacrosRequests *macros_requests;
    MilterOption *option;
    MilterServerContextState current_state;
    GHashTable *reply_statuses;
    guint reply_code;
    gchar *reply_extended_code;
    gchar *reply_message;
    gdouble retry_connect_time;

    MilterHeaders *original_headers;
    MilterHeaders *headers;
    gint processing_header_index;
    GIOChannel *body_file;
    gchar *body_file_name;
    gchar *end_of_message_chunk;
    gsize end_of_message_size;
    gboolean sent_end_of_message;
    guint sent_body_count;
    gboolean replaced_body;
};

typedef struct _NegotiateData NegotiateData;
struct _NegotiateData
{
    MilterManagerChildren *children;
    MilterManagerChild *child;
    MilterOption *option;
    guint error_signal_id;
    guint ready_signal_id;
    guint connection_timeout_signal_id;
    gboolean is_retry;
};

enum
{
    PROP_0,
    PROP_CONFIGURATION
};

MILTER_IMPLEMENT_ERROR_EMITTABLE(error_emittable_init);
MILTER_IMPLEMENT_FINISHED_EMITTABLE(finished_emittable_init);
MILTER_IMPLEMENT_REPLY_SIGNALS(reply_init);
G_DEFINE_TYPE_WITH_CODE(MilterManagerChildren, milter_manager_children, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(MILTER_TYPE_ERROR_EMITTABLE, error_emittable_init)
    G_IMPLEMENT_INTERFACE(MILTER_TYPE_FINISHED_EMITTABLE, finished_emittable_init)
    G_IMPLEMENT_INTERFACE(MILTER_TYPE_REPLY_SIGNALS, reply_init))

static void dispose        (GObject         *object);
static void set_property   (GObject         *object,
                            guint            prop_id,
                            const GValue    *value,
                            GParamSpec      *pspec);
static void get_property   (GObject         *object,
                            guint            prop_id,
                            GValue          *value,
                            GParamSpec      *pspec);

static void setup_server_context_signals
                           (MilterManagerChildren *children,
                            MilterServerContext *server_context);

static void teardown_server_context_signals
                           (MilterManagerChild *child,
                            gpointer user_data);

static gboolean child_establish_connection
                           (MilterManagerChild *child,
                            MilterOption *option,
                            MilterManagerChildren *children,
                            gboolean is_retry);
static void prepare_retry_establish_connection
                           (MilterManagerChild *child,
                            MilterOption *option,
                            MilterManagerChildren *children,
                            gboolean is_retry);
static void remove_queue_in_negotiate
                           (MilterManagerChildren *children,
                            MilterManagerChild *child);
static MilterServerContext *get_first_child_in_command_waiting_child_queue
                           (MilterManagerChildren *children);
static gboolean write_to_body_file
                           (MilterManagerChildren *children,
                            const gchar *chunk,
                            gsize size);
static gboolean send_body_to_child
                           (MilterManagerChildren *children,
                            MilterServerContext *context);
static gboolean send_next_header_to_child
                           (MilterManagerChildren *children,
                            MilterServerContext *context);
static gboolean send_next_command
                           (MilterManagerChildren *children,
                            MilterServerContext *context,
                            MilterServerContextState current_state);

static NegotiateData *negotiate_data_new  (MilterManagerChildren *children,
                                           MilterManagerChild *child,
                                           MilterOption *option,
                                           gboolean is_retry);
static void           negotiate_data_free (NegotiateData *data);
static void           negotiate_data_hash_key_free (gpointer data);

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
remove_retry_negotiate_timeout (gpointer data)
{
    guint timeout_id;

    if (!data)
        return;

    timeout_id = GPOINTER_TO_UINT(data);

    g_source_remove(timeout_id);
}

static void
milter_manager_children_init (MilterManagerChildren *milter)
{
    MilterManagerChildrenPrivate *priv;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(milter);
    priv->configuration = NULL;
    priv->reply_queue = g_queue_new();
    priv->command_waiting_child_queue = NULL;
    priv->command_queue = NULL;
    priv->try_negotiate_ids =
        g_hash_table_new_full(g_direct_hash, g_direct_equal,
                              negotiate_data_hash_key_free,
                              remove_retry_negotiate_timeout);
    priv->milters = NULL;
    priv->quitted_milters = NULL;
    priv->macros_requests = milter_macros_requests_new();
    priv->option = NULL;
    priv->reply_statuses = g_hash_table_new(g_direct_hash, g_direct_equal);

    priv->original_headers = NULL;
    priv->headers = milter_headers_new();
    priv->processing_header_index = 0;
    priv->body_file = NULL;
    priv->body_file_name = NULL;
    priv->end_of_message_chunk = NULL;
    priv->end_of_message_size = 0;
    priv->sent_end_of_message = FALSE;
    priv->sent_body_count = 0;
    priv->replaced_body = FALSE;

    priv->current_state = MILTER_SERVER_CONTEXT_STATE_START;

    priv->reply_code = 0;
    priv->reply_extended_code = NULL;
    priv->reply_message = NULL;

    priv->retry_connect_time = 5.0;
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

    if (priv->command_waiting_child_queue) {
        g_list_free(priv->command_waiting_child_queue);
        priv->command_waiting_child_queue = NULL;
    }

    if (priv->command_queue) {
        g_list_free(priv->command_queue);
        priv->command_queue = NULL;
    }

    if (priv->try_negotiate_ids) {
        g_hash_table_unref(priv->try_negotiate_ids);
        priv->try_negotiate_ids = NULL;
    }

    if (priv->configuration) {
        g_object_unref(priv->configuration);
        priv->configuration = NULL;
    }

    if (priv->milters) {
        g_list_foreach(priv->milters,
                       (GFunc)teardown_server_context_signals, object);
        g_list_foreach(priv->milters, (GFunc)g_object_unref, NULL);
        g_list_free(priv->milters);
        priv->milters = NULL;
    }

    if (priv->quitted_milters) {
        g_list_foreach(priv->quitted_milters, (GFunc)g_object_unref, NULL);
        g_list_free(priv->quitted_milters);
        priv->quitted_milters = NULL;
    }

    if (priv->macros_requests) {
        g_object_unref(priv->macros_requests);
        priv->macros_requests = NULL;
    }

    if (priv->option) {
        g_object_unref(priv->option);
        priv->option = NULL;
    }

    if (priv->reply_statuses) {
        g_hash_table_unref(priv->reply_statuses);
        priv->reply_statuses = NULL;
    }

    if (priv->original_headers) {
        g_object_unref(priv->original_headers);
        priv->original_headers = NULL;
    }

    if (priv->headers) {
        g_object_unref(priv->headers);
        priv->headers = NULL;
    }

    if (priv->body_file) {
        g_io_channel_unref(priv->body_file);
        priv->body_file = NULL;
    }

    if (priv->body_file_name) {
        g_unlink(priv->body_file_name);
        g_free(priv->body_file_name);
        priv->body_file_name = NULL;
    }

    if (priv->end_of_message_chunk) {
        g_free(priv->end_of_message_chunk);
        priv->end_of_message_chunk = NULL;
    }

    if (priv->reply_extended_code) {
        g_free(priv->reply_extended_code);
        priv->reply_extended_code = NULL;
    }
    if (priv->reply_message) {
        g_free(priv->reply_message);
        priv->reply_message = NULL;
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

    priv->milters = g_list_append(priv->milters, g_object_ref(child));
}

guint
milter_manager_children_length (MilterManagerChildren *children)
{
    return g_list_length(MILTER_MANAGER_CHILDREN_GET_PRIVATE(children)->milters);
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

    milters = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children)->milters;
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

static MilterStatus
get_reply_status_for_state (MilterManagerChildren *children,
                            MilterServerContextState state)
{
    MilterManagerChildrenPrivate *priv;
    gpointer value;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    value = g_hash_table_lookup(priv->reply_statuses,
                                GINT_TO_POINTER(state));
    if (!value)
        return MILTER_STATUS_NOT_CHANGE;

    return (MilterStatus)(GPOINTER_TO_INT(value));
}

static void
compile_reply_status (MilterManagerChildren *children,
                      MilterServerContextState state,
                      MilterStatus status)
{
    MilterManagerChildrenPrivate *priv;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    if (milter_manager_children_is_important_status(children, state, status)) {
        g_hash_table_insert(priv->reply_statuses,
                            GINT_TO_POINTER(state),
                            GINT_TO_POINTER(status));
    }
}

static void
cb_ready (MilterServerContext *context, gpointer user_data)
{
    NegotiateData *negotiate_data = (NegotiateData*)user_data;
    MilterManagerChildrenPrivate *priv;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(negotiate_data->children);

    milter_statistics("Start filter process of %s(%p)",
                      milter_server_context_get_name(context),
                      context);
    setup_server_context_signals(negotiate_data->children, context);
    milter_server_context_negotiate(context, negotiate_data->option);
    g_hash_table_remove(priv->try_negotiate_ids, negotiate_data);
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

    remove_queue_in_negotiate(children, MILTER_MANAGER_CHILD(context));
}

static void
expire_child (MilterManagerChildren *children,
              MilterServerContext *context)
{
    MilterManagerChildrenPrivate *priv;
    const gchar *child_name;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    child_name = milter_server_context_get_name(context);
    milter_statistics("End of filter process of %s(%p)", child_name, context);

    teardown_server_context_signals(MILTER_MANAGER_CHILD(context), children);
    priv->milters = g_list_remove(priv->milters, context);
    priv->quitted_milters = g_list_prepend(priv->quitted_milters, context);

    if (!priv->milters)
        milter_finished_emittable_emit(MILTER_FINISHED_EMITTABLE(children));
}

static void
expire_all_children (MilterManagerChildren *children)
{
    MilterManagerChildrenPrivate *priv;
    GList *node;
    GList *milters_copy;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    milter_manager_children_abort(children);
    milter_manager_children_quit(children);

    milters_copy = g_list_copy(priv->milters);
    for (node = milters_copy; node; node = g_list_next(node)) {
        MilterServerContext *context = node->data;
        expire_child(children, context);
    }
    g_list_free(milters_copy);
}

static void
remove_child_from_queue (MilterManagerChildren *children,
                         MilterServerContext *context)
{
    MilterManagerChildrenPrivate *priv;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    g_queue_remove(priv->reply_queue, context);

    if (g_queue_is_empty(priv->reply_queue)) {
        MilterStatus status;
        status = get_reply_status_for_state(children, priv->current_state);

        if (priv->reply_code > 0) {
            g_signal_emit_by_name(children, "reply-code",
                                  priv->reply_code, priv->reply_extended_code,
                                  priv->reply_message);
        } else {
            if (status != MILTER_STATUS_NOT_CHANGE &&
                priv->current_state != MILTER_SERVER_CONTEXT_STATE_BODY) {
                g_signal_emit_by_name(children,
                                      status_to_signal_name(status));
            }
        }

        switch (status) {
          case MILTER_STATUS_REJECT:
            /* FIXME: children should have the current state. */
            if (milter_server_context_get_state(context) !=
                MILTER_SERVER_CONTEXT_STATE_ENVELOPE_RECIPIENT) {
                expire_all_children(children);
            }
            break;
          case MILTER_STATUS_DISCARD:
            expire_all_children(children);
            break;
          default:
            break;
        }
    }
}

static void
emit_reply_status_of_state (MilterManagerChildren *children,
                            MilterServerContextState state)
{
    MilterStatus status;

    status = get_reply_status_for_state(children, state);
    g_signal_emit_by_name(children, status_to_signal_name(status));
}

static gboolean
emit_replace_body_signal (MilterManagerChildren *children)
{
    MilterManagerChildrenPrivate *priv;
    GError *error = NULL;
    GIOStatus status = G_IO_STATUS_NORMAL;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    g_io_channel_seek_position(priv->body_file, 0, G_SEEK_SET, &error);
    if (error) {
        milter_error_emittable_emit(MILTER_ERROR_EMITTABLE(children),
                error);
        g_error_free(error);

        return FALSE;
    }

    while (status == G_IO_STATUS_NORMAL) {
        gchar buffer[MILTER_CHUNK_SIZE + 1];
        gsize read_size;

        status = g_io_channel_read_chars(priv->body_file, buffer, MILTER_CHUNK_SIZE,
                                         &read_size, &error);
    
        if (status == G_IO_STATUS_NORMAL)
            g_signal_emit_by_name(children, "replace-body", buffer, read_size);
    }

    if (error) {
        milter_error_emittable_emit(MILTER_ERROR_EMITTABLE(children),
                error);
        g_error_free(error);

        return FALSE;
    }

    return TRUE;
}

static MilterStepFlags
command_to_no_step_flag (MilterCommand command)
{
    switch (command) {
      case MILTER_COMMAND_CONNECT:
        return MILTER_STEP_NO_CONNECT;
        break;
      case MILTER_COMMAND_HELO:
        return MILTER_STEP_NO_HELO;
        break;
      case MILTER_COMMAND_ENVELOPE_FROM:
        return MILTER_STEP_NO_ENVELOPE_FROM;
        break;
      case MILTER_COMMAND_ENVELOPE_RECIPIENT:
        return MILTER_STEP_NO_ENVELOPE_RECIPIENT;
        break;
      case MILTER_COMMAND_DATA:
        return MILTER_STEP_NO_DATA;
        break;
      case MILTER_COMMAND_HEADER:
        return MILTER_STEP_NO_HEADERS;
        break;
      case MILTER_COMMAND_END_OF_HEADER:
        return MILTER_STEP_NO_END_OF_HEADER;
        break;
      case MILTER_COMMAND_BODY:
        return MILTER_STEP_NO_BODY;
        break;
      case MILTER_COMMAND_UNKNOWN:
        return MILTER_STEP_NO_UNKNOWN;
        break;
      case MILTER_COMMAND_END_OF_MESSAGE:
      default:
        return 0;
        break;
    }
    return 0;
}

static gboolean
send_command_to_child (MilterManagerChildren *children,
                       MilterServerContext *child,
                       MilterCommand command)
{
    switch (command) {
      case MILTER_COMMAND_DATA:
        return milter_server_context_data(child);
        break;
      case MILTER_COMMAND_HEADER:
        return send_next_header_to_child(children, child);
        break;
      case MILTER_COMMAND_END_OF_HEADER:
        return milter_server_context_end_of_header(child);
        break;
      case MILTER_COMMAND_BODY:
        return send_body_to_child(children, child);
        break;
      case MILTER_COMMAND_END_OF_MESSAGE:
      {
        MilterManagerChildrenPrivate *priv;
        priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

        priv->processing_header_index = 0;
        return milter_server_context_end_of_message(child,
                                                    priv->end_of_message_chunk,
                                                    priv->end_of_message_size);
        break;
      }
      default:
        break;
    }
    return FALSE;
}

static void
emit_signals_on_end_of_message (MilterManagerChildren *children)
{
    MilterManagerChildrenPrivate *priv;
    const GList *header_list, *node;
    MilterHeaders *processing_headers;
    gint i;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);
    if (priv->replaced_body)
        emit_replace_body_signal(children);

    processing_headers = milter_headers_copy(priv->original_headers);
    header_list = milter_headers_get_list(priv->headers);

    /* FIXME thes following process admits of improvement. */
    for (node = header_list, i = 0;
         node;
         node = g_list_next(node), i++) {
        MilterHeader *header = node->data;
        MilterHeader *found_header;
        gint index;

        if (milter_headers_find(processing_headers, header)) {
            milter_headers_remove(processing_headers, header);
            continue;
        }

        found_header = milter_headers_lookup_by_name(processing_headers, header->name);
        if (!found_header) {
            g_signal_emit_by_name(children, "insert-header",
                                  i, header->name, header->value);
            continue;
        }
        index = milter_headers_index_in_same_header_name(priv->original_headers,
                                                         found_header);
        if (index == -1) {
            g_signal_emit_by_name(children, "add-header",
                                  header->name, header->value);
        } else {
            g_signal_emit_by_name(children, "change-header",
                                  header->name, index, header->value);
            milter_headers_remove(processing_headers, found_header);
        }
    }

    if (milter_headers_length(processing_headers) > 0) {
        header_list = milter_headers_get_list(processing_headers);

        for (node = header_list; node; node = g_list_next(node)) {
            MilterHeader *header = node->data;
            gint index;

            index = milter_headers_index_in_same_header_name(priv->original_headers,
                                                             header);
            g_signal_emit_by_name(children, "delete-header",
                                  header->name, index);
        }
    }

    g_object_unref(processing_headers);
}

static MilterCommand
fetch_first_command_for_child_in_queue (MilterServerContext *child,
                                        GList **queue)
{
    GList *node;

    for (node = *queue; node; node = g_list_next(node)) {
        MilterCommand next_command;
        MilterStepFlags step;

        next_command = GPOINTER_TO_INT(node->data);
        step = command_to_no_step_flag(next_command);

        if (!milter_server_context_is_enable_step(child, step))
            return next_command;
    }

    return -1;
}

static void
send_first_command_to_next_child (MilterManagerChildren *children,
                                  MilterServerContext *context,
                                  MilterServerContextState current_state)
{
    MilterManagerChildrenPrivate *priv;
    MilterServerContext *next_child;
    MilterCommand first_command;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    priv->sent_end_of_message = TRUE;
    priv->processing_header_index = 0;
    priv->command_waiting_child_queue = 
        g_list_remove(priv->command_waiting_child_queue, context);

    next_child = get_first_child_in_command_waiting_child_queue(children);
    if (!next_child) {
        if (current_state == MILTER_SERVER_CONTEXT_STATE_END_OF_MESSAGE)
            emit_signals_on_end_of_message(children);

        emit_reply_status_of_state(children, current_state);
        return;
    }
    milter_server_context_quit(context);

    first_command = fetch_first_command_for_child_in_queue(next_child, 
                                                           &priv->command_queue);
    if (first_command == -1) 
        g_signal_emit_by_name(children, "continue");
    else
        send_command_to_child(children, next_child, first_command);
}

static void
cb_continue (MilterServerContext *context, gpointer user_data)
{
    MilterManagerChildren *children = user_data;
    MilterServerContextState state;
    MilterManagerChildrenPrivate *priv;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    state = milter_server_context_get_state(context);

    compile_reply_status(children, state, MILTER_STATUS_CONTINUE);
    switch (state) {
      case MILTER_SERVER_CONTEXT_STATE_DATA:
      case MILTER_SERVER_CONTEXT_STATE_END_OF_HEADER:
        send_next_command(children, context, state);
        break;
      case MILTER_SERVER_CONTEXT_STATE_HEADER:
        if (!priv->sent_end_of_message) {
            g_signal_emit_by_name(children, "continue");
            return;
        }
        if (priv->processing_header_index <
            milter_headers_length(priv->headers)) {
            send_next_header_to_child(children, context);
        } else {
            send_next_command(children, context, state);
        }
        break;
      case MILTER_SERVER_CONTEXT_STATE_BODY:
        priv->sent_body_count--;
        if (!priv->sent_end_of_message) {
            g_signal_emit_by_name(children, "continue");
            return;
        }
        if (priv->sent_body_count == 0)
            send_next_command(children, context, state);
        break;
      case MILTER_SERVER_CONTEXT_STATE_END_OF_MESSAGE:
        send_first_command_to_next_child(children, context, state);
        break;
      default:
        remove_child_from_queue(children, context);
        break;
    }
}

static void
cb_temporary_failure (MilterServerContext *context, gpointer user_data)
{
    MilterManagerChildren *children = user_data;
    MilterServerContextState state;

    state = milter_server_context_get_state(context);

    compile_reply_status(children, state, MILTER_STATUS_TEMPORARY_FAILURE);
    switch (state) {
      case MILTER_SERVER_CONTEXT_STATE_ENVELOPE_RECIPIENT:
        remove_child_from_queue(children, context);
        break;
      case MILTER_SERVER_CONTEXT_STATE_END_OF_MESSAGE:
        g_signal_emit_by_name(children, "temporary-failure");
        expire_all_children(children);
        break;
      case MILTER_SERVER_CONTEXT_STATE_DATA:
      case MILTER_SERVER_CONTEXT_STATE_END_OF_HEADER:
      case MILTER_SERVER_CONTEXT_STATE_HEADER:
      case MILTER_SERVER_CONTEXT_STATE_BODY:
      default:
        milter_server_context_quit(context);
        break;
    }
}

static void
cb_reject (MilterServerContext *context, gpointer user_data)
{
    MilterManagerChildren *children = user_data;
    MilterServerContextState state;

    state = milter_server_context_get_state(context);

    compile_reply_status(children, state, MILTER_STATUS_REJECT);
    switch (state) {
      case MILTER_SERVER_CONTEXT_STATE_ENVELOPE_RECIPIENT:
        remove_child_from_queue(children, context);
        break;
      case MILTER_SERVER_CONTEXT_STATE_END_OF_MESSAGE:
        g_signal_emit_by_name(children, "reject");
        expire_all_children(children);
        break;
      case MILTER_SERVER_CONTEXT_STATE_DATA:
      case MILTER_SERVER_CONTEXT_STATE_END_OF_HEADER:
      case MILTER_SERVER_CONTEXT_STATE_HEADER:
      case MILTER_SERVER_CONTEXT_STATE_BODY:
      default:
        milter_server_context_quit(context);
        break;
    }
}

static void
cb_reply_code (MilterServerContext *context,
               guint code,
               const gchar *extended_code,
               const gchar *message,
               gpointer user_data)
{
    MilterManagerChildren *children = user_data;
    MilterManagerChildrenPrivate *priv;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    priv->reply_code = code;
    if (priv->reply_extended_code)
        g_free(priv->reply_extended_code);
    priv->reply_extended_code = g_strdup(extended_code);
    if (priv->reply_message)
        g_free(priv->reply_message);
    priv->reply_message = g_strdup(message);

    cb_reject(context, user_data);
}

static void
cb_accept (MilterServerContext *context, gpointer user_data)
{
    MilterManagerChildren *children = user_data;
    MilterServerContextState state;

    state = milter_server_context_get_state(context);

    compile_reply_status(children, state, MILTER_STATUS_ACCEPT);
    switch (state) {
      case MILTER_SERVER_CONTEXT_STATE_DATA:
      case MILTER_SERVER_CONTEXT_STATE_HEADER:
      case MILTER_SERVER_CONTEXT_STATE_END_OF_HEADER:
      case MILTER_SERVER_CONTEXT_STATE_BODY:
      case MILTER_SERVER_CONTEXT_STATE_END_OF_MESSAGE:
        send_first_command_to_next_child(children, context, state);
        break;
      default:
        milter_server_context_quit(context);
        break;
    }
}

static void
cb_discard (MilterServerContext *context, gpointer user_data)
{
    MilterManagerChildren *children = user_data;
    MilterServerContextState state;

    state = milter_server_context_get_state(context);

    compile_reply_status(children, state, MILTER_STATUS_DISCARD);
    switch (state) {
      case MILTER_SERVER_CONTEXT_STATE_END_OF_MESSAGE:
        g_signal_emit_by_name(children, "discard");
        expire_all_children(children);
        break;
      case MILTER_SERVER_CONTEXT_STATE_DATA:
      case MILTER_SERVER_CONTEXT_STATE_HEADER:
      case MILTER_SERVER_CONTEXT_STATE_END_OF_HEADER:
      case MILTER_SERVER_CONTEXT_STATE_BODY:
      default:
        milter_server_context_quit(context);
        break;
    }
}

static void
cb_skip (MilterServerContext *context, gpointer user_data)
{
    MilterManagerChildren *children = user_data;
    MilterServerContextState state;
    MilterManagerChildrenPrivate *priv;

    state = milter_server_context_get_state(context);

    compile_reply_status(children, state, MILTER_STATUS_SKIP);

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);
    if (!priv->sent_end_of_message)
        g_signal_emit_by_name(children, "continue");
    else
        send_next_command(children, context, state);
}

static void
cb_add_header (MilterServerContext *context,
               const gchar *name, const gchar *value,
               gpointer user_data)
{
    MilterManagerChildren *children = user_data;
    MilterManagerChildrenPrivate *priv;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    milter_headers_add_header(priv->headers,
                              name, value);
}

static void
cb_insert_header (MilterServerContext *context,
                  guint32 index, const gchar *name, const gchar *value,
                  gpointer user_data)
{
    MilterManagerChildren *children = user_data;
    MilterManagerChildrenPrivate *priv;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    milter_headers_insert_header(priv->headers,
                                 index, name, value);
}

static void
cb_change_header (MilterServerContext *context,
                  const gchar *name, guint32 index, const gchar *value,
                  gpointer user_data)
{
    MilterManagerChildren *children = user_data;
    MilterManagerChildrenPrivate *priv;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    milter_headers_change_header(priv->headers,
                                 name, index, value);
}

static void
cb_delete_header (MilterServerContext *context,
                  const gchar *name, guint32 index,
                  gpointer user_data)
{
    MilterManagerChildren *children = user_data;
    MilterManagerChildrenPrivate *priv;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    milter_headers_delete_header(priv->headers, name, index);
}

static void
cb_change_from (MilterServerContext *context,
                const gchar *from, const gchar *parameters,
                gpointer user_data)
{
    g_signal_emit_by_name(user_data, "change-from", from, parameters);
}

static void
cb_add_recipient (MilterServerContext *context,
                  const gchar *recipient, const gchar *parameters,
                  gpointer user_data)
{
    g_signal_emit_by_name(user_data, "add-recipient", recipient, parameters);
}

static void
cb_delete_recipient (MilterServerContext *context,
                     const gchar *recipient,
                     gpointer user_data)
{
    g_signal_emit_by_name(user_data, "delete-recipient", recipient);
}

static void
cb_replace_body (MilterServerContext *context,
                 const gchar *chunk, gsize chunk_size,
                 gpointer user_data)
{
    MilterManagerChildren *children = user_data;
    MilterManagerChildrenPrivate *priv;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    if (!write_to_body_file(children, chunk, chunk_size))
        return;

    priv->replaced_body = TRUE;
}

static void
cb_progress (MilterServerContext *context, gpointer user_data)
{
    MilterServerContextState state;

    state = milter_server_context_get_state(context);

    if (state != MILTER_SERVER_CONTEXT_STATE_END_OF_MESSAGE) {
        milter_error("PROGRESS reply is only allowed in end of message session");
        return;
    }

    g_signal_emit_by_name(user_data, "progress");
}

static void
cb_quarantine (MilterServerContext *context,
               const gchar *reason,
               gpointer user_data)
{
    MilterManagerChildren *children = user_data;
    MilterServerContextState state;

    state = milter_server_context_get_state(context);

    if (state != MILTER_SERVER_CONTEXT_STATE_END_OF_MESSAGE) {
        MilterManagerChildrenPrivate *priv;

        priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

        milter_server_context_quit(context);
        milter_error("QUARANTINE reply is only allowed in end of message session");
        return;
    }

    g_signal_emit_by_name(children, "quarantine", reason);
}

static void
cb_connection_failure (MilterServerContext *context, gpointer user_data)
{
    g_signal_emit_by_name(user_data, "connection-failure");
}

static void
cb_shutdown (MilterServerContext *context, gpointer user_data)
{
    g_signal_emit_by_name(user_data, "shutdown");
}

static void
cb_writing_timeout (MilterServerContext *context, gpointer user_data)
{
    MilterManagerChildren *children = MILTER_MANAGER_CHILDREN(user_data);

    milter_error("writing to %s is timed out.",
                 milter_server_context_get_name(context));

    expire_child(children, context);
    remove_child_from_queue(children, context);
}

static void
cb_reading_timeout (MilterServerContext *context, gpointer user_data)
{
    MilterManagerChildren *children = MILTER_MANAGER_CHILDREN(user_data);

    milter_error("reading from %s is timed out.",
                 milter_server_context_get_name(context));

    expire_child(children, context);
    remove_child_from_queue(children, context);
}

static void
cb_end_of_message_timeout (MilterServerContext *context, gpointer user_data)
{
    MilterManagerChildren *children = MILTER_MANAGER_CHILDREN(user_data);

    milter_error("The response of end-of-message from %s is timed out.",
                 milter_server_context_get_name(context));

    expire_child(children, context);
    remove_child_from_queue(children, context);
}

static void
cb_error (MilterErrorEmittable *emittable, GError *error, gpointer user_data)
{
    MilterManagerChildren *children = MILTER_MANAGER_CHILDREN(user_data);
    MilterServerContext *context = MILTER_SERVER_CONTEXT(emittable);
    MilterManagerChildrenPrivate *priv;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(user_data);

    milter_error("error: %s", error->message);

    milter_error_emittable_emit(MILTER_ERROR_EMITTABLE(user_data),
                                error);

    expire_child(children, context);
    remove_child_from_queue(children, context);
}

static void
cb_finished (MilterAgent *agent, gpointer user_data)
{
    MilterManagerChildren *children;
    MilterManagerChildrenPrivate *priv;
    MilterServerContext *context = MILTER_SERVER_CONTEXT(agent);
    gchar *state_string;
    const gchar *child_name;

    children = MILTER_MANAGER_CHILDREN(user_data);
    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    expire_child(children, context);
    state_string = milter_utils_get_enum_name(MILTER_TYPE_SERVER_CONTEXT_STATE,
                                              priv->current_state);
    child_name = milter_server_context_get_name(context);

    milter_debug("%s exits on %s.", child_name, state_string);
            
    g_free(state_string);

    switch (priv->current_state) {
      case MILTER_SERVER_CONTEXT_STATE_DATA:
      case MILTER_SERVER_CONTEXT_STATE_HEADER:
      case MILTER_SERVER_CONTEXT_STATE_END_OF_HEADER:
      case MILTER_SERVER_CONTEXT_STATE_END_OF_MESSAGE:
      case MILTER_SERVER_CONTEXT_STATE_BODY:
        return;
        break;
      default:
        remove_child_from_queue(children, context);
        break;
    }
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

    CONNECT(writing_timeout);
    CONNECT(reading_timeout);
    CONNECT(end_of_message_timeout);

    CONNECT(error);
    CONNECT(finished);
#undef CONNECT
}

static void
teardown_server_context_signals (MilterManagerChild *child,
                                 gpointer user_data)
{
#define DISCONNECT(name)                                                \
    g_signal_handlers_disconnect_by_func(child,                         \
                                         G_CALLBACK(cb_ ## name),       \
                                         user_data)

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

    DISCONNECT(writing_timeout);
    DISCONNECT(reading_timeout);
    DISCONNECT(end_of_message_timeout);

    DISCONNECT(error);
    DISCONNECT(finished);
#undef DISCONNECT
}

static gboolean
retry_establish_connection (gpointer user_data)
{
    NegotiateData *data = user_data;
    MilterManagerChildrenPrivate *priv;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(data->children);

    child_establish_connection(data->child,
                               data->option,
                               data->children,
                               TRUE);

    g_hash_table_remove(priv->try_negotiate_ids, data);

    return FALSE;
}

static gboolean
milter_manager_children_start_child (MilterManagerChildren *children,
                                     MilterManagerChild *child)
{
    GError *error = NULL;

    milter_manager_child_start(child, &error);
    if (error) {
        milter_error("Error: %s", error->message);
        milter_error_emittable_emit(MILTER_ERROR_EMITTABLE(children),
                                    error);
        g_error_free(error);
        return FALSE;
    }

    return TRUE;
}

static void
remove_queue_in_negotiate (MilterManagerChildren *children,
                           MilterManagerChild *child)
{
    MilterManagerChildrenPrivate *priv;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);
    g_queue_remove(priv->reply_queue, child);
    if (g_queue_is_empty(priv->reply_queue)) {
        if (priv->option) {
            g_signal_emit_by_name(children, "negotiate-reply",
                                  priv->option, priv->macros_requests);
        } else {
            GError *error = NULL;
            g_set_error(&error,
                        MILTER_MANAGER_CHILDREN_ERROR,
                        MILTER_MANAGER_CHILDREN_ERROR_NO_NEGOTIATION_RESPONSE,
                        "There is no negotiation response from milters.");
            milter_error("%s", error->message);
            milter_error_emittable_emit(MILTER_ERROR_EMITTABLE(children),
                                        error);
            g_error_free(error);
        }
    }
}

static void
clear_try_negotiate_data (NegotiateData *data)
{
    MilterManagerChildrenPrivate *priv;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(data->children);

    remove_queue_in_negotiate(data->children, data->child);
    g_hash_table_remove(priv->try_negotiate_ids, data);
}

static void
cb_connection_timeout (MilterServerContext *context, gpointer user_data)
{
    NegotiateData *data = user_data;

    milter_error("connection to %s is timed out.",
                 milter_server_context_get_name(context));
    clear_try_negotiate_data(data);
}

static void
cb_connection_error (MilterErrorEmittable *emittable, GError *error, gpointer user_data)
{
    NegotiateData *data = user_data;
    MilterManagerChildrenPrivate *priv;
    gboolean privilege;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(data->children);
    milter_debug("connection_error: %s", error->message);

    /* ignore MILTER_MANAGER_CHILD_ERROR_MILTER_EXIT */
    if (error->domain != MILTER_SERVER_CONTEXT_ERROR ||
        data->is_retry) {
        milter_error_emittable_emit(MILTER_ERROR_EMITTABLE(data->children),
                                    error);
        clear_try_negotiate_data(data);
        return;
    }

    privilege =
        milter_manager_configuration_is_privilege_mode(priv->configuration);
    if (!privilege) {
        clear_try_negotiate_data(data);
        return;
    }

    if (!milter_manager_children_start_child(data->children, data->child)) {
        clear_try_negotiate_data(data);
        return;
    }

    prepare_retry_establish_connection(data->child,
                                       data->option,
                                       data->children, 
                                       TRUE);
    g_hash_table_remove(priv->try_negotiate_ids, data);
}

static NegotiateData *
negotiate_data_new (MilterManagerChildren *children,
                    MilterManagerChild *child,
                    MilterOption *option,
                    gboolean is_retry)
{
    NegotiateData *negotiate_data;

    negotiate_data = g_new0(NegotiateData, 1);
    negotiate_data->child = g_object_ref(child);
    negotiate_data->children = children;
    negotiate_data->option = g_object_ref(option);
    negotiate_data->is_retry = is_retry;
    negotiate_data->error_signal_id =
        g_signal_connect(child, "error",
                         G_CALLBACK(cb_connection_error),
                         negotiate_data);
    negotiate_data->ready_signal_id =
        g_signal_connect(child, "ready",
                         G_CALLBACK(cb_ready),
                         negotiate_data);
    negotiate_data->connection_timeout_signal_id =
        g_signal_connect(child, "connection-timeout",
                         G_CALLBACK(cb_connection_timeout),
                         negotiate_data);

    return negotiate_data;
}

static void
negotiate_data_free (NegotiateData *data)
{
    if (data->connection_timeout_signal_id > 0)
        g_signal_handler_disconnect(data->child,
                                    data->connection_timeout_signal_id);
    if (data->error_signal_id > 0)
        g_signal_handler_disconnect(data->child, data->error_signal_id);
    if (data->ready_signal_id > 0)
        g_signal_handler_disconnect(data->child, data->ready_signal_id);

    g_object_unref(data->child);
    g_object_unref(data->option);

    g_free(data);
}

static void
negotiate_data_hash_key_free (gpointer data)
{
    negotiate_data_free(data);
}

static void
prepare_retry_establish_connection (MilterManagerChild *child,
                                    MilterOption *option,
                                    MilterManagerChildren *children,
                                    gboolean is_retry)
{
    MilterManagerChildrenPrivate *priv;
    NegotiateData *negotiate_data;
    guint timeout_id;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    negotiate_data = negotiate_data_new(children, child, option, is_retry);
    timeout_id = g_timeout_add(priv->retry_connect_time * 1000,
                               retry_establish_connection,
                               negotiate_data);

    g_hash_table_insert(priv->try_negotiate_ids,
                        negotiate_data, GUINT_TO_POINTER(timeout_id));
}

static void
prepare_negotiate (MilterManagerChild *child,
                   MilterOption *option,
                   MilterManagerChildren *children,
                   gboolean is_retry)
{
    MilterManagerChildrenPrivate *priv;
    NegotiateData *negotiate_data;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    negotiate_data = negotiate_data_new(children, child, option, is_retry);
    g_hash_table_insert(priv->try_negotiate_ids,
                        negotiate_data, NULL);
}

static gboolean
child_establish_connection (MilterManagerChild *child,
                            MilterOption *option,
                            MilterManagerChildren *children,
                            gboolean is_retry)
{
    MilterManagerChildrenPrivate *priv;
    MilterServerContext *context;
    GError *error = NULL;

    context = MILTER_SERVER_CONTEXT(child);
    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    if (!milter_server_context_establish_connection(context, &error)) {

        milter_error("Error: %s", error->message);
        milter_error_emittable_emit(MILTER_ERROR_EMITTABLE(children),
                                    error);

        g_error_free(error);
        if (is_retry)
            return FALSE;

        prepare_retry_establish_connection(child, option, children, TRUE);
        return FALSE;
    }

    prepare_negotiate(child, option, children, is_retry);

    return TRUE;
}

static MilterServerContextState
command_to_state (MilterCommand command)
{
    switch (command) {
      case MILTER_COMMAND_CONNECT:
        return MILTER_SERVER_CONTEXT_STATE_CONNECT;
        break;
      case MILTER_COMMAND_HELO:
        return MILTER_SERVER_CONTEXT_STATE_HELO;
        break;
      case MILTER_COMMAND_ENVELOPE_FROM:
        return MILTER_SERVER_CONTEXT_STATE_ENVELOPE_FROM;
        break;
      case MILTER_COMMAND_ENVELOPE_RECIPIENT:
        return MILTER_SERVER_CONTEXT_STATE_ENVELOPE_RECIPIENT;
        break;
      case MILTER_COMMAND_DATA:
        return MILTER_SERVER_CONTEXT_STATE_DATA;
        break;
      case MILTER_COMMAND_HEADER:
        return MILTER_SERVER_CONTEXT_STATE_HEADER;
        break;
      case MILTER_COMMAND_END_OF_HEADER:
        return MILTER_SERVER_CONTEXT_STATE_END_OF_HEADER;
        break;
      case MILTER_COMMAND_BODY:
        return MILTER_SERVER_CONTEXT_STATE_BODY;
        break;
      case MILTER_COMMAND_END_OF_MESSAGE:
        return MILTER_SERVER_CONTEXT_STATE_END_OF_MESSAGE;
        break;
      default:
        return MILTER_SERVER_CONTEXT_STATE_START;
        break;
    }

    return MILTER_SERVER_CONTEXT_STATE_START;
}

static MilterCommand
state_to_command (MilterServerContextState state)
{
    switch (state) {
      case MILTER_SERVER_CONTEXT_STATE_DATA:
        return MILTER_COMMAND_DATA;
        break;
      case MILTER_SERVER_CONTEXT_STATE_HEADER:
        return MILTER_COMMAND_HEADER;
        break;
      case MILTER_SERVER_CONTEXT_STATE_END_OF_HEADER:
        return MILTER_COMMAND_END_OF_HEADER;
        break;
      case MILTER_SERVER_CONTEXT_STATE_BODY:
        return MILTER_COMMAND_BODY;
        break;
      case MILTER_SERVER_CONTEXT_STATE_END_OF_MESSAGE:
        return MILTER_COMMAND_END_OF_MESSAGE;
        break;
      default:
        return MILTER_COMMAND_UNKNOWN;
        break;
    }

    return MILTER_COMMAND_UNKNOWN;
}

static MilterCommand
get_next_command (MilterManagerChildren *children, 
                  MilterServerContext *context,
                  MilterServerContextState state)
{
    MilterManagerChildrenPrivate *priv;
    GList *node;
    MilterCommand current_command;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    current_command = state_to_command(state);

    node = g_list_find(priv->command_queue, GINT_TO_POINTER(current_command));
    if (!node || !g_list_next(node))
        return -1;

    node = g_list_next(node);
    return fetch_first_command_for_child_in_queue(context, &node);
}

static gboolean
send_next_command (MilterManagerChildren *children,
                   MilterServerContext *context,
                   MilterServerContextState current_state)
{
    MilterCommand next_command;
    MilterManagerChildrenPrivate *priv;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    next_command = get_next_command(children, context, current_state);
    if (next_command == -1) {
        g_signal_emit_by_name(children, "continue");
        return FALSE;
    }

    send_command_to_child(children, context,  next_command);

    return TRUE;
}

static void
init_command_waiting_child_queue (MilterManagerChildren *children, MilterCommand command)
{
    MilterManagerChildrenPrivate *priv;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    if (!g_list_find(priv->command_queue, GINT_TO_POINTER(command)))
        priv->command_queue = g_list_append(priv->command_queue, GINT_TO_POINTER(command));

    if (priv->command_waiting_child_queue)
        return;

    priv->command_waiting_child_queue = g_list_copy(priv->milters);
}

static MilterServerContext *
get_first_child_in_command_waiting_child_queue (MilterManagerChildren *children)
{
    MilterManagerChildrenPrivate *priv;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);
    if (!priv->command_waiting_child_queue)
        return NULL;

    return MILTER_SERVER_CONTEXT(g_list_first(priv->command_waiting_child_queue)->data);
}

static void
init_reply_queue (MilterManagerChildren *children,
                  MilterServerContextState state)
{
    MilterManagerChildrenPrivate *priv;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    g_queue_clear(priv->reply_queue);
    priv->current_state = state;
    g_hash_table_insert(priv->reply_statuses,
                        GINT_TO_POINTER(state),
                        GINT_TO_POINTER(MILTER_STATUS_NOT_CHANGE));
}

gboolean
milter_manager_children_negotiate (MilterManagerChildren *children,
                                   MilterOption          *option)
{
    GList *node;
    MilterManagerChildrenPrivate *priv;
    gboolean success = TRUE;
    gboolean privilege;
    MilterStatus fallback_status;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    privilege =
        milter_manager_configuration_is_privilege_mode(priv->configuration);
    fallback_status =
        milter_manager_configuration_get_fallback_status(priv->configuration);

    init_reply_queue(children, MILTER_SERVER_CONTEXT_STATE_NEGOTIATE);
    for (node = priv->milters; node; node = g_list_next(node)) {
        MilterManagerChild *child = MILTER_MANAGER_CHILD(node->data);

        if (!child_establish_connection(child, option, children, FALSE)) {
            if (privilege &&
                milter_manager_children_start_child(children, child)) {
                prepare_retry_establish_connection(child, option, children,
                                                   FALSE);
                g_queue_push_tail(priv->reply_queue, child);
            } else {
                success = FALSE;
            }
        } else {
            g_queue_push_tail(priv->reply_queue, child);
        }
    }

    return success;
}

gboolean
milter_manager_children_define_macro (MilterManagerChildren *children,
                                      MilterCommand command,
                                      GHashTable *macros)
{
    GList *child;
    MilterManagerChildrenPrivate *priv;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    for (child = priv->milters; child; child = g_list_next(child)) {
        MilterProtocolAgent *agent = MILTER_PROTOCOL_AGENT(child->data);

        milter_protocol_agent_set_macros_hash_table(agent,
                                                    command,
                                                    macros);
    }
    return TRUE;
}

static gboolean
milter_manager_children_check_alive (MilterManagerChildren *children)
{
    MilterManagerChildrenPrivate *priv;
    GError *error = NULL;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);
    if (priv->milters)
        return TRUE;

    g_set_error(&error,
                MILTER_MANAGER_CHILDREN_ERROR,
                MILTER_MANAGER_CHILDREN_ERROR_NO_ALIVE_MILTER,
                "All milters are no longer alive.");
    milter_error("%s", error->message);
    milter_error_emittable_emit(MILTER_ERROR_EMITTABLE(children),
                                error);
    g_error_free(error);

    return FALSE;
}

static gboolean
milter_manager_children_is_demanding_command (MilterManagerChildren *children,
                                              MilterCommand command)
{
    GList *child;
    MilterManagerChildrenPrivate *priv;
    MilterStepFlags step;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    step = command_to_no_step_flag(command);

    for (child = priv->milters; child; child = g_list_next(child)) {
        MilterServerContext *context = MILTER_SERVER_CONTEXT(child->data);

        if (!milter_server_context_is_enable_step(context, step))
            return TRUE;
    }

    return FALSE;
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

    if (!milter_manager_children_check_alive(children))
        return FALSE;

    if (!milter_manager_children_is_demanding_command(children,
                                                      MILTER_COMMAND_CONNECT)) {
        g_signal_emit_by_name(children, "continue");
        return TRUE;
    }

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    init_reply_queue(children, MILTER_SERVER_CONTEXT_STATE_CONNECT);
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

    if (!milter_manager_children_check_alive(children))
        return FALSE;

    if (!milter_manager_children_is_demanding_command(children,
                                                      MILTER_COMMAND_HELO)) {
        g_signal_emit_by_name(children, "continue");
        return TRUE;
    }

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    init_reply_queue(children, MILTER_SERVER_CONTEXT_STATE_HELO);
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

    if (!milter_manager_children_check_alive(children))
        return FALSE;

    if (!milter_manager_children_is_demanding_command(children,
                                                      MILTER_COMMAND_ENVELOPE_FROM)) {
        g_signal_emit_by_name(children, "continue");
        return TRUE;
    }

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    init_reply_queue(children, MILTER_SERVER_CONTEXT_STATE_ENVELOPE_FROM);
    for (child = priv->milters; child; child = g_list_next(child)) {
        MilterServerContext *context = MILTER_SERVER_CONTEXT(child->data);

        if (milter_server_context_is_enable_step(context,
                                                 MILTER_STEP_NO_ENVELOPE_FROM))
            continue;

        g_queue_push_tail(priv->reply_queue, context);
        success |= milter_server_context_envelope_from(context, from);
    }

    return success;
}

gboolean
milter_manager_children_envelope_recipient (MilterManagerChildren *children,
                                            const gchar           *recipient)
{
    GList *child;
    MilterManagerChildrenPrivate *priv;
    gboolean success = TRUE;

    if (!milter_manager_children_check_alive(children))
        return FALSE;

    if (!milter_manager_children_is_demanding_command(children, MILTER_COMMAND_ENVELOPE_RECIPIENT)) {
        g_signal_emit_by_name(children, "continue");
        return TRUE;
    }

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    init_reply_queue(children, MILTER_SERVER_CONTEXT_STATE_ENVELOPE_RECIPIENT);
    for (child = priv->milters; child; child = g_list_next(child)) {
        MilterServerContext *context = MILTER_SERVER_CONTEXT(child->data);

        if (milter_server_context_is_enable_step(context, MILTER_STEP_NO_ENVELOPE_RECIPIENT))
            continue;

        g_queue_push_tail(priv->reply_queue, context);
        success |= milter_server_context_envelope_recipient(context, recipient);
    }

    return success;
}

static gboolean
send_command_to_first_waiting_child (MilterManagerChildren *children,
                                     MilterCommand command)
{
    MilterServerContext *first_child;
    MilterStepFlags no_step;

    first_child = get_first_child_in_command_waiting_child_queue(children);
    if (!first_child)
        return FALSE;

    no_step = command_to_no_step_flag(command);
    if (!milter_server_context_is_enable_step(first_child, no_step))
        return send_command_to_child(children, first_child, command);

    /*
     * If the first child is not needed the command, 
     * send dummy "continue" to shift next state.
     */
    milter_server_context_set_state(first_child,
                                    command_to_state(command));
    g_signal_emit_by_name(first_child, "continue");
    return TRUE;
}

static void
emit_no_demand_command_error (MilterManagerChildren *children,
                              MilterCommand command)
{
    gchar *command_string;
    GError *error = NULL;

    command_string = milter_utils_inspect_enum(MILTER_TYPE_COMMAND,
                                               command);
    g_set_error(&error,
                MILTER_MANAGER_CHILDREN_ERROR,
                MILTER_MANAGER_CHILDREN_ERROR_NO_DEMAND_COMMAND,
                "No milter demands %s.", command_string);
    milter_error("%s", error->message);
    milter_error_emittable_emit(MILTER_ERROR_EMITTABLE(children),
                                error);
    g_free(command_string);
    g_error_free(error);
}

gboolean
milter_manager_children_data (MilterManagerChildren *children)
{
    MilterManagerChildrenPrivate *priv;

    if (!milter_manager_children_check_alive(children))
        return FALSE;

    if (!milter_manager_children_is_demanding_command(children,
                                                      MILTER_COMMAND_DATA)) {
        emit_no_demand_command_error(children, MILTER_COMMAND_UNKNOWN);
        return FALSE;
    }

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    priv->current_state = MILTER_SERVER_CONTEXT_STATE_DATA;
    init_command_waiting_child_queue(children, MILTER_COMMAND_DATA);

    return send_command_to_first_waiting_child(children, MILTER_COMMAND_DATA);
}

gboolean
milter_manager_children_unknown (MilterManagerChildren *children,
                                 const gchar           *command)
{
    GList *child;
    MilterManagerChildrenPrivate *priv;
    gboolean success = TRUE;

    if (!milter_manager_children_check_alive(children))
        return FALSE;

    if (!milter_manager_children_is_demanding_command(children,
                                                      MILTER_COMMAND_UNKNOWN)) {
        emit_no_demand_command_error(children, MILTER_COMMAND_UNKNOWN);
        return FALSE;
    }

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    init_reply_queue(children, MILTER_SERVER_CONTEXT_STATE_UNKNOWN);
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

static gboolean 
send_next_header_to_child (MilterManagerChildren *children, MilterServerContext *context)
{
    MilterManagerChildrenPrivate *priv;
    MilterHeader *header;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    if (milter_server_context_is_enable_step(context, MILTER_STEP_NO_HEADERS))
        return TRUE;

    priv->current_state = MILTER_SERVER_CONTEXT_STATE_HEADER;

    priv->processing_header_index++;
    header = milter_headers_get_nth_header(priv->headers,
                                           priv->processing_header_index);
    if (!header)
        return FALSE;

    return milter_server_context_header(context,
                                        header->name,
                                        header->value);
}

gboolean
milter_manager_children_header (MilterManagerChildren *children,
                                const gchar           *name,
                                const gchar           *value)
{
    MilterManagerChildrenPrivate *priv;

    if (!milter_manager_children_check_alive(children))
        return FALSE;

    if (!milter_manager_children_is_demanding_command(children,
                                                      MILTER_COMMAND_HEADER)) {
        g_signal_emit_by_name(children, "continue");
        return TRUE;
    }

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    priv->current_state = MILTER_SERVER_CONTEXT_STATE_HEADER;
    milter_headers_add_header(priv->headers, name, value);
    init_command_waiting_child_queue(children, MILTER_COMMAND_HEADER);

    return send_command_to_first_waiting_child(children, MILTER_COMMAND_HEADER);
}

gboolean
milter_manager_children_end_of_header (MilterManagerChildren *children)
{
    MilterManagerChildrenPrivate *priv;

    if (!milter_manager_children_check_alive(children))
        return FALSE;

    if (!milter_manager_children_is_demanding_command(children,
                                                      MILTER_COMMAND_END_OF_HEADER)) {
        emit_no_demand_command_error(children, MILTER_COMMAND_UNKNOWN);
        return FALSE;
    }

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    priv->current_state = MILTER_SERVER_CONTEXT_STATE_END_OF_HEADER;
    init_command_waiting_child_queue(children, MILTER_COMMAND_END_OF_HEADER);

    return send_command_to_first_waiting_child(children,
                                               MILTER_COMMAND_END_OF_HEADER);
}

static gboolean
open_body_file (MilterManagerChildren *children)
{
    gint fd;
    GError *error = NULL;
    MilterManagerChildrenPrivate *priv;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    fd = g_file_open_tmp(NULL, &priv->body_file_name, &error);
    priv->body_file = g_io_channel_unix_new(fd);
    if (error) {
        milter_error_emittable_emit(MILTER_ERROR_EMITTABLE(children),
                                    error);
        g_error_free(error);
        return FALSE;
    }

    g_io_channel_set_encoding(priv->body_file, NULL, &error);
    if (error) {
        milter_error_emittable_emit(MILTER_ERROR_EMITTABLE(children),
                                    error);
        g_error_free(error);
        return FALSE;
    }

    return TRUE;
}

static gboolean
write_to_body_file (MilterManagerChildren *children,
                    const gchar *chunk,
                    gsize size)
{
    GError *error = NULL;
    gsize written_size;
    MilterManagerChildrenPrivate *priv;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    if (!priv->body_file && !open_body_file(children))
        return FALSE;

    if (!priv->body_file)
        return TRUE;

    if (!chunk || size == 0)
        return TRUE;

    g_io_channel_write_chars(priv->body_file, chunk, size, &written_size, &error);
    if (error) {
        milter_error_emittable_emit(MILTER_ERROR_EMITTABLE(children),
                                    error);
        g_error_free(error);
        return FALSE;
    }

    return TRUE;
}

gboolean
milter_manager_children_body (MilterManagerChildren *children,
                              const gchar           *chunk,
                              gsize                  size)
{
    MilterManagerChildrenPrivate *priv;
    MilterServerContext *first_child;

    if (!milter_manager_children_check_alive(children))
        return FALSE;

    if (!milter_manager_children_is_demanding_command(children, MILTER_COMMAND_BODY)) {
        g_signal_emit_by_name(children, "continue");
        return TRUE;
    }
        
    init_command_waiting_child_queue(children, MILTER_COMMAND_BODY);

    first_child = get_first_child_in_command_waiting_child_queue(children);
    if (!first_child)
        return FALSE;

    if (!write_to_body_file(children, chunk, size))
        return FALSE;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);
    priv->current_state = MILTER_SERVER_CONTEXT_STATE_BODY;
    priv->sent_body_count++;

    if (!milter_server_context_is_enable_step(first_child, MILTER_STEP_NO_BODY) && 
        !milter_server_context_get_skip_body(first_child)) {
        return milter_server_context_body(first_child, chunk, size);
    }

    /*
     * If the first child is not needed the command, 
     * send dummy "continue" to shift next state.
     */
    milter_server_context_set_state(first_child, MILTER_SERVER_CONTEXT_STATE_BODY);
    g_signal_emit_by_name(first_child, "continue");
    return TRUE;
}

static gboolean 
send_body_to_child (MilterManagerChildren *children, MilterServerContext *context)
{
    MilterManagerChildrenPrivate *priv;
    gboolean success = TRUE;
    GError *error = NULL;
    GIOStatus status = G_IO_STATUS_NORMAL;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    if (milter_server_context_is_enable_step(context, MILTER_STEP_NO_BODY))
        return TRUE;

    if (milter_server_context_get_skip_body(context))
        return TRUE;

    priv->current_state = MILTER_SERVER_CONTEXT_STATE_BODY;

    if (!priv->body_file)
        return TRUE;

    priv->sent_body_count = 0;
    g_io_channel_seek_position(priv->body_file, 0, G_SEEK_SET, &error);

    while (status == G_IO_STATUS_NORMAL && success) {
        gchar buffer[MILTER_CHUNK_SIZE + 1];
        gsize read_size;

        status = g_io_channel_read_chars(priv->body_file, buffer, MILTER_CHUNK_SIZE,
                                         &read_size, &error);
    
        if (status == G_IO_STATUS_NORMAL) {
            success &= milter_server_context_body(context,
                                                  buffer, read_size);
            if (success)
                priv->sent_body_count++;
        }
    }

    if (error) {
        milter_error_emittable_emit(MILTER_ERROR_EMITTABLE(children),
                error);
        g_error_free(error);

        return FALSE;
    }

    g_io_channel_seek_position(priv->body_file, 0, G_SEEK_SET, &error);
    if (error) {
        milter_error_emittable_emit(MILTER_ERROR_EMITTABLE(children),
                error);
        g_error_free(error);

        return FALSE;
    }

    return success;
}

gboolean
milter_manager_children_end_of_message (MilterManagerChildren *children,
                                        const gchar           *chunk,
                                        gsize                  size)
{
    MilterManagerChildrenPrivate *priv;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    if (!milter_manager_children_check_alive(children))
        return FALSE;

    init_command_waiting_child_queue(children, MILTER_COMMAND_END_OF_MESSAGE);

    priv->processing_header_index = 0;
    if (!priv->original_headers)
        priv->original_headers = milter_headers_copy(priv->headers);

    if (priv->end_of_message_chunk)
        g_free(priv->end_of_message_chunk);
    priv->end_of_message_chunk = g_strdup(chunk);
    priv->end_of_message_size = size;
    if (priv->body_file)
        g_io_channel_seek_position(priv->body_file, 0, G_SEEK_SET, NULL);

    priv->current_state = MILTER_SERVER_CONTEXT_STATE_END_OF_MESSAGE;
    return send_command_to_first_waiting_child(children,
                                               MILTER_COMMAND_END_OF_MESSAGE);
}

gboolean
milter_manager_children_quit (MilterManagerChildren *children)
{
    GList *child;
    MilterManagerChildrenPrivate *priv;
    gboolean success = TRUE;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    init_reply_queue(children, MILTER_SERVER_CONTEXT_STATE_QUIT);
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

    init_reply_queue(children, MILTER_SERVER_CONTEXT_STATE_ABORT);
    for (child = priv->milters; child; child = g_list_next(child)) {
        MilterServerContext *context = MILTER_SERVER_CONTEXT(child->data);
        g_queue_push_tail(priv->reply_queue, context);
        success |= milter_server_context_abort(context);
    }

    return success;
}


gboolean
milter_manager_children_is_important_status (MilterManagerChildren *children,
                                             MilterServerContextState state,
                                             MilterStatus status)
{
    MilterManagerChildrenPrivate *priv;
    MilterStatus a, b;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    a = get_reply_status_for_state(children, state);
    b = status;

    switch (a) {
      case MILTER_STATUS_REJECT:
        if (state != MILTER_SERVER_CONTEXT_STATE_ENVELOPE_RECIPIENT)
            return FALSE;
        return (b == MILTER_STATUS_DISCARD);
        break;
      case MILTER_STATUS_DISCARD:
        if (state == MILTER_SERVER_CONTEXT_STATE_ENVELOPE_RECIPIENT)
            return FALSE;
        return (b == MILTER_STATUS_REJECT);
        break;
      case MILTER_STATUS_TEMPORARY_FAILURE:
        if (b == MILTER_STATUS_NOT_CHANGE)
            return FALSE;
        return TRUE;
        break;
      case MILTER_STATUS_ACCEPT:
        if (b == MILTER_STATUS_NOT_CHANGE ||
            b == MILTER_STATUS_TEMPORARY_FAILURE)
            return FALSE;
        return TRUE;
        break;
      case MILTER_STATUS_SKIP:
        if (b == MILTER_STATUS_NOT_CHANGE ||
            b == MILTER_STATUS_ACCEPT ||
            b == MILTER_STATUS_TEMPORARY_FAILURE)
            return FALSE;
        return TRUE;
        break;
      case MILTER_STATUS_CONTINUE:
        if (b == MILTER_STATUS_NOT_CHANGE ||
            b == MILTER_STATUS_ACCEPT ||
            b == MILTER_STATUS_TEMPORARY_FAILURE ||
            b == MILTER_STATUS_SKIP)
            return FALSE;
        return TRUE;
      default:
        return TRUE;
        break;
    }

    return TRUE;
}

void
milter_manager_children_set_status (MilterManagerChildren *children,
                                    MilterServerContextState state,
                                    MilterStatus status)
{
    MilterManagerChildrenPrivate *priv;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);
    
    g_hash_table_insert(priv->reply_statuses, 
                        GINT_TO_POINTER(state),
                        GINT_TO_POINTER(status));
}

void
milter_manager_children_set_retry_connect_time (MilterManagerChildren *children,
                                                gdouble time)
{
    MILTER_MANAGER_CHILDREN_GET_PRIVATE(children)->retry_connect_time = time;
}

MilterServerContextState
milter_manager_children_get_current_state (MilterManagerChildren *children)
{
    return MILTER_MANAGER_CHILDREN_GET_PRIVATE(children)->current_state;
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
