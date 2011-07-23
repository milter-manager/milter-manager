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

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <unistd.h>
#include <errno.h>

#include <milter/core.h>
#include "milter/core/milter-marshalers.h"
#include "milter-server-context.h"

#define NULL_SAFE_NAME(name) ((name) ? (name) : "(unknown)")

enum
{
    STOP_ON_CONNECT,
    STOP_ON_HELO,
    STOP_ON_ENVELOPE_FROM,
    STOP_ON_ENVELOPE_RECIPIENT,
    STOP_ON_DATA,
    STOP_ON_HEADER,
    STOP_ON_END_OF_HEADER,
    STOP_ON_BODY,
    STOP_ON_END_OF_MESSAGE,

    READY,
    STOPPED,

    CONNECTION_TIMEOUT,
    WRITING_TIMEOUT,
    READING_TIMEOUT,
    END_OF_MESSAGE_TIMEOUT,

    MESSAGE_PROCESSED,

    STATE_TRANSITED,

    LAST_SIGNAL
};

static gint signals[LAST_SIGNAL] = {0};

static gboolean    stop_on_accumulator(GSignalInvocationHint *hint,
                                       GValue *return_accumulator,
                                       const GValue *context_return,
                                       gpointer data);

#define MILTER_SERVER_CONTEXT_GET_PRIVATE(obj)                       \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj),                              \
                                 MILTER_TYPE_SERVER_CONTEXT,         \
                                 MilterServerContextPrivate))

typedef struct _MilterServerContextPrivate	MilterServerContextPrivate;
struct _MilterServerContextPrivate
{
    GIOChannel *client_channel;

    gchar *spec;
    gint domain;
    struct sockaddr *address;
    socklen_t address_size;
    MilterStatus status;
    MilterStatus envelope_recipient_status;
    MilterServerContextState state;
    GList *next_states;
    MilterServerContextState last_state;
    gchar *reply_code;
    MilterOption *option;

    gdouble connection_timeout;
    gdouble writing_timeout;
    gdouble reading_timeout;
    gdouble end_of_message_timeout;
    guint timeout_id;
    guint connect_watch_id;

    gboolean skip_body;
    GString *body;

    gchar *name;
    GList *body_response_queue;
    guint process_body_count;
    gboolean sent_end_of_message;

    GTimer *elapsed;

    gboolean negotiated;
    gboolean processing_message;
    gboolean quitted;

    gchar *current_recipient;

    MilterMessageResult *message_result;
};

enum
{
    PROP_0,
    PROP_NAME,
    PROP_STATUS,
    PROP_STATE,
    PROP_CONNECTION_TIMEOUT,
    PROP_WRITING_TIMEOUT,
    PROP_READING_TIMEOUT,
    PROP_END_OF_MESSAGE_TIMEOUT,
    PROP_MESSAGE_RESULT
};

MILTER_IMPLEMENT_REPLY_SIGNALS(reply_init)
G_DEFINE_TYPE_WITH_CODE(MilterServerContext,
                        milter_server_context,
                        MILTER_TYPE_PROTOCOL_AGENT,
                        G_IMPLEMENT_INTERFACE(MILTER_TYPE_REPLY_SIGNALS,
                                              reply_init))

static void dispose        (GObject         *object);
static void set_property   (GObject         *object,
                            guint            prop_id,
                            const GValue    *value,
                            GParamSpec      *pspec);
static void get_property   (GObject         *object,
                            guint            prop_id,
                            GValue          *value,
                            GParamSpec      *pspec);

static gboolean stop_on_connect      (MilterServerContext *context,
                                      const gchar   *host_name,
                                      const struct sockaddr *address,
                                      socklen_t      address_length);
static gboolean stop_on_helo         (MilterServerContext *context,
                                      const gchar   *fqdn);
static gboolean stop_on_envelope_from(MilterServerContext *context,
                                      const gchar   *from);
static gboolean stop_on_envelope_recipient
                                     (MilterServerContext *context,
                                      const gchar   *recipient);
static gboolean stop_on_header       (MilterServerContext *context,
                                      const gchar   *name,
                                      const gchar   *value);
static gboolean stop_on_body         (MilterServerContext *context,
                                      const gchar   *chunk,
                                      gsize          size);
static gboolean write_packet         (MilterServerContext      *context,
                                      const gchar              *packet,
                                      gsize                     packet_size,
                                      MilterServerContextState  next_state);

static MilterDecoder *decoder_new    (MilterAgent *agent);
static MilterEncoder *encoder_new    (MilterAgent *agent);
static void           flushed        (MilterAgent *agent);



static void
milter_server_context_class_init (MilterServerContextClass *klass)
{
    GObjectClass *gobject_class;
    GParamSpec *spec;
    MilterAgentClass *agent_class;

    gobject_class = G_OBJECT_CLASS(klass);
    agent_class = MILTER_AGENT_CLASS(klass);

    gobject_class->dispose      = dispose;
    gobject_class->set_property = set_property;
    gobject_class->get_property = get_property;

    agent_class->decoder_new   = decoder_new;
    agent_class->encoder_new   = encoder_new;
    agent_class->flushed       = flushed;

    klass->stop_on_connect          = stop_on_connect;
    klass->stop_on_helo             = stop_on_helo;
    klass->stop_on_envelope_from    = stop_on_envelope_from;
    klass->stop_on_envelope_recipient = stop_on_envelope_recipient;
    klass->stop_on_header           = stop_on_header;
    klass->stop_on_body             = stop_on_body;

    signals[READY] =
        g_signal_new("ready",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterServerContextClass,
                                     ready),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__VOID,
                     G_TYPE_NONE, 0);

    signals[STOPPED] =
        g_signal_new("stopped",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterServerContextClass, stopped),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__VOID,
                     G_TYPE_NONE, 0);

    signals[STOP_ON_CONNECT] =
        g_signal_new("stop-on-connect",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterServerContextClass, stop_on_connect),
                     stop_on_accumulator, NULL,
                     _milter_marshal_BOOLEAN__STRING_POINTER_UINT,
                     G_TYPE_BOOLEAN, 3,
                     G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_UINT);

    signals[STOP_ON_HELO] =
        g_signal_new("stop-on-helo",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterServerContextClass, stop_on_helo),
                     stop_on_accumulator, NULL,
                     _milter_marshal_BOOLEAN__STRING,
                     G_TYPE_BOOLEAN, 1, G_TYPE_STRING);

    signals[STOP_ON_ENVELOPE_FROM] =
        g_signal_new("stop-on-envelope-from",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterServerContextClass,
                                     stop_on_envelope_from),
                     stop_on_accumulator, NULL,
                     _milter_marshal_BOOLEAN__STRING,
                     G_TYPE_BOOLEAN, 1, G_TYPE_STRING);

    signals[STOP_ON_ENVELOPE_RECIPIENT] =
        g_signal_new("stop-on-envelope-recipient",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterServerContextClass,
                                     stop_on_envelope_recipient),
                     stop_on_accumulator, NULL,
                     _milter_marshal_BOOLEAN__STRING,
                     G_TYPE_BOOLEAN, 1, G_TYPE_STRING);

    signals[STOP_ON_DATA] =
        g_signal_new("stop-on-data",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterServerContextClass, stop_on_data),
                     stop_on_accumulator, NULL,
                     _milter_marshal_BOOLEAN__VOID,
                     G_TYPE_BOOLEAN, 0);

    signals[STOP_ON_HEADER] =
        g_signal_new("stop-on-header",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterServerContextClass, stop_on_header),
                     stop_on_accumulator, NULL,
                     _milter_marshal_BOOLEAN__STRING_STRING,
                     G_TYPE_BOOLEAN, 2, G_TYPE_STRING, G_TYPE_STRING);

    signals[STOP_ON_END_OF_HEADER] =
        g_signal_new("stop-on-end-of-header",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterServerContextClass,
                                     stop_on_end_of_header),
                     stop_on_accumulator, NULL,
                     _milter_marshal_BOOLEAN__VOID,
                     G_TYPE_BOOLEAN, 0);

    signals[STOP_ON_BODY] =
        g_signal_new("stop-on-body",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterServerContextClass, stop_on_body),
                     stop_on_accumulator, NULL,
#if GLIB_SIZEOF_SIZE_T == 8
                     _milter_marshal_BOOLEAN__STRING_UINT64,
                     G_TYPE_BOOLEAN, 2, G_TYPE_STRING, G_TYPE_UINT64
#else
                     _milter_marshal_BOOLEAN__STRING_UINT,
                     G_TYPE_BOOLEAN, 2, G_TYPE_STRING, G_TYPE_UINT
#endif
            );

    signals[STOP_ON_END_OF_MESSAGE] =
        g_signal_new("stop-on-end-of-message",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterServerContextClass,
                                     stop_on_end_of_message),
                     stop_on_accumulator, NULL,
#if GLIB_SIZEOF_SIZE_T == 8
                     _milter_marshal_BOOLEAN__STRING_UINT64,
                     G_TYPE_BOOLEAN, 2, G_TYPE_STRING, G_TYPE_UINT64
#else
                     _milter_marshal_BOOLEAN__STRING_UINT,
                     G_TYPE_BOOLEAN, 2, G_TYPE_STRING, G_TYPE_UINT
#endif
            );

    spec = g_param_spec_string("name",
                               "Name",
                               "The name of the MilterServerContext",
                               NULL,
                               G_PARAM_READWRITE);
    g_object_class_install_property(gobject_class, PROP_NAME, spec);

    spec = g_param_spec_enum("status",
                             "Status",
                             "The status of the MilterServerContext",
                             MILTER_TYPE_STATUS,
                             MILTER_STATUS_NOT_CHANGE,
                             G_PARAM_READWRITE);
    g_object_class_install_property(gobject_class, PROP_STATUS, spec);

    spec = g_param_spec_enum("state",
                             "State",
                             "The state of the MilterServerContext",
                             MILTER_TYPE_SERVER_CONTEXT_STATE,
                             MILTER_SERVER_CONTEXT_STATE_START,
                             G_PARAM_READWRITE);
    g_object_class_install_property(gobject_class, PROP_STATE, spec);

    spec = g_param_spec_double("connection-timeout",
                               "Connection timeout",
                               "The timeout seconds for connecting to a filter",
                               0,
                               G_MAXDOUBLE,
                               MILTER_SERVER_CONTEXT_DEFAULT_CONNECTION_TIMEOUT,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
    g_object_class_install_property(gobject_class, PROP_CONNECTION_TIMEOUT, spec);

    spec = g_param_spec_double("writing-timeout",
                               "Writing timeout",
                               "The timeout seconds for writing information from MTA to a filter",
                               0,
                               G_MAXDOUBLE,
                               MILTER_SERVER_CONTEXT_DEFAULT_WRITING_TIMEOUT,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
    g_object_class_install_property(gobject_class, PROP_WRITING_TIMEOUT, spec);

    spec = g_param_spec_double("reading-timeout",
                               "Reading timeout",
                               "The timeout seconds for reading reply from a filter",
                               0,
                               G_MAXDOUBLE,
                               MILTER_SERVER_CONTEXT_DEFAULT_READING_TIMEOUT,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
    g_object_class_install_property(gobject_class, PROP_READING_TIMEOUT, spec);

    spec = g_param_spec_double("end-of-message-timeout",
                               "End of message timeout",
                               "The timeout seconds between sending end-of-message to a filter "
                               "and waiting for its reply",
                               0,
                               G_MAXDOUBLE,
                               MILTER_SERVER_CONTEXT_DEFAULT_END_OF_MESSAGE_TIMEOUT,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
    g_object_class_install_property(gobject_class, PROP_END_OF_MESSAGE_TIMEOUT, spec);

    spec = g_param_spec_object("message-result",
                               "Message result",
                               "Result of the current processing message.",
                               MILTER_TYPE_MESSAGE_RESULT,
                               G_PARAM_READWRITE);
    g_object_class_install_property(gobject_class, PROP_MESSAGE_RESULT, spec);

    signals[CONNECTION_TIMEOUT] =
        g_signal_new("connection-timeout",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterServerContextClass,
                                     connection_timeout),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__VOID,
                     G_TYPE_NONE, 0);

    signals[WRITING_TIMEOUT] =
        g_signal_new("writing-timeout",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterServerContextClass,
                                     writing_timeout),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__VOID,
                     G_TYPE_NONE, 0);

    signals[READING_TIMEOUT] =
        g_signal_new("reading-timeout",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterServerContextClass,
                                     reading_timeout),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__VOID,
                     G_TYPE_NONE, 0);

    signals[END_OF_MESSAGE_TIMEOUT] =
        g_signal_new("end-of-message-timeout",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterServerContextClass,
                                     end_of_message_timeout),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__VOID,
                     G_TYPE_NONE, 0);

    signals[MESSAGE_PROCESSED] =
        g_signal_new("message-processed",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterServerContextClass,
                                     message_processed),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__OBJECT,
                     G_TYPE_NONE, 1, MILTER_TYPE_MESSAGE_RESULT);

    signals[STATE_TRANSITED] =
        g_signal_new("state-transited",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterServerContextClass, state_transited),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__ENUM,
                     G_TYPE_NONE, 1, MILTER_TYPE_SERVER_CONTEXT_STATE);

    g_type_class_add_private(gobject_class, sizeof(MilterServerContextPrivate));
}

static void
milter_server_context_init (MilterServerContext *context)
{
    MilterServerContextPrivate *priv;

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(context);

    priv->client_channel = NULL;
    priv->spec = NULL;
    priv->domain = PF_UNSPEC;
    priv->address = NULL;
    priv->address_size = 0;

    priv->status = MILTER_STATUS_NOT_CHANGE;
    priv->envelope_recipient_status = MILTER_STATUS_DEFAULT;
    priv->state = MILTER_SERVER_CONTEXT_STATE_START;
    priv->next_states = NULL;
    priv->last_state = MILTER_SERVER_CONTEXT_STATE_START;

    priv->option = NULL;
    priv->name = NULL;
    priv->body_response_queue = NULL;
    priv->process_body_count = 0;
    priv->sent_end_of_message = FALSE;

    priv->timeout_id = 0;
    priv->connection_timeout = MILTER_SERVER_CONTEXT_DEFAULT_CONNECTION_TIMEOUT;
    priv->writing_timeout = MILTER_SERVER_CONTEXT_DEFAULT_WRITING_TIMEOUT;
    priv->reading_timeout = MILTER_SERVER_CONTEXT_DEFAULT_READING_TIMEOUT;
    priv->end_of_message_timeout =
        MILTER_SERVER_CONTEXT_DEFAULT_END_OF_MESSAGE_TIMEOUT;

    priv->skip_body = FALSE;
    priv->body = g_string_new(NULL);

    priv->elapsed = g_timer_new();
    g_timer_stop(priv->elapsed);
    g_timer_reset(priv->elapsed);

    priv->negotiated = FALSE;
    priv->processing_message = FALSE;
    priv->quitted = FALSE;

    priv->current_recipient = NULL;

    priv->message_result = NULL;
}

static void
disable_timeout (MilterServerContext *context)
{
    MilterServerContextPrivate *priv;

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(context);
    if (milter_need_debug_log()) {
        const gchar *name;

        name = milter_server_context_get_name(context);
        milter_debug("[%u] [server][timeout][disable] [%s] [%u] (%p)",
                     milter_agent_get_tag(MILTER_AGENT(context)),
                     NULL_SAFE_NAME(name),
                     priv->timeout_id,
                     context);
    }
    if (priv->timeout_id > 0) {
        MilterEventLoop *loop;
        loop = milter_agent_get_event_loop(MILTER_AGENT(context));
        milter_event_loop_remove(loop, priv->timeout_id);
        priv->timeout_id = 0;
    }
}

static void
dispose_connect_watch (MilterServerContext *context)
{
    MilterServerContextPrivate *priv;

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(context);
    if (priv->connect_watch_id > 0) {
        MilterEventLoop *loop;
        loop = milter_agent_get_event_loop(MILTER_AGENT(context));
        milter_event_loop_remove(loop, priv->connect_watch_id);
        priv->connect_watch_id = 0;
    }
}

static void
dispose_client_channel (MilterServerContextPrivate *priv)
{
    if (priv->client_channel) {
        g_io_channel_unref(priv->client_channel);
        priv->client_channel = NULL;
    }
}

static void
ensure_message_result (MilterServerContextPrivate *priv)
{
    if (!priv->message_result) {
        priv->message_result = milter_message_result_new();
        milter_message_result_start(priv->message_result);
    }
}

static void
dispose_message_result (MilterServerContextPrivate *priv)
{
    if (priv->message_result) {
        g_object_unref(priv->message_result);
        priv->message_result = NULL;
    }
}

static void
dispose_next_states (MilterServerContext *context)
{
    MilterServerContextPrivate *priv;

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(context);

    if (priv->next_states) {
        if (priv->next_states && milter_need_debug_log()) {
            GList *node;
            GString *unflushed_states;

            unflushed_states = g_string_new(NULL);
            for (node = priv->next_states; node; node = g_list_next(node)) {
                MilterServerContextState state = GPOINTER_TO_UINT(node->data);
                gchar *state_name;

                state_name =
                    milter_utils_get_enum_nick_name(
                        MILTER_TYPE_SERVER_CONTEXT_STATE, state);
                if (unflushed_states->len > 0) {
                    g_string_append(unflushed_states, ",");
                }
                g_string_append(unflushed_states, state_name);
            }
            milter_debug("[%u] [server][dispose][next-states][unflushed] [%s] "
                         "<%s>",
                         milter_agent_get_tag(MILTER_AGENT(context)),
                         NULL_SAFE_NAME(priv->name),
                         unflushed_states->str);
            g_string_free(unflushed_states, TRUE);
        }
        g_list_free(priv->next_states);
        priv->next_states = NULL;
    }
}

static void
dispose (GObject *object)
{
    MilterServerContext *context;
    MilterServerContextPrivate *priv;

    context = MILTER_SERVER_CONTEXT(object);
    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(context);

    milter_debug("[%u] [server][dispose] [%s] (%p)",
                 milter_agent_get_tag(MILTER_AGENT(context)),
                 NULL_SAFE_NAME(priv->name),
                 context);

    disable_timeout(context);
    dispose_connect_watch(context);
    dispose_client_channel(priv);

    if (priv->spec) {
        g_free(priv->spec);
        priv->spec = NULL;
    }

    if (priv->address) {
        g_free(priv->address);
        priv->address = NULL;
    }

    dispose_next_states(context);

    if (priv->option) {
        g_object_unref(priv->option);
        priv->option = NULL;
    }

    if (priv->body) {
        if (priv->body->len > 0) {
            milter_error("[%u] [server][dispose][body][remained] [%s] "
                         "<%" G_GSIZE_FORMAT ">",
                         milter_agent_get_tag(MILTER_AGENT(context)),
                         NULL_SAFE_NAME(priv->name),
                         priv->body->len);
        }
        g_string_free(priv->body, TRUE);
        priv->body = NULL;
    }

    if (priv->name) {
        g_free(priv->name);
        priv->name = NULL;
    }

    if (priv->body_response_queue) {
        g_list_free(priv->body_response_queue);
        priv->body_response_queue = NULL;
    }

    if (priv->elapsed) {
        g_timer_destroy(priv->elapsed);
        priv->elapsed = NULL;
    }

    if (priv->current_recipient) {
        g_free(priv->current_recipient);
        priv->current_recipient = NULL;
    }

    dispose_message_result(priv);

    G_OBJECT_CLASS(milter_server_context_parent_class)->dispose(object);
}

static void
set_property (GObject      *object,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
    MilterServerContext *context;
    MilterServerContextPrivate *priv;

    context = MILTER_SERVER_CONTEXT(object);
    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(context);
    switch (prop_id) {
    case PROP_NAME:
        milter_server_context_set_name(context, g_value_get_string(value));
        break;
    case PROP_STATUS:
        priv->status = g_value_get_enum(value);
        break;
    case PROP_STATE:
        milter_server_context_set_state(context, g_value_get_enum(value));
        break;
    case PROP_CONNECTION_TIMEOUT:
        priv->connection_timeout = g_value_get_double(value);
        break;
    case PROP_WRITING_TIMEOUT:
        priv->writing_timeout = g_value_get_double(value);
        break;
    case PROP_READING_TIMEOUT:
        priv->reading_timeout = g_value_get_double(value);
        break;
    case PROP_END_OF_MESSAGE_TIMEOUT:
        priv->end_of_message_timeout = g_value_get_double(value);
        break;
    case PROP_MESSAGE_RESULT:
        milter_server_context_set_message_result(context,
                                                 g_value_get_object(value));
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
    MilterServerContextPrivate *priv;

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(object);
    switch (prop_id) {
    case PROP_NAME:
        g_value_set_string(value, priv->name);
        break;
    case PROP_STATUS:
        g_value_set_enum(value, priv->status);
        break;
    case PROP_STATE:
        g_value_set_enum(value, priv->state);
        break;
    case PROP_CONNECTION_TIMEOUT:
        g_value_set_double(value, priv->connection_timeout);
        break;
    case PROP_WRITING_TIMEOUT:
        g_value_set_double(value, priv->writing_timeout);
        break;
    case PROP_READING_TIMEOUT:
        g_value_set_double(value, priv->reading_timeout);
        break;
    case PROP_END_OF_MESSAGE_TIMEOUT:
        g_value_set_double(value, priv->end_of_message_timeout);
        break;
    case PROP_MESSAGE_RESULT:
        g_value_set_object(value, priv->message_result);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

GQuark
milter_server_context_error_quark (void)
{
    return g_quark_from_static_string("milter-server-context-error-quark");
}

MilterServerContext *
milter_server_context_new (void)
{
    return g_object_new(MILTER_TYPE_SERVER_CONTEXT, NULL);
}

MilterStatus
milter_server_context_get_status (MilterServerContext *context)
{
    return MILTER_SERVER_CONTEXT_GET_PRIVATE(context)->status;
}

void
milter_server_context_set_status (MilterServerContext *context,
                                  MilterStatus status)
{
    MILTER_SERVER_CONTEXT_GET_PRIVATE(context)->status = status;
}

MilterServerContextState
milter_server_context_get_state (MilterServerContext *context)
{
    return MILTER_SERVER_CONTEXT_GET_PRIVATE(context)->state;
}

void
milter_server_context_set_state (MilterServerContext *context,
                                 MilterServerContextState state)
{
    MilterServerContextPrivate *priv;
    MilterServerContextState previous_state;
    MilterServerContextState previous_last_state;

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(context);
    previous_state = priv->state;
    previous_last_state = priv->last_state;
    if (MILTER_SERVER_CONTEXT_STATE_NEGOTIATE <= state &&
        state <= MILTER_SERVER_CONTEXT_STATE_END_OF_MESSAGE)
        priv->last_state = state;
    priv->state = state;

    if (milter_need_debug_log()) {
        const gchar *name;
        gchar *state_name, *last_state_name;
        gchar *previous_state_name, *previous_last_state_name;

        name = milter_server_context_get_name(context);
        state_name =
            milter_utils_get_enum_nick_name(MILTER_TYPE_SERVER_CONTEXT_STATE,
                                            priv->state);
        last_state_name =
            milter_utils_get_enum_nick_name(MILTER_TYPE_SERVER_CONTEXT_STATE,
                                            priv->last_state);
        previous_state_name =
            milter_utils_get_enum_nick_name(MILTER_TYPE_SERVER_CONTEXT_STATE,
                                            previous_state);
        previous_last_state_name =
            milter_utils_get_enum_nick_name(MILTER_TYPE_SERVER_CONTEXT_STATE,
                                            previous_last_state);

        milter_debug("[%u] [server][state][set] [%s] <%s>:<%s> -> <%s>:<%s>",
                     milter_agent_get_tag(MILTER_AGENT(context)),
                     NULL_SAFE_NAME(name),
                     previous_state_name, previous_last_state_name,
                     state_name, last_state_name);
        g_free(state_name);
        g_free(last_state_name);
        g_free(previous_state_name);
        g_free(previous_last_state_name);
    }
}

MilterServerContextState
milter_server_context_get_last_state (MilterServerContext *context)
{
    return MILTER_SERVER_CONTEXT_GET_PRIVATE(context)->last_state;
}

static gboolean
stop_on_connect (MilterServerContext *context,
               const gchar   *host_name,
               const struct sockaddr *address,
               socklen_t      address_length)
{
    return FALSE;
}

static gboolean
stop_on_helo (MilterServerContext *context, const gchar *fqdn)
{
    return FALSE;
}

static gboolean
stop_on_envelope_from (MilterServerContext *context, const gchar *from)
{
    return FALSE;
}

static gboolean
stop_on_envelope_recipient (MilterServerContext *context, const gchar *recipient)
{
    return FALSE;
}

static gboolean
stop_on_header (MilterServerContext *context,
                const gchar   *name,
                const gchar   *value)
{
    return FALSE;
}

static gboolean
stop_on_body (MilterServerContext *context,
              const gchar   *chunk,
              gsize          size)
{
    return FALSE;
}

static gboolean
cb_writing_timeout (gpointer data)
{
    MilterServerContext *context = data;
    MilterAgent *agent;

    agent = MILTER_AGENT(context);
    if (milter_need_debug_log()) {
        MilterServerContextPrivate *priv;
        const gchar *name = NULL;

        priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(context);
        if (priv)
            name = milter_server_context_get_name(context);
        milter_debug("[%u] [server][timeout][writing] [%s] [%u] (%p)",
                     priv ? milter_agent_get_tag(agent) : 0,
                     NULL_SAFE_NAME(name),
                     priv ? priv->timeout_id : 0,
                     context);
    }
    g_signal_emit(context, signals[WRITING_TIMEOUT], 0);
    milter_agent_shutdown(agent);

    return FALSE;
}

static gboolean
cb_end_of_message_timeout (gpointer data)
{
    MilterServerContext *context = data;
    MilterAgent *agent;

    agent = MILTER_AGENT(context);
    if (milter_need_debug_log()) {
        MilterServerContextPrivate *priv;
        const gchar *name = NULL;

        priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(context);
        if (priv)
            name = milter_server_context_get_name(context);
        milter_debug("[%u] [server][timeout][end-of-message] [%s] [%u] (%p)",
                     priv ? milter_agent_get_tag(agent) : 0,
                     NULL_SAFE_NAME(name),
                     priv ? priv->timeout_id : 0,
                     context);
    }
    g_signal_emit(context, signals[END_OF_MESSAGE_TIMEOUT], 0);
    milter_agent_shutdown(agent);

    return FALSE;
}

static gboolean
cb_reading_timeout (gpointer data)
{
    MilterServerContext *context = data;
    MilterAgent *agent;

    agent = MILTER_AGENT(context);
    if (milter_need_debug_log()) {
        MilterServerContextPrivate *priv;
        const gchar *name = NULL;

        priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(context);
        if (priv)
            name = milter_server_context_get_name(context);
        milter_debug("[%u] [server][timeout][reading] [%s] [%u] (%p)",
                     priv ? milter_agent_get_tag(agent) : 0,
                     NULL_SAFE_NAME(name),
                     priv ? priv->timeout_id : 0,
                     context);
    }
    g_signal_emit(context, signals[READING_TIMEOUT], 0);
    milter_agent_shutdown(agent);

    return FALSE;
}

gboolean
milter_server_context_is_processing (MilterServerContext *context)
{
    MilterServerContextPrivate *priv;

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(context);

    return priv->timeout_id != 0;
}

static GHashTable *
filter_macros (GHashTable *macros, GList *request_symbols)
{
    GList *symbol;
    GHashTable *filtered_macros;

    filtered_macros = g_hash_table_new_full(g_str_hash, g_str_equal,
                                            g_free, g_free);

    for (symbol = request_symbols; symbol; symbol = g_list_next(symbol)) {
        const gchar *value;
        value = g_hash_table_lookup(macros, symbol->data);
        if (value) {
            g_hash_table_insert(filtered_macros,
                                g_strdup(symbol->data),
                                g_strdup(value));
        }
    }

    if (g_hash_table_size(filtered_macros) == 0) {
        g_hash_table_unref(filtered_macros);
        filtered_macros = NULL;
    }

    return filtered_macros;
}

static void
prepend_macro (MilterServerContext *context, GString *packed_packet,
               MilterCommand command)
{
    GHashTable *macros, *filtered_macros = NULL, *target_macros;
    GList *request_symbols = NULL;
    const gchar *packet = NULL;
    gsize packet_size;
    MilterEncoder *encoder;
    MilterAgent *agent;
    MilterProtocolAgent *protocol_agent;
    MilterMacrosRequests *macros_requests;

    agent = MILTER_AGENT(context);
    protocol_agent = MILTER_PROTOCOL_AGENT(context);

    milter_protocol_agent_set_macro_context(protocol_agent, command);
    macros = milter_protocol_agent_get_macros(protocol_agent);
    milter_protocol_agent_set_macro_context(protocol_agent,
                                            MILTER_COMMAND_UNKNOWN);
    if (!macros || g_hash_table_size(macros) == 0)
        return;

    target_macros = macros;
    macros_requests = milter_protocol_agent_get_macros_requests(protocol_agent);

    if (macros_requests) {
        request_symbols =
            milter_macros_requests_get_symbols(macros_requests, command);
        if (request_symbols)
            filtered_macros = filter_macros(macros, request_symbols);
        if (!filtered_macros)
            return;
        target_macros = filtered_macros;
    }

    encoder = milter_agent_get_encoder(agent);
    milter_command_encoder_encode_define_macro(MILTER_COMMAND_ENCODER(encoder),
                                               &packet, &packet_size,
                                               command,
                                               target_macros);
    if (filtered_macros)
        g_hash_table_unref(filtered_macros);

    if (milter_need_debug_log()) {
        gchar *command_name;
        gchar *inspected_macros;

        command_name =
            milter_utils_get_enum_nick_name(MILTER_TYPE_COMMAND, command);
        inspected_macros =
            milter_utils_inspect_hash_string_string(target_macros);
        milter_debug("[%u] [server][send][%s][macros] %s: %s",
                     milter_agent_get_tag(agent),
                     command_name,
                     inspected_macros,
                     milter_server_context_get_name(context));
        g_free(command_name);
        g_free(inspected_macros);
    }

    g_string_prepend_len(packed_packet, packet, packet_size);
}

static void
emit_message_processed_signal (MilterServerContext *context)
{
    MilterServerContextPrivate *priv;
    MilterState next_state = MILTER_STATE_INVALID;

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(context);
    if (!priv->message_result)
        return;

    switch (priv->state) {
    case MILTER_SERVER_CONTEXT_STATE_ENVELOPE_FROM:
        next_state = MILTER_STATE_ENVELOPE_FROM_REPLIED;
        break;
    case MILTER_SERVER_CONTEXT_STATE_ENVELOPE_RECIPIENT:
        next_state = MILTER_STATE_ENVELOPE_RECIPIENT_REPLIED;
        break;
    case MILTER_SERVER_CONTEXT_STATE_DATA:
        next_state = MILTER_STATE_DATA_REPLIED;
        break;
    case MILTER_SERVER_CONTEXT_STATE_HEADER:
        next_state = MILTER_STATE_HEADER_REPLIED;
        break;
    case MILTER_SERVER_CONTEXT_STATE_END_OF_HEADER:
        next_state = MILTER_STATE_END_OF_HEADER_REPLIED;
        break;
    case MILTER_SERVER_CONTEXT_STATE_BODY:
        next_state = MILTER_STATE_BODY_REPLIED;
        break;
    case MILTER_SERVER_CONTEXT_STATE_END_OF_MESSAGE:
        next_state = MILTER_STATE_END_OF_MESSAGE_REPLIED;
        break;
    default:
        break;
    }

    if (next_state != MILTER_STATE_INVALID)
        milter_message_result_set_state(priv->message_result, next_state);
    milter_message_result_set_status(priv->message_result, priv->status);
    milter_message_result_stop(priv->message_result);
    milter_message_result_set_elapsed_time(priv->message_result,
                                           g_timer_elapsed(priv->elapsed, NULL));
    g_signal_emit_by_name(context, "message-processed", priv->message_result);
    g_object_unref(priv->message_result);
    priv->message_result = NULL;
}

gboolean
milter_server_context_need_reply (MilterServerContext *context,
                                  MilterServerContextState state)
{
    MilterServerContextPrivate *priv;
    MilterStepFlags step;

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(context);
    step = milter_option_get_step(priv->option);

    switch (state) {
    case MILTER_SERVER_CONTEXT_STATE_CONNECT:
        return !(step & MILTER_STEP_NO_REPLY_CONNECT);
        break;
    case MILTER_SERVER_CONTEXT_STATE_HELO:
        return !(step & MILTER_STEP_NO_REPLY_HELO);
        break;
    case MILTER_SERVER_CONTEXT_STATE_ENVELOPE_FROM:
        return !(step & MILTER_STEP_NO_REPLY_ENVELOPE_FROM);
        break;
    case MILTER_SERVER_CONTEXT_STATE_ENVELOPE_RECIPIENT:
        return !(step & MILTER_STEP_NO_REPLY_ENVELOPE_RECIPIENT);
        break;
    case MILTER_SERVER_CONTEXT_STATE_DATA:
        return !(step & MILTER_STEP_NO_REPLY_DATA);
        break;
    case MILTER_SERVER_CONTEXT_STATE_HEADER:
        return !(step & MILTER_STEP_NO_REPLY_HEADER);
        break;
    case MILTER_SERVER_CONTEXT_STATE_END_OF_HEADER:
        return !(step & MILTER_STEP_NO_REPLY_END_OF_HEADER);
        break;
    case MILTER_SERVER_CONTEXT_STATE_BODY:
        return !(step & MILTER_STEP_NO_REPLY_BODY);
        break;
    case MILTER_SERVER_CONTEXT_STATE_UNKNOWN:
        return !(step & MILTER_STEP_NO_REPLY_UNKNOWN);
        break;
    default:
        return TRUE;
        break;
    }
}

static void
reset_end_of_message_timeout (MilterServerContext *context)
{
    MilterAgent *agent;
    MilterEventLoop *loop;
    MilterServerContextPrivate *priv;

    disable_timeout(context);

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(context);
    agent = MILTER_AGENT(context);
    loop = milter_agent_get_event_loop(agent);
    priv->timeout_id =
        milter_event_loop_add_timeout(loop,
                                      priv->end_of_message_timeout,
                                      cb_end_of_message_timeout,
                                      context);
    if (milter_need_debug_log()) {
        const gchar *name;

        name = milter_server_context_get_name(context);
        milter_debug("[%u] [server][timeout][end-of-message][registered][%g] "
                     "[%s] <%u> (%p)",
                     milter_agent_get_tag(agent),
                     priv->end_of_message_timeout,
                     NULL_SAFE_NAME(name),
                     priv->timeout_id,
                     context);
    }
}

static void
append_body_response_queue (MilterServerContext *context)
{
    MilterServerContextPrivate *priv;

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(context);
    priv->body_response_queue = g_list_append(priv->body_response_queue,
                                              GUINT_TO_POINTER(0));
}

static void
increment_process_body_count (MilterServerContext *context)
{
    MilterServerContextPrivate *priv;
    GList *last_node;
    guint process_body_count;

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(context);

    last_node = g_list_last(priv->body_response_queue);
    process_body_count = GPOINTER_TO_UINT(last_node->data);
    process_body_count++;
    last_node->data = GUINT_TO_POINTER(process_body_count);
}

static gboolean
flush_body (MilterServerContext *context)
{
    MilterServerContextPrivate *priv;
    guint tag = 0;
    const gchar *name = NULL;
    MilterEncoder *encoder;
    MilterCommandEncoder *command_encoder;
    MilterServerContextState state = MILTER_SERVER_CONTEXT_STATE_BODY;
    const gchar *packet = NULL;
    gsize packet_size;
    gsize packed_size;

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(context);

    if (milter_need_debug_log()) {
        tag = milter_agent_get_tag(MILTER_AGENT(context));
        name = milter_server_context_get_name(context);
    }

    milter_debug("[%u] [server][body][flush] [%s] <%" G_GSIZE_FORMAT ">",
                 tag, NULL_SAFE_NAME(name), priv->body->len);

    milter_debug("[%u] [server][timer][continue] [%s] %g",
                 tag, NULL_SAFE_NAME(name),
                 g_timer_elapsed(priv->elapsed, NULL));
    g_timer_continue(priv->elapsed);
    encoder = milter_agent_get_encoder(MILTER_AGENT(context));
    command_encoder = MILTER_COMMAND_ENCODER(encoder);
    append_body_response_queue(context);

    milter_command_encoder_encode_body(command_encoder,
                                       &packet, &packet_size,
                                       priv->body->str,
                                       priv->body->len,
                                       &packed_size);
    if (!write_packet(context, packet, packet_size, state))
        return FALSE;

    g_string_erase(priv->body, 0, packed_size);
    increment_process_body_count(context);

    g_timer_stop(priv->elapsed);
    milter_debug("[%u] [server][timer][stop] [%s] <%g>",
                 tag, NULL_SAFE_NAME(name),
                 g_timer_elapsed(priv->elapsed, NULL));

    return TRUE;
}

static gboolean
process_next_state_body (MilterServerContext *context)
{
    MilterServerContextPrivate *priv;
    guint tag = 0;
    const gchar *name = NULL;
    MilterServerContextState state = MILTER_SERVER_CONTEXT_STATE_BODY;
    MilterEventLoop *loop;

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(context);

    if (milter_need_debug_log()) {
        tag = milter_agent_get_tag(MILTER_AGENT(context));
        name = milter_server_context_get_name(context);
    }

    milter_debug("[%u] [server][flushed][next][body] [%s] <%" G_GSIZE_FORMAT ">",
                 tag, NULL_SAFE_NAME(name), priv->body->len);

    if (priv->body->len == 0)
        return TRUE;

    if (!milter_server_context_need_reply(context, state))
        return flush_body(context);

    disable_timeout(context);
    loop = milter_agent_get_event_loop(MILTER_AGENT(context));
    priv->timeout_id =
        milter_event_loop_add_timeout(loop,
                                      priv->reading_timeout,
                                      cb_reading_timeout,
                                      context);
    milter_debug("[%u] [server][timeout][reading][registered][%g] "
                 "[%s] <%u> (%p)",
                 tag,
                 priv->reading_timeout,
                 NULL_SAFE_NAME(name),
                 priv->timeout_id,
                 context);

    return TRUE;
}

static void
process_next_state (MilterServerContext *context,
                    MilterServerContextState next_state)
{
    MilterAgent *agent;
    MilterServerContextPrivate *priv;
    guint tag = 0;
    const gchar *name = NULL;
    gchar *state_name = NULL;
    gchar *next_state_name = NULL;

    agent = MILTER_AGENT(context);
    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(context);

    if (milter_need_debug_log()) {
        tag = milter_agent_get_tag(agent);
        name = milter_server_context_get_name(context);
        state_name =
            milter_utils_get_enum_nick_name(MILTER_TYPE_SERVER_CONTEXT_STATE,
                                            priv->state);
        next_state_name =
            milter_utils_get_enum_nick_name(MILTER_TYPE_SERVER_CONTEXT_STATE,
                                            next_state);
    }

    milter_debug("[%u] [server][flushed][next] [%s] <%s> -> <%s>",
                 tag, NULL_SAFE_NAME(name), state_name, next_state_name);

    if (next_state == MILTER_SERVER_CONTEXT_STATE_INVALID) {
        GError *error = NULL;

        if (!state_name) {
            state_name =
                milter_utils_get_enum_nick_name(MILTER_TYPE_SERVER_CONTEXT_STATE,
                                                priv->state);
        }

        g_set_error(&error,
                    MILTER_SERVER_CONTEXT_ERROR,
                    MILTER_SERVER_CONTEXT_ERROR_INVALID_STATE,
                    "flushed with invalid next state: <%s>",
                    state_name);
        g_free(state_name);
        if (next_state_name)
            g_free(next_state_name);
        milter_error("[%u] [server][flushed][error][next-state][invalid] "
                     "[%s] %s",
                     tag, NULL_SAFE_NAME(name), error->message);
        milter_error_emittable_emit(MILTER_ERROR_EMITTABLE(context), error);
        g_error_free(error);
        return;
    }

    disable_timeout(context);

    switch (next_state) {
    case MILTER_SERVER_CONTEXT_STATE_END_OF_MESSAGE:
        if (priv->process_body_count == 0)
            milter_server_context_set_state(context, next_state);
        else
            priv->sent_end_of_message = TRUE;
        reset_end_of_message_timeout(context);
        break;
    case MILTER_SERVER_CONTEXT_STATE_BODY:
        milter_server_context_set_state(context, next_state);
        process_next_state_body(context);
        break;
    case MILTER_SERVER_CONTEXT_STATE_QUIT:
        milter_agent_shutdown(agent);
        milter_server_context_set_state(context, next_state);
        break;
    case MILTER_SERVER_CONTEXT_STATE_ABORT:
        if (MILTER_STATUS_IS_PASS(priv->status) &&
            (MILTER_SERVER_CONTEXT_STATE_DEFINE_MACRO <= priv->state &&
             priv->state < MILTER_SERVER_CONTEXT_STATE_END_OF_MESSAGE)) {
            if (priv->state == MILTER_SERVER_CONTEXT_STATE_ENVELOPE_RECIPIENT &&
                priv->envelope_recipient_status != MILTER_STATUS_DEFAULT) {
                priv->status = priv->envelope_recipient_status;
            } else {
                priv->status = MILTER_STATUS_ABORT;
            }
        }
        milter_server_context_set_state(context, next_state);
        emit_message_processed_signal(context);
        milter_server_context_reset_message_related_data(context);
        break;
    case MILTER_SERVER_CONTEXT_STATE_ENVELOPE_FROM:
        priv->processing_message = TRUE;
        /* FALLTHROUGH */
    default:
        milter_server_context_set_state(context, next_state);
        if (milter_server_context_need_reply(context, next_state)) {
            MilterEventLoop *loop;

            loop = milter_agent_get_event_loop(agent);
            priv->timeout_id =
                milter_event_loop_add_timeout(loop,
                                              priv->reading_timeout,
                                              cb_reading_timeout,
                                              context);
            milter_debug("[%u] [server][timeout][reading][registered][%g] "
                         "[%s] <%u> (%p)",
                         tag,
                         priv->reading_timeout,
                         NULL_SAFE_NAME(name),
                         priv->timeout_id,
                         context);
        } else {
            g_timer_stop(priv->elapsed);
        }
        break;
    }

    if (state_name)
        g_free(state_name);
    if (next_state_name)
        g_free(next_state_name);
}

static void
flushed (MilterAgent *agent)
{
    MilterServerContext *context;
    MilterServerContextPrivate *priv;
    guint tag = 0;
    const gchar *name = NULL;
    GList *node, *next_states;

    context = MILTER_SERVER_CONTEXT(agent);
    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(context);

    if (milter_need_debug_log()) {
        gchar *state_name = NULL;
        GString *next_state_names = NULL;

        tag = milter_agent_get_tag(agent);
        name = milter_server_context_get_name(context);
        state_name =
            milter_utils_get_enum_nick_name(MILTER_TYPE_SERVER_CONTEXT_STATE,
                                            priv->state);
        next_state_names = g_string_new(NULL);
        for (node = priv->next_states; node; node = g_list_next(node)) {
            MilterServerContextState next_state;
            gchar *next_state_name;

            next_state = GPOINTER_TO_UINT(node->data);
            next_state_name =
                milter_utils_get_enum_nick_name(MILTER_TYPE_SERVER_CONTEXT_STATE,
                                                next_state);
            if (next_state_names->len > 0)
                g_string_append(next_state_names, ",");
            g_string_append(next_state_names, next_state_name);
            g_free(next_state_name);
        }
        milter_debug("[%u] [server][flushed] [%s] <%s> -> <%s>",
                     tag, NULL_SAFE_NAME(name),
                     state_name, next_state_names->str);
        g_free(state_name);
        g_string_free(next_state_names, TRUE);
    }

    next_states = priv->next_states;
    priv->next_states = NULL;
    for (node = next_states; node; node = g_list_next(node)) {
        MilterServerContextState next_state;

        next_state = GPOINTER_TO_UINT(node->data);
        process_next_state(context, next_state);
        g_signal_emit(context, signals[STATE_TRANSITED], 0, next_state);
    }
    g_list_free(next_states);

    milter_debug("[%u] [server][flushed][done] [%s]",
                 tag, NULL_SAFE_NAME(name));
}

static gboolean
write_packet (MilterServerContext *context,
              const gchar *packet, gsize packet_size,
              MilterServerContextState next_state)
{
    GError *agent_error = NULL;
    MilterServerContextPrivate *priv;
    GString *packed_packet;
    guint tag;
    MilterEventLoop *loop;
    const gchar *name;

    if (!packet)
        return FALSE;

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(context);

    tag = milter_agent_get_tag(MILTER_AGENT(context));
    loop = milter_agent_get_event_loop(MILTER_AGENT(context));
    name = milter_server_context_get_name(context);

    milter_debug("[%u] [server][write] [%s] (%p)",
                 tag, name, context);

    switch (next_state) {
    case MILTER_SERVER_CONTEXT_STATE_ABORT:
    case MILTER_SERVER_CONTEXT_STATE_QUIT:
        break;
    default:
        if (milter_server_context_is_processing(context)) {
            gchar *inspected_current_state;
            gchar *inspected_next_state;
            GError *error = NULL;

            inspected_current_state =
                milter_utils_get_enum_nick_name(MILTER_TYPE_SERVER_CONTEXT_STATE,
                                                priv->state);
            inspected_next_state =
                milter_utils_get_enum_nick_name(MILTER_TYPE_SERVER_CONTEXT_STATE,
                                                next_state);
            g_set_error(&error,
                        MILTER_SERVER_CONTEXT_ERROR,
                        MILTER_SERVER_CONTEXT_ERROR_BUSY,
                        "previous command has been processing: %s -> %s",
                        inspected_current_state, inspected_next_state);
            milter_error("[%u] [server][error] [%s] %s",
                         tag, name, error->message);
            milter_error_emittable_emit(MILTER_ERROR_EMITTABLE(context),
                                        error);
            g_error_free(error);
            g_free(inspected_current_state);
            g_free(inspected_next_state);
            return FALSE;
        }
        if (next_state != MILTER_SERVER_CONTEXT_STATE_NEGOTIATE &&
            next_state != MILTER_SERVER_CONTEXT_STATE_BODY) {
            milter_debug("[%u] [server][timer][continue] %g: %s",
                         tag, g_timer_elapsed(priv->elapsed, NULL), name);
            g_timer_continue(priv->elapsed);
        }
        disable_timeout(context);
        priv->timeout_id = milter_event_loop_add_timeout(loop,
                                                         priv->writing_timeout,
                                                         cb_writing_timeout,
                                                         context);
        if (milter_need_debug_log()) {
            const gchar *name;

            name = milter_server_context_get_name(context);
            milter_debug("[%u] [server][timeout][writing][registered][%g] "
                         "[%s] <%u> (%p)",
                         tag,
                         priv->writing_timeout,
                         NULL_SAFE_NAME(name),
                         priv->timeout_id,
                         context);
        }
        break;
    }

    packed_packet = g_string_new_len(packet, packet_size);
    switch (next_state) {
    case MILTER_SERVER_CONTEXT_STATE_HELO:
        prepend_macro(context, packed_packet, MILTER_COMMAND_HELO);
        break;
    case MILTER_SERVER_CONTEXT_STATE_CONNECT:
        prepend_macro(context, packed_packet, MILTER_COMMAND_CONNECT);
        break;
    case MILTER_SERVER_CONTEXT_STATE_ENVELOPE_FROM:
        prepend_macro(context, packed_packet, MILTER_COMMAND_ENVELOPE_FROM);
        break;
    case MILTER_SERVER_CONTEXT_STATE_ENVELOPE_RECIPIENT:
        prepend_macro(context, packed_packet, MILTER_COMMAND_ENVELOPE_RECIPIENT);
        break;
    case MILTER_SERVER_CONTEXT_STATE_DATA:
        prepend_macro(context, packed_packet, MILTER_COMMAND_DATA);
        break;
    case MILTER_SERVER_CONTEXT_STATE_HEADER:
        prepend_macro(context, packed_packet, MILTER_COMMAND_HEADER);
        break;
    case MILTER_SERVER_CONTEXT_STATE_END_OF_HEADER:
        prepend_macro(context, packed_packet, MILTER_COMMAND_END_OF_HEADER);
        break;
    case MILTER_SERVER_CONTEXT_STATE_BODY:
        prepend_macro(context, packed_packet, MILTER_COMMAND_BODY);
        break;
    case MILTER_SERVER_CONTEXT_STATE_END_OF_MESSAGE:
        prepend_macro(context, packed_packet, MILTER_COMMAND_END_OF_MESSAGE);
        break;
    default:
        break;
    }

    milter_agent_write_packet(MILTER_AGENT(context),
                              packed_packet->str, packed_packet->len,
                              &agent_error);
    g_string_free(packed_packet, TRUE);

    if (agent_error) {
        GError *error = NULL;

        disable_timeout(context);
        milter_utils_set_error_with_sub_error(
            &error,
            MILTER_SERVER_CONTEXT_ERROR,
            MILTER_SERVER_CONTEXT_ERROR_IO_ERROR,
            agent_error,
            "Failed to write to milter");
        milter_error("[%u] [server][error][write] [%s] %s",
                     tag, NULL_SAFE_NAME(name), error->message);
        milter_error_emittable_emit(MILTER_ERROR_EMITTABLE(context),
                                    error);
        g_error_free(error);

        g_timer_stop(priv->elapsed);
        milter_debug("[%u] [server][timer][stop] [%s] <%g>",
                     tag, NULL_SAFE_NAME(name),
                     g_timer_elapsed(priv->elapsed, NULL));

        return FALSE;
    }

    priv->next_states = g_list_append(priv->next_states,
                                      GUINT_TO_POINTER(next_state));
    return TRUE;
}

static void
stop_on_state (MilterServerContext *context, MilterServerContextState state)
{
    MilterServerContextPrivate *priv;

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(context);

    if (milter_need_debug_log()) {
        gchar *inspected_state;
        inspected_state =
            milter_utils_get_enum_nick_name(MILTER_TYPE_SERVER_CONTEXT_STATE,
                                            state);
        milter_debug("[%u] [server][stop][%s] %s",
                     milter_agent_get_tag(MILTER_AGENT(context)),
                     inspected_state,
                     milter_server_context_get_name(context));
        g_free(inspected_state);
    }

    priv->status = MILTER_STATUS_STOP;
    milter_server_context_set_state(context, state);

    g_signal_emit_by_name(context, "stopped");

    if (MILTER_SERVER_CONTEXT_STATE_ENVELOPE_FROM <= priv->state &&
        priv->state <= MILTER_SERVER_CONTEXT_STATE_END_OF_MESSAGE) {
        emit_message_processed_signal(context);
    }
}

gboolean
milter_server_context_negotiate (MilterServerContext *context,
                                 MilterOption        *option)
{
    const gchar *packet = NULL;
    gsize packet_size;
    MilterEncoder *encoder;
    MilterServerContextPrivate *priv;

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(context);

    g_timer_start(priv->elapsed);
    if (milter_need_debug_log()) {
        guint tag;
        const gchar *name;
        gchar *inspected_option;

        tag = milter_agent_get_tag(MILTER_AGENT(context));
        name = milter_server_context_get_name(context);

        milter_debug("[%u] [server][timer][start] %s", tag, name);

        inspected_option = milter_option_inspect(option);
        milter_debug("[%u] [server][send][negotiate] %s: %s",
                     tag, inspected_option, name);
        g_free(inspected_option);
    }

    if (priv->option)
        g_object_unref(priv->option);
    priv->option = milter_option_copy(option);

    encoder = milter_agent_get_encoder(MILTER_AGENT(context));
    milter_command_encoder_encode_negotiate(MILTER_COMMAND_ENCODER(encoder),
                                            &packet, &packet_size, option);

    return write_packet(context, packet, packet_size,
                        MILTER_SERVER_CONTEXT_STATE_NEGOTIATE);
}

gboolean
milter_server_context_helo (MilterServerContext *context,
                            const gchar *fqdn)
{
    const gchar *packet = NULL;
    gsize packet_size;
    MilterEncoder *encoder;
    gboolean stop = FALSE;
    guint tag = 0;
    const gchar *name = NULL;

    if (milter_need_debug_log()) {
        tag = milter_agent_get_tag(MILTER_AGENT(context));
        name = milter_server_context_get_name(context);
    }

    milter_debug("[%u] [server][send][helo] <%s>: %s", tag, fqdn, name);

    g_signal_emit(context, signals[STOP_ON_HELO], 0, fqdn, &stop);
    if (stop) {
        stop_on_state(context, MILTER_SERVER_CONTEXT_STATE_HELO);
        return TRUE;
    }

    if (milter_server_context_is_enable_step(context, MILTER_STEP_NO_HELO)) {
        milter_debug("[%u] [server][helo][skip] %s", tag, name);
        milter_server_context_set_state(context,
                                        MILTER_SERVER_CONTEXT_STATE_HELO);
        g_signal_emit_by_name(context, "continue");
        return TRUE;
    }

    encoder = milter_agent_get_encoder(MILTER_AGENT(context));
    milter_command_encoder_encode_helo(MILTER_COMMAND_ENCODER(encoder),
                                       &packet, &packet_size, fqdn);

    return write_packet(context, packet, packet_size,
                        MILTER_SERVER_CONTEXT_STATE_HELO);
}

void
milter_server_context_set_option (MilterServerContext *context,
                                  MilterOption        *option)
{
    MilterServerContextPrivate *priv;

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(context);

    if (priv->option)
        g_object_unref(priv->option);

    if (option)
        priv->option = milter_option_copy(option);
    else
        priv->option = NULL;
}

MilterOption *
milter_server_context_get_option (MilterServerContext *context)
{
    return MILTER_SERVER_CONTEXT_GET_PRIVATE(context)->option;
}

gboolean
milter_server_context_is_enable_step (MilterServerContext *context,
                                      MilterStepFlags step)
{
    MilterServerContextPrivate *priv;

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(context);

    return priv->option ? (milter_option_get_step(priv->option) & step) : TRUE;
}

gboolean
milter_server_context_connect (MilterServerContext *context,
                               const gchar         *host_name,
                               struct sockaddr     *address,
                               socklen_t            address_length)
{
    const gchar *packet = NULL;
    gsize packet_size;
    MilterEncoder *encoder;
    gboolean stop = FALSE;
    guint tag = 0;
    const gchar *name = NULL;

    if (milter_need_debug_log()) {
        tag = milter_agent_get_tag(MILTER_AGENT(context));
        name = milter_server_context_get_name(context);
    }

    milter_debug("[%u] [server][send][connect] <%s>: %s",
                 tag, host_name, name);

    milter_protocol_agent_set_macro_context(MILTER_PROTOCOL_AGENT(context),
                                            MILTER_COMMAND_CONNECT);
    milter_debug("[%u] [server][stop-on-connect][start] %s", tag, name);
    g_signal_emit(context, signals[STOP_ON_CONNECT], 0,
                  host_name, address, address_length, &stop);
    milter_debug("[%u] [server][stop-on-connect][end] %s: stop=<%s>",
                 tag, name, stop ? "true" : "false");
    if (stop) {
        stop_on_state(context, MILTER_SERVER_CONTEXT_STATE_CONNECT);
        return TRUE;
    }

    if (milter_server_context_is_enable_step(context, MILTER_STEP_NO_CONNECT)) {
        milter_debug("[%u] [server][connect][skip] %s", tag, name);
        milter_server_context_set_state(context,
                                        MILTER_SERVER_CONTEXT_STATE_CONNECT);
        g_signal_emit_by_name(context, "continue");
        return TRUE;
    }

    encoder = milter_agent_get_encoder(MILTER_AGENT(context));
    milter_command_encoder_encode_connect(MILTER_COMMAND_ENCODER(encoder),
                                          &packet, &packet_size,
                                          host_name, address, address_length);

    return write_packet(context, packet, packet_size,
                        MILTER_SERVER_CONTEXT_STATE_CONNECT);
}

void
milter_server_context_reset_message_related_data (MilterServerContext *context)
{
    MilterServerContextPrivate *priv;
    MilterProtocolAgent *agent;

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(context);

    agent = MILTER_PROTOCOL_AGENT(context);
    milter_protocol_agent_clear_message_related_macros(agent);

    priv->skip_body = FALSE;
    priv->processing_message = FALSE;
    priv->envelope_recipient_status = MILTER_STATUS_DEFAULT;

    if (priv->body_response_queue) {
        g_list_free(priv->body_response_queue);
        priv->body_response_queue = NULL;
    }
    priv->process_body_count = 0;
    priv->sent_end_of_message = FALSE;

    dispose_message_result(priv);
}

gboolean
milter_server_context_envelope_from (MilterServerContext *context,
                                     const gchar         *from)
{
    MilterServerContextPrivate *priv;
    const gchar *packet = NULL;
    gsize packet_size;
    MilterEncoder *encoder;
    gboolean stop = FALSE;
    guint tag = 0;
    const gchar *name = NULL;

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(context);
    priv->status = MILTER_STATUS_NOT_CHANGE;

    if (milter_need_debug_log()) {
        tag = milter_agent_get_tag(MILTER_AGENT(context));
        name = milter_server_context_get_name(context);
    }

    milter_debug("[%u] [server][send][envelope-from] <%s>: %s",
                 tag, from, name);

    /* TODO:
     * Those information will be cleared if both [abort]
     * packet and [envelope-from] packet are flushed at the
     * same time. Because
     * milter_server_context_reset_message_related_data() is
     * called in process_next_state() when [abort] packet is
     * flushed:
     *
     *   milter_server_context_abort() ->
     *   milter_server_context_envelope_from() ->
     *   (flushed) ->
     *   process_next_state() for [abort] ->
     *   milter_server_context_reset_message_related_data()
     *     resets data set at milter_server_context_envelope_from().
     */
    ensure_message_result(priv);
    milter_message_result_set_from(priv->message_result, from);
    milter_message_result_set_state(priv->message_result,
                                    MILTER_STATE_ENVELOPE_FROM);

    milter_protocol_agent_set_macro_context(MILTER_PROTOCOL_AGENT(context),
                                            MILTER_COMMAND_ENVELOPE_FROM);
    milter_debug("[%u] [server][stop-on-envelope-from][start] %s", tag, name);
    g_signal_emit(context, signals[STOP_ON_ENVELOPE_FROM], 0, from, &stop);
    milter_debug("[%u] [server][stop-on-envelope-from][end] %s: stop=<%s>",
                 tag, name, stop ? "true" : "false");
    if (stop) {
        stop_on_state(context, MILTER_SERVER_CONTEXT_STATE_ENVELOPE_FROM);
        return TRUE;
    }

    priv->processing_message = TRUE;
    if (milter_server_context_is_enable_step(context,
                                             MILTER_STEP_NO_ENVELOPE_FROM)) {
        milter_debug("[%u] [server][envelope-from][skip] %s", tag, name);
        milter_server_context_set_state(
            context, MILTER_SERVER_CONTEXT_STATE_ENVELOPE_FROM);
        g_signal_emit_by_name(context, "continue");
        return TRUE;
    }

    encoder = milter_agent_get_encoder(MILTER_AGENT(context));
    milter_command_encoder_encode_envelope_from(MILTER_COMMAND_ENCODER(encoder),
                                                &packet, &packet_size, from);

    return write_packet(context, packet, packet_size,
                        MILTER_SERVER_CONTEXT_STATE_ENVELOPE_FROM);
}

gboolean
milter_server_context_envelope_recipient (MilterServerContext *context,
                                          const gchar         *recipient)
{
    MilterServerContextPrivate *priv;
    const gchar *packet = NULL;
    gsize packet_size;
    MilterEncoder *encoder;
    MilterCommandEncoder *command_encoder;
    gboolean stop = FALSE;
    guint tag = 0;
    const gchar *name = NULL;

    if (milter_need_debug_log()) {
        tag = milter_agent_get_tag(MILTER_AGENT(context));
        name = milter_server_context_get_name(context);
    }

    milter_debug("[%u] [server][send][envelope-recipient] <%s>: %s",
                 tag, recipient, name);

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(context);

    if (priv->current_recipient)
        g_free(priv->current_recipient);
    priv->current_recipient = g_strdup(recipient);

    ensure_message_result(priv);
    milter_message_result_add_recipient(priv->message_result, recipient);
    milter_message_result_set_state(priv->message_result,
                                    MILTER_STATE_ENVELOPE_RECIPIENT);

    milter_protocol_agent_set_macro_context(MILTER_PROTOCOL_AGENT(context),
                                            MILTER_COMMAND_ENVELOPE_RECIPIENT);
    milter_debug("[%u] [server][stop-on-envelope-recipient][start] %s",
                 tag, name);
    g_signal_emit(context, signals[STOP_ON_ENVELOPE_RECIPIENT], 0,
                  recipient, &stop);
    milter_debug("[%u] [server][stop-on-envelope-recipient][end] %s: stop=<%s>",
                 tag, name, stop ? "true" : "false");
    if (stop) {
        stop_on_state(context, MILTER_SERVER_CONTEXT_STATE_ENVELOPE_RECIPIENT);
        return TRUE;
    }

    if (milter_server_context_is_enable_step(
            context, MILTER_STEP_NO_ENVELOPE_RECIPIENT)) {
        milter_debug("[%u] [server][envelope-recipient][skip] %s", tag, name);
        milter_server_context_set_state(
            context, MILTER_SERVER_CONTEXT_STATE_ENVELOPE_RECIPIENT);
        g_signal_emit_by_name(context, "continue");
        return TRUE;
    }

    encoder = milter_agent_get_encoder(MILTER_AGENT(context));
    command_encoder = MILTER_COMMAND_ENCODER(encoder);
    milter_command_encoder_encode_envelope_recipient(command_encoder,
                                                     &packet, &packet_size,
                                                     recipient);

    return write_packet(context, packet, packet_size,
                        MILTER_SERVER_CONTEXT_STATE_ENVELOPE_RECIPIENT);
}

gboolean
milter_server_context_data (MilterServerContext *context)
{
    MilterServerContextPrivate *priv;
    const gchar *packet = NULL;
    gsize packet_size;
    MilterEncoder *encoder;
    gboolean stop = FALSE;
    guint tag = 0;
    const gchar *name = NULL;

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(context);

    if (milter_need_debug_log()) {
        tag = milter_agent_get_tag(MILTER_AGENT(context));
        name = milter_server_context_get_name(context);
    }

    milter_debug("[%u] [server][send][data] %s", tag, name);

    ensure_message_result(priv);
    milter_message_result_set_state(priv->message_result, MILTER_STATE_DATA);

    milter_protocol_agent_set_macro_context(MILTER_PROTOCOL_AGENT(context),
                                            MILTER_COMMAND_DATA);
    milter_debug("[%u] [server][stop-on-data][start] %s", tag, name);
    g_signal_emit(context, signals[STOP_ON_DATA], 0, &stop);
    milter_debug("[%u] [server][stop-on-data][end] %s: stop=<%s>",
                 tag, name, stop ? "true" : "false");
    if (stop) {
        stop_on_state(context, MILTER_SERVER_CONTEXT_STATE_DATA);
        return TRUE;
    }

    if (milter_option_get_version(priv->option) < 4 ||
        milter_server_context_is_enable_step(context, MILTER_STEP_NO_DATA)) {
        milter_debug("[%u] [server][data][skip] %s", tag, name);
        milter_server_context_set_state(context,
                                        MILTER_SERVER_CONTEXT_STATE_DATA);
        g_signal_emit_by_name(context, "continue");
        return TRUE;
    }

    encoder = milter_agent_get_encoder(MILTER_AGENT(context));
    milter_command_encoder_encode_data(MILTER_COMMAND_ENCODER(encoder),
                                       &packet, &packet_size);

    return write_packet(context, packet, packet_size,
                        MILTER_SERVER_CONTEXT_STATE_DATA);
}

gboolean
milter_server_context_unknown (MilterServerContext *context,
                               const gchar         *command)
{
    const gchar *packet = NULL;
    gsize packet_size;
    MilterEncoder *encoder;
    guint tag = 0;
    const gchar *name = NULL;

    if (milter_need_debug_log()) {
        tag = milter_agent_get_tag(MILTER_AGENT(context));
        name = milter_server_context_get_name(context);
    }

    milter_debug("[%u] [server][send][unknown] [%s] <%s>",
                 tag, NULL_SAFE_NAME(name), command);

    if (milter_server_context_is_enable_step(context, MILTER_STEP_NO_UNKNOWN)) {
        milter_debug("[%u] [server][unknown][skip] [%s]",
                     tag, NULL_SAFE_NAME(name));
        milter_server_context_set_state(context,
                                        MILTER_SERVER_CONTEXT_STATE_UNKNOWN);
        g_signal_emit_by_name(context, "continue");
        return TRUE;
    }

    encoder = milter_agent_get_encoder(MILTER_AGENT(context));
    milter_command_encoder_encode_unknown(MILTER_COMMAND_ENCODER(encoder),
                                          &packet, &packet_size, command);

    return write_packet(context, packet, packet_size,
                        MILTER_SERVER_CONTEXT_STATE_UNKNOWN);
}

gboolean
milter_server_context_header (MilterServerContext *context,
                              const gchar         *header_name,
                              const gchar         *header_value)
{
    MilterServerContextPrivate *priv;
    const gchar *packet = NULL;
    gsize packet_size;
    MilterEncoder *encoder;
    gboolean stop = FALSE;
    guint tag;
    const gchar *name;
    MilterHeaders *headers;

    tag = milter_agent_get_tag(MILTER_AGENT(context));
    name = milter_server_context_get_name(context);
    milter_debug("[%u] [server][send][header] [%s] <%s>=<%s>",
                 tag, NULL_SAFE_NAME(name), header_name, header_value);

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(context);

    ensure_message_result(priv);
    headers = milter_message_result_get_headers(priv->message_result);
    milter_headers_add_header(headers, header_name, header_value);
    milter_message_result_set_state(priv->message_result, MILTER_STATE_HEADER);

    milter_protocol_agent_set_macro_context(MILTER_PROTOCOL_AGENT(context),
                                            MILTER_COMMAND_HEADER);
    milter_debug("[%u] [server][stop-on-header][start] [%s]",
                 tag, NULL_SAFE_NAME(name));
    g_signal_emit(context, signals[STOP_ON_HEADER], 0,
                  header_name, header_value, &stop);
    milter_debug("[%u] [server][stop-on-header][end] [%s] stop=<%s>",
                 tag, NULL_SAFE_NAME(name), stop ? "true" : "false");
    if (stop) {
        stop_on_state(context, MILTER_SERVER_CONTEXT_STATE_HEADER);
        return TRUE;
    }

    if (milter_server_context_is_enable_step(context, MILTER_STEP_NO_HEADERS)) {
        milter_debug("[%u] [server][header][skip] [%s]",
                     tag, NULL_SAFE_NAME(name));
        milter_server_context_set_state(context,
                                        MILTER_SERVER_CONTEXT_STATE_HEADER);
        g_signal_emit_by_name(context, "continue");
        return TRUE;
    }

    encoder = milter_agent_get_encoder(MILTER_AGENT(context));
    milter_command_encoder_encode_header(MILTER_COMMAND_ENCODER(encoder),
                                         &packet, &packet_size,
                                         header_name, header_value);

    return write_packet(context, packet, packet_size,
                        MILTER_SERVER_CONTEXT_STATE_HEADER);
}

gboolean
milter_server_context_end_of_header (MilterServerContext *context)
{
    MilterServerContextPrivate *priv;
    const gchar *packet = NULL;
    gsize packet_size;
    MilterEncoder *encoder;
    gboolean stop = FALSE;
    guint tag = 0;
    const gchar *name = NULL;

    if (milter_need_debug_log()) {
        tag = milter_agent_get_tag(MILTER_AGENT(context));
        name = milter_server_context_get_name(context);
    }

    milter_debug("[%u] [server][send][end-of-header] [%s]",
                 tag, NULL_SAFE_NAME(name));

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(context);

    ensure_message_result(priv);
    milter_message_result_set_state(priv->message_result,
                                    MILTER_STATE_END_OF_HEADER);

    milter_protocol_agent_set_macro_context(MILTER_PROTOCOL_AGENT(context),
                                            MILTER_COMMAND_END_OF_HEADER);
    milter_debug("[%u] [server][stop-on-end-of-header][start] [%s]",
                 tag, NULL_SAFE_NAME(name));
    g_signal_emit(context, signals[STOP_ON_END_OF_HEADER], 0, &stop);
    milter_debug("[%u] [server][stop-on-end-of-header][end] [%s] stop=<%s>",
                 tag, NULL_SAFE_NAME(name), stop ? "true" : "false");
    if (stop) {
        stop_on_state(context, MILTER_SERVER_CONTEXT_STATE_END_OF_HEADER);
        return TRUE;
    }

    if (milter_server_context_is_enable_step(context,
                                             MILTER_STEP_NO_END_OF_HEADER)) {
        milter_debug("[%u] [server][end-of-header][skip] [%s]",
                     tag, NULL_SAFE_NAME(name));
        milter_server_context_set_state(
            context, MILTER_SERVER_CONTEXT_STATE_END_OF_HEADER);
        g_signal_emit_by_name(context, "continue");
        return TRUE;
    }

    encoder = milter_agent_get_encoder(MILTER_AGENT(context));
    milter_command_encoder_encode_end_of_header(MILTER_COMMAND_ENCODER(encoder),
                                                &packet, &packet_size);

    return write_packet(context, packet, packet_size,
                        MILTER_SERVER_CONTEXT_STATE_END_OF_HEADER);
}

gboolean
milter_server_context_body (MilterServerContext *context,
                            const gchar         *chunk,
                            gsize                size)
{
    MilterServerContextState state = MILTER_SERVER_CONTEXT_STATE_BODY;
    gboolean stop = FALSE;
    MilterServerContextPrivate *priv;
    guint tag = 0;
    const gchar *name = NULL;

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(context);

    if (milter_need_debug_log()) {
        tag = milter_agent_get_tag(MILTER_AGENT(context));
        name = milter_server_context_get_name(context);
    }

    milter_debug("[%u] [server][send][body] [%s] size=<%" G_GSIZE_FORMAT ">",
                 tag, NULL_SAFE_NAME(name), size);

    ensure_message_result(priv);
    milter_message_result_add_body_size(priv->message_result, size);
    milter_message_result_set_state(priv->message_result, MILTER_STATE_BODY);

    milter_protocol_agent_set_macro_context(MILTER_PROTOCOL_AGENT(context),
                                            MILTER_COMMAND_BODY);
    milter_debug("[%u] [server][stop-on-body][start] [%s]",
                 tag, NULL_SAFE_NAME(name));
    g_signal_emit(context, signals[STOP_ON_BODY], 0, chunk, size, &stop);
    milter_debug("[%u] [server][stop-on-body][end] [%s]: stop=<%s>",
                 tag, NULL_SAFE_NAME(name), stop ? "true" : "false");
    if (stop) {
        stop_on_state(context, state);
        return TRUE;
    }

    if (milter_server_context_is_enable_step(context, MILTER_STEP_NO_BODY)) {
        milter_debug("[%u] [server][body][skip] [%s]",
                     tag, NULL_SAFE_NAME(name));
        milter_server_context_set_state(context, state);
        g_signal_emit_by_name(context, "skip");
        return TRUE;
    }

    g_string_append_len(priv->body, chunk, size);
    return flush_body(context);
}

gboolean
milter_server_context_end_of_message (MilterServerContext *context,
                                      const gchar         *chunk,
                                      gsize                size)
{
    MilterServerContextPrivate *priv;
    const gchar *packet = NULL;
    gsize packet_size;
    MilterEncoder *encoder;
    gboolean stop = FALSE;
    guint tag = 0;
    const gchar *name = NULL;

    if (milter_need_debug_log()) {
        tag = milter_agent_get_tag(MILTER_AGENT(context));
        name = milter_server_context_get_name(context);
    }

    milter_debug("[%u] [server][send][end-of-message] [%s]", tag, name);

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(context);

    ensure_message_result(priv);
    milter_message_result_add_body_size(priv->message_result, size);
    milter_message_result_set_state(priv->message_result,
                                    MILTER_STATE_END_OF_MESSAGE);

    milter_protocol_agent_set_macro_context(MILTER_PROTOCOL_AGENT(context),
                                            MILTER_COMMAND_END_OF_MESSAGE);
    milter_debug("[%u] [server][stop-on-end-of-message][start] %s", tag, name);
    g_signal_emit(context, signals[STOP_ON_END_OF_MESSAGE], 0,
                  chunk, size, &stop);
    milter_debug("[%u] [server][stop-on-end-of-message][end] %s: stop=<%s>",
                 tag, name, stop ? "true" : "false");
    if (stop) {
        stop_on_state(context, MILTER_SERVER_CONTEXT_STATE_END_OF_MESSAGE);
        return TRUE;
    }

    encoder = milter_agent_get_encoder(MILTER_AGENT(context));
    milter_command_encoder_encode_end_of_message(MILTER_COMMAND_ENCODER(encoder),
                                                 &packet, &packet_size,
                                                 chunk, size);

    return write_packet(context, packet, packet_size,
                        MILTER_SERVER_CONTEXT_STATE_END_OF_MESSAGE);
}

gboolean
milter_server_context_quit (MilterServerContext *context)
{
    MilterServerContextPrivate *priv;
    const gchar *packet = NULL;
    gsize packet_size;
    MilterEncoder *encoder;

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(context);
    priv->quitted = TRUE;

    milter_debug("[%u] [server][send][quit] [%s]",
                 milter_agent_get_tag(MILTER_AGENT(context)),
                 milter_server_context_get_name(context));
    encoder = milter_agent_get_encoder(MILTER_AGENT(context));
    milter_command_encoder_encode_quit(MILTER_COMMAND_ENCODER(encoder),
                                       &packet, &packet_size);

    return write_packet(context, packet, packet_size,
                        MILTER_SERVER_CONTEXT_STATE_QUIT);
}


gboolean
milter_server_context_abort (MilterServerContext *context)
{
    MilterServerContextPrivate *priv;
    const gchar *packet = NULL;
    gsize packet_size;
    MilterEncoder *encoder;
    gboolean success;

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(context);

    milter_debug("[%u] [server][send][abort] [%s]",
                 milter_agent_get_tag(MILTER_AGENT(context)),
                 milter_server_context_get_name(context));
    encoder = milter_agent_get_encoder(MILTER_AGENT(context));
    milter_command_encoder_encode_abort(MILTER_COMMAND_ENCODER(encoder),
                                        &packet, &packet_size);

    success = write_packet(context, packet, packet_size,
                           MILTER_SERVER_CONTEXT_STATE_ABORT);
    priv->processing_message = FALSE;
    return success;
}

static gboolean
check_reply_after_quit (MilterServerContext *context,
                        MilterServerContextState state,
                        const gchar *reply_name)
{
    MilterServerContextPrivate *priv;

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(context);
    if (!priv->quitted)
        return TRUE;

    if (milter_need_warning_log()) {
        gchar *state_name;

        state_name = milter_utils_get_enum_nick_name(
            MILTER_TYPE_SERVER_CONTEXT_STATE, state);
        milter_warning("[%u] [server][reply][quitted][%s][%s] [%s]",
                       milter_agent_get_tag(MILTER_AGENT(context)),
                       state_name, reply_name,
                       milter_server_context_get_name(context));
        g_free(state_name);
    }

    return FALSE;
}

static void
invalid_state (MilterServerContext *context,
               MilterServerContextState state,
               const gchar *action,
               const gchar *expected_states)
{
    GError *error = NULL;
    gchar *state_name;
    MilterServerContextPrivate *priv;
    guint tag;
    const gchar *name;

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(context);
    tag = milter_agent_get_tag(MILTER_AGENT(context));
    name = milter_server_context_get_name(context);

    g_timer_stop(priv->elapsed);
    milter_debug("[%u] [server][timer][stop] [%s] <%g>",
                 tag, name, g_timer_elapsed(priv->elapsed, NULL));

    check_reply_after_quit(context, state, action);

    state_name =
        milter_utils_get_enum_nick_name(MILTER_TYPE_SERVER_CONTEXT_STATE,
                                        state);
    g_set_error(&error,
                MILTER_SERVER_CONTEXT_ERROR,
                MILTER_SERVER_CONTEXT_ERROR_INVALID_STATE,
                "<%s> should be caused on <%s> but was <%s>",
                action, expected_states, state_name);
    milter_error("[%u] [server][error][%s][state][invalid][%s] [%s] %s",
                 tag, action, state_name, NULL_SAFE_NAME(name),
                 error->message);
    milter_error_emittable_emit(MILTER_ERROR_EMITTABLE(context),
                                error);
    g_error_free(error);
    g_free(state_name);
}

static void
cb_decoder_negotiate_reply (MilterDecoder *decoder,
                            MilterOption *option,
                            MilterMacrosRequests *macros_requests,
                            gpointer user_data)
{
    MilterServerContext *context;
    MilterServerContextState state;
    MilterServerContextPrivate *priv;
    guint tag = 0;
    const gchar *name = NULL;

    context = MILTER_SERVER_CONTEXT(user_data);
    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(context);
    state = priv->state;

    disable_timeout(context);

    if (milter_need_log(MILTER_LOG_LEVEL_ERROR |
                        MILTER_LOG_LEVEL_DEBUG)) {
        tag = milter_agent_get_tag(MILTER_AGENT(context));
        name = milter_server_context_get_name(context);
    }

    if (milter_need_debug_log()) {
        gchar *inspected_option;

        inspected_option = milter_option_inspect(option);
        milter_debug("[%u] [server][receive][negotiate-reply] [%s] %s",
                     tag, name, inspected_option);
        g_free(inspected_option);
    }

    if (priv->option) {
        if (!milter_option_combine(priv->option, option)) {
            GError *error = NULL;

            g_set_error(&error,
                        MILTER_SERVER_CONTEXT_ERROR,
                        MILTER_SERVER_CONTEXT_ERROR_NEWER_VERSION_REQUESTED,
                        "unsupported newer version is requested: %d < %d",
                        milter_option_get_version(priv->option),
                        milter_option_get_version(option));
            milter_error("[%u] [server][error] [%s] %s",
                         tag, name, error->message);
            milter_error_emittable_emit(MILTER_ERROR_EMITTABLE(context), error);
            g_error_free(error);
            return;
        }
    } else {
        priv->option = milter_option_copy(option);
    }

    if (state == MILTER_SERVER_CONTEXT_STATE_NEGOTIATE) {
        g_timer_stop(priv->elapsed);
        milter_debug("[%u] [server][timer][stop] [%s] <%g>",
                     tag, name, g_timer_elapsed(priv->elapsed, NULL));
        if (check_reply_after_quit(context, state, "negotiate-reply")) {
            MilterProtocolAgent *agent;
            agent = MILTER_PROTOCOL_AGENT(context);
            priv->negotiated = TRUE;
            milter_protocol_agent_set_macros_requests(agent, macros_requests);
            g_signal_emit_by_name(context, "negotiate-reply",
                                  option, macros_requests);
        }
    } else {
        invalid_state(context, state, "negotiate-reply", "negotiate");
    }
}

static void
clear_process_body_count (MilterServerContext *context)
{
    MilterServerContextPrivate *priv;

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(context);
    if (priv->body_response_queue) {
        g_list_free(priv->body_response_queue);
        priv->body_response_queue = NULL;
    }

    if (priv->sent_end_of_message)
        milter_server_context_set_state(
            context, MILTER_SERVER_CONTEXT_STATE_END_OF_MESSAGE);
}

static void
decrement_process_body_count (MilterServerContext *context)
{
    MilterServerContextPrivate *priv;
    GList *first_node;
    guint process_body_count;
    guint tag = 0;
    const gchar *name = NULL;

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(context);

    if (milter_need_log(MILTER_LOG_LEVEL_ERROR |
                        MILTER_LOG_LEVEL_DEBUG)) {
        tag = milter_agent_get_tag(MILTER_AGENT(context));
        name = milter_server_context_get_name(context);
    }

    if (!priv->body_response_queue) {
        milter_error("[%u] [server][error] [%s] "
                     "not wait for response on body anymore",
                     tag, name);
        return;
    }

    first_node = g_list_first(priv->body_response_queue);
    process_body_count = GPOINTER_TO_UINT(first_node->data);
    process_body_count--;

    if (process_body_count == 0) {
        priv->body_response_queue =
            g_list_delete_link(priv->body_response_queue, first_node);
        g_timer_stop(priv->elapsed);
        milter_debug("[%u] [server][timer][stop] [%s] <%g>",
                     tag, name, g_timer_elapsed(priv->elapsed, NULL));
        g_signal_emit_by_name(context, "continue");
        if (priv->sent_end_of_message)
            milter_server_context_set_state(
                context, MILTER_SERVER_CONTEXT_STATE_END_OF_MESSAGE);
    } else {
        first_node->data = GUINT_TO_POINTER(process_body_count);
    }
}

static void
cb_decoder_continue (MilterReplyDecoder *decoder, gpointer user_data)
{
    MilterServerContext *context;
    MilterServerContextPrivate *priv;
    guint tag = 0;
    const gchar *name = NULL;

    context = MILTER_SERVER_CONTEXT(user_data);
    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(context);

    disable_timeout(context);

    if (milter_need_debug_log()) {
        gchar *state_name;

        tag = milter_agent_get_tag(MILTER_AGENT(context));
        name = milter_server_context_get_name(context);

        state_name = milter_utils_get_enum_nick_name(
            MILTER_TYPE_SERVER_CONTEXT_STATE, priv->state);
        milter_debug("[%u] [server][receive][continue][%s] [%s]",
                     tag, state_name, name);
        g_free(state_name);
    }

    switch (priv->state) {
      case MILTER_SERVER_CONTEXT_STATE_BODY:
        if (check_reply_after_quit(context, priv->state, "continue")) {
            decrement_process_body_count(context);
            if (priv->body->len > 0) {
                if (!flush_body(context)) {
                    milter_debug("[%u] [server][receive][continue]"
                                 "[body][flush][error] [%s]",
                                 tag, name);
                }
            }
        }
        break;
      case MILTER_SERVER_CONTEXT_STATE_CONNECT:
      case MILTER_SERVER_CONTEXT_STATE_HELO:
      case MILTER_SERVER_CONTEXT_STATE_ENVELOPE_FROM:
      case MILTER_SERVER_CONTEXT_STATE_ENVELOPE_RECIPIENT:
      case MILTER_SERVER_CONTEXT_STATE_UNKNOWN:
      case MILTER_SERVER_CONTEXT_STATE_DATA:
      case MILTER_SERVER_CONTEXT_STATE_HEADER:
      case MILTER_SERVER_CONTEXT_STATE_END_OF_HEADER:
      case MILTER_SERVER_CONTEXT_STATE_END_OF_MESSAGE:
        g_timer_stop(priv->elapsed);
        milter_debug("[%u] [server][timer][stop] [%s] <%g>",
                     tag, name, g_timer_elapsed(priv->elapsed, NULL));
        if (check_reply_after_quit(context, priv->state, "continue")) {
            g_signal_emit_by_name(context, "continue");
            if (priv->state == MILTER_SERVER_CONTEXT_STATE_END_OF_MESSAGE)
                emit_message_processed_signal(context);
        }
        break;
      default:
          invalid_state(context, priv->state, "continue",
                        "connect, helo, envelope-from, envelope-recipient, "
                        "data, header, end-of-header, body, end-of-message, "
                        "unknown");
        break;
    }
}

static void
cb_decoder_reply_code (MilterReplyDecoder *decoder,
                       guint code,
                       const gchar *extended_code,
                       const gchar *message,
                       gpointer user_data)
{
    MilterServerContext *context = user_data;
    MilterServerContextState state;
    MilterServerContextPrivate *priv;
    MilterStatus status;
    guint tag = 0;
    const gchar *name = NULL;

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(context);

    if (milter_need_debug_log()) {
        tag = milter_agent_get_tag(MILTER_AGENT(context));
        name = milter_server_context_get_name(context);
    }

    if (400 <= code && code < 500) {
        status = MILTER_STATUS_TEMPORARY_FAILURE;
    } else {
        status = MILTER_STATUS_REJECT;
    }

    if (priv->state == MILTER_SERVER_CONTEXT_STATE_ENVELOPE_RECIPIENT) {
        priv->envelope_recipient_status = status;
        ensure_message_result(priv);
        if (status == MILTER_STATUS_TEMPORARY_FAILURE) {
            milter_message_result_add_temporary_failed_recipient(
                priv->message_result, priv->current_recipient);
        } else {
            milter_message_result_add_rejected_recipient(
                priv->message_result, priv->current_recipient);
        }
        milter_message_result_remove_recipient(
            priv->message_result, priv->current_recipient);
        g_free(priv->current_recipient);
        priv->current_recipient = NULL;
    } else {
        priv->status = status;
    }

    disable_timeout(context);

    milter_debug("[%u] [server][receive][reply-code] [%s] <%d %s %s>",
                 tag, name, code, extended_code, message);

    state = priv->state;

    switch (state) {
    case MILTER_SERVER_CONTEXT_STATE_BODY:
        if (check_reply_after_quit(context, state, "reply-code")) {
            clear_process_body_count(context);
        }
    case MILTER_SERVER_CONTEXT_STATE_CONNECT:
    case MILTER_SERVER_CONTEXT_STATE_HELO:
    case MILTER_SERVER_CONTEXT_STATE_ENVELOPE_FROM:
    case MILTER_SERVER_CONTEXT_STATE_ENVELOPE_RECIPIENT:
    case MILTER_SERVER_CONTEXT_STATE_UNKNOWN:
    case MILTER_SERVER_CONTEXT_STATE_DATA:
    case MILTER_SERVER_CONTEXT_STATE_HEADER:
    case MILTER_SERVER_CONTEXT_STATE_END_OF_HEADER:
    case MILTER_SERVER_CONTEXT_STATE_END_OF_MESSAGE:
        g_timer_stop(priv->elapsed);
        milter_debug("[%u] [server][timer][stop] [%s] %g",
                     tag, name, g_timer_elapsed(priv->elapsed, NULL));
        if (check_reply_after_quit(context, state, "reply-code")) {
            g_signal_emit_by_name(context, "reply-code",
                                  code, extended_code, message);
            if (MILTER_SERVER_CONTEXT_STATE_ENVELOPE_FROM <= priv->state &&
                priv->state <= MILTER_SERVER_CONTEXT_STATE_END_OF_MESSAGE &&
                priv->state != MILTER_SERVER_CONTEXT_STATE_ENVELOPE_RECIPIENT) {
                emit_message_processed_signal(context);
            }
        }
        break;
    default:
        invalid_state(context, state, "reply-code",
                      "connect, helo, envelope-from, envelope-recipient, "
                      "data, header, end-of-header, body, end-of-message, "
                      "unknown");
        break;
    }
}

static void
cb_decoder_temporary_failure (MilterReplyDecoder *decoder, gpointer user_data)
{
    MilterServerContext *context = user_data;
    MilterServerContextPrivate *priv;

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(context);

    if (priv->state == MILTER_SERVER_CONTEXT_STATE_ENVELOPE_RECIPIENT) {
        priv->envelope_recipient_status = MILTER_STATUS_TEMPORARY_FAILURE;
        ensure_message_result(priv);
        milter_message_result_add_temporary_failed_recipient(
            priv->message_result, priv->current_recipient);
        milter_message_result_remove_recipient(
            priv->message_result, priv->current_recipient);
        g_free(priv->current_recipient);
        priv->current_recipient = NULL;
    } else {
        priv->status = MILTER_STATUS_TEMPORARY_FAILURE;
    }

    disable_timeout(context);
    g_timer_stop(priv->elapsed);

    if (milter_need_debug_log()) {
        guint tag;
        const gchar *name;
        gchar *state_name;

        tag = milter_agent_get_tag(MILTER_AGENT(context));
        name = milter_server_context_get_name(context);

        state_name =
            milter_utils_get_enum_nick_name(MILTER_TYPE_SERVER_CONTEXT_STATE,
                                            priv->state);
        milter_debug("[%u] [server][receive][temporary-failure][%s] [%s]",
                     tag, state_name, name);
        g_free(state_name);

        milter_debug("[%u] [server][timer][stop] [%s] %g",
                     tag, name, g_timer_elapsed(priv->elapsed, NULL));
    }

    if (!check_reply_after_quit(context, priv->state, "temporary-failure"))
        return;

    g_signal_emit_by_name(context, "temporary-failure");
    if (MILTER_SERVER_CONTEXT_STATE_ENVELOPE_FROM <= priv->state &&
        priv->state <= MILTER_SERVER_CONTEXT_STATE_END_OF_MESSAGE &&
        priv->state != MILTER_SERVER_CONTEXT_STATE_ENVELOPE_RECIPIENT) {
        emit_message_processed_signal(context);
    }

    switch (priv->state) {
    case MILTER_SERVER_CONTEXT_STATE_BODY:
        clear_process_body_count(context);
        break;
    default:
        break;
    }
}

static void
cb_decoder_reject (MilterReplyDecoder *decoder, gpointer user_data)
{
    MilterServerContext *context = user_data;
    MilterServerContextPrivate *priv;

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(context);

    if (priv->state == MILTER_SERVER_CONTEXT_STATE_ENVELOPE_RECIPIENT) {
        priv->envelope_recipient_status = MILTER_STATUS_REJECT;
        ensure_message_result(priv);
        milter_message_result_add_rejected_recipient(priv->message_result,
                                                     priv->current_recipient);
        milter_message_result_remove_recipient(priv->message_result,
                                               priv->current_recipient);
        g_free(priv->current_recipient);
        priv->current_recipient = NULL;
    } else {
        priv->status = MILTER_STATUS_REJECT;
    }

    disable_timeout(context);
    g_timer_stop(priv->elapsed);

    if (milter_need_debug_log()) {
        guint tag;
        const gchar *name;
        gchar *state_name;

        tag = milter_agent_get_tag(MILTER_AGENT(context));
        name = milter_server_context_get_name(context);

        state_name =
            milter_utils_get_enum_nick_name(MILTER_TYPE_SERVER_CONTEXT_STATE,
                                            priv->state);
        milter_debug("[%u] [server][receive][reject][%s] [%s]",
                     tag, state_name, name);
        g_free(state_name);

        milter_debug("[%u] [server][timer][stop] [%s] <%g>",
                     tag, name, g_timer_elapsed(priv->elapsed, NULL));
    }

    if (!check_reply_after_quit(context, priv->state, "reject"))
        return;

    g_signal_emit_by_name(context, "reject");
    if (MILTER_SERVER_CONTEXT_STATE_ENVELOPE_FROM <= priv->state &&
        priv->state <= MILTER_SERVER_CONTEXT_STATE_END_OF_MESSAGE &&
        priv->state != MILTER_SERVER_CONTEXT_STATE_ENVELOPE_RECIPIENT) {
        emit_message_processed_signal(context);
    }

    switch (priv->state) {
    case MILTER_SERVER_CONTEXT_STATE_BODY:
        clear_process_body_count(context);
        break;
    default:
        break;
    }
}

static void
cb_decoder_accept (MilterReplyDecoder *decoder, gpointer user_data)
{
    MilterServerContext *context = user_data;
    MilterServerContextPrivate *priv;

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(context);
    priv->status = MILTER_STATUS_ACCEPT;

    disable_timeout(context);
    g_timer_stop(priv->elapsed);

    if (milter_need_debug_log()) {
        guint tag;
        const gchar *name;
        gchar *state_name;

        tag = milter_agent_get_tag(MILTER_AGENT(context));
        name = milter_server_context_get_name(context);

        state_name =
            milter_utils_get_enum_nick_name(MILTER_TYPE_SERVER_CONTEXT_STATE,
                                            priv->state);
        milter_debug("[%u] [server][receive][accept][%s] [%s]",
                     tag, state_name, name);
        g_free(state_name);

        milter_debug("[%u] [server][timer][stop] [%s] %g",
                     tag, name, g_timer_elapsed(priv->elapsed, NULL));
    }

    if (!check_reply_after_quit(context, priv->state, "accept"))
        return;

    g_signal_emit_by_name(context, "accept");
    if (MILTER_SERVER_CONTEXT_STATE_ENVELOPE_FROM <= priv->state &&
        priv->state <= MILTER_SERVER_CONTEXT_STATE_END_OF_MESSAGE) {
        emit_message_processed_signal(context);
    }

    switch (priv->state) {
    case MILTER_SERVER_CONTEXT_STATE_BODY:
        clear_process_body_count(context);
        break;
    default:
        break;
    }
}

static void
cb_decoder_discard (MilterReplyDecoder *decoder, gpointer user_data)
{
    MilterServerContext *context = user_data;
    MilterServerContextPrivate *priv;

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(user_data);
    priv->status = MILTER_STATUS_DISCARD;

    disable_timeout(context);
    g_timer_stop(priv->elapsed);

    if (milter_need_debug_log()) {
        guint tag;
        const gchar *name;
        gchar *state_name;

        tag = milter_agent_get_tag(MILTER_AGENT(context));
        name = milter_server_context_get_name(context);

        state_name = milter_utils_get_enum_nick_name(
            MILTER_TYPE_SERVER_CONTEXT_STATE, priv->state);
        milter_debug("[%u] [server][receive][discard][%s] [%s]",
                     tag, state_name, name);
        g_free(state_name);

        milter_debug("[%u] [server][timer][stop] [%s] <%g>",
                     tag, name, g_timer_elapsed(priv->elapsed, NULL));
    }

    if (!check_reply_after_quit(context, priv->state, "discard"))
        return;

    g_signal_emit_by_name(context, "discard");
    if (MILTER_SERVER_CONTEXT_STATE_ENVELOPE_FROM <= priv->state &&
        priv->state <= MILTER_SERVER_CONTEXT_STATE_END_OF_MESSAGE) {
        emit_message_processed_signal(context);
    }

    switch (priv->state) {
    case MILTER_SERVER_CONTEXT_STATE_BODY:
        clear_process_body_count(context);
        break;
    default:
        break;
    }
}

static void
cb_decoder_add_header (MilterReplyDecoder *decoder,
                       const gchar *name, const gchar *value,
                       gpointer user_data)
{
    MilterServerContext *context = user_data;
    MilterServerContextPrivate *priv;

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(context);

    milter_debug("[%u] [server][receive][add-header] [%s] <%s>=<%s>",
                 milter_agent_get_tag(MILTER_AGENT(context)),
                 milter_server_context_get_name(context),
                 MILTER_LOG_NULL_SAFE_STRING(name),
                 MILTER_LOG_NULL_SAFE_STRING(value));

    if (priv->state == MILTER_SERVER_CONTEXT_STATE_END_OF_MESSAGE) {
        MilterHeaders *headers;

        if (!check_reply_after_quit(context, priv->state, "add-header"))
            return;

        g_signal_emit_by_name(context, "add-header", name, value);

        ensure_message_result(priv);
        headers = milter_message_result_get_added_headers(priv->message_result);
        milter_headers_add_header(headers, name, value);
    } else {
        invalid_state(context, priv->state, "add-header", "end-of-message");
    }
}

static void
cb_decoder_insert_header (MilterReplyDecoder *decoder,
                          guint32 index,
                          const gchar *name,
                          const gchar *value,
                          gpointer user_data)
{
    MilterServerContext *context = user_data;
    MilterServerContextPrivate *priv;

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(context);

    milter_debug("[%u] [server][receive][insert-header] [%s] [%u]<%s>=<%s>",
                 milter_agent_get_tag(MILTER_AGENT(context)),
                 milter_server_context_get_name(context),
                 index,
                 MILTER_LOG_NULL_SAFE_STRING(name),
                 MILTER_LOG_NULL_SAFE_STRING(value));

    if (priv->state == MILTER_SERVER_CONTEXT_STATE_END_OF_MESSAGE) {
        MilterHeaders *headers;

        if (!check_reply_after_quit(context, priv->state, "insert-header"))
            return;

        g_signal_emit_by_name(context, "insert-header", index, name, value);

        ensure_message_result(priv);
        headers = milter_message_result_get_added_headers(priv->message_result);
        milter_headers_add_header(headers, name, value);
    } else {
        invalid_state(context, priv->state, "insert-header", "end-of-message");
    }
}

static void
cb_decoder_change_header (MilterReplyDecoder *decoder,
                          const gchar *name,
                          gint index,
                          const gchar *value,
                          gpointer user_data)
{
    MilterServerContext *context = user_data;
    MilterServerContextPrivate *priv;

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(context);

    milter_debug("[%u] [server][receive][change-header] [%s] <%s>[%u]=<%s>",
                 milter_agent_get_tag(MILTER_AGENT(context)),
                 milter_server_context_get_name(context),
                 MILTER_LOG_NULL_SAFE_STRING(name),
                 index,
                 MILTER_LOG_NULL_SAFE_STRING(value));

    if (priv->state == MILTER_SERVER_CONTEXT_STATE_END_OF_MESSAGE) {
        MilterHeaders *headers;

        if (!check_reply_after_quit(context, priv->state, "change-header"))
            return;

        g_signal_emit_by_name(context, "change-header", name, index, value);

        ensure_message_result(priv);
        headers = milter_message_result_get_added_headers(priv->message_result);
        milter_headers_add_header(headers, name, value);
    } else {
        invalid_state(context, priv->state, "change-header", "end-of-message");
    }
}

static void
cb_decoder_delete_header (MilterReplyDecoder *decoder,
                          const gchar *name,
                          gint index,
                          gpointer user_data)
{
    MilterServerContext *context = user_data;
    MilterServerContextPrivate *priv;

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(context);

    milter_debug("[%u] [server][receive][delete-header] [%s] <%s>[%u]",
                 milter_agent_get_tag(MILTER_AGENT(context)),
                 milter_server_context_get_name(context),
                 MILTER_LOG_NULL_SAFE_STRING(name),
                 index);

    if (priv->state == MILTER_SERVER_CONTEXT_STATE_END_OF_MESSAGE) {
        MilterHeaders *headers;

        if (!check_reply_after_quit(context, priv->state, "delete-header"))
            return;

        g_signal_emit_by_name(context, "delete-header", name, index);

        ensure_message_result(priv);
        headers = milter_message_result_get_removed_headers(priv->message_result);
        milter_headers_add_header(headers, name, NULL);
    } else {
        invalid_state(context, priv->state, "delete-header", "end-of-message");
    }
}

static void
cb_decoder_change_from (MilterReplyDecoder *decoder,
                        const gchar *from,
                        const gchar *parameters,
                        gpointer user_data)
{
    MilterServerContext *context = user_data;
    MilterServerContextPrivate *priv;

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(context);

    milter_debug("[%u] [server][receive][change-from] [%s] <%s>:<%s>",
                 milter_agent_get_tag(MILTER_AGENT(context)),
                 milter_server_context_get_name(context),
                 MILTER_LOG_NULL_SAFE_STRING(from),
                 MILTER_LOG_NULL_SAFE_STRING(parameters));

    if (priv->state == MILTER_SERVER_CONTEXT_STATE_END_OF_MESSAGE) {
        if (!check_reply_after_quit(context, priv->state, "change-from"))
            return;
        g_signal_emit_by_name(context, "change-from", from, parameters);
    } else {
        invalid_state(context, priv->state, "change-from", "end-of-message");
    }
}

static void
cb_decoder_add_recipient (MilterReplyDecoder *decoder,
                          const gchar *recipient,
                          const gchar *parameters,
                          gpointer user_data)
{
    MilterServerContext *context = user_data;
    MilterServerContextPrivate *priv;

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(context);

    milter_debug("[%u] [server][receive][add-recipient] [%s] <%s>:<%s>",
                 milter_agent_get_tag(MILTER_AGENT(context)),
                 milter_server_context_get_name(context),
                 MILTER_LOG_NULL_SAFE_STRING(recipient),
                 MILTER_LOG_NULL_SAFE_STRING(parameters));

    if (priv->state == MILTER_SERVER_CONTEXT_STATE_END_OF_MESSAGE) {
        if (!check_reply_after_quit(context, priv->state, "add-recipient"))
            return;
        g_signal_emit_by_name(context, "add-recipient", recipient, parameters);
    } else {
        invalid_state(context, priv->state, "add-recipient", "end-of-message");
    }
}

static void
cb_decoder_delete_recipient (MilterReplyDecoder *decoder,
                             const gchar *recipient,
                             gpointer user_data)
{
    MilterServerContext *context = user_data;
    MilterServerContextPrivate *priv;

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(user_data);

    milter_debug("[%u] [server][receive][delete-recipient] [%s] <%s>",
                 milter_agent_get_tag(MILTER_AGENT(context)),
                 milter_server_context_get_name(context),
                 MILTER_LOG_NULL_SAFE_STRING(recipient));

    if (priv->state == MILTER_SERVER_CONTEXT_STATE_END_OF_MESSAGE) {
        if (!check_reply_after_quit(context, priv->state, "delete-recipient"))
            return;
        g_signal_emit_by_name(user_data, "delete-recipient", recipient);
    } else {
        invalid_state(context, priv->state,
                      "delete-recipient", "end-of-message");
    }
}

static void
cb_decoder_replace_body (MilterReplyDecoder *decoder,
                         const gchar *body,
                         gsize body_size,
                         gpointer user_data)
{
    MilterServerContext *context = user_data;
    MilterServerContextPrivate *priv;

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(context);

    milter_debug("[%u] [server][receive][replace-body] [%s] "
                 "body_size=%" G_GSIZE_FORMAT,
                 milter_agent_get_tag(MILTER_AGENT(context)),
                 milter_server_context_get_name(context),
                 body_size);

    if (priv->state == MILTER_SERVER_CONTEXT_STATE_END_OF_MESSAGE) {
        if (!check_reply_after_quit(context, priv->state, "replace-body"))
            return;
        g_signal_emit_by_name(context, "replace-body", body, body_size);
    } else {
        invalid_state(context, priv->state, "replace-body", "end-of-message");
    }
}

static void
cb_decoder_progress (MilterReplyDecoder *decoder, gpointer user_data)
{
    MilterServerContext *context = user_data;
    MilterServerContextPrivate *priv;

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(user_data);

    milter_debug("[%u] [server][receive][progress] [%s]",
                 milter_agent_get_tag(MILTER_AGENT(context)),
                 milter_server_context_get_name(context));

    if (priv->state == MILTER_SERVER_CONTEXT_STATE_END_OF_MESSAGE) {
        if (!check_reply_after_quit(context, priv->state, "progress"))
            return;
        reset_end_of_message_timeout(context);
        g_signal_emit_by_name(context, "progress");
    } else {
        invalid_state(context, priv->state, "progress", "end-of-message");
    }
}

static void
cb_decoder_quarantine (MilterReplyDecoder *decoder,
                       const gchar *reason,
                       gpointer user_data)
{
    MilterServerContext *context = user_data;
    MilterServerContextPrivate *priv;

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(user_data);

    priv->status = MILTER_STATUS_QUARANTINE;
    milter_debug("[%u] [server][receive][quarantine] [%s] <%s>",
                 milter_agent_get_tag(MILTER_AGENT(context)),
                 milter_server_context_get_name(context),
                 reason);

    if (priv->state == MILTER_SERVER_CONTEXT_STATE_END_OF_MESSAGE) {
        if (!check_reply_after_quit(context, priv->state, "quarantine"))
            return;

        g_signal_emit_by_name(user_data, "quarantine", reason);

        ensure_message_result(priv);
        milter_message_result_set_quarantine(priv->message_result, TRUE);
    } else {
        invalid_state(context, priv->state, "quarantine", "end-of-message");
    }
}

static void
cb_decoder_connection_failure (MilterReplyDecoder *decoder, gpointer user_data)
{
    MilterServerContext *context = user_data;
    MilterServerContextPrivate *priv;

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(user_data);

    disable_timeout(context);
    g_timer_stop(priv->elapsed);

    priv->status = MILTER_STATUS_ERROR;

    if (milter_need_debug_log()) {
        guint tag;
        const gchar *name;

        tag = milter_agent_get_tag(MILTER_AGENT(context));
        name = milter_server_context_get_name(context);

        milter_debug("[%u] [server][receive][connection-failure] [%s]",
                     tag, name);
        milter_debug("[%u] [server][timer][stop] [%s] <%g>",
                     tag, name, g_timer_elapsed(priv->elapsed, NULL));
    }

    if (!check_reply_after_quit(context, priv->state, "connection-failure"))
        return;
    g_signal_emit_by_name(user_data, "connection-failure");
}

static void
cb_decoder_shutdown (MilterReplyDecoder *decoder, gpointer user_data)
{
    MilterServerContext *context = user_data;
    MilterServerContextPrivate *priv;

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(user_data);

    disable_timeout(context);
    g_timer_stop(priv->elapsed);

    if (milter_need_debug_log()) {
        guint tag;
        const gchar *name;

        tag = milter_agent_get_tag(MILTER_AGENT(context));
        name = milter_server_context_get_name(context);

        milter_debug("[%u] [server][receive][shutdown] [%s]", tag, name);
        milter_debug("[%u] [server][timer][stop] [%s] <%g>",
                     tag, name, g_timer_elapsed(priv->elapsed, NULL));
    }

    if (!check_reply_after_quit(context, priv->state, "shutdown"))
        return;
    g_signal_emit_by_name(user_data, "shutdown");
}

static void
cb_decoder_skip (MilterReplyDecoder *decoder, gpointer user_data)
{
    MilterServerContext *context = user_data;
    MilterServerContextPrivate *priv;
    guint tag = 0;
    const gchar *name = NULL;

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(context);

    if (milter_need_debug_log()) {
        tag = milter_agent_get_tag(MILTER_AGENT(context));
        name = milter_server_context_get_name(context);
    }

    disable_timeout(context);

    milter_debug("[%u] [server][receive][skip] [%s]", tag, name);

    if (priv->state == MILTER_SERVER_CONTEXT_STATE_BODY) {
        g_timer_stop(priv->elapsed);
        milter_debug("[%u] [server][timer][stop] [%s] <%g>",
                     tag, name, g_timer_elapsed(priv->elapsed, NULL));
        if (!check_reply_after_quit(context, priv->state, "skip"))
            return;
        g_signal_emit_by_name(context, "skip");
        clear_process_body_count(context);
        priv->skip_body = TRUE;
    } else {
        invalid_state(context, priv->state, "skip", "body");
    }
}

static MilterDecoder *
decoder_new (MilterAgent *agent)
{
    MilterDecoder *decoder;

    decoder = milter_reply_decoder_new();

#define CONNECT(name)                                                   \
    g_signal_connect(decoder, #name, G_CALLBACK(cb_decoder_ ## name),   \
                     agent)

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

#undef CONNECT

    return decoder;
}

static MilterEncoder *
encoder_new (MilterAgent *agent)
{
    return milter_command_encoder_new();
}

gboolean
milter_server_context_set_connection_spec (MilterServerContext *context,
                                           const gchar *spec,
                                           GError **error)
{
    MilterServerContextPrivate *priv;
    gboolean success;

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(context);

    if (priv->address) {
        g_free(priv->address);
        priv->address = NULL;
    }

    if (priv->spec) {
        g_free(priv->spec);
    }
    priv->spec = g_strdup(spec);

    success = milter_connection_parse_spec(spec,
                                           &(priv->domain),
                                           &(priv->address),
                                           &(priv->address_size),
                                           error);
    return success;
}

static gboolean
cb_connection_timeout (gpointer data)
{
    MilterServerContext *context = data;
    MilterServerContextPrivate *priv;
    MilterAgent *agent;
    const gchar *name = NULL;

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(context);
    if (priv)
        name = milter_server_context_get_name(context);
    agent = MILTER_AGENT(context);
    milter_debug("[%u] [server][timeout][connection] [%s] [%u] (%p)",
                 priv ? milter_agent_get_tag(agent) : 0,
                 NULL_SAFE_NAME(name),
                 priv ? priv->timeout_id : 0,
                 context);
    milter_error("[%u] [server][timeout][connection] [%s]",
                 priv ? milter_agent_get_tag(agent) : 0,
                 NULL_SAFE_NAME(name));
    g_signal_emit(context, signals[CONNECTION_TIMEOUT], 0);

    return FALSE;
}

static gboolean
prepare_reader (MilterServerContext *context)
{
    MilterReader *reader;
    MilterServerContextPrivate *priv;

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(context);

    reader = milter_reader_io_channel_new(priv->client_channel);
    milter_agent_set_reader(MILTER_AGENT(context), reader);
    g_object_unref(reader);

    return TRUE;
}

static gboolean
prepare_writer (MilterServerContext *context)
{
    MilterWriter *writer;
    MilterServerContextPrivate *priv;

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(context);

    writer = milter_writer_io_channel_new(priv->client_channel);
    milter_agent_set_writer(MILTER_AGENT(context), writer);
    g_object_unref(writer);

    return TRUE;
}

static gboolean
connect_watch_func (GIOChannel *channel, GIOCondition condition, gpointer data)
{
    MilterServerContext *context = data;
    MilterServerContextPrivate *priv;
    gint socket_errno = 0;
    socklen_t option_length;

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(context);

    disable_timeout(context);
    dispose_connect_watch(context);

    option_length = sizeof(socket_errno);
    if (getsockopt(g_io_channel_unix_get_fd(priv->client_channel),
                   SOL_SOCKET, SO_ERROR,
                   &socket_errno, &option_length) == -1) {
        socket_errno = errno;
    }

    if (socket_errno) {
        GError *error = NULL;

        error = g_error_new(MILTER_SERVER_CONTEXT_ERROR,
                            MILTER_SERVER_CONTEXT_ERROR_CONNECTION_FAILURE,
                            "Failed to connect to %s: %s",
                            priv->spec, g_strerror(socket_errno));
        milter_error("[server][error][connect] [%s] %s",
                     milter_server_context_get_name(context),
                     error->message);
        milter_error_emittable_emit(MILTER_ERROR_EMITTABLE(context), error);
        g_error_free(error);

        dispose_client_channel(priv);
    } else {
        if (prepare_reader(context) && prepare_writer(context)) {
            GError *error = NULL;
            milter_agent_start(MILTER_AGENT(context), &error);
            if (error) {
                milter_error("[server][error][connected][start] [%s] %s",
                             milter_server_context_get_name(context),
                             error->message);
                milter_error_emittable_emit(MILTER_ERROR_EMITTABLE(context),
                                            error);
                g_error_free(error);

                dispose_client_channel(priv);
            } else {
                g_signal_emit(context, signals[READY], 0);
            }
        } else {
            dispose_client_channel(priv);
        }
    }

    return FALSE;
}

gboolean
milter_server_context_establish_connection (MilterServerContext *context,
                                            GError **error)
{
    MilterAgent *agent;
    MilterServerContextPrivate *priv;
    MilterEventLoop *loop;
    GError *io_error = NULL;
    gint client_fd;

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(context);

    if (!priv->address) {
        g_set_error(error,
                    MILTER_SERVER_CONTEXT_ERROR,
                    MILTER_SERVER_CONTEXT_ERROR_NO_SPEC,
                    "No spec set yet.");
        return FALSE;
    }

    client_fd = socket(priv->domain, SOCK_STREAM, 0);
    if (client_fd == -1) {
        g_set_error(error,
                    MILTER_SERVER_CONTEXT_ERROR,
                    MILTER_SERVER_CONTEXT_ERROR_CONNECTION_FAILURE,
                    "Failed to socket(): %s", g_strerror(errno));
        return FALSE;
    }

    priv->client_channel = g_io_channel_unix_new(client_fd);
    g_io_channel_set_close_on_unref(priv->client_channel, TRUE);

    g_io_channel_set_encoding(priv->client_channel, NULL, &io_error);
    if (io_error) {
        dispose_client_channel(priv);
        milter_utils_set_error_with_sub_error(
            error,
            MILTER_SERVER_CONTEXT_ERROR,
            MILTER_SERVER_CONTEXT_ERROR_CONNECTION_FAILURE,
            io_error,
            "Failed to set encoding for preparing connect()");
        return FALSE;
    }

    g_io_channel_set_flags(priv->client_channel,
                           G_IO_FLAG_NONBLOCK |
                           G_IO_FLAG_IS_READABLE |
                           G_IO_FLAG_IS_WRITEABLE,
                           &io_error);
    if (io_error) {
        dispose_client_channel(priv);
        milter_utils_set_error_with_sub_error(
            error,
            MILTER_SERVER_CONTEXT_ERROR,
            MILTER_SERVER_CONTEXT_ERROR_CONNECTION_FAILURE,
            io_error,
            "Failed to set flags for preparing connect()");
        return FALSE;
    }

    agent = MILTER_AGENT(context);
    loop = milter_agent_get_event_loop(agent);
    priv->connect_watch_id =
        milter_event_loop_watch_io(loop,
                                   priv->client_channel,
                                   G_IO_OUT |
                                   G_IO_ERR | G_IO_HUP | G_IO_NVAL,
                                   connect_watch_func, context);

    disable_timeout(context);
    priv->timeout_id = milter_event_loop_add_timeout(loop,
                                                     priv->connection_timeout,
                                                     cb_connection_timeout,
                                                     context);
    if (milter_need_debug_log()) {
        const gchar *name = NULL;

        if (priv)
            name = milter_server_context_get_name(context);
        milter_debug("[%u] [server][timeout][connection][registered][%g] "
                     "[%s] <%u> (%p)",
                     milter_agent_get_tag(agent),
                     priv->connection_timeout,
                     NULL_SAFE_NAME(name),
                     priv->timeout_id,
                     context);
    }

    if (connect(client_fd, priv->address, priv->address_size) == -1) {
        if (errno == EINPROGRESS)
            return TRUE;

        disable_timeout(context);
        dispose_connect_watch(context);
        dispose_client_channel(priv);
        g_set_error(error,
                    MILTER_SERVER_CONTEXT_ERROR,
                    MILTER_SERVER_CONTEXT_ERROR_CONNECTION_FAILURE,
                    "Failed to connect to %s: %s",
                    priv->spec, g_strerror(errno));

        return FALSE;
    }

    if (milter_need_debug_log()) {
        gchar *spec;

        spec = milter_connection_address_to_spec(priv->address);
        milter_debug("[%u] [server][established] [%s] %d:%s",
                     milter_agent_get_tag(MILTER_AGENT(context)),
                     milter_server_context_get_name(context),
                     client_fd,
                     spec);
        g_free(spec);
    }

    return TRUE;
}

void
milter_server_context_set_connection_timeout (MilterServerContext *context,
                                              gdouble timeout)
{
    MILTER_SERVER_CONTEXT_GET_PRIVATE(context)->connection_timeout = timeout;
}

void
milter_server_context_set_writing_timeout (MilterServerContext *context,
                                           gdouble timeout)
{
    MILTER_SERVER_CONTEXT_GET_PRIVATE(context)->writing_timeout = timeout;
}

void
milter_server_context_set_reading_timeout (MilterServerContext *context,
                                           gdouble timeout)
{
    MILTER_SERVER_CONTEXT_GET_PRIVATE(context)->reading_timeout = timeout;
}

void
milter_server_context_set_end_of_message_timeout (MilterServerContext *context,
                                                  gdouble timeout)
{
    MILTER_SERVER_CONTEXT_GET_PRIVATE(context)->end_of_message_timeout = timeout;
}

gboolean
milter_server_context_get_skip_body (MilterServerContext *context)
{
    return MILTER_SERVER_CONTEXT_GET_PRIVATE(context)->skip_body;
}

const gchar *
milter_server_context_get_name (MilterServerContext *context)
{
    return MILTER_SERVER_CONTEXT_GET_PRIVATE(context)->name;
}

void
milter_server_context_set_name (MilterServerContext *context, const gchar *name)
{
    MilterServerContextPrivate *priv;

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(context);
    if (priv->name)
        g_free(priv->name);
    priv->name = g_strdup(name);
}

static gboolean
stop_on_accumulator (GSignalInvocationHint *hint,
                     GValue *return_accumulator,
                     const GValue *context_return,
                     gpointer data)
{
    gboolean stop;

    stop = g_value_get_boolean(context_return);
    g_value_set_boolean(return_accumulator, stop);

    return !stop;
}

gdouble
milter_server_context_get_elapsed (MilterServerContext *context)
{
    return g_timer_elapsed(MILTER_SERVER_CONTEXT_GET_PRIVATE(context)->elapsed,
                           NULL);
}

gboolean
milter_server_context_is_negotiated (MilterServerContext *context)
{
    return MILTER_SERVER_CONTEXT_GET_PRIVATE(context)->negotiated;
}

gboolean
milter_server_context_is_processing_message (MilterServerContext *context)
{
    MilterServerContextPrivate *priv;

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(context);
    if (!priv->negotiated)
        return FALSE;
    if (priv->quitted)
        return FALSE;
    return priv->processing_message;
}

void
milter_server_context_set_processing_message (MilterServerContext *context,
                                              gboolean processing_message)
{
    MILTER_SERVER_CONTEXT_GET_PRIVATE(context)->processing_message = processing_message;
}

gboolean
milter_server_context_is_quitted (MilterServerContext *context)
{
    MilterServerContextPrivate *priv;

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(context);
    if (!priv->negotiated)
        return TRUE;
    return priv->quitted;
}

void
milter_server_context_set_quitted (MilterServerContext *context,
                                   gboolean quitted)
{
    MILTER_SERVER_CONTEXT_GET_PRIVATE(context)->quitted = quitted;
}

MilterMessageResult *
milter_server_context_get_message_result (MilterServerContext *context)
{
    return MILTER_SERVER_CONTEXT_GET_PRIVATE(context)->message_result;
}

void
milter_server_context_set_message_result (MilterServerContext *context,
                                          MilterMessageResult *result)
{
    MilterServerContextPrivate *priv;

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(context);
    if (priv->message_result == result)
        return;

    if (priv->message_result)
        g_object_unref(priv->message_result);
    priv->message_result = result;
    if (priv->message_result)
        g_object_ref(priv->message_result);
}

gboolean
milter_server_context_has_accepted_recipient (MilterServerContext *context)
{
    MilterServerContextPrivate *priv;
    GList *rejected_recipients, *temporary_failed_recipients;

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(context);

    if (!priv->message_result)
        return TRUE;

    rejected_recipients =
        milter_message_result_get_rejected_recipients(priv->message_result);
    temporary_failed_recipients =
        milter_message_result_get_temporary_failed_recipients(priv->message_result);
    if (rejected_recipients == NULL && temporary_failed_recipients == NULL) {
        return TRUE;
    } else {
        GList *recipients;
        recipients = milter_message_result_get_recipients(priv->message_result);
        return recipients != NULL;
    }
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
