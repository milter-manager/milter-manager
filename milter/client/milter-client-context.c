/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Copyright (C) 2008-2009  Kouhei Sutou <kou@cozmixng.org>
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

#include <milter/core.h>
#include <milter/core/milter-marshalers.h>
#include "../client.h"
#include "milter-client-context.h"
#include "milter-client-enum-types.h"

enum
{
    NEGOTIATE,
    NEGOTIATE_RESPONSE,
    CONNECT,
    CONNECT_RESPONSE,
    HELO,
    HELO_RESPONSE,
    ENVELOPE_FROM,
    ENVELOPE_FROM_RESPONSE,
    ENVELOPE_RECIPIENT,
    ENVELOPE_RECIPIENT_RESPONSE,
    DATA,
    DATA_RESPONSE,
    UNKNOWN,
    UNKNOWN_RESPONSE,
    HEADER,
    HEADER_RESPONSE,
    END_OF_HEADER,
    END_OF_HEADER_RESPONSE,
    BODY,
    BODY_RESPONSE,
    END_OF_MESSAGE,
    END_OF_MESSAGE_RESPONSE,
    ABORT,
    ABORT_RESPONSE,
    DEFINE_MACRO,

    TIMEOUT,
    LAST_SIGNAL
};

static gint signals[LAST_SIGNAL] = {0};

static gboolean    status_accumulator  (GSignalInvocationHint *hint,
                                        GValue *return_accumulator,
                                        const GValue *context_return,
                                        gpointer data);

#define MILTER_CLIENT_CONTEXT_GET_PRIVATE(obj)                          \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj),                                 \
                                 MILTER_TYPE_CLIENT_CONTEXT,            \
                                 MilterClientContextPrivate))

typedef struct _MilterClientContextPrivate	MilterClientContextPrivate;
struct _MilterClientContextPrivate
{
    gpointer private_data;
    GDestroyNotify private_data_destroy;
    MilterClientContextState state;
    MilterOption *option;
    guint reply_code;
    gchar *extended_reply_code;
    gchar *reply_message;
    guint timeout;
    guint timeout_id;
};

static void         finished           (MilterFinishedEmittable *emittable);

MILTER_IMPLEMENT_FINISHED_EMITTABLE_WITH_CODE(finished_emittable_init,
                                              iface->finished = finished)
G_DEFINE_TYPE_WITH_CODE(MilterClientContext, milter_client_context,
                        MILTER_TYPE_PROTOCOL_AGENT,
                        G_IMPLEMENT_INTERFACE(MILTER_TYPE_FINISHED_EMITTABLE,
                                              finished_emittable_init))

static void dispose        (GObject         *object);
static void set_property   (GObject         *object,
                            guint            prop_id,
                            const GValue    *value,
                            GParamSpec      *pspec);
static void get_property   (GObject         *object,
                            guint            prop_id,
                            GValue          *value,
                            GParamSpec      *pspec);

static MilterDecoder *decoder_new    (MilterAgent *agent);
static MilterEncoder *encoder_new    (MilterAgent *agent);

static MilterStatus default_negotiate  (MilterClientContext *context,
                                        MilterOption  *option,
                                        MilterMacrosRequests *macros_requests);
static MilterStatus default_connect    (MilterClientContext *context,
                                        const gchar   *host_name,
                                        struct sockaddr *address,
                                        socklen_t      address_length);
static MilterStatus default_helo       (MilterClientContext *context,
                                        const gchar   *fqdn);
static MilterStatus default_envelope_from
                                       (MilterClientContext *context,
                                        const gchar   *from);
static MilterStatus default_envelope_recipient
                                       (MilterClientContext *context,
                                        const gchar   *recipient);
static MilterStatus default_unknown    (MilterClientContext *context,
                                        const gchar   *command);
static MilterStatus default_data       (MilterClientContext *context);
static MilterStatus default_header     (MilterClientContext *context,
                                        const gchar   *name,
                                        const gchar   *value);
static MilterStatus default_end_of_header
                                       (MilterClientContext *context);
static MilterStatus default_body       (MilterClientContext *context,
                                        const gchar   *chunk,
                                        gsize          size);
static MilterStatus default_end_of_message
                                       (MilterClientContext *context);
static MilterStatus default_abort      (MilterClientContext *context);
static void         negotiate_response (MilterClientContext *context,
                                        MilterOption        *option,
                                        MilterMacrosRequests *macros_requests,
                                        MilterStatus        status);
static void         connect_response   (MilterClientContext *context,
                                        MilterStatus         status);
static void         helo_response      (MilterClientContext *context,
                                        MilterStatus         status);
static void         envelope_from_response
                                       (MilterClientContext *context,
                                        MilterStatus         status);
static void         envelope_recipient_response
                                       (MilterClientContext *context,
                                        MilterStatus         status);
static void         unknown_response   (MilterClientContext *context,
                                        MilterStatus         status);
static void         data_response      (MilterClientContext *context,
                                        MilterStatus         status);
static void         header_response    (MilterClientContext *context,
                                        MilterStatus         status);
static void         end_of_header_response
                                       (MilterClientContext *context,
                                        MilterStatus         status);
static void         body_response      (MilterClientContext *context,
                                        MilterStatus         status);
static void         end_of_message_response
                                       (MilterClientContext *context,
                                        MilterStatus         status);
static void         abort_response     (MilterClientContext *context,
                                        MilterStatus         status);

static void
milter_client_context_class_init (MilterClientContextClass *klass)
{
    GObjectClass *gobject_class;
    MilterAgentClass *agent_class;

    gobject_class = G_OBJECT_CLASS(klass);
    agent_class = MILTER_AGENT_CLASS(klass);

    gobject_class->dispose      = dispose;
    gobject_class->set_property = set_property;
    gobject_class->get_property = get_property;

    agent_class->decoder_new   = decoder_new;
    agent_class->encoder_new   = encoder_new;

    klass->negotiate = default_negotiate;
    klass->connect = default_connect;
    klass->helo = default_helo;
    klass->envelope_from = default_envelope_from;
    klass->envelope_recipient = default_envelope_recipient;
    klass->unknown = default_unknown;
    klass->data = default_data;
    klass->header = default_header;
    klass->end_of_header = default_end_of_header;
    klass->body = default_body;
    klass->end_of_message = default_end_of_message;
    klass->abort = default_abort;

    klass->negotiate_response = negotiate_response;
    klass->connect_response = connect_response;
    klass->helo_response = helo_response;
    klass->envelope_from_response = envelope_from_response;
    klass->envelope_recipient_response = envelope_recipient_response;
    klass->unknown_response = unknown_response;
    klass->data_response = data_response;
    klass->header_response = header_response;
    klass->end_of_header_response = end_of_header_response;
    klass->body_response = body_response;
    klass->end_of_message_response = end_of_message_response;
    klass->abort_response = abort_response;

    /**
     * MilterClientContext::negotiate:
     * @context: the context that received the signal.
     * @option: the negotiate option from MTA.
     * @macros_requests: the macros requests.
     *
     * This signal is emitted on negotiate request from MTA.
     * If you want to modify actions (%MilterActionFlags)
     * and steps (%MilterStepFlags), you modify @option. If
     * you want to add macros that you want to receive, you
     * modify @macros_requests.
     *
     * All available response statuses are the followings:
     *
     * <rd>
     * : %MILTER_STATUS_ALL_OPTIONS
     *    Enables all available actions and steps.
     *
     * : %MILTER_STATUS_REJECT
     *    Rejects the current session.
     *
     * : %MILTER_STATUS_CONTINUE
     *    Continues processing the current session with
     *    actions, steps and macros requests that are
     *    specified by @option and @macros_requests.
     *
     * : %MILTER_STATUS_PROGRESS
     *    It means that the processing in callback is in
     *    progress and returning response status is
     *    pending. The main loop for the milter is
     *    continued.
     *
     *    If you returns this status, you need to emit
     *    #MilterClientContext::negotiate-response signal by
     *    yourself with one of the above
     *    %MilterStatus<!-- -->es.
     *
     *    This status may be used for a milter that has IO
     *    wait. (e.g. The milter may need to connect to the
     *    its server like clamav-milter) Normally, this
     *    status will not be used.
     * </rd>
     *
     * See also <ulink
     * url="https://www.milter.org/developers/api/xxfi_negotiate">
     * xxfi_negotiate</ulink>
     * on milter.org.
     *
     * Returns: response status.
     */
    signals[NEGOTIATE] =
        g_signal_new("negotiate",
                     MILTER_TYPE_CLIENT_CONTEXT,
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterClientContextClass, negotiate),
                     status_accumulator, NULL,
                     _milter_marshal_ENUM__OBJECT_OBJECT,
                     MILTER_TYPE_STATUS, 2,
                     MILTER_TYPE_OPTION, MILTER_TYPE_MACROS_REQUESTS);

    /**
     * MilterClientContext::negotiate-response:
     * @context: the context that received the signal.
     * @option: the negotiate option from MTA.
     * @macros_requests: the macros requests.
     * @status: the response status.
     *
     * This signal is emitted implicitly after
     * #MilterClientContext::negotiate returns a status
     * except %MILTER_STATUS_PROGRESS. This signal's default
     * handler replies a response to MTA. You don't need to
     * connect this signal.
     *
     * If you returns %MILTER_STATUS_PROGRESS in
     * #MilterClientContext::negotiate handler, you need to
     * emit this signal to @context by yourself.
     */
    signals[NEGOTIATE_RESPONSE] =
        g_signal_new("negotiate-response",
                     MILTER_TYPE_CLIENT_CONTEXT,
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterClientContextClass,
                                     negotiate_response),
                     NULL, NULL,
                     _milter_marshal_VOID__OBJECT_OBJECT_ENUM,
                     G_TYPE_NONE, 3,
                     MILTER_TYPE_OPTION, MILTER_TYPE_MACROS_REQUESTS,
                     MILTER_TYPE_STATUS);

    /**
     * MilterClientContext::define-macro:
     * @context: the context that received the signal.
     * @command: the command to be defined macros.
     * @macros: the macro definitions.
     *
     * This signal is emitted when macro definition is
     * received. Normally, this signal isn't needed to be
     * connected.
     */
    signals[DEFINE_MACRO] =
        g_signal_new("define-macro",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterClientContextClass, define_macro),
                     NULL, NULL,
                     _milter_marshal_VOID__ENUM_POINTER,
                     G_TYPE_NONE, 2, MILTER_TYPE_COMMAND, G_TYPE_POINTER);

    /**
     * MilterClientContext::connect:
     * @context: the context that received the signal.
     * @host_name: the host name of connected SMTP client.
     * @address: the address of connected SMTP
     *           client. (struct sockaddr)
     * @address_size: the size of @address. (socklen_t)
     *
     * This signal is emitted on SMTP client is connected to
     * MTA.
     *
     * All available response statuses are the followings:
     *
     * <rd>
     * : %MILTER_STATUS_CONTINUE
     *    Continues processing the current connection.
     *
     * : %MILTER_STATUS_REJECT
     *    Rejects the current connection.
     *
     *    #MilterFinishedEmittable::finished will be emitted.
     *
     * : %MILTER_STATUS_ACCEPT
     *    Accepts the current connection without further
     *    more processing.
     *
     *    #MilterFinishedEmittable::finished will be emitted.
     *
     * : %MILTER_STATUS_TEMPORARY_FAILURE
     *    Rejects the current connection with a temporary
     *    failure. (i.e. 4xx status code in SMTP)
     *
     *    #MilterFinishedEmittable::finished will be emitted.
     *
     * : %MILTER_STATUS_NO_REPLY
     *    Doesn't send a reply back to MTA.
     *
     *    The milter must set %MILTER_STEP_NO_REPLY_CONNECT
     *    in #MilerClientContext::negotiate handler.
     *
     * : %MILTER_STATUS_PROGRESS
     *    It means that the processing in callback is in
     *    progress and returning response status is
     *    pending. The main loop for the milter is
     *    continued.
     *
     *    If you returns this status, you need to emit
     *    #MilterClientContext::connect-response signal by
     *    yourself with one of the above
     *    %MilterStatus<!-- -->es.
     *
     *    This status may be used for a milter that has IO
     *    wait. (e.g. The milter may need to connect to the
     *    its server like clamav-milter) Normally, this
     *    status will not be used.
     * </rd>
     *
     * See also <ulink
     * url="https://www.milter.org/developers/api/xxfi_connect">
     * xxfi_connect</ulink>
     * on milter.org.
     *
     * Returns: response status.
     */
    signals[CONNECT] =
        g_signal_new("connect",
                     MILTER_TYPE_CLIENT_CONTEXT,
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterClientContextClass, connect),
                     status_accumulator, NULL,
                     _milter_marshal_ENUM__STRING_POINTER_UINT,
                     MILTER_TYPE_STATUS, 3,
                     G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_UINT);

    /**
     * MilterClientContext::connect-response:
     * @context: the context that received the signal.
     * @status: the response status.
     *
     * This signal is emitted implicitly after
     * #MilterClientContext::connect returns a status except
     * %MILTER_STATUS_PROGRESS. This signal's default
     * handler replies a response to MTA. You don't need to
     * connect this signal.
     *
     * If you returns %MILTER_STATUS_PROGRESS in
     * #MilterClientContext::connect, you need to emit this
     * signal to @context by yourself.
     */
    signals[CONNECT_RESPONSE] =
        g_signal_new("connect-response",
                     MILTER_TYPE_CLIENT_CONTEXT,
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterClientContextClass,
                                     connect_response),
                     NULL, NULL,
                     _milter_marshal_VOID__ENUM,
                     G_TYPE_NONE, 1, MILTER_TYPE_STATUS);

    /**
     * MilterClientContext::helo:
     * @context: the context that received the signal.
     * @fqdn: the FQDN in SMTP's "HELO"/"EHLO" command.
     *
     * This signal is emitted on SMTP's "HELO"/"EHLO"
     * command.
     *
     * All available response statuses are the followings:
     *
     * <rd>
     * : %MILTER_STATUS_CONTINUE
     *    Continues processing the current connection.
     *
     * : %MILTER_STATUS_REJECT
     *    Rejects the current connection.
     *
     *    #MilterFinishedEmittable::finished will be emitted.
     *
     * : %MILTER_STATUS_ACCEPT
     *    Accepts the current connection without further
     *    more processing.
     *
     *    #MilterFinishedEmittable::finished will be emitted.
     *
     * : %MILTER_STATUS_TEMPORARY_FAILURE
     *    Rejects the current connection with a temporary
     *    failure. (i.e. 4xx status code in SMTP)
     *
     *    #MilterFinishedEmittable::finished will be emitted.
     *
     * : %MILTER_STATUS_NO_REPLY
     *    Doesn't send a reply back to MTA.
     *
     *    The milter must set %MILTER_STEP_NO_REPLY_HELO in
     *    #MilerClientContext::negotiate handler.
     *
     * : %MILTER_STATUS_PROGRESS
     *    It means that the processing in callback is in
     *    progress and returning response status is
     *    pending. The main loop for the milter is
     *    continued.
     *
     *    If you returns this status, you need to emit
     *    #MilterClientContext::helo-response signal by
     *    yourself with one of the above
     *    %MilterStatus<!-- -->es.
     *
     *    This status may be used for a milter that has IO
     *    wait. (e.g. The milter may need to connect to the
     *    its server like clamav-milter) Normally, this
     *    status will not be used.
     * </rd>
     *
     * See also <ulink
     * url="https://www.milter.org/developers/api/xxfi_helo">
     * xxfi_helo</ulink>
     * on milter.org.
     *
     * Returns: response status.
     */
    signals[HELO] =
        g_signal_new("helo",
                     MILTER_TYPE_CLIENT_CONTEXT,
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterClientContextClass, helo),
                     status_accumulator, NULL,
                     _milter_marshal_ENUM__STRING,
                     MILTER_TYPE_STATUS, 1, G_TYPE_STRING);

    /**
     * MilterClientContext::helo-response:
     * @context: the context that received the signal.
     * @status: the response status.
     *
     * This signal is emitted implicitly after
     * #MilterClientContext::helo returns a status except
     * %MILTER_STATUS_PROGRESS. This signal's default
     * handler replies a response to MTA. You don't need to
     * connect this signal.
     *
     * If you returns %MILTER_STATUS_PROGRESS in
     * #MilterClientContext::helo, you need to emit this
     * signal to @context by yourself.
     */
    signals[HELO_RESPONSE] =
        g_signal_new("helo-response",
                     MILTER_TYPE_CLIENT_CONTEXT,
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterClientContextClass, helo_response),
                     NULL, NULL,
                     _milter_marshal_VOID__ENUM,
                     G_TYPE_NONE, 1, MILTER_TYPE_STATUS);

    /**
     * MilterClientContext::envelope-from:
     * @context: the context that received the signal.
     * @from: the envelope from address in SMTP's "MAIL
     *        FROM" command.
     *
     * This signal is emitted on SMTP's "MAIL FROM" command.
     *
     * All available response statuses are the followings:
     *
     * <rd>
     * : %MILTER_STATUS_CONTINUE
     *    Continues processing the current message.
     *
     * : %MILTER_STATUS_REJECT
     *    Rejects the current envelope from address and
     *    message. A new envelope from may be specified.
     *
     *    #MilterClientContext::abort is not emitted.
     *
     * : %MILTER_STATUS_DISCARD
     *    Accepts the current message and discards it silently.
     *
     *    #MilterClientContext::abort is not emitted.
     *
     * : %MILTER_STATUS_ACCEPT
     *    Accepts the current message without further
     *    more processing.
     *
     *    #MilterClientContext::abort is not emitted.
     *
     * : %MILTER_STATUS_TEMPORARY_FAILURE
     *    Rejects the current envelope from address and
     *    message with temporary failure. (i.e. 4xx
     *    status code in SMTP) A new envelope from address
     *    may be specified.
     *
     *    #MilterClientContext::abort is not emitted.
     *
     * : %MILTER_STATUS_NO_REPLY
     *    Doesn't send a reply back to MTA.
     *
     *    The milter must set
     *    %MILTER_STEP_NO_REPLY_ENVELOPE_FROM in
     *    #MilerClientContext::negotiate handler.
     *
     * : %MILTER_STATUS_PROGRESS
     *    It means that the processing in callback is in
     *    progress and returning response status is
     *    pending. The main loop for the milter is
     *    continued.
     *
     *    If you returns this status, you need to emit
     *    #MilterClientContext::envelope-from-response
     *    signal by yourself with one of the above
     *    %MilterStatus<!-- -->es.
     *
     *    This status may be used for a milter that has IO
     *    wait. (e.g. The milter may need to connect to the
     *    its server like clamav-milter) Normally, this
     *    status will not be used.
     * </rd>
     *
     * See also <ulink
     * url="https://www.milter.org/developers/api/xxfi_envfrom">
     * xxfi_envfrom</ulink>
     * on milter.org.
     *
     * Returns: response status.
     */
    signals[ENVELOPE_FROM] =
        g_signal_new("envelope-from",
                     MILTER_TYPE_CLIENT_CONTEXT,
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterClientContextClass, envelope_from),
                     status_accumulator, NULL,
                     _milter_marshal_ENUM__STRING,
                     MILTER_TYPE_STATUS, 1, G_TYPE_STRING);

    /**
     * MilterClientContext::envelope-from-response:
     * @context: the context that received the signal.
     * @status: the response status.
     *
     * This signal is emitted implicitly after
     * #MilterClientContext::envelope-from returns a status
     * except %MILTER_STATUS_PROGRESS. This signal's default
     * handler replies a response to MTA. You don't need to
     * connect this signal.
     *
     * If you returns %MILTER_STATUS_PROGRESS in
     * #MilterClientContext::envelope-from, you need to emit
     * this signal to @context by yourself.
     */
    signals[ENVELOPE_FROM_RESPONSE] =
        g_signal_new("envelope-from-response",
                     MILTER_TYPE_CLIENT_CONTEXT,
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterClientContextClass,
                                     envelope_from_response),
                     NULL, NULL,
                     _milter_marshal_VOID__ENUM,
                     G_TYPE_NONE, 1, MILTER_TYPE_STATUS);

    /**
     * MilterClientContext::envelope-recipient:
     * @context: the context that received the signal.
     * @recipient: the envelope recipient address in SMTP's
     *             "RCPT TO" command.
     *
     * This signal is emitted on SMTP's "RCPT TO" command.
     *
     * All available response statuses are the followings:
     *
     * <rd>
     * : %MILTER_STATUS_CONTINUE
     *    Continues processing the current message.
     *
     * : %MILTER_STATUS_REJECT
     *    Rejects the current envelope recipient
     *    address. Processing the current messages is
     *    continued.
     *
     *    #MilterClientContext::abort is not emitted.
     *
     * : %MILTER_STATUS_DISCARD
     *    Accepts the current message and discards it silently.
     *
     *    #MilterClientContext::abort is not emitted.
     *
     * : %MILTER_STATUS_ACCEPT
     *    Accepts the current envelope recipient.
     *
     *    #MilterClientContext::abort is not emitted.
     *
     * : %MILTER_STATUS_TEMPORARY_FAILURE
     *    Rejects the current envelope recipient address
     *    with temporary failure. (i.e. 4xx status code in
     *    SMTP) Processing the current message is continued.
     *
     *    #MilterClientContext::abort is not emitted.
     *
     * : %MILTER_STATUS_NO_REPLY
     *    Doesn't send a reply back to MTA.
     *
     *    The milter must set
     *    %MILTER_STEP_NO_REPLY_ENVELOPE_RECIPIENT in
     *    #MilerClientContext::negotiate handler.
     *
     * : %MILTER_STATUS_PROGRESS
     *    It means that the processing in callback is in
     *    progress and returning response status is
     *    pending. The main loop for the milter is
     *    continued.
     *
     *    If you returns this status, you need to emit
     *    #MilterClientContext::envelope-recipient-response
     *    signal by yourself with one of the above
     *    %MilterStatus<!-- -->es.
     *
     *    This status may be used for a milter that has IO
     *    wait. (e.g. The milter may need to connect to the
     *    its server like clamav-milter) Normally, this
     *    status will not be used.
     * </rd>
     *
     * See also <ulink
     * url="https://www.milter.org/developers/api/xxfi_envrcpt">
     * xxfi_envrcpt</ulink>
     * on milter.org.
     *
     * Returns: response status.
     */
    signals[ENVELOPE_RECIPIENT] =
        g_signal_new("envelope-recipient",
                     MILTER_TYPE_CLIENT_CONTEXT,
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterClientContextClass,
                                     envelope_recipient),
                     status_accumulator, NULL,
                     _milter_marshal_ENUM__STRING,
                     MILTER_TYPE_STATUS, 1, G_TYPE_STRING);

    /**
     * MilterClientContext::envelope-recipient-response:
     * @context: the context that received the signal.
     * @status: the response status.
     *
     * This signal is emitted implicitly after
     * #MilterClientContext::envelope-recipient returns a
     * status except %MILTER_STATUS_PROGRESS. This signal's
     * default handler replies a response to MTA. You don't
     * need to connect this signal.
     *
     * If you returns %MILTER_STATUS_PROGRESS in
     * #MilterClientContext::envelope-recipient, you need to
     * emit this signal to @context by yourself.
     */
    signals[ENVELOPE_RECIPIENT_RESPONSE] =
        g_signal_new("envelope-recipient-response",
                     MILTER_TYPE_CLIENT_CONTEXT,
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterClientContextClass,
                                     envelope_recipient_response),
                     NULL, NULL,
                     _milter_marshal_VOID__ENUM,
                     G_TYPE_NONE, 1, MILTER_TYPE_STATUS);

    /**
     * MilterClientContext::data:
     * @context: the context that received the signal.
     *
     * This signal is emitted on SMTP's "DATA" command.
     *
     * All available response statuses are the followings:
     *
     * <rd>
     * : %MILTER_STATUS_CONTINUE
     *    Continues processing the current message.
     *
     * : %MILTER_STATUS_REJECT
     *    Rejects the current message.
     *
     *    #MilterClientContext::abort is not emitted.
     *
     * : %MILTER_STATUS_DISCARD
     *    Accepts the current message and discards it silently.
     *
     *    #MilterClientContext::abort is not emitted.
     *
     * : %MILTER_STATUS_ACCEPT
     *    Accepts the current envelope recipient.
     *
     *    #MilterClientContext::abort is not emitted.
     *
     * : %MILTER_STATUS_TEMPORARY_FAILURE
     *    Rejects the current message with temporary
     *    failure. (i.e. 4xx status code in SMTP)
     *
     *    #MilterClientContext::abort is not emitted.
     *
     * : %MILTER_STATUS_NO_REPLY
     *    Doesn't send a reply back to MTA.
     *
     *    The milter must set %MILTER_STEP_NO_REPLY_DATA
     *    in #MilerClientContext::negotiate handler.
     *
     * : %MILTER_STATUS_PROGRESS
     *    It means that the processing in callback is in
     *    progress and returning response status is
     *    pending. The main loop for the milter is
     *    continued.
     *
     *    If you returns this status, you need to emit
     *    #MilterClientContext::data-response
     *    signal by yourself with one of the above
     *    %MilterStatus<!-- -->es.
     *
     *    This status may be used for a milter that has IO
     *    wait. (e.g. The milter may need to connect to the
     *    its server like clamav-milter) Normally, this
     *    status will not be used.
     * </rd>
     *
     * See also <ulink
     * url="https://www.milter.org/developers/api/xxfi_data">
     * xxfi_data</ulink> on milter.org.
     *
     * Returns: response status.
     */
    signals[DATA] =
        g_signal_new("data",
                     MILTER_TYPE_CLIENT_CONTEXT,
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterClientContextClass, data),
                     status_accumulator, NULL,
                     _milter_marshal_ENUM__VOID,
                     MILTER_TYPE_STATUS, 0);

    /**
     * MilterClientContext::data-response:
     * @context: the context that received the signal.
     * @status: the response status.
     *
     * This signal is emitted implicitly after
     * #MilterClientContext::data returns a status except
     * %MILTER_STATUS_PROGRESS. This signal's default
     * handler replies a response to MTA. You don't need to
     * connect this signal.
     *
     * If you returns %MILTER_STATUS_PROGRESS in
     * #MilterClientContext::data, you need to emit this
     * signal to @context by yourself.
     */
    signals[DATA_RESPONSE] =
        g_signal_new("data-response",
                     MILTER_TYPE_CLIENT_CONTEXT,
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterClientContextClass, data_response),
                     NULL, NULL,
                     _milter_marshal_VOID__ENUM,
                     G_TYPE_NONE, 1, MILTER_TYPE_STATUS);

    /**
     * MilterClientContext::unknown:
     * @context: the context that received the signal.
     * @command: the unknown SMTP command.
     *
     * This signal is emitted on unknown or unimplemented
     * SMTP command is sent.
     *
     * All available response statuses are the followings:
     *
     * <rd>
     * : %MILTER_STATUS_REJECT
     *    Rejects the current message.
     *
     *    #MilterClientContext::abort is not emitted.
     *
     * : %MILTER_STATUS_TEMPORARY_FAILURE
     *    Rejects the current message with temporary
     *    failure. (i.e. 4xx status code in SMTP)
     *
     *    #MilterClientContext::abort is not emitted.
     *
     * : %MILTER_STATUS_NO_REPLY
     *    Doesn't send a reply back to MTA.
     *
     *    The milter must set %MILTER_STEP_NO_REPLY_DATA
     *    in #MilerClientContext::negotiate handler.
     *
     * : %MILTER_STATUS_PROGRESS
     *    It means that the processing in callback is in
     *    progress and returning response status is
     *    pending. The main loop for the milter is
     *    continued.
     *
     *    If you returns this status, you need to emit
     *    #MilterClientContext::unknown-response
     *    signal by yourself with one of the above
     *    %MilterStatus<!-- -->es.
     *
     *    This status may be used for a milter that has IO
     *    wait. (e.g. The milter may need to connect to the
     *    its server like clamav-milter) Normally, this
     *    status will not be used.
     * </rd>
     *
     * Note that the unknown or unimplemented SMTP command
     * will always be rejected by MTA.
     *
     * See also <ulink
     * url="https://www.milter.org/developers/api/xxfi_unknown">
     * xxfi_unknown</ulink> on milter.org.
     *
     * Returns: response status.
     */
    signals[UNKNOWN] =
        g_signal_new("unknown",
                     MILTER_TYPE_CLIENT_CONTEXT,
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterClientContextClass, unknown),
                     status_accumulator, NULL,
                     _milter_marshal_ENUM__STRING,
                     MILTER_TYPE_STATUS, 1, G_TYPE_STRING);

    /**
     * MilterClientContext::unknown-response:
     * @context: the context that received the signal.
     * @status: the response status.
     *
     * This signal is emitted implicitly after
     * #MilterClientContext::unknown returns a status except
     * %MILTER_STATUS_PROGRESS. This signal's default
     * handler replies a response to MTA. You don't need to
     * connect this signal.
     *
     * If you returns %MILTER_STATUS_PROGRESS in
     * #MilterClientContext::unknown, you need to emit this
     * signal to @context by yourself.
     */
    signals[UNKNOWN_RESPONSE] =
        g_signal_new("unknown-response",
                     MILTER_TYPE_CLIENT_CONTEXT,
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterClientContextClass, unknown_response),
                     NULL, NULL,
                     _milter_marshal_VOID__ENUM,
                     G_TYPE_NONE, 1, MILTER_TYPE_STATUS);

    /**
     * MilterClientContext::header:
     * @context: the context that received the signal.
     * @name: the header name.
     * @value: the header value. @value may include folded
     *         white space.
     *
     * This signal is emitted on each header. If
     * %MILTER_STEP_HEADER_LEADING_SPACE is set in
     * #MilterClientContext::negotiate, @value have spaces
     * after header name and value separator ":".
     *
     * Example:
     *
     * |[
     * From: from &lt;from@example.com&gt;
     * To: recipient &lt;recipient@example.com&gt;
     * Subject:a subject
     * ]|
     *
     * With %MILTER_STEP_HEADER_VALUE_WITH_LEADING_SPACE:
     *
     * |[
     * "From", " from &lt;from@example.com&gt;"
     * "To", " recipient &lt;recipient@example.com&gt;"
     * "Subject", "a subject"
     * ]|
     *
     * Without %MILTER_STEP_HEADER_VALUE_WITH_LEADING_SPACE:
     *
     * |[
     * "From", "from &lt;from@example.com&gt;"
     * "To", "recipient &lt;recipient@example.com&gt;"
     * "Subject", "a subject"
     * ]|
     *
     * All available response statuses are the followings:
     *
     * <rd>
     * : %MILTER_STATUS_CONTINUE
     *    Continues processing the current message.
     *
     * : %MILTER_STATUS_REJECT
     *    Rejects the current message.
     *
     *    #MilterClientContext::abort is not emitted.
     *
     * : %MILTER_STATUS_DISCARD
     *    Accepts the current message and discards it silently.
     *
     *    #MilterClientContext::abort is not emitted.
     *
     * : %MILTER_STATUS_ACCEPT
     *    Accepts the current message without further
     *    more processing.
     *
     *    #MilterClientContext::abort is not emitted.
     *
     * : %MILTER_STATUS_TEMPORARY_FAILURE
     *    Rejects the current message with temporary
     *    failure. (i.e. 4xx status code in SMTP)
     *
     *    #MilterClientContext::abort is not emitted.
     *
     * : %MILTER_STATUS_NO_REPLY
     *    Doesn't send a reply back to MTA.
     *
     *    The milter must set %MILTER_STEP_NO_REPLY_HEADER
     *    in #MilerClientContext::negotiate handler.
     *
     * : %MILTER_STATUS_PROGRESS
     *    It means that the processing in callback is in
     *    progress and returning response status is
     *    pending. The main loop for the milter is
     *    continued.
     *
     *    If you returns this status, you need to emit
     *    #MilterClientContext::header-response
     *    signal by yourself with one of the above
     *    %MilterStatus<!-- -->es.
     *
     *    This status may be used for a milter that has IO
     *    wait. (e.g. The milter may need to connect to the
     *    its server like clamav-milter) Normally, this
     *    status will not be used.
     * </rd>
     *
     * See also <ulink
     * url="https://www.milter.org/developers/api/xxfi_header">
     * xxfi_header</ulink> on milter.org.
     *
     * Returns: response status.
     */
    signals[HEADER] =
        g_signal_new("header",
                     MILTER_TYPE_CLIENT_CONTEXT,
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterClientContextClass, header),
                     status_accumulator, NULL,
                     _milter_marshal_ENUM__STRING_STRING,
                     MILTER_TYPE_STATUS, 2, G_TYPE_STRING, G_TYPE_STRING);

    /**
     * MilterClientContext::header-response:
     * @context: the context that received the signal.
     * @status: the response status.
     *
     * This signal is emitted implicitly after
     * #MilterClientContext::header returns a status except
     * %MILTER_STATUS_PROGRESS. This signal's default
     * handler replies a response to MTA. You don't need to
     * connect this signal.
     *
     * If you returns %MILTER_STATUS_PROGRESS in
     * #MilterClientContext::header, you need to emit this
     * signal to @context by yourself.
     */
    signals[HEADER_RESPONSE] =
        g_signal_new("header-response",
                     MILTER_TYPE_CLIENT_CONTEXT,
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterClientContextClass, header_response),
                     NULL, NULL,
                     _milter_marshal_VOID__ENUM,
                     G_TYPE_NONE, 1, MILTER_TYPE_STATUS);

    /**
     * MilterClientContext::end-of-header:
     * @context: the context that received the signal.
     *
     * This signal is emitted on all headers are processed.
     *
     * All available response statuses are the followings:
     *
     * <rd>
     * : %MILTER_STATUS_CONTINUE
     *    Continues processing the current message.
     *
     * : %MILTER_STATUS_REJECT
     *    Rejects the current message.
     *
     *    #MilterClientContext::abort is not emitted.
     *
     * : %MILTER_STATUS_DISCARD
     *    Accepts the current message and discards it silently.
     *
     *    #MilterClientContext::abort is not emitted.
     *
     * : %MILTER_STATUS_ACCEPT
     *    Accepts the current message without further
     *    more processing.
     *
     *    #MilterClientContext::abort is not emitted.
     *
     * : %MILTER_STATUS_TEMPORARY_FAILURE
     *    Rejects the current message with temporary
     *    failure. (i.e. 4xx status code in SMTP)
     *
     *    #MilterClientContext::abort is not emitted.
     *
     * : %MILTER_STATUS_NO_REPLY
     *    Doesn't send a reply back to MTA.
     *
     *    The milter must set
     *    %MILTER_STEP_NO_REPLY_END_OF_HEADER
     *    in #MilerClientContext::negotiate handler.
     *
     * : %MILTER_STATUS_PROGRESS
     *    It means that the processing in callback is in
     *    progress and returning response status is
     *    pending. The main loop for the milter is
     *    continued.
     *
     *    If you returns this status, you need to emit
     *    #MilterClientContext::end-of-header-response
     *    signal by yourself with one of the above
     *    %MilterStatus<!-- -->es.
     *
     *    This status may be used for a milter that has IO
     *    wait. (e.g. The milter may need to connect to the
     *    its server like clamav-milter) Normally, this
     *    status will not be used.
     * </rd>
     *
     * See also <ulink
     * url="https://www.milter.org/developers/api/xxfi_eof">
     * xxfi_eof</ulink> on milter.org.
     *
     * Returns: response status.
     */
    signals[END_OF_HEADER] =
        g_signal_new("end-of-header",
                     MILTER_TYPE_CLIENT_CONTEXT,
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterClientContextClass, end_of_header),
                     status_accumulator, NULL,
                     _milter_marshal_ENUM__VOID,
                     MILTER_TYPE_STATUS, 0);

    /**
     * MilterClientContext::end-of-header-response:
     * @context: the context that received the signal.
     * @status: the response status.
     *
     * This signal is emitted implicitly after
     * #MilterClientContext::end-of-header returns a status
     * except %MILTER_STATUS_PROGRESS. This signal's default
     * handler replies a response to MTA. You don't need to
     * connect this signal.
     *
     * If you returns %MILTER_STATUS_PROGRESS in
     * #MilterClientContext::end-of-header, you need to emit
     * this signal to @context by yourself.
     */
    signals[END_OF_HEADER_RESPONSE] =
        g_signal_new("end-of-header-response",
                     MILTER_TYPE_CLIENT_CONTEXT,
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterClientContextClass,
                                     end_of_header_response),
                     NULL, NULL,
                     _milter_marshal_VOID__ENUM,
                     G_TYPE_NONE, 1, MILTER_TYPE_STATUS);

    /**
     * MilterClientContext::body:
     * @context: the context that received the signal.
     * @chunk: the body chunk.
     * @size: the size of @chunk.
     *
     * This signal is emitted on body data is received. This
     * signal is emitted zero or more times between
     * #MilterClientContext::end-of-header and
     * #MilterClientContext::end-of-message.
     *
     * All available response statuses are the followings:
     *
     * <rd>
     * : %MILTER_STATUS_CONTINUE
     *    Continues processing the current message.
     *
     * : %MILTER_STATUS_REJECT
     *    Rejects the current message.
     *
     *    #MilterClientContext::abort is not emitted.
     *
     * : %MILTER_STATUS_DISCARD
     *    Accepts the current message and discards it silently.
     *
     *    #MilterClientContext::abort is not emitted.
     *
     * : %MILTER_STATUS_ACCEPT
     *    Accepts the current message without further
     *    more processing.
     *
     *    #MilterClientContext::abort is not emitted.
     *
     * : %MILTER_STATUS_TEMPORARY_FAILURE
     *    Rejects the current message with temporary
     *    failure. (i.e. 4xx status code in SMTP)
     *
     *    #MilterClientContext::abort is not emitted.
     *
     * : %MILTER_STATUS_SKIP
     *    Skips further body
     *    processing. #MilterClientContext::end-of-message
     *    is emitted.
     *
     * : %MILTER_STATUS_NO_REPLY
     *    Doesn't send a reply back to MTA.
     *
     *    The milter must set %MILTER_STEP_NO_REPLY_BODY
     *    in #MilerClientContext::negotiate handler.
     *
     * : %MILTER_STATUS_PROGRESS
     *    It means that the processing in callback is in
     *    progress and returning response status is
     *    pending. The main loop for the milter is
     *    continued.
     *
     *    If you returns this status, you need to emit
     *    #MilterClientContext::body-response
     *    signal by yourself with one of the above
     *    %MilterStatus<!-- -->es.
     *
     *    This status may be used for a milter that has IO
     *    wait. (e.g. The milter may need to connect to the
     *    its server like clamav-milter) Normally, this
     *    status will not be used.
     * </rd>
     *
     * See also <ulink
     * url="https://www.milter.org/developers/api/xxfi_body">
     * xxfi_body</ulink> on milter.org.
     *
     * Returns: response status.
     */
    signals[BODY] =
        g_signal_new("body",
                     MILTER_TYPE_CLIENT_CONTEXT,
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterClientContextClass, body),
                     status_accumulator, NULL,
                     _milter_marshal_ENUM__STRING_UINT,
                     MILTER_TYPE_STATUS, 2, G_TYPE_STRING, G_TYPE_UINT);

    /**
     * MilterClientContext::body-response:
     * @context: the context that received the signal.
     * @status: the response status.
     *
     * This signal is emitted implicitly after
     * #MilterClientContext::body returns a status except
     * %MILTER_STATUS_PROGRESS. This signal's default
     * handler replies a response to MTA. You don't need to
     * connect this signal.
     *
     * If you returns %MILTER_STATUS_PROGRESS in
     * #MilterClientContext::body, you need to emit this
     * signal to @context by yourself.
     */
    signals[BODY_RESPONSE] =
        g_signal_new("body_response",
                     MILTER_TYPE_CLIENT_CONTEXT,
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterClientContextClass, body_response),
                     NULL, NULL,
                     _milter_marshal_VOID__ENUM,
                     G_TYPE_NONE, 1, MILTER_TYPE_STATUS);

    /**
     * MilterClientContext::end-of-message:
     * @context: the context that received the signal.
     *
     * This signal is emitted after all
     * #MilterClientContext::body are emitted. All message
     * modifications can be done only in this signal
     * handler. The modifications can be done with
     * milter_client_context_add_header(),
     * milter_client_context_change_from() and so on.
     *
     * All available response statuses are the followings:
     *
     * <rd>
     * : %MILTER_STATUS_CONTINUE
     *    Continues processing the current message.
     *
     * : %MILTER_STATUS_DISCARD
     *    Accepts the current message and discards it silently.
     *
     *    #MilterClientContext::abort is not emitted.
     *
     * : %MILTER_STATUS_ACCEPT
     *    Accepts the current message without further
     *    more processing.
     *
     *    #MilterClientContext::abort is not emitted.
     *
     * : %MILTER_STATUS_TEMPORARY_FAILURE
     *    Rejects the current message with temporary
     *    failure. (i.e. 4xx status code in SMTP)
     *
     *    #MilterClientContext::abort is not emitted.
     *
     * : %MILTER_STATUS_PROGRESS
     *    It means that the processing in callback is in
     *    progress and returning response status is
     *    pending. The main loop for the milter is
     *    continued.
     *
     *    If you returns this status, you need to emit
     *    #MilterClientContext::end-of-message-response
     *    signal by yourself with one of the above
     *    %MilterStatus<!-- -->es.
     *
     *    This status may be used for a milter that has IO
     *    wait. (e.g. The milter may need to connect to the
     *    its server like clamav-milter) Normally, this
     *    status will not be used.
     * </rd>
     *
     * See also <ulink
     * url="https://www.milter.org/developers/api/xxfi_eom">
     * xxfi_eom</ulink> on milter.org.
     *
     * Returns: response status.
     */
    signals[END_OF_MESSAGE] =
        g_signal_new("end-of-message",
                     MILTER_TYPE_CLIENT_CONTEXT,
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterClientContextClass, end_of_message),
                     status_accumulator, NULL,
                     _milter_marshal_ENUM__VOID,
                     MILTER_TYPE_STATUS, 0);

    /**
     * MilterClientContext::end-of-message-response:
     * @context: the context that received the signal.
     * @status: the response status.
     *
     * This signal is emitted implicitly after
     * #MilterClientContext::end-of-message returns a status
     * except %MILTER_STATUS_PROGRESS. This signal's default
     * handler replies a response to MTA. You don't need to
     * connect this signal.
     *
     * If you returns %MILTER_STATUS_PROGRESS in
     * #MilterClientContext::end-of-message, you need to
     * emit this signal to @context by yourself.
     */
    signals[END_OF_MESSAGE_RESPONSE] =
        g_signal_new("end-of-message-response",
                     MILTER_TYPE_CLIENT_CONTEXT,
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterClientContextClass,
                                     end_of_message_response),
                     NULL, NULL,
                     _milter_marshal_VOID__ENUM,
                     G_TYPE_NONE, 1, MILTER_TYPE_STATUS);

    /**
     * MilterClientContext::abort:
     * @context: the context that received the signal.
     *
     * This signal may be emitted at any time between
     * #MilterClientContext::envelope-from and
     * #MilterClientContext::end-of-message. This signal is
     * only emitted if the milter causes an internal error
     * and the message processing isn't completed. For
     * example, if the milter has already returned
     * %MILTER_STATUS_ACCEPT, %MILTER_STATUS_REJECT,
     * %MILTER_STATUS_DISCARD and
     * %MILTER_STATUS_TEMPORARY_FAILURE, this signal will
     * not emitted.
     *
     * If the milter has any resources allocated for the
     * message between #MilterClientContext::envelope-from
     * and #MilterClientContext::end-of-message, should be
     * freed in this signal. But any resources allocated for
     * the connection should not be freed in this signal. It
     * should be freed in #MilterFinishedEmittable::finished.
     *
     * All available response statuses are the followings:
     *
     * <rd>
     * : %MILTER_STATUS_CONTINUE
     *    Continues processing the current message.
     *
     * : %MILTER_STATUS_DISCARD
     *    Accepts the current message and discards it silently.
     *
     *    #MilterClientContext::abort is not emitted.
     *
     * : %MILTER_STATUS_ACCEPT
     *    Accepts the current message without further
     *    more processing.
     *
     *    #MilterClientContext::abort is not emitted.
     *
     * : %MILTER_STATUS_TEMPORARY_FAILURE
     *    Rejects the current message with temporary
     *    failure. (i.e. 4xx status code in SMTP)
     *
     *    #MilterClientContext::abort is not emitted.
     *
     * : %MILTER_STATUS_PROGRESS
     *    It means that the processing in callback is in
     *    progress and returning response status is
     *    pending. The main loop for the milter is
     *    continued.
     *
     *    If you returns this status, you need to emit
     *    #MilterClientContext::end-of-message-response
     *    signal by yourself with one of the above
     *    %MilterStatus<!-- -->es.
     *
     *    This status may be used for a milter that has IO
     *    wait. (e.g. The milter may need to connect to the
     *    its server like clamav-milter) Normally, this
     *    status will not be used.
     * </rd>
     *
     * See also <ulink
     * url="https://www.milter.org/developers/api/xxfi_abort">
     * xxfi_abort</ulink> on milter.org.
     *
     * Returns: response status.
     */
    signals[ABORT] =
        g_signal_new("abort",
                     MILTER_TYPE_CLIENT_CONTEXT,
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterClientContextClass, abort),
                     status_accumulator, NULL,
                     _milter_marshal_ENUM__VOID,
                     MILTER_TYPE_STATUS, 0);

    /**
     * MilterClientContext::abort-response:
     * @context: the context that received the signal.
     * @status: the response status.
     *
     * This signal is emitted implicitly after
     * #MilterClientContext::abort returns a status except
     * %MILTER_STATUS_PROGRESS. This signal's default
     * handler replies a response to MTA. You don't need to
     * connect this signal.
     *
     * If you returns %MILTER_STATUS_PROGRESS in
     * #MilterClientContext::abort, you need to emit this
     * signal to @context by yourself.
     */
    signals[ABORT_RESPONSE] =
        g_signal_new("abort-response",
                     MILTER_TYPE_CLIENT_CONTEXT,
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterClientContextClass, abort_response),
                     NULL, NULL,
                     _milter_marshal_VOID__ENUM,
                     G_TYPE_NONE, 1, MILTER_TYPE_STATUS);

    /**
     * MilterClientContext::timeout:
     * @context: the context that received the signal.
     *
     * This signal is emitted if MTA doesn't return the next
     * command within milter_client_context_get_timeout()
     * seconds. If #MilterClientContext::timeout is emitted,
     * the current connection is aborted and
     * #MilterFinishedEmittable::finished are emitted.
     * #MilterClientContext::abort may be emitted before
     * #MilterFinishedEmittable::finished if the milter
     * is processing a message.
     */
    signals[TIMEOUT] =
        g_signal_new("timeout",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterClientContextClass, timeout),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__VOID,
                     G_TYPE_NONE, 0);

    g_type_class_add_private(gobject_class, sizeof(MilterClientContextPrivate));
}

static void
milter_client_context_init (MilterClientContext *context)
{
    MilterClientContextPrivate *priv;

    priv = MILTER_CLIENT_CONTEXT_GET_PRIVATE(context);
    priv->private_data = NULL;
    priv->private_data_destroy = NULL;
    priv->state = MILTER_CLIENT_CONTEXT_STATE_START;
    priv->option = NULL;
    priv->reply_code = 0;
    priv->extended_reply_code = NULL;
    priv->reply_message = NULL;
    priv->timeout = 7210;
    priv->timeout_id = 0;
}

static void
disable_timeout (MilterClientContext *context)
{
    MilterClientContextPrivate *priv;

    priv = MILTER_CLIENT_CONTEXT_GET_PRIVATE(context);

    if (priv->timeout_id > 0) {
        g_source_remove(priv->timeout_id);
        priv->timeout_id = 0;
    }
}

static void
dispose (GObject *object)
{
    MilterClientContextPrivate *priv;

    priv = MILTER_CLIENT_CONTEXT_GET_PRIVATE(object);

    disable_timeout(MILTER_CLIENT_CONTEXT(object));

    if (priv->private_data) {
        if (priv->private_data_destroy)
            priv->private_data_destroy(priv->private_data);
        priv->private_data = NULL;
    }
    priv->private_data_destroy = NULL;

    if (priv->option) {
        g_object_unref(priv->option);
        priv->option = NULL;
    }

    if (priv->extended_reply_code) {
        g_free(priv->extended_reply_code);
        priv->extended_reply_code = NULL;
    }

    if (priv->reply_message) {
        g_free(priv->reply_message);
        priv->reply_message = NULL;
    }

    G_OBJECT_CLASS(milter_client_context_parent_class)->dispose(object);
}

static void
set_property (GObject      *object,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
    MilterClientContextPrivate *priv;

    priv = MILTER_CLIENT_CONTEXT_GET_PRIVATE(object);
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
    MilterClientContextPrivate *priv;

    priv = MILTER_CLIENT_CONTEXT_GET_PRIVATE(object);
    switch (prop_id) {
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

GQuark
milter_client_context_error_quark (void)
{
    return g_quark_from_static_string("milter-client-context-error-quark");
}

MilterClientContext *
milter_client_context_new (void)
{
    return g_object_new(MILTER_TYPE_CLIENT_CONTEXT, NULL);
}

gpointer
milter_client_context_get_private_data (MilterClientContext *context)
{
    MilterClientContextPrivate *priv;

    priv = MILTER_CLIENT_CONTEXT_GET_PRIVATE(context);
    return priv->private_data;
}

void
milter_client_context_set_private_data (MilterClientContext *context,
                                        gpointer data,
                                        GDestroyNotify destroy)
{
    MilterClientContextPrivate *priv;

    priv = MILTER_CLIENT_CONTEXT_GET_PRIVATE(context);
    if (priv->private_data && priv->private_data_destroy) {
        priv->private_data_destroy(priv->private_data);
    }
    priv->private_data = data;
    priv->private_data_destroy = destroy;
}

static const gchar *
extended_code_parse_class (const gchar *extended_code,
                           const gchar *current_point,
                           guint *klass, GError **error)
{
    gint i;

    for (i = 0; current_point[i]; i++) {
        if (current_point[i] == '.') {
            if (i == 0) {
                g_set_error(error,
                            MILTER_CLIENT_CONTEXT_ERROR,
                            MILTER_CLIENT_CONTEXT_ERROR_INVALID_CODE,
                            "class of extended status code is missing: <%s>",
                            extended_code);
                return NULL;
            }
            *klass = atoi(current_point);
            return current_point + i + 1;
        } else if (i >= 1) {
            g_set_error(error,
                        MILTER_CLIENT_CONTEXT_ERROR,
                        MILTER_CLIENT_CONTEXT_ERROR_INVALID_CODE,
                        "class of extended status code should be '2', "
                        "'4' or '5': <%s>", extended_code);
            return NULL;
        } else if (!(current_point[i] == '2' ||
                     current_point[i] == '4' ||
                     current_point[i] == '5')) {
            g_set_error(error,
                        MILTER_CLIENT_CONTEXT_ERROR,
                        MILTER_CLIENT_CONTEXT_ERROR_INVALID_CODE,
                        "class of extended status code should be '2', "
                        "'4' or '5': <%s>: <%c>",
                        extended_code, current_point[i]);
            return NULL;
        }
    }

    if (i == 0) {
        g_set_error(error,
                    MILTER_CLIENT_CONTEXT_ERROR,
                    MILTER_CLIENT_CONTEXT_ERROR_INVALID_CODE,
                    "class of extended status code is missing: <%s>",
                    extended_code);
        return NULL;
    } else {
        return current_point + i;
    }
}

static const gchar *
extended_code_parse_subject (const gchar *extended_code,
                             const gchar *current_point,
                             guint *subject, GError **error)
{
    gint i;

    for (i = 0; current_point[i]; i++) {
        if (current_point[i] == '.') {
            if (i == 0) {
                g_set_error(error,
                            MILTER_CLIENT_CONTEXT_ERROR,
                            MILTER_CLIENT_CONTEXT_ERROR_INVALID_CODE,
                            "subject of extended status code is missing: <%s>",
                            extended_code);
                return NULL;
            }
            *subject = atoi(current_point);
            return current_point + i + 1;
        } else if (i >= 3) {
            gchar subject_component[5];

            strncpy(subject_component, current_point, 4);
            subject_component[4] = '\0';
            g_set_error(error,
                        MILTER_CLIENT_CONTEXT_ERROR,
                        MILTER_CLIENT_CONTEXT_ERROR_INVALID_CODE,
                        "subject of extended status code should be "
                        "less than 1000: <%s>: <%s>",
                        subject_component, extended_code);
            return NULL;
        } else if (!g_ascii_isdigit(current_point[i])) {
            g_set_error(error,
                        MILTER_CLIENT_CONTEXT_ERROR,
                        MILTER_CLIENT_CONTEXT_ERROR_INVALID_CODE,
                        "subject of extended status code should be digit: "
                        "<%s>: <%c>",
                        extended_code, current_point[i]);
            return NULL;
        }
    }

    if (i == 0) {
        g_set_error(error,
                    MILTER_CLIENT_CONTEXT_ERROR,
                    MILTER_CLIENT_CONTEXT_ERROR_INVALID_CODE,
                    "subject of extended status code is missing: <%s>",
                    extended_code);
        return NULL;
    } else {
        return current_point + i;
    }
}

static const gchar *
extended_code_parse_detail (const gchar *extended_code,
                            const gchar *current_point,
                            guint *detail, GError **error)
{
    gint i;

    for (i = 0; current_point[i]; i++) {
        if (i >= 3) {
            gchar detail_component[5];

            strncpy(detail_component, current_point, 4);
            detail_component[4] = '\0';
            g_set_error(error,
                        MILTER_CLIENT_CONTEXT_ERROR,
                        MILTER_CLIENT_CONTEXT_ERROR_INVALID_CODE,
                        "detail of extended status code should be "
                        "less than 1000: <%s>: <%s>",
                        detail_component, extended_code);
                return NULL;
        } else if (!g_ascii_isdigit(current_point[i])) {
            g_set_error(error,
                        MILTER_CLIENT_CONTEXT_ERROR,
                        MILTER_CLIENT_CONTEXT_ERROR_INVALID_CODE,
                        "detail of extended status code should be digit: "
                        "<%c>: <%s>",
                        current_point[i], extended_code);
            return NULL;
        }
    }

    if (i == 0) {
        g_set_error(error,
                    MILTER_CLIENT_CONTEXT_ERROR,
                    MILTER_CLIENT_CONTEXT_ERROR_INVALID_CODE,
                    "detail of extended status code is missing: <%s>",
                    extended_code);
        return NULL;
    } else {
        return current_point + i;
    }
}

static gboolean
extended_code_parse (const gchar *extended_code,
                     guint *klass, guint *subject, guint *detail,
                     GError **error)
{
    const gchar *current_point;

    current_point = extended_code;
    current_point = extended_code_parse_class(extended_code, current_point,
                                              klass, error);
    if (!current_point)
        return FALSE;
    if (*current_point == '\0') {
        g_set_error(error,
                    MILTER_CLIENT_CONTEXT_ERROR,
                    MILTER_CLIENT_CONTEXT_ERROR_INVALID_CODE,
                    "subject of extended status code is missing: <%s>",
                    extended_code);
        return FALSE;
    }

    current_point = extended_code_parse_subject(extended_code, current_point,
                                                subject, error);
    if (!current_point)
        return FALSE;
    if (*current_point == '\0') {
        g_set_error(error,
                    MILTER_CLIENT_CONTEXT_ERROR,
                    MILTER_CLIENT_CONTEXT_ERROR_INVALID_CODE,
                    "detail of extended status code is missing: <%s>",
                    extended_code);
        return FALSE;
    }

    current_point = extended_code_parse_detail(extended_code, current_point,
                                               detail, error);
    if (!current_point)
        return FALSE;

    return TRUE;
}

gboolean
milter_client_context_set_reply (MilterClientContext *context,
                                 guint code,
                                 const gchar *extended_code,
                                 const gchar *message,
                                 GError **error)
{
    MilterClientContextPrivate *priv;

    priv = MILTER_CLIENT_CONTEXT_GET_PRIVATE(context);

    if (!(400 <= code && code < 600)) {
        g_set_error(error,
                    MILTER_CLIENT_CONTEXT_ERROR,
                    MILTER_CLIENT_CONTEXT_ERROR_INVALID_CODE,
                    "return code should be 4XX or 5XX: <%u>", code);
        return FALSE;
    }

    if (extended_code) {
        guint klass = 0, subject, detail;

        if (!extended_code_parse(extended_code, &klass, &subject, &detail,
                                 error))
            return FALSE;

        if (klass != code / 100) {
            g_set_error(error,
                        MILTER_CLIENT_CONTEXT_ERROR,
                        MILTER_CLIENT_CONTEXT_ERROR_INVALID_CODE,
                        "extended code should use "
                        "the same class of status code: <%u>:<%s>",
                        code, extended_code);
            return FALSE;
        }
    }

    priv->reply_code = code;
    if (priv->extended_reply_code)
        g_free(priv->extended_reply_code);
    priv->extended_reply_code = g_strdup(extended_code);
    if (priv->reply_message)
        g_free(priv->reply_message);
    priv->reply_message = g_strdup(message);

    return TRUE;
}

gchar *
milter_client_context_format_reply (MilterClientContext *context)
{
    MilterClientContextPrivate *priv;

    priv = MILTER_CLIENT_CONTEXT_GET_PRIVATE(context);

    return milter_utils_format_reply_code(priv->reply_code,
                                          priv->extended_reply_code,
                                          priv->reply_message);
}

static gboolean
cb_timeout (gpointer data)
{
    g_signal_emit(data, signals[TIMEOUT], 0);
    milter_agent_shutdown(MILTER_AGENT(data));

    return FALSE;
}

static gboolean
write_packet (MilterClientContext *context, gchar *packet, gsize packet_size)
{
    MilterClientContextPrivate *priv;
    gboolean success;
    GError *agent_error = NULL;

    if (!packet)
        return FALSE;

    priv = MILTER_CLIENT_CONTEXT_GET_PRIVATE(context);

    disable_timeout(context);
    priv->timeout_id = milter_utils_timeout_add(priv->timeout,
                                                cb_timeout,
                                                context);
    success = milter_agent_write_packet(MILTER_AGENT(context),
                                        packet, packet_size,
                                        &agent_error);
    g_free(packet);
    if (agent_error) {
        GError *error = NULL;
        milter_utils_set_error_with_sub_error(&error,
                                              MILTER_CLIENT_CONTEXT_ERROR,
                                              MILTER_CLIENT_CONTEXT_ERROR_IO_ERROR,
                                              agent_error,
                                              "Failed to write to MTA");
        milter_error("[client][error][write] %s", error->message);
        milter_error_emittable_emit(MILTER_ERROR_EMITTABLE(context),
                                    error);
        g_error_free(error);
    }

    return success;
}

static gboolean
validate_state (const gchar *operation,
                MilterClientContextState expected_state,
                MilterClientContextState actual_state,
                GError **error)
{
    gchar *expected_state_nick;
    gchar *actual_state_nick;

    if (expected_state == actual_state)
        return TRUE;

    expected_state_nick =
        milter_utils_get_enum_nick_name(MILTER_TYPE_CLIENT_CONTEXT_STATE,
                                        expected_state);
    actual_state_nick =
        milter_utils_get_enum_nick_name(MILTER_TYPE_CLIENT_CONTEXT_STATE,
                                        actual_state);
    g_set_error(error,
                MILTER_CLIENT_CONTEXT_ERROR,
                MILTER_CLIENT_CONTEXT_ERROR_INVALID_STATE,
                "%s can only be done on %s state: <%s>",
                operation, expected_state_nick, actual_state_nick);
    g_free(expected_state_nick);
    g_free(actual_state_nick);

    return FALSE;
}

static gboolean
validate_action (const gchar *operation,
                 MilterActionFlags expected_action,
                 MilterOption *option,
                 GError **error)
{
    MilterActionFlags actual_action;
    gchar *expected_action_names;
    gchar *actual_action_names;

    actual_action = milter_option_get_action(option);
    if ((actual_action & expected_action) == expected_action)
        return TRUE;

    expected_action_names =
        milter_utils_get_flags_names(MILTER_TYPE_ACTION_FLAGS,
                                     expected_action);
    actual_action_names =
        milter_utils_get_flags_names(MILTER_TYPE_ACTION_FLAGS,
                                     actual_action);
    g_set_error(error,
                MILTER_CLIENT_CONTEXT_ERROR,
                MILTER_CLIENT_CONTEXT_ERROR_INVALID_ACTION,
                "%s can only be done with <%s> action flags: <%s>",
                operation, expected_action_names, actual_action_names);
    g_free(expected_action_names);
    g_free(actual_action_names);

    return FALSE;
}

gboolean
milter_client_context_add_header (MilterClientContext *context,
                                  const gchar *name, const gchar *value,
                                  GError **error)
{
    MilterClientContextPrivate *priv;
    gchar *packet = NULL;
    gsize packet_size;
    MilterEncoder *encoder;

    if (!name || !value) {
        g_set_error(error,
                    MILTER_CLIENT_CONTEXT_ERROR,
                    MILTER_CLIENT_CONTEXT_ERROR_NULL,
                    "both header name and value should not be NULL: <%s>=<%s>",
                    name ? name : "NULL",
                    value ? value : "NULL");
        return FALSE;
    }

    priv = MILTER_CLIENT_CONTEXT_GET_PRIVATE(context);
    if (!validate_state("add-header",
                        MILTER_CLIENT_CONTEXT_STATE_END_OF_MESSAGE,
                        priv->state,
                        error))
        return FALSE;

    if (!validate_action("add-header",
                         MILTER_ACTION_ADD_HEADERS,
                         priv->option,
                         error))
        return FALSE;

    milter_debug("[client][send][add-header] <%s>=<%s>", name, value);
    encoder = milter_agent_get_encoder(MILTER_AGENT(context));
    milter_reply_encoder_encode_add_header(MILTER_REPLY_ENCODER(encoder),
                                           &packet, &packet_size,
                                           name, value);
    return write_packet(context, packet, packet_size);
}

gboolean
milter_client_context_insert_header (MilterClientContext *context,
                                     guint32 index,
                                     const gchar *name, const gchar *value,
                                     GError **error)
{
    MilterClientContextPrivate *priv;
    gchar *packet = NULL;
    gsize packet_size;
    MilterEncoder *encoder;

    if (!name || !value) {
        g_set_error(error,
                    MILTER_CLIENT_CONTEXT_ERROR,
                    MILTER_CLIENT_CONTEXT_ERROR_NULL,
                    "both header name and value should not be NULL: <%s>=<%s>",
                    name ? name : "NULL",
                    value ? value : "NULL");
        return FALSE;
    }

    priv = MILTER_CLIENT_CONTEXT_GET_PRIVATE(context);
    if (!validate_state("insert-header",
                        MILTER_CLIENT_CONTEXT_STATE_END_OF_MESSAGE,
                        priv->state,
                        error))
        return FALSE;

    if (!validate_action("insert-header",
                         MILTER_ACTION_ADD_HEADERS,
                         priv->option,
                         error))
        return FALSE;

    milter_debug("[client][send][insert-header] [%u]<%s>=<%s>",
                 index, name, value);

    encoder = milter_agent_get_encoder(MILTER_AGENT(context));
    milter_reply_encoder_encode_insert_header(MILTER_REPLY_ENCODER(encoder),
                                              &packet, &packet_size,
                                              index, name, value);
    return write_packet(context, packet, packet_size);
}

gboolean
milter_client_context_change_header (MilterClientContext *context,
                                     const gchar *name, guint32 index,
                                     const gchar *value)
{
    gchar *packet = NULL;
    gsize packet_size;
    MilterEncoder *encoder;

    milter_debug("[client][send][change-header] <%s>[%u]=<%s>",
                 name, index, value);

    encoder = milter_agent_get_encoder(MILTER_AGENT(context));
    milter_reply_encoder_encode_change_header(MILTER_REPLY_ENCODER(encoder),
                                              &packet, &packet_size,
                                              name, index, value);
    return write_packet(context, packet, packet_size);
}

gboolean
milter_client_context_delete_header (MilterClientContext *context,
                                     const gchar *name, guint32 index)
{
    gchar *packet = NULL;
    gsize packet_size;
    MilterEncoder *encoder;

    milter_debug("[client][send][delete-header] <%s>[%u]", name, index);

    encoder = milter_agent_get_encoder(MILTER_AGENT(context));
    milter_reply_encoder_encode_delete_header(MILTER_REPLY_ENCODER(encoder),
                                              &packet, &packet_size,
                                              name, index);
    return write_packet(context, packet, packet_size);
}

gboolean
milter_client_context_change_from (MilterClientContext *context,
                                   const gchar *from,
                                   const gchar *parameters)
{
    MilterEncoder *encoder;
    gchar *packet = NULL;
    gsize packet_size;

    milter_debug("[client][send][change-from] <%s>:<%s>",
                 from,
                 parameters ? parameters : "NULL");

    encoder = milter_agent_get_encoder(MILTER_AGENT(context));
    milter_reply_encoder_encode_change_from(MILTER_REPLY_ENCODER(encoder),
                                            &packet, &packet_size,
                                            from, parameters);
    return write_packet(context, packet, packet_size);
}

gboolean
milter_client_context_add_recipient (MilterClientContext *context,
                                     const gchar *recipient,
                                     const gchar *parameters)
{
    MilterEncoder *encoder;
    gchar *packet = NULL;
    gsize packet_size;

    milter_debug("[client][send][add-recipient] <%s>:<%s>",
                 recipient,
                 parameters ? parameters : "NULL");

    encoder = milter_agent_get_encoder(MILTER_AGENT(context));
    milter_reply_encoder_encode_add_recipient(MILTER_REPLY_ENCODER(encoder),
                                              &packet, &packet_size,
                                              recipient, parameters);
    return write_packet(context, packet, packet_size);
}

gboolean
milter_client_context_delete_recipient (MilterClientContext *context,
                                        const gchar *recipient)
{
    MilterEncoder *encoder;
    gchar *packet = NULL;
    gsize packet_size;

    milter_debug("[client][send][delete-recipient] <%s>", recipient);

    encoder = milter_agent_get_encoder(MILTER_AGENT(context));
    milter_reply_encoder_encode_delete_recipient(MILTER_REPLY_ENCODER(encoder),
                                                 &packet, &packet_size,
                                                 recipient);
    return write_packet(context, packet, packet_size);
}

gboolean
milter_client_context_replace_body (MilterClientContext *context,
                                    const gchar *body, gsize body_size)
{
    gsize rest_size, packed_size = 0;
    MilterEncoder *encoder;
    MilterReplyEncoder *reply_encoder;

    milter_debug("[client][send][replace-body] <%" G_GSIZE_FORMAT ">",
                 body_size);

    encoder = milter_agent_get_encoder(MILTER_AGENT(context));
    reply_encoder = MILTER_REPLY_ENCODER(encoder);
    for (rest_size = body_size; rest_size > 0; rest_size -= packed_size) {
        gchar *packet = NULL;
        gsize packet_size;
        gsize offset;

        offset = body_size - rest_size;
        milter_reply_encoder_encode_replace_body(reply_encoder,
                                                 &packet, &packet_size,
                                                 body + offset,
                                                 rest_size,
                                                 &packed_size);
        if (packed_size == 0)
            return FALSE;

        if (!write_packet(context, packet, packet_size))
            return FALSE;
    }

    return TRUE;
}

gboolean
milter_client_context_progress (MilterClientContext *context)
{
    MilterEncoder *encoder;
    gchar *packet = NULL;
    gsize packet_size;

    milter_debug("[client][send][progress]");

    encoder = milter_agent_get_encoder(MILTER_AGENT(context));
    milter_reply_encoder_encode_progress(MILTER_REPLY_ENCODER(encoder),
                                         &packet, &packet_size);

    return write_packet(context, packet, packet_size);
}

gboolean
milter_client_context_quarantine (MilterClientContext *context,
                                  const gchar *reason)
{
    MilterEncoder *encoder;
    gchar *packet = NULL;
    gsize packet_size;

    milter_debug("[client][reply][quarantine] <%s>", reason);

    /* quarantine allows only on end-of-message. */
    milter_statistics("[reply][end-of-message][quarantine]");

    encoder = milter_agent_get_encoder(MILTER_AGENT(context));
    milter_reply_encoder_encode_quarantine(MILTER_REPLY_ENCODER(encoder),
                                           &packet, &packet_size,
                                           reason);

    return write_packet(context, packet, packet_size);
}

static gboolean
reply (MilterClientContext *context, MilterStatus status)
{
    gchar *packet = NULL;
    gsize packet_size;
    MilterEncoder *encoder;
    MilterReplyEncoder *reply_encoder;

    encoder = milter_agent_get_encoder(MILTER_AGENT(context));
    reply_encoder = MILTER_REPLY_ENCODER(encoder);

    switch (status) {
    case MILTER_STATUS_CONTINUE:
        milter_reply_encoder_encode_continue(reply_encoder,
                                             &packet, &packet_size);
        break;
    case MILTER_STATUS_TEMPORARY_FAILURE:
    case MILTER_STATUS_REJECT:
    {
            gchar *code;

            code = milter_client_context_format_reply(context);
            if (code) {
                milter_reply_encoder_encode_reply_code(reply_encoder,
                                                       &packet, &packet_size,
                                                       code);
                g_free(code);
            } else {
                if (status == MILTER_STATUS_TEMPORARY_FAILURE)
                    milter_reply_encoder_encode_temporary_failure(reply_encoder,
                                                                  &packet,
                                                                  &packet_size);
                else
                    milter_reply_encoder_encode_reject(reply_encoder,
                                                       &packet, &packet_size);
            }
            break;
    }
    case MILTER_STATUS_ACCEPT:
        milter_reply_encoder_encode_accept(reply_encoder, &packet, &packet_size);
        break;
    case MILTER_STATUS_DISCARD:
        milter_reply_encoder_encode_discard(reply_encoder,
                                            &packet, &packet_size);
        break;
    case MILTER_STATUS_SKIP:
        milter_reply_encoder_encode_skip(reply_encoder, &packet, &packet_size);
        break;
    default:
        break;
    }

    if (!packet)
        return TRUE;

    return write_packet(context, packet, packet_size);
}

static MilterStatus
default_negotiate (MilterClientContext *context, MilterOption *option,
                   MilterMacrosRequests *macros_requests)
{
    return MILTER_STATUS_NOT_CHANGE;
}

static MilterStatus
default_connect (MilterClientContext *context, const gchar *host_name,
                 struct sockaddr *address, socklen_t address_length)
{
    return MILTER_STATUS_NOT_CHANGE;
}

static MilterStatus
default_helo (MilterClientContext *context, const gchar *fqdn)
{
    return MILTER_STATUS_NOT_CHANGE;
}

static MilterStatus
default_envelope_from (MilterClientContext *context, const gchar *from)
{
    return MILTER_STATUS_NOT_CHANGE;
}

static MilterStatus
default_envelope_recipient (MilterClientContext *context, const gchar *recipient)
{
    return MILTER_STATUS_NOT_CHANGE;
}

static MilterStatus
default_unknown (MilterClientContext *context, const gchar *command)
{
    return MILTER_STATUS_NOT_CHANGE;
}

static MilterStatus
default_data (MilterClientContext *context)
{
    return MILTER_STATUS_NOT_CHANGE;
}

static MilterStatus
default_header (MilterClientContext *context, const gchar *name, const gchar *value)
{
    return MILTER_STATUS_NOT_CHANGE;
}

static MilterStatus
default_end_of_header (MilterClientContext *context)
{
    return MILTER_STATUS_NOT_CHANGE;
}

static MilterStatus
default_body (MilterClientContext *context, const gchar *chunk, gsize size)
{
    return MILTER_STATUS_NOT_CHANGE;
}

static MilterStatus
default_end_of_message (MilterClientContext *context)
{
    return MILTER_STATUS_NOT_CHANGE;
}

static MilterStatus
default_abort (MilterClientContext *context)
{
    return MILTER_STATUS_NOT_CHANGE;
}

static void
set_macro_context (MilterClientContext *context, MilterCommand macro_context)
{
    MilterProtocolAgent *agent;

    agent = MILTER_PROTOCOL_AGENT(context);
    milter_protocol_agent_set_macro_context(agent, macro_context);
}

static void
negotiate_response (MilterClientContext *context,
                    MilterOption *option, MilterMacrosRequests *macros_requests,
                    MilterStatus status)
{
    gchar *packet;
    gsize packet_size;
    MilterEncoder *encoder;
    MilterClientContextPrivate *priv;
    gchar *status_name;

    if (status == MILTER_STATUS_DEFAULT)
        status = MILTER_STATUS_ALL_OPTIONS;

    priv = MILTER_CLIENT_CONTEXT_GET_PRIVATE(context);
    /* FIXME: should check the previous state */
    priv->state = MILTER_CLIENT_CONTEXT_STATE_NEGOTIATE_REPLIED;

    switch (status) {
    case MILTER_STATUS_CONTINUE:
        milter_client_context_set_option(context, option);
    case MILTER_STATUS_ALL_OPTIONS:
    {
        gchar *inspected_option;

        inspected_option = milter_option_inspect(option);
        milter_debug("[client][reply][negotiate] %s", inspected_option);
        g_free(inspected_option);

        encoder = milter_agent_get_encoder(MILTER_AGENT(context));

        milter_reply_encoder_encode_negotiate(MILTER_REPLY_ENCODER(encoder),
                                              &packet, &packet_size,
                                              priv->option, macros_requests);
        write_packet(MILTER_CLIENT_CONTEXT(context), packet, packet_size);
        break;
    }
    case MILTER_STATUS_REJECT:
        encoder = milter_agent_get_encoder(MILTER_AGENT(context));
        milter_reply_encoder_encode_reject(MILTER_REPLY_ENCODER(encoder),
                                           &packet, &packet_size);
        write_packet(MILTER_CLIENT_CONTEXT(context), packet, packet_size);
        break;
    default:
        /* FIXME: error */
        break;
    }

    status_name = milter_utils_get_enum_nick_name(MILTER_TYPE_STATUS, status);
    milter_statistics("[reply][negotiate][%s]", status_name);
    g_free(status_name);
}

static void
connect_response (MilterClientContext *context, MilterStatus status)
{
    MilterClientContextPrivate *priv;
    gchar *status_name;

    if (status == MILTER_STATUS_DEFAULT)
        status = MILTER_STATUS_CONTINUE;

    priv = MILTER_CLIENT_CONTEXT_GET_PRIVATE(context);
    /* FIXME: should check the previous state */
    priv->state = MILTER_CLIENT_CONTEXT_STATE_CONNECT_REPLIED;

    reply(context, status);
    status_name = milter_utils_get_enum_nick_name(MILTER_TYPE_STATUS, status);
    milter_statistics("[reply][connect][%s]", status_name);
    g_free(status_name);
}

static void
helo_response (MilterClientContext *context, MilterStatus status)
{
    MilterClientContextPrivate *priv;
    gchar *status_name;

    if (status == MILTER_STATUS_DEFAULT)
        status = MILTER_STATUS_CONTINUE;

    priv = MILTER_CLIENT_CONTEXT_GET_PRIVATE(context);
    /* FIXME: should check the previous state */
    priv->state = MILTER_CLIENT_CONTEXT_STATE_HELO_REPLIED;

    reply(context, status);
    status_name = milter_utils_get_enum_nick_name(MILTER_TYPE_STATUS, status);
    milter_statistics("[reply][helo][%s]", status_name);
    g_free(status_name);
}

static void
envelope_from_response (MilterClientContext *context, MilterStatus status)
{
    MilterClientContextPrivate *priv;
    gchar *status_name;

    if (status == MILTER_STATUS_DEFAULT)
        status = MILTER_STATUS_CONTINUE;

    priv = MILTER_CLIENT_CONTEXT_GET_PRIVATE(context);
    /* FIXME: should check the previous state */
    priv->state = MILTER_CLIENT_CONTEXT_STATE_ENVELOPE_FROM_REPLIED;

    reply(context, status);

    status_name = milter_utils_get_enum_nick_name(MILTER_TYPE_STATUS, status);
    milter_statistics("[reply][evenlope-from][%s]", status_name);
    g_free(status_name);
}

static void
envelope_recipient_response (MilterClientContext *context, MilterStatus status)
{
    MilterClientContextPrivate *priv;
    gchar *status_name;

    if (status == MILTER_STATUS_DEFAULT)
        status = MILTER_STATUS_CONTINUE;

    priv = MILTER_CLIENT_CONTEXT_GET_PRIVATE(context);
    /* FIXME: should check the previous state */
    priv->state = MILTER_CLIENT_CONTEXT_STATE_ENVELOPE_RECIPIENT_REPLIED;

    reply(context, status);
    status_name = milter_utils_get_enum_nick_name(MILTER_TYPE_STATUS, status);
    milter_statistics("[reply][evenlope-recipient][%s]", status_name);
    g_free(status_name);
}

static void
unknown_response (MilterClientContext *context, MilterStatus status)
{
    MilterClientContextPrivate *priv;
    gchar *status_name;

    if (status == MILTER_STATUS_DEFAULT)
        status = MILTER_STATUS_CONTINUE;

    priv = MILTER_CLIENT_CONTEXT_GET_PRIVATE(context);
    /* FIXME: should check the previous state */
    priv->state = MILTER_CLIENT_CONTEXT_STATE_UNKNOWN_REPLIED;

    reply(context, status);
    status_name = milter_utils_get_enum_nick_name(MILTER_TYPE_STATUS, status);
    milter_statistics("[reply][unknown][%s]", status_name);
    g_free(status_name);
}

static void
data_response (MilterClientContext *context, MilterStatus status)
{
    MilterClientContextPrivate *priv;
    gchar *status_name;

    if (status == MILTER_STATUS_DEFAULT)
        status = MILTER_STATUS_CONTINUE;

    priv = MILTER_CLIENT_CONTEXT_GET_PRIVATE(context);
    /* FIXME: should check the previous state */
    priv->state = MILTER_CLIENT_CONTEXT_STATE_DATA_REPLIED;

    reply(context, status);
    status_name = milter_utils_get_enum_nick_name(MILTER_TYPE_STATUS, status);
    milter_statistics("[reply][data][%s]", status_name);
    g_free(status_name);
}

static void
header_response (MilterClientContext *context, MilterStatus status)
{
    MilterClientContextPrivate *priv;
    gchar *status_name;

    if (status == MILTER_STATUS_DEFAULT)
        status = MILTER_STATUS_CONTINUE;

    priv = MILTER_CLIENT_CONTEXT_GET_PRIVATE(context);
    /* FIXME: should check the previous state */
    priv->state = MILTER_CLIENT_CONTEXT_STATE_HEADER_REPLIED;

    reply(context, status);
    status_name = milter_utils_get_enum_nick_name(MILTER_TYPE_STATUS, status);
    milter_statistics("[reply][header][%s]", status_name);
    g_free(status_name);
}

static void
end_of_header_response (MilterClientContext *context, MilterStatus status)
{
    MilterClientContextPrivate *priv;
    gchar *status_name;

    if (status == MILTER_STATUS_DEFAULT)
        status = MILTER_STATUS_CONTINUE;

    priv = MILTER_CLIENT_CONTEXT_GET_PRIVATE(context);
    /* FIXME: should check the previous state */
    priv->state = MILTER_CLIENT_CONTEXT_STATE_END_OF_HEADER_REPLIED;

    reply(context, status);
    status_name = milter_utils_get_enum_nick_name(MILTER_TYPE_STATUS, status);
    milter_statistics("[reply][end-of-header][%s]", status_name);
    g_free(status_name);
}

static void
body_response (MilterClientContext *context, MilterStatus status)
{
    MilterClientContextPrivate *priv;
    gchar *status_name;

    if (status == MILTER_STATUS_DEFAULT)
        status = MILTER_STATUS_CONTINUE;

    priv = MILTER_CLIENT_CONTEXT_GET_PRIVATE(context);
    /* FIXME: should check the previous state */
    priv->state = MILTER_CLIENT_CONTEXT_STATE_BODY_REPLIED;

    reply(context, status);
    status_name = milter_utils_get_enum_nick_name(MILTER_TYPE_STATUS, status);
    milter_statistics("[reply][body][%s]", status_name);
    g_free(status_name);
}

static void
end_of_message_response (MilterClientContext *context, MilterStatus status)
{
    MilterClientContextPrivate *priv;
    gchar *status_name;

    if (status == MILTER_STATUS_DEFAULT)
        status = MILTER_STATUS_CONTINUE;

    priv = MILTER_CLIENT_CONTEXT_GET_PRIVATE(context);
    /* FIXME: should check the previous state */
    priv->state = MILTER_CLIENT_CONTEXT_STATE_END_OF_MESSAGE_REPLIED;

    reply(context, status);
    status_name = milter_utils_get_enum_nick_name(MILTER_TYPE_STATUS, status);
    milter_statistics("[reply][end-of-message][%s]", status_name);
    g_free(status_name);
}

static void
abort_response (MilterClientContext *context, MilterStatus status)
{
    MilterClientContextPrivate *priv;

    if (status == MILTER_STATUS_DEFAULT)
        status = MILTER_STATUS_CONTINUE;

    priv = MILTER_CLIENT_CONTEXT_GET_PRIVATE(context);
    /* FIXME: should check the previous state */
    priv->state = MILTER_CLIENT_CONTEXT_STATE_ABORT_REPLIED;
}

static void
finished (MilterFinishedEmittable *emittable)
{
    MilterClientContext *context;
    MilterClientContextPrivate *priv;

    context = MILTER_CLIENT_CONTEXT(emittable);
    priv = MILTER_CLIENT_CONTEXT_GET_PRIVATE(context);
    /* FIXME: should check the previous state */
    priv->state = MILTER_CLIENT_CONTEXT_STATE_FINISHED;

    if (finished_emittable_parent->finished)
        finished_emittable_parent->finished(emittable);
}

static void
cb_decoder_negotiate (MilterDecoder *decoder, MilterOption *option,
                      gpointer user_data)
{
    MilterStatus status = MILTER_STATUS_NOT_CHANGE;
    MilterClientContext *context = MILTER_CLIENT_CONTEXT(user_data);
    MilterOption *copied_option;
    MilterMacrosRequests *macros_requests;
    MilterClientContextPrivate *priv;

    priv = MILTER_CLIENT_CONTEXT_GET_PRIVATE(context);
    /* FIXME: should check the previous state */
    priv->state = MILTER_CLIENT_CONTEXT_STATE_NEGOTIATE;

    disable_timeout(context);
    copied_option = milter_option_copy(option);
    milter_client_context_set_option(context, copied_option);
    g_object_unref(copied_option);
    macros_requests = milter_macros_requests_new();
    g_signal_emit(context, signals[NEGOTIATE], 0,
                  option, macros_requests, &status);
    if (status != MILTER_STATUS_PROGRESS)
        g_signal_emit(context, signals[NEGOTIATE_RESPONSE], 0,
                      option, macros_requests, status);
    g_object_unref(macros_requests);
}

static void
cb_decoder_define_macro (MilterDecoder *decoder, MilterCommand macro_context,
                         GHashTable *macros, gpointer user_data)
{
    MilterClientContext *context = user_data;

    disable_timeout(context);
    milter_protocol_agent_set_macros_hash_table(MILTER_PROTOCOL_AGENT(context),
                                                macro_context, macros);
    g_signal_emit(context, signals[DEFINE_MACRO], 0, macro_context, macros);
}

static void
cb_decoder_connect (MilterDecoder *decoder, const gchar *host_name,
                    struct sockaddr *address, socklen_t address_length,
                    gpointer user_data)
{
    MilterClientContext *context = user_data;
    MilterClientContextPrivate *priv;
    MilterStatus status;

    priv = MILTER_CLIENT_CONTEXT_GET_PRIVATE(context);
    /* FIXME: should check the previous state */
    priv->state = MILTER_CLIENT_CONTEXT_STATE_CONNECT;

    disable_timeout(context);
    set_macro_context(context, MILTER_COMMAND_CONNECT);
    g_signal_emit(context, signals[CONNECT], 0,
                  host_name, address, address_length, &status);
    if (status == MILTER_STATUS_PROGRESS)
        return;
    g_signal_emit(context, signals[CONNECT_RESPONSE], 0, status);
}

static void
cb_decoder_helo (MilterDecoder *decoder, const gchar *fqdn, gpointer user_data)
{
    MilterClientContext *context = user_data;
    MilterClientContextPrivate *priv;
    MilterStatus status;

    priv = MILTER_CLIENT_CONTEXT_GET_PRIVATE(context);
    /* FIXME: should check the previous state */
    priv->state = MILTER_CLIENT_CONTEXT_STATE_HELO;

    disable_timeout(context);
    set_macro_context(context, MILTER_COMMAND_HELO);
    g_signal_emit(context, signals[HELO], 0, fqdn, &status);
    if (status == MILTER_STATUS_PROGRESS)
        return;
    g_signal_emit(context, signals[HELO_RESPONSE], 0, status);
}

static void
cb_decoder_envelope_from (MilterDecoder *decoder,
                          const gchar *from, gpointer user_data)
{
    MilterClientContext *context = user_data;
    MilterClientContextPrivate *priv;
    MilterStatus status;

    priv = MILTER_CLIENT_CONTEXT_GET_PRIVATE(context);
    /* FIXME: should check the previous state */
    priv->state = MILTER_CLIENT_CONTEXT_STATE_ENVELOPE_FROM;

    disable_timeout(context);
    set_macro_context(context, MILTER_COMMAND_ENVELOPE_FROM);
    g_signal_emit(context, signals[ENVELOPE_FROM], 0, from, &status);
    if (status == MILTER_STATUS_PROGRESS)
        return;
    g_signal_emit(context, signals[ENVELOPE_FROM_RESPONSE], 0, status);
}

static void
cb_decoder_envelope_recipient (MilterDecoder *decoder,
                               const gchar *to, gpointer user_data)
{
    MilterClientContext *context = user_data;
    MilterClientContextPrivate *priv;
    MilterStatus status;

    priv = MILTER_CLIENT_CONTEXT_GET_PRIVATE(context);
    /* FIXME: should check the previous state */
    priv->state = MILTER_CLIENT_CONTEXT_STATE_ENVELOPE_RECIPIENT;

    disable_timeout(context);
    set_macro_context(context, MILTER_COMMAND_ENVELOPE_RECIPIENT);
    g_signal_emit(context, signals[ENVELOPE_RECIPIENT], 0, to, &status);
    if (status == MILTER_STATUS_PROGRESS)
        return;
    g_signal_emit(context, signals[ENVELOPE_RECIPIENT_RESPONSE], 0, status);
}

static void
cb_decoder_unknown (MilterDecoder *decoder,
                    const gchar *command, gpointer user_data)
{
    MilterClientContext *context = user_data;
    MilterClientContextPrivate *priv;
    MilterStatus status;

    priv = MILTER_CLIENT_CONTEXT_GET_PRIVATE(context);
    /* FIXME: should check the previous state */
    priv->state = MILTER_CLIENT_CONTEXT_STATE_UNKNOWN;

    disable_timeout(context);
    g_signal_emit(context, signals[UNKNOWN], 0, command, &status);
    if (status == MILTER_STATUS_PROGRESS)
        return;
    g_signal_emit(context, signals[UNKNOWN_RESPONSE], 0, status);
}

static void
cb_decoder_data (MilterDecoder *decoder, gpointer user_data)
{
    MilterClientContext *context = user_data;
    MilterClientContextPrivate *priv;
    MilterStatus status;

    priv = MILTER_CLIENT_CONTEXT_GET_PRIVATE(context);
    /* FIXME: should check the previous state */
    priv->state = MILTER_CLIENT_CONTEXT_STATE_DATA;

    disable_timeout(context);
    set_macro_context(context, MILTER_COMMAND_DATA);
    g_signal_emit(context, signals[DATA], 0, &status);
    if (status == MILTER_STATUS_PROGRESS)
        return;
    g_signal_emit(context, signals[DATA_RESPONSE], 0, status);
}

static void
cb_decoder_header (MilterDecoder *decoder,
                   const gchar *name, const gchar *value,
                   gpointer user_data)
{
    MilterClientContext *context = user_data;
    MilterClientContextPrivate *priv;
    MilterStatus status;

    priv = MILTER_CLIENT_CONTEXT_GET_PRIVATE(context);
    /* FIXME: should check the previous state */
    priv->state = MILTER_CLIENT_CONTEXT_STATE_HEADER;

    disable_timeout(context);
    set_macro_context(context, MILTER_COMMAND_HEADER);
    g_signal_emit(context, signals[HEADER], 0, name, value, &status);
    if (status == MILTER_STATUS_PROGRESS)
        return;
    g_signal_emit(context, signals[HEADER_RESPONSE], 0, status);
}

static void
cb_decoder_end_of_header (MilterDecoder *decoder, gpointer user_data)
{
    MilterClientContext *context = user_data;
    MilterClientContextPrivate *priv;
    MilterStatus status;

    priv = MILTER_CLIENT_CONTEXT_GET_PRIVATE(context);
    /* FIXME: should check the previous state */
    priv->state = MILTER_CLIENT_CONTEXT_STATE_END_OF_HEADER;

    disable_timeout(context);
    set_macro_context(context, MILTER_COMMAND_END_OF_HEADER);
    g_signal_emit(context, signals[END_OF_HEADER], 0, &status);
    if (status == MILTER_STATUS_PROGRESS)
        return;
    g_signal_emit(context, signals[END_OF_HEADER_RESPONSE], 0, status);
}

static void
cb_decoder_body (MilterDecoder *decoder, const gchar *chunk, gsize chunk_size,
                 gpointer user_data)
{
    MilterClientContext *context = user_data;
    MilterClientContextPrivate *priv;
    MilterStatus status;

    priv = MILTER_CLIENT_CONTEXT_GET_PRIVATE(context);
    /* FIXME: should check the previous state */
    priv->state = MILTER_CLIENT_CONTEXT_STATE_BODY;

    disable_timeout(context);
    set_macro_context(context, MILTER_COMMAND_BODY);
    g_signal_emit(context, signals[BODY], 0, chunk, chunk_size, &status);
    if (status == MILTER_STATUS_PROGRESS)
        return;
    g_signal_emit(context, signals[BODY_RESPONSE], 0, status);
}

static void
cb_decoder_end_of_message (MilterDecoder *decoder, const gchar *chunk,
                           gsize chunk_size, gpointer user_data)
{
    MilterClientContext *context = user_data;
    MilterClientContextPrivate *priv;
    MilterStatus status;

    priv = MILTER_CLIENT_CONTEXT_GET_PRIVATE(context);

    disable_timeout(context);
    if (chunk && chunk_size > 0) {
        /* FIXME: should check the previous state */
        priv->state = MILTER_CLIENT_CONTEXT_STATE_BODY;
        set_macro_context(context, MILTER_COMMAND_BODY);
        g_signal_emit(context, signals[BODY], 0, chunk, chunk_size, &status);
        if (status != MILTER_STATUS_PROGRESS)
            g_signal_emit(context, signals[BODY_RESPONSE], 0, status);
        else
            /* FIXME: how handle this situation? */;
    }

    /* FIXME: should check the previous state */
    priv->state = MILTER_CLIENT_CONTEXT_STATE_END_OF_MESSAGE;
    set_macro_context(context, MILTER_COMMAND_END_OF_MESSAGE);
    g_signal_emit(context, signals[END_OF_MESSAGE], 0, &status);
    if (status == MILTER_STATUS_PROGRESS)
        return;
    g_signal_emit(context, signals[END_OF_MESSAGE_RESPONSE], 0, status);
}

static void
cb_decoder_quit (MilterDecoder *decoder, gpointer user_data)
{
    MilterClientContext *context;
    MilterClientContextPrivate *priv;

    context = MILTER_CLIENT_CONTEXT(user_data);

    priv = MILTER_CLIENT_CONTEXT_GET_PRIVATE(context);
    /* FIXME: should check the previous state */
    priv->state = MILTER_CLIENT_CONTEXT_STATE_QUIT;

    milter_agent_shutdown(MILTER_AGENT(context));
}

static void
cb_decoder_abort (MilterDecoder *decoder, gpointer user_data)
{
    MilterClientContext *context = MILTER_CLIENT_CONTEXT(user_data);
    MilterClientContextPrivate *priv;
    MilterStatus status;

    priv = MILTER_CLIENT_CONTEXT_GET_PRIVATE(context);
    /* FIXME: should check the previous state */
    priv->state = MILTER_CLIENT_CONTEXT_STATE_ABORT;

    disable_timeout(context);
    g_signal_emit(context, signals[ABORT], 0, &status);
    if (status == MILTER_STATUS_PROGRESS)
        return;
    g_signal_emit(context, signals[ABORT_RESPONSE], 0, status);
}

static MilterDecoder *
decoder_new (MilterAgent *agent)
{
    MilterDecoder *decoder;

    decoder = milter_command_decoder_new();

#define CONNECT(name)                                                   \
    g_signal_connect(decoder, #name, G_CALLBACK(cb_decoder_ ## name),   \
                     agent)

    CONNECT(negotiate);
    CONNECT(define_macro);
    CONNECT(connect);
    CONNECT(helo);
    CONNECT(envelope_from);
    CONNECT(envelope_recipient);
    CONNECT(unknown);
    CONNECT(data);
    CONNECT(header);
    CONNECT(end_of_header);
    CONNECT(body);
    CONNECT(end_of_message);
    CONNECT(quit);
    CONNECT(abort);

#undef CONNECT

    return decoder;
}

static MilterEncoder *
encoder_new (MilterAgent *agent)
{
    return milter_reply_encoder_new();
}

static gboolean
status_accumulator (GSignalInvocationHint *hint,
                    GValue *return_accumulator,
                    const GValue *context_return,
                    gpointer data)
{
    MilterStatus status;

    status = g_value_get_enum(context_return);
    if (status != MILTER_STATUS_NOT_CHANGE)
        g_value_set_enum(return_accumulator, status);
    status = g_value_get_enum(return_accumulator);

    return status == MILTER_STATUS_DEFAULT ||
        status == MILTER_STATUS_CONTINUE ||
        status == MILTER_STATUS_ALL_OPTIONS;
}

gboolean
milter_client_context_feed (MilterClientContext *context,
                            const gchar *chunk, gsize size,
                            GError **error)
{
    MilterDecoder *decoder;

    decoder = milter_agent_get_decoder(MILTER_AGENT(context));
    return milter_decoder_decode(decoder, chunk, size, error);
}

void
milter_client_context_set_timeout (MilterClientContext *context,
                                   guint timeout)
{
    MILTER_CLIENT_CONTEXT_GET_PRIVATE(context)->timeout = timeout;
}

guint
milter_client_context_get_timeout (MilterClientContext *context)
{
    return MILTER_CLIENT_CONTEXT_GET_PRIVATE(context)->timeout;
}

void
milter_client_context_set_state (MilterClientContext *context,
                                 MilterClientContextState state)
{
    MILTER_CLIENT_CONTEXT_GET_PRIVATE(context)->state = state;
}

MilterClientContextState
milter_client_context_get_state (MilterClientContext *context)
{
    return MILTER_CLIENT_CONTEXT_GET_PRIVATE(context)->state;
}

void
milter_client_context_set_option (MilterClientContext *context,
                                  MilterOption *option)
{
    MilterClientContextPrivate *priv;

    priv = MILTER_CLIENT_CONTEXT_GET_PRIVATE(context);
    if (priv->option)
        g_object_unref(priv->option);
    priv->option = option;
    if (priv->option)
        g_object_ref(priv->option);
}

MilterOption *
milter_client_context_get_option (MilterClientContext *context)
{
    return MILTER_CLIENT_CONTEXT_GET_PRIVATE(context)->option;
}


/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
