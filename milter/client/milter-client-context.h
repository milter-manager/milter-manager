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

#ifndef __MILTER_CLIENT_CONTEXT_H__
#define __MILTER_CLIENT_CONTEXT_H__

#include <glib-object.h>

#include <milter/core.h>
#include <milter/client/milter-client-objects.h>

G_BEGIN_DECLS

/**
 * SECTION: milter-client-context
 * @title: MilterClientContext
 * @short_description: Process milter protocol.
 *
 * The %MilterClientContext processes one milter protocol
 * session. It means %MilterClientContext instance is
 * created for each milter protocol session.
 *
 * To process each milter protocol command, you need to
 * connect signals of
 * %MilterClientContext. %MilterClientContext has signals
 * that correspond to milter protocol events:
 *
 * <rd>
 * : ((<xxfi_negotiate|URL:https://www.milter.org/developers/api/xxfi_negotiate>))
 *    #MilterClientContext::negotiate
 * : ((<xxfi_connect|URL:https://www.milter.org/developers/api/xxfi_connect>))
 *    #MilterClientContext::connect
 * : ((<xxfi_helo|URL:https://www.milter.org/developers/api/xxfi_helo>))
 *    #MilterClientContext::helo
 * : ((<xxfi_envfrom|URL:https://www.milter.org/developers/api/xxfi_envfrom>))
 *    #MilterClientContext::envelope-from
 * : ((<xxfi_envrcpt|URL:https://www.milter.org/developers/api/xxfi_envrcpt>))
 *    #MilterClientContext::envelope-recipient
 * : ((<xxfi_data|URL:https://www.milter.org/developers/api/xxfi_data>))
 *    #MilterClientContext::data
 * : ((<xxfi_unknown|URL:https://www.milter.org/developers/api/xxfi_unknown>))
 *    #MilterClientContext::unknown
 * : ((<xxfi_header|URL:https://www.milter.org/developers/api/xxfi_header>))
 *    #MilterClientContext::header
 * : ((<xxfi_eoh|URL:https://www.milter.org/developers/api/xxfi_eoh>))
 *    #MilterClientContext::end-of-header
 * : ((<xxfi_body|URL:https://www.milter.org/developers/api/xxfi_body>))
 *    #MilterClientContext::body
 * : ((<xxfi_eom|URL:https://www.milter.org/developers/api/xxfi_eom>))
 *    #MilterClientContext::end-of-message
 * : ((<xxfi_abort|URL:https://www.milter.org/developers/api/xxfi_abort>))
 *    #MilterClientContext::abort.
 *
 *    NOTE: You will need to check whether the current state is
 *    message processing or not. You can use
 *    %MILTER_CLIENT_CONTEXT_STATE_IN_MESSAGE_PROCESSING for it.
 * : ((<xxfi_close|URL:https://www.milter.org/developers/api/xxfi_close>))
 *    #MilterFinishedEmittable::finished
 * </rd>
 *
 * Here is an example to connect signals. It connects all
 * signals and each connected signal handler prints its
 * event name:
 *
 * |[
 * static MilterStatus
 * cb_negotiate (MilterClientContext *context, MilterOption *option,
 *               gpointer user_data)
 * {
 *     g_print("negotiate\n");
 *     return MILTER_STATUS_ALL_OPTIONS;
 * }
 * 
 * static MilterStatus
 * cb_connect (MilterClientContext *context, const gchar *host_name,
 *             const struct sockaddr *address, socklen_t address_length,
 *             gpointer user_data)
 * {
 *     g_print("connect\n");
 *     return MILTER_STATUS_CONTINUE;
 * }
 * 
 * static MilterStatus
 * cb_helo (MilterClientContext *context, const gchar *fqdn, gpointer user_data)
 * {
 *     g_print("helo\n");
 *     return MILTER_STATUS_CONTINUE;
 * }
 * 
 * static MilterStatus
 * cb_envelope_from (MilterClientContext *context, const gchar *from,
 *                   gpointer user_data)
 * {
 *     g_print("envelope-from\n");
 *     return MILTER_STATUS_CONTINUE;
 * }
 * 
 * static MilterStatus
 * cb_envelope_recipient (MilterClientContext *context, const gchar *to,
 *                        gpointer user_data)
 * {
 *     g_print("envelope-recipient\n");
 *     return MILTER_STATUS_CONTINUE;
 * }
 * 
 * static MilterStatus
 * cb_data (MilterClientContext *context, gpointer user_data)
 * {
 *     g_print("data\n");
 *     return MILTER_STATUS_CONTINUE;
 * }
 * 
 * static MilterStatus
 * cb_header (MilterClientContext *context, const gchar *name, const gchar *value,
 *            gpointer user_data)
 * {
 *     g_print("header\n");
 *     return MILTER_STATUS_CONTINUE;
 * }
 * 
 * static MilterStatus
 * cb_end_of_header (MilterClientContext *context, gpointer user_data)
 * {
 *     g_print("end-of-header\n");
 *     return MILTER_STATUS_CONTINUE;
 * }
 * 
 * static MilterStatus
 * cb_body (MilterClientContext *context, const gchar *chunk, gsize length,
 *          gpointer user_data)
 * {
 *     g_print("body\n");
 *     return MILTER_STATUS_CONTINUE;
 * }
 * 
 * static MilterStatus
 * cb_end_of_message (MilterClientContext *context,
 *                    const gchar *chunk, gsize length,
 *                    gpointer user_data)
 * {
 *     g_print("end-of-message\n");
 *     return MILTER_STATUS_CONTINUE;
 * }
 * 
 * static MilterStatus
 * cb_abort (MilterClientContext *context, MilterClientContextState state,
 *           gpointer user_data)
 * {
 *     g_print("abort\n");
 *     return MILTER_STATUS_CONTINUE;
 * }
 * 
 * static MilterStatus
 * cb_unknown (MilterClientContext *context, const gchar *command,
 *             gpointer user_data)
 * {
 *     g_print("unknown\n");
 *     return MILTER_STATUS_CONTINUE;
 * }
 * 
 * static void
 * setup_context_signals (MilterClientContext *context)
 * {
 * #define CONNECT(name)                                                   \
 *     g_signal_connect(context, #name, G_CALLBACK(cb_ ## name), NULL)
 * 
 *     CONNECT(negotiate);
 *     CONNECT(connect);
 *     CONNECT(helo);
 *     CONNECT(envelope_from);
 *     CONNECT(envelope_recipient);
 *     CONNECT(data);
 *     CONNECT(header);
 *     CONNECT(end_of_header);
 *     CONNECT(body);
 *     CONNECT(end_of_message);
 *     CONNECT(abort);
 *     CONNECT(unknown);
 * 
 * #undef CONNECT
 * }
 * ]|
 */

/**
 * MILTER_CLIENT_CONTEXT_ERROR:
 *
 * Used to get the #GError quark for #MilterClientContext errors.
 */
#define MILTER_CLIENT_CONTEXT_ERROR           (milter_client_context_error_quark())

#define MILTER_TYPE_CLIENT_CONTEXT            (milter_client_context_get_type())
#define MILTER_CLIENT_CONTEXT(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), MILTER_TYPE_CLIENT_CONTEXT, MilterClientContext))
#define MILTER_CLIENT_CONTEXT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), MILTER_TYPE_CLIENT_CONTEXT, MilterClientContextClass))
#define MILTER_IS_CLIENT_CONTEXT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), MILTER_TYPE_CLIENT_CONTEXT))
#define MILTER_IS_CLIENT_CONTEXT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), MILTER_TYPE_CLIENT_CONTEXT))
#define MILTER_CLIENT_CONTEXT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), MILTER_TYPE_CLIENT_CONTEXT, MilterClientContextClass))

/**
 * MilterClientContextError:
 * @MILTER_CLIENT_CONTEXT_ERROR_INVALID_CODE: Indicates a
 * status code specified by
 * milter_client_context_set_reply() is invalid.
 * @MILTER_CLIENT_CONTEXT_ERROR_IO_ERROR: Indicates an IO
 * error causing on writing/reading milter protocol data.
 * @MILTER_CLIENT_CONTEXT_ERROR_NULL: Indicates unexpected
 * %NULL is passed.
 * @MILTER_CLIENT_CONTEXT_ERROR_INVALID_STATE: Indicates
 * unexpected operation is requested on the current
 * %MilterClientContextState.
 * @MILTER_CLIENT_CONTEXT_ERROR_INVALID_ACTION: Indicates
 * unexpected operation is requested on the context's
 * %MilterActionFlags.
 * @MILTER_CLIENT_CONTEXT_ERROR_NULL: Indicates unexpected
 * empty value is passed.
 *
 * These identify the variable errors that can occur while
 * calling %MilterClientContext functions.
 */
typedef enum
{
    MILTER_CLIENT_CONTEXT_ERROR_INVALID_CODE,
    MILTER_CLIENT_CONTEXT_ERROR_IO_ERROR,
    MILTER_CLIENT_CONTEXT_ERROR_NULL,
    MILTER_CLIENT_CONTEXT_ERROR_INVALID_STATE,
    MILTER_CLIENT_CONTEXT_ERROR_INVALID_ACTION,
    MILTER_CLIENT_CONTEXT_ERROR_EMPTY
} MilterClientContextError;

/**
 * MilterClientContextState:
 * @MILTER_CLIENT_CONTEXT_STATE_INVALID: Invalid state.
 * @MILTER_CLIENT_CONTEXT_STATE_START: Just started.
 * @MILTER_CLIENT_CONTEXT_STATE_NEGOTIATE: Starting negotiation.
 * @MILTER_CLIENT_CONTEXT_STATE_NEGOTIATE_REPLIED: Received
 * negotiation response.
 * @MILTER_CLIENT_CONTEXT_STATE_CONNECT: Sent connection
 * information.
 * @MILTER_CLIENT_CONTEXT_STATE_CONNECT_REPLIED: Received
 * connection information response.
 * @MILTER_CLIENT_CONTEXT_STATE_HELO: Starting HELO.
 * @MILTER_CLIENT_CONTEXT_STATE_HELO_REPLIED: Received
 * HELO response.
 * @MILTER_CLIENT_CONTEXT_STATE_ENVELOPE_FROM: Starting MAIL
 * FROM command.
 * @MILTER_CLIENT_CONTEXT_STATE_ENVELOPE_FROM_REPLIED: Receive
 * MAIL FROM response.
 * @MILTER_CLIENT_CONTEXT_STATE_ENVELOPE_RECIPIENT: Starting
 * RCPT TO command.
 * @MILTER_CLIENT_CONTEXT_STATE_ENVELOPE_RECIPIENT_REPLIED: Receive
 * RCPT TO response.
 * @MILTER_CLIENT_CONTEXT_STATE_DATA: Starting DATA command.
 * @MILTER_CLIENT_CONTEXT_STATE_DATA_REPLIED: Receive
 * DATA response.
 * @MILTER_CLIENT_CONTEXT_STATE_UNKNOWN: Receiving unknown
 * SMTP command.
 * @MILTER_CLIENT_CONTEXT_STATE_UNKNOWN_REPLIED: Receive
 * unknown SMTP command response.
 * @MILTER_CLIENT_CONTEXT_STATE_HEADER: Sent a header.
 * @MILTER_CLIENT_CONTEXT_STATE_HEADER_REPLIED: Receive
 * header response.
 * @MILTER_CLIENT_CONTEXT_STATE_END_OF_HEADER: All headers are
 * sent.
 * @MILTER_CLIENT_CONTEXT_STATE_END_OF_HEADER_REPLIED: Receive
 * end-of-header response.
 * @MILTER_CLIENT_CONTEXT_STATE_BODY: Sending body chunks.
 * @MILTER_CLIENT_CONTEXT_STATE_BODY_REPLIED: Received
 * body response.
 * @MILTER_CLIENT_CONTEXT_STATE_END_OF_MESSAGE: All body
 * chunks are sent.
 * @MILTER_CLIENT_CONTEXT_STATE_END_OF_MESSAGE_REPLIED: Receive
 * end-of-message response.
 * @MILTER_CLIENT_CONTEXT_STATE_QUIT: Starting quitting.
 * @MILTER_CLIENT_CONTEXT_STATE_QUIT_REPLIED: Receive
 * quit response.
 * @MILTER_CLIENT_CONTEXT_STATE_ABORT: Starting aborting.
 * @MILTER_CLIENT_CONTEXT_STATE_ABORT_REPLIED: Receive
 * abort response.
 * @MILTER_CLIENT_CONTEXT_STATE_FINISHED: Finished.
 *
 * These identify the state of %MilterClientContext.
 */
typedef enum
{
    MILTER_CLIENT_CONTEXT_STATE_INVALID,
    MILTER_CLIENT_CONTEXT_STATE_START,
    MILTER_CLIENT_CONTEXT_STATE_NEGOTIATE,
    MILTER_CLIENT_CONTEXT_STATE_NEGOTIATE_REPLIED,
    MILTER_CLIENT_CONTEXT_STATE_CONNECT,
    MILTER_CLIENT_CONTEXT_STATE_CONNECT_REPLIED,
    MILTER_CLIENT_CONTEXT_STATE_HELO,
    MILTER_CLIENT_CONTEXT_STATE_HELO_REPLIED,
    MILTER_CLIENT_CONTEXT_STATE_ENVELOPE_FROM,
    MILTER_CLIENT_CONTEXT_STATE_ENVELOPE_FROM_REPLIED,
    MILTER_CLIENT_CONTEXT_STATE_ENVELOPE_RECIPIENT,
    MILTER_CLIENT_CONTEXT_STATE_ENVELOPE_RECIPIENT_REPLIED,
    MILTER_CLIENT_CONTEXT_STATE_DATA,
    MILTER_CLIENT_CONTEXT_STATE_DATA_REPLIED,
    MILTER_CLIENT_CONTEXT_STATE_UNKNOWN,
    MILTER_CLIENT_CONTEXT_STATE_UNKNOWN_REPLIED,
    MILTER_CLIENT_CONTEXT_STATE_HEADER,
    MILTER_CLIENT_CONTEXT_STATE_HEADER_REPLIED,
    MILTER_CLIENT_CONTEXT_STATE_END_OF_HEADER,
    MILTER_CLIENT_CONTEXT_STATE_END_OF_HEADER_REPLIED,
    MILTER_CLIENT_CONTEXT_STATE_BODY,
    MILTER_CLIENT_CONTEXT_STATE_BODY_REPLIED,
    MILTER_CLIENT_CONTEXT_STATE_END_OF_MESSAGE,
    MILTER_CLIENT_CONTEXT_STATE_END_OF_MESSAGE_REPLIED,
    MILTER_CLIENT_CONTEXT_STATE_QUIT,
    MILTER_CLIENT_CONTEXT_STATE_QUIT_REPLIED,
    MILTER_CLIENT_CONTEXT_STATE_ABORT,
    MILTER_CLIENT_CONTEXT_STATE_ABORT_REPLIED,
    MILTER_CLIENT_CONTEXT_STATE_FINISHED
} MilterClientContextState;

#define MILTER_CLIENT_CONTEXT_STATE_IN_MESSAGE_PROCESSING(state)        \
    (MILTER_CLIENT_CONTEXT_STATE_ENVELOPE_FROM <= (state) &&            \
     (state) < MILTER_CLIENT_CONTEXT_STATE_END_OF_MESSAGE)

typedef struct _MilterClientContextClass    MilterClientContextClass;

struct _MilterClientContext
{
    MilterProtocolAgent object;
};

struct _MilterClientContextClass
{
    MilterProtocolAgentClass parent_class;

    MilterStatus (*negotiate)          (MilterClientContext *context,
                                        MilterOption  *option,
                                        MilterMacrosRequests *macros_requests);
    void         (*negotiate_response) (MilterClientContext *context,
                                        MilterOption  *option,
                                        MilterMacrosRequests *macros_requests,
                                        MilterStatus   status);
    void         (*define_macro)       (MilterClientContext *context,
                                        MilterCommand command,
                                        GHashTable *macros);
    MilterStatus (*connect)            (MilterClientContext *context,
                                        const gchar   *host_name,
                                        struct sockaddr *address,
                                        socklen_t      address_length);
    void         (*connect_response)   (MilterClientContext *context,
                                        MilterStatus         status);
    MilterStatus (*helo)               (MilterClientContext *context,
                                        const gchar   *fqdn);
    void         (*helo_response)      (MilterClientContext *context,
                                        MilterStatus         status);
    MilterStatus (*envelope_from)      (MilterClientContext *context,
                                        const gchar   *from);
    void         (*envelope_from_response)
                                       (MilterClientContext *context,
                                        MilterStatus         status);
    MilterStatus (*envelope_recipient) (MilterClientContext *context,
                                        const gchar   *recipient);
    void         (*envelope_recipient_response)
                                       (MilterClientContext *context,
                                        MilterStatus         status);
    MilterStatus (*data)               (MilterClientContext *context);
    void         (*data_response)      (MilterClientContext *context,
                                        MilterStatus         status);
    MilterStatus (*unknown)            (MilterClientContext *context,
                                        const gchar   *command);
    void         (*unknown_response)   (MilterClientContext *context,
                                        MilterStatus         status);
    MilterStatus (*header)             (MilterClientContext *context,
                                        const gchar   *name,
                                        const gchar   *value);
    void         (*header_response)    (MilterClientContext *context,
                                        MilterStatus         status);
    MilterStatus (*end_of_header)      (MilterClientContext *context);
    void         (*end_of_header_response)
                                       (MilterClientContext *context,
                                        MilterStatus         status);
    MilterStatus (*body)               (MilterClientContext *context,
                                        const gchar         *chunk,
                                        gsize                size);
    void         (*body_response)      (MilterClientContext *context,
                                        MilterStatus         status);
    MilterStatus (*end_of_message)     (MilterClientContext *context,
                                        const gchar         *chunk,
                                        gsize                size);
    void         (*end_of_message_response)
                                       (MilterClientContext *context,
                                        MilterStatus         status);
    MilterStatus (*abort)              (MilterClientContext *context,
                                        MilterClientContextState state);
    void         (*abort_response)     (MilterClientContext *context,
                                        MilterStatus         status);
    void         (*timeout)            (MilterClientContext *context);

    void         (*message_processed)  (MilterClientContext *context,
                                        MilterMessageResult *result);
};

GQuark               milter_client_context_error_quark       (void);

GType                milter_client_context_get_type          (void) G_GNUC_CONST;

/**
 * milter_client_context_new:
 * @client: a %MilterClient for the context.
 *
 * Creates a new context object. Normally, context object is
 * created by %MilterClient and passed by
 * #MilterClient::connection-established signal.
 *
 * Returns: a new %MilterClientContext object.
 */
MilterClientContext *milter_client_context_new               (MilterClient *client);

/**
 * milter_client_context_feed:
 * @context: a %MilterClientContext.
 * @chunk: the string to be fed to @context.
 * @size: the size of @chunk.
 * @error: return location for an error, or %NULL.
 *
 * Feeds a chunk to the @context. You can use it for testing
 * or debugging.
 *
 * Returns: %TRUE on success.
 */
gboolean             milter_client_context_feed              (MilterClientContext *context,
                                                              const gchar *chunk,
                                                              gsize size,
                                                              GError **error);

/**
 * milter_client_context_get_private_data:
 * @context: a %MilterClientContext.
 *
 * Gets the private data of the @context.
 *
 * Returns: the private data set by
 * milter_client_context_set_private_data() or %NULL.
 */
gpointer             milter_client_context_get_private_data  (MilterClientContext *context);

/**
 * milter_client_context_set_private_data:
 * @context: a %MilterClientContext.
 * @data: the private data.
 * @destroy: the destroy function for @data or %NULL.
 *
 * Sets the private data of the @context. @data is
 * destroyed by @destroy when @data is unset. @data is unset
 * when new private data is set or @context is destroyed.
 */
void                 milter_client_context_set_private_data  (MilterClientContext *context,
                                                              gpointer data,
                                                              GDestroyNotify destroy);

/**
 * milter_client_context_set_reply:
 * @context: a %MilterClientContext.
 * @code: the three-digit SMTP error reply
 *        code. (RFC 2821) Only 4xx and 5xx are accepted.
 * @extended_code: the extended reply code (RFC 1893/2034),
 *                 or %NULL.
 * @message: the text part of the SMTP reply, or %NULL.
 * @error: return location for an error, or %NULL.
 *
 * Sets the error reply code. 4xx @code is used on
 * %MILTER_REPLY_TEMPORARY_FAILURE. 5xx @code is used on
 * %MILTER_REPLY_REJECT.
 *
 * See also <ulink
 * url="https://www.milter.org/developers/api/smfi_setreply">
 * smfi_setreply</ulink> on milter.org.
 *
 * Returns: %TRUE on success.
 */
gboolean             milter_client_context_set_reply         (MilterClientContext *context,
                                                              guint code,
                                                              const gchar *extended_code,
                                                              const gchar *message,
                                                              GError **error);

/**
 * milter_client_context_format_reply:
 * @context: a %MilterClientContext.
 *
 * Formats the current error reply code specified by
 * milter_client_context_set_reply(). If error reply code
 * isn't set, this function returns %NULL.
 *
 * Returns: formatted reply code, or %NULL.
 */
gchar               *milter_client_context_format_reply      (MilterClientContext *context);

/**
 * milter_client_context_add_header:
 * @context: a %MilterClientContext.
 * @name: the header name.
 * @value: the header value.
 * @error: return location for an error, or %NULL.
 *
 * Adds a header to the current message's header list. This
 * function can be called in
 * #MilterClientContext::end-of-message signal.
 *
 * See also <ulink
 * url="https://www.milter.org/developers/api/smfi_addheader">
 * smfi_addheader</ulink> on milter.org.
 *
 * FIXME: write about %MILTER_ACTION_ADD_HEADERS.
 *
 * Returns: %TRUE on success.
 */
gboolean             milter_client_context_add_header        (MilterClientContext *context,
                                                              const gchar *name,
                                                              const gchar *value,
                                                              GError     **error);

/**
 * milter_client_context_insert_header:
 * @context: a %MilterClientContext.
 * @index: the index to be inserted.
 * @name: the header name.
 * @value: the header value.
 * @error: return location for an error, or %NULL.
 *
 * Inserts a header into the current message's header lists
 * at @index. This function can be called in
 * #MilterClientContext::end-of-message signal.  See also
 * <ulink
 * url="https://www.milter.org/developers/api/smfi_insheader">
 * smfi_insheader</ulink> on milter.org.
 *
 * FIXME: write about MILTER_ACTION_ADD_HEADERS.
 *
 * Returns: %TRUE on success.
 */
gboolean             milter_client_context_insert_header     (MilterClientContext *context,
                                                              guint32      index,
                                                              const gchar *name,
                                                              const gchar *value,
                                                              GError     **error);

/**
 * milter_client_context_change_header:
 * @context: a %MilterClientContext.
 * @name: the header name.
 * @index: the index of headers that all of them are named
 *         @name. (1-based) FIXME: should change 0-based?
 * @value: the header value. Use %NULL to delete the target
 *         header.
 * @error: return location for an error, or %NULL.
 *
 * Changes a header that is located at @index in headers
 * that all of them are named @name. If @value is %NULL, the
 * header is deleted. This function can be
 * called in #MilterClientContext::end-of-message signal.
 *
 * See also <ulink
 * url="https://www.milter.org/developers/api/smfi_chgheader">
 * smfi_chgheader</ulink> on milter.org.
 *
 * FIXME: write about MILTER_ACTION_CHANGE_HEADERS.
 *
 * Returns: %TRUE on success.
 */
gboolean             milter_client_context_change_header     (MilterClientContext *context,
                                                              const gchar *name,
                                                              guint32      index,
                                                              const gchar *value,
                                                              GError     **error);

/**
 * milter_client_context_delete_header:
 * @context: a %MilterClientContext.
 * @name: the header name.
 * @index: the index of headers that all of them are named
 * @name. (1-based) FIXME: should change 0-based?
 * @error: return location for an error, or %NULL.
 *
 * Deletes a header that is located at @index in headers
 * that all of them are named @name. This function can be
 * called in #MilterClientContext::end-of-message signal.
 * This function works same as
 * milter_client_context_change_header() with %NULL as
 * @value.
 *
 * FIXME: write about MILTER_ACTION_CHANGE_HEADERS.
 *
 * Returns: %TRUE on success.
 */
gboolean             milter_client_context_delete_header     (MilterClientContext *context,
                                                              const gchar *name,
                                                              guint32      index,
                                                              GError     **error);

/**
 * milter_client_context_change_from:
 * @context: a %MilterClientContext.
 * @from: the new envelope from address.
 * @parameters: the ESMTP's 'MAIL FROM' parameter. It can be
 * %NULL.
 * @error: return location for an error, or %NULL.
 *
 * Changes the envelope from address of the current message.
 * ESMTP's 'MAIL FROM' parameter can be set by
 * @parameters. @parameters may be %NULL. This function can be
 * called in #MilterClientContext::end-of-message signal.
 * See also <ulink
 * url="https://www.milter.org/developers/api/smfi_chgfrom">smfi_chgfrom
 * </ulink> on milter.org.
 *
 * FIXME: write about MILTER_ACTION_CHANGE_FROM.
 *
 * Returns: %TRUE on success.
 */
gboolean             milter_client_context_change_from       (MilterClientContext *context,
                                                              const gchar *from,
                                                              const gchar *parameters,
                                                              GError     **error);

/**
 * milter_client_context_add_recipient:
 * @context: a %MilterClientContext.
 * @recipient: the new envelope recipient address.
 * @parameters: the ESMTP's 'RCPT TO' parameter. It can be
 * %NULL.
 * @error: return location for an error, or %NULL.
 *
 * Adds a new envelope recipient address to the current
 * message.  ESMTP's 'RCPT TO' parameter can be set by
 * @parameters. @parameters may be %NULL. This function can
 * be called in #MilterClientContext::end-of-message
 * signal. See also <ulink
 * url="https://www.milter.org/developers/api/smfi_addrcpt">smfi_addrcpt
 * </ulink> and <ulink
 * url="https://www.milter.org/developers/api/smfi_addrcpt_par">smfi_addrcpt_par
 * </ulink> on milter.org.
 *
 * FIXME: write about MILTER_ACTION_ADD_RECIPIENT and
 * MILTER_ACTION_ADD_ENVELOPE_RECIPIENT_WITH_PARAMETERS.
 *
 * Returns: %TRUE on success.
 */
gboolean             milter_client_context_add_recipient     (MilterClientContext *context,
                                                              const gchar *recipient,
                                                              const gchar *parameters,
                                                              GError     **error);

/**
 * milter_client_context_delete_recipient:
 * @context: a %MilterClientContext.
 * @recipient: the envelope recipient address to be removed.
 * @error: return location for an error, or %NULL.
 *
 * Removes a envelope recipient that named @recipient. This
 * function can be called in
 * #MilterClientContext::end-of-message signal. See also
 * <ulink
 * url="https://www.milter.org/developers/api/smfi_delrcpt">smfi_delrcpt
 * </ulink> on milter.org.
 *
 * FIXME: write about MILTER_ACTION_DELETE_RECIPIENT.
 *
 * Returns: %TRUE on success.
 */
gboolean             milter_client_context_delete_recipient  (MilterClientContext *context,
                                                              const gchar *recipient,
                                                              GError     **error);


/**
 * milter_client_context_replace_body:
 * @context: a %MilterClientContext.
 * @body: the new body.
 * @body_size: the size of @body.
 * @error: return location for an error, or %NULL.
 *
 * Replaces the body of the current message with @body. This
 * function can be called in
 * #MilterClientContext::end-of-message signal. See also
 * <ulink
 * url="https://www.milter.org/developers/api/smfi_replacebody">smfi_replacebody
 * </ulink> on milter.org.
 *
 * FIXME: write about MILTER_ACTION_CHANGE_BODY.
 *
 * Returns: %TRUE on success.
 */
gboolean             milter_client_context_replace_body      (MilterClientContext *context,
                                                              const gchar *body,
                                                              gsize        body_size,
                                                              GError     **error);

/**
 * milter_client_context_progress:
 * @context: a %MilterClientContext.
 *
 * Notifies the MTA that this milter is still in
 * progress. This function can be called in
 * #MilterClientContext::end-of-message signal. See also
 * <ulink
 * url="https://www.milter.org/developers/api/smfi_progress">smfi_progress
 * </ulink> on milter.org.
 *
 * Returns: %TRUE on success.
 */
gboolean             milter_client_context_progress          (MilterClientContext *context);

/**
 * milter_client_context_quarantine:
 * @context: a %MilterClientContext.
 * @reason: the reason why the current message is quarantined.
 *
 * Quarantines the current message with @reason. This
 * function can be called in
 * #MilterClientContext::end-of-message signal. See also
 * <ulink
 * url="https://www.milter.org/developers/api/smfi_quarantine">smfi_quarantine
 * </ulink> on milter.org.
 *
 * FIXME: write about MILTER_ACTION_QUARANTINE.
 *
 * Returns: %TRUE on success.
 */
gboolean             milter_client_context_quarantine        (MilterClientContext *context,
                                                              const gchar *reason);

/**
 * milter_client_context_set_timeout:
 * @context: a %MilterClientContext.
 * @timeout: the timeout by seconds. (default is 7210 seconds)
 *
 * Sets the timeout by seconds. If MTA doesn't responses in
 * @timeout seconds, #MilterClientContext::timeout
 * signal is emitted. See also
 * <ulink
 * url="https://www.milter.org/developers/api/smfi_settimeout">smfi_settimeout
 * </ulink> on milter.org.
 */
void                 milter_client_context_set_timeout       (MilterClientContext *context,
                                                              guint timeout);

/**
 * milter_client_context_get_timeout:
 * @context: a %MilterClientContext.
 *
 * Gets the timeout by seconds.
 *
 * Returns: timeout by seconds.
 */
guint                milter_client_context_get_timeout       (MilterClientContext *context);

/**
 * milter_client_context_set_state:
 * @context: a %MilterClientContext.
 * @state: a %MilterClientContextState.
 *
 * Sets the current state.
 *
 * <note>This is for testing. Don't use it directory for normal use.</note>
 */
void                 milter_client_context_set_state         (MilterClientContext *context,
                                                              MilterClientContextState state);

/**
 * milter_client_context_get_state:
 * @context: a %MilterClientContext.
 *
 * Gets the current state.
 *
 * <note>This is for testing. Don't use it directory for normal use.</note>
 *
 * Returns: the current state.
 */
MilterClientContextState milter_client_context_get_state     (MilterClientContext *context);

/**
 * milter_client_context_set_quarantine_reason:
 * @context: a %MilterClientContext.
 * @reason: a quarantine reason.
 *
 * Sets the quarantine reason.
 *
 * <note>This is for testing. Don't use it directory for normal use.</note>
 */
void                 milter_client_context_set_quarantine_reason
                                                             (MilterClientContext *context,
                                                              const gchar         *reason);

/**
 * milter_client_context_get_quarantine_reason:
 * @context: a %MilterClientContext.
 *
 * Gets the quarantine reason.
 *
 * <note>This is for testing. Don't use it directory for normal use.</note>
 *
 * Returns: the quarantine reason.
 */
const gchar         *milter_client_context_get_quarantine_reason
                                                             (MilterClientContext *context);

/**
 * milter_client_context_get_last_state:
 * @context: a %MilterClientContext.
 *
 * Gets the last state. It's one of start, negotiate,
 * connect, helo, envelope-from, envelope-recipient, data,
 * unknown, header, end-of-header, body and end-of-message.
 *
 * Returns: the last state.
 */
MilterClientContextState milter_client_context_get_last_state(MilterClientContext *context);

/**
 * milter_client_context_set_status:
 * @context: a %MilterClientContext.
 * @status: a %MilterStatus.
 *
 * Sets the current status.
 */
void                 milter_client_context_set_status        (MilterClientContext *context,
                                                              MilterStatus status);

/**
 * milter_client_context_get_status:
 * @context: a %MilterClientContext.
 *
 * Gets the current status.
 *
 * Returns: the current status.
 */
MilterStatus             milter_client_context_get_status    (MilterClientContext *context);

/**
 * milter_client_context_set_option:
 * @context: a %MilterClientContext.
 * @option: a %MilterOption.
 *
 * Sets the option for the context.
 */
void                 milter_client_context_set_option        (MilterClientContext *context,
                                                              MilterOption *option);

/**
 * milter_client_context_get_option:
 * @context: a %MilterClientContext.
 *
 * Gets the option for the context.
 *
 * Returns: the option for the context.
 */
MilterOption        *milter_client_context_get_option        (MilterClientContext *context);

/**
 * milter_client_context_set_socket_address:
 * @context: a %MilterClientContext.
 * @address: a %MilterGenericSocketAddress.
 *
 * Sets the socket address of connected server for the context.
 */
void                 milter_client_context_set_socket_address(MilterClientContext *context,
                                                              MilterGenericSocketAddress *address);

/**
 * milter_client_context_get_option:
 * @context: a %MilterClientContext.
 *
 * Gets the socket address of connected server for the context.
 *
 * Returns: the socket address of connected server for the context.
 */
MilterGenericSocketAddress *milter_client_context_get_socket_address
                                                             (MilterClientContext *context);

/**
 * milter_client_context_get_message_result:
 * @context: a %MilterClientContext.
 *
 * Gets the message result of @context.
 *
 * Returns: the message result of @context.
 */
MilterMessageResult *milter_client_context_get_message_result
                                                       (MilterClientContext *context);


/**
 * milter_client_context_set_message_result:
 * @context: a %MilterClientContext.
 * @result: the message result.
 *
 * Sets the message result of @context.
 */
void                 milter_client_context_set_message_result
                                                       (MilterClientContext *context,
                                                        MilterMessageResult *result);

/**
 * milter_client_context_reset_message_related_data:
 * @context: a %MilterClientContext.
 *
 * Resets message related data of @context.
 *
 * Normally, you don't need to call this function.
 */
void                 milter_client_context_reset_message_related_data
                                                       (MilterClientContext *context);

/**
 * milter_client_context_get_n_processing_sessions:
 * @context: a %MilterClientContext.
 *
 * Returns number of the current processing sessions.
 *
 * Returns: number of the current processing sessions.
 */
guint                milter_client_context_get_n_processing_sessions
                                                       (MilterClientContext  *context);

/**
 * milter_client_context_set_packet_buffer_size:
 * @context: a %MilterClientContext.
 * @size: a packet buffer size in bytes. (The deafult is 0
 *        bytes. It means buffering is disabled.)
 *
 * Sets the packet buffer size for the context. Packets on
 * end-of-message are buffered until the buffer size is
 * full. Packet buffering is for performance.
 */
void                 milter_client_context_set_packet_buffer_size
                                                       (MilterClientContext *context,
                                                        guint                size);

/**
 * milter_client_context_get_packet_buffer_size:
 * @context: a %MilterClientContext.
 *
 * Gets the packet buffer size for the context.
 *
 * Returns: the packet buffer size for the context.
 */
guint                milter_client_context_get_packet_buffer_size
                                                       (MilterClientContext *context);

G_END_DECLS

#endif /* __MILTER_CLIENT_CONTEXT_H__ */

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
