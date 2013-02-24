/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Copyright (C) 2008-2013  Kouhei Sutou <kou@clear-code.com>
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

#ifndef __MILTER_CLIENT_CLIENT_H__
#define __MILTER_CLIENT_CLIENT_H__

#include <milter/core.h>
#include <milter/client/milter-client-context.h>

G_BEGIN_DECLS

/**
 * SECTION: milter-client
 * @title: Client object
 * @short_description: The client object to implement a milter.
 *
 * %MilterClient implements a milter client object that
 * provides listening socket, accepting connections,
 * changing effective user/group and so on. Each milter
 * session is processed by %MilterClientContext that is
 * created by %MilterClient.
 */

/**
 * MILTER_CLIENT_DEFAULT_SUSPEND_TIME_ON_UNACCEPTABLE:
 *
 * The suspend time on unacceptable situation. See
 * milter_client_get_suspend_time_on_unacceptable() for more
 * details of the situation.
 */
#define MILTER_CLIENT_DEFAULT_SUSPEND_TIME_ON_UNACCEPTABLE 5

/**
 * MILTER_CLIENT_DEFAULT_MAX_CONNECTIONS:
 *
 * The default max connections. See
 * milter_client_get_max_connections() for more
 * details of the situation.
 *
 * No limit by default.
 */
#define MILTER_CLIENT_DEFAULT_MAX_CONNECTIONS 0

/**
 * MILTER_CLIENT_MAX_N_WORKERS:
 *
 * The maximum number of worker processes.
 */
#define MILTER_CLIENT_MAX_N_WORKERS 1000

/**
 * MILTER_CLIENT_ERROR:
 *
 * Used to get the #GError quark for #MilterClient errors.
 */
#define MILTER_CLIENT_ERROR           (milter_client_error_quark())

#define MILTER_TYPE_CLIENT            (milter_client_get_type())
#define MILTER_CLIENT(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), MILTER_TYPE_CLIENT, MilterClient))
#define MILTER_CLIENT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), MILTER_TYPE_CLIENT, MilterClientClass))
#define MILTER_IS_CLIENT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), MILTER_TYPE_CLIENT))
#define MILTER_IS_CLIENT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), MILTER_TYPE_CLIENT))
#define MILTER_CLIENT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), MILTER_TYPE_CLIENT, MilterClientClass))

/**
 * MilterClientEventLoopBackend:
 * @MILTER_CLIENT_EVENT_LOOP_BACKEND_GLIB: Let main loop use GLib.
 * @MILTER_CLIENT_EVENT_LOOP_BACKEND_LIBEV: Let main loop use libev.
 */
typedef enum
{
    MILTER_CLIENT_EVENT_LOOP_BACKEND_DEFAULT,
    MILTER_CLIENT_EVENT_LOOP_BACKEND_GLIB,
    MILTER_CLIENT_EVENT_LOOP_BACKEND_LIBEV
} MilterClientEventLoopBackend;

/**
 * MilterClientError:
 * @MILTER_CLIENT_ERROR_RUNNING: Indicates main loop has
 * been running.
 * @MILTER_CLIENT_ERROR_UNIX_SOCKET: Indicates UNIX domain
 * socket related error.
 * @MILTER_CLIENT_ERROR_IO_ERROR: Indicates IO related
 * error.
 *
 * These identify the variable errors that can occur while
 * calling %MilterClient functions.
 */
typedef enum
{
    MILTER_CLIENT_ERROR_RUNNING,
    MILTER_CLIENT_ERROR_UNIX_SOCKET,
    MILTER_CLIENT_ERROR_IO_ERROR,
    MILTER_CLIENT_ERROR_THREAD,
    MILTER_CLIENT_ERROR_PASSWORD_ENTRY,
    MILTER_CLIENT_ERROR_GROUP_ENTRY,
    MILTER_CLIENT_ERROR_DROP_PRIVILEGE,
    MILTER_CLIENT_ERROR_DETACH_IO,
    MILTER_CLIENT_ERROR_DAEMONIZE,
    MILTER_CLIENT_ERROR_NOT_LISTENED_YET,
    MILTER_CLIENT_ERROR_PROCESS,
    MILTER_CLIENT_ERROR_PID_FILE
} MilterClientError;

typedef struct _MilterClientClass    MilterClientClass;

typedef GPid (*MilterClientCustomForkFunc) (MilterClient *client);

/**
 * MilterClient:
 *
 * %MilterClient is a front object. It accepts connections
 * from MTA and dispatches it.
 **/
struct _MilterClient
{
    GObject object;
};

struct _MilterClientClass
{
    GObjectClass parent_class;

    void   (*connection_established)      (MilterClient *client,
                                           MilterClientContext *context);
    void   (*listen_started)              (MilterClient *client,
                                           struct sockaddr *address,
                                           socklen_t        address_size);
    const gchar *(*get_connection_spec)   (MilterClient *client);
    gboolean (*set_connection_spec)       (MilterClient *client,
                                           const gchar  *spec,
                                           GError      **error);
    guint  (*get_unix_socket_mode)        (MilterClient *client);
    void   (*set_unix_socket_mode)        (MilterClient *client,
                                           guint         mode);
    const gchar *(*get_unix_socket_group) (MilterClient *client);
    void   (*set_unix_socket_group)       (MilterClient *client,
                                           const gchar  *group);
    gboolean (*is_remove_unix_socket_on_close)
                                          (MilterClient *client);
    guint  (*get_suspend_time_on_unacceptable)
                                          (MilterClient *client);
    void   (*set_suspend_time_on_unacceptable)
                                          (MilterClient *client,
                                           guint         suspend_time);
    guint  (*get_max_connections)         (MilterClient *client);
    void   (*set_max_connections)         (MilterClient *client,
                                           guint         max_connections);
    const gchar *(*get_effective_user)    (MilterClient *client);
    void   (*set_effective_user)          (MilterClient *client,
                                           const gchar  *effective_user);
    const gchar *(*get_effective_group)   (MilterClient *client);
    void   (*set_effective_group)         (MilterClient *client,
                                           const gchar  *effective_group);
    guint  (*get_maintenance_interval)    (MilterClient *client);
    void   (*set_maintenance_interval)    (MilterClient *client,
                                           guint         maintenance_interval);
    void   (*maintain)                    (MilterClient *client);
    void   (*sessions_finished)           (MilterClient *client,
                                           guint         n_finished_sessions);
    void   (*event_loop_created)          (MilterClient *client,
                                           MilterEventLoop *loop);
    MilterClientEventLoopBackend
           (*get_event_loop_backend)      (MilterClient *client);
    void   (*set_event_loop_backend)      (MilterClient *client,
                                           MilterClientEventLoopBackend backend);
    guint  (*get_n_workers)               (MilterClient *client);
    void   (*set_n_workers)               (MilterClient *client,
                                           guint         n_workers);
    guint  (*get_default_packet_buffer_size)
                                          (MilterClient *client);
    void   (*set_default_packet_buffer_size)
                                          (MilterClient *client,
                                           guint         size);
    GPid   (*fork)                        (MilterClient *client);
    const gchar *(*get_pid_file)          (MilterClient *client);
    void   (*set_pid_file)                (MilterClient *client,
                                           const gchar  *pid_file);
    gboolean (*is_remove_pid_file_on_exit)(MilterClient *client);
    void   (*set_remove_pid_file_on_exit) (MilterClient *client,
                                           gboolean      remove);
    gboolean (*is_run_as_daemon)          (MilterClient *client);
    void   (*set_run_as_daemon)           (MilterClient *client,
                                           gboolean      daemon);
    guint  (*get_max_pending_finished_sessions)
                                          (MilterClient *client);
    void   (*set_max_pending_finished_sessions)
                                          (MilterClient *client,
                                           guint         n_sessions);
};

GQuark               milter_client_error_quark       (void);

GType                milter_client_get_type          (void) G_GNUC_CONST;


/**
 * milter_client_new:
 *
 * Creates a new client.
 *
 * Returns: a new %MilterClient object.
 */
MilterClient        *milter_client_new               (void);

/**
 * milter_client_get_default_connection_spec:
 * @client: a %MilterClient.
 *
 * Gets the default connection specification. The default
 * connection specification should be freed when no longer
 * needed.
 *
 * Returns: the copy of default connection specification.
 */
gchar               *milter_client_get_default_connection_spec
                                                     (MilterClient  *client);

/**
 * milter_client_get_connection_spec:
 * @client: a %MilterClient.
 *
 * Gets the connection specification. The connection specification
 * should not be freed.
 *
 * Returns: the connection specification.
 */
const gchar         *milter_client_get_connection_spec
                                                     (MilterClient  *client);

/**
 * milter_client_set_connection_spec:
 * @client: a %MilterClient.
 * @spec: a connection specification like inet:10025.
 * @error: return location for an error, or %NULL.
 *
 * Sets a connection specification. If @spec is invalid
 * format and @error is not %NULL, error detail is stored
 * into @error.
 *
 * Returns: %TRUE on success.
 */
gboolean             milter_client_set_connection_spec
                                                     (MilterClient  *client,
                                                      const gchar   *spec,
                                                      GError       **error);

/**
 * milter_client_get_listen_channel:
 * @client: a %MilterClient.
 *
 * Gets the channel for listening connection.
 *
 * Returns: the listen channel.
 */
GIOChannel          *milter_client_get_listen_channel(MilterClient  *client);

/**
 * milter_client_set_listen_channel:
 * @client: a %MilterClient.
 * @channel: a channel for listening connection.
 *
 * Sets a channel for listening connection.
 */
void                 milter_client_set_listen_channel(MilterClient  *client,
                                                      GIOChannel    *channel);

/**
 * milter_client_set_listen_backlog:
 * @client: a %MilterClient.
 * @backlog: a backlog for listen(2).
 *
 * Sets a backlog for listen(2).
 */
void                 milter_client_set_listen_backlog(MilterClient  *client,
                                                      gint           backlog);

/**
 * milter_client_listen:
 * @client: a %MilterClient.
 * @error: return location for an error, or %NULL.
 *
 * Listens with configured parameters: connection spec and
 * backlog.
 *
 * Returns: %TRUE if listen successfully.
 */
gboolean             milter_client_listen            (MilterClient  *client,
                                                      GError       **error);

/**
 * milter_client_is_remove_unix_socket_on_create:
 * @client: a %MilterClient.
 *
 * Gets whether removing UNIX domain socket before new UNIX
 * domain socket is created.
 *
 * Returns: %TRUE if removing UNIX domain socket on create,
 * %FALSE otherwise.
 */
gboolean             milter_client_is_remove_unix_socket_on_create
                                                     (MilterClient  *client);

/**
 * milter_client_set_remove_unix_socket_on_create:
 * @client: a %MilterClient.
 * @remove: %TRUE if removing UNIX domain socket before new
 *          UNIX domain socket is create.
 *
 * Sets whether removing UNIX domain socket before new UNIX
 * domain socket is created.
 */
void                 milter_client_set_remove_unix_socket_on_create
                                                     (MilterClient  *client,
                                                      gboolean       remove);

/**
 * milter_client_set_timeout:
 * @client: a %MilterClient.
 * @timeout: a timeout by seconds.
 *
 * Sets the timeout value for established
 * %MilterClientContext.
 */
void                 milter_client_set_timeout       (MilterClient  *client,
                                                      guint          timeout);

/**
 * milter_client_get_unix_socket_mode:
 * @client: a %MilterClient.
 *
 * Gets the UNIX domain socket mode.
 *
 * Returns: the UNIX domain socket mode.
 */
guint                milter_client_get_unix_socket_mode
                                                     (MilterClient  *client);

/**
 * milter_client_set_unix_socket_mode:
 * @client: a %MilterClient.
 * @mode: the UNIX domain socket mode.
 *
 * Sets the UNIX domain socket mode. @mode == '0' means that
 * the default UNIX domain socket mode will be used.
 */
void                 milter_client_set_unix_socket_mode
                                                     (MilterClient  *client,
                                                      guint          mode);

/**
 * milter_client_get_default_unix_socket_mode:
 * @client: a %MilterClient.
 *
 * Gets the default UNIX domain socket mode.
 *
 * Returns: the default UNIX domain socket mode.
 */
guint                milter_client_get_default_unix_socket_mode
                                                     (MilterClient  *client);

/**
 * milter_client_set_default_unix_socket_mode:
 * @client: a %MilterClient.
 * @mode: the UNIX domain socket mode.
 *
 * Sets the default UNIX domain socket mode.
 */
void                 milter_client_set_default_unix_socket_mode
                                                     (MilterClient  *client,
                                                      guint          mode);

/**
 * milter_client_get_unix_socket_group:
 * @client: a %MilterClient.
 *
 * Gets the group of UNIX domain socket.
 *
 * Returns: the group name for UNIX domain socket.
 */
const gchar         *milter_client_get_unix_socket_group
                                                     (MilterClient  *client);

/**
 * milter_client_set_unix_socket_group:
 * @client: a %MilterClient.
 * @group: the UNIX domain socket group.
 *
 * Sets the group of UNIX domain socket.
 */
void                 milter_client_set_unix_socket_group
                                                     (MilterClient  *client,
                                                      const gchar   *group);

/**
 * milter_client_get_default_unix_socket_group:
 * @client: a %MilterClient.
 *
 * Gets the default group of UNIX domain socket.
 *
 * Returns: the default group name for UNIX domain socket.
 */
const gchar         *milter_client_get_default_unix_socket_group
                                                     (MilterClient  *client);

/**
 * milter_client_set_default_unix_socket_group:
 * @client: a %MilterClient.
 * @group: the group name.
 *
 * Sets the default group of UNIX domain socket.
 */
void                 milter_client_set_default_unix_socket_group
                                                     (MilterClient  *client,
                                                      const gchar   *group);

/**
 * milter_client_is_remove_unix_socket_on_close:
 * @client: a %MilterClient.
 *
 * Gets whether removing UNIX domain socket after the socket
 * is closed.
 *
 * Returns: %TRUE if removing UNIX domain socket on close,
 * %FALSE otherwise.
 */
gboolean             milter_client_is_remove_unix_socket_on_close
                                                     (MilterClient  *client);

/**
 * milter_client_get_default_remove_unix_socket_on_close:
 * @client: a %MilterClient.
 *
 * Gets default value whether removing UNIX domain socket
 * after the socket is closed.
 *
 * Returns: %TRUE if removing UNIX domain socket on close,
 * %FALSE otherwise.
 */
gboolean             milter_client_get_default_remove_unix_socket_on_close
                                                     (MilterClient  *client);

/**
 * milter_client_set_default_remove_unix_socket_on_close:
 * @client: a %MilterClient.
 * @remove: %TRUE if removing UNIX domain socket after the
 *          socket is closed.
 *
 * Sets default value whether removing UNIX domain socket
 * after the socket is closed.
 */
void                 milter_client_set_default_remove_unix_socket_on_close
                                                     (MilterClient  *client,
                                                      gboolean       remove);

/**
 * milter_client_get_suspend_time_on_unacceptable:
 * @client: a %MilterClient.
 *
 * Gets suspend time on unacceptable situation. The
 * situation is occurred when many SMTP clients connect to
 * SMTP server. The situation means that your mail system
 * is overflowed. You need to take some
 * measures. e.g. Reducing number of milters, running
 * milters on other machines, upgrade machine spec, growing
 * number of max file descriptors and so on.
 *
 * Returns: suspend time in seconds.
 */
guint                milter_client_get_suspend_time_on_unacceptable
                                                     (MilterClient  *client);

/**
 * milter_client_set_suspend_time_on_unacceptable:
 * @client: a %MilterClient.
 * @suspend_time: suspend time in seconds.
 *
 * Sets suspend time on unacceptable situation. See
 * milter_client_get_suspend_time_on_unacceptable() for more
 * details of the situation.
 */
void                 milter_client_set_suspend_time_on_unacceptable
                                                     (MilterClient  *client,
                                                      guint          suspend_time);

/**
 * milter_client_get_max_connections:
 * @client: a %MilterClient.
 *
 * Gets the maximum number of connections. If the current
 * processing connections are over the max connections,
 * accepting a new connection is suspend in
 * milter_client_get_suspend_time_on_unacceptable() seconds.
 *
 * 0 means 'no limit'.
 *
 * Returns: the maximum number of connections.
 */
guint                milter_client_get_max_connections
                                                     (MilterClient  *client);

/**
 * milter_client_set_max_connections:
 * @client: a %MilterClient.
 * @max_connections: the maximum number of connections.
 *
 * Sets the maximum number of connections. 0 means 'no
 * limit'. See milter_client_get_max_connections() for more
 * details.
 */
void                 milter_client_set_max_connections
                                                     (MilterClient  *client,
                                                      guint          max_connections);

/**
 * milter_client_get_effective_user:
 * @client: a %MilterClient.
 *
 * Gets the effective user.
 *
 * Returns: the effective user.
 */
const gchar         *milter_client_get_effective_user(MilterClient  *client);

/**
 * milter_client_set_effective_user:
 * @client: a %MilterClient.
 * @effective_user: the effective user.
 *
 * Sets the effective user of this process.
 */
void                 milter_client_set_effective_user(MilterClient  *client,
                                                      const gchar   *effective_group);

/**
 * milter_client_get_effective_group:
 * @client: a %MilterClient.
 *
 * Gets the effective group.
 *
 * Returns: the effective group.
 */
const gchar         *milter_client_get_effective_group
                                                     (MilterClient  *client);

/**
 * milter_client_set_effective_group:
 * @client: a %MilterClient.
 * @effective_group: the effective group.
 *
 * Sets the effective group of this process.
 */
void                 milter_client_set_effective_group
                                                     (MilterClient  *client,
                                                      const gchar   *effective_group);

/**
 * milter_client_get_maintenance_interval:
 * @client: a %MilterClient.
 *
 * Gets the maintenance interval. #MilterClient::maintain
 * signal is emitted each 'maintenance interval' sessions
 * finished.
 *
 * 0 means 'maintain signal isn't needed'.
 *
 * Returns: the maintenance interval.
 */
guint                milter_client_get_maintenance_interval
                                                     (MilterClient  *client);

/**
 * milter_client_set_maintenance_interval:
 * @client: a %MilterClient.
 * @maintenance_interval: the maintenance interval.
 *
 * Sets the maintenance interval. 0 means 'maintain
 * signal isn't needed'. See
 * milter_client_get_maintenance_interval() for more
 * details.
 */
void                 milter_client_set_maintenance_interval
                                                     (MilterClient  *client,
                                                      guint          n_sessions);


/**
 * milter_client_set_default_packet_buffer_size:
 * @client: a %MilterClient.
 * @size: a packet buffer size in bytes. (deafult is
 *        %MILTER_CLIENT_CONTEXT_DEFAULT_PACKET_BUFFER_SIZE.)
 *
 * Sets the default packet buffer size for each client
 * context created by the client. See
 * milter_client_contexdt_set_packet_buffer_size() for more
 * packet buffer size information.
 */
void                 milter_client_set_default_packet_buffer_size
                                                       (MilterClient *context,
                                                        guint         size);

/**
 * milter_client_get_default_packet_buffer_size:
 * @client: a %MilterClient.
 *
 * Gets the default packet buffer size for each client
 * context created by the client.
 *
 * Returns: the defalt packet buffer size for each client
 * context created by the context.
 */
guint                milter_client_get_default_packet_buffer_size
                                                       (MilterClient *client);

/**
 * milter_client_create_context:
 * @client: a %MilterClient.
 *
 * Creates a new %MilterClientContext that is initialized with
 * the @client configuration.
 *
 * Returns: a newly allocated %MilterClientContext. It
 * should be unref-ed when no longer needed.
 */
MilterClientContext *milter_client_create_context      (MilterClient *client);

/**
 * milter_client_create_event_loop:
 * @client: a %MilterClient.
 * @use_default_context: whther to use default context or not.
 *
 * Creates a new %MilterEventLoop that is initialized with
 * the @client configuration.
 *
 * Returns: a newly allocated %MilterEventLoop. It
 * should be unref-ed when no longer needed.
 */
MilterEventLoop     *milter_client_create_event_loop   (MilterClient *client,
                                                        gboolean      use_default_context);

/**
 * milter_client_run:
 * @client: a %MilterClient.
 * @error: return location for an error, or %NULL.
 *
 * Runs main loop. If the main loop isn't quitted
 * successfully and @error is not %NULL, error detail is
 * stored into @error.
 *
 * Returns: %TRUE if main loop is quitted successfully,
 * %FALSE otherwise.
 */
gboolean             milter_client_run               (MilterClient  *client,
                                                      GError       **error);

#ifndef MILTER_DISABLE_DEPRECATED
/**
 * milter_client_main:
 * @client: a %MilterClient.
 *
 * Runs main loop.
 *
 * Returns: %TRUE if main loop is quitted successfully,
 * %FALSE otherwise.
 *
 * Deprecated: 1.6.2: Use milter_client_run() instead.
 */
gboolean             milter_client_main              (MilterClient  *client);
#endif

/**
 * milter_client_shutdown:
 * @client: a %MilterClient.
 *
 * Shuts main loop down.
 */
void                 milter_client_shutdown          (MilterClient  *client);

/**
 * milter_client_processing_context_foreach:
 * @client: a %MilterClient.
 * @func: the function to call with each processing %MilterClientContext.
 * @user_data: user data to pass to the function.
 *
 * Calls a function for each processing
 * %MilterClientContext.
 */
void                 milter_client_processing_context_foreach
                                                     (MilterClient  *client,
                                                      GFunc          func,
                                                      gpointer       user_data);


/**
 * milter_client_drop_privilege:
 * @client: a %MilterClient.
 * @error: return location for an error, or %NULL.
 *
 * Drops privilege of this process.
 *
 * Returns: %TRUE if drop privilege successfully.
 */
gboolean             milter_client_drop_privilege    (MilterClient  *client,
                                                      GError       **error);

/**
 * milter_client_daemonize:
 * @client: a %MilterClient.
 * @error: return location for an error, or %NULL.
 *
 * Daemonizes this process.
 *
 * Returns: %TRUE if daemonize successfully.
 */
gboolean             milter_client_daemonize         (MilterClient  *client,
                                                      GError       **error);

/**
 * milter_client_session_started:
 * @client: a %MilterClient.
 *
 * Calls this function when the @client starts a session.
 */
void                 milter_client_session_started   (MilterClient  *client);

/**
 * milter_client_session_finished:
 * @client: a %MilterClient.
 *
 * Calls this function when the @client finished a session.
 */
void                 milter_client_session_finished  (MilterClient  *client);

/**
 * milter_client_get_n_processing_sessions:
 * @client: a %MilterClient.
 *
 * Returns number of the current processing sessions.
 *
 * Returns: number of the current processing sessions.
 */
guint                milter_client_get_n_processing_sessions
                                                     (MilterClient  *client);

/**
 * milter_client_is_processing:
 * @client: a %MilterClient.
 *
 * Returns whether @client has one or more processing sessions or not.
 *
 * Returns: %TRUE if @client has one or more processsing
 * sessions, %FALSE otherwise.
 */
gboolean             milter_client_is_processing     (MilterClient  *client);

/**
 * milter_client_get_syslog_facility:
 * @client: a %MilterClient.
 *
 * Gets the syslog facility name of the @client.
 *
 * Returns: syslog facility name.
 */
const gchar          *milter_client_get_syslog_facility
                                                     (MilterClient  *client);

/**
 * milter_client_set_syslog_identify:
 * @client: a %MilterClient.
 * @identify: the identify in syslog.
 *
 * Sets the syslog identify of the @client.
 */
void                  milter_client_set_syslog_identify
                                                     (MilterClient  *client,
                                                      const gchar   *identify);

/**
 * milter_client_get_syslog_identify:
 * @client: a %MilterClient.
 *
 * Gets the syslog identify name of the @client.
 *
 * Returns: syslog identify name.
 */
const gchar          *milter_client_get_syslog_identify
                                                     (MilterClient  *client);

/**
 * milter_client_set_syslog_facility:
 * @client: a %MilterClient.
 *
 * Sets the syslog facility name of the @client.
 */
void                  milter_client_set_syslog_facility
                                                     (MilterClient  *client,
                                                      const gchar   *facility);

/**
 * milter_client_start_syslog:
 * @client: a %MilterClient.
 *
 * Starts logging by syslog with @identify name.
 */
void                 milter_client_start_syslog      (MilterClient  *client);

/**
 * milter_client_stop_syslog:
 * @client: a %MilterClient.
 *
 * Stops logging by syslog.
 */
void                 milter_client_stop_syslog       (MilterClient  *client);

/**
 * milter_client_get_syslog_enabled:
 * @client: a %MilterClient.
 *
 * Gets whether syslog is enabled.
 *
 * Returns: %TRUE if syslog is enabled, %FALSE otherwise.
 */
gboolean             milter_client_get_syslog_enabled(MilterClient  *client);

/**
 * milter_client_get_event_loop:
 * @client: a %MilterClient.
 *
 * Gets the %MilterEventLoop for processing requests.
 *
 * Returns: the %MilterEventLoop for processing requests.
 */
MilterEventLoop     *milter_client_get_event_loop   (MilterClient  *client);

/**
 * milter_client_get_event_loop_backend:
 * @client: a %MilterClient.
 *
 * Gets the %MilterClientEventLoopBackend of @client.
 *
 * Returns: the %MilterClientEventLoopBackend of @client.
 */
MilterClientEventLoopBackend
                     milter_client_get_event_loop_backend
                                                     (MilterClient  *client);

/**
 * milter_client_set_event_loop_backend:
 * @client: a %MilterClient.
 * @backend: a %MilterClinetEventLoopBackend
 *
 * Sets the %MilterClientEventLoopBackend of @client.
 */
void                 milter_client_set_event_loop_backend
                                                     (MilterClient  *client,
                                                      MilterClientEventLoopBackend backend);

/**
 * milter_client_get_n_workers:
 * @client: a %MilterClient.
 *
 * Gets the number of worker processes of @client.
 *
 * Returns: the number of worker processes of @client.
 */
guint                milter_client_get_n_workers     (MilterClient  *client);

/**
 * milter_client_set_n_workers:
 * @client: a %MilterClient.
 * @n_workers: the number of worker processes.
 *
 * Sets the number of worker processes of @client.
 */
void                 milter_client_set_n_workers     (MilterClient  *client,
                                                      guint          n_workers);

/**
 * milter_client_fork:
 * @client: a %MilterClient.
 *
 * Forks new process. If custom fork is set by
 * milter_client_set_custom_fork_func(), it will be called.
 *
 * Returns: child process PID for parent process, 0 for child
 * process or -1 on error.
 */
GPid                 milter_client_fork                 (MilterClient *client);

/**
 * milter_client_fork_without_custom:
 * @client: a %MilterClient.
 *
 * Forks new process. The custom fork for the client never
 * called even if the custom fork is set by
 * milter_client_set_custom_fork(). This function will be
 * called in custom fork.
 *
 * Returns: child process PID for parent process, 0 for child
 * process or -1 on error.
 */
GPid                 milter_client_fork_without_custom  (MilterClient *client);

/**
 * milter_client_set_custom_fork_func:
 * @client: a %MilterClient.
 * @custom_fork: a %MilterClientCustomForkFunc.
 *
 * Sets custom fork function for the client. It will be
 * called in milter_client_fork(). If @custom_fork is %NULL,
 * the system fork(2) will be called.
 */
void                 milter_client_set_custom_fork_func (MilterClient *client,
                                                         MilterClientCustomForkFunc custom_fork);

/**
 * milter_client_set_custom_fork_func:
 * @client: a %MilterClient.
 * @custom_fork: a %MilterClientCustomForkFunc.
 *
 * Gets the custom fork function for the client.
 *
 * Returns: the custom fork function for the client.
 */
MilterClientCustomForkFunc
                     milter_client_get_custom_fork_func (MilterClient *client);

/**
 * milter_client_set_worker_id:
 * @client: a %MilterClient.
 *
 * Gets the worker ID of the client. 0 in parent process, 1 or more in
 * worker processes.
 *
 * Returns: the worker ID of the client.
 */
guint                milter_client_get_worker_id        (MilterClient *client);

/**
 * milter_client_get_pid_file:
 * @client: a %MilterClient.
 *
 * Gets the PID file of @client.
 *
 * Returns: the PID file of @client.
 */
const gchar         *milter_client_get_pid_file         (MilterClient  *client);

/**
 * milter_client_set_pid_file:
 * @client: a %MilterClient.
 * @pid_file: the PID file of @client.
 *
 * Sets the PID file of @client.
 */
void                 milter_client_set_pid_file         (MilterClient  *client,
                                                         const gchar   *pid_file);

/**
 * milter_client_is_remove_pid_file_on_exit:
 * @client: a %MilterClient.
 *
 * Gets whether removing PID file after main loop is quitted.
 *
 * Returns: %TRUE if removing PID file on exit, %FALSE otherwise.
 */
gboolean             milter_client_is_remove_pid_file_on_exit
                                                     (MilterClient  *client);

/**
 * milter_client_set_remove_pid_file_on_exit:
 * @client: a %MilterClient.
 * @remove: %TRUE if removing PID file on exit.
 *
 * Sets whether removing PID file after main loop is quitted.
 */
void                 milter_client_set_remove_pid_file_on_exit
                                                     (MilterClient  *client,
                                                      gboolean       remove);

/**
 * milter_client_is_run_as_daemon:
 * @client: a %MilterClient.
 *
 * Gets whether running as a daemon or not.
 *
 * Returns: %TRUE if running as a daemon, %FALSE otherwise.
 */
gboolean             milter_client_is_run_as_daemon  (MilterClient  *client);

/**
 * milter_client_set_run_as_daemon:
 * @client: a %MilterClient.
 * @daemon: %TRUE if runnning as a daemon.
 *
 * Sets whether running as a daemon.
 */
void                 milter_client_set_run_as_daemon (MilterClient  *client,
                                                      gboolean       daemon);

/**
 * milter_client_get_max_pending_finished_sessions:
 * @client: a %MilterClient.
 *
 * Gets the maximum number of pending finished sessions of @client.
 *
 * Returns: the maximum number of pending finished sessions of @client.
 */
guint                milter_client_get_max_pending_finished_sessions
                                                     (MilterClient  *client);

/**
 * milter_client_set_max_pending_finished_sessions:
 * @client: a %MilterClient.
 * @n_sessions: the maximum number of pending finished sessions.
 *
 * Sets the maximum number of pending finished sessions of @client. If
 * the number of finished sessions that are not freed is larger than
 * @n_sessions, all those finished sessions are freed immediately.
 *
 * 0 means that this cheack is disabled.
 */
void                 milter_client_set_max_pending_finished_sessions
                                                     (MilterClient  *client,
                                                      guint          n_sessions);

G_END_DECLS

#endif /* __MILTER_CLIENT_CLIENT_H__ */

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
