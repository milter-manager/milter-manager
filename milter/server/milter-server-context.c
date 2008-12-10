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

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h>

#include <unistd.h>
#include <errno.h>

#include <milter/core.h>
#include "milter/core/milter-marshalers.h"
#include "milter-server-context.h"

enum
{
    CHECK_CONNECT,
    CHECK_HELO,
    CHECK_ENVELOPE_FROM,
    CHECK_ENVELOPE_RECIPIENT,
    CHECK_HEADER,
    CHECK_BODY,
    CHECK_END_OF_MESSAGE,

    READY,

    CONNECTION_TIMEOUT,
    WRITING_TIMEOUT,
    READING_TIMEOUT,
    END_OF_MESSAGE_TIMEOUT,

    LAST_SIGNAL
};

static gint signals[LAST_SIGNAL] = {0};

static gboolean    check_accumulator  (GSignalInvocationHint *hint,
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

static gboolean check_connect        (MilterServerContext *context,
                                      const gchar   *host_name,
                                      const struct sockaddr *address,
                                      socklen_t      address_length);
static gboolean check_helo           (MilterServerContext *context,
                                      const gchar   *fqdn);
static gboolean check_envelope_from  (MilterServerContext *context,
                                      const gchar   *from);
static gboolean check_envelope_recipient
                                     (MilterServerContext *context,
                                      const gchar   *recipient);
static gboolean check_header         (MilterServerContext *context,
                                      const gchar   *name,
                                      const gchar   *value);
static gboolean check_body           (MilterServerContext *context,
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

    klass->check_connect          = check_connect;
    klass->check_helo             = check_helo;
    klass->check_envelope_from    = check_envelope_from;
    klass->check_envelope_recipient = check_envelope_recipient;
    klass->check_header           = check_header;
    klass->check_body             = check_body;

    signals[READY] =
        g_signal_new("ready",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterServerContextClass,
                                     ready),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__VOID,
                     G_TYPE_NONE, 0);

    signals[CHECK_CONNECT] =
        g_signal_new("check-connect",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterServerContextClass, check_connect),
                     check_accumulator, NULL,
                     _milter_marshal_BOOLEAN__STRING_POINTER_UINT,
                     G_TYPE_BOOLEAN, 3,
                     G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_UINT);

    signals[CHECK_HELO] =
        g_signal_new("check-helo",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterServerContextClass, check_helo),
                     check_accumulator, NULL,
                     _milter_marshal_BOOLEAN__STRING,
                     G_TYPE_BOOLEAN, 1, G_TYPE_STRING);

    signals[CHECK_ENVELOPE_FROM] =
        g_signal_new("check-envelope-from",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterServerContextClass,
                                     check_envelope_from),
                     check_accumulator, NULL,
                     _milter_marshal_BOOLEAN__STRING,
                     G_TYPE_BOOLEAN, 1, G_TYPE_STRING);

    signals[CHECK_ENVELOPE_RECIPIENT] =
        g_signal_new("check-envelope-recipient",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterServerContextClass,
                                     check_envelope_recipient),
                     check_accumulator, NULL,
                     _milter_marshal_BOOLEAN__STRING,
                     G_TYPE_BOOLEAN, 1, G_TYPE_STRING);

    signals[CHECK_HEADER] =
        g_signal_new("check-header",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterServerContextClass, check_header),
                     check_accumulator, NULL,
                     _milter_marshal_BOOLEAN__STRING_STRING,
                     G_TYPE_BOOLEAN, 2, G_TYPE_STRING, G_TYPE_STRING);

    signals[CHECK_BODY] =
        g_signal_new("check-body",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterServerContextClass, check_body),
                     check_accumulator, NULL,
#if GLIB_SIZEOF_SIZE_T == 8
                     _milter_marshal_BOOLEAN__STRING_UINT64,
                     G_TYPE_BOOLEAN, 2, G_TYPE_STRING, G_TYPE_UINT64
#else
                     _milter_marshal_BOOLEAN__STRING_UINT,
                     G_TYPE_BOOLEAN, 2, G_TYPE_STRING, G_TYPE_UINT
#endif
            );

    signals[CHECK_END_OF_MESSAGE] =
        g_signal_new("check-end-of-message",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterServerContextClass,
                                     check_end_of_message),
                     check_accumulator, NULL,
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
check_connect (MilterServerContext *context,
               const gchar   *host_name,
               const struct sockaddr *address,
               socklen_t      address_length)
{
    return FALSE;
}

static gboolean
check_helo (MilterServerContext *context, const gchar *fqdn)
{
    return FALSE;
}

static gboolean
check_envelope_from (MilterServerContext *context, const gchar *from)
{
    return FALSE;
}

static gboolean
check_envelope_recipient (MilterServerContext *context, const gchar *recipient)
{
    return FALSE;
}

static gboolean
check_header (MilterServerContext *context,
              const gchar   *name,
              const gchar   *value)
{
    return FALSE;
}

static gboolean
check_body (MilterServerContext *context,
            const gchar   *chunk,
            gsize          size)
{
    return FALSE;
}

static gboolean
cb_writing_timeout (gpointer data)
{
    g_signal_emit(data, signals[WRITING_TIMEOUT], 0);

    return FALSE;
}

static gboolean
cb_end_of_message_timeout (gpointer data)
{
    g_signal_emit(data, signals[END_OF_MESSAGE_TIMEOUT], 0);

    return FALSE;
}

static gboolean
cb_reading_timeout (gpointer data)
{
    g_signal_emit(data, signals[READING_TIMEOUT], 0);

    return FALSE;
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

    if (priv->timeout_id > 0) {
        gchar *inspected_state;
        GError *error = NULL;

        inspected_state =
            milter_utils_get_enum_name(MILTER_TYPE_SERVER_CONTEXT_STATE,
                                       priv->state);
        g_set_error(&error,
                    MILTER_SERVER_CONTEXT_ERROR,
                    MILTER_SERVER_CONTEXT_ERROR_BUSY,
                    "Previous command(%s) has been processing in milter",
                    inspected_state);
        milter_error("%s", error->message);
        milter_error_emittable_emit(MILTER_ERROR_EMITTABLE(context),
                                    error);
        g_error_free(error);
        g_free(inspected_state);
        return FALSE;
    }

    priv->timeout_id = milter_utils_timeout_add(priv->writing_timeout,
                                                cb_writing_timeout,
                                                context);
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
        milter_error("%s", error->message);
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
      case MILTER_SERVER_CONTEXT_STATE_ABORT:
      case MILTER_SERVER_CONTEXT_STATE_QUIT:
        priv->state = next_state;
        disable_timeout(priv);
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
pass_state (MilterServerContext *context, MilterServerContextState state)
{
    gchar *inspected_state;
    MilterServerContextPrivate *priv;

    inspected_state = milter_utils_get_enum_name(MILTER_TYPE_SERVER_CONTEXT_STATE,
                                                 state);
    milter_debug("pass state: %s", inspected_state);
    milter_statistics("Pass filter process of %s(%p) on %s",
                      milter_server_context_get_name(context),
                      context,
                      inspected_state);
    g_free(inspected_state);
    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(context);
    priv->state = state;
    g_signal_emit_by_name(context, "accept");
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

        inspected_option = milter_utils_inspect_object(G_OBJECT(option));
        milter_debug("%s is sending NEGOTIATE: %s",
                     milter_server_context_get_name(context),
                     inspected_option);
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
    gboolean pass;

    milter_debug("%s is sending HELO: <%s>",
                 milter_server_context_get_name(context),
                 fqdn);

    g_signal_emit(context, signals[CHECK_HELO],0, fqdn, &pass);
    if (pass) {
        pass_state(context, MILTER_SERVER_CONTEXT_STATE_HELO);
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
    gboolean pass = FALSE;

    milter_debug("%s is sending CONNECT: host_name=<%s>",
                 milter_server_context_get_name(context),
                 host_name);

    g_signal_emit(context, signals[CHECK_CONNECT], 0,
                  host_name, address, address_length, &pass);
    if (pass) {
        pass_state(context, MILTER_SERVER_CONTEXT_STATE_CONNECT);
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
    gboolean pass;

    milter_debug("%s is sending MAIL: <%s>",
                 milter_server_context_get_name(context),
                 from);

    g_signal_emit(context, signals[CHECK_ENVELOPE_FROM], 0, from, &pass);
    if (pass) {
        pass_state(context, MILTER_SERVER_CONTEXT_STATE_ENVELOPE_FROM);
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
    gboolean pass;

    milter_debug("%s is sending RCPT: <%s>",
                 milter_server_context_get_name(context),
                 recipient);

    g_signal_emit(context, signals[CHECK_ENVELOPE_RECIPIENT], 0,
                  recipient, &pass);
    if (pass) {
        pass_state(context, MILTER_SERVER_CONTEXT_STATE_ENVELOPE_RECIPIENT);
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

    milter_debug("%s is sending DATA",
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

    milter_debug("%s is sending UNKNOWN: <%s>",
                 milter_server_context_get_name(context),
                 command);

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
    gboolean pass;

    milter_debug("%s is sending HEADER: <%s>=<%s>",
                 milter_server_context_get_name(context),
                 name, value);

    g_signal_emit(context, signals[CHECK_HEADER], 0, name, value, &pass);
    if (pass) {
        pass_state(context, MILTER_SERVER_CONTEXT_STATE_HEADER);
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

    milter_debug("%s is sending END_OF_HEADER",
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
    gboolean pass;

    milter_debug("%s is sending BODY: body_size=%" G_GSIZE_FORMAT,
                 milter_server_context_get_name(context),
                 size);

    g_signal_emit(context, signals[CHECK_BODY], 0, chunk, size, &pass);
    if (pass) {
        pass_state(context, MILTER_SERVER_CONTEXT_STATE_BODY);
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
    gboolean pass;

    milter_debug("%s is sending END_OF_MESSAGE.",
                 milter_server_context_get_name(context));

    g_signal_emit(context, signals[CHECK_END_OF_MESSAGE],0, chunk, size, &pass);
    if (pass) {
        pass_state(context, MILTER_SERVER_CONTEXT_STATE_END_OF_MESSAGE);
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

    milter_debug("%s is sending QUIT",
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

    milter_debug("%s is sending ABORT",
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

    state_name = milter_utils_get_enum_name(MILTER_TYPE_SERVER_CONTEXT_STATE,
                                            state);
    g_set_error(&error,
                MILTER_SERVER_CONTEXT_ERROR,
                MILTER_SERVER_CONTEXT_ERROR_INVALID_STATE,
                "Invalid state: %s", state_name);
    milter_error("%s", error->message);
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

        inspected_option = milter_utils_inspect_object(G_OBJECT(option));
        milter_debug("%s received NEGOTIATE REPLY: %s",
                     milter_server_context_get_name(context),
                     inspected_option);
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
        milter_error("%s does not wait for response on body anymore.",
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
    gchar *state_string;

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(context);

    disable_timeout(priv);

    state_string = milter_utils_get_enum_name(MILTER_TYPE_SERVER_CONTEXT_STATE,
                                              priv->state);
    milter_debug("%s received CONTINUE on %s.",
                 milter_server_context_get_name(context),
                 state_string);
    g_free(state_string);

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

    milter_debug("%s received REPLY_CODE: <%d %s %s>",
                 milter_server_context_get_name(context),
                 code, extended_code, message);

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

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(context);

    disable_timeout(priv);

    milter_debug("%s received TEMPORARY_FAILURE",
                 milter_server_context_get_name(context));

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

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(context);

    disable_timeout(priv);

    milter_debug("%s received REJECT",
                 milter_server_context_get_name(context));

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
    gchar *state_string;

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(context);

    disable_timeout(priv);

    state = priv->state;
    state_string = milter_utils_get_enum_name(MILTER_TYPE_SERVER_CONTEXT_STATE,
                                              state);
    milter_debug("%s received ACCEPT on %s,",
                 milter_server_context_get_name(context), state_string);
    g_free(state_string);

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

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(user_data);

    disable_timeout(priv);

    milter_debug("%s received DISCARD",
                 milter_server_context_get_name(context));

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

    milter_debug("%s received ADD_HEADER: <%s>=<%s>",
                 milter_server_context_get_name(context),
                name, value);

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

    milter_debug("%s received INSERT_HEADER: [%u] <%s>=<%s>",
                 milter_server_context_get_name(context),
                 index, name, value);

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

    milter_debug("%s received CHANGE_HEADER: <%s>[%u]=<%s>",
                 milter_server_context_get_name(context),
                 name, index, value);

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

    milter_debug("%s received DELETE_HEADER: <%s>[%u]",
                 milter_server_context_get_name(context),
                name, index);

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

    milter_debug("%s received CHANGE_FROM: <%s>=<%s>",
                 milter_server_context_get_name(context),
                 from, parameters);

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

    milter_debug("%s received ADD_RECIPIENT: <%s>=<%s>",
                 milter_server_context_get_name(context),
                 recipient, parameters);

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

    milter_debug("%s received DELETE_RECIPIENT: <%s>",
                 milter_server_context_get_name(context),
                 recipient);

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

    milter_debug("%s received REPLACE_BODY: body_size = %" G_GSIZE_FORMAT,
                 milter_server_context_get_name(context),
                 body_size);

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

    milter_debug("%s received PROGRESS",
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

    milter_debug("%s received QUARANTINE: <%s>",
                 milter_server_context_get_name(context),
                 reason);

    g_signal_emit_by_name(user_data, "quarantine", reason);
}

static void
cb_decoder_connection_failure (MilterReplyDecoder *decoder, gpointer user_data)
{
    MilterServerContext *context = user_data;
    MilterServerContextPrivate *priv;

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(user_data);

    disable_timeout(priv);

    milter_debug("%s received CONNECTION_FAILURE",
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

    milter_debug("%s received SHUTDOWN",
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

    milter_debug("%s received SKIP",
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
    g_io_channel_set_close_on_unref(io_channel, TRUE);

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
    milter_error("connection timeout");
    g_signal_emit(data, signals[CONNECTION_TIMEOUT], 0);

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
        milter_error_emittable_emit(MILTER_ERROR_EMITTABLE(context),
                                    error);
        g_error_free(io_error);
        g_error_free(error);
        return FALSE;
    }

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
        milter_error_emittable_emit(MILTER_ERROR_EMITTABLE(context),
                                    error);
        g_error_free(io_error);
        g_error_free(error);
        return FALSE;
    }
    writer = milter_writer_io_channel_new(write_channel);
    /* g_io_channel_unref(write_channel); */
    milter_agent_set_writer(MILTER_AGENT(context), writer);
    g_object_unref(writer);

    return TRUE;
}

static gboolean
connect_watch_func (GIOChannel *channel, GIOCondition condition, gpointer data)
{
    MilterServerContext *context = data;
    MilterServerContextPrivate *priv;

    priv = MILTER_SERVER_CONTEXT_GET_PRIVATE(context);

    disable_timeout(priv);

    if (condition & G_IO_OUT) {
        if (prepare_reader(context) &&
            prepare_writer(context))
            g_signal_emit(context, signals[READY], 0);
    }

    if (condition & G_IO_ERR ||
        condition & G_IO_HUP ||
        condition & G_IO_NVAL) {
        gchar *message;
        GError *error = NULL;

        message = milter_utils_inspect_io_condition_error(condition);
        g_set_error(&error,
                    MILTER_SERVER_CONTEXT_ERROR,
                    MILTER_SERVER_CONTEXT_ERROR_CONNECTION_FAILURE,
                    "Connection error on %s: %s", priv->spec, message);
        milter_error_emittable_emit(MILTER_ERROR_EMITTABLE(context),
                                    error);
        milter_error("error!: %s", message);
        g_error_free(error);
        g_free(message);
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
        milter_error("failed to socket(): %s", g_strerror(errno));
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
            "Failed to prepare connect().");
        milter_error("failed to prepare connection().");
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
        GError *local_error = NULL;

        if (errno == EINPROGRESS)
            return TRUE;

        close_client_fd(priv);
        disable_timeout(priv);
        local_error = g_error_new(MILTER_SERVER_CONTEXT_ERROR,
                                  MILTER_SERVER_CONTEXT_ERROR_CONNECTION_FAILURE,
                                  "Failed to connect to %s: %s",
                                  priv->spec, g_strerror(errno));
        milter_error("%s", local_error->message);
        g_propagate_error(error, local_error);

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
check_accumulator (GSignalInvocationHint *hint,
                   GValue *return_accumulator,
                   const GValue *context_return,
                   gpointer data)
{
    gboolean pass;

    pass = g_value_get_boolean(context_return);
    g_value_set_boolean(return_accumulator, pass);

    return !pass;
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
