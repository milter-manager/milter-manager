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

#ifndef __MILTER_SERVER_CONTEXT_H__
#define __MILTER_SERVER_CONTEXT_H__

#include <glib-object.h>

#include <milter/core.h>
#include <milter/server/milter-server-enum-types.h>

G_BEGIN_DECLS

/**
 * SECTION: milter-server-context
 * @title: MilterServerContext
 * @short_description: Process server side milter protocol.
 *
 * The %MilterServerContext processes one server side milter
 * protocol session. It means %MilterServerContext instance
 * is created for each milter protocol session.
 */

/**
 * MILTER_SERVER_CONTEXT_ERROR:
 *
 * Used to get the #GError quark for #MilterServerContext errors.
 */
#define MILTER_SERVER_CONTEXT_ERROR           (milter_server_context_error_quark())

#define MILTER_TYPE_SERVER_CONTEXT            (milter_server_context_get_type())
#define MILTER_SERVER_CONTEXT(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), MILTER_TYPE_SERVER_CONTEXT, MilterServerContext))
#define MILTER_SERVER_CONTEXT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), MILTER_TYPE_SERVER_CONTEXT, MilterServerContextClass))
#define MILTER_IS_SERVER_CONTEXT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), MILTER_TYPE_SERVER_CONTEXT))
#define MILTER_IS_SERVER_CONTEXT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), MILTER_TYPE_SERVER_CONTEXT))
#define MILTER_SERVER_CONTEXT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), MILTER_TYPE_SERVER_CONTEXT, MilterServerContextClass))

/**
 * MILTER_SERVER_CONTEXT_DEFAULT_CONNECTION_TIMEOUT:
 *
 * The default connection timeout by seconds.
 **/
#define MILTER_SERVER_CONTEXT_DEFAULT_CONNECTION_TIMEOUT     300

/**
 * MILTER_SERVER_CONTEXT_DEFAULT_WRITING_TIMEOUT:
 *
 * The default writing timeout by seconds.
 **/
#define MILTER_SERVER_CONTEXT_DEFAULT_WRITING_TIMEOUT         10

/**
 * MILTER_SERVER_CONTEXT_DEFAULT_READING_TIMEOUT:
 *
 * The default reading timeout by seconds.
 **/
#define MILTER_SERVER_CONTEXT_DEFAULT_READING_TIMEOUT         10

/**
 * MILTER_SERVER_CONTEXT_DEFAULT_END_OF_MESSAGE_TIMEOUT:
 *
 * The default end-of-message response timeout by seconds.
 **/
#define MILTER_SERVER_CONTEXT_DEFAULT_END_OF_MESSAGE_TIMEOUT 300

/**
 * MilterServerContextError:
 * @MILTER_SERVER_CONTEXT_ERROR_CONNECTION_FAILURE: Indicates a
 * connection failure.
 * @MILTER_SERVER_CONTEXT_ERROR_NO_SPEC: Indicates the
 * connection spec isn't set.
 * @MILTER_SERVER_CONTEXT_ERROR_INVALID_STATE: Indicates
 * unexpected response is received on the current
 * %MilterServerContextState.
 * @MILTER_SERVER_CONTEXT_ERROR_BUSY: Indicates
 * a new operation is requested before the previous
 * operation's response is received.
 * @MILTER_SERVER_CONTEXT_ERROR_IO_ERROR: Indicates an IO
 * error causing on writing/reading milter protocol data.
 * @MILTER_SERVER_CONTEXT_ERROR_NEWER_VERSION_REQUESTED:
 * Indicates unsupported newer version is requested.
 *
 * These identify the variable errors that can occur while
 * calling %MilterServerContext functions.
 */
typedef enum
{
    MILTER_SERVER_CONTEXT_ERROR_CONNECTION_FAILURE,
    MILTER_SERVER_CONTEXT_ERROR_NO_SPEC,
    MILTER_SERVER_CONTEXT_ERROR_INVALID_STATE,
    MILTER_SERVER_CONTEXT_ERROR_BUSY,
    MILTER_SERVER_CONTEXT_ERROR_IO_ERROR,
    MILTER_SERVER_CONTEXT_ERROR_NEWER_VERSION_REQUESTED
} MilterServerContextError;

/**
 * MilterServerContextState:
 * @MILTER_SERVER_CONTEXT_STATE_INVALID: Invalid state.
 * @MILTER_SERVER_CONTEXT_STATE_START: Just started.
 * @MILTER_SERVER_CONTEXT_STATE_DEFINE_MACRO: Sent macro definition.
 * @MILTER_SERVER_CONTEXT_STATE_NEGOTIATE: Negotiating.
 * @MILTER_SERVER_CONTEXT_STATE_CONNECT: Sent connection
 * information.
 * @MILTER_SERVER_CONTEXT_STATE_HELO: Sent HELO information.
 * @MILTER_SERVER_CONTEXT_STATE_ENVELOPE_FROM: Sent MAIL
 * FROM command information.
 * @MILTER_SERVER_CONTEXT_STATE_ENVELOPE_RECIPIENT: Sent
 * RCPT TO command information.
 * @MILTER_SERVER_CONTEXT_STATE_DATA: Sent DATA command
 * information.
 * @MILTER_SERVER_CONTEXT_STATE_UNKNOWN: Sent unknown
 * SMTP command.
 * @MILTER_SERVER_CONTEXT_STATE_HEADER: Sent a header.
 * @MILTER_SERVER_CONTEXT_STATE_END_OF_HEADER: Sent all headers.
 * @MILTER_SERVER_CONTEXT_STATE_BODY: Sent a body chunk.
 * @MILTER_SERVER_CONTEXT_STATE_END_OF_MESSAGE: Sent all body
 * chunks.
 * @MILTER_SERVER_CONTEXT_STATE_QUIT: Sent quit request.
 * @MILTER_SERVER_CONTEXT_STATE_ABORT: Sent abort request.
 *
 * These identify the state of %MilterServerContext.
 */
typedef enum
{
    MILTER_SERVER_CONTEXT_STATE_INVALID,
    MILTER_SERVER_CONTEXT_STATE_START,
    MILTER_SERVER_CONTEXT_STATE_DEFINE_MACRO,
    MILTER_SERVER_CONTEXT_STATE_NEGOTIATE,
    MILTER_SERVER_CONTEXT_STATE_CONNECT,
    MILTER_SERVER_CONTEXT_STATE_HELO,
    MILTER_SERVER_CONTEXT_STATE_ENVELOPE_FROM,
    MILTER_SERVER_CONTEXT_STATE_ENVELOPE_RECIPIENT,
    MILTER_SERVER_CONTEXT_STATE_DATA,
    MILTER_SERVER_CONTEXT_STATE_UNKNOWN,
    MILTER_SERVER_CONTEXT_STATE_HEADER,
    MILTER_SERVER_CONTEXT_STATE_END_OF_HEADER,
    MILTER_SERVER_CONTEXT_STATE_BODY,
    MILTER_SERVER_CONTEXT_STATE_END_OF_MESSAGE,
    MILTER_SERVER_CONTEXT_STATE_QUIT,
    MILTER_SERVER_CONTEXT_STATE_ABORT
} MilterServerContextState;

typedef struct _MilterServerContext         MilterServerContext;
typedef struct _MilterServerContextClass    MilterServerContextClass;

struct _MilterServerContext
{
    MilterProtocolAgent object;
};

struct _MilterServerContextClass
{
    MilterProtocolAgentClass parent_class;

    gboolean (*stop_on_connect)    (MilterServerContext *context,
                                    const gchar *host_name,
                                    const struct sockaddr *address,
                                    socklen_t address_length);
    gboolean (*stop_on_helo)       (MilterServerContext *context,
                                    const gchar *fqdn);
    gboolean (*stop_on_envelope_from)
                                   (MilterServerContext *context,
                                    const gchar *from);
    gboolean (*stop_on_envelope_recipient)
                                   (MilterServerContext *context,
                                    const gchar *to);
    gboolean (*stop_on_data)       (MilterServerContext *context);
    gboolean (*stop_on_header)     (MilterServerContext *context,
                                    const gchar *name,
                                    const gchar *value);
    gboolean (*stop_on_end_of_header)
                                   (MilterServerContext *context);
    gboolean (*stop_on_body)       (MilterServerContext *context,
                                    const gchar *chunk,
                                    gsize size);
    gboolean (*stop_on_end_of_message)
                                   (MilterServerContext *context,
                                    const gchar *chunk,
                                    gsize size);

    void (*ready)               (MilterServerContext *context);
    void (*stopped)             (MilterServerContext *context);

    void (*connection_timeout)  (MilterServerContext *context);
    void (*writing_timeout)     (MilterServerContext *context);
    void (*reading_timeout)     (MilterServerContext *context);
    void (*end_of_message_timeout)
                                (MilterServerContext *context);

    void (*message_processed)   (MilterServerContext *context,
                                 MilterMessageResult *result);

    void (*state_transited)     (MilterServerContext *context,
                                 MilterServerContextState state);
};

GQuark               milter_server_context_error_quark (void);

GType                milter_server_context_get_type    (void) G_GNUC_CONST;

/**
 * milter_server_context_new:
 *
 * Creates a new context object.
 *
 * Returns: a new %MilterServerContext object.
 */
MilterServerContext *milter_server_context_new         (void);

/**
 * milter_server_context_set_connection_timeout:
 * @context: a %MilterServerContext.
 * @timeout: the connection timeout by seconds. (default is
 *           %MILTER_SERVER_CONTEXT_DEFAULT_CONNECTION_TIMEOUT)
 *
 * Sets the timeout by seconds on connection. If @context
 * doesn't connects to client in @timeout seconds,
 * #MilterServerContext::timeout signal is emitted.
 */
void                 milter_server_context_set_connection_timeout
                                                       (MilterServerContext *context,
                                                        gdouble timeout);

/**
 * milter_server_context_set_writing_timeout:
 * @context: a %MilterServerContext.
 * @timeout: the writing timeout by seconds. (default is
 *           %MILTER_SERVER_CONTEXT_DEFAULT_WRITING_TIMEOUT)
 *
 * Sets the timeout by seconds on writing. If @context
 * doesn't write to client socket in @timeout seconds,
 * #MilterServerContext::timeout signal is emitted.
 */
void                 milter_server_context_set_writing_timeout
                                                       (MilterServerContext *context,
                                                        gdouble timeout);

/**
 * milter_server_context_set_reading_timeout:
 * @context: a %MilterServerContext.
 * @timeout: the reading timeout by seconds. (default is
 *           %MILTER_SERVER_CONTEXT_DEFAULT_READING_TIMEOUT)
 *
 * Sets the timeout by seconds on reading. If @context
 * doesn't receive response from client socket in @timeout
 * seconds, #MilterServerContext::timeout signal is emitted.
 */
void                 milter_server_context_set_reading_timeout
                                                       (MilterServerContext *context,
                                                        gdouble timeout);

/**
 * milter_server_context_set_end_of_message_timeout:
 * @context: a %MilterServerContext.
 * @timeout: the timeout by seconds on end-of-message.
 *           (default is
 *           %MILTER_SERVER_CONTEXT_DEFAULT_END_OF_MESSAGE_TIMEOUT)
 *
 * Sets the timeout by seconds on end-of-message. If
 * @context doesn't receive response for end-of-message from
 * client socket in @timeout seconds,
 * #MilterServerContext::timeout signal is emitted.
 */
void                 milter_server_context_set_end_of_message_timeout
                                                       (MilterServerContext *context,
                                                        gdouble timeout);

/**
 * milter_server_context_set_connection_spec:
 * @context: a %MilterServerContext.
 * @spec: the connection spec of client.
 * @error: return location for an error, or %NULL.
 *
 * Sets a connection specification of client. If @spec is
 * invalid format and @error is not %NULL, error detail is
 * stored into @error.
 *
 * Returns: %TRUE on success.
 */
gboolean             milter_server_context_set_connection_spec
                                                       (MilterServerContext *context,
                                                        const gchar *spec,
                                                        GError **error);

/**
 * milter_server_context_establish_connection:
 * @context: a %MilterServerContext.
 * @error: return location for an error, or %NULL.
 *
 * Establishes a connection to client. If establishing is
 * failed and @error is not %NULL, error detail is stored
 * into @error.
 *
 * Returns: %TRUE on success.
 */
gboolean             milter_server_context_establish_connection
                                                       (MilterServerContext *context,
                                                        GError **error);


/**
 * milter_server_context_get_status:
 * @context: a %MilterServerContext.
 *
 * Gets the current status.
 *
 * Returns: the current status.
 */
MilterStatus         milter_server_context_get_status  (MilterServerContext *context);

/**
 * milter_server_context_set_status:
 * @context: a %MilterServerContext.
 * @status: the new status.
 *
 * Sets the current status.
 */
void                 milter_server_context_set_status  (MilterServerContext *context,
                                                        MilterStatus         status);


/**
 * milter_server_context_get_state:
 * @context: a %MilterServerContext.
 *
 * Gets the current state.
 *
 * Returns: the current state.
 */
MilterServerContextState milter_server_context_get_state (MilterServerContext *context);

/**
 * milter_server_context_set_state:
 * @context: a %MilterServerContext.
 * @state: the new state.
 *
 * Sets the current state.
 */
void                 milter_server_context_set_state   (MilterServerContext *context,
                                                        MilterServerContextState state);

/**
 * milter_server_context_get_last_state:
 * @context: a %MilterServerContext.
 *
 * Gets the last state. It's one of start, negotiate,
 * connect, helo, envelope-from, envelope-recipient, data,
 * unknown, header, end-of-header, body and end-of-message.
 *
 * Returns: the last state.
 */
MilterServerContextState milter_server_context_get_last_state (MilterServerContext *context);

/**
 * milter_server_context_is_processing:
 * @context: a %MilterServerContext.
 *
 * Gets whether waiting response.
 *
 * Returns: %TRUE if any response is received after the last
 * writing, %FALSE otherwise.
 */
gboolean             milter_server_context_is_processing
                                                       (MilterServerContext *context);

/**
 * milter_server_context_negotiate:
 * @context: a %MilterServerContext.
 * @option: the negotiate option.
 *
 * Negotiates with client.
 *
 * Returns: %TRUE on success.
 */
gboolean             milter_server_context_negotiate   (MilterServerContext *context,
                                                        MilterOption        *option);

/**
 * milter_server_context_connect:
 * @context: a %MilterServerContext.
 * @host_name: the host name of connected SMTP client.
 * @address: the address of connected SMTP client.
 * @address_length: the length of @address.
 *
 * Sends connected SMTP client information.
 *
 * Returns: %TRUE on success.
 */
gboolean             milter_server_context_connect     (MilterServerContext *context,
                                                        const gchar         *host_name,
                                                        struct sockaddr     *address,
                                                        socklen_t            address_length);

/**
 * milter_server_context_helo:
 * @context: a %MilterServerContext.
 * @fqdn: the FQDN.
 *
 * Sends the FQDN passed on HELO.
 *
 * Returns: %TRUE on success.
 */
gboolean             milter_server_context_helo        (MilterServerContext *context,
                                                        const gchar *fqdn);

/**
 * milter_server_context_envelope_from:
 * @context: a %MilterServerContext.
 * @from: the envelope from address.
 *
 * Sends the parameter passed on MAIL FROM.
 *
 * Returns: %TRUE on success.
 */
gboolean             milter_server_context_envelope_from
                                                       (MilterServerContext *context,
                                                        const gchar         *from);

/**
 * milter_server_context_envelope_recipient:
 * @context: a %MilterServerContext.
 * @recipient: the envelope recipient address.
 *
 * Sends the parameter passed on RCPT TO.
 *
 * Returns: %TRUE on success.
 */
gboolean             milter_server_context_envelope_recipient
                                                       (MilterServerContext *context,
                                                        const gchar         *recipient);

/**
 * milter_server_context_data:
 * @context: a %MilterServerContext.
 *
 * Notifies DATA is received.
 *
 * Returns: %TRUE on success.
 */
gboolean             milter_server_context_data        (MilterServerContext *context);

/**
 * milter_server_context_unknown:
 * @context: a %MilterServerContext.
 * @command: the unknown SMTP command.
 *
 * Sends received unknown SMTP command.
 *
 * Returns: %TRUE on success.
 */
gboolean             milter_server_context_unknown     (MilterServerContext *context,
                                                        const gchar         *command);

/**
 * milter_server_context_header:
 * @context: a %MilterServerContext.
 * @name: the header name.
 * @value: the header value.
 *
 * Sends a header.
 *
 * Returns: %TRUE on success.
 */
gboolean             milter_server_context_header      (MilterServerContext *context,
                                                        const gchar         *name,
                                                        const gchar         *value);

/**
 * milter_server_context_end_of_header:
 * @context: a %MilterServerContext.
 *
 * Notifies all headers are sent.
 *
 * Returns: %TRUE on success.
 */
gboolean             milter_server_context_end_of_header
                                                       (MilterServerContext *context);

/**
 * milter_server_context_body:
 * @context: a %MilterServerContext.
 * @chunk: the body chunk.
 * @size: the size of @chunk.
 *
 * Sends a body chunk.
 *
 * Returns: %TRUE on success.
 */
gboolean             milter_server_context_body        (MilterServerContext *context,
                                                        const gchar         *chunk,
                                                        gsize                size);

/**
 * milter_server_context_end_of_message:
 * @context: a %MilterServerContext.
 * @chunk: the body chunk. maybe %NULL.
 * @size: the size of @chunk.
 *
 * Notifies all body chunks are sent with optional the last
 * body chunk.
 *
 * Returns: %TRUE on success.
 */
gboolean             milter_server_context_end_of_message
                                                       (MilterServerContext *context,
                                                        const gchar         *chunk,
                                                        gsize                size);

/**
 * milter_server_context_quit:
 * @context: a %MilterServerContext.
 *
 * Quits the current connection.
 *
 * Returns: %TRUE on success.
 */
gboolean             milter_server_context_quit        (MilterServerContext *context);

/**
 * milter_server_context_abort:
 * @context: a %MilterServerContext.
 *
 * Aborts the current connection.
 *
 * Returns: %TRUE on success.
 */
gboolean             milter_server_context_abort       (MilterServerContext *context);

/**
 * milter_server_context_set_option:
 * @context: a %MilterServerContext.
 * @option: the negotiate option.
 *
 * Set negotiate option.
 */
void                 milter_server_context_set_option  (MilterServerContext *context,
                                                        MilterOption        *option);

/**
 * milter_server_context_get_option:
 * @context: a %MilterServerContext.
 *
 * Get negotiate option.
 */
MilterOption        *milter_server_context_get_option  (MilterServerContext *context);

/**
 * milter_server_context_is_enable_step:
 * @context: a %MilterServerContext.
 * @step: the step flag.
 *
 * Gets whether @step flag is enabled in the @context's option.
 *
 * Returns: %TRUE if @step flags is enabled, %FALSE otherwise.
 */
gboolean             milter_server_context_is_enable_step
                                                       (MilterServerContext *context,
                                                        MilterStepFlags      step);

/**
 * milter_server_context_get_skip_body:
 * @context: a %MilterServerContext.
 *
 * Gets whether @context received skip response on sending
 * body chunks.
 *
 * Returns: %TRUE if body chunks are skipped, %FALSE
 * otherwise.
 */
gboolean             milter_server_context_get_skip_body
                                                       (MilterServerContext *context);

/**
 * milter_server_context_get_name:
 * @context: a %MilterServerContext.
 *
 * Gets the name of @context.
 *
 * Returns: the name of @context.
 */
const gchar         *milter_server_context_get_name    (MilterServerContext *context);


/**
 * milter_server_context_set_name:
 * @context: a %MilterServerContext.
 * @name: the name.
 *
 * Sets the name of @context.
 */
void                 milter_server_context_set_name    (MilterServerContext *context,
                                                        const gchar *name);

/**
 * milter_server_context_get_elapsed:
 * @context: a %MilterServerContext.
 *
 * Gets the elapsed time.
 *
 * Returns: the elapsed time of @context.
 */
gdouble              milter_server_context_get_elapsed (MilterServerContext *context);

/**
 * milter_server_context_is_negotiated:
 * @context: a %MilterServerContext.
 *
 * Gets whether negotiation is succeeded.
 *
 * Returns: %TRUE if @context is negotiated, %FALSE otherwise.
 */
gboolean             milter_server_context_is_negotiated
                                                       (MilterServerContext *context);

/**
 * milter_server_context_is_processing_message:
 * @context: a %MilterServerContext.
 *
 * Gets whether the context is processing message.
 *
 * Returns: %TRUE if @context is processing message, %FALSE otherwise.
 */
gboolean             milter_server_context_is_processing_message
                                                       (MilterServerContext *context);

/**
 * milter_server_context_set_processing_message:
 * @context: a %MilterServerContext.
 * @processing: whether the context is processing message.
 *
 * Sets whether the context is processing message.
 */
void                 milter_server_context_set_processing_message
                                                       (MilterServerContext *context,
                                                        gboolean processing_message);

/**
 * milter_server_context_is_quitted:
 * @context: a %MilterServerContext.
 *
 * Gets whether the context is quitted.
 *
 * Returns: %TRUE if @context is quitted, %FALSE otherwise.
 */
gboolean             milter_server_context_is_quitted  (MilterServerContext *context);

/**
 * milter_server_context_set_quitted:
 * @context: a %MilterServerContext.
 * @quitted: whether the context is quitted.
 *
 * Sets whether the context is quitted.
 */
void                 milter_server_context_set_quitted (MilterServerContext *context,
                                                        gboolean quitted);

/**
 * milter_server_context_reset_message_related_data:
 * @context: a %MilterServerContext.
 *
 * Resets message related data in the context. It should be
 * called before each 2nd or later messages in the same
 * milter session. There are no bad effects if it is called
 * before 1st message.
 */
void                 milter_server_context_reset_message_related_data
                                                       (MilterServerContext *context);

/**
 * milter_server_context_get_message_result:
 * @context: a %MilterServerContext.
 *
 * Gets the message result of @context.
 *
 * Returns: the message result of @context.
 */
MilterMessageResult *milter_server_context_get_message_result
                                                       (MilterServerContext *context);


/**
 * milter_server_context_set_message_result:
 * @context: a %MilterServerContext.
 * @result: the message result.
 *
 * Sets the message result of @context.
 */
void                 milter_server_context_set_message_result
                                                       (MilterServerContext *context,
                                                        MilterMessageResult *result);

/**
 * milter_server_context_need_reply:
 * @context: a %MilterServerContext.
 * @state: a %MilterServerContextState.
 *
 * Gets whether the context needs to reply on @state.
 *
 * Returns: %TRUE if @context needs to reply on @state, %FALSE otherwise.
 */
gboolean             milter_server_context_need_reply (MilterServerContext *context,
                                                       MilterServerContextState state);

/**
 * milter_server_context_has_accepted_recipient:
 * @context: a %MilterServerContext.
 *
 * Returns %TRUE if the context has one or more accepted
 * recipients. If the context doesn't know about recipients,
 * it always returns %FALSE.
 *
 * Returns: %TRUE if @context has one or more accepted
 * recipients, %FALSE otherwise.
 */
gboolean             milter_server_context_has_accepted_recipient
                                                      (MilterServerContext *context);

G_END_DECLS

#endif /* __MILTER_SERVER_CONTEXT_H__ */

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
