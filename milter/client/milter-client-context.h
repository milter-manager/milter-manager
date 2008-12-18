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

#ifndef __MILTER_CLIENT_CONTEXT_H__
#define __MILTER_CLIENT_CONTEXT_H__

#include <glib-object.h>

#include <milter/core.h>

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
 *    #MilterClientContext::abort
 * : ((<xxfi_close|URL:https://www.milter.org/developers/api/xxfi_close>))
 *    #MilterClientContext::quit
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
 * cb_end_of_message (MilterClientContext *context, gpointer user_data)
 * {
 *     g_print("end-of-message\n");
 *     return MILTER_STATUS_CONTINUE;
 * }
 * 
 * static MilterStatus
 * cb_abort (MilterClientContext *context, gpointer user_data)
 * {
 *     g_print("abort\n");
 *     return MILTER_STATUS_CONTINUE;
 * }
 * 
 * static MilterStatus
 * cb_quit (MilterClientContext *context, gpointer user_data)
 * {
 *     g_print("quit\n");
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
 *     CONNECT(quit);
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
 *
 * These identify the variable errors that can occur while
 * calling %MilterClientContext functions.
 */
typedef enum
{
    MILTER_CLIENT_CONTEXT_ERROR_INVALID_CODE,
    MILTER_CLIENT_CONTEXT_ERROR_IO_ERROR
} MilterClientContextError;

typedef struct _MilterClientContext         MilterClientContext;
typedef struct _MilterClientContextClass    MilterClientContextClass;

struct _MilterClientContext
{
    MilterProtocolAgent object;
};

struct _MilterClientContextClass
{
    MilterProtocolAgentClass parent_class;

    MilterStatus (*negotiate)          (MilterClientContext *context,
                                        MilterOption  *option);
    void         (*negotiate_response) (MilterClientContext *context,
                                        MilterOption  *option,
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
    MilterStatus (*end_of_message)     (MilterClientContext *context);
    void         (*end_of_message_response)
                                       (MilterClientContext *context,
                                        MilterStatus         status);
    MilterStatus (*quit)               (MilterClientContext *context);
    void         (*quit_response)      (MilterClientContext *context,
                                        MilterStatus         status);
    MilterStatus (*abort)              (MilterClientContext *context);
    void         (*abort_response)     (MilterClientContext *context,
                                        MilterStatus         status);
    void         (*mta_timeout)        (MilterClientContext *context);
};

GQuark               milter_client_context_error_quark       (void);

GType                milter_client_context_get_type          (void) G_GNUC_CONST;

/**
 * milter_client_context_new:
 *
 * Creates a new context object. Normally, context object is
 * created by %MilterClient and passed by
 * #MilterClient::connection-established signal.
 *
 * Returns: a new %MilterClientContext object.
 */
MilterClientContext *milter_client_context_new               (void);

/**
 * milter_client_context_feed:
 * @context: a %MilterClientContext.
 * @chunk: the string to be fed to @context.
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
 * when new private data is set or @context is @destroyed.
 */
void                 milter_client_context_set_private_data  (MilterClientContext *context,
                                                              gpointer data,
                                                              GDestroyNotify destroy);

/**
 * milter_client_context_set_reply:
 * @context: a %MilterClientContext.
 * @code: the three-digit SMTP error reply
 * code. (RFC 2821) Only 4xx and 5xx are accepted.
 * @extended_code: the extended reply code (RFC 1893/2034),
 * or %NULL.
 * @message: the text part of the SMTP reply, or %NULL.
 * @error: return location for an error, or %NULL.
 *
 * Sets the error reply code. Set error reply code with 4xx
 * @code is used on %MILTER_REPLY_TEMPORARY_FAILURE. Set
 * error reply reply code with 5xx @code is used on
 * %MILTER_REPLY_REJECT. See also <link
 * linked="https://www.milter.org/developers/api/smfi_setreply">smfi_setreply()
 * on milter.org</link>.
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
 * milter_client_context_negotiate_reply:
 * @context: a %MilterClientContext.
 * @option: the negotiate option.
 * @macros_requests: the macro requests.
 *
 * Formats the current error reply code specified by
 * milter_client_context_set_reply(). If error reply code
 * isn't set, this function returns %NULL.
 *
 * Returns: formatted reply code, or %NULL.
 */
gboolean             milter_client_context_negotiate_reply   (MilterClientContext *context,
                                                              MilterOption         *option,
                                                              MilterMacrosRequests *macros_requests);
gboolean             milter_client_context_add_header        (MilterClientContext *context,
                                                              const gchar *name,
                                                              const gchar *value);
gboolean             milter_client_context_insert_header     (MilterClientContext *context,
                                                              guint32      index,
                                                              const gchar *name,
                                                              const gchar *value);
gboolean             milter_client_context_change_header     (MilterClientContext *context,
                                                              const gchar *name,
                                                              guint32      index,
                                                              const gchar *value);
gboolean             milter_client_context_delete_header     (MilterClientContext *context,
                                                              const gchar *name,
                                                              guint32      index);
gboolean             milter_client_context_change_from       (MilterClientContext *context,
                                                              const gchar *from,
                                                              const gchar *parameters);
gboolean             milter_client_context_add_recipient     (MilterClientContext *context,
                                                              const gchar *recipient,
                                                              const gchar *parameters);
gboolean             milter_client_context_delete_recipient  (MilterClientContext *context,
                                                              const gchar *recipient);
gboolean             milter_client_context_replace_body      (MilterClientContext *context,
                                                              const gchar *body,
                                                              gsize        body_size);
gboolean             milter_client_context_progress          (MilterClientContext *context);
gboolean             milter_client_context_quarantine        (MilterClientContext *context,
                                                              const gchar *reason);
void                 milter_client_context_set_mta_timeout   (MilterClientContext *context,
                                                              guint timeout);

G_END_DECLS

#endif /* __MILTER_CLIENT_CONTEXT_H__ */

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
