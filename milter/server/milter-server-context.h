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

#ifndef __MILTER_SERVER_CONTEXT_H__
#define __MILTER_SERVER_CONTEXT_H__

#include <glib-object.h>

#include <milter/core.h>
#include <milter/server/milter-server-enum-types.h>

G_BEGIN_DECLS

#define MILTER_SERVER_CONTEXT_ERROR           (milter_server_context_error_quark())

#define MILTER_TYPE_SERVER_CONTEXT            (milter_server_context_get_type())
#define MILTER_SERVER_CONTEXT(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), MILTER_TYPE_SERVER_CONTEXT, MilterServerContext))
#define MILTER_SERVER_CONTEXT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), MILTER_TYPE_SERVER_CONTEXT, MilterServerContextClass))
#define MILTER_SERVER_CONTEXT_IS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), MILTER_TYPE_SERVER_CONTEXT))
#define MILTER_SERVER_CONTEXT_IS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), MILTER_TYPE_SERVER_CONTEXT))
#define MILTER_SERVER_CONTEXT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), MILTER_TYPE_SERVER_CONTEXT, MilterServerContextClass))

#define MILTER_SERVER_CONTEXT_DEFAULT_CONNECTION_TIMEOUT     300
#define MILTER_SERVER_CONTEXT_DEFAULT_WRITING_TIMEOUT         10
#define MILTER_SERVER_CONTEXT_DEFAULT_READING_TIMEOUT         10
#define MILTER_SERVER_CONTEXT_DEFAULT_END_OF_MESSAGE_TIMEOUT 300

typedef enum
{
    MILTER_SERVER_CONTEXT_ERROR_CONNECTION_FAILURE,
    MILTER_SERVER_CONTEXT_ERROR_INVALID_REPLY,
    MILTER_SERVER_CONTEXT_ERROR_NO_SPEC,
    MILTER_SERVER_CONTEXT_ERROR_INVALID_STATE,
    MILTER_SERVER_CONTEXT_ERROR_BUSY,
    MILTER_SERVER_CONTEXT_ERROR_IO_ERROR,
    MILTER_SERVER_CONTEXT_ERROR_DECODE_ERROR,
    MILTER_SERVER_CONTEXT_ERROR_NEWER_VERSION_REQUESTED
} MilterServerContextError;

typedef enum
{
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
    gboolean (*stop_on_header)     (MilterServerContext *context,
                                    const gchar *name,
                                    const gchar *value);
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
};

GQuark               milter_server_context_error_quark (void);

GType                milter_server_context_get_type    (void) G_GNUC_CONST;

MilterServerContext *milter_server_context_new         (void);
void                 milter_server_context_set_connection_timeout
                                                       (MilterServerContext *context,
                                                        gdouble timeout);
void                 milter_server_context_set_writing_timeout
                                                       (MilterServerContext *context,
                                                        gdouble timeout);
void                 milter_server_context_set_reading_timeout
                                                       (MilterServerContext *context,
                                                        gdouble timeout);
void                 milter_server_context_set_end_of_message_timeout
                                                       (MilterServerContext *context,
                                                        gdouble timeout);
gboolean             milter_server_context_set_connection_spec
                                                       (MilterServerContext *context,
                                                        const gchar *spec,
                                                        GError **error);
gboolean             milter_server_context_establish_connection
                                                       (MilterServerContext *context,
                                                        GError **error);

MilterServerContextState milter_server_context_get_state (MilterServerContext *context);
void                 milter_server_context_set_state   (MilterServerContext *context,
                                                        MilterServerContextState state);
gboolean             milter_server_context_is_processing
                                                       (MilterServerContext *context);

gboolean             milter_server_context_negotiate   (MilterServerContext *context,
                                                        MilterOption        *option);
gboolean             milter_server_context_connect     (MilterServerContext *context,
                                                        const gchar         *host_name,
                                                        struct sockaddr     *address,
                                                        socklen_t            address_length);
gboolean             milter_server_context_helo        (MilterServerContext *context,
                                                        const gchar *fqdn);
gboolean             milter_server_context_envelope_from
                                                       (MilterServerContext *context,
                                                        const gchar         *from);
gboolean             milter_server_context_envelope_recipient
                                                       (MilterServerContext *context,
                                                        const gchar         *recipient);
gboolean             milter_server_context_data        (MilterServerContext *context);
gboolean             milter_server_context_unknown     (MilterServerContext *context,
                                                        const gchar         *command);
gboolean             milter_server_context_header      (MilterServerContext *context,
                                                        const gchar         *name,
                                                        const gchar         *value);
gboolean             milter_server_context_end_of_header
                                                       (MilterServerContext *context);
gboolean             milter_server_context_body        (MilterServerContext *context,
                                                        const gchar         *chunk,
                                                        gsize                size);
gboolean             milter_server_context_end_of_message
                                                       (MilterServerContext *context,
                                                        const gchar         *chunk,
                                                        gsize                size);
gboolean             milter_server_context_quit        (MilterServerContext *context);
gboolean             milter_server_context_abort       (MilterServerContext *context);
gboolean             milter_server_context_is_enable_step
                                                       (MilterServerContext *context,
                                                        MilterStepFlags      step);
gboolean             milter_server_context_get_skip_body
                                                       (MilterServerContext *context);
const gchar         *milter_server_context_get_name    (MilterServerContext *context);

void                 milter_server_context_set_name    (MilterServerContext *context,
                                                        const gchar *name);
G_END_DECLS

#endif /* __MILTER_SERVER_CONTEXT_H__ */

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
