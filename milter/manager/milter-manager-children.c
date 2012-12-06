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

#include "milter-manager-children.h"

#include <glib/gstdio.h>
#include "milter-manager-configuration.h"
#include "milter/core.h"
#include "milter-manager-launch-command-encoder.h"

#define MAX_ON_MEMORY_BODY_SIZE 5242880 /* 5Mbyte */

#define MAX_SUPPORTED_MILTER_PROTOCOL_VERSION 6

#define MILTER_MANAGER_CHILDREN_GET_PRIVATE(obj)                    \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj),                             \
                                 MILTER_TYPE_MANAGER_CHILDREN,      \
                                 MilterManagerChildrenPrivate))

typedef struct _PendingMessageRequest PendingMessageRequest;
struct _PendingMessageRequest
{
    MilterCommand command;
    union {
        struct _HeaderArguments {
            gchar *name;
            gchar *value;
        } header;
        struct _BodyArguments {
            gchar *chunk;
            gsize size;
        } body;
        struct _EndOfMessageArguments {
            gchar *chunk;
            gsize size;
        } end_of_message;
    } arguments;
};

typedef struct _MilterManagerChildrenPrivate	MilterManagerChildrenPrivate;
struct _MilterManagerChildrenPrivate
{
    GList *milters;
    GQueue *reply_queue;
    GList *command_waiting_child_queue; /* storing child milters which is waiting for commands after DATA command */
    GList *command_queue; /* storing commands after DATA command */
    PendingMessageRequest *pending_message_request;
    GHashTable *try_negotiate_ids;
    MilterManagerConfiguration *configuration;
    MilterMacrosRequests *macros_requests;
    MilterOption *option;
    MilterStepFlags initial_yes_steps;
    MilterStepFlags requested_yes_steps;
    gboolean negotiated;
    gboolean all_expired_as_fallback_on_negotiated;
    MilterServerContextState state;
    MilterServerContextState processing_state;
    GHashTable *reply_statuses;
    guint reply_code;
    gchar *reply_extended_code;
    gchar *reply_message;
    gdouble retry_connect_time;

    struct sockaddr *smtp_client_address;
    socklen_t smtp_client_address_length;
    MilterHeaders *original_headers;
    MilterHeaders *headers;
    gint processing_header_index;
    GString *body;
    GIOChannel *body_file;
    gchar *body_file_name;
    gchar *end_of_message_chunk;
    gsize end_of_message_size;
    guint sending_body;
    guint sent_body_offset;
    gboolean replaced_body_for_each_child;
    gboolean replaced_body;
    gchar *change_from;
    gchar *change_from_parameters;
    gchar *quarantine_reason;
    MilterWriter *launcher_writer;
    MilterReader *launcher_reader;

    gboolean finished;
    gboolean emitted_reply_for_message_oriented_command;

    guint tag;

    MilterEventLoop *event_loop;

    guint lazy_reply_negotiate_id;
};

typedef struct _NegotiateData NegotiateData;
struct _NegotiateData
{
    MilterManagerChildren *children;
    MilterManagerChild *child;
    MilterOption *option;
    gulong error_signal_id;
    gulong ready_signal_id;
    gulong connection_timeout_signal_id;
    gboolean is_retry;
};

typedef struct _NegotiateTimeoutID NegotiateTimeoutID;
struct _NegotiateTimeoutID
{
    MilterEventLoop *loop;
    guint timeout_id;
};

enum
{
    PROP_0,
    PROP_CONFIGURATION,
    PROP_TAG,
    PROP_EVENT_LOOP
};

static void         finished           (MilterFinishedEmittable *emittable);

MILTER_IMPLEMENT_ERROR_EMITTABLE(error_emittable_init);
MILTER_IMPLEMENT_FINISHED_EMITTABLE_WITH_CODE(finished_emittable_init,
                                              iface->finished = finished)
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
static gboolean write_body (MilterManagerChildren *children,
                            const gchar *chunk,
                            gsize size);
static MilterStatus init_child_for_body
                           (MilterManagerChildren *children,
                            MilterServerContext *context);
static MilterStatus send_body_to_child
                           (MilterManagerChildren *children,
                            MilterServerContext *context);
static MilterStatus send_next_header_to_child
                           (MilterManagerChildren *children,
                            MilterServerContext *context);
static MilterStatus send_next_command
                           (MilterManagerChildren *children,
                            MilterServerContext *context,
                            MilterServerContextState processing_state);
static MilterStatus send_first_command_to_next_child
                           (MilterManagerChildren *children,
                            MilterServerContext *context);

static NegotiateData *negotiate_data_new  (MilterManagerChildren *children,
                                           MilterManagerChild *child,
                                           MilterOption *option,
                                           gboolean is_retry);
static void           negotiate_data_free (NegotiateData *data);
static void           negotiate_data_hash_key_free (gpointer data);
static NegotiateTimeoutID *
                      negotiate_timeout_id_new  (MilterEventLoop *loop,
                                                 guint            timeout_id);
static void           negotiate_timeout_id_free (NegotiateTimeoutID *id);
static void           negotiate_timeout_id_hash_value_free (gpointer data);

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
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    g_object_class_install_property(gobject_class, PROP_CONFIGURATION, spec);

    spec = g_param_spec_uint("tag",
                             "Tag",
                             "The tag of the agent",
                             0, G_MAXUINT, 0,
                             G_PARAM_READABLE);
    g_object_class_install_property(gobject_class, PROP_TAG, spec);

    spec = g_param_spec_object("event-loop",
                               "Event Loop",
                               "The event loop of the milter controller",
                               MILTER_TYPE_EVENT_LOOP,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    g_object_class_install_property(gobject_class, PROP_EVENT_LOOP, spec);

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
    priv->command_waiting_child_queue = NULL;
    priv->command_queue = NULL;
    priv->pending_message_request = NULL;
    priv->try_negotiate_ids =
        g_hash_table_new_full(g_direct_hash, g_direct_equal,
                              negotiate_data_hash_key_free,
                              negotiate_timeout_id_hash_value_free);
    priv->milters = NULL;
    priv->macros_requests = milter_macros_requests_new();
    priv->option = NULL;
    priv->initial_yes_steps = MILTER_STEP_NONE;
    priv->requested_yes_steps = MILTER_STEP_NONE;
    priv->negotiated = FALSE;
    priv->all_expired_as_fallback_on_negotiated = FALSE;
    priv->reply_statuses = g_hash_table_new(g_direct_hash, g_direct_equal);

    priv->smtp_client_address = NULL;
    priv->smtp_client_address_length = 0;
    priv->original_headers = NULL;
    priv->headers = NULL;
    priv->processing_header_index = 0;
    priv->body = NULL;
    priv->body_file = NULL;
    priv->body_file_name = NULL;
    priv->end_of_message_chunk = NULL;
    priv->end_of_message_size = 0;
    priv->sending_body = FALSE;
    priv->sent_body_offset = 0;
    priv->replaced_body = FALSE;
    priv->replaced_body_for_each_child = FALSE;
    priv->change_from = NULL;
    priv->change_from_parameters = NULL;
    priv->quarantine_reason = NULL;

    priv->state = MILTER_SERVER_CONTEXT_STATE_START;
    priv->processing_state = MILTER_SERVER_CONTEXT_STATE_START;

    priv->reply_code = 0;
    priv->reply_extended_code = NULL;
    priv->reply_message = NULL;

    priv->retry_connect_time = 5.0;
    priv->launcher_reader = NULL;
    priv->launcher_writer = NULL;

    priv->finished = FALSE;
    priv->emitted_reply_for_message_oriented_command = FALSE;

    priv->tag = 0;

    priv->event_loop = NULL;

    priv->lazy_reply_negotiate_id = 0;
}

static void
dispose_lazy_reply_negotiate_id (MilterManagerChildrenPrivate *priv)
{
    if (priv->lazy_reply_negotiate_id == 0)
        return;

    milter_event_loop_remove(priv->event_loop,
                             priv->lazy_reply_negotiate_id);
    priv->lazy_reply_negotiate_id = 0;
}

static PendingMessageRequest *
pending_message_request_new (MilterCommand command)
{
    PendingMessageRequest *request;

    request = g_new0(PendingMessageRequest, 1);
    request->command = command;

    return request;
}

static PendingMessageRequest *
pending_header_request_new (const gchar *name, const gchar *value)
{
    PendingMessageRequest *request;

    request = pending_message_request_new(MILTER_COMMAND_HEADER);
    request->arguments.header.name = g_strdup(name);
    request->arguments.header.value = g_strdup(value);

    return request;
}

static PendingMessageRequest *
pending_end_of_header_request_new (void)
{
    PendingMessageRequest *request;

    request = pending_message_request_new(MILTER_COMMAND_END_OF_HEADER);

    return request;
}

static PendingMessageRequest *
pending_body_request_new (const gchar *chunk, gsize size)
{
    PendingMessageRequest *request;

    request = pending_message_request_new(MILTER_COMMAND_BODY);
    request->arguments.body.chunk = g_strndup(chunk, size);
    request->arguments.body.size = size;

    return request;
}

static PendingMessageRequest *
pending_end_of_message_request_new (const gchar *chunk, gsize size)
{
    PendingMessageRequest *request;

    request = pending_message_request_new(MILTER_COMMAND_END_OF_MESSAGE);
    request->arguments.end_of_message.chunk = g_strndup(chunk, size);
    request->arguments.end_of_message.size = size;

    return request;
}

static void
pending_message_request_free (PendingMessageRequest *request)
{
    switch (request->command) {
    case MILTER_COMMAND_HEADER:
        g_free(request->arguments.header.name);
        g_free(request->arguments.header.value);
        break;
    case MILTER_COMMAND_BODY:
        g_free(request->arguments.body.chunk);
        break;
    case MILTER_COMMAND_END_OF_MESSAGE:
        g_free(request->arguments.end_of_message.chunk);
        break;
    default:
        break;
    }
    g_free(request);
}

static void
dispose_pending_message_request (MilterManagerChildrenPrivate *priv)
{
    if (priv->pending_message_request) {
        pending_message_request_free(priv->pending_message_request);
        priv->pending_message_request = NULL;
    }
}

static void
dispose_body_related_data (MilterManagerChildrenPrivate *priv)
{
    priv->emitted_reply_for_message_oriented_command = FALSE;

    if (priv->body) {
        g_string_free(priv->body, TRUE);
        priv->body = NULL;
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
}

static void
dispose_reply_related_data (MilterManagerChildrenPrivate *priv)
{
    priv->reply_code = 0;

    if (priv->reply_extended_code) {
        g_free(priv->reply_extended_code);
        priv->reply_extended_code = NULL;
    }

    if (priv->reply_message) {
        g_free(priv->reply_message);
        priv->reply_message = NULL;
    }
}

static void
dispose_message_related_data (MilterManagerChildrenPrivate *priv)
{
    dispose_pending_message_request(priv);

    if (priv->command_waiting_child_queue) {
        g_list_free(priv->command_waiting_child_queue);
        priv->command_waiting_child_queue = NULL;
    }

    if (priv->command_queue) {
        g_list_free(priv->command_queue);
        priv->command_queue = NULL;
    }

    if (priv->original_headers) {
        g_object_unref(priv->original_headers);
        priv->original_headers = NULL;
    }

    if (priv->headers) {
        g_object_unref(priv->headers);
        priv->headers = NULL;
    }

    dispose_body_related_data(priv);

    if (priv->end_of_message_chunk) {
        g_free(priv->end_of_message_chunk);
        priv->end_of_message_chunk = NULL;
    }

    if (priv->change_from) {
        g_free(priv->change_from);
        priv->change_from = NULL;
    }

    if (priv->change_from_parameters) {
        g_free(priv->change_from_parameters);
        priv->change_from_parameters = NULL;
    }

    if (priv->quarantine_reason) {
        g_free(priv->quarantine_reason);
        priv->quarantine_reason = NULL;
    }
}

static void
dispose_smtp_client_address (MilterManagerChildrenPrivate *priv)
{
    if (priv->smtp_client_address) {
        g_free(priv->smtp_client_address);
        priv->smtp_client_address = NULL;
    }
    priv->smtp_client_address_length = 0;
}

static void
dispose (GObject *object)
{
    MilterManagerChildrenPrivate *priv;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(object);

    milter_debug("[%u] [children][dispose]", priv->tag);

    dispose_lazy_reply_negotiate_id(priv);

    if (priv->reply_queue) {
        g_queue_free(priv->reply_queue);
        priv->reply_queue = NULL;
    }

    if (priv->try_negotiate_ids) {
        g_hash_table_unref(priv->try_negotiate_ids);
        priv->try_negotiate_ids = NULL;
    }

    dispose_smtp_client_address(priv);

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

    dispose_reply_related_data(priv);
    dispose_message_related_data(priv);

    milter_manager_children_set_launcher_channel(MILTER_MANAGER_CHILDREN(object),
                                                 NULL, NULL);

    if (priv->event_loop) {
        g_object_unref(priv->event_loop);
        priv->event_loop = NULL;
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
    case PROP_TAG:
        milter_manager_children_set_tag(MILTER_MANAGER_CHILDREN(object),
                                        g_value_get_uint(value));
        break;
    case PROP_EVENT_LOOP:
        if (priv->event_loop)
            g_object_unref(priv->event_loop);
        priv->event_loop = g_value_dup_object(value);
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
    case PROP_TAG:
        g_value_set_uint(value, priv->tag);
        break;
    case PROP_EVENT_LOOP:
        g_value_set_object(value, priv->event_loop);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static void
finished (MilterFinishedEmittable *emittable)
{
    MilterManagerChildren *children;
    MilterManagerChildrenPrivate *priv;

    children = MILTER_MANAGER_CHILDREN(emittable);
    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    priv->finished = TRUE;
}

GQuark
milter_manager_children_error_quark (void)
{
    return g_quark_from_static_string("milter-manager-children-error-quark");
}

MilterManagerChildren *
milter_manager_children_new (MilterManagerConfiguration *configuration,
                             MilterEventLoop *event_loop)
{
    return g_object_new(MILTER_TYPE_MANAGER_CHILDREN,
                        "configuration", configuration,
                        "event-loop", event_loop,
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
    milter_agent_set_event_loop(MILTER_AGENT(child), priv->event_loop);
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
    NegotiateData *negotiate_data = (NegotiateData *)user_data;
    MilterManagerChildrenPrivate *priv;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(negotiate_data->children);

    milter_debug("[%u] [children][milter][start] [%u] %s",
                 priv->tag,
                 milter_agent_get_tag(MILTER_AGENT(context)),
                 milter_server_context_get_name(context));

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

    if (priv->option) {
        milter_option_merge(priv->option, option);
        priv->requested_yes_steps |= milter_option_get_step_yes(option);
        priv->negotiated = TRUE;
    } else {
        GError *error;

        error = g_error_new(MILTER_MANAGER_CHILDREN_ERROR,
                            MILTER_MANAGER_CHILDREN_ERROR_NO_NEGOTIATION,
                            "[%u] negotiation isn't started but "
                            "negotiation response is arrived: %s",
                            milter_agent_get_tag(MILTER_AGENT(context)),
                            milter_server_context_get_name(context));
        milter_error("[%u] [children][error][negotiate] %s",
                     priv->tag, error->message);
        milter_error_emittable_emit(MILTER_ERROR_EMITTABLE(children), error);
        g_error_free(error);
    }

    remove_queue_in_negotiate(children, MILTER_MANAGER_CHILD(context));
}

static void
report_result (MilterManagerChildren *children,
               MilterServerContext *context)
{
    MilterManagerChildrenPrivate *priv;
    MilterStatus status;
    gdouble elapsed;
    MilterServerContextState state, last_state;
    gchar *status_name, *statistic_status_name;
    gchar *state_name, *last_state_name;
    const gchar *child_name;
    guint tag;

    if (!(milter_need_log(MILTER_LOG_LEVEL_DEBUG |
                          MILTER_LOG_LEVEL_STATISTICS)))
        return;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);


    child_name = milter_server_context_get_name(context);

    state = milter_server_context_get_state(context);
    state_name = milter_utils_get_enum_nick_name(
        MILTER_TYPE_SERVER_CONTEXT_STATE, state);
    last_state = milter_server_context_get_last_state(context);
    last_state_name =
        milter_utils_get_enum_nick_name(MILTER_TYPE_SERVER_CONTEXT_STATE,
                                        last_state);
    status = milter_server_context_get_status(context);
    status_name = milter_utils_get_enum_nick_name(MILTER_TYPE_STATUS, status);
    tag = milter_agent_get_tag(MILTER_AGENT(context));
    elapsed = milter_server_context_get_elapsed(context);
    milter_debug("[%u] [children][milter][end][%s][%s][%s][%g] [%u] %s",
                 priv->tag,
                 state_name, last_state_name,
                 status_name,
                 elapsed,
                 tag, child_name);
    if (MILTER_STATUS_IS_PASS(status)) {
        statistic_status_name = "pass";
    } else {
        statistic_status_name = status_name;
    }
    milter_statistics("[milter][end][%s][%s][%g](%u): %s",
                      last_state_name, statistic_status_name,
                      elapsed, tag, child_name);
    g_free(status_name);
    g_free(state_name);
    g_free(last_state_name);
}

static void
expire_child (MilterManagerChildren *children,
              MilterServerContext *context)
{
    report_result(children, context);
    milter_server_context_set_quitted(context, TRUE);
    teardown_server_context_signals(MILTER_MANAGER_CHILD(context), children);
}

static void
abort_all_children (MilterManagerChildren *children)
{
    milter_manager_children_abort(children);
}

static void
emit_reply_status_of_state (MilterManagerChildren *children,
                            MilterServerContextState state)
{
    MilterManagerChildrenPrivate *priv;
    MilterStatus status;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);
    status = get_reply_status_for_state(children, state);

    if ((((priv->reply_code / 100) == 4) &&
         status == MILTER_STATUS_TEMPORARY_FAILURE) ||
        (((priv->reply_code / 100) == 5) &&
         status == MILTER_STATUS_REJECT)) {
        g_signal_emit_by_name(children, "reply-code",
                              priv->reply_code, priv->reply_extended_code,
                              priv->reply_message);
        dispose_reply_related_data(priv);
    } else {
        if (status != MILTER_STATUS_NOT_CHANGE) {
            g_signal_emit_by_name(children,
                                  status_to_signal_name(status));
        }
    }

    if (state == MILTER_SERVER_CONTEXT_STATE_END_OF_MESSAGE) {
        MilterManagerChildrenPrivate *priv;

        /* FIXME: emit message-processed signal for logging */
        priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);
        dispose_message_related_data(priv);
    }
}

gboolean
milter_manager_children_is_waiting_reply (MilterManagerChildren *children)
{
    MilterManagerChildrenPrivate *priv;
    MilterServerContext *current_child;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);
    switch (priv->state) {
    case MILTER_SERVER_CONTEXT_STATE_NEGOTIATE:
    case MILTER_SERVER_CONTEXT_STATE_CONNECT:
    case MILTER_SERVER_CONTEXT_STATE_HELO:
    case MILTER_SERVER_CONTEXT_STATE_ENVELOPE_FROM:
    case MILTER_SERVER_CONTEXT_STATE_ENVELOPE_RECIPIENT:
    case MILTER_SERVER_CONTEXT_STATE_DATA:
    case MILTER_SERVER_CONTEXT_STATE_UNKNOWN:
        return !g_queue_is_empty(priv->reply_queue);
        break;
    case MILTER_SERVER_CONTEXT_STATE_HEADER:
    case MILTER_SERVER_CONTEXT_STATE_END_OF_HEADER:
    case MILTER_SERVER_CONTEXT_STATE_BODY:
    case MILTER_SERVER_CONTEXT_STATE_END_OF_MESSAGE:
        current_child = get_first_child_in_command_waiting_child_queue(children);
        if (!current_child)
            return FALSE;
        return milter_server_context_is_processing(current_child);
        break;
    default:
        return FALSE;
        break;
    }
}

static gboolean
process_pending_message_request (MilterManagerChildren *children)
{
    gboolean processed = FALSE;
    MilterManagerChildrenPrivate *priv;
    PendingMessageRequest *request;
    gchar *command_name = NULL;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);
    request = priv->pending_message_request;

    if (milter_need_debug_log()) {
        command_name = milter_utils_get_enum_nick_name(
            MILTER_TYPE_COMMAND, request->command);
        milter_debug("[%u] [children][pending-message-request][%s][process]",
                     priv->tag, command_name);
    }

    switch (request->command) {
    case MILTER_COMMAND_HEADER:
        processed =
            milter_manager_children_header(children,
                                           request->arguments.header.name,
                                           request->arguments.header.value);
        break;
    case MILTER_COMMAND_END_OF_HEADER:
        processed = milter_manager_children_end_of_header(children);
        break;
    case MILTER_COMMAND_BODY:
        processed =
            milter_manager_children_body(children,
                                         request->arguments.body.chunk,
                                         request->arguments.body.size);
        break;
    case MILTER_COMMAND_END_OF_MESSAGE:
        processed =
            milter_manager_children_end_of_message(
                children,
                request->arguments.end_of_message.chunk,
                request->arguments.end_of_message.size);
        break;
    default:
        if (milter_need_error_log()) {
            if (!command_name) {
                command_name = milter_utils_get_enum_nick_name(
                    MILTER_TYPE_COMMAND, request->command);
            }
            milter_error("[%u] [children][error][pending-message-request] "
                         "unknown command: <%s>(%d)",
                         priv->tag, command_name, request->command);
        }
        break;
    }

    if (command_name)
        g_free(command_name);

    return processed;
}

static MilterStatus
remove_child_from_queue (MilterManagerChildren *children,
                         MilterServerContext *context)
{
    MilterManagerChildrenPrivate *priv;
    MilterStatus status = MILTER_STATUS_PROGRESS;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    g_queue_remove(priv->reply_queue, context);

    if (!g_queue_is_empty(priv->reply_queue))
        return status;

    status = get_reply_status_for_state(children, priv->processing_state);
    if (priv->pending_message_request) {
        if (status == MILTER_STATUS_CONTINUE &&
            process_pending_message_request(children)) {
            status = MILTER_STATUS_PROGRESS;
        }
        dispose_pending_message_request(priv);
        if (status == MILTER_STATUS_PROGRESS)
            return status;
    }
    emit_reply_status_of_state(children, priv->processing_state);

    switch (status) {
    case MILTER_STATUS_REJECT:
    case MILTER_STATUS_TEMPORARY_FAILURE:
        if (priv->processing_state !=
            MILTER_SERVER_CONTEXT_STATE_ENVELOPE_RECIPIENT) {
            abort_all_children(children);
        }
        break;
    case MILTER_STATUS_DISCARD:
        abort_all_children(children);
        break;
    default:
        break;
    }

    return status;
}

static gboolean
emit_replace_body_signal_file (MilterManagerChildren *children)
{
    MilterManagerChildrenPrivate *priv;
    GError *error = NULL;
    GIOStatus status = G_IO_STATUS_NORMAL;
    gsize chunk_size;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    g_io_channel_seek_position(priv->body_file, 0, G_SEEK_SET, &error);
    if (error) {
        milter_error("[%u] [children][error][body][read][seek] %s",
                     priv->tag, error->message);
        milter_error_emittable_emit(MILTER_ERROR_EMITTABLE(children),
                                    error);
        g_error_free(error);

        return FALSE;
    }

    chunk_size =
        milter_manager_configuration_get_chunk_size(priv->configuration);
    while (status == G_IO_STATUS_NORMAL) {
        gchar buffer[MILTER_CHUNK_SIZE + 1];
        gsize read_size;

        status = g_io_channel_read_chars(priv->body_file,
                                         buffer, chunk_size,
                                         &read_size, &error);

        if (status == G_IO_STATUS_NORMAL)
            g_signal_emit_by_name(children, "replace-body", buffer, read_size);
    }

    if (error) {
        milter_error("[%u] [children][error][body][read] %s",
                     priv->tag, error->message);
        milter_error_emittable_emit(MILTER_ERROR_EMITTABLE(children),
                                    error);
        g_error_free(error);

        return FALSE;
    }

    return TRUE;
}

static gboolean
emit_replace_body_signal_string (MilterManagerChildren *children)
{
    MilterManagerChildrenPrivate *priv;
    gsize offset, chunk_size, write_size;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    chunk_size =
        milter_manager_configuration_get_chunk_size(priv->configuration);
    for (offset = 0; offset < priv->body->len; offset += write_size) {
        write_size = MIN(priv->body->len - offset, chunk_size);
        g_signal_emit_by_name(children, "replace-body",
                              priv->body->str + offset,
                              write_size);
    }

    return TRUE;
}

static gboolean
emit_replace_body_signal (MilterManagerChildren *children)
{
    MilterManagerChildrenPrivate *priv;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);
    if (priv->body)
        return emit_replace_body_signal_string(children);
    else
        return emit_replace_body_signal_file(children);
}

static MilterStatus
send_command_to_child (MilterManagerChildren *children,
                       MilterServerContext *context,
                       MilterCommand command)
{
    MilterManagerChildrenPrivate *priv;
    MilterManagerChild *child;
    MilterStatus status;

    child = MILTER_MANAGER_CHILD(context);
    status = milter_manager_child_get_fallback_status(child);

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);
    switch (command) {
    case MILTER_COMMAND_HEADER:
        status = send_next_header_to_child(children, context);
        break;
    case MILTER_COMMAND_END_OF_HEADER:
        priv->processing_state = MILTER_SERVER_CONTEXT_STATE_END_OF_HEADER;
        if (milter_server_context_end_of_header(context)) {
            status = MILTER_STATUS_PROGRESS;
            if (!milter_server_context_need_reply(context,
                                                 priv->processing_state)) {
                g_signal_emit_by_name(context, "continue");
            }
        }
        break;
    case MILTER_COMMAND_BODY:
        status = send_body_to_child(children, context);
        break;
    case MILTER_COMMAND_END_OF_MESSAGE:
        priv->processing_header_index = 0;
        priv->processing_state = MILTER_SERVER_CONTEXT_STATE_END_OF_MESSAGE;
        if (milter_server_context_end_of_message(context,
                                                 priv->end_of_message_chunk,
                                                 priv->end_of_message_size))
            status = MILTER_STATUS_PROGRESS;
        break;
    default:
        break;
    }

    return status;
}

static void
emit_header_signals (MilterManagerChildren *children)
{
    MilterManagerChildrenPrivate *priv;
    const GList *header_list, *node;
    MilterHeaders *processing_headers;
    gint i;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    processing_headers = milter_headers_copy(priv->original_headers);
    header_list = milter_headers_get_list(priv->headers);

    /* FIXME the following process admits improvement. */
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

        found_header = milter_headers_lookup_by_name(processing_headers,
                                                     header->name);
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

        for (node = g_list_last((GList *)header_list);
             node;
             node = g_list_previous(node)) {
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

static void
emit_signals_on_end_of_message (MilterManagerChildren *children)
{
    MilterManagerChildrenPrivate *priv;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);
    if (priv->replaced_body)
        emit_replace_body_signal(children);

    if (priv->original_headers)
        emit_header_signals(children);

    if (priv->change_from)
        g_signal_emit_by_name(children, "change-from",
                              priv->change_from, priv->change_from_parameters);
    if (priv->quarantine_reason)
        g_signal_emit_by_name(children, "quarantine", priv->quarantine_reason);
}

static void
emit_reply_for_message_oriented_command (MilterManagerChildren *children,
                                         MilterServerContextState state)
{
    MilterManagerChildrenPrivate *priv;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    if (priv->emitted_reply_for_message_oriented_command)
        return;

    priv->emitted_reply_for_message_oriented_command = TRUE;

    if (state == MILTER_SERVER_CONTEXT_STATE_END_OF_MESSAGE)
        emit_signals_on_end_of_message(children);

    emit_reply_status_of_state(children, state);
}

static MilterCommand
fetch_first_command_for_child_in_queue (MilterServerContext *child,
                                        GList **queue)
{
    GList *node;

    for (node = *queue; node; node = g_list_next(node)) {
        MilterCommand next_command;

        next_command = GPOINTER_TO_INT(node->data);
        return next_command;
    }

    return -1;
}

static MilterStatus
send_first_command_to_next_child (MilterManagerChildren *children,
                                  MilterServerContext *context)
{
    MilterManagerChildrenPrivate *priv;
    MilterServerContext *next_child;
    MilterCommand first_command;

    /* FIXME: don't want to return PROGRESS. */
    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    priv->processing_header_index = 0;
    priv->command_waiting_child_queue =
        g_list_remove(priv->command_waiting_child_queue, context);

    next_child = get_first_child_in_command_waiting_child_queue(children);
    if (!next_child) {
        emit_reply_for_message_oriented_command(children, priv->state);
        return MILTER_STATUS_PROGRESS;
    }

    first_command = fetch_first_command_for_child_in_queue(next_child,
                                                           &priv->command_queue);
    if (first_command == -1)
        g_signal_emit_by_name(children, "continue");
    else
        send_command_to_child(children, next_child, first_command);

    return MILTER_STATUS_PROGRESS;
}

static void
handle_status (MilterManagerChildren *children,
               MilterStatus status)
{
    if (status == MILTER_STATUS_PROGRESS)
        return;

    g_signal_emit_by_name(children, status_to_signal_name(status));
}

static void
cb_continue (MilterServerContext *context, gpointer user_data)
{
    MilterManagerChildren *children = user_data;
    MilterServerContextState state;
    MilterManagerChildrenPrivate *priv;
    MilterStatus status = MILTER_STATUS_NOT_CHANGE;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    state = milter_server_context_get_state(context);
    compile_reply_status(children, state, MILTER_STATUS_CONTINUE);

    switch (state) {
    case MILTER_SERVER_CONTEXT_STATE_END_OF_HEADER:
        status = send_next_command(children, context, state);
        break;
    case MILTER_SERVER_CONTEXT_STATE_HEADER:
        if (priv->state < state) {
            status = MILTER_STATUS_NOT_CHANGE;
        } else {
            if (priv->processing_header_index <
                milter_headers_length(priv->headers)) {
                status = send_next_header_to_child(children, context);
            } else {
                status = send_next_command(children, context, state);
            }
        }
        break;
    case MILTER_SERVER_CONTEXT_STATE_BODY:
        if (priv->state < state) {
            status = MILTER_STATUS_NOT_CHANGE;
        } else {
            if (priv->sending_body)
                status = send_body_to_child(children, context);
            if (status == MILTER_STATUS_NOT_CHANGE)
                status = send_next_command(children, context, state);
        }
        break;
    case MILTER_SERVER_CONTEXT_STATE_END_OF_MESSAGE:
        status = send_first_command_to_next_child(children, context);
        break;
    default:
        remove_child_from_queue(children, context);
        status = MILTER_STATUS_PROGRESS;
        break;
    }
    handle_status(children, status);
}

static void
cb_temporary_failure (MilterServerContext *context, gpointer user_data)
{
    MilterManagerChildren *children = user_data;
    MilterManagerChildrenPrivate *priv;
    MilterServerContextState state;
    MilterStatus status = MILTER_STATUS_TEMPORARY_FAILURE;
    gboolean evaluation_mode;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);
    state = milter_server_context_get_state(context);

    evaluation_mode =
        milter_manager_child_is_evaluation_mode(MILTER_MANAGER_CHILD(context));
    if (evaluation_mode) {
        status = MILTER_STATUS_ACCEPT;
        if (milter_need_debug_log()) {
            gchar *state_name;
            state_name = milter_utils_get_enum_nick_name(
                MILTER_TYPE_SERVER_CONTEXT_STATE, state);
            milter_debug("[%u] [children][evaluation][temporary-failure][%s] "
                         "[%u] %s",
                         priv->tag,
                         state_name,
                         milter_agent_get_tag(MILTER_AGENT(context)),
                         milter_server_context_get_name(context));
            g_free(state_name);
        }
    }
    compile_reply_status(children, state, status);

    switch (state) {
    case MILTER_SERVER_CONTEXT_STATE_CONNECT:
    case MILTER_SERVER_CONTEXT_STATE_HELO:
        /* FIXME: should expire all children? */
        milter_server_context_quit(context);
        break;
    case MILTER_SERVER_CONTEXT_STATE_ENVELOPE_FROM:
    case MILTER_SERVER_CONTEXT_STATE_DATA:
        milter_server_context_set_processing_message(context, FALSE);
        remove_child_from_queue(children, context);
        break;
    case MILTER_SERVER_CONTEXT_STATE_ENVELOPE_RECIPIENT:
        remove_child_from_queue(children, context);
        break;
    case MILTER_SERVER_CONTEXT_STATE_HEADER:
    case MILTER_SERVER_CONTEXT_STATE_END_OF_HEADER:
    case MILTER_SERVER_CONTEXT_STATE_BODY:
    case MILTER_SERVER_CONTEXT_STATE_END_OF_MESSAGE:
        milter_server_context_set_processing_message(context, FALSE);
        emit_reply_for_message_oriented_command(children, state);
        milter_manager_children_abort(children);
        break;
    default:
        if (milter_need_error_log()) {
            gchar *state_name;
            state_name = milter_utils_get_enum_nick_name(
                MILTER_TYPE_SERVER_CONTEXT_STATE, state);
            milter_error("[%u] [children][error][invalid-state]"
                         "[temporary-failure][%s] [%u] %s",
                         priv->tag,
                         state_name,
                         milter_agent_get_tag(MILTER_AGENT(context)),
                         milter_server_context_get_name(context));
            g_free(state_name);
        }
        milter_server_context_quit(context);
        break;
    }
}

static void
cb_reject (MilterServerContext *context, gpointer user_data)
{
    MilterManagerChildren *children = user_data;
    MilterManagerChildrenPrivate *priv;
    MilterServerContextState state;
    MilterStatus status = MILTER_STATUS_REJECT;
    gboolean evaluation_mode;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);
    state = milter_server_context_get_state(context);

    evaluation_mode =
        milter_manager_child_is_evaluation_mode(MILTER_MANAGER_CHILD(context));
    if (evaluation_mode) {
        status = MILTER_STATUS_ACCEPT;
        if (milter_need_debug_log()) {
            gchar *state_name;
            state_name = milter_utils_get_enum_nick_name(
                MILTER_TYPE_SERVER_CONTEXT_STATE, state);
            milter_debug("[%u] [children][evaluation][reject][%s] [%u] %s",
                         priv->tag,
                         state_name,
                         milter_agent_get_tag(MILTER_AGENT(context)),
                         milter_server_context_get_name(context));
            g_free(state_name);
        }
    }
    compile_reply_status(children, state, status);

    switch (state) {
    case MILTER_SERVER_CONTEXT_STATE_CONNECT:
    case MILTER_SERVER_CONTEXT_STATE_HELO:
        /* FIXME: should expire all children? */
        milter_server_context_quit(context);
        break;
    case MILTER_SERVER_CONTEXT_STATE_ENVELOPE_FROM:
    case MILTER_SERVER_CONTEXT_STATE_DATA:
        milter_server_context_set_processing_message(context, FALSE);
        remove_child_from_queue(children, context);
        break;
    case MILTER_SERVER_CONTEXT_STATE_ENVELOPE_RECIPIENT:
        remove_child_from_queue(children, context);
        break;
    case MILTER_SERVER_CONTEXT_STATE_HEADER:
    case MILTER_SERVER_CONTEXT_STATE_END_OF_HEADER:
    case MILTER_SERVER_CONTEXT_STATE_BODY:
    case MILTER_SERVER_CONTEXT_STATE_END_OF_MESSAGE:
        milter_server_context_set_processing_message(context, FALSE);
        emit_reply_for_message_oriented_command(children, state);
        milter_manager_children_abort(children);
        break;
    default:
        if (milter_need_error_log()) {
            gchar *state_name;
            state_name = milter_utils_get_enum_nick_name(
                MILTER_TYPE_SERVER_CONTEXT_STATE, state);
            milter_error("[%u] [children][error][invalid-state][reject][%s] "
                         "[%u] %s",
                         priv->tag,
                         state_name,
                         milter_agent_get_tag(MILTER_AGENT(context)),
                         milter_server_context_get_name(context));
            g_free(state_name);
        }
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

    dispose_reply_related_data(priv);
    priv->reply_code = code;
    priv->reply_extended_code = g_strdup(extended_code);
    priv->reply_message = g_strdup(message);

    if ((priv->reply_code / 100) == 4) {
        cb_temporary_failure(context, user_data);
    } else {
        cb_reject(context, user_data);
    }
}

static void
cb_accept (MilterServerContext *context, gpointer user_data)
{
    MilterManagerChildren *children = user_data;
    MilterManagerChildrenPrivate *priv;
    MilterServerContextState state;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);
    state = milter_server_context_get_state(context);

    compile_reply_status(children, state, MILTER_STATUS_ACCEPT);
    switch (state) {
    case MILTER_SERVER_CONTEXT_STATE_CONNECT:
    case MILTER_SERVER_CONTEXT_STATE_HELO:
        milter_server_context_quit(context);
        break;
    case MILTER_SERVER_CONTEXT_STATE_ENVELOPE_FROM:
    case MILTER_SERVER_CONTEXT_STATE_DATA:
        milter_server_context_set_processing_message(context, FALSE);
        remove_child_from_queue(children, context);
        break;
    case MILTER_SERVER_CONTEXT_STATE_ENVELOPE_RECIPIENT:
        /* TODO: remove the following line: accepting message is a bug. */
        milter_server_context_set_processing_message(context, FALSE);
        remove_child_from_queue(children, context);
        break;
    case MILTER_SERVER_CONTEXT_STATE_HEADER:
    case MILTER_SERVER_CONTEXT_STATE_END_OF_HEADER:
    case MILTER_SERVER_CONTEXT_STATE_BODY:
    case MILTER_SERVER_CONTEXT_STATE_END_OF_MESSAGE:
        milter_server_context_set_processing_message(context, FALSE);
        send_first_command_to_next_child(children, context);
        break;
    default:
        if (milter_need_error_log()) {
            gchar *state_name;
            state_name = milter_utils_get_enum_nick_name(
                MILTER_TYPE_SERVER_CONTEXT_STATE, state);
            milter_error("[%u] [children][error][invalid-state][accept][%s] [%u] %s",
                         priv->tag,
                         state_name,
                         milter_agent_get_tag(MILTER_AGENT(context)),
                         milter_server_context_get_name(context));
            g_free(state_name);
        }
        milter_server_context_quit(context);
        break;
    }
}

static void
cb_discard (MilterServerContext *context, gpointer user_data)
{
    MilterManagerChildren *children = user_data;
    MilterManagerChildrenPrivate *priv;
    MilterServerContextState state;
    MilterStatus status = MILTER_STATUS_DISCARD;
    gboolean evaluation_mode;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);
    state = milter_server_context_get_state(context);

    evaluation_mode =
        milter_manager_child_is_evaluation_mode(MILTER_MANAGER_CHILD(context));
    if (evaluation_mode) {
        status = MILTER_STATUS_ACCEPT;
        if (milter_need_debug_log()) {
            gchar *state_name;
            state_name =
                milter_utils_get_enum_nick_name(
                    MILTER_TYPE_SERVER_CONTEXT_STATE, state);
            milter_debug("[%u] [children][evaluation][discard][%s] [%u] %s",
                         priv->tag,
                         state_name,
                         milter_agent_get_tag(MILTER_AGENT(context)),
                         milter_server_context_get_name(context));
            g_free(state_name);
        }
    }
    compile_reply_status(children, state, status);

    switch (state) {
    case MILTER_SERVER_CONTEXT_STATE_ENVELOPE_FROM:
    case MILTER_SERVER_CONTEXT_STATE_DATA:
        milter_server_context_set_processing_message(context, FALSE);
        remove_child_from_queue(children, context);
        break;
    case MILTER_SERVER_CONTEXT_STATE_ENVELOPE_RECIPIENT:
        remove_child_from_queue(children, context);
        break;
    case MILTER_SERVER_CONTEXT_STATE_HEADER:
    case MILTER_SERVER_CONTEXT_STATE_END_OF_HEADER:
    case MILTER_SERVER_CONTEXT_STATE_BODY:
    case MILTER_SERVER_CONTEXT_STATE_END_OF_MESSAGE:
        milter_server_context_set_processing_message(context, FALSE);
        emit_reply_for_message_oriented_command(children, state);
        milter_manager_children_abort(children);
        break;
    default:
        if (milter_need_error_log()) {
            gchar *state_name;
            state_name = milter_utils_get_enum_nick_name(
                MILTER_TYPE_SERVER_CONTEXT_STATE, state);
            milter_error("[%u] [children][error][invalid-state][discard][%s] "
                         "[%u] %s",
                         priv->tag,
                         state_name,
                         milter_agent_get_tag(MILTER_AGENT(context)),
                         milter_server_context_get_name(context));
            g_free(state_name);
        }
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
    if (priv->state < MILTER_SERVER_CONTEXT_STATE_END_OF_MESSAGE) {
        g_signal_emit_by_name(children, "continue");
    } else {
        send_next_command(children, context, state);
    }
}

static gboolean
need_header_value_leading_space_conversion (MilterManagerChildren *children,
                                            MilterServerContext *context)
{
    MilterManagerChildrenPrivate *priv;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    if (milter_server_context_is_enable_step(
            context,
            MILTER_STEP_HEADER_VALUE_WITH_LEADING_SPACE))
        return FALSE;

    return (milter_option_get_step(priv->option) &
            MILTER_STEP_HEADER_VALUE_WITH_LEADING_SPACE);
}

static gchar *
normalize_header_value (MilterManagerChildren *children,
                        MilterServerContext *context,
                        const gchar *value)
{
    if (need_header_value_leading_space_conversion(children, context)) {
        if (value && value[0] != ' ')
            return g_strconcat(" ", value, NULL);
    }

    return NULL;
}

static gboolean
is_end_of_message_state (MilterManagerChildren *children,
                         MilterServerContext *context,
                         const gchar *requested_action_name,
                         const gchar *format,
                         ...)
{
    MilterManagerChildrenPrivate *priv;
    MilterServerContextState state;
    va_list args;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);
    state = milter_server_context_get_state(context);

    if (state == MILTER_SERVER_CONTEXT_STATE_END_OF_MESSAGE)
        return TRUE;

    if (milter_need_error_log()) {
        gchar *state_name;
        gchar *additional_info = NULL;

        if (format) {
            va_start(args, format);
            additional_info = g_strdup_vprintf(format, args);
            va_end(args);
        }

        state_name =
            milter_utils_get_enum_nick_name(MILTER_TYPE_SERVER_CONTEXT_STATE,
                                            state);
        milter_error("[%u] [children][error][invalid-state][%s][%s]%s "
                     "[%u] only allowed in end of message session: %s",
                     priv->tag,
                     requested_action_name,
                     state_name,
                     additional_info ? additional_info : "",
                     milter_agent_get_tag(MILTER_AGENT(context)),
                     milter_server_context_get_name(context));
        g_free(state_name);

        if (additional_info)
            g_free(additional_info);

    }

    milter_server_context_quit(context);
    return FALSE;
}

static gboolean
is_evaluation_mode (MilterManagerChildren *children,
                    MilterServerContext *context,
                    const gchar *requested_action_name,
                    const gchar *format,
                    ...)
{
    MilterManagerChildrenPrivate *priv;
    va_list args;
    GString *additional_info = NULL;

    if (!milter_manager_child_is_evaluation_mode(MILTER_MANAGER_CHILD(context)))
        return FALSE;

    if (!milter_need_debug_log())
        return TRUE;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    if (format) {
        gchar *additional_info_content;

        additional_info = g_string_new(" ");
        va_start(args, format);
        additional_info_content = g_strdup_vprintf(format, args);
        va_end(args);
        g_string_append(additional_info, additional_info_content);
        g_free(additional_info_content);
    }

    milter_debug("[%u] [children][evaluation][%s]%s [%u] %s",
                 priv->tag,
                 requested_action_name,
                 additional_info ? additional_info->str : "",
                 milter_agent_get_tag(MILTER_AGENT(context)),
                 milter_server_context_get_name(context));
    if (additional_info)
        g_string_free(additional_info, TRUE);

    return TRUE;
}

static void
cb_add_header (MilterServerContext *context,
               const gchar *name, const gchar *value,
               gpointer user_data)
{
    MilterManagerChildren *children = user_data;
    MilterManagerChildrenPrivate *priv;
    gchar *normalized_value = NULL;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    if (!is_end_of_message_state(children, context, "add-header",
                                 "<%s>=<%s>",
                                 name,
                                 MILTER_LOG_NULL_SAFE_STRING(value)))
        return;

    if (value) {
        milter_statistics("[milter][header][add](%u): <%s>=<%s>: %s",
                          milter_agent_get_tag(MILTER_AGENT(context)),
                          name,
                          value,
                          milter_server_context_get_name(context));
    }

    if (is_evaluation_mode(children, context, "add-header",
                           "<%s>=<%s>",
                           name,
                           MILTER_LOG_NULL_SAFE_STRING(value)))
        return;

    normalized_value = normalize_header_value(children, context, value);
    milter_headers_add_header(priv->headers, name,
                              normalized_value ? normalized_value : value);

    if (normalized_value)
        g_free(normalized_value);
}

static void
cb_insert_header (MilterServerContext *context,
                  guint32 index, const gchar *name, const gchar *value,
                  gpointer user_data)
{
    MilterManagerChildren *children = user_data;
    MilterManagerChildrenPrivate *priv;
    gchar *normalized_value = NULL;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    if (!is_end_of_message_state(children, context, "insert-header",
                                 "[%u]<%s>=<%s>",
                                 index,
                                 name,
                                 MILTER_LOG_NULL_SAFE_STRING(value)))
        return;

    if (value) {
        milter_statistics("[milter][header][add](%u): <%s>=<%s>: %s",
                          milter_agent_get_tag(MILTER_AGENT(context)),
                          name,
                          value,
                          milter_server_context_get_name(context));
    }

    if (is_evaluation_mode(children, context, "insert-header",
                           "[%u]<%s>=<%s>",
                           index,
                           name,
                           MILTER_LOG_NULL_SAFE_STRING(value)))
        return;

    normalized_value = normalize_header_value(children, context, value);
    milter_headers_insert_header(priv->headers, index, name,
                                 normalized_value ? normalized_value : value);

    if (normalized_value)
        g_free(normalized_value);
}

static void
cb_change_header (MilterServerContext *context,
                  const gchar *name, guint32 index, const gchar *value,
                  gpointer user_data)
{
    MilterManagerChildren *children = user_data;
    MilterManagerChildrenPrivate *priv;
    gchar *normalized_value = NULL;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    if (!is_end_of_message_state(children, context, "change-header",
                                 "<%s>[%u]=<%s>",
                                 name,
                                 index,
                                 MILTER_LOG_NULL_SAFE_STRING(value)))
        return;

    if (value) {
        milter_statistics("[milter][header][add](%u): <%s>=<%s>: %s",
                          milter_agent_get_tag(MILTER_AGENT(context)),
                          name,
                          value,
                          milter_server_context_get_name(context));
    }

    if (is_evaluation_mode(children, context, "change-header",
                           "<%s>[%u]=<%s>",
                           name,
                           index,
                           MILTER_LOG_NULL_SAFE_STRING(value)))
        return;

    normalized_value = normalize_header_value(children, context, value);
    milter_headers_change_header(priv->headers,
                                 name, index,
                                 normalized_value ? normalized_value : value);

    if (normalized_value)
        g_free(normalized_value);
}

static void
cb_delete_header (MilterServerContext *context,
                  const gchar *name, guint32 index,
                  gpointer user_data)
{
    MilterManagerChildren *children = user_data;
    MilterManagerChildrenPrivate *priv;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    if (!is_end_of_message_state(children, context, "delete-header",
                                 "<%s>[%u]", name, index))
        return;

    if (is_evaluation_mode(children, context, "delete-header",
                           "<%s>[%u]", name, index))
        return;

    milter_headers_delete_header(priv->headers, name, index);
}

static void
cb_change_from (MilterServerContext *context,
                const gchar *from, const gchar *parameters,
                gpointer user_data)
{
    MilterManagerChildren *children = user_data;
    MilterManagerChildrenPrivate *priv;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    if (!is_end_of_message_state(children, context, "change-from",
                                 "<<%s> <%s>>",
                                 from,
                                 MILTER_LOG_NULL_SAFE_STRING(parameters)))
        return;

    if (is_evaluation_mode(children, context, "change-from",
                           "<<%s> <%s>>",
                           from,
                           MILTER_LOG_NULL_SAFE_STRING(parameters)))
        return;

    if (priv->change_from)
        g_free(priv->change_from);
    if (priv->change_from_parameters)
        g_free(priv->change_from_parameters);
    priv->change_from = g_strdup(from);
    priv->change_from_parameters = g_strdup(parameters);
}

static void
cb_add_recipient (MilterServerContext *context,
                  const gchar *recipient, const gchar *parameters,
                  gpointer user_data)
{
    MilterManagerChildren *children = user_data;

    if (!is_end_of_message_state(children, context, "add-recipient",
                                 "<<%s> <%s>>",
                                 recipient,
                                 MILTER_LOG_NULL_SAFE_STRING(parameters)))
        return;

    if (is_evaluation_mode(children, context, "add-recipient",
                           "<<%s> <%s>>",
                           recipient,
                           MILTER_LOG_NULL_SAFE_STRING(parameters)))
        return;

    g_signal_emit_by_name(children, "add-recipient", recipient, parameters);
}

static void
cb_delete_recipient (MilterServerContext *context,
                     const gchar *recipient,
                     gpointer user_data)
{
    MilterManagerChildren *children = user_data;

    if (!is_end_of_message_state(children, context, "delete-recipient",
                                 "<%s>", recipient))
        return;

    if (is_evaluation_mode(children, context, "delete-recipient",
                           "<%s>", recipient))
        return;

    g_signal_emit_by_name(children, "delete-recipient", recipient);
}

static void
cb_replace_body (MilterServerContext *context,
                 const gchar *chunk, gsize chunk_size,
                 gpointer user_data)
{
    MilterManagerChildren *children = user_data;
    MilterManagerChildrenPrivate *priv;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    if (!is_end_of_message_state(children, context, "replace-body",
                                 "<%" G_GSIZE_FORMAT ">", chunk_size))
        return;

    if (is_evaluation_mode(children, context, "replace-body",
                           "<%" G_GSIZE_FORMAT ">", chunk_size))
        return;

    if (!priv->replaced_body_for_each_child)
        dispose_body_related_data(priv);

    if (!write_body(children, chunk, chunk_size))
        return;

    priv->replaced_body_for_each_child = TRUE;
    priv->replaced_body = TRUE;
}

static void
cb_progress (MilterServerContext *context, gpointer user_data)
{
    MilterManagerChildren *children = user_data;

    if (!is_end_of_message_state(children, context, "progress", NULL))
        return;

    if (milter_need_debug_log()) {
        MilterManagerChildrenPrivate *priv;
        guint tag;
        const gchar *child_name;

        priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);
        tag = milter_agent_get_tag(MILTER_AGENT(context));
        child_name = milter_server_context_get_name(context);

        milter_debug("[%u] [children][progress] [%u] %s",
                     priv->tag, tag, child_name);
    }
    g_signal_emit_by_name(user_data, "progress");
}

static void
cb_quarantine (MilterServerContext *context,
               const gchar *reason,
               gpointer user_data)
{
    MilterManagerChildren *children = user_data;
    MilterManagerChildrenPrivate *priv;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    if (!is_end_of_message_state(children, context, "quarantine",
                                 "<%s>", reason))
        return;

    if (is_evaluation_mode(children, context, "quarantine",
                           "<%s>", reason))
        return;

    if (priv->quarantine_reason)
        g_free(priv->quarantine_reason);
    priv->quarantine_reason = g_strdup(reason);
}

static void
cb_connection_failure (MilterServerContext *context, gpointer user_data)
{
    g_signal_emit_by_name(user_data, "connection-failure");
}

static void
cb_shutdown (MilterServerContext *context, gpointer user_data)
{
    MilterManagerChildren *children = user_data;

    g_signal_emit_by_name(children, "shutdown");
}

static void
cb_stopped (MilterServerContext *context, gpointer user_data)
{
    MilterManagerChildren *children = user_data;
    MilterServerContextState state;
    MilterManagerChildrenPrivate *priv;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);
    state = milter_server_context_get_state(context);

    switch (state) {
    case MILTER_SERVER_CONTEXT_STATE_CONNECT:
    case MILTER_SERVER_CONTEXT_STATE_HELO:
        compile_reply_status(children, state, MILTER_STATUS_ACCEPT);
        milter_server_context_abort(context);
        milter_server_context_quit(context);
        break;
    case MILTER_SERVER_CONTEXT_STATE_ENVELOPE_FROM:
    case MILTER_SERVER_CONTEXT_STATE_DATA:
        compile_reply_status(children, state, MILTER_STATUS_ACCEPT);
        cb_continue(context, user_data);
        break;
    case MILTER_SERVER_CONTEXT_STATE_ENVELOPE_RECIPIENT:
        milter_server_context_set_status(context, MILTER_STATUS_NOT_CHANGE);
        cb_continue(context, user_data);
        break;
    case MILTER_SERVER_CONTEXT_STATE_HEADER:
    case MILTER_SERVER_CONTEXT_STATE_END_OF_HEADER:
    case MILTER_SERVER_CONTEXT_STATE_BODY:
    case MILTER_SERVER_CONTEXT_STATE_END_OF_MESSAGE:
        compile_reply_status(children, state, MILTER_STATUS_ACCEPT);
        milter_server_context_abort(context);
        send_first_command_to_next_child(children, context);
        break;
    default:
        if (milter_need_error_log()) {
            gchar *state_name;
            state_name = milter_utils_get_enum_nick_name(
                MILTER_TYPE_SERVER_CONTEXT_STATE, state);
            milter_error("[%u] [children][error][invalid-state][stopped][%s] "
                         "[%u] %s",
                         priv->tag,
                         state_name,
                         milter_agent_get_tag(MILTER_AGENT(context)),
                         milter_server_context_get_name(context));
            g_free(state_name);
        }

        compile_reply_status(children, state, MILTER_STATUS_TEMPORARY_FAILURE);
        milter_server_context_abort(context);
        milter_server_context_quit(context);

        break;
    }
}

static void
cb_writing_timeout (MilterServerContext *context, gpointer user_data)
{
    MilterManagerChildren *children;
    MilterManagerChildrenPrivate *priv;
    MilterManagerChild *child;
    MilterServerContextState state;
    MilterStatus fallback_status;

    children = MILTER_MANAGER_CHILDREN(user_data);
    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);
    state = milter_server_context_get_state(context);
    child = MILTER_MANAGER_CHILD(context);
    fallback_status = milter_manager_child_get_fallback_status(child);

    if (milter_need_error_log()) {
        gchar *state_name;
        gchar *fallback_status_name;

        state_name =
            milter_utils_get_enum_nick_name(MILTER_TYPE_SERVER_CONTEXT_STATE,
                                            state);
        fallback_status_name =
            milter_utils_get_enum_nick_name(MILTER_TYPE_STATUS,
                                            fallback_status);
        milter_error("[%u] [children][timeout][writing][%s][%s] [%u] %s",
                     priv->tag,
                     state_name,
                     fallback_status_name,
                     milter_agent_get_tag(MILTER_AGENT(context)),
                     milter_server_context_get_name(context));
        g_free(state_name);
        g_free(fallback_status_name);
    }

    compile_reply_status(children, state, fallback_status);
    expire_child(children, context);
    remove_child_from_queue(children, context);
}

static void
cb_reading_timeout (MilterServerContext *context, gpointer user_data)
{
    MilterManagerChildren *children;
    MilterManagerChildrenPrivate *priv;
    MilterManagerChild *child;
    MilterServerContextState state;
    MilterStatus fallback_status;

    children = MILTER_MANAGER_CHILDREN(user_data);
    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);
    state = milter_server_context_get_state(context);
    child = MILTER_MANAGER_CHILD(context);
    fallback_status = milter_manager_child_get_fallback_status(child);

    if (milter_need_error_log()) {
        gchar *state_name;
        gchar *fallback_status_name;

        state_name =
            milter_utils_get_enum_nick_name(MILTER_TYPE_SERVER_CONTEXT_STATE,
                                            state);
        fallback_status_name =
            milter_utils_get_enum_nick_name(MILTER_TYPE_STATUS,
                                            fallback_status);
        milter_error("[%u] [children][timeout][reading][%s][%s] [%u] %s",
                     priv->tag,
                     state_name,
                     fallback_status_name,
                     milter_agent_get_tag(MILTER_AGENT(context)),
                     milter_server_context_get_name(context));
        g_free(state_name);
        g_free(fallback_status_name);
    }

    compile_reply_status(children, state, fallback_status);
    expire_child(children, context);
    remove_child_from_queue(children, context);
}

static void
cb_end_of_message_timeout (MilterServerContext *context, gpointer user_data)
{
    MilterManagerChildren *children;
    MilterManagerChildrenPrivate *priv;
    MilterManagerChild *child;
    MilterServerContextState state;
    MilterStatus fallback_status;

    children = MILTER_MANAGER_CHILDREN(user_data);
    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);
    state = milter_server_context_get_state(context);
    child = MILTER_MANAGER_CHILD(context);
    fallback_status = milter_manager_child_get_fallback_status(child);

    if (milter_need_error_log()) {
        gchar *state_name;
        gchar *fallback_status_name;

        state_name =
            milter_utils_get_enum_nick_name(MILTER_TYPE_SERVER_CONTEXT_STATE,
                                            state);
        fallback_status_name =
            milter_utils_get_enum_nick_name(MILTER_TYPE_STATUS,
                                            fallback_status);
        milter_error("[%u] [children][timeout][end-of-message][%s][%s] "
                     "[%u] %s",
                     priv->tag,
                     state_name,
                     fallback_status_name,
                     milter_agent_get_tag(MILTER_AGENT(context)),
                     milter_server_context_get_name(context));
        g_free(state_name);
        g_free(fallback_status_name);
    }

    compile_reply_status(children, state, fallback_status);
    expire_child(children, context);
    remove_child_from_queue(children, context);
}

static void
cb_error (MilterErrorEmittable *emittable, GError *error, gpointer user_data)
{
    MilterServerContext *context;
    MilterManagerChildren *children;
    MilterManagerChildrenPrivate *priv;
    MilterManagerChild *child;
    MilterServerContextState state;
    MilterStatus fallback_status;

    context = MILTER_SERVER_CONTEXT(emittable);
    children = MILTER_MANAGER_CHILDREN(user_data);
    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);
    state = milter_server_context_get_state(context);
    child = MILTER_MANAGER_CHILD(context);
    fallback_status = milter_manager_child_get_fallback_status(child);

    if (milter_need_error_log()) {
        gchar *state_name;
        gchar *fallback_status_name;

        state_name =
            milter_utils_get_enum_nick_name(MILTER_TYPE_SERVER_CONTEXT_STATE,
                                            state);
        fallback_status_name =
            milter_utils_get_enum_nick_name(MILTER_TYPE_STATUS,
                                            fallback_status);
        milter_error("[%u] [children][error][%s][%s] [%u] %s: %s",
                     priv->tag,
                     state_name,
                     fallback_status_name,
                     milter_agent_get_tag(MILTER_AGENT(context)),
                     error->message,
                     milter_server_context_get_name(context));
        g_free(state_name);
        g_free(fallback_status_name);
    }

    compile_reply_status(children, state, fallback_status);
    expire_child(children, context);
    remove_child_from_queue(children, context);
}

static void
cb_finished (MilterAgent *agent, gpointer user_data)
{
    MilterManagerChildren *children;
    MilterManagerChildrenPrivate *priv;
    MilterServerContext *context = MILTER_SERVER_CONTEXT(agent);

    children = MILTER_MANAGER_CHILDREN(user_data);
    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    if (milter_server_context_is_processing(context)) {
        MilterManagerChild *child;
        MilterStatus fallback_status;
        MilterServerContextState state;

        child = MILTER_MANAGER_CHILD(context);
        fallback_status = milter_manager_child_get_fallback_status(child);
        state = milter_server_context_get_state(context);
        milter_server_context_set_status(context, fallback_status);
        compile_reply_status(children, state, fallback_status);
    }

    expire_child(children, context);

    if (milter_need_debug_log()) {
        gchar *state_name;

        state_name =
            milter_utils_get_enum_nick_name(MILTER_TYPE_SERVER_CONTEXT_STATE,
                                            /* FIXME: use server's status */
                                            priv->processing_state);
        milter_debug("[%u] [children][end][%s] [%u] %s",
                     priv->tag,
                     state_name,
                     milter_agent_get_tag(MILTER_AGENT(context)),
                     milter_server_context_get_name(context));
        g_free(state_name);
    }

    switch (priv->processing_state) {
    case MILTER_SERVER_CONTEXT_STATE_NEGOTIATE:
        remove_queue_in_negotiate(children, MILTER_MANAGER_CHILD(context));
        break;
    case MILTER_SERVER_CONTEXT_STATE_HEADER:
    case MILTER_SERVER_CONTEXT_STATE_END_OF_HEADER:
    case MILTER_SERVER_CONTEXT_STATE_BODY:
    case MILTER_SERVER_CONTEXT_STATE_END_OF_MESSAGE:
        if (!priv->emitted_reply_for_message_oriented_command &&
            milter_server_context_is_processing(context)) {
            milter_debug("[%u] [children][unexpected] [%u] "
                         "finish without receiving response: %s",
                         priv->tag,
                         milter_agent_get_tag(MILTER_AGENT(context)),
                         milter_server_context_get_name(context));
            send_first_command_to_next_child(children, context);
        }
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

    CONNECT(stopped);

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

    DISCONNECT(stopped);

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
    MilterManagerLaunchCommandEncoder *encoder;
    const gchar *packet = NULL;
    gsize packet_size;
    gchar *command;
    gchar *user_name;
    MilterManagerChildrenPrivate *priv;
    MilterServerContext *context;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);
    context = MILTER_SERVER_CONTEXT(child);

    if (!priv->launcher_writer) {
        milter_debug("[%u] [children][start-child][skip] [%u]"
                     "launcher isn't available: %s",
                     priv->tag,
                     milter_agent_get_tag(MILTER_AGENT(context)),
                     milter_server_context_get_name(context));
        return FALSE;
    }

    command = milter_manager_child_get_command_line_string(child);
    user_name = milter_manager_child_get_user_name(child);

    if (!command) {
        milter_debug("[%u] [children][start-child][skip] [%u] "
                     "command isn't set: %s",
                     priv->tag,
                     milter_agent_get_tag(MILTER_AGENT(context)),
                     milter_server_context_get_name(context));
        return FALSE;
    }

    milter_debug("[%u] [children][start-child] [%u] <%s>@<%s>",
                 priv->tag,
                 milter_agent_get_tag(MILTER_AGENT(context)),
                 command,
                 user_name ? user_name : "[current-user]");
    /* TODO: create priv->launch_command_encoder. */
    encoder = MILTER_MANAGER_LAUNCH_COMMAND_ENCODER(milter_manager_launch_command_encoder_new());
    milter_manager_launch_command_encoder_encode_launch(encoder,
                                                        &packet, &packet_size,
                                                        command,
                                                        user_name);
    milter_writer_write(priv->launcher_writer, packet, packet_size, &error);
    g_object_unref(encoder);

    if (error) {
        milter_error("[%u] [children][error][start-child][write] [%u] %s: %s",
                     priv->tag,
                     milter_agent_get_tag(MILTER_AGENT(context)),
                     error->message,
                     milter_server_context_get_name(context));
        milter_error_emittable_emit(MILTER_ERROR_EMITTABLE(children),
                                    error);
        g_error_free(error);
        return FALSE;
    }

    milter_writer_flush(priv->launcher_writer, &error);
    if (error) {
        milter_error("[%u] [children][error][start-child][flush] [%u] %s: %s",
                     priv->tag,
                     milter_agent_get_tag(MILTER_AGENT(context)),
                     error->message,
                     milter_server_context_get_name(context));
        milter_error_emittable_emit(MILTER_ERROR_EMITTABLE(children),
                                    error);
        g_error_free(error);
        return FALSE;
    }

    return TRUE;
}

static void
check_fallback_status_on_negotiate (MilterManagerChildren *children)
{
    MilterManagerChildrenPrivate *priv;
    GList *node;
    MilterStatus status = MILTER_STATUS_CONTINUE;
    gchar *status_name = NULL;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);
    for (node = priv->milters; node; node = g_list_next(node)) {
        MilterManagerChild *child;
        MilterServerContext *context;
        MilterStatus fallback_status;

        child = MILTER_MANAGER_CHILD(node->data);
        context = MILTER_SERVER_CONTEXT(child);
        if (milter_server_context_is_negotiated(context))
            continue;

        fallback_status = milter_manager_child_get_fallback_status(child);
        if (milter_status_compare(status, fallback_status) < 0) {
            status = fallback_status;
        }
    }

    switch (status) {
    case MILTER_STATUS_REJECT:
    case MILTER_STATUS_TEMPORARY_FAILURE:
    case MILTER_STATUS_DISCARD:
        if (milter_need_log(MILTER_LOG_LEVEL_DEBUG)) {
            status_name = milter_utils_get_enum_nick_name(MILTER_TYPE_STATUS,
                                                          status);
            milter_debug("[%u] [children][negotiate][fallback][all-expire][%s]",
                         priv->tag, status_name);
        }
        for (node = priv->milters; node; node = g_list_next(node)) {
            MilterServerContext *context;

            context = MILTER_SERVER_CONTEXT(node->data);
            if (!milter_server_context_is_negotiated(context))
                continue;

            if (milter_need_log(MILTER_LOG_LEVEL_DEBUG)) {
                guint child_tag;
                const gchar *child_name;

                child_tag = milter_agent_get_tag(MILTER_AGENT(context));
                child_name = milter_server_context_get_name(context);
                milter_debug("[%u] [children][negotiate][fallback][expire][%s] "
                             "[%u] %s",
                             priv->tag, status_name,
                             child_tag, MILTER_LOG_NULL_SAFE_STRING(child_name));
            }
            priv->all_expired_as_fallback_on_negotiated = TRUE;
            expire_child(children, context);
        }
        break;
    default:
        break;
    }

    if (status_name)
        g_free(status_name);
}

static void
reply_negotiate (MilterManagerChildren *children)
{
    MilterManagerChildrenPrivate *priv;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    if (!priv->option) {
        milter_error("[%u] [children][error][negotiate][not-started]",
                     priv->tag);
        g_signal_emit_by_name(children, "abort");
        return;
    }

    if (!priv->negotiated) {
        milter_error("[%u] [children][error][negotiate][no-response]",
                     priv->tag);
    }

    milter_option_remove_step(priv->option,
                              MILTER_STEP_NO_EVENT_MASK |
                              (MILTER_STEP_YES_MASK &
                               ~(priv->initial_yes_steps &
                                 priv->requested_yes_steps)));
    g_signal_emit_by_name(children, "negotiate-reply",
                          priv->option, priv->macros_requests);

    check_fallback_status_on_negotiate(children);
}

static void
remove_queue_in_negotiate (MilterManagerChildren *children,
                           MilterManagerChild *child)
{
    MilterManagerChildrenPrivate *priv;
    MilterServerContext *context;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);
    context = MILTER_SERVER_CONTEXT(child);
    milter_debug("[%u] [children][negotiate][done] [%u] %s",
                 priv->tag,
                 milter_agent_get_tag(MILTER_AGENT(context)),
                 milter_server_context_get_name(context));
    g_queue_remove(priv->reply_queue, child);
    if (!priv->finished && g_queue_is_empty(priv->reply_queue)) {
        reply_negotiate(children);
    }
}

static void
clear_try_negotiate_data (NegotiateData *data)
{
    MilterManagerChildrenPrivate *priv;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(data->children);

    remove_queue_in_negotiate(data->children, data->child);
    expire_child(data->children, MILTER_SERVER_CONTEXT(data->child));
    g_hash_table_remove(priv->try_negotiate_ids, data);
}

static void
cb_connection_timeout (MilterServerContext *context, gpointer user_data)
{
    NegotiateData *data = user_data;
    MilterManagerChildrenPrivate *priv;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(data->children);
    milter_error("[%u] [children][timeout][connection] [%u] %s",
                 priv->tag,
                 milter_agent_get_tag(MILTER_AGENT(context)),
                 milter_server_context_get_name(context));
    clear_try_negotiate_data(data);
}

static void
cb_connection_error (MilterErrorEmittable *emittable, GError *error, gpointer user_data)
{
    NegotiateData *data = user_data;
    MilterServerContext *context;
    MilterManagerChildrenPrivate *priv;
    gboolean privilege;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(data->children);
    context = MILTER_SERVER_CONTEXT(data->child);
    milter_error("[%u] [children][error][connection] [%u] %s: %s",
                 priv->tag,
                 milter_agent_get_tag(MILTER_AGENT(context)),
                 error->message,
                 milter_server_context_get_name(context));

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
        milter_debug("[%u] [children][connection][not-retry] [%u] "
                     "not privilege mode: %s",
                     priv->tag,
                     milter_agent_get_tag(MILTER_AGENT(context)),
                     milter_server_context_get_name(context));
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

static NegotiateTimeoutID *
negotiate_timeout_id_new (MilterEventLoop *loop, guint timeout_id)
{
    NegotiateTimeoutID *id;

    id = g_new0(NegotiateTimeoutID, 1);
    id->loop = g_object_ref(loop);
    id->timeout_id = timeout_id;

    return id;
}

static void
negotiate_timeout_id_free (NegotiateTimeoutID *id)
{
    milter_event_loop_remove(id->loop, id->timeout_id);
    g_object_unref(id->loop);
    g_free(id);
}

static void
negotiate_timeout_id_hash_value_free (gpointer data)
{
    if (data)
        negotiate_timeout_id_free(data);
}

static void
prepare_retry_establish_connection (MilterManagerChild *child,
                                    MilterOption *option,
                                    MilterManagerChildren *children,
                                    gboolean is_retry)
{
    MilterManagerChildrenPrivate *priv;
    NegotiateData *negotiate_data;
    NegotiateTimeoutID *negotiate_timeout_id;
    guint timeout_id;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    negotiate_data = negotiate_data_new(children, child, option, is_retry);
    timeout_id = milter_event_loop_add_timeout(priv->event_loop,
                                               priv->retry_connect_time,
                                               retry_establish_connection,
                                               negotiate_data);
    negotiate_timeout_id =
        negotiate_timeout_id_new(priv->event_loop, timeout_id);

    g_hash_table_insert(priv->try_negotiate_ids,
                        negotiate_data, negotiate_timeout_id);
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
    g_hash_table_insert(priv->try_negotiate_ids, negotiate_data, NULL);
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
        milter_error("[%u] [children][error][connection] [%u] %s: %s",
                     priv->tag,
                     milter_agent_get_tag(MILTER_AGENT(context)),
                     error->message,
                     milter_server_context_get_name(context));
        milter_error_emittable_emit(MILTER_ERROR_EMITTABLE(children),
                                    error);

        g_error_free(error);
        if (is_retry) {
            remove_queue_in_negotiate(children, child);
            expire_child(children, context);
        } else {
            prepare_retry_establish_connection(child, option, children, TRUE);
        }
        return FALSE;
    }

    prepare_negotiate(child, option, children, is_retry);

    return TRUE;
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

static MilterStatus
send_next_command (MilterManagerChildren *children,
                   MilterServerContext *context,
                   MilterServerContextState processing_state)
{
    MilterCommand next_command;

    next_command = get_next_command(children, context, processing_state);
    if (next_command == -1)
        return MILTER_STATUS_NOT_CHANGE;

    if (next_command == MILTER_COMMAND_BODY &&
        processing_state != MILTER_SERVER_CONTEXT_STATE_BODY) {
        MilterStatus status;
        status = init_child_for_body(children, context);
        if (status != MILTER_STATUS_NOT_CHANGE)
            return status;
    }

    return send_command_to_child(children, context, next_command);
}

static void
init_command_waiting_child_queue (MilterManagerChildren *children, MilterCommand command)
{
    MilterManagerChildrenPrivate *priv;
    GList *node;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    if (!g_list_find(priv->command_queue, GINT_TO_POINTER(command)))
        priv->command_queue = g_list_append(priv->command_queue,
                                            GINT_TO_POINTER(command));

    if (priv->command_waiting_child_queue)
        return;

    for (node = priv->milters; node; node = g_list_next(node)) {
        MilterServerContext *context;

        context = MILTER_SERVER_CONTEXT(node->data);
        if (milter_server_context_is_processing_message(context)) {
            if (milter_server_context_has_accepted_recipient(context)) {
                priv->command_waiting_child_queue =
                    g_list_append(priv->command_waiting_child_queue, context);
            } else {
                milter_server_context_abort(context);
            }
        }
    }
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
queue_init (GQueue *queue)
{
    g_return_if_fail(queue != NULL);

    queue->head = queue->tail = NULL;
    queue->length = 0;
}

static void
queue_clear (GQueue *queue)
{
    g_return_if_fail(queue != NULL);

    g_list_free(queue->head);
    queue_init(queue);
}

static void
set_state (MilterManagerChildren *children,
           MilterServerContextState state)
{
    MilterManagerChildrenPrivate *priv;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    priv->state = state;
    priv->processing_state = priv->state;
}

static void
init_reply_queue (MilterManagerChildren *children,
                  MilterServerContextState state)
{
    MilterManagerChildrenPrivate *priv;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    set_state(children, state);
    queue_clear(priv->reply_queue);
    g_hash_table_insert(priv->reply_statuses,
                        GINT_TO_POINTER(state),
                        GINT_TO_POINTER(MILTER_STATUS_NOT_CHANGE));
}

static gboolean
cb_idle_reply_negotiate_on_no_child (gpointer user_data)
{
    MilterManagerChildren *children = user_data;

    reply_negotiate(children);
    return FALSE;
}

gboolean
milter_manager_children_negotiate (MilterManagerChildren *children,
                                   MilterOption          *option,
                                   MilterMacrosRequests  *macros_requests)
{
    GList *node, *copied_milters;
    MilterManagerChildrenPrivate *priv;
    gboolean success = TRUE;
    gboolean privilege;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    if (priv->option)
        g_object_unref(priv->option);
    priv->option = option;
    if (priv->option) {
        g_object_ref(priv->option);
        if (milter_option_get_version(priv->option) >
            MAX_SUPPORTED_MILTER_PROTOCOL_VERSION) {
            milter_option_set_version(priv->option,
                                      MAX_SUPPORTED_MILTER_PROTOCOL_VERSION);
        }
        priv->initial_yes_steps = milter_option_get_step_yes(priv->option);
    }

    if (!priv->milters) {
        priv->negotiated = TRUE;
        dispose_lazy_reply_negotiate_id(priv);
        priv->lazy_reply_negotiate_id =
            milter_event_loop_add_idle_full(priv->event_loop,
                                            G_PRIORITY_DEFAULT,
                                            cb_idle_reply_negotiate_on_no_child,
                                            children,
                                            NULL);
        return success;
    }

    privilege =
        milter_manager_configuration_is_privilege_mode(priv->configuration);

    init_reply_queue(children, MILTER_SERVER_CONTEXT_STATE_NEGOTIATE);
    for (node = priv->milters; node; node = g_list_next(node)) {
        MilterManagerChild *child = MILTER_MANAGER_CHILD(node->data);
        g_queue_push_tail(priv->reply_queue, child);
    }

    copied_milters = g_list_copy(priv->milters);
    for (node = copied_milters; node; node = g_list_next(node)) {
        MilterManagerChild *child = MILTER_MANAGER_CHILD(node->data);

        if (!child_establish_connection(child, option, children, FALSE)) {
            if (privilege &&
                milter_manager_children_start_child(children, child)) {
                prepare_retry_establish_connection(child, option, children,
                                                   FALSE);
            }
        }
    }
    g_list_free(copied_milters);

    return success;
}

gboolean
milter_manager_children_define_macro (MilterManagerChildren *children,
                                      MilterCommand command,
                                      GHashTable *macros)
{
    GList *node;
    MilterManagerChildrenPrivate *priv;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    for (node = priv->milters; node; node = g_list_next(node)) {
        MilterManagerChild *child = node->data;
        MilterServerContext *context;
        MilterProtocolAgent *agent;

        context = MILTER_SERVER_CONTEXT(child);
        agent = MILTER_PROTOCOL_AGENT(child);
        switch (command) {
        case MILTER_COMMAND_CONNECT:
        case MILTER_COMMAND_HELO:
            if (!milter_server_context_is_negotiated(context))
                continue;
            break;
        case MILTER_COMMAND_ENVELOPE_FROM:
            if (milter_server_context_is_quitted(context))
                continue;
            break;
        case MILTER_COMMAND_UNKNOWN:
            if (milter_server_context_is_quitted(context))
                continue;
            break;
        case MILTER_COMMAND_ENVELOPE_RECIPIENT:
        case MILTER_COMMAND_DATA:
        case MILTER_COMMAND_HEADER:
        case MILTER_COMMAND_END_OF_HEADER:
        case MILTER_COMMAND_BODY:
        case MILTER_COMMAND_END_OF_MESSAGE:
            if (!milter_server_context_is_processing_message(context))
                continue;
            break;
        default:
            break;
        }
        milter_protocol_agent_set_macros_hash_table(agent, command, macros);
    }
    return TRUE;
}

static gboolean
milter_manager_children_check_alive (MilterManagerChildren *children)
{
    MilterManagerChildrenPrivate *priv;
    GList *node;
    GError *error = NULL;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    if (priv->all_expired_as_fallback_on_negotiated) {
        milter_debug("[%u] [children][alive][check][expired-on-negotiate]",
                     priv->tag);
        return FALSE;
    }

    for (node = priv->milters; node; node = g_list_next(node)) {
        MilterServerContext *context;

        context = MILTER_SERVER_CONTEXT(node->data);
        if (!milter_server_context_is_quitted(context))
            return TRUE;
    }

    g_set_error(&error,
                MILTER_MANAGER_CHILDREN_ERROR,
                MILTER_MANAGER_CHILDREN_ERROR_NO_ALIVE_MILTER,
                "All milters are no longer alive.");
    milter_error("[%u] [children][error][alive] %s",
                     priv->tag, error->message);
    milter_error_emittable_emit(MILTER_ERROR_EMITTABLE(children),
                                error);
    g_error_free(error);

    return FALSE;
}

static gboolean
milter_manager_children_check_processing_message (MilterManagerChildren *children)
{
    MilterManagerChildrenPrivate *priv;
    GList *node;
    GError *error = NULL;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);
    for (node = priv->milters; node; node = g_list_next(node)) {
        MilterServerContext *context;

        context = MILTER_SERVER_CONTEXT(node->data);
        if (milter_server_context_is_processing_message(context))
            return TRUE;
    }

    g_set_error(&error,
                MILTER_MANAGER_CHILDREN_ERROR,
                MILTER_MANAGER_CHILDREN_ERROR_NO_MESSAGE_PROCESSING_MILTER,
                "None of milters are processing message.");
    milter_error("[%u] [children][error][message-processing] %s",
                 priv->tag, error->message);
    milter_error_emittable_emit(MILTER_ERROR_EMITTABLE(children),
                                error);
    g_error_free(error);

    return FALSE;
}

gboolean
milter_manager_children_connect (MilterManagerChildren *children,
                                 const gchar           *host_name,
                                 struct sockaddr       *address,
                                 socklen_t              address_length)
{
    GList *child, *targets;
    MilterManagerChildrenPrivate *priv;
    gboolean success = FALSE;
    gint n_queued_milters;
    MilterServerContextState state = MILTER_SERVER_CONTEXT_STATE_CONNECT;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    dispose_smtp_client_address(priv);
    priv->smtp_client_address = g_memdup(address, address_length);
    priv->smtp_client_address_length = address_length;

    if (!milter_manager_children_check_alive(children))
        return FALSE;

    init_reply_queue(children, state);
    for (child = priv->milters; child; child = g_list_next(child)) {
        MilterServerContext *context = MILTER_SERVER_CONTEXT(child->data);

        if (!milter_server_context_is_quitted(context))
            g_queue_push_tail(priv->reply_queue, context);
    }

    n_queued_milters = priv->reply_queue->length;
    targets = g_list_copy(priv->reply_queue->head);
    for (child = targets; child; child = g_list_next(child)) {
        MilterServerContext *context = MILTER_SERVER_CONTEXT(child->data);
        if (milter_server_context_connect(context,
                                          host_name,
                                          address,
                                          address_length)) {
            success = TRUE;
        }
    }
    milter_debug("[%u] [children][connect][sent] %d",
                 priv->tag, n_queued_milters);
    for (child = targets; child; child = g_list_next(child)) {
        MilterServerContext *context = MILTER_SERVER_CONTEXT(child->data);
        if (!milter_server_context_need_reply(context, state)) {
            cb_continue(context, children);
        }
    }
    g_list_free(targets);

    return success;
}

gboolean
milter_manager_children_helo (MilterManagerChildren *children,
                              const gchar           *fqdn)
{
    GList *child, *targets;
    MilterManagerChildrenPrivate *priv;
    gboolean success = FALSE;
    gint n_queued_milters;
    MilterServerContextState state = MILTER_SERVER_CONTEXT_STATE_HELO;

    if (!milter_manager_children_check_alive(children))
        return FALSE;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    init_reply_queue(children, state);
    for (child = priv->milters; child; child = g_list_next(child)) {
        MilterServerContext *context = MILTER_SERVER_CONTEXT(child->data);

        if (!milter_server_context_is_quitted(context))
            g_queue_push_tail(priv->reply_queue, context);
    }

    n_queued_milters = priv->reply_queue->length;
    targets = g_list_copy(priv->reply_queue->head);
    for (child = targets; child; child = g_list_next(child)) {
        MilterServerContext *context = MILTER_SERVER_CONTEXT(child->data);
        if (milter_server_context_helo(context, fqdn))
            success = TRUE;
    }
    milter_debug("[%u] [children][helo][sent] %d",
                 priv->tag, n_queued_milters);
    for (child = targets; child; child = g_list_next(child)) {
        MilterServerContext *context = MILTER_SERVER_CONTEXT(child->data);
        if (!milter_server_context_need_reply(context, state)) {
            cb_continue(context, children);
        }
    }
    g_list_free(targets);

    return success;
}

gboolean
milter_manager_children_envelope_from (MilterManagerChildren *children,
                                       const gchar           *from)
{
    GList *child, *targets;
    MilterManagerChildrenPrivate *priv;
    gboolean success = FALSE;
    gint n_queued_milters;
    MilterServerContextState state = MILTER_SERVER_CONTEXT_STATE_ENVELOPE_FROM;

    if (!milter_manager_children_check_alive(children))
        return FALSE;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    init_reply_queue(children, state);
    for (child = priv->milters; child; child = g_list_next(child)) {
        MilterServerContext *context = MILTER_SERVER_CONTEXT(child->data);

        if (!milter_server_context_is_quitted(context))
            g_queue_push_tail(priv->reply_queue, context);
    }

    n_queued_milters = priv->reply_queue->length;
    targets = g_list_copy(priv->reply_queue->head);
    for (child = targets; child; child = g_list_next(child)) {
        MilterServerContext *context = MILTER_SERVER_CONTEXT(child->data);
        if (milter_server_context_envelope_from(context, from))
            success = TRUE;
    }
    milter_debug("[%u] [children][envelope-from][sent] %d",
                 priv->tag, n_queued_milters);
    for (child = targets; child; child = g_list_next(child)) {
        MilterServerContext *context = MILTER_SERVER_CONTEXT(child->data);
        if (!milter_server_context_need_reply(context, state)) {
            cb_continue(context, children);
        }
    }
    g_list_free(targets);

    return success;
}

gboolean
milter_manager_children_envelope_recipient (MilterManagerChildren *children,
                                            const gchar           *recipient)
{
    GList *child, *targets;
    MilterManagerChildrenPrivate *priv;
    gboolean success = FALSE;
    gint n_queued_milters;
    MilterServerContextState state =
        MILTER_SERVER_CONTEXT_STATE_ENVELOPE_RECIPIENT;

    if (!milter_manager_children_check_processing_message(children))
        return FALSE;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    init_reply_queue(children, state);
    for (child = priv->milters; child; child = g_list_next(child)) {
        MilterServerContext *context = MILTER_SERVER_CONTEXT(child->data);

        if (milter_server_context_is_processing_message(context))
            g_queue_push_tail(priv->reply_queue, context);
    }

    n_queued_milters = priv->reply_queue->length;
    targets = g_list_copy(priv->reply_queue->head);
    for (child = targets; child; child = g_list_next(child)) {
        MilterServerContext *context = MILTER_SERVER_CONTEXT(child->data);
        if (milter_server_context_envelope_recipient(context, recipient))
            success = TRUE;
    }
    milter_debug("[%u] [children][envelope-recipient][sent] %d",
                 priv->tag, n_queued_milters);
    for (child = targets; child; child = g_list_next(child)) {
        MilterServerContext *context = MILTER_SERVER_CONTEXT(child->data);
        if (!milter_server_context_need_reply(context, state)) {
            cb_continue(context, children);
        }
    }
    g_list_free(targets);

    return success;
}

gboolean
milter_manager_children_data (MilterManagerChildren *children)
{
    GList *child, *targets;
    MilterManagerChildrenPrivate *priv;
    gboolean success = FALSE;
    gint n_queued_milters;
    MilterServerContextState state = MILTER_SERVER_CONTEXT_STATE_DATA;

    if (!milter_manager_children_check_processing_message(children))
        return FALSE;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    init_reply_queue(children, state);
    for (child = priv->milters; child; child = g_list_next(child)) {
        MilterServerContext *context = MILTER_SERVER_CONTEXT(child->data);

        if (milter_server_context_is_processing_message(context)) {
            if (milter_server_context_has_accepted_recipient(context)) {
                g_queue_push_tail(priv->reply_queue, context);
            } else {
                milter_server_context_abort(context);
            }
        }
    }

    n_queued_milters = priv->reply_queue->length;
    targets = g_list_copy(priv->reply_queue->head);
    for (child = targets; child; child = g_list_next(child)) {
        MilterServerContext *context = MILTER_SERVER_CONTEXT(child->data);
        if (milter_server_context_data(context))
            success = TRUE;
    }
    milter_debug("[%u] [children][data][sent] %d", priv->tag, n_queued_milters);
    for (child = targets; child; child = g_list_next(child)) {
        MilterServerContext *context = MILTER_SERVER_CONTEXT(child->data);
        if (!milter_server_context_need_reply(context, state)) {
            cb_continue(context, children);
        }
    }
    g_list_free(targets);

    return success;
}

static MilterStatus
send_command_to_first_waiting_child (MilterManagerChildren *children,
                                     MilterCommand command)
{
    MilterServerContext *first_child;

    first_child = get_first_child_in_command_waiting_child_queue(children);
    if (!first_child)
        return MILTER_STATUS_NOT_CHANGE;

    return send_command_to_child(children, first_child, command);
}

gboolean
milter_manager_children_unknown (MilterManagerChildren *children,
                                 const gchar           *command)
{
    GList *child, *targets;
    MilterManagerChildrenPrivate *priv;
    gboolean success = FALSE;
    gint n_queued_milters;
    MilterServerContextState state = MILTER_SERVER_CONTEXT_STATE_UNKNOWN;

    if (!milter_manager_children_check_alive(children))
        return FALSE;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    init_reply_queue(children, state);
    for (child = priv->milters; child; child = g_list_next(child)) {
        MilterServerContext *context = MILTER_SERVER_CONTEXT(child->data);

        if (!milter_server_context_is_quitted(context))
            g_queue_push_tail(priv->reply_queue, context);
    }

    n_queued_milters = priv->reply_queue->length;
    targets = g_list_copy(priv->reply_queue->head);
    for (child = targets; child; child = g_list_next(child)) {
        MilterServerContext *context = MILTER_SERVER_CONTEXT(child->data);
        if (milter_server_context_unknown(context, command))
            success = TRUE;
    }
    milter_debug("[%u] [children][unknown][sent] %d",
                 priv->tag, n_queued_milters);
    for (child = targets; child; child = g_list_next(child)) {
        MilterServerContext *context = MILTER_SERVER_CONTEXT(child->data);
        if (!milter_server_context_need_reply(context, state)) {
            cb_continue(context, children);
        }
    }
    g_list_free(targets);

    return success;
}

static MilterStatus
send_next_header_to_child (MilterManagerChildren *children, MilterServerContext *context)
{
    MilterManagerChildrenPrivate *priv;
    MilterHeader *header;
    gint value_offset = 0;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);
    priv->processing_state = MILTER_SERVER_CONTEXT_STATE_HEADER;

    priv->processing_header_index++;
    header = milter_headers_get_nth_header(priv->headers,
                                           priv->processing_header_index);
    if (!header)
        return MILTER_STATUS_NOT_CHANGE;

    if (need_header_value_leading_space_conversion(children, context)) {
        if (header->value && header->value[0] == ' ')
            value_offset = 1;
    }

    if (milter_server_context_header(context,
                                     header->name,
                                     header->value + value_offset)) {
        MilterStatus status = MILTER_STATUS_PROGRESS;
        if (!milter_server_context_need_reply(context, priv->processing_state)) {
            g_signal_emit_by_name(context, "continue");
        }
        return status;
    } else {
        MilterManagerChild *child;

        child = MILTER_MANAGER_CHILD(context);
        return milter_manager_child_get_fallback_status(child);
    }
}

static gboolean
need_data_commmand_emulation (MilterManagerChildrenPrivate *priv)
{
    return (priv->state == MILTER_SERVER_CONTEXT_STATE_ENVELOPE_RECIPIENT &&
            milter_option_get_version(priv->option) >= 4);
}

gboolean
milter_manager_children_header (MilterManagerChildren *children,
                                const gchar           *name,
                                const gchar           *value)
{
    MilterManagerChildrenPrivate *priv;

    if (!milter_manager_children_check_processing_message(children))
        return FALSE;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    if (need_data_commmand_emulation(priv)) {
        gboolean success;
        milter_debug("[%u] [children][data-command-emulation][header] "
                     "<%s>=<%s>", priv->tag, name, value);
        success = milter_manager_children_data(children);
        if (success) {
            milter_debug("[%u] [children][pending-message-request][header]"
                         "[keep] <%s>=<%s>",
                         priv->tag, name, value);
            dispose_pending_message_request(priv);
            priv->pending_message_request =
                pending_header_request_new(name, value);
        }
        return success;
    }

    priv->state = MILTER_SERVER_CONTEXT_STATE_HEADER;
    priv->processing_state = priv->state;
    if (!priv->headers)
        priv->headers = milter_headers_new();
    milter_headers_append_header(priv->headers, name, value);
    init_command_waiting_child_queue(children, MILTER_COMMAND_HEADER);

    return MILTER_STATUS_PROGRESS ==
        send_command_to_first_waiting_child(children, MILTER_COMMAND_HEADER);
}

gboolean
milter_manager_children_end_of_header (MilterManagerChildren *children)
{
    MilterManagerChildrenPrivate *priv;

    if (!milter_manager_children_check_processing_message(children))
        return FALSE;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    if (need_data_commmand_emulation(priv)) {
        gboolean success;
        milter_debug("[%u] [children][data-command-emulation][end-of-header]",
                     priv->tag);
        success = milter_manager_children_data(children);
        if (success) {
            milter_debug("[%u] [children][pending-message-request]"
                         "[end-of-header][keep]",
                         priv->tag);
            dispose_pending_message_request(priv);
            priv->pending_message_request =
                pending_end_of_header_request_new();
        }
        return success;
    }

    priv->state = MILTER_SERVER_CONTEXT_STATE_END_OF_HEADER;
    priv->processing_state = priv->state;
    init_command_waiting_child_queue(children, MILTER_COMMAND_END_OF_HEADER);

    return MILTER_STATUS_PROGRESS ==
        send_command_to_first_waiting_child(children,
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
    if (error) {
        milter_error("[%u] [children][error][body][open] %s",
                     priv->tag, error->message);
        milter_error_emittable_emit(MILTER_ERROR_EMITTABLE(children),
                                    error);
        g_error_free(error);
        return FALSE;
    }
    priv->body_file = g_io_channel_unix_new(fd);
    g_io_channel_set_close_on_unref(priv->body_file, TRUE);

    g_io_channel_set_encoding(priv->body_file, NULL, &error);
    if (error) {
        milter_error("[%u] [children][error][body][encoding] %s",
                     priv->tag, error->message);
        milter_error_emittable_emit(MILTER_ERROR_EMITTABLE(children),
                                    error);
        g_error_free(error);
        return FALSE;
    }

    return TRUE;
}

static gboolean
write_body_to_file (MilterManagerChildren *children,
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
        milter_error("[%u] [children][error][body][write] %s",
                     priv->tag,
                     error->message);
        milter_error_emittable_emit(MILTER_ERROR_EMITTABLE(children),
                                    error);
        g_error_free(error);
        return FALSE;
    }

    return TRUE;
}

static gboolean
write_body_to_string (MilterManagerChildren *children,
                      const gchar *chunk,
                      gsize size)
{
    MilterManagerChildrenPrivate *priv;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    if (!priv->body)
        priv->body = g_string_new_len(chunk, size);
    else
        g_string_append_len(priv->body, chunk, size);

    if (priv->body->len > MAX_ON_MEMORY_BODY_SIZE) {
        gboolean success;

        success = write_body_to_file(children, priv->body->str, priv->body->len);
        g_string_free(priv->body, TRUE);
        priv->body = NULL;

        return success;
    }

    return TRUE;
}

static gboolean
write_body (MilterManagerChildren *children,
            const gchar *chunk, gsize size)
{
    MilterManagerChildrenPrivate *priv;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    if (priv->body_file)
        return write_body_to_file(children, chunk, size);
    else
        return write_body_to_string(children, chunk, size);
}

gboolean
milter_manager_children_body (MilterManagerChildren *children,
                              const gchar           *chunk,
                              gsize                  size)
{
    MilterManagerChildrenPrivate *priv;
    MilterServerContext *first_child;
    MilterServerContextState state = MILTER_SERVER_CONTEXT_STATE_BODY;

    if (!milter_manager_children_check_processing_message(children))
        return FALSE;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    if (need_data_commmand_emulation(priv)) {
        gboolean success;
        milter_debug("[%u] [children][data-command-emulation][body] "
                     "size=%" G_GSIZE_FORMAT,
                     priv->tag, size);
        success = milter_manager_children_data(children);
        if (success) {
            milter_debug("[%u] [children][pending-message-request][body][keep] "
                         "size=%" G_GSIZE_FORMAT,
                         priv->tag, size);
            dispose_pending_message_request(priv);
            priv->pending_message_request =
                pending_body_request_new(chunk, size);
        }
        return success;
    }

    milter_debug("[%u] [children][body] size=%" G_GSIZE_FORMAT,
                 priv->tag, size);

    init_command_waiting_child_queue(children, MILTER_COMMAND_BODY);

    first_child = get_first_child_in_command_waiting_child_queue(children);
    if (!first_child)
        return FALSE;

    if (!write_body(children, chunk, size))
        return FALSE;

    priv->state = state;
    priv->processing_state = state;
    priv->replaced_body_for_each_child = FALSE;
    priv->sending_body = FALSE;

    if (milter_server_context_get_skip_body(first_child)) {
        /*
         * If the first child is not needed the command,
         * send dummy "continue" to shift next state.
         */
        milter_server_context_set_state(first_child, MILTER_SERVER_CONTEXT_STATE_BODY);
        g_signal_emit_by_name(first_child, "continue");
        return TRUE;
    } else {
        return milter_server_context_body(first_child, chunk, size);
    }

}

static MilterStatus
init_child_for_body_string (MilterManagerChildren *children,
                            MilterServerContext *context)
{
    MilterManagerChildrenPrivate *priv;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);
    priv->sent_body_offset = 0;

    return MILTER_STATUS_NOT_CHANGE;
}

static MilterStatus
init_child_for_body_file (MilterManagerChildren *children,
                          MilterServerContext *context)
{
    MilterManagerChildrenPrivate *priv;
    GError *error = NULL;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);
    if (!priv->body_file)
        return MILTER_STATUS_NOT_CHANGE;

    g_io_channel_seek_position(priv->body_file, 0, G_SEEK_SET, &error);
    if (error) {
        MilterManagerChild *child;

        milter_error("[%u] [children][error][body][send][seek] [%u] %s: %s",
                     priv->tag,
                     milter_agent_get_tag(MILTER_AGENT(context)),
                     error->message,
                     milter_server_context_get_name(context));
        milter_error_emittable_emit(MILTER_ERROR_EMITTABLE(children), error);
        g_error_free(error);

        child = MILTER_MANAGER_CHILD(context);
        return milter_manager_child_get_fallback_status(child);
    }

    return MILTER_STATUS_NOT_CHANGE;
}

static MilterStatus
init_child_for_body (MilterManagerChildren *children,
                     MilterServerContext *context)
{
    MilterManagerChildrenPrivate *priv;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    milter_debug("[%u] [children][body][init] [%u] %s",
                 priv->tag,
                 milter_agent_get_tag(MILTER_AGENT(context)),
                 milter_server_context_get_name(context));

    priv->processing_state = MILTER_SERVER_CONTEXT_STATE_BODY;

    priv->replaced_body_for_each_child = FALSE;
    priv->sending_body = TRUE;
    if (priv->body)
        return init_child_for_body_string(children, context);
    else
        return init_child_for_body_file(children, context);
}

static MilterStatus
send_body_to_child_file (MilterManagerChildren *children,
                         MilterServerContext *context)
{
    MilterStatus status = MILTER_STATUS_PROGRESS;
    GError *error = NULL;
    GIOStatus io_status = G_IO_STATUS_NORMAL;
    MilterManagerChildrenPrivate *priv;
    gchar buffer[MILTER_CHUNK_SIZE + 1];
    gsize read_size, chunk_size;
    MilterManagerChild *child;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);
    if (!priv->body_file)
        return MILTER_STATUS_NOT_CHANGE;

    chunk_size =
        milter_manager_configuration_get_chunk_size(priv->configuration);
    io_status = g_io_channel_read_chars(priv->body_file,
                                        buffer, chunk_size,
                                        &read_size, &error);

    child = MILTER_MANAGER_CHILD(context);
    switch (io_status) {
    case G_IO_STATUS_ERROR:
        status = milter_manager_child_get_fallback_status(child);
        break;
    case G_IO_STATUS_NORMAL:
        if (milter_server_context_body(context, buffer, read_size)) {
            status = MILTER_STATUS_PROGRESS;
        } else {
            status = milter_manager_child_get_fallback_status(child);
        }
        break;
    case G_IO_STATUS_EOF:
        status = MILTER_STATUS_NOT_CHANGE;
        break;
    case G_IO_STATUS_AGAIN:
        status = MILTER_STATUS_PROGRESS;
        break;
    default:
        status = milter_manager_child_get_fallback_status(child);
        break;
    }

    if (error) {
        milter_error("[%u] [children][error][body][send] [%u] %s: %s",
                     priv->tag,
                     milter_agent_get_tag(MILTER_AGENT(context)),
                     error->message,
                     milter_server_context_get_name(context));
        milter_error_emittable_emit(MILTER_ERROR_EMITTABLE(children), error);
        g_error_free(error);

        status = milter_manager_child_get_fallback_status(child);
    }

    return status;
}

static MilterStatus
send_body_to_child_string (MilterManagerChildren *children,
                           MilterServerContext *context)
{
    MilterStatus status = MILTER_STATUS_PROGRESS;
    MilterManagerChildrenPrivate *priv;
    gsize chunk_size, write_size;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);
    if (priv->sent_body_offset >= priv->body->len)
        return MILTER_STATUS_NOT_CHANGE;

    chunk_size =
        milter_manager_configuration_get_chunk_size(priv->configuration);
    write_size = MIN(priv->body->len - priv->sent_body_offset, chunk_size);
    if (milter_server_context_body(context,
                                   priv->body->str + priv->sent_body_offset,
                                   write_size)) {
        priv->sent_body_offset += write_size;
        init_command_waiting_child_queue(children, MILTER_COMMAND_BODY);
    } else {
        MilterManagerChild *child;

        child = MILTER_MANAGER_CHILD(context);
        status = milter_manager_child_get_fallback_status(child);
    }

    return status;
}

static MilterStatus
send_body_to_child (MilterManagerChildren *children, MilterServerContext *context)
{
    MilterManagerChildrenPrivate *priv;
    MilterStatus status;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);
    priv->processing_state = MILTER_SERVER_CONTEXT_STATE_BODY;

    if (milter_server_context_get_skip_body(context)) {
        priv->sending_body = FALSE;
        return MILTER_STATUS_NOT_CHANGE;
    }

    if (priv->body)
        status = send_body_to_child_string(children, context);
    else
        status = send_body_to_child_file(children, context);

    if (status == MILTER_STATUS_PROGRESS &&
        !milter_server_context_need_reply(context, priv->processing_state)) {
        g_signal_emit_by_name(context, "continue");
    }

    if (status != MILTER_STATUS_PROGRESS)
        priv->sending_body = FALSE;

    return status;
}

gboolean
milter_manager_children_end_of_message (MilterManagerChildren *children,
                                        const gchar           *chunk,
                                        gsize                  size)
{
    MilterManagerChildrenPrivate *priv;

    if (!milter_manager_children_check_processing_message(children))
        return FALSE;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    if (need_data_commmand_emulation(priv)) {
        gboolean success;
        milter_debug("[%u] [children][data-command-emulation][end-of-message] "
                     "size=%" G_GSIZE_FORMAT,
                     priv->tag, size);
        success = milter_manager_children_data(children);
        if (success) {
            milter_debug("[%u] [children][pending-message-request]"
                         "[end-of-message][keep] size=%" G_GSIZE_FORMAT,
                         priv->tag, size);
            dispose_pending_message_request(priv);
            priv->pending_message_request =
                pending_end_of_message_request_new(chunk, size);
        }
        return success;
    }

    init_command_waiting_child_queue(children, MILTER_COMMAND_END_OF_MESSAGE);

    priv->processing_header_index = 0;
    if (!priv->headers)
        priv->headers = milter_headers_new();
    if (!priv->original_headers)
        priv->original_headers = milter_headers_copy(priv->headers);

    if (priv->end_of_message_chunk)
        g_free(priv->end_of_message_chunk);
    priv->end_of_message_chunk = g_strdup(chunk);
    priv->end_of_message_size = size;
    if (priv->body_file)
        g_io_channel_seek_position(priv->body_file, 0, G_SEEK_SET, NULL);

    priv->state = MILTER_SERVER_CONTEXT_STATE_END_OF_MESSAGE;
    priv->processing_state = priv->state;
    return MILTER_STATUS_PROGRESS ==
        send_command_to_first_waiting_child(children,
                                            MILTER_COMMAND_END_OF_MESSAGE);
}

gboolean
milter_manager_children_quit (MilterManagerChildren *children)
{
    GList *child;
    MilterManagerChildrenPrivate *priv;
    gboolean success = TRUE;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    set_state(children, MILTER_SERVER_CONTEXT_STATE_QUIT);
    for (child = priv->milters; child; child = g_list_next(child)) {
        MilterServerContext *context;
        MilterServerContextState state;

        context = MILTER_SERVER_CONTEXT(child->data);

        if (milter_server_context_is_quitted(context))
            continue;

        state = milter_server_context_get_state(context);
        if (state == MILTER_SERVER_CONTEXT_STATE_QUIT)
            continue;

        if (!milter_server_context_quit(context))
            success = FALSE;
    }

    if (!priv->finished)
        milter_finished_emittable_emit(MILTER_FINISHED_EMITTABLE(children));

    return success;
}

gboolean
milter_manager_children_abort (MilterManagerChildren *children)
{
    GList *child;
    MilterManagerChildrenPrivate *priv;
    gboolean success = TRUE;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    set_state(children, MILTER_SERVER_CONTEXT_STATE_ABORT);
    for (child = priv->milters; child; child = g_list_next(child)) {
        MilterServerContext *context = MILTER_SERVER_CONTEXT(child->data);
        MilterServerContextState state;

        state = milter_server_context_get_state(context);
        if (milter_server_context_is_processing_message(context) ||
            state == MILTER_SERVER_CONTEXT_STATE_ENVELOPE_FROM) {
            if (!milter_server_context_abort(context)) {
                success = FALSE;
            }
        }
    }

    /* FIXME: should emit some signal for logging? */
    dispose_message_related_data(priv);

    for (child = priv->milters; child; child = g_list_next(child)) {
        MilterServerContext *context = MILTER_SERVER_CONTEXT(child->data);

        if (!milter_server_context_is_quitted(context))
            milter_server_context_reset_message_related_data(context);
    }

    return success;
}


gboolean
milter_manager_children_is_important_status (MilterManagerChildren *children,
                                             MilterServerContextState state,
                                             MilterStatus status)
{
    MilterStatus current_status;

    current_status = get_reply_status_for_state(children, state);
    return milter_status_compare(current_status, status) < 0;
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
milter_manager_children_get_processing_state (MilterManagerChildren *children)
{
    return MILTER_MANAGER_CHILDREN_GET_PRIVATE(children)->processing_state;
}

static void
cb_launcher_reader_flow (MilterReader *reader,
                         const gchar *data, gsize data_size,
                         gpointer user_data)
{

}

void
milter_manager_children_set_launcher_channel (MilterManagerChildren *children,
                                              GIOChannel *read_channel,
                                              GIOChannel *write_channel)
{
    MilterManagerChildrenPrivate *priv;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);

    if (priv->launcher_writer) {
        g_object_unref(priv->launcher_writer);
        priv->launcher_writer = NULL;
    }
    if (write_channel) {
        priv->launcher_writer = milter_writer_io_channel_new(write_channel);
        milter_writer_set_tag(priv->launcher_writer, priv->tag);
        milter_writer_start(priv->launcher_writer, priv->event_loop);
    }

    if (priv->launcher_reader) {
        g_signal_handlers_disconnect_by_func(priv->launcher_reader,
                                             G_CALLBACK(cb_launcher_reader_flow),
                                             children);
        g_object_unref(priv->launcher_reader);
        priv->launcher_reader = NULL;
    }

    if (read_channel) {
        priv->launcher_reader = milter_reader_io_channel_new(read_channel);
        g_signal_connect(priv->launcher_reader, "flow",
                         G_CALLBACK(cb_launcher_reader_flow), children);
        milter_reader_set_tag(priv->launcher_reader, priv->tag);
        milter_reader_start(priv->launcher_reader, priv->event_loop);
    }
}

guint
milter_manager_children_get_tag (MilterManagerChildren *children)
{
    return MILTER_MANAGER_CHILDREN_GET_PRIVATE(children)->tag;
}

void
milter_manager_children_set_tag (MilterManagerChildren *children, guint tag)
{
    MILTER_MANAGER_CHILDREN_GET_PRIVATE(children)->tag = tag;
}

gboolean
milter_manager_children_get_smtp_client_address (MilterManagerChildren *children,
                                                 struct sockaddr       **address,
                                                 socklen_t             *address_length)
{
    MilterManagerChildrenPrivate *priv;

    priv = MILTER_MANAGER_CHILDREN_GET_PRIVATE(children);
    if (!priv->smtp_client_address)
        return FALSE;

    *address_length = priv->smtp_client_address_length;
    *address = g_memdup(priv->smtp_client_address, *address_length);

    return TRUE;
}

MilterOption *
milter_manager_children_get_option (MilterManagerChildren *children)
{
    return MILTER_MANAGER_CHILDREN_GET_PRIVATE(children)->option;
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
