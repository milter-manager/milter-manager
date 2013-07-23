/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Copyright (C) 2008-2013  Kouhei Sutou <kou@clear-code.com>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifdef HAVE_CONFIG_H
#  include "../config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <glib/gstdio.h>
#include <glib/gi18n.h>
#include <fcntl.h>

#ifdef HAVE_LOCALE_H
#  include <locale.h>
#endif

#include <glib/gprintf.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <milter/server.h>
#include <milter/core.h>

#define DEFAULT_NEGOTIATE_VERSION 6

static gboolean verbose = FALSE;
static gboolean output_message = FALSE;
static gchar *spec = NULL;
static gint negotiate_version = DEFAULT_NEGOTIATE_VERSION;
static gint n_threads = 0;
static gchar *connect_host = NULL;
static struct sockaddr *connect_address = NULL;
static socklen_t connect_address_length = 0;
static GHashTable *connect_macros = NULL;
static gchar *helo_host = NULL;
static GHashTable *helo_macros = NULL;
static gchar *envelope_from = NULL;
static GHashTable *envelope_from_macros = NULL;
static gchar **recipients = NULL;
static GHashTable *envelope_recipient_macros = NULL;
static GHashTable *data_macros = NULL;
static GHashTable *end_of_header_macros = NULL;
static gchar **body_chunks = NULL;
static GHashTable *end_of_message_macros = NULL;
static gchar *unknown_command = NULL;
static gchar *authenticated_name = NULL;
static gchar *authenticated_type = NULL;
static gchar *authenticated_author = NULL;
static gboolean use_color = FALSE;
static MilterHeaders *option_headers = NULL;
static gdouble connection_timeout = MILTER_SERVER_CONTEXT_DEFAULT_CONNECTION_TIMEOUT;
static gdouble writing_timeout = MILTER_SERVER_CONTEXT_DEFAULT_WRITING_TIMEOUT;
static gdouble reading_timeout = MILTER_SERVER_CONTEXT_DEFAULT_READING_TIMEOUT;
static gdouble end_of_message_timeout = MILTER_SERVER_CONTEXT_DEFAULT_END_OF_MESSAGE_TIMEOUT;

#define MILTER_TEST_SERVER_ERROR                                \
    (g_quark_from_static_string("milter-test-server-error-quark"))

typedef enum
{
    MILTER_TEST_SERVER_ERROR_INVALID_HEADER,
    MILTER_TEST_SERVER_ERROR_INVALID_MAIL_ADDRESS,
    MILTER_TEST_SERVER_ERROR_TIMEOUT
} MilterTestServerError;

typedef struct _Message
{
    gchar *envelope_from;
    gchar *original_envelope_from;
    GList *recipients;
    GList *original_recipients;
    MilterHeaders *headers;
    MilterHeaders *original_headers;
    GString *body_string;
    GString *replaced_body_string;
} Message;

typedef struct _ProcessData
{
    MilterEventLoop *loop;
    GTimer *timer;
    gboolean success;
    guint reply_code;
    gchar *reply_extended_code;
    gchar *reply_message;
    gchar *quarantine_reason;
    MilterOption *option;
    MilterHeaders *option_headers;
    gint current_recipient;
    gchar **body_chunks;
    gint current_body_chunk;
    Message *message;
    GError *error;
} ProcessData;

#define RED_COLOR "\033[01;31m"
#define RED_BACK_COLOR "\033[41m"
#define GREEN_COLOR "\033[01;32m"
#define GREEN_BACK_COLOR "\033[01;42m"
#define YELLOW_COLOR "\033[01;33m"
#define BLUE_COLOR "\033[01;34m"
#define BLUE_BACK_COLOR "\033[01;44m"
#define MAGENTA_COLOR "\033[01;35m"
#define CYAN_COLOR "\033[01;36m"
#define WHITE_COLOR "\033[01;37m"
#define NORMAL_COLOR "\033[00m"
#define NO_COLOR ""

static void
remove_recipient (GList **recipients, const gchar *recipient)
{
    GList *node;

    for (node = *recipients; node; node = g_list_next(node)) {
        const gchar *_recipient = node->data;

        if (g_str_equal(_recipient, recipient)) {
            *recipients = g_list_remove_link(*recipients, node);
            return;
        }
    }
}

static void
send_quit (MilterServerContext *context, ProcessData *data)
{
    milter_server_context_quit(context);
    milter_agent_shutdown(MILTER_AGENT(context));
}

static void
send_abort (MilterServerContext *context, ProcessData *data)
{
    milter_server_context_abort(context);
    send_quit(context, data);
}

static void
set_macro (gpointer key, gpointer value, gpointer user_data)
{
    MilterProtocolAgent *agent = user_data;
    GList *symbol;
    MilterCommand command = GPOINTER_TO_UINT(key);
    GList *symbols = value;

    for (symbol = symbols; symbol; symbol = g_list_next(symbols)) {
        milter_protocol_agent_set_macro(agent,
                                        command,
                                        symbol->data, symbol->data);
    }
}

static void
set_macros_for_connect (MilterProtocolAgent *agent)
{
    milter_protocol_agent_set_macros_hash_table(agent, MILTER_COMMAND_CONNECT,
                                                connect_macros);
}

static void
set_macros_for_helo (MilterProtocolAgent *agent)
{
    milter_protocol_agent_set_macros_hash_table(agent, MILTER_COMMAND_HELO,
                                                helo_macros);
}

static void
set_macros_for_envelope_from (MilterProtocolAgent *agent)
{
    milter_protocol_agent_set_macros_hash_table(agent,
                                                MILTER_COMMAND_ENVELOPE_FROM,
                                                envelope_from_macros);
}

static void
set_macros_for_envelope_recipient (MilterProtocolAgent *agent,
                                   const gchar *recipient)
{
    milter_protocol_agent_set_macros_hash_table(agent,
                                                MILTER_COMMAND_ENVELOPE_RECIPIENT,
                                                envelope_recipient_macros);

    milter_protocol_agent_set_macros(agent, MILTER_COMMAND_ENVELOPE_RECIPIENT,
                                     "{rcpt_addr}", recipient,
                                     NULL);
}

static void
set_macros_for_data (MilterProtocolAgent *agent)
{
    milter_protocol_agent_set_macros_hash_table(agent,
                                                MILTER_COMMAND_DATA,
                                                data_macros);
}

static void
set_macros_for_header (MilterProtocolAgent *agent)
{
}

static void
set_macros_for_end_of_header (MilterProtocolAgent *agent)
{
    milter_protocol_agent_set_macros_hash_table(agent,
                                                MILTER_COMMAND_END_OF_HEADER,
                                                end_of_header_macros);
}

static void
set_macros_for_body (MilterProtocolAgent *agent)
{
}

static void
set_macros_for_end_of_message (MilterProtocolAgent *agent)
{
    milter_protocol_agent_set_macros_hash_table(agent,
                                                MILTER_COMMAND_END_OF_MESSAGE,
                                                end_of_message_macros);
}

static void
set_macros_for_unknown (MilterProtocolAgent *agent)
{
}

static gboolean
send_connect (MilterServerContext *context, ProcessData *data)
{
    const gchar *host_name;

    set_macros_for_connect(MILTER_PROTOCOL_AGENT(context));
    if (milter_option_get_step(data->option) & MILTER_STEP_NO_CONNECT)
        return FALSE;

    if (connect_host)
        host_name = connect_host;
    else
        host_name = "mx.example.net";

    if (connect_address) {
        milter_server_context_connect(context,
                                      host_name,
                                      connect_address,
                                      connect_address_length);
    } else {
        struct sockaddr_in address;
        const gchar ip_address[] = "192.168.123.123";
        uint16_t port = 50443;

        address.sin_family = AF_INET;
        address.sin_port = g_htons(port);
        inet_pton(AF_INET, ip_address, &(address.sin_addr));
        milter_server_context_connect(context,
                                      host_name,
                                      (struct sockaddr *)(&address),
                                      sizeof(address));
    }

    return TRUE;
}

static gboolean
send_helo (MilterServerContext *context, ProcessData *data)
{
    set_macros_for_helo(MILTER_PROTOCOL_AGENT(context));
    if (milter_option_get_step(data->option) & MILTER_STEP_NO_HELO)
        return FALSE;

    milter_server_context_helo(context, helo_host);
    return TRUE;
}

static gboolean
send_envelope_from (MilterServerContext *context, ProcessData *data)
{
    milter_server_context_reset_message_related_data(context);
    set_macros_for_envelope_from(MILTER_PROTOCOL_AGENT(context));
    if (milter_option_get_step(data->option) & MILTER_STEP_NO_ENVELOPE_FROM)
        return FALSE;

    milter_server_context_envelope_from(context, envelope_from);
    return TRUE;
}

static gboolean
send_envelope_recipient (MilterServerContext *context, ProcessData *data)
{
    gchar *recipient;

    recipient = recipients[data->current_recipient];
    if (!recipient)
        return FALSE;

    set_macros_for_envelope_recipient(MILTER_PROTOCOL_AGENT(context),
                                      recipient);
    if (milter_option_get_step(data->option) & MILTER_STEP_NO_ENVELOPE_RECIPIENT)
        return FALSE;

    milter_server_context_envelope_recipient(context, recipient);
    data->current_recipient++;
    return TRUE;
}

static gboolean
send_unknown (MilterServerContext *context, ProcessData *data)
{
    if (milter_option_get_step(data->option) & MILTER_STEP_NO_UNKNOWN)
        return FALSE;
    if (!unknown_command)
        return FALSE;
    if (milter_option_get_version(data->option) < 3)
        return FALSE;

    set_macros_for_unknown(MILTER_PROTOCOL_AGENT(context));
    milter_server_context_unknown(context, unknown_command);
    return TRUE;
}

static gboolean
send_data (MilterServerContext *context, ProcessData *data)
{
    set_macros_for_data(MILTER_PROTOCOL_AGENT(context));
    if (milter_option_get_step(data->option) & MILTER_STEP_NO_DATA)
        return FALSE;

    if (milter_option_get_version(data->option) < 4)
        return FALSE;

    milter_server_context_data(context);
    return TRUE;
}

static gboolean
send_header (MilterServerContext *context, ProcessData *data)
{
    MilterHeader *header;

    set_macros_for_header(MILTER_PROTOCOL_AGENT(context));
    if (milter_option_get_step(data->option) & MILTER_STEP_NO_HEADERS)
        return FALSE;
    if (milter_headers_length(data->option_headers) == 0)
        return FALSE;

    header = milter_headers_get_nth_header(data->option_headers, 1);
    milter_server_context_header(context, header->name, header->value);
    milter_headers_remove(data->option_headers, header);
    return TRUE;
}

static gboolean
send_end_of_header (MilterServerContext *context, ProcessData *data)
{
    set_macros_for_end_of_header(MILTER_PROTOCOL_AGENT(context));
    if (milter_option_get_step(data->option) & MILTER_STEP_NO_END_OF_HEADER)
        return FALSE;

    milter_server_context_end_of_header(context);
    return TRUE;
}

static gboolean
send_body (MilterServerContext *context, ProcessData *data)
{
    gchar *body_chunk;

    set_macros_for_body(MILTER_PROTOCOL_AGENT(context));
    if (milter_option_get_step(data->option) & MILTER_STEP_NO_BODY)
        return FALSE;

    body_chunk = data->body_chunks[data->current_body_chunk];
    if (!body_chunk)
        return FALSE;

    milter_server_context_body(context, body_chunk, strlen(body_chunk));
    data->current_body_chunk++;
    return TRUE;
}

static gboolean
send_end_of_message (MilterServerContext *context, ProcessData *data)
{
    set_macros_for_end_of_message(MILTER_PROTOCOL_AGENT(context));
    milter_server_context_end_of_message(context, NULL, 0);
    return TRUE;
}

static void
cb_continue (MilterServerContext *context, gpointer user_data)
{
    ProcessData *data = user_data;

    switch (milter_server_context_get_state(context)) {
    case MILTER_SERVER_CONTEXT_STATE_NEGOTIATE:
        if (send_connect(context, data))
            break;
    case MILTER_SERVER_CONTEXT_STATE_CONNECT:
        if (send_helo(context, data))
            break;
    case MILTER_SERVER_CONTEXT_STATE_HELO:
        if (send_envelope_from(context, data))
            break;
    case MILTER_SERVER_CONTEXT_STATE_ENVELOPE_FROM:
        if (send_envelope_recipient(context, data))
            break;
    case MILTER_SERVER_CONTEXT_STATE_ENVELOPE_RECIPIENT:
        if (send_envelope_recipient(context, data))
            break;
        if (send_unknown(context, data))
            break;
    case MILTER_SERVER_CONTEXT_STATE_UNKNOWN:
        if (send_data(context, data))
            break;
    case MILTER_SERVER_CONTEXT_STATE_DATA:
        if (send_header(context, data))
            break;
    case MILTER_SERVER_CONTEXT_STATE_HEADER:
        if (send_header(context, data))
            break;
        if (send_end_of_header(context, data))
            break;
    case MILTER_SERVER_CONTEXT_STATE_END_OF_HEADER:
        if (send_body(context, data))
            break;
    case MILTER_SERVER_CONTEXT_STATE_BODY:
        if (send_body(context, data))
            break;
        send_end_of_message(context, data);
        break;
    case MILTER_SERVER_CONTEXT_STATE_END_OF_MESSAGE:
        send_quit(context, data);
        break;
    default:
        milter_error("Unknown state.");
        send_abort(context, data);
        break;
    }
}

static void
cb_negotiate_reply (MilterServerContext *context, MilterOption *option,
                    MilterMacrosRequests *macros_requests, gpointer user_data)
{
    ProcessData *data = user_data;

    if (data->option) {
        milter_error("duplicated negotiate");
        send_abort(context, data);
    } else {
        data->option = g_object_ref(option);
        if (macros_requests)
            milter_macros_requests_foreach(macros_requests,
                                           (GHFunc)set_macro, context);
        cb_continue(context, user_data);
    }
}

static void
cb_temporary_failure (MilterServerContext *context, gpointer user_data)
{
    ProcessData *data = user_data;
    MilterServerContextState state;

    state = milter_server_context_get_state(context);

    switch (state) {
    case MILTER_SERVER_CONTEXT_STATE_ENVELOPE_RECIPIENT:
        remove_recipient(&(data->message->recipients),
                         *(recipients + data->current_recipient - 1));
        if (data->message->recipients) {
            cb_continue(context, user_data);
            break;
        }
    default:
        send_abort(context, data);
        data->success = FALSE;
        break;
    }
}

static void
cb_reject (MilterServerContext *context, gpointer user_data)
{
    ProcessData *data = user_data;
    MilterServerContextState state;

    state = milter_server_context_get_state(context);

    switch (state) {
    case MILTER_SERVER_CONTEXT_STATE_ENVELOPE_RECIPIENT:
        remove_recipient(&(data->message->recipients),
                         *(recipients + data->current_recipient - 1));
        if (data->message->recipients) {
            cb_continue(context, user_data);
            break;
        }
    default:
        send_abort(context, data);
        data->success = FALSE;
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
    ProcessData *data = user_data;

    data->reply_code = code;
    if (data->reply_extended_code)
        g_free(data->reply_extended_code);
    data->reply_extended_code = g_strdup(extended_code);
    if (data->reply_message)
        g_free(data->reply_message);
    data->reply_message = g_strdup(message);

    if (400 <= code && code < 500)
        cb_temporary_failure(context, user_data);
    else
        cb_reject(context, user_data);
}

static void
cb_accept (MilterServerContext *context, gpointer user_data)
{
    ProcessData *data = user_data;

    send_abort(context, data);
    data->success = TRUE;
}

static void
cb_discard (MilterServerContext *context, gpointer user_data)
{
    ProcessData *data = user_data;

    send_abort(context, data);
    data->success = FALSE;
}

static void
cb_add_header (MilterServerContext *context,
               const gchar *name, const gchar *value,
               gpointer user_data)
{
    ProcessData *data = user_data;

    milter_headers_add_header(data->message->headers, name, value);
}

static void
cb_insert_header (MilterServerContext *context,
                  guint32 index, const gchar *name, const gchar *value,
                  gpointer user_data)
{
    ProcessData *data = user_data;

    milter_headers_insert_header(data->message->headers, index, name, value);
}

static void
cb_change_header (MilterServerContext *context,
                  const gchar *name, guint32 index, const gchar *value,
                  gpointer user_data)
{
    ProcessData *data = user_data;

    milter_headers_change_header(data->message->headers, name, index, value);
}

static void
cb_delete_header (MilterServerContext *context,
                  const gchar *name, guint32 index,
                  gpointer user_data)
{
    ProcessData *data = user_data;

    milter_headers_delete_header(data->message->headers, name, index);
}

static gchar *
normalize_envelope_address (const gchar *address)
{
    if (g_str_has_prefix(address, "<") &&
        g_str_has_suffix(address, ">")) {
        return g_strdup(address);
    } else {
        return g_strdup_printf("<%s>", address);
    }
}

static void
cb_change_from (MilterServerContext *context,
                const gchar *from, const gchar *parameters,
                gpointer user_data)
{
    ProcessData *data = user_data;

    if (data->message->envelope_from)
        g_free(data->message->envelope_from);
    data->message->envelope_from = normalize_envelope_address(from);
}

static void
cb_add_recipient (MilterServerContext *context,
                  const gchar *recipient, const gchar *parameters,
                  gpointer user_data)
{
    ProcessData *data = user_data;

    data->message->recipients =
        g_list_append(data->message->recipients,
                      normalize_envelope_address(recipient));
}

static void
cb_delete_recipient (MilterServerContext *context, const gchar *recipient,
                     gpointer user_data)
{
    ProcessData *data = user_data;
    gchar *normalized_recipient;

    normalized_recipient = normalize_envelope_address(recipient);
    remove_recipient(&(data->message->recipients), normalized_recipient);
    g_free(normalized_recipient);
}

static void
cb_replace_body (MilterServerContext *context,
                 const gchar *body, gsize body_size,
                 gpointer user_data)
{
    ProcessData *data = user_data;

    g_string_append_len(data->message->replaced_body_string, body, body_size);
}

static void
cb_progress (MilterServerContext *context, gpointer user_data)
{
}

static void
cb_quarantine (MilterServerContext *context, const gchar *reason,
               gpointer user_data)
{
    ProcessData *data = user_data;

    data->quarantine_reason = g_strdup(reason);
}

static void
cb_connection_failure (MilterServerContext *context, gpointer user_data)
{
    ProcessData *data = user_data;

    send_abort(context, data);
}

static void
cb_shutdown (MilterServerContext *context, gpointer user_data)
{
    ProcessData *data = user_data;

    send_abort(context, data);
}

static void
cb_skip (MilterServerContext *context, gpointer user_data)
{
    ProcessData *data = user_data;
    while (data->body_chunks[data->current_body_chunk])
        data->current_body_chunk++;

    cb_continue(context, user_data);
}

static void
cb_error (MilterErrorEmittable *emittable, GError *error, gpointer user_data)
{
    ProcessData *data = user_data;

    data->error = g_error_copy(error);

    milter_agent_shutdown(MILTER_AGENT(emittable));
}

static void
cb_finished (MilterFinishedEmittable *emittable, gpointer user_data)
{
    ProcessData *data = user_data;

    g_timer_stop(data->timer);
    milter_event_loop_quit(data->loop);
}

static void
cb_connection_timeout (MilterServerContext *context, gpointer user_data)
{
    ProcessData *data = user_data;

    data->success = FALSE;
    data->error = g_error_new(MILTER_TEST_SERVER_ERROR,
                              MILTER_TEST_SERVER_ERROR_TIMEOUT,
                              "connection timeout");
}

static void
cb_writing_timeout (MilterServerContext *context, gpointer user_data)
{
    ProcessData *data = user_data;

    data->success = FALSE;
    data->error = g_error_new(MILTER_TEST_SERVER_ERROR,
                              MILTER_TEST_SERVER_ERROR_TIMEOUT,
                              "writing timeout");
}

static void
cb_reading_timeout (MilterServerContext *context, gpointer user_data)
{
    ProcessData *data = user_data;

    data->success = FALSE;
    data->error = g_error_new(MILTER_TEST_SERVER_ERROR,
                              MILTER_TEST_SERVER_ERROR_TIMEOUT,
                              "reading timeout");
    send_abort(context, data);
}

static void
cb_end_of_message_timeout (MilterServerContext *context, gpointer user_data)
{
    ProcessData *data = user_data;

    data->success = FALSE;
    data->error = g_error_new(MILTER_TEST_SERVER_ERROR,
                              MILTER_TEST_SERVER_ERROR_TIMEOUT,
                              "end-of-message timeout");
    send_abort(context, data);
}

static void
cb_state_transited (MilterServerContext *context,
                    MilterServerContextState state,
                    gpointer user_data)
{
    ProcessData *data = user_data;
    MilterStepFlags step = MILTER_STEP_NONE;

    if (data->option)
        step = milter_option_get_step(data->option);

    switch (state) {
    case MILTER_SERVER_CONTEXT_STATE_CONNECT:
        if (!(step & MILTER_STEP_NO_REPLY_CONNECT))
            break;
        if (send_helo(context, data))
            break;
    case MILTER_SERVER_CONTEXT_STATE_HELO:
        if (!(step & MILTER_STEP_NO_REPLY_HELO))
            break;
        if (send_envelope_from(context, data))
            break;
    case MILTER_SERVER_CONTEXT_STATE_ENVELOPE_FROM:
        if (!(step & MILTER_STEP_NO_REPLY_ENVELOPE_FROM))
            break;
        if (send_envelope_recipient(context, data))
            break;
    case MILTER_SERVER_CONTEXT_STATE_ENVELOPE_RECIPIENT:
        if (!(step & MILTER_STEP_NO_REPLY_ENVELOPE_RECIPIENT))
            break;
        if (send_envelope_recipient(context, data))
            break;
        if (send_unknown(context, data))
            break;
        if (send_data(context, data))
            break;
    case MILTER_SERVER_CONTEXT_STATE_UNKNOWN:
        if (!(step & MILTER_STEP_NO_REPLY_UNKNOWN))
            break;
        if (send_data(context, data))
            break;
    case MILTER_SERVER_CONTEXT_STATE_DATA:
        if (!(step & MILTER_STEP_NO_REPLY_DATA))
            break;
        if (send_header(context, data))
            break;
    case MILTER_SERVER_CONTEXT_STATE_HEADER:
        if (!(step & MILTER_STEP_NO_REPLY_HEADER))
            break;
        if (send_header(context, data))
            break;
        if (send_end_of_header(context, data))
            break;
    case MILTER_SERVER_CONTEXT_STATE_END_OF_HEADER:
        if (!(step & MILTER_STEP_NO_REPLY_END_OF_HEADER))
            break;
        if (send_body(context, data))
            break;
    case MILTER_SERVER_CONTEXT_STATE_BODY:
        if (!(step & MILTER_STEP_NO_REPLY_BODY))
            break;
        if (send_body(context, data))
            break;
        if (send_end_of_message(context, data))
            break;
    default:
        break;
    }
}

static void
setup (MilterServerContext *context, ProcessData *data)
{
#define CONNECT(name)                                                   \
    g_signal_connect(context, #name, G_CALLBACK(cb_ ## name), data)

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

    CONNECT(connection_timeout);
    CONNECT(writing_timeout);
    CONNECT(reading_timeout);
    CONNECT(end_of_message_timeout);

    CONNECT(state_transited);

#undef CONNECT
}

static void
negotiate (MilterServerContext *context)
{
    MilterOption *option;

    option = milter_option_new(negotiate_version,
                               MILTER_ACTION_ADD_HEADERS |
                               MILTER_ACTION_CHANGE_BODY |
                               MILTER_ACTION_ADD_ENVELOPE_RECIPIENT |
                               MILTER_ACTION_DELETE_ENVELOPE_RECIPIENT |
                               MILTER_ACTION_CHANGE_HEADERS |
                               MILTER_ACTION_QUARANTINE |
                               MILTER_ACTION_CHANGE_ENVELOPE_FROM |
                               MILTER_ACTION_ADD_ENVELOPE_RECIPIENT_WITH_PARAMETERS |
                               MILTER_ACTION_SET_SYMBOL_LIST,
                               MILTER_STEP_NO_CONNECT |
                               MILTER_STEP_NO_HELO |
                               MILTER_STEP_NO_ENVELOPE_FROM |
                               MILTER_STEP_NO_ENVELOPE_RECIPIENT |
                               MILTER_STEP_NO_BODY |
                               MILTER_STEP_NO_HEADERS |
                               MILTER_STEP_NO_END_OF_HEADER |
                               MILTER_STEP_NO_REPLY_HEADER |
                               MILTER_STEP_NO_UNKNOWN |
                               MILTER_STEP_NO_DATA |
                               MILTER_STEP_SKIP |
                               MILTER_STEP_ENVELOPE_RECIPIENT_REJECTED |
                               MILTER_STEP_NO_REPLY_CONNECT |
                               MILTER_STEP_NO_REPLY_HELO |
                               MILTER_STEP_NO_REPLY_ENVELOPE_FROM |
                               MILTER_STEP_NO_REPLY_ENVELOPE_RECIPIENT |
                               MILTER_STEP_NO_REPLY_DATA |
                               MILTER_STEP_NO_REPLY_UNKNOWN |
                               MILTER_STEP_NO_REPLY_END_OF_HEADER |
                               MILTER_STEP_NO_REPLY_BODY |
                               MILTER_STEP_HEADER_VALUE_WITH_LEADING_SPACE);
    milter_server_context_negotiate(context, option);
    g_object_unref(option);
}

static void
cb_ready (MilterServerContext *context, gpointer user_data)
{
    ProcessData *data = user_data;
    setup(context, data);
    g_timer_start(data->timer);
    negotiate(context);
}

static void
cb_connection_error (MilterErrorEmittable *emittable, GError *error, gpointer user_data)
{
    ProcessData *data = user_data;

    data->success = FALSE;
    data->error = g_error_copy(error);
    milter_event_loop_quit(data->loop);
}

static gboolean
set_name (const gchar *option_name,
          const gchar *value,
          gpointer data,
          GError **error)
{
    g_set_prgname(value);
    return TRUE;
}

static gboolean
print_version (const gchar *option_name,
               const gchar *value,
               gpointer data,
               GError **error)
{
    g_print("%s %s\n", g_get_prgname(), VERSION);
    exit(EXIT_SUCCESS);
    return TRUE;
}

static gboolean
parse_spec_arg (const gchar *option_name,
                const gchar *value,
                gpointer data,
                GError **error)
{
    GError *spec_error = NULL;
    gchar *normalized_value;
    gboolean success;

    if (g_str_has_prefix(value, "/"))
        normalized_value = g_strdup_printf("unix:%s", value);
    else
        normalized_value = g_strdup(value);

    success = milter_connection_parse_spec(normalized_value,
                                           NULL, NULL, NULL,
                                           &spec_error);
    if (success) {
        spec = normalized_value;
    } else {
        g_set_error(error,
                    G_OPTION_ERROR,
                    G_OPTION_ERROR_BAD_VALUE,
                    "%s", spec_error->message);
        g_error_free(spec_error);
        g_free(normalized_value);
    }

    return success;
}

static gboolean
parse_connect_address_arg (const gchar *option_name,
                           const gchar *value,
                           gpointer data,
                           GError **error)
{
    GError *spec_error = NULL;
    gboolean success;

    success = milter_connection_parse_spec(value,
                                           NULL,
                                           &connect_address,
                                           &connect_address_length,
                                           &spec_error);
    if (!success) {
        g_set_error(error,
                    G_OPTION_ERROR,
                    G_OPTION_ERROR_BAD_VALUE,
                    "%s", spec_error->message);
        g_error_free(spec_error);
    }

    return success;
}

static gboolean
parse_macro_arg (GHashTable *macros,
                 const gchar *option_name,
                 const gchar *value,
                 gpointer data,
                 GError **error)
{
    gboolean success = TRUE;
    gchar **macro;

    macro = g_strsplit(value, ":", 2);
    if (!macro[1]) {
        g_set_error(error,
                    G_OPTION_ERROR,
                    G_OPTION_ERROR_BAD_VALUE,
                    "macro name and value delimiter ':' doesn't exist: <%s>",
                    value);
        success = FALSE;
    } else {
        g_hash_table_insert(macros, g_strdup(macro[0]), g_strdup(macro[1]));
    }
    g_strfreev(macro);

    return success;
}

static gboolean
parse_connect_macro_arg (const gchar *option_name,
                         const gchar *value,
                         gpointer data,
                         GError **error)
{
    return parse_macro_arg(connect_macros, option_name, value, data, error);
}

static gboolean
parse_helo_macro_arg (const gchar *option_name,
                      const gchar *value,
                      gpointer data,
                      GError **error)
{
    return parse_macro_arg(helo_macros, option_name, value, data, error);
}

static gboolean
parse_envelope_from_macro_arg (const gchar *option_name,
                               const gchar *value,
                               gpointer data,
                               GError **error)
{
    return parse_macro_arg(envelope_from_macros, option_name, value,
                           data, error);
}

static gboolean
parse_envelope_recipient_macro_arg (const gchar *option_name,
                                    const gchar *value,
                                    gpointer data,
                                    GError **error)
{
    return parse_macro_arg(envelope_recipient_macros, option_name, value,
                           data, error);
}

static gboolean
parse_data_macro_arg (const gchar *option_name,
                      const gchar *value,
                      gpointer data,
                      GError **error)
{
    return parse_macro_arg(data_macros, option_name, value, data, error);
}

static gboolean
parse_header_arg (const gchar *option_name,
                  const gchar *value,
                  gpointer data,
                  GError **error)
{
    gchar **strings;

    strings = g_strsplit(value, ":", 2);
    if (g_strv_length(strings) != 2) {
        g_set_error(error,
                    G_OPTION_ERROR,
                    G_OPTION_ERROR_BAD_VALUE,
                    _("headers option should be 'NAME:VALUE'"));
        g_strfreev(strings);
        return FALSE;
    }

    milter_headers_add_header(option_headers, strings[0], strings[1]);

    g_strfreev(strings);

    return TRUE;
}

static gboolean
parse_end_of_header_macro_arg (const gchar *option_name,
                               const gchar *value,
                               gpointer data,
                               GError **error)
{
    return parse_macro_arg(end_of_header_macros, option_name, value,
                           data, error);
}

static gboolean
parse_end_of_message_macro_arg (const gchar *option_name,
                                const gchar *value,
                                gpointer data,
                                GError **error)
{
    return parse_macro_arg(end_of_message_macros, option_name, value,
                           data, error);
}

static gboolean
set_envelope_from (const gchar *from)
{
    if (envelope_from)
        g_free(envelope_from);
    envelope_from = g_strdup(from);

    return TRUE;
}

static gchar *
extract_path_from_mail_address (const gchar *mail_address, GError **error)
{
    const gchar *path;

    path = strstr(mail_address, "<");
    if (path) {
        const gchar *end_of_path;

        end_of_path = strstr(path, ">");
        if (end_of_path) {
            return g_strndup(path, end_of_path + 1 - path);
        } else {
            g_set_error(error,
                        MILTER_TEST_SERVER_ERROR,
                        MILTER_TEST_SERVER_ERROR_INVALID_MAIL_ADDRESS,
                        "invalid mail address: <%s>", mail_address);
            return NULL;
        }
    } else {
        GString *extracted_path;
        const gchar *end_of_path;

        path = mail_address;
        while (path[0] && path[0] == ' ')
            path++;
        end_of_path = path;
        while (end_of_path[0] && end_of_path[0] != ' ')
            end_of_path++;
        /* FIXME: should check any garbages after end_of_path. */
        extracted_path = g_string_new("<");
        g_string_append_len(extracted_path, path, end_of_path - path);
        g_string_append(extracted_path, ">");
        return g_string_free(extracted_path, FALSE);
    }
}

static gboolean
parse_header (const gchar *name, const gchar *value,
              GList **recipient_list, GError **error)
{
    if (strcmp(name, "From") == 0) {
        gchar *reverse_path;

        reverse_path = extract_path_from_mail_address(value, error);
        if (!reverse_path)
            return FALSE;
        set_envelope_from(reverse_path);
        g_free(reverse_path);
    } else if (strcmp(name, "To") == 0) {
        gchar *forward_path;

        forward_path = extract_path_from_mail_address(value, error);
        if (!forward_path)
            return FALSE;
        *recipient_list = g_list_append(*recipient_list, forward_path);
    }

    return TRUE;
}

static gboolean
is_header (const gchar *line)
{
    if (!g_ascii_isalnum(line[0]))
        return FALSE;

    if (!strstr(line, ":"))
        return FALSE;
    return TRUE;
}

static void
append_header (MilterHeaders *headers, const char *header_line)
{
    gchar **header_and_value;

    header_and_value = g_strsplit(header_line, ":", 2);
    milter_headers_append_header(headers,
                                 header_and_value[0],
                                 g_strchug(header_and_value[1]));
    g_strfreev(header_and_value);

}

static void
append_header_value (MilterHeaders *headers, const gchar *value,
                     const gchar *new_line)
{
    MilterHeader *last_header;
    guint last;
    gchar *old_value;

    last = milter_headers_length(headers);
    last_header = milter_headers_get_nth_header(headers, last);

    old_value = last_header->value;
    last_header->value = g_strdup_printf("%s%s%s",
                                         last_header->value,
                                         new_line,
                                         value);
    g_free(old_value);
}

static gboolean
parse_mail_contents_header_part_collect_headers (gchar ***lines_,
                                                 MilterHeaders *headers,
                                                 GError **error)
{
    gchar **lines = *lines_;

    for (; *lines; lines++) {
        gchar *line = *lines;
        gsize line_length;
        gboolean have_cr = FALSE;

        line_length = strlen(line);
        if (line_length > 0 && line[line_length - 1] == '\r') {
            line[line_length - 1] = '\0';
            have_cr = TRUE;
        }

        if (line[0] == '\0') {
            lines++;
            break;
        } else if (is_header(line)) {
            append_header(headers, line);
        } else if (g_ascii_isspace(line[0])) {
            const gchar *new_line;
            if (have_cr) {
                new_line = "\r\n";
            } else {
                new_line = "\n";
            }
            append_header_value(headers, line, new_line);
        } else {
            g_set_error(error,
                        MILTER_TEST_SERVER_ERROR,
                        MILTER_TEST_SERVER_ERROR_INVALID_HEADER,
                        "invalid header: <%s>",
                        line);
            return FALSE;
        }
    }
    *lines_ = lines;

    return TRUE;
}

static gboolean
parse_mail_contents_header_part_parse_headers (MilterHeaders *headers,
                                               GError **error)
{
    const GList *header_list;
    GList *recipient_list = NULL;

    header_list = milter_headers_get_list(headers);
    for (; header_list; header_list = g_list_next(header_list)) {
        MilterHeader *header = header_list->data;
        if (!parse_header(header->name, header->value,
                          &recipient_list, error)) {
            return FALSE;
        }
    }

    if (recipient_list) {
        gint i, length;
        GList *node;

        length = g_list_length(recipient_list);
        recipients = g_new0(gchar *, length + 1);
        for (i = 0, node = recipient_list; node; i++, node = g_list_next(node)) {
            recipients[i] = node->data;
        }
        recipients[length] = NULL;
        g_list_free(recipient_list);
    }

    return TRUE;
}

static gboolean
parse_mail_contents_header_part (gchar ***lines_, GError **error)
{
    MilterHeaders *headers;
    const GList *header_list;

    headers = milter_headers_new();
    if (!parse_mail_contents_header_part_collect_headers(lines_,
                                                         headers,
                                                         error)) {
        g_object_unref(headers);
        return FALSE;
    }

    if (!parse_mail_contents_header_part_parse_headers(headers, error)) {
        g_object_unref(headers);
        return FALSE;
    }

    header_list = milter_headers_get_list(headers);
    for (; header_list; header_list = g_list_next(header_list)) {
        MilterHeader *header = header_list->data;
        milter_headers_append_header(option_headers,
                                     header->name, header->value);
    }

    g_object_unref(headers);

    return TRUE;
}

static gboolean
parse_mail_contents_body_part (gchar ***lines_, GError **error)
{
    gchar **lines = *lines_;
    GString *body_string;
    GPtrArray *chunks;

    chunks = g_ptr_array_new();
    body_string = g_string_new(NULL);
    for (; *lines; lines++) {
        gchar *line = *lines;
        gsize chunk_length;
        gsize line_length;

        line_length = strlen(line);
        chunk_length = body_string->len + line_length + strlen("\n");
        if (chunk_length > MILTER_CHUNK_SIZE) {
            g_ptr_array_add(chunks, g_strdup(body_string->str));
            g_string_truncate(body_string, 0);
        }
        g_string_append(body_string, line);
        g_string_append(body_string, "\n");
    }
    *lines_ = lines;

    if (body_string->len > 0) {
        gsize last_new_line_length;

        last_new_line_length = strlen("\n");
        if (body_string->str[body_string->len] == '\r') {
            last_new_line_length += strlen("\r");
        }
        g_ptr_array_add(chunks,
                        g_strndup(body_string->str,
                                  body_string->len - last_new_line_length));
    }
    g_string_free(body_string, TRUE);
    g_ptr_array_add(chunks, NULL);
    body_chunks = (gchar **)g_ptr_array_free(chunks, FALSE);

    return TRUE;
}

static gboolean
parse_mail_contents (const gchar *contents, GError **error)
{
    gchar **lines, **first_lines;

    lines = g_strsplit(contents, "\n", -1);
    first_lines = lines;

    /* Ignore mbox separation 'From ' mark. */
    if (g_str_has_prefix(*lines, "From "))
        lines++;

    if (!parse_mail_contents_header_part(&lines, error)) {
        g_strfreev(first_lines);
        return FALSE;
    }

    if (!parse_mail_contents_body_part(&lines, error)) {
        g_strfreev(first_lines);
        return FALSE;
    }

    g_strfreev(first_lines);

    return TRUE;
}

static gboolean
parse_mail_file_arg (const gchar *option_name,
                     const gchar *value,
                     gpointer data,
                     GError **error)
{
    gchar *contents = NULL;
    gsize length;
    GError *internal_error = NULL;
    GIOChannel *io_channel;

    if (g_str_equal(value, "-")) {
        io_channel = g_io_channel_unix_new(STDIN_FILENO);
    } else {
        if (!g_file_test(value, G_FILE_TEST_EXISTS)) {
            g_set_error(error,
                    G_OPTION_ERROR,
                    G_OPTION_ERROR_BAD_VALUE,
                    _("%s does not exist."), value);
            return FALSE;
        }
        io_channel = g_io_channel_new_file(value, "r", &internal_error);
        if (!io_channel) {
            g_set_error(error,
                        G_OPTION_ERROR,
                        G_OPTION_ERROR_FAILED,
                        _("Loading from %s failed.: %s"),
                        value, internal_error->message);
            g_error_free(internal_error);
            return FALSE;
        }
    }

    g_io_channel_set_encoding(io_channel, NULL, NULL);
    if (g_io_channel_read_to_end(io_channel, &contents, &length, &internal_error) != G_IO_STATUS_NORMAL) {
        g_set_error(error,
                    G_OPTION_ERROR,
                    G_OPTION_ERROR_FAILED,
                    _("Loading from %s failed.: %s"),
                    value, internal_error->message);
        g_error_free(internal_error);
        g_io_channel_unref(io_channel);
        return FALSE;
    }
    g_io_channel_unref(io_channel);

    if (!parse_mail_contents(contents, &internal_error)) {
        g_set_error(error,
                    G_OPTION_ERROR,
                    G_OPTION_ERROR_FAILED,
                    "%s", internal_error->message);
        g_error_free(internal_error);
        return FALSE;
    }

    g_free(contents);
    return TRUE;
}

static gboolean
parse_color_arg (const gchar *option_name, const gchar *value,
                 gpointer data, GError **error)
{
    if (value == NULL ||
        g_utf8_collate(value, "yes") == 0 ||
        g_utf8_collate(value, "true") == 0) {
        use_color = TRUE;
    } else if (g_utf8_collate(value, "no") == 0 ||
               g_utf8_collate(value, "false") == 0) {
        use_color = FALSE;
    } else if (g_utf8_collate(value, "auto") == 0) {
        use_color = milter_utils_guess_console_color_usability();
    } else {
        g_set_error(error,
                    G_OPTION_ERROR,
                    G_OPTION_ERROR_BAD_VALUE,
                    _("Invalid color value: %s"), value);
        return FALSE;
    }

    return TRUE;
}

static const GOptionEntry option_entries[] =
{
    {"name", 0, 0, G_OPTION_ARG_CALLBACK, set_name,
     N_("Use NAME as the milter server name"), "NAME"},
    {"connection-spec", 's', 0, G_OPTION_ARG_CALLBACK, parse_spec_arg,
     N_("The spec of client socket. "
        "(unix:PATH|inet:PORT[@HOST]|inet6:PORT[@HOST])"),
     "SPEC"},
    {"negotiate-version", 0, 0, G_OPTION_ARG_INT, &negotiate_version,
     N_("Use VERSION as milter protocol version on negotiate. "
        "(" G_STRINGIFY(DEFAULT_NEGOTIATE_VERSION) ")"),
     "VERSION"},
    {"connect-host", 0, 0, G_OPTION_ARG_STRING, &connect_host,
     N_("Use HOST as host name on connect"), "HOST"},
    {"connect-address", 0, 0, G_OPTION_ARG_CALLBACK, parse_connect_address_arg,
     N_("Use SPEC for address on connect. "
        "(unix:PATH|inet:PORT[@HOST]|inet6:PORT[@HOST])"),
     "SPEC"},
    {"connect-macro", 0, 0, G_OPTION_ARG_CALLBACK, parse_connect_macro_arg,
     N_("Add a macro that has NAME name and VALUE value on xxfi_connect(). "
        "To add N macros, use --connect-macro option N times."),
     "NAME:VALUE"},
    {"helo-fqdn", 0, 0, G_OPTION_ARG_STRING, &helo_host,
     N_("Use FQDN for HELO/EHLO command"), "FQDN"},
    {"helo-macro", 0, 0, G_OPTION_ARG_CALLBACK, parse_helo_macro_arg,
     N_("Add a macro that has NAME name and VALUE value on xxfi_helo(). "
        "To add N macros, use --helo-macro option N times."),
     "NAME:VALUE"},
    {"from", 0, G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_STRING, &envelope_from,
     N_("Use FROM as a sender address. "
         "(Deprecated. Use --envelope-from instead.)"), "FROM"},
    {"envelope-from", 'f', 0, G_OPTION_ARG_STRING, &envelope_from,
     N_("Use FROM as a sender address"), "FROM"},
    {"envelope-from-macro", 0, 0, G_OPTION_ARG_CALLBACK,
     parse_envelope_from_macro_arg,
     N_("Add a macro that has NAME name and VALUE value on xxfi_envfrom(). "
        "To add N macros, use --envelope-from-macro option N times."),
     "NAME:VALUE"},
    {"recipient", 0, G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_STRING_ARRAY,
     &recipients,
     N_("Add a recipient RECIPIENT. "
        "To add N recipients, use --recipient option N times. "
        "(Deprecated. Use --envelope-recipient instead."),
     "RECIPIENT"},
    {"envelope-recipient", 'r', 0, G_OPTION_ARG_STRING_ARRAY, &recipients,
     N_("Add a recipient RECIPIENT. "
        "To add N recipients, use --envelope-recipient option N times."),
     "RECIPIENT"},
    {"envelope-recipient-macro", 0, 0, G_OPTION_ARG_CALLBACK,
     parse_envelope_recipient_macro_arg,
     N_("Add a macro that has NAME name and VALUE value on xxfi_envrcpt(). "
        "To add N macros, use --envelope-recipient-macro option N times."),
     "NAME:VALUE"},
    {"data-macro", 0, 0, G_OPTION_ARG_CALLBACK, parse_data_macro_arg,
     N_("Add a macro that has NAME name and VALUE value on xxfi_data(). "
        "To add N macros, use --data-macro option N times."),
     "NAME:VALUE"},
    {"header", 'h', 0, G_OPTION_ARG_CALLBACK, parse_header_arg,
     N_("Add a header. To add n headers, use --header option n times."),
     "NAME:VALUE"},
    {"end-of-header-macro", 0, 0, G_OPTION_ARG_CALLBACK,
     parse_end_of_header_macro_arg,
     N_("Add a macro that has NAME name and VALUE value on xxfi_eoh(). "
        "To add N macros, use --end-of-header-macro option N times."),
     "NAME:VALUE"},
    {"body", 'b', 0, G_OPTION_ARG_STRING_ARRAY, &body_chunks,
     N_("Add a body chunk. To add n body chunks, use --body option n times."),
     "CHUNK"},
    {"end-of-message-macro", 0, 0, G_OPTION_ARG_CALLBACK,
     parse_end_of_message_macro_arg,
     N_("Add a macro that has NAME name and VALUE value on xxfi_eom(). "
        "To add N macros, use --end-of-message-macro option N times."),
     "NAME:VALUE"},
    {"unknown", 0, 0, G_OPTION_ARG_STRING, &unknown_command,
     N_("Use COMMAND for unknown SMTP command."), "COMMAND"},
    {"authenticated-name", 0, 0, G_OPTION_ARG_STRING, &authenticated_name,
     N_("Use NAME for authenticated SASL login name. "
        "(This is used for {auth_authen} macro value.)"),
     "NAME"},
    {"authenticated-type", 0, 0, G_OPTION_ARG_STRING, &authenticated_type,
     N_("Use TYPE for authenticated SASL login method. "
        "(This is used for {auth_type} macro value.)"), "TYPE"},
    {"authenticated-author", 0, 0, G_OPTION_ARG_STRING, &authenticated_author,
     N_("Use AUTHOR for authenticated SASL sender. "
        "(This is used for {auth_author} macro value.)"), "AUTHOR"},
    {"mail-file", 'm', 0, G_OPTION_ARG_CALLBACK, parse_mail_file_arg,
     N_("Use mail placed at PATH as mail content."), "PATH"},
    {"output-message", 0, 0, G_OPTION_ARG_NONE, &output_message,
     N_("Output modified message"), NULL},
    {"color", 0, G_OPTION_FLAG_OPTIONAL_ARG,
     G_OPTION_ARG_CALLBACK, parse_color_arg,
     N_("Output modified message with colors"), "[yes|true|no|false|auto]"},
    {"connection-timeout", 0, 0, G_OPTION_ARG_DOUBLE, &connection_timeout,
     N_("Timeout after SECONDS seconds on connecting to a milter."), "SECONDS"},
    {"reading-timeout", 0, 0, G_OPTION_ARG_DOUBLE, &reading_timeout,
     N_("Timeout after SECONDS seconds on reading a command."), "SECONDS"},
    {"writing-timeout", 0, 0, G_OPTION_ARG_DOUBLE, &writing_timeout,
     N_("Timeout after SECONDS seconds on writing a command."), "SECONDS"},
    {"end-of-message-timeout", 0, 0, G_OPTION_ARG_DOUBLE, &writing_timeout,
     N_("Timeout after SECONDS seconds on end-of-message command."), "SECONDS"},
    {"threads", 't', 0, G_OPTION_ARG_INT, &n_threads,
     N_("Create N threads."), "N"},
    {"verbose", 'v', 0, G_OPTION_ARG_NONE, &verbose,
     N_("Be verbose"), NULL},
    {"version", 0, G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK, print_version,
     N_("Show version"), NULL},
    {NULL}
};

static Message *
message_new (void)
{
    Message *message;
    gint i;

    message = g_new0(Message, 1);
    message->envelope_from = g_strdup(envelope_from);
    message->original_envelope_from = g_strdup(envelope_from);
    message->recipients = NULL;
    message->original_recipients = NULL;
    for (i = 0; i < g_strv_length(recipients); i++) {
        message->recipients = \
            g_list_append(message->recipients, g_strdup(recipients[i]));
        message->original_recipients = \
            g_list_append(message->original_recipients, g_strdup(recipients[i]));
    }
    message->headers = milter_headers_copy(option_headers);
    message->original_headers = milter_headers_copy(option_headers);

    message->body_string = g_string_new(NULL);
    for (i = 0; i < g_strv_length(body_chunks); i++)
        g_string_append(message->body_string, g_strdup(body_chunks[i]));
    message->replaced_body_string = g_string_new(NULL);

    return message;
}

static void
free_message (Message *message)
{
    if (message->envelope_from)
        g_free(message->envelope_from);
    if (message->original_envelope_from)
        g_free(message->original_envelope_from);
    if (message->recipients) {
        g_list_foreach(message->recipients, (GFunc)g_free, NULL);
        g_list_free(message->recipients);
    }
    if (message->original_recipients) {
        g_list_foreach(message->original_recipients, (GFunc)g_free, NULL);
        g_list_free(message->original_recipients);
    }
    if (message->headers)
        g_object_unref(message->headers);
    if (message->original_headers)
        g_object_unref(message->original_headers);
    if (message->body_string)
        g_string_free(message->body_string, TRUE);

    g_free(message);
}

static void
init_process_data (ProcessData *data)
{
    data->loop = milter_glib_event_loop_new(NULL);
    data->timer = g_timer_new();
    data->success = TRUE;
    data->quarantine_reason = NULL;
    data->option = NULL;
    data->option_headers = milter_headers_copy(option_headers);
    data->body_chunks = g_strdupv(body_chunks);
    data->current_body_chunk = 0;
    data->current_recipient = 0;
    data->reply_code = 0;
    data->reply_extended_code = NULL;
    data->reply_message = NULL;
    data->message = message_new();
    data->error = NULL;
}

static void
free_process_data (ProcessData *data)
{
    if (data->loop)
        g_object_unref(data->loop);
    if (data->timer)
        g_timer_destroy(data->timer);
    if (data->option)
        g_object_unref(data->option);
    if (data->option_headers)
        g_object_unref(data->option_headers);
    if (data->body_chunks)
        g_strfreev(data->body_chunks);
    if (data->reply_extended_code)
        g_free(data->reply_extended_code);
    if (data->reply_message)
        g_free(data->reply_message);
    if (data->quarantine_reason)
        g_free(data->quarantine_reason);
    if (data->message)
        free_message(data->message);
    if (data->error)
        g_error_free(data->error);
}

static void
add_macro_values (GHashTable *macros, const gchar *name, ...)
{
    va_list args;

    va_start(args, name);
    while (name) {
        const gchar *value;

        value = va_arg(args, const gchar *);
        g_hash_table_insert(macros, g_strdup(name), g_strdup(value));

        name = va_arg(args, const gchar *);
    }
    va_end(args);
}

static GHashTable *
macros_new (void)
{
    return g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
}

static gboolean
pre_option_parse (GOptionContext *option_context,
                  GOptionGroup *option_group,
                  gpointer data,
                  GError **error)
{
    option_headers = milter_headers_new();
    use_color = milter_utils_guess_console_color_usability();

    connect_macros = macros_new();
    helo_macros = macros_new();
    envelope_from_macros = macros_new();
    envelope_recipient_macros = macros_new();
    data_macros = macros_new();
    end_of_header_macros = macros_new();
    end_of_message_macros = macros_new();

    return TRUE;
}

static void
apply_options_to_connect_macros (void)
{
    add_macro_values(connect_macros,
                     "{daemon_name}", g_get_prgname(),
                     "{if_name}", "localhost",
                     "{if_addr}", "127.0.0.1",
                     "j", "mail.example.com",
                     NULL);
}

static void
apply_options_to_helo_macros (void)
{
    add_macro_values(helo_macros,
                     "{tls_version}", "0",
                     "{cipher}", "0",
                     "{cipher_bits}", "0",
                     "{cert_subject}", "cert_subject",
                     "{cert_issuer}", "cert_issuer",
                     NULL);
}

static void
apply_options_to_envelope_from_macros (void)
{
    add_macro_values(envelope_from_macros,
                     "i", "i",
                     "{mail_mailer}", "mail_mailer",
                     "{mail_host}", "mail_host",
                     "{mail_addr}", "mail_addr",
                     NULL);

    if (authenticated_name)
        add_macro_values(envelope_from_macros,
                         "{auth_authen}", authenticated_name,
                         NULL);
    if (authenticated_type)
        add_macro_values(envelope_from_macros,
                         "{auth_type}", authenticated_type,
                         NULL);
    if (authenticated_author)
        add_macro_values(envelope_from_macros,
                         "{auth_author}", authenticated_author,
                         NULL);
}

static void
apply_options_to_envelope_recipient_macros (void)
{
    add_macro_values(envelope_recipient_macros,
                     "{rcpt_mailer}", "rcpt_mailer",
                     "{rcpt_host}", "rcpt_host",
                     NULL);
}

static void
apply_options_to_data_macros (void)
{
}

static void
apply_options_to_end_of_header_macros (void)
{
}

static void
apply_options_to_end_of_message_macros (void)
{
    add_macro_values(end_of_message_macros,
                     "i", "message-id",
                     NULL);
}

static void
apply_options_to_macros (void)
{
    apply_options_to_connect_macros();
    apply_options_to_helo_macros();
    apply_options_to_envelope_from_macros();
    apply_options_to_envelope_recipient_macros();
    apply_options_to_data_macros();
    apply_options_to_end_of_header_macros();
    apply_options_to_end_of_message_macros();
}

static gboolean
post_option_parse (GOptionContext *option_context,
                   GOptionGroup *option_group,
                   gpointer data,
                   GError **error)
{
    guint i, n_recipients;
    gchar **normalized_recipients;

    if (!helo_host)
        helo_host = g_strdup("delian");

    if (!envelope_from)
        envelope_from = "sender@example.com";
    envelope_from = normalize_envelope_address(envelope_from);

    if (!recipients)
        recipients = g_strsplit("receiver@example.org", ",", -1);
    n_recipients = g_strv_length(recipients);
    normalized_recipients = g_new0(gchar *, n_recipients + 1);
    for (i = 0; i < n_recipients; i++) {
        normalized_recipients[i] = normalize_envelope_address(recipients[i]);
    }
    normalized_recipients[i] = NULL;
    g_strfreev(recipients);
    recipients = normalized_recipients;

    if (!milter_headers_lookup_by_name(option_headers, "From"))
        milter_headers_append_header(option_headers, "From", envelope_from);

    if (!milter_headers_lookup_by_name(option_headers, "To"))
        milter_headers_append_header(option_headers, "To", recipients[0]);

    if (!body_chunks) {
        const gchar body[] =
            "La de da de da 1.\n"
            "La de da de da 2.\n"
            "La de da de da 3.\n"
            "La de da de da 4.";
        body_chunks = g_strsplit(body, ",", -1);
    }

    apply_options_to_macros();

    return TRUE;
}

static void
free_option_values (void)
{
    if (option_headers)
        g_object_unref(option_headers);

    if (connect_macros)
        g_hash_table_unref(connect_macros);
    if (helo_macros)
        g_hash_table_unref(helo_macros);
    if (envelope_from_macros)
        g_hash_table_unref(envelope_from_macros);
    if (envelope_recipient_macros)
        g_hash_table_unref(envelope_recipient_macros);
    if (data_macros)
        g_hash_table_unref(data_macros);
    if (end_of_header_macros)
        g_hash_table_unref(end_of_header_macros);
    if (end_of_message_macros)
        g_hash_table_unref(end_of_message_macros);
}

static void
print_envelope_item (const gchar *prefix, const gchar *label,
                     const gchar *address)
{
    const gchar *color_starter = NO_COLOR;
    const gchar *color_finisher = NO_COLOR;

    if (use_color) {
        if (g_str_has_prefix(prefix, "+")) {
            color_starter = GREEN_COLOR;
            color_finisher = NORMAL_COLOR;
        } else if (g_str_has_prefix(prefix, "-")) {
            color_starter = RED_COLOR;
            color_finisher = NORMAL_COLOR;
        };
    }

    g_print("%s%s %s:%s%s\n",
            color_starter,
            prefix, label, address,
            color_finisher);
}

static void
print_envelope_from (Message *message)
{
    const gchar *label = "MAIL FROM";
    const gchar *original_envelope_from = message->original_envelope_from;
    const gchar *result_envelope_from = message->envelope_from;

    if (milter_utils_strcmp0(original_envelope_from,
                             result_envelope_from) == 0) {
        print_envelope_item(" ", label, original_envelope_from);
    } else {
        print_envelope_item("-", label, original_envelope_from);
        print_envelope_item("+", label, result_envelope_from);
    }
}

static void
print_envelope_recipients (Message *message)
{
    const gchar *label = "RCPT TO";
    const GList *original_node, *result_node;

    original_node = message->original_recipients;
    result_node = message->recipients;
    while (original_node && result_node) {
        const gchar *original_recipient = original_node->data;
        const gchar *result_recipient = result_node->data;

        if (milter_utils_strcmp0(original_recipient, result_recipient) == 0) {
            print_envelope_item(" ", label, original_recipient);
            result_node = g_list_next(result_node);
            original_node = g_list_next(original_node);
        } else if (milter_utils_strcmp0(original_recipient, result_recipient) != 0) {
            print_envelope_item("-", label, original_recipient);
            print_envelope_item("+", label, result_recipient);
            result_node = g_list_next(result_node);
            original_node = g_list_next(original_node);
        } else if (!g_list_find_custom((GList*)result_node,
                                       original_recipient,
                                       (GCompareFunc)milter_utils_strcmp0)){
            print_envelope_item("-", label, original_recipient);
            original_node = g_list_next(original_node);
        } else {
            print_envelope_item("+", label, result_recipient);
            result_node = g_list_next(result_node);
        }
    }

    for (; original_node; original_node = g_list_next(original_node)) {
        const gchar *original_recipient = original_node->data;
        print_envelope_item("-", label, original_recipient);
    }

    for (; result_node; result_node = g_list_next(result_node)) {
        const gchar *result_recipient = result_node->data;
        print_envelope_item("+", label, result_recipient);
    }
}

static void
print_envelope (Message *message)
{
    print_envelope_from(message);
    print_envelope_recipients(message);
}

static void
print_header (const gchar *prefix, MilterHeader *header)
{
    gchar **value_lines, **first_line;
    const gchar *color_starter = NO_COLOR;
    const gchar *color_finisher = NO_COLOR;

    if (use_color) {
        if (g_str_has_prefix(prefix, "+")) {
            color_starter = GREEN_COLOR;
            color_finisher = NORMAL_COLOR;
        } else if (g_str_has_prefix(prefix, "-")) {
            color_starter = RED_COLOR;
            color_finisher = NORMAL_COLOR;
        }
    }

    value_lines = g_strsplit(header->value, "\n", -1);
    first_line = value_lines;

    if (!*value_lines) {
        g_print("%s%s %s:%s\n",
                color_starter,
                prefix, header->name,
                color_finisher);
        g_strfreev(value_lines);
        return;
    }

    g_print("%s%s %s: %s%s\n",
            color_starter,
            prefix, header->name, *value_lines,
            color_finisher);
    value_lines++;

    while (*value_lines) {
        g_print("%s%s %s%s\n",
                color_starter,
                prefix, *value_lines,
                color_finisher);
        value_lines++;
    }

    g_strfreev(first_line);
}

static void
print_headers (Message *message)
{
    const GList *node, *result_node;

    result_node = milter_headers_get_list(message->headers);

    node = milter_headers_get_list(message->original_headers);
    while (node && result_node) {
        MilterHeader *original_header = node->data;
        MilterHeader *result_header = result_node->data;

        if (milter_header_equal(original_header, result_header)) {
            print_header(" ", original_header);
            result_node = g_list_next(result_node);
            node = g_list_next(node);
        } else if (!g_list_find_custom((GList*)node,
                                       result_header,
                                       (GCompareFunc)milter_header_compare)) {
            print_header("+", result_header);
            result_node = g_list_next(result_node);
        } else if (!g_list_find_custom((GList*)result_node,
                                       original_header,
                                       (GCompareFunc)milter_header_compare)) {
            print_header("-", original_header);
            node = g_list_next(node);
        } else if (g_str_equal(original_header->name, result_header->name)) {
            print_header("-", original_header);
            print_header("+", result_header);
            result_node = g_list_next(result_node);
            node = g_list_next(node);
        } else {
            print_header("+", result_header);
            result_node = g_list_next(result_node);
        }
    }

    while (node) {
        MilterHeader *header = node->data;
        print_header("-", header);
        node = g_list_next(node);
    }

    for (node = result_node; node; node = g_list_next(node)) {
        MilterHeader *header = node->data;
        print_header("+", header);
    }
}

static gchar *
get_charset (const gchar *value)
{
    gchar *end_pos, *pos;

    pos = strstr(value, "charset=");
    if (!pos)
        return NULL;

    pos += strlen("charset=");

    if (pos[0] == '"' || pos[0] == '\'') {
        end_pos = strchr(pos + 1, pos[0]);
        if (!end_pos)
            return NULL;
        return g_strndup(pos + 1, end_pos - pos - 1);
    }

    end_pos = strchr(pos, ';');
    if (!end_pos)
        return g_strdup(pos);

    return g_strndup(pos, end_pos - pos);
}

static gchar *
get_message_charset (Message *message)
{
    MilterHeader *header;

    header = milter_headers_lookup_by_name(message->headers,
                                           "Content-Type");
    if (!header)
        return NULL;

    return get_charset(header->value);
}

static void
print_body (Message *message)
{
    gchar *charset = NULL;
    gchar *body_string;
    gssize body_size;

    if (message->replaced_body_string->len > 0) {
        body_string = message->replaced_body_string->str;
        body_size = message->replaced_body_string->len;
    } else {
        body_string = message->body_string->str;
        body_size = message->body_string->len;
    }

    charset = get_message_charset(message);
    if (charset) {
        gchar *translated_body;
        gsize bytes_read, bytes_written;
        GError *error = NULL;

        translated_body = g_convert(body_string,
                                    body_size,
                                    "UTF-8",
                                    charset,
                                    &bytes_read,
                                    &bytes_written,
                                    &error);
        if (error) {
            g_print("Error:---------------------------------\n");
            g_print("%s\n", error->message);
            g_print("---------------------------------------\n");
            g_error_free(error);
        }

        if (translated_body) {
            g_print("%s\n", translated_body);
            g_free(translated_body);
        } else {
            g_print("%s\n", body_string);
        }

        if (charset)
            g_free(charset);
    } else {
        g_print("%s\n", body_string);
    }
}

static void
print_message (Message *message)
{
    g_print("Envelope:------------------------------\n");
    print_envelope(message);
    g_print("Header:--------------------------------\n");
    print_headers(message);
    g_print("Body:----------------------------------\n");
    print_body(message);
}

static void
print_status (MilterServerContext *context, ProcessData *data)
{
    MilterStatus status;
    const gchar *status_name;

    status = milter_server_context_get_status(context);
    if (MILTER_STATUS_IS_PASS(status)) {
        status_name = "pass";
    } else {
        GEnumValue *value;
        GEnumClass *enum_class;

        enum_class = g_type_class_ref(MILTER_TYPE_STATUS);
        value = g_enum_get_value(enum_class, status);
        status_name = value->value_nick;
        g_type_class_unref(enum_class);
    }

    g_print("status: %s", status_name);
    if (data->reply_code > 0 ||
        data->reply_extended_code ||
        data->reply_message) {
        g_print(" [%d]<%s>(%s)",
                data->reply_code,
                MILTER_LOG_NULL_SAFE_STRING(data->reply_extended_code),
                MILTER_LOG_NULL_SAFE_STRING(data->reply_message));
    }
    g_print("\n");
}

static void
print_result (MilterServerContext *context, ProcessData *data)
{
    if (data->error) {
        g_print("%s\n", data->error->message);
        return;
    }

    print_status(context, data);
    g_print("elapsed-time: %g seconds\n", g_timer_elapsed(data->timer, NULL));
    if (data->quarantine_reason) {
        g_print("Quarantine reason: <%s>\n", data->quarantine_reason);
    }

    if (output_message) {
        g_print("\n");
        print_message(data->message);
    }
}

static void
setup_context (MilterServerContext *context, ProcessData *process_data)
{
    milter_agent_set_event_loop(MILTER_AGENT(context), process_data->loop);

    milter_server_context_set_name(context, g_get_prgname());

    milter_server_context_set_connection_timeout(context, connection_timeout);
    milter_server_context_set_reading_timeout(context, reading_timeout);
    milter_server_context_set_writing_timeout(context, writing_timeout);
    milter_server_context_set_end_of_message_timeout(context,
                                                     end_of_message_timeout);

    g_signal_connect(context, "ready", G_CALLBACK(cb_ready), process_data);
    g_signal_connect(context, "error", G_CALLBACK(cb_connection_error), process_data);
}

static gboolean
start_process (MilterServerContext *context, ProcessData *process_data)
{
    gboolean success;
    GError *error = NULL;

    success = milter_server_context_set_connection_spec(context, spec, &error);
    if (success)
        success = milter_server_context_establish_connection(context, &error);

    if (!success) {
        g_print("%s\n", error->message);
        g_error_free(error);
        return FALSE;
    }

    milter_event_loop_run(process_data->loop);

    print_result(context, process_data);

    return TRUE;
}

static gpointer
test_server_thread (gpointer data)
{
    gboolean success;
    ProcessData *process_data = data;
    MilterServerContext *context;

    context = milter_server_context_new();
    setup_context(context, process_data);

    success = start_process(context, process_data);

    g_object_unref(context);

    return GINT_TO_POINTER(success);
}

int
main (int argc, char *argv[])
{
    gboolean success = TRUE;
    GError *error = NULL;
    GOptionContext *option_context;
    GOptionGroup *main_group;

#ifdef HAVE_LOCALE_H
    setlocale(LC_ALL, "");
#endif

    milter_init();
    milter_server_init();

    option_context = g_option_context_new(NULL);
    g_option_context_add_main_entries(option_context, option_entries, NULL);
    main_group = g_option_context_get_main_group(option_context);
    g_option_group_set_parse_hooks(main_group,
                                   pre_option_parse,
                                   post_option_parse);

    if (!g_option_context_parse(option_context, &argc, &argv, &error)) {
        g_print("%s\n", error->message);
        g_error_free(error);
        g_option_context_free(option_context);
        free_option_values();
        exit(EXIT_FAILURE);
    }

    if (verbose)
        g_setenv("MILTER_LOG_LEVEL", "all", FALSE);

    if (n_threads > 0) {
        GThread **threads;
        ProcessData *process_data;
        gint i;

        threads = g_new0(GThread *, n_threads);
        process_data = g_new0(ProcessData, n_threads);
        for (i = 0; i < n_threads; i++) {
            init_process_data(&process_data[i]);
            threads[i] = g_thread_create(test_server_thread, &process_data[i], TRUE, NULL);
        }

        for (i = 0; i < n_threads; i++) {
            success &= GPOINTER_TO_INT(g_thread_join(threads[i]));
            free_process_data(&process_data[i]);
        }

        g_free(process_data);
        g_free(threads);
    } else {
        ProcessData process_data;
        init_process_data(&process_data);
        success = GPOINTER_TO_INT(test_server_thread(&process_data));
        free_process_data(&process_data);
    }


    milter_server_quit();
    milter_quit();
    g_option_context_free(option_context);
    free_option_values();

    exit(success ? EXIT_SUCCESS : EXIT_FAILURE);
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
