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

enum
{
    STOP_ON_CONNECT,
    STOP_ON_HELO,
    STOP_ON_ENVELOPE_FROM,
    STOP_ON_ENVELOPE_RECIPIENT,
    STOP_ON_HEADER,
    STOP_ON_BODY,
    STOP_ON_END_OF_MESSAGE,

    READY,
    STOPPED,

    CONNECTION_TIMEOUT,
    WRITING_TIMEOUT,
    READING_TIMEOUT,
    END_OF_MESSAGE_TIMEOUT,

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
    gint client_fd;

    gchar *spec;
    gint domain;
    struct sockaddr *address;
    socklen_t address_size;
    MilterServerContextState state;
    gchar *reply_code;
    MilterOption *option;

    gdouble connection_timeout;
    gdouble writing_timeout;
    gdouble reading_timeout;
    gdouble end_of_message_timeout;
    guint timeout_id;
    guint connect_watch_id;

    gboolean skip_body;

    gchar *name;
    GList *body_response_queue;
    guint process_body_count;
    gboolean sent_end_of_message;
};

enum
{
    PROP_0,
    PROP_NAME,
    PROP_CONNECTION_TIMEOUT,
    PROP_WRITING_TIMEOUT,
    PROP_READING_TIMEOUT,
    PROP_END_OF_MESSAGE_TIMEOUT
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

static MilterDecoder *decoder_new    (MilterAgent *agent);
static MilterEncoder *encoder_new    (MilterAgent *agent);

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

    signals[STOP_ON_HEADER] =
        g_signal_new("stop-on-header",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterServerContextClass, stop_on_header),
                     stop_on_accumulator, NULL,
                     _milter_marshal_BOOLEAN__STRING_STRING,
                     G_TYPE_BOOLEAN, 2, G_TYPE_STRING, G_TYPE_STRING);

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

    g_type_class_add_private(gobject_class, sizeof(MilterServerContextPrivate));
}

static void
milter_server_context_init (MilterServerContext *context)
{
    MilterServerContextPrivate *priv;

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(context);

    priv->client_fd = -1;
    priv->spec = NULL;
    priv->domain = PF_UNSPEC;
    priv->address = NULL;
    priv->address_size = 0;

    priv->state = MILTER_SERVER_CONTEXT_STATE_START;

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
}

static void
close_client_fd (MilterServerContextPrivate *priv)
{
    if (priv->client_fd > 0) {
        close(priv->client_fd);
        priv->client_fd = -1;
    }
}

static void
disable_timeout (MilterServerContextPrivate *priv)
{
    if (priv->timeout_id > 0) {
        g_source_remove(priv->timeout_id);
        priv->timeout_id = 0;
    }
}

static void
dispose (GObject *object)
{
    MilterServerContextPrivate *priv;

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(object);

    disable_timeout(priv);

    if (priv->connect_watch_id > 0) {
        g_source_remove(priv->connect_watch_id);
        priv->connect_watch_id = 0;
    }

    if (priv->spec) {
        g_free(priv->spec);
        priv->spec = NULL;
    }

    if (priv->address) {
        g_free(priv->address);
        priv->address = NULL;
    }

    if (priv->option) {
        g_object_unref(priv->option);
        priv->option = NULL;
    }

    if (priv->name) {
        g_free(priv->name);
        priv->name = NULL;
    }

    if (priv->body_response_queue) {
        g_list_free(priv->body_response_queue);
        priv->body_response_queue = NULL;
    }
    G_OBJECT_CLASS(milter_server_context_parent_class)->dispose(object);

    close_client_fd(priv);
}

static void
set_property (GObject      *object,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
    MilterServerContextPrivate *priv;

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(object);
    switch (prop_id) {
      case PROP_NAME:
        milter_server_context_set_name(MILTER_SERVER_CONTEXT(object),
                                       g_value_get_string(value));
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

MilterServerContextState
milter_server_context_get_state (MilterServerContext *context)
{
    return MILTER_SERVER_CONTEXT_GET_PRIVATE(context)->state;
}

void
milter_server_context_set_state (MilterServerContext *context,
                                 MilterServerContextState state)
{
    MILTER_SERVER_CONTEXT_GET_PRIVATE(context)->state = state;
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
    g_signal_emit(data, signals[WRITING_TIMEOUT], 0);
    milter_agent_shutdown(MILTER_AGENT(data));

    return FALSE;
}

static gboolean
cb_end_of_message_timeout (gpointer data)
{
    g_signal_emit(data, signals[END_OF_MESSAGE_TIMEOUT], 0);
    milter_agent_shutdown(MILTER_AGENT(data));

    return FALSE;
}

static gboolean
cb_reading_timeout (gpointer data)
{
    g_signal_emit(data, signals[READING_TIMEOUT], 0);
    milter_agent_shutdown(MILTER_AGENT(data));

    return FALSE;
}

static gboolean
is_processing_previous_command (MilterServerContext *context)
{
    MilterServerContextPrivate *priv;
    gchar *inspected_state;
    GError *error = NULL;

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(context);

    if (priv->timeout_id == 0)
        return FALSE;

    inspected_state =
        milter_utils_get_enum_name(MILTER_TYPE_SERVER_CONTEXT_STATE,
                                   priv->state);
    g_set_error(&error,
                MILTER_SERVER_CONTEXT_ERROR,
                MILTER_SERVER_CONTEXT_ERROR_BUSY,
                "Previous command(%s) has been processing in milter",
                inspected_state);
    milter_error("[server][error] %s", error->message);
    milter_error_emittable_emit(MILTER_ERROR_EMITTABLE(context),
                                error);
    g_error_free(error);
    g_free(inspected_state);

    return TRUE;
}

static gboolean
write_packet (MilterServerContext *context, gchar *packet, gsize packet_size,
              MilterServerContextState next_state)
{
    GError *agent_error = NULL;
    MilterServerContextPrivate *priv;

    if (!packet)
        return FALSE;

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(context);

    switch (next_state) {
      case MILTER_SERVER_CONTEXT_STATE_ABORT:
      case MILTER_SERVER_CONTEXT_STATE_QUIT:
        break;
      default:
        if (is_processing_previous_command(context))
            return FALSE;
        priv->timeout_id = milter_utils_timeout_add(priv->writing_timeout,
                                                    cb_writing_timeout,
                                                    context);
        break;
    }

    milter_agent_write_packet(MILTER_AGENT(context),
                              packet, packet_size,
                              &agent_error);
    disable_timeout(priv);
    g_free(packet);

    if (agent_error) {
        GError *error = NULL;
        milter_utils_set_error_with_sub_error(
            &error,
            MILTER_SERVER_CONTEXT_ERROR,
            MILTER_SERVER_CONTEXT_ERROR_IO_ERROR,
            agent_error,
            "Failed to write to milter");
        milter_error("[server][error][write] %s", error->message);
        milter_error_emittable_emit(MILTER_ERROR_EMITTABLE(context),
                                    error);
        g_error_free(error);

        return FALSE;
    }

    switch (next_state) {
      case MILTER_SERVER_CONTEXT_STATE_END_OF_MESSAGE:
        if (priv->process_body_count == 0)
            priv->state = next_state;
        else
            priv->sent_end_of_message = TRUE;
        priv->timeout_id = milter_utils_timeout_add(priv->end_of_message_timeout,
                                                    cb_end_of_message_timeout,
                                                    context);
        break;
      case MILTER_SERVER_CONTEXT_STATE_BODY:
      case MILTER_SERVER_CONTEXT_STATE_DEFINE_MACRO:
        priv->state = next_state;
        disable_timeout(priv);
        break;
      case MILTER_SERVER_CONTEXT_STATE_QUIT:
        milter_agent_shutdown(MILTER_AGENT(context));
      case MILTER_SERVER_CONTEXT_STATE_ABORT:
          /* FIXME: should shutdown on ABORT? */
        priv->state = next_state;
        break;
      default:
        priv->state = next_state;
        priv->timeout_id = milter_utils_timeout_add(priv->reading_timeout,
                                                    cb_reading_timeout,
                                                    context);
        break;
    }

    return TRUE;
}

static void
stop_on_state (MilterServerContext *context, MilterServerContextState state)
{
    gchar *inspected_state;
    MilterServerContextPrivate *priv;

    inspected_state = milter_utils_get_enum_nick_name(MILTER_TYPE_SERVER_CONTEXT_STATE,
                                                      state);
    milter_debug("[client][stop][%s] %s",
                 inspected_state,
                 milter_server_context_get_name(context));
    milter_statistics("[stop][%s]: %s",
                      inspected_state,
                      milter_server_context_get_name(context));
    g_free(inspected_state);
    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(context);
    priv->state = state;
    g_signal_emit_by_name(context, "stopped");
}

gboolean
milter_server_context_negotiate (MilterServerContext *context,
                                 MilterOption        *option)
{
    gchar *packet = NULL;
    gsize packet_size;
    MilterEncoder *encoder;
    MilterServerContextPrivate *priv;

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(context);

    {
        gchar *inspected_option;

        inspected_option = milter_option_inspect(option);
        milter_debug("[server][send][negotiate] %s: %s",
                     inspected_option,
                     milter_server_context_get_name(context));
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
    gchar *packet = NULL;
    gsize packet_size;
    MilterEncoder *encoder;
    gboolean stop = FALSE;

    milter_debug("[server][send][helo] <%s>: %s",
                 fqdn,
                 milter_server_context_get_name(context));

    g_signal_emit(context, signals[STOP_ON_HELO], 0, fqdn, &stop);
    if (stop) {
        stop_on_state(context, MILTER_SERVER_CONTEXT_STATE_HELO);
        return TRUE;
    }

    encoder = milter_agent_get_encoder(MILTER_AGENT(context));
    milter_command_encoder_encode_helo(MILTER_COMMAND_ENCODER(encoder),
                                       &packet, &packet_size, fqdn);

    return write_packet(context, packet, packet_size,
                        MILTER_SERVER_CONTEXT_STATE_HELO);
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

static gboolean
write_macro (MilterServerContext *context,
             MilterCommand command)
{
    GHashTable *macros, *filtered_macros = NULL, *target_macros;
    GList *request_symbols = NULL;
    gchar *command_name;
    gchar *inspected_macros;
    gchar *packet = NULL;
    gsize packet_size;
    MilterEncoder *encoder;
    MilterAgent *agent;
    MilterProtocolAgent *protocol_agent;
    MilterMacrosRequests *macros_requests;
    MilterServerContextPrivate *priv;

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(context);
    agent = MILTER_AGENT(context);
    protocol_agent = MILTER_PROTOCOL_AGENT(context);

    milter_protocol_agent_set_macro_context(protocol_agent, command);
    macros = milter_protocol_agent_get_macros(protocol_agent);
    milter_protocol_agent_set_macro_context(protocol_agent,
                                            MILTER_COMMAND_UNKNOWN);
    if (!macros || g_hash_table_size(macros) == 0)
        return TRUE;

    target_macros = macros;
    macros_requests = milter_protocol_agent_get_macros_requests(protocol_agent);

    if (macros_requests) {
        request_symbols =
            milter_macros_requests_get_symbols(macros_requests, command);
        if (request_symbols)
            filtered_macros = filter_macros(macros, request_symbols);
        if (!filtered_macros)
            return TRUE;
        target_macros = filtered_macros;
    }

    encoder = milter_agent_get_encoder(agent);
    milter_command_encoder_encode_define_macro(MILTER_COMMAND_ENCODER(encoder),
                                               &packet, &packet_size,
                                               command,
                                               target_macros);
    if (filtered_macros)
        g_hash_table_unref(filtered_macros);

    command_name = milter_utils_get_enum_nick_name(MILTER_TYPE_COMMAND, command);
    inspected_macros = milter_utils_inspect_hash_string_string(target_macros);
    milter_debug("[server][send][%s][macros] %s: %s",
                 command_name,
                 inspected_macros,
                 milter_server_context_get_name(context));
    g_free(command_name);
    g_free(inspected_macros);

    return write_packet(context, packet, packet_size,
                        MILTER_SERVER_CONTEXT_STATE_DEFINE_MACRO);
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
    gchar *packet = NULL;
    gsize packet_size;
    MilterEncoder *encoder;
    gboolean stop = FALSE;

    milter_debug("[server][send][connect] <%s>: %s",
                 host_name,
                 milter_server_context_get_name(context));

    g_signal_emit(context, signals[STOP_ON_CONNECT], 0,
                  host_name, address, address_length, &stop);
    if (stop) {
        stop_on_state(context, MILTER_SERVER_CONTEXT_STATE_CONNECT);
        return TRUE;
    }

    write_macro(context, MILTER_COMMAND_CONNECT);

    encoder = milter_agent_get_encoder(MILTER_AGENT(context));
    milter_command_encoder_encode_connect(MILTER_COMMAND_ENCODER(encoder),
                                          &packet, &packet_size,
                                          host_name, address, address_length);

    return write_packet(context, packet, packet_size,
                        MILTER_SERVER_CONTEXT_STATE_CONNECT);
}

gboolean
milter_server_context_envelope_from (MilterServerContext *context,
                                     const gchar         *from)
{
    gchar *packet = NULL;
    gsize packet_size;
    MilterEncoder *encoder;
    gboolean stop = FALSE;

    milter_debug("[server][send][envelope-from] <%s>: %s",
                 from,
                 milter_server_context_get_name(context));

    g_signal_emit(context, signals[STOP_ON_ENVELOPE_FROM], 0, from, &stop);
    if (stop) {
        stop_on_state(context, MILTER_SERVER_CONTEXT_STATE_ENVELOPE_FROM);
        return TRUE;
    }

    write_macro(context, MILTER_COMMAND_ENVELOPE_FROM);

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
    gchar *packet = NULL;
    gsize packet_size;
    MilterEncoder *encoder;
    MilterCommandEncoder *command_encoder;
    gboolean stop = FALSE;

    milter_debug("[server][send][envelope-recipient] <%s>: %s",
                 recipient,
                 milter_server_context_get_name(context));

    g_signal_emit(context, signals[STOP_ON_ENVELOPE_RECIPIENT], 0,
                  recipient, &stop);
    if (stop) {
        stop_on_state(context, MILTER_SERVER_CONTEXT_STATE_ENVELOPE_RECIPIENT);
        return TRUE;
    }

    write_macro(context, MILTER_COMMAND_ENVELOPE_RECIPIENT);

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
    gchar *packet = NULL;
    gsize packet_size;
    MilterEncoder *encoder;

    milter_debug("[server][send][data] %s",
                 milter_server_context_get_name(context));

    write_macro(context, MILTER_COMMAND_DATA);

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
    gchar *packet = NULL;
    gsize packet_size;
    MilterEncoder *encoder;

    milter_debug("[server][send][unknown] <%s>: %s",
                 command,
                 milter_server_context_get_name(context));

    encoder = milter_agent_get_encoder(MILTER_AGENT(context));
    milter_command_encoder_encode_unknown(MILTER_COMMAND_ENCODER(encoder),
                                          &packet, &packet_size, command);

    return write_packet(context, packet, packet_size,
                        MILTER_SERVER_CONTEXT_STATE_UNKNOWN);
}

gboolean
milter_server_context_header (MilterServerContext *context,
                              const gchar         *name,
                              const gchar         *value)
{
    gchar *packet = NULL;
    gsize packet_size;
    MilterEncoder *encoder;
    gboolean stop = FALSE;

    milter_debug("[server][send][header] <%s>=<%s>: %s",
                 name, value,
                 milter_server_context_get_name(context));

    g_signal_emit(context, signals[STOP_ON_HEADER], 0, name, value, &stop);
    if (stop) {
        stop_on_state(context, MILTER_SERVER_CONTEXT_STATE_HEADER);
        return TRUE;
    }

    write_macro(context, MILTER_COMMAND_HEADER);

    encoder = milter_agent_get_encoder(MILTER_AGENT(context));
    milter_command_encoder_encode_header(MILTER_COMMAND_ENCODER(encoder),
                                         &packet, &packet_size, name, value);

    return write_packet(context, packet, packet_size,
                        MILTER_SERVER_CONTEXT_STATE_HEADER);
}

gboolean
milter_server_context_end_of_header (MilterServerContext *context)
{
    gchar *packet = NULL;
    gsize packet_size;
    MilterEncoder *encoder;

    milter_debug("[server][send][end-of-header] %s",
                 milter_server_context_get_name(context));

    write_macro(context, MILTER_COMMAND_END_OF_HEADER);

    encoder = milter_agent_get_encoder(MILTER_AGENT(context));
    milter_command_encoder_encode_end_of_header(MILTER_COMMAND_ENCODER(encoder),
                                                &packet, &packet_size);

    return write_packet(context, packet, packet_size,
                        MILTER_SERVER_CONTEXT_STATE_END_OF_HEADER);
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

gboolean
milter_server_context_body (MilterServerContext *context,
                            const gchar         *chunk,
                            gsize                size)
{
    gchar *packet = NULL;
    gsize packet_size;
    MilterEncoder *encoder;
    MilterCommandEncoder *command_encoder;
    gboolean stop = FALSE;

    milter_debug("[server][send][body] size=%" G_GSIZE_FORMAT ": %s",
                 size,
                 milter_server_context_get_name(context));

    g_signal_emit(context, signals[STOP_ON_BODY], 0, chunk, size, &stop);
    if (stop) {
        stop_on_state(context, MILTER_SERVER_CONTEXT_STATE_BODY);
        return TRUE;
    }

    write_macro(context, MILTER_COMMAND_BODY);

    encoder = milter_agent_get_encoder(MILTER_AGENT(context));
    command_encoder = MILTER_COMMAND_ENCODER(encoder);
    append_body_response_queue(context);

    while (size > 0) {
        gsize packed_size;

        milter_command_encoder_encode_body(command_encoder,
                                           &packet, &packet_size,
                                           chunk, size, &packed_size);

        if (!write_packet(context, packet, packet_size,
                          MILTER_SERVER_CONTEXT_STATE_BODY))
            return FALSE;

        chunk += packed_size;
        size -= packed_size;
        increment_process_body_count(context);
    }

    return TRUE;
}

gboolean
milter_server_context_end_of_message (MilterServerContext *context,
                                      const gchar         *chunk,
                                      gsize                size)
{
    gchar *packet = NULL;
    gsize packet_size;
    MilterEncoder *encoder;
    gboolean stop = FALSE;

    milter_debug("[server][send][end-of-message] %s",
                 milter_server_context_get_name(context));

    g_signal_emit(context, signals[STOP_ON_END_OF_MESSAGE], 0,
                  chunk, size, &stop);
    if (stop) {
        stop_on_state(context, MILTER_SERVER_CONTEXT_STATE_END_OF_MESSAGE);
        return TRUE;
    }

    write_macro(context, MILTER_COMMAND_END_OF_MESSAGE);

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
    gchar *packet = NULL;
    gsize packet_size;
    MilterEncoder *encoder;

    milter_debug("[server][send][quit] %s",
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
    gchar *packet = NULL;
    gsize packet_size;
    MilterEncoder *encoder;

    milter_debug("[server][send][abort] %s",
                 milter_server_context_get_name(context));
    encoder = milter_agent_get_encoder(MILTER_AGENT(context));
    milter_command_encoder_encode_abort(MILTER_COMMAND_ENCODER(encoder),
                                        &packet, &packet_size);

    return write_packet(context, packet, packet_size,
                        MILTER_SERVER_CONTEXT_STATE_ABORT);
}

static void
invalid_state (MilterServerContext *context,
               MilterServerContextState state)
{
    GError *error = NULL;
    gchar *state_name;

    state_name = milter_utils_get_enum_nick_name(MILTER_TYPE_SERVER_CONTEXT_STATE,
                                                 state);
    g_set_error(&error,
                MILTER_SERVER_CONTEXT_ERROR,
                MILTER_SERVER_CONTEXT_ERROR_INVALID_STATE,
                "Invalid state: %s", state_name);
    milter_error("[server][error][state] %s: %s",
                 error->message,
                 milter_server_context_get_name(context));
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
    MilterServerContext *context = MILTER_SERVER_CONTEXT(user_data);
    MilterServerContextState state;
    MilterServerContextPrivate *priv;

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(context);
    state = priv->state;

    disable_timeout(priv);

    {
        gchar *inspected_option;

        inspected_option = milter_option_inspect(option);
        milter_debug("[server][receive][negotiate-reply] %s: %s",
                     inspected_option,
                     milter_server_context_get_name(context));
        g_free(inspected_option);
    }
    if (priv->option)
        milter_option_combine(priv->option, option);
    else
        priv->option = milter_option_copy(option);

    if (state == MILTER_SERVER_CONTEXT_STATE_NEGOTIATE) {
        g_signal_emit_by_name(context, "negotiate-reply",
                              option, macros_requests);
        milter_protocol_agent_set_macros_requests(MILTER_PROTOCOL_AGENT(context),
                                                  macros_requests);
    } else {
        invalid_state(context, state);
    }
}

static void
clear_process_body_count (MilterServerContext *context)
{
    MilterServerContextPrivate *priv;

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(context);
    g_list_free(priv->body_response_queue);
    priv->body_response_queue = NULL;

    if (priv->sent_end_of_message)
        priv->state = MILTER_SERVER_CONTEXT_STATE_END_OF_MESSAGE;
}

static void
decrement_process_body_count (MilterServerContext *context)
{
    MilterServerContextPrivate *priv;
    GList *first_node;
    guint process_body_count;

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(context);
    if (!priv->body_response_queue) {
        milter_error("[server][error] not wait for response on body anymore: %s",
                     milter_server_context_get_name(context));
        return;
    }

    first_node = g_list_first(priv->body_response_queue);
    process_body_count = GPOINTER_TO_UINT(first_node->data);
    process_body_count--;

    if (process_body_count == 0) {
        priv->body_response_queue =
            g_list_delete_link(priv->body_response_queue, first_node);
        g_signal_emit_by_name(context, "continue");
        if (priv->sent_end_of_message)
            priv->state = MILTER_SERVER_CONTEXT_STATE_END_OF_MESSAGE;
    } else {
        first_node->data = GUINT_TO_POINTER(process_body_count);
    }
}

static void
cb_decoder_continue (MilterReplyDecoder *decoder, gpointer user_data)
{
    MilterServerContext *context = user_data;
    MilterServerContextPrivate *priv;
    gchar *state_name;

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(context);

    disable_timeout(priv);

    state_name = milter_utils_get_enum_nick_name(MILTER_TYPE_SERVER_CONTEXT_STATE,
                                                 priv->state);
    milter_debug("[server][receive][continue][%s] %s",
                 state_name,
                 milter_server_context_get_name(context));
    g_free(state_name);

    switch (priv->state) {
      case MILTER_SERVER_CONTEXT_STATE_BODY:
        decrement_process_body_count(context);
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
        g_signal_emit_by_name(context, "continue");
        break;
      default:
        invalid_state(context, priv->state);
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

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(context);

    disable_timeout(priv);

    milter_debug("[server][receive][reply-code] <%d %s %s>: %s",
                 code, extended_code, message,
                 milter_server_context_get_name(context));

    state = priv->state;

    switch (state) {
      case MILTER_SERVER_CONTEXT_STATE_CONNECT: /* more? */
        invalid_state(context, state);
        break;
      default:
        g_signal_emit_by_name(context, "reply-code", code, extended_code, message);
        break;
    }

    switch (state) {
      case MILTER_SERVER_CONTEXT_STATE_BODY:
        clear_process_body_count(context);
        break;
      default:
        break;
    }
}

static void
cb_decoder_temporary_failure (MilterReplyDecoder *decoder, gpointer user_data)
{
    MilterServerContext *context = user_data;
    MilterServerContextPrivate *priv;
    gchar *state_name;

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(context);

    disable_timeout(priv);

    state_name = milter_utils_get_enum_nick_name(MILTER_TYPE_SERVER_CONTEXT_STATE,
                                                 priv->state);
    milter_debug("[server][receive][temporary-failure][%s] %s",
                 state_name,
                 milter_server_context_get_name(context));
    g_free(state_name);

    g_signal_emit_by_name(context, "temporary-failure");

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
    gchar *state_name;

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(context);

    disable_timeout(priv);

    state_name = milter_utils_get_enum_nick_name(MILTER_TYPE_SERVER_CONTEXT_STATE,
                                                 priv->state);
    milter_debug("[server][receive][reject][%s] %s",
                 state_name,
                 milter_server_context_get_name(context));
    g_free(state_name);

    g_signal_emit_by_name(context, "reject");

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
    MilterServerContextState state;
    gchar *state_name;

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(context);

    disable_timeout(priv);

    state = priv->state;
    state_name = milter_utils_get_enum_nick_name(MILTER_TYPE_SERVER_CONTEXT_STATE,
                                                 state);
    milter_debug("[server][receive][accept][%s] %s",
                 state_name,
                 milter_server_context_get_name(context));
    g_free(state_name);

    g_signal_emit_by_name(context, "accept");

    switch (state) {
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
    gchar *state_name;

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(user_data);

    disable_timeout(priv);

    state_name = milter_utils_get_enum_nick_name(MILTER_TYPE_SERVER_CONTEXT_STATE,
                                                 priv->state);
    milter_debug("[server][receive][discard][%s] %s",
                 state_name,
                 milter_server_context_get_name(context));
    g_free(state_name);

    g_signal_emit_by_name(context, "discard");

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

    disable_timeout(priv);

    milter_debug("[server][receive][add-header] <%s>=<%s>: %s",
                 name, value,
                 milter_server_context_get_name(context));

    if (priv->state == MILTER_SERVER_CONTEXT_STATE_END_OF_MESSAGE)
        g_signal_emit_by_name(context, "add-header", name, value);
    else
        invalid_state(context, priv->state);
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

    disable_timeout(priv);

    milter_debug("[server][receive][insert-header] [%u]<%s>=<%s>: %s",
                 index, name, value,
                 milter_server_context_get_name(context));

    if (priv->state == MILTER_SERVER_CONTEXT_STATE_END_OF_MESSAGE)
        g_signal_emit_by_name(context, "insert-header", index, name, value);
    else
        invalid_state(context, priv->state);
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

    disable_timeout(priv);

    milter_debug("[server][receive][change-header] <%s>[%u]=<%s>: %s",
                 name, index, value,
                 milter_server_context_get_name(context));

    if (priv->state == MILTER_SERVER_CONTEXT_STATE_END_OF_MESSAGE)
        g_signal_emit_by_name(context, "change-header", name, index, value);
    else
        invalid_state(context, priv->state);
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

    disable_timeout(priv);

    milter_debug("[server][receive][delete-header] <%s>[%u]: %s",
                 name, index,
                 milter_server_context_get_name(context));

    if (priv->state == MILTER_SERVER_CONTEXT_STATE_END_OF_MESSAGE)
        g_signal_emit_by_name(context, "delete-header", name, index);
    else
        invalid_state(context, priv->state);
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

    disable_timeout(priv);

    milter_debug("[server][receive][change-from] <%s>:<%s>: %s",
                 from, parameters,
                 milter_server_context_get_name(context));

    if (priv->state == MILTER_SERVER_CONTEXT_STATE_END_OF_MESSAGE)
        g_signal_emit_by_name(context, "change-from", from, parameters);
    else
        invalid_state(context, priv->state);
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

    disable_timeout(priv);

    milter_debug("[server][receive][add-recipient] <%s>:<%s>: %s",
                 recipient, parameters,
                 milter_server_context_get_name(context));

    if (priv->state == MILTER_SERVER_CONTEXT_STATE_END_OF_MESSAGE)
        g_signal_emit_by_name(context, "add-recipient", recipient, parameters);
    else
        invalid_state(context, priv->state);
}

static void
cb_decoder_delete_recipient (MilterReplyDecoder *decoder,
                             const gchar *recipient,
                             gpointer user_data)
{
    MilterServerContext *context = user_data;
    MilterServerContextPrivate *priv;

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(user_data);

    disable_timeout(priv);

    milter_debug("[server][receive][delete-recipient]: <%s>: %s",
                 recipient,
                 milter_server_context_get_name(context));

    g_signal_emit_by_name(user_data, "delete-recipient",recipient);
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

    disable_timeout(priv);

    milter_debug("[server][receive][replace-body] "
                 "body_size=%" G_GSIZE_FORMAT ": %s",
                 body_size,
                 milter_server_context_get_name(context));

    if (priv->state == MILTER_SERVER_CONTEXT_STATE_END_OF_MESSAGE)
        g_signal_emit_by_name(context, "replace-body", body, body_size);
    else
        invalid_state(context, priv->state);
}

static void
cb_decoder_progress (MilterReplyDecoder *decoder, gpointer user_data)
{
    MilterServerContext *context = user_data;
    MilterServerContextPrivate *priv;

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(user_data);

    disable_timeout(priv);

    milter_debug("[server][receive][progress] %s",
                 milter_server_context_get_name(context));

    g_signal_emit_by_name(user_data, "progress");
}

static void
cb_decoder_quarantine (MilterReplyDecoder *decoder,
                       const gchar *reason,
                       gpointer user_data)
{
    MilterServerContext *context = user_data;
    MilterServerContextPrivate *priv;

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(user_data);

    disable_timeout(priv);

    milter_debug("[server][receive][quarantine] <%s>: %s",
                 reason,
                 milter_server_context_get_name(context));

    g_signal_emit_by_name(user_data, "quarantine", reason);
}

static void
cb_decoder_connection_failure (MilterReplyDecoder *decoder, gpointer user_data)
{
    MilterServerContext *context = user_data;
    MilterServerContextPrivate *priv;

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(user_data);

    disable_timeout(priv);

    milter_debug("[server][receive][connection-failure] %s",
                 milter_server_context_get_name(context));

    g_signal_emit_by_name(user_data, "connection-failure");
}

static void
cb_decoder_shutdown (MilterReplyDecoder *decoder, gpointer user_data)
{
    MilterServerContext *context = user_data;
    MilterServerContextPrivate *priv;

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(user_data);

    disable_timeout(priv);

    milter_debug("[server][receive][shutdown] %s",
                 milter_server_context_get_name(context));

    g_signal_emit_by_name(user_data, "shutdown");
}

static void
cb_decoder_skip (MilterReplyDecoder *decoder, gpointer user_data)
{
    MilterServerContext *context = user_data;
    MilterServerContextPrivate *priv;

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(context);

    disable_timeout(priv);

    milter_debug("[server][receive][skip] %s",
                 milter_server_context_get_name(context));

    if (priv->state == MILTER_SERVER_CONTEXT_STATE_BODY) {
        g_signal_emit_by_name(context, "skip");
        clear_process_body_count(context);
        priv->skip_body = TRUE;
    } else {
        invalid_state(context, priv->state);
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

static GIOChannel *
make_channel (int fd, GIOFlags flags, GError **error)
{
    GIOChannel *io_channel;

    io_channel = g_io_channel_unix_new(fd);

    if (g_io_channel_set_encoding(io_channel, NULL, error) !=
        G_IO_STATUS_NORMAL) {
        g_io_channel_unref(io_channel);
        return NULL;
    }

    g_io_channel_set_flags(io_channel, G_IO_FLAG_NONBLOCK | flags, error);
    g_io_channel_set_buffered(io_channel, FALSE);

    return io_channel;
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

    milter_error("[server][timeout][connection] %s",
                 milter_server_context_get_name(context));
    g_signal_emit(context, signals[CONNECTION_TIMEOUT], 0);

    return FALSE;
}

static gboolean
prepare_reader (MilterServerContext *context)
{
    MilterReader *reader;
    GIOChannel *read_channel = NULL;
    GError *io_error = NULL;
    MilterServerContextPrivate *priv;

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(context);

    read_channel = make_channel(priv->client_fd,
                                G_IO_FLAG_IS_READABLE, &io_error);
    if (!read_channel) {
        GError *error = NULL;
        close_client_fd(priv);
        milter_utils_set_error_with_sub_error(
            &error,
            MILTER_SERVER_CONTEXT_ERROR,
            MILTER_SERVER_CONTEXT_ERROR_CONNECTION_FAILURE,
            io_error,
            "Failed to prepare reading channel");
        milter_error("[server][error][reader] %s: %s",
                     error->message,
                     milter_server_context_get_name(context));
        milter_error_emittable_emit(MILTER_ERROR_EMITTABLE(context),
                                    error);
        g_error_free(io_error);
        g_error_free(error);
        return FALSE;
    }

    g_io_channel_set_close_on_unref(read_channel, TRUE);

    reader = milter_reader_io_channel_new(read_channel);
    g_io_channel_unref(read_channel);
    milter_agent_set_reader(MILTER_AGENT(context), reader);
    g_object_unref(reader);

    return TRUE;
}

static gboolean
prepare_writer (MilterServerContext *context)
{
    MilterWriter *writer;
    GIOChannel *write_channel = NULL;
    GError *io_error = NULL;
    MilterServerContextPrivate *priv;

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(context);

    write_channel = make_channel(priv->client_fd,
                                 G_IO_FLAG_IS_WRITEABLE, &io_error);
    if (!write_channel) {
        GError *error = NULL;
        close_client_fd(priv);
        milter_utils_set_error_with_sub_error(
            &error,
            MILTER_SERVER_CONTEXT_ERROR,
            MILTER_SERVER_CONTEXT_ERROR_CONNECTION_FAILURE,
            io_error,
            "Failed to prepare writing channel");
        milter_error("[server][error][writer] %s: %s",
                     error->message,
                     milter_server_context_get_name(context));
        milter_error_emittable_emit(MILTER_ERROR_EMITTABLE(context),
                                    error);
        g_error_free(io_error);
        g_error_free(error);
        return FALSE;
    }
    writer = milter_writer_io_channel_new(write_channel);
    g_io_channel_unref(write_channel);
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

    disable_timeout(priv);

    option_length = sizeof(socket_errno);
    if (getsockopt(priv->client_fd, SOL_SOCKET, SO_ERROR,
                   &socket_errno, &option_length) == -1) {
        socket_errno = errno;
    }

    if (socket_errno) {
        GError *error = NULL;

        error = g_error_new(MILTER_SERVER_CONTEXT_ERROR,
                            MILTER_SERVER_CONTEXT_ERROR_CONNECTION_FAILURE,
                            "Failed to connect to %s: %s",
                            priv->spec, g_strerror(socket_errno));
        milter_error("[server][error][connect] %s: %s",
                     error->message,
                     milter_server_context_get_name(context));
        milter_error_emittable_emit(MILTER_ERROR_EMITTABLE(context), error);
        g_error_free(error);

        close_client_fd(priv);
    } else {
        if (prepare_reader(context) && prepare_writer(context)) {
            g_signal_emit(context, signals[READY], 0);
            milter_agent_start(MILTER_AGENT(context));
        } else {
            close_client_fd(priv);
        }
    }

    return FALSE;
}

gboolean
milter_server_context_establish_connection (MilterServerContext *context,
                                            GError **error)
{
    MilterServerContextPrivate *priv;
    GIOChannel *channel = NULL;
    GError *io_error = NULL;

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(context);

    if (!priv->address) {
        g_set_error(error,
                    MILTER_SERVER_CONTEXT_ERROR,
                    MILTER_SERVER_CONTEXT_ERROR_NO_SPEC,
                    "No spec set yet.");
        return FALSE;
    }

    priv->client_fd = socket(priv->domain, SOCK_STREAM, 0);
    if (priv->client_fd == -1) {
        g_set_error(error,
                    MILTER_SERVER_CONTEXT_ERROR,
                    MILTER_SERVER_CONTEXT_ERROR_CONNECTION_FAILURE,
                    "Failed to socket(): %s", g_strerror(errno));
        return FALSE;
    }

    channel = g_io_channel_unix_new(priv->client_fd);
    g_io_channel_set_flags(channel, G_IO_FLAG_NONBLOCK, &io_error);
    if (io_error) {
        close_client_fd(priv);
        milter_utils_set_error_with_sub_error(
            error,
            MILTER_SERVER_CONTEXT_ERROR,
            MILTER_SERVER_CONTEXT_ERROR_CONNECTION_FAILURE,
            io_error,
            "Failed to prepare connect()");
        return FALSE;
    }

    priv->connect_watch_id = g_io_add_watch(channel,
                                            G_IO_OUT |
                                            G_IO_ERR | G_IO_HUP | G_IO_NVAL,
                                            connect_watch_func, context);
    g_io_channel_unref(channel);

    priv->timeout_id = milter_utils_timeout_add(priv->connection_timeout,
                                                cb_connection_timeout, context);

    if (connect(priv->client_fd, priv->address, priv->address_size) == -1) {
        if (errno == EINPROGRESS)
            return TRUE;

        close_client_fd(priv);
        disable_timeout(priv);
        g_set_error(error,
                    MILTER_SERVER_CONTEXT_ERROR,
                    MILTER_SERVER_CONTEXT_ERROR_CONNECTION_FAILURE,
                    "Failed to connect to %s: %s",
                    priv->spec, g_strerror(errno));

        return FALSE;
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

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
