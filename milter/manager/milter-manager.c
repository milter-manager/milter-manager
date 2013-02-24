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

#ifdef HAVE_CONFIG_H
#  include "../../config.h"
#endif /* HAVE_CONFIG_H */

#include <locale.h>
#include <glib/gi18n.h>

#include <stdlib.h>
#include <signal.h>
#include <errno.h>

#include "milter-manager.h"
#include "milter-manager-leader.h"

#define MILTER_MANAGER_GET_PRIVATE(obj)                 \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj),                 \
                                 MILTER_TYPE_MANAGER,   \
                                 MilterManagerPrivate))

typedef struct _MilterManagerPrivate MilterManagerPrivate;
struct _MilterManagerPrivate
{
    MilterManagerConfiguration *configuration;
    GList *leaders;
    GList *next_connection_checked_leader;
    gboolean connection_checking;

    GIOChannel *launcher_read_channel;
    GIOChannel *launcher_write_channel;

    guint periodical_connection_checker_id;
    guint current_periodical_connection_check_interval;

    GList *finished_leaders;

    gboolean is_custom_n_workers;
    gboolean is_custom_run_as_daemon;
    gboolean is_custom_max_pending_finished_sessions;
};

enum
{
    PROP_0,
    PROP_CONFIGURATION
};

G_DEFINE_TYPE(MilterManager, milter_manager, MILTER_TYPE_CLIENT)

static void dispose        (GObject         *object);
static void set_property   (GObject         *object,
                            guint            prop_id,
                            const GValue    *value,
                            GParamSpec      *pspec);
static void get_property   (GObject         *object,
                            guint            prop_id,
                            GValue          *value,
                            GParamSpec      *pspec);

static void   connection_established      (MilterClient *client,
                                           MilterClientContext *context);
static const gchar *get_connection_spec   (MilterClient *client);
static gboolean     set_connection_spec   (MilterClient *client,
                                           const gchar  *spec,
                                           GError      **error);
static guint  get_unix_socket_mode        (MilterClient *client);
static void   set_unix_socket_mode        (MilterClient *client,
                                           guint         mode);
static const gchar *get_unix_socket_group (MilterClient *client);
static void         set_unix_socket_group (MilterClient *client,
                                           const gchar  *group);
static gboolean is_remove_unix_socket_on_close (MilterClient *client);
static guint  get_suspend_time_on_unacceptable
                                          (MilterClient *client);
static void   set_suspend_time_on_unacceptable
                                          (MilterClient *client,
                                           guint         suspend_time);
static guint  get_max_connections         (MilterClient *client);
static void   set_max_connections         (MilterClient *client,
                                           guint         max_connections);
static const gchar *get_effective_user    (MilterClient *client);
static void         set_effective_user    (MilterClient *client,
                                           const gchar  *effective_user);
static const gchar *get_effective_group   (MilterClient *client);
static void         set_effective_group   (MilterClient *client,
                                           const gchar  *effective_group);
static guint  get_maintenance_interval    (MilterClient *client);
static void   set_maintenance_interval    (MilterClient *client,
                                           guint         n_sessions);
static void   maintain                    (MilterClient *client);
static void   sessions_finished           (MilterClient *client,
                                           guint         n_finished_sessions);
static void   event_loop_created          (MilterClient *client,
                                           MilterEventLoop *loop);
static void   cb_leader_finished          (MilterFinishedEmittable *emittable,
                                           gpointer user_data);
static guint  get_n_workers               (MilterClient *client);
static void   set_n_workers               (MilterClient *client,
                                           guint         n_workers);
static MilterClientEventLoopBackend get_event_loop_backend
                                          (MilterClient *client);
static void   set_event_loop_backend      (MilterClient *client,
                                           MilterClientEventLoopBackend backend);
static guint  get_default_packet_buffer_size
                                          (MilterClient *client);
static void   set_default_packet_buffer_size
                                          (MilterClient *client,
                                           guint         size);
static const gchar *get_pid_file          (MilterClient *client);
static void   set_pid_file                (MilterClient *client,
                                           const gchar  *pid_file);
static gboolean is_run_as_daemon          (MilterClient *client);
static void   set_run_as_daemon           (MilterClient *client,
                                           gboolean      daemon);
static GPid   fork_delegate               (MilterClient *client);
static guint  get_max_pending_finished_sessions
                                          (MilterClient *client);
static void   set_max_pending_finished_sessions
                                          (MilterClient *client,
                                           guint         n_sessions);

static void
milter_manager_class_init (MilterManagerClass *klass)
{
    GObjectClass *gobject_class;
    MilterClientClass *client_class;
    GParamSpec *spec;

    gobject_class = G_OBJECT_CLASS(klass);
    client_class = MILTER_CLIENT_CLASS(klass);

    gobject_class->dispose      = dispose;
    gobject_class->set_property = set_property;
    gobject_class->get_property = get_property;

    client_class->connection_established      = connection_established;
    client_class->get_connection_spec         = get_connection_spec;
    client_class->set_connection_spec         = set_connection_spec;
    client_class->get_unix_socket_mode        = get_unix_socket_mode;
    client_class->set_unix_socket_mode        = set_unix_socket_mode;
    client_class->get_unix_socket_group       = get_unix_socket_group;
    client_class->set_unix_socket_group       = set_unix_socket_group;
    client_class->is_remove_unix_socket_on_close = is_remove_unix_socket_on_close;
    client_class->get_suspend_time_on_unacceptable = get_suspend_time_on_unacceptable;
    client_class->set_suspend_time_on_unacceptable = set_suspend_time_on_unacceptable;
    client_class->get_max_connections = get_max_connections;
    client_class->set_max_connections = set_max_connections;
    client_class->get_effective_user = get_effective_user;
    client_class->set_effective_user = set_effective_user;
    client_class->get_effective_group = get_effective_group;
    client_class->set_effective_group = set_effective_group;
    client_class->get_maintenance_interval = get_maintenance_interval;
    client_class->set_maintenance_interval = set_maintenance_interval;
    client_class->get_n_workers = get_n_workers;
    client_class->set_n_workers = set_n_workers;
    client_class->get_event_loop_backend = get_event_loop_backend;
    client_class->set_event_loop_backend = set_event_loop_backend;
    client_class->get_default_packet_buffer_size =
        get_default_packet_buffer_size;
    client_class->set_default_packet_buffer_size =
        set_default_packet_buffer_size;
    client_class->get_pid_file = get_pid_file;
    client_class->set_pid_file = set_pid_file;
    client_class->is_run_as_daemon = is_run_as_daemon;
    client_class->set_run_as_daemon = set_run_as_daemon;
    client_class->fork = fork_delegate;
    client_class->maintain = maintain;
    client_class->sessions_finished = sessions_finished;
    client_class->event_loop_created = event_loop_created;
    client_class->get_max_pending_finished_sessions =
        get_max_pending_finished_sessions;
    client_class->set_max_pending_finished_sessions =
        set_max_pending_finished_sessions;

    spec = g_param_spec_object("configuration",
                               "Configuration",
                               "The configuration of the milter controller",
                               MILTER_TYPE_MANAGER_CONFIGURATION,
                               G_PARAM_READWRITE);
    g_object_class_install_property(gobject_class, PROP_CONFIGURATION, spec);

    g_type_class_add_private(gobject_class,
                             sizeof(MilterManagerPrivate));
}

static void
milter_manager_init (MilterManager *manager)
{
    MilterManagerPrivate *priv;

    priv = MILTER_MANAGER_GET_PRIVATE(manager);

    priv->configuration = NULL;
    priv->leaders = NULL;
    priv->next_connection_checked_leader = NULL;
    priv->connection_checking = FALSE;

    priv->launcher_read_channel = NULL;
    priv->launcher_write_channel = NULL;

    priv->periodical_connection_checker_id = 0;
    priv->current_periodical_connection_check_interval = 0;

    priv->finished_leaders = NULL;
}

static void
configuration_set_manager (MilterManagerConfiguration *configuration,
                           MilterManager *manager)
{
    g_object_set_data(G_OBJECT(configuration), "manager", manager);
}

static void
dispose_periodical_connection_checker (MilterManager *manager)
{
    MilterManagerPrivate *priv;
    MilterEventLoop *loop;

    priv = MILTER_MANAGER_GET_PRIVATE(manager);
    priv->current_periodical_connection_check_interval = 0;

    if (priv->periodical_connection_checker_id == 0)
        return;

    loop = milter_client_get_event_loop(MILTER_CLIENT(manager));
    milter_event_loop_remove(loop, priv->periodical_connection_checker_id);
    priv->periodical_connection_checker_id = 0;
}

static void
dispose_finished_leaders (MilterManagerPrivate *priv)
{
    GList *node;
    guint n_leaders = 0;

    if (!priv->finished_leaders)
        return;

    for (node = priv->finished_leaders; node; node = g_list_next(node)) {
        MilterManagerLeader *leader = node->data;
        g_object_unref(leader);
        n_leaders++;
    }
    g_list_free(priv->finished_leaders);
    priv->finished_leaders = NULL;

    milter_debug("[manager][dispose][leaders] %u", n_leaders);
}

static void
dispose (GObject *object)
{
    MilterManager *manager;
    MilterManagerPrivate *priv;

    manager = MILTER_MANAGER(object);
    priv = MILTER_MANAGER_GET_PRIVATE(manager);

    dispose_periodical_connection_checker(manager);
    dispose_finished_leaders(priv);

    if (priv->configuration) {
        configuration_set_manager(priv->configuration, NULL);
        g_object_unref(priv->configuration);
        priv->configuration = NULL;
    }

    if (priv->leaders) {
        g_list_free(priv->leaders);
        priv->leaders = NULL;
    }
    priv->next_connection_checked_leader = NULL;

    milter_manager_set_launcher_channel(MILTER_MANAGER(object), NULL, NULL);

    G_OBJECT_CLASS(milter_manager_parent_class)->dispose(object);
}

static void
set_property (GObject      *object,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
    MilterManager *manager;
    MilterManagerPrivate *priv;

    manager = MILTER_MANAGER(object);
    priv = MILTER_MANAGER_GET_PRIVATE(manager);
    switch (prop_id) {
    case PROP_CONFIGURATION:
        if (priv->configuration) {
            configuration_set_manager(priv->configuration, NULL);
            g_object_unref(priv->configuration);
        }
        priv->configuration = g_value_get_object(value);
        if (priv->configuration) {
            g_object_ref(priv->configuration);
            configuration_set_manager(priv->configuration, manager);
        }
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
    MilterManagerPrivate *priv;

    priv = MILTER_MANAGER_GET_PRIVATE(object);
    switch (prop_id) {
      case PROP_CONFIGURATION:
        g_value_set_object(value, priv->configuration);
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

MilterManager *
milter_manager_new (MilterManagerConfiguration *configuration)
{
    return g_object_new(MILTER_TYPE_MANAGER,
                        "configuration", configuration,
                        "syslog-identify", "milter-manager",
                        "syslog-facility", "mail",
                        "start-syslog", TRUE,
                        NULL);
}

static gboolean
connection_check (gpointer data)
{
    MilterManager *manager = data;
    MilterManagerPrivate *priv;
    GList *node, *next_node;
    guint i;
    guint n_leaders_per_connection_check = 30; /* FIXME: make customizable */

    priv = MILTER_MANAGER_GET_PRIVATE(manager);

    priv->connection_checking = TRUE;

    if (!priv->next_connection_checked_leader)
        priv->next_connection_checked_leader = priv->leaders;
    node = priv->next_connection_checked_leader;
    i = 0;
    while (node && i < n_leaders_per_connection_check) {
        MilterManagerLeader *leader = node->data;
        next_node = g_list_next(node);
        if (!milter_manager_leader_check_connection(leader)) {
            priv->leaders = g_list_delete_link(priv->leaders, node);
        }
        node = next_node;
        i++;
    }
    priv->next_connection_checked_leader = node;

    priv->connection_checking = FALSE;

    return priv->leaders != NULL;
}

static void
start_periodical_connection_checker (MilterManager *manager)
{
    MilterManagerPrivate *priv;
    guint interval;

    priv = MILTER_MANAGER_GET_PRIVATE(manager);

    interval = milter_manager_configuration_get_connection_check_interval(priv->configuration);

    if (interval > 0) {
        if (priv->periodical_connection_checker_id == 0 ||
            interval != priv->current_periodical_connection_check_interval) {
            MilterEventLoop *loop;
            milter_debug("[manager][connection-check][start] <%u> -> <%u>: %s",
                         priv->current_periodical_connection_check_interval,
                         interval,
                         priv->periodical_connection_checker_id == 0 ?
                         "initial" : "update");
            dispose_periodical_connection_checker(manager);
            priv->current_periodical_connection_check_interval = interval;
            loop = milter_client_get_event_loop(MILTER_CLIENT(manager));
            priv->periodical_connection_checker_id =
                milter_event_loop_add_timeout(loop, interval,
                                              connection_check, manager);
        }
    } else {
        dispose_periodical_connection_checker(manager);
    }
}

static MilterStatus
cb_client_negotiate (MilterClientContext *context, MilterOption *option,
                     MilterMacrosRequests *macros_requests, gpointer user_data)
{
    MilterManagerLeader *leader = user_data;

    if (milter_need_debug_log()) {
        gchar *inspected_option, *inspected_macros_requests;

        inspected_option = milter_option_inspect(option);
        inspected_macros_requests =
            milter_utils_inspect_object(G_OBJECT(macros_requests));
        milter_debug("[%u] [manager][receive][negotiate] <%s>:<%s>",
                     milter_agent_get_tag(MILTER_AGENT(context)),
                     inspected_option, inspected_macros_requests);
        g_free(inspected_option);
        g_free(inspected_macros_requests);
    }

    return milter_manager_leader_negotiate(leader, option, macros_requests);
}

static MilterStatus
cb_client_connect (MilterClientContext *context, const gchar *host_name,
                   struct sockaddr *address, socklen_t address_length,
                   gpointer user_data)
{
    MilterManagerLeader *leader = user_data;

    if (milter_need_debug_log()) {
        gchar *spec;

        spec = milter_connection_address_to_spec(address);
        milter_debug("[%u] [manager][receive][connect] <%s>:<%s>",
                     milter_agent_get_tag(MILTER_AGENT(context)),
                     host_name, spec);
        g_free(spec);
    }

    return milter_manager_leader_connect(leader, host_name,
                                         address, address_length);
}

static MilterStatus
cb_client_helo (MilterClientContext *context, const gchar *fqdn,
                gpointer user_data)
{
    MilterManagerLeader *leader = user_data;

    milter_debug("[%u] [manager][receive][helo] <%s>",
                 milter_agent_get_tag(MILTER_AGENT(context)),
                 fqdn);

    return milter_manager_leader_helo(leader, fqdn);
}

static MilterStatus
cb_client_envelope_from (MilterClientContext *context, const gchar *from,
                         gpointer user_data)
{
    MilterManagerLeader *leader = user_data;

    milter_debug("[%u] [manager][receive][envelope-from] <%s>",
                 milter_agent_get_tag(MILTER_AGENT(context)),
                 from);

    return milter_manager_leader_envelope_from(leader, from);
}

static MilterStatus
cb_client_envelope_recipient (MilterClientContext *context,
                              const gchar *recipient, gpointer user_data)
{
    MilterManagerLeader *leader = user_data;

    milter_debug("[%u] [manager][receive][envelope-recipient] <%s>",
                 milter_agent_get_tag(MILTER_AGENT(context)),
                 recipient);

    return milter_manager_leader_envelope_recipient(leader, recipient);
}

static MilterStatus
cb_client_data (MilterClientContext *context, gpointer user_data)
{
    MilterManagerLeader *leader = user_data;

    milter_debug("[%u] [manager][receive][data]",
                 milter_agent_get_tag(MILTER_AGENT(context)));

    return milter_manager_leader_data(leader);
}

static MilterStatus
cb_client_unknown (MilterClientContext *context, const gchar *command,
                   gpointer user_data)
{
    MilterManagerLeader *leader = user_data;

    milter_debug("[%u] [manager][receive][unknown] <%s>",
                 milter_agent_get_tag(MILTER_AGENT(context)),
                 command);

    return milter_manager_leader_unknown(leader, command);
}

static MilterStatus
cb_client_header (MilterClientContext *context,
                  const gchar *name, const gchar *value,
                  gpointer user_data)
{
    MilterManagerLeader *leader = user_data;

    milter_debug("[%u] [manager][receive][header] <%s>=<%s>",
                 milter_agent_get_tag(MILTER_AGENT(context)),
                 name, value);

    return milter_manager_leader_header(leader, name, value);
}

static MilterStatus
cb_client_end_of_header (MilterClientContext *context, gpointer user_data)
{
    MilterManagerLeader *leader = user_data;

    milter_debug("[%u] [manager][receive][end-of-header]",
                 milter_agent_get_tag(MILTER_AGENT(context)));

    return milter_manager_leader_end_of_header(leader);
}

static MilterStatus
cb_client_body (MilterClientContext *context, const gchar *chunk, gsize size,
                gpointer user_data)
{
    MilterManagerLeader *leader = user_data;

    milter_debug("[%u] [manager][receive][body] <%" G_GSIZE_FORMAT ">",
                 milter_agent_get_tag(MILTER_AGENT(context)),
                 size);

    return milter_manager_leader_body(leader, chunk, size);
}

static MilterStatus
cb_client_end_of_message (MilterClientContext *context,
                          const gchar *chunk, gsize size,
                          gpointer user_data)
{
    MilterManagerLeader *leader = user_data;

    milter_debug("[%u] [manager][receive][end-of-message] "
                 "<%" G_GSIZE_FORMAT ">",
                 milter_agent_get_tag(MILTER_AGENT(context)),
                 size);

    return milter_manager_leader_end_of_message(leader, chunk, size);
}

static MilterStatus
cb_client_abort (MilterClientContext *context, MilterClientContextState state,
                 gpointer user_data)
{
    MilterManagerLeader *leader = user_data;

    if (milter_need_debug_log()) {
        gchar *state_name;

        state_name =
            milter_utils_get_enum_nick_name(MILTER_TYPE_CLIENT_CONTEXT_STATE,
                                            state);
        milter_debug("[%u] [manager][receive][abort][%s]",
                     milter_agent_get_tag(MILTER_AGENT(context)),
                     state_name);
        g_free(state_name);
    }

    return milter_manager_leader_abort(leader);
}

static void
cb_client_define_macro (MilterClientContext *context, MilterCommand command,
                        GHashTable *macros, gpointer user_data)
{
    MilterManagerLeader *leader = user_data;

    if (milter_need_debug_log()) {
        gchar *inspected_macros;

        inspected_macros = milter_utils_inspect_hash_string_string(macros);
        milter_debug("[%u] [manager][receive][define-macro] <%s>",
                     milter_agent_get_tag(MILTER_AGENT(context)),
                     inspected_macros);
        g_free(inspected_macros);
    }

    milter_manager_leader_define_macro(leader, command, macros);
}

static void
cb_client_timeout (MilterClientContext *context, gpointer user_data)
{
    MilterManagerLeader *leader = user_data;

    milter_debug("[%u] [manager][timeout]",
                 milter_agent_get_tag(MILTER_AGENT(context)));

    milter_manager_leader_timeout(leader);
}

static void
cb_client_finished (MilterClientContext *context, gpointer user_data)
{
    MilterManagerLeader *leader = user_data;

    if (milter_need_log(MILTER_LOG_LEVEL_DEBUG |
                        MILTER_LOG_LEVEL_STATISTICS)) {
        MilterAgent *agent;
        guint tag;
        gdouble elapsed;
        MilterStatus status;
        gchar *status_name, *statistics_status_name;

        agent = MILTER_AGENT(context);
        tag = milter_agent_get_tag(agent);
        elapsed = milter_agent_get_elapsed(agent);

        status = milter_client_context_get_status(context);
        status_name = milter_utils_get_enum_nick_name(MILTER_TYPE_STATUS,
                                                      status);

        if (milter_need_debug_log()) {
            MilterClientContextState state;
            gchar *state_name;
            state = milter_client_context_get_state(context);
            state_name = milter_utils_get_enum_nick_name(
                MILTER_TYPE_CLIENT_CONTEXT_STATE, state);
            milter_debug("[%u] [manager][finish][%s][%s][%g]",
                         tag, state_name, status_name, elapsed);
            g_free(state_name);
        }

        if (milter_need_statistics_log()) {
            MilterClientContextState last_state;
            gchar *last_state_name;

            last_state = milter_client_context_get_last_state(context);
            last_state_name =
                milter_utils_get_enum_nick_name(
                    MILTER_TYPE_CLIENT_CONTEXT_STATE, last_state);
            if (MILTER_STATUS_IS_PASS(status)) {
                statistics_status_name = "pass";
            } else {
                statistics_status_name = status_name;
            }
            milter_statistics("[session][end][%s][%s][%g](%u)",
                              last_state_name, statistics_status_name,
                              elapsed, tag);
            g_free(last_state_name);
        }
        g_free(status_name);
    }

    milter_manager_leader_quit(leader);
}

typedef struct _LeaderFinishData
{
    MilterManager *manager;
    MilterClientContext *client_context;
} LeaderFinishData;

static void
teardown_client_context_signals (MilterClientContext *context,
                                 MilterManagerLeader *leader,
                                 LeaderFinishData *finished_data)
{
#define DISCONNECT(name)                                                \
    g_signal_handlers_disconnect_by_func(context,                       \
                                         G_CALLBACK(cb_client_ ## name), \
                                         leader)

    DISCONNECT(negotiate);
    DISCONNECT(connect);
    DISCONNECT(helo);
    DISCONNECT(envelope_from);
    DISCONNECT(envelope_recipient);
    DISCONNECT(data);
    DISCONNECT(unknown);
    DISCONNECT(header);
    DISCONNECT(end_of_header);
    DISCONNECT(body);
    DISCONNECT(end_of_message);
    DISCONNECT(abort);
    DISCONNECT(define_macro);
    DISCONNECT(timeout);

    DISCONNECT(finished);

#undef DISCONNECT
    g_signal_handlers_disconnect_by_func(leader,
                                         G_CALLBACK(cb_leader_finished),
                                         finished_data);
}

static void
cb_leader_finished (MilterFinishedEmittable *emittable, gpointer user_data)
{
    LeaderFinishData *finish_data = user_data;
    MilterClientContext *client_context;
    MilterManagerLeader *leader;
    MilterManagerPrivate *priv;

    client_context = finish_data->client_context;

    leader = MILTER_MANAGER_LEADER(emittable);
    teardown_client_context_signals(client_context, leader, finish_data);

    priv = MILTER_MANAGER_GET_PRIVATE(finish_data->manager);
    if (!priv->connection_checking) {
        GList *node;
        node = g_list_find(priv->leaders, leader);
        if (node) {
            if (priv->next_connection_checked_leader == node)
                priv->next_connection_checked_leader = g_list_next(node);
            priv->leaders = g_list_delete_link(priv->leaders, node);
        }
        if (!priv->leaders) {
            milter_debug("[manager][connection-check][dispose] no leaders");
            dispose_periodical_connection_checker(finish_data->manager);
        }
    }

    priv->finished_leaders = g_list_prepend(priv->finished_leaders, leader);

    g_free(finish_data);
}

static void
setup_context_signals (MilterClientContext *context,
                       MilterManager *manager)
{
    MilterManagerLeader *leader;
    MilterManagerPrivate *priv;
    LeaderFinishData *finish_data;

    priv = MILTER_MANAGER_GET_PRIVATE(manager);

    leader = milter_manager_leader_new(priv->configuration, context);
    priv->leaders = g_list_prepend(priv->leaders, leader);

#define CONNECT(name)                                   \
    g_signal_connect(context, #name,                    \
                     G_CALLBACK(cb_client_ ## name),    \
                     leader)

    CONNECT(negotiate);
    CONNECT(connect);
    CONNECT(helo);
    CONNECT(envelope_from);
    CONNECT(envelope_recipient);
    CONNECT(data);
    CONNECT(unknown);
    CONNECT(header);
    CONNECT(end_of_header);
    CONNECT(body);
    CONNECT(end_of_message);
    CONNECT(abort);
    CONNECT(define_macro);
    CONNECT(timeout);

    CONNECT(finished);

#undef CONNECT

    finish_data = g_new(LeaderFinishData, 1);
    finish_data->manager = manager;
    finish_data->client_context = context;
    g_signal_connect(leader, "finished",
                     G_CALLBACK(cb_leader_finished), finish_data);
    milter_manager_leader_set_launcher_channel(leader,
                                               priv->launcher_read_channel,
                                               priv->launcher_write_channel);

    g_signal_emit_by_name(priv->configuration, "connected", leader);
}

static void
connection_established (MilterClient *client, MilterClientContext *context)
{
    MilterManager *manager;

    manager = MILTER_MANAGER(client);
    setup_context_signals(context, MILTER_MANAGER(client));

    milter_debug("[%u] [manager][session][start]",
                 milter_agent_get_tag(MILTER_AGENT(context)));

    start_periodical_connection_checker(manager);
}

static const gchar *
get_connection_spec (MilterClient *client)
{
    MilterManager *manager;
    MilterManagerPrivate *priv;
    MilterManagerConfiguration *configuration;
    const gchar *spec;

    manager = MILTER_MANAGER(client);
    priv = MILTER_MANAGER_GET_PRIVATE(manager);
    configuration = priv->configuration;
    spec =
        milter_manager_configuration_get_manager_connection_spec(configuration);
    if (spec) {
        return spec;
    } else {
        MilterClientClass *klass;

        klass = MILTER_CLIENT_CLASS(milter_manager_parent_class);
        return klass->get_connection_spec(MILTER_CLIENT(manager));
    }
}

static gboolean
set_connection_spec (MilterClient *client, const gchar *spec, GError **error)
{
    MilterClientClass *klass;
    MilterManager *manager;
    gboolean success;

    manager = MILTER_MANAGER(client);
    klass = MILTER_CLIENT_CLASS(milter_manager_parent_class);
    success = klass->set_connection_spec(MILTER_CLIENT(manager), spec, error);
    if (success) {
        MilterManagerPrivate *priv;
        MilterManagerConfiguration *configuration;

        priv = MILTER_MANAGER_GET_PRIVATE(manager);
        configuration = priv->configuration;
        milter_manager_configuration_set_manager_connection_spec(configuration,
                                                                 spec);
    }
    return success;
}

static guint
get_unix_socket_mode (MilterClient *client)
{
    MilterManager *manager;
    MilterManagerPrivate *priv;
    MilterManagerConfiguration *configuration;
    guint mode;

    manager = MILTER_MANAGER(client);
    priv = MILTER_MANAGER_GET_PRIVATE(manager);
    configuration = priv->configuration;
    mode =
        milter_manager_configuration_get_manager_unix_socket_mode(configuration);
    if (mode == 0) {
        MilterClientClass *klass;

        klass = MILTER_CLIENT_CLASS(milter_manager_parent_class);
        mode = klass->get_unix_socket_mode(client);
    }
    return mode;
}

static void
set_unix_socket_mode (MilterClient *client, guint mode)
{
    MilterClientClass *klass;
    MilterManager *manager;
    MilterManagerPrivate *priv;
    MilterManagerConfiguration *configuration;

    manager = MILTER_MANAGER(client);
    klass = MILTER_CLIENT_CLASS(milter_manager_parent_class);
    klass->set_unix_socket_mode(client, mode);

    priv = MILTER_MANAGER_GET_PRIVATE(manager);
    configuration = priv->configuration;
    milter_manager_configuration_set_manager_unix_socket_mode(configuration,
                                                              mode);
}

static const gchar *
get_unix_socket_group (MilterClient *client)
{
    MilterManager *manager;
    MilterManagerPrivate *priv;
    MilterManagerConfiguration *configuration;
    const gchar *group;

    manager = MILTER_MANAGER(client);
    priv = MILTER_MANAGER_GET_PRIVATE(manager);
    configuration = priv->configuration;
    group = milter_manager_configuration_get_manager_unix_socket_group(configuration);
    if (!group) {
        MilterClientClass *klass;

        klass = MILTER_CLIENT_CLASS(milter_manager_parent_class);
        group = klass->get_unix_socket_group(client);
    }
    return group;
}

static void
set_unix_socket_group (MilterClient *client, const gchar *group)
{
    MilterClientClass *klass;
    MilterManager *manager;
    MilterManagerPrivate *priv;
    MilterManagerConfiguration *configuration;

    manager = MILTER_MANAGER(client);
    klass = MILTER_CLIENT_CLASS(milter_manager_parent_class);
    klass->set_unix_socket_group(client, group);

    priv = MILTER_MANAGER_GET_PRIVATE(manager);
    configuration = priv->configuration;
    return milter_manager_configuration_set_manager_unix_socket_group(configuration,
                                                                      group);
}

static gboolean
is_remove_unix_socket_on_close (MilterClient *client)
{
    MilterManager *manager;
    MilterManagerPrivate *priv;
    MilterManagerConfiguration *configuration;

    manager = MILTER_MANAGER(client);
    priv = MILTER_MANAGER_GET_PRIVATE(manager);
    configuration = priv->configuration;
    return milter_manager_configuration_is_remove_manager_unix_socket_on_close(configuration);
}

static guint
get_suspend_time_on_unacceptable (MilterClient *client)
{
    MilterManager *manager;
    MilterManagerPrivate *priv;
    MilterManagerConfiguration *configuration;

    manager = MILTER_MANAGER(client);
    priv = MILTER_MANAGER_GET_PRIVATE(manager);
    configuration = priv->configuration;
    return milter_manager_configuration_get_suspend_time_on_unacceptable(configuration);
}

static void
set_suspend_time_on_unacceptable (MilterClient *client, guint suspend_time)
{
    MilterManager *manager;
    MilterManagerPrivate *priv;
    MilterManagerConfiguration *configuration;

    manager = MILTER_MANAGER(client);
    priv = MILTER_MANAGER_GET_PRIVATE(manager);
    configuration = priv->configuration;
    milter_manager_configuration_set_suspend_time_on_unacceptable(configuration,
                                                                  suspend_time);
}

static guint
get_max_connections (MilterClient *client)
{
    MilterManager *manager;
    MilterManagerPrivate *priv;
    MilterManagerConfiguration *configuration;

    manager = MILTER_MANAGER(client);
    priv = MILTER_MANAGER_GET_PRIVATE(manager);
    configuration = priv->configuration;
    return milter_manager_configuration_get_max_connections(configuration);
}

static void
set_max_connections (MilterClient *client, guint max_connections)
{
    MilterManager *manager;
    MilterManagerPrivate *priv;
    MilterManagerConfiguration *configuration;

    manager = MILTER_MANAGER(client);
    priv = MILTER_MANAGER_GET_PRIVATE(manager);
    configuration = priv->configuration;
    milter_manager_configuration_set_max_connections(configuration,
                                                     max_connections);
}

static const gchar *
get_effective_user (MilterClient *client)
{
    MilterManager *manager;
    MilterManagerPrivate *priv;
    MilterManagerConfiguration *configuration;
    const gchar *effective_user;

    manager = MILTER_MANAGER(client);
    priv = MILTER_MANAGER_GET_PRIVATE(manager);
    configuration = priv->configuration;
    effective_user =
        milter_manager_configuration_get_effective_user(configuration);
    if (!effective_user) {
        MilterClientClass *klass;

        klass = MILTER_CLIENT_CLASS(milter_manager_parent_class);
        effective_user = klass->get_effective_user(client);
    }
    return effective_user;
}

static void
set_effective_user (MilterClient *client, const gchar *effective_user)
{
    MilterClientClass *klass;
    MilterManager *manager;
    MilterManagerPrivate *priv;
    MilterManagerConfiguration *configuration;

    manager = MILTER_MANAGER(client);
    klass = MILTER_CLIENT_CLASS(milter_manager_parent_class);
    klass->set_effective_user(client, effective_user);

    priv = MILTER_MANAGER_GET_PRIVATE(manager);
    configuration = priv->configuration;
    milter_manager_configuration_set_effective_user(configuration,
                                                    effective_user);
}

static const gchar *
get_effective_group (MilterClient *client)
{
    MilterManager *manager;
    MilterManagerPrivate *priv;
    MilterManagerConfiguration *configuration;
    const gchar *effective_group;

    manager = MILTER_MANAGER(client);
    priv = MILTER_MANAGER_GET_PRIVATE(manager);
    configuration = priv->configuration;
    effective_group =
        milter_manager_configuration_get_effective_group(configuration);
    if (!effective_group) {
        MilterClientClass *klass;

        klass = MILTER_CLIENT_CLASS(milter_manager_parent_class);
        effective_group = klass->get_effective_group(client);
    }
    return effective_group;
}

static void
set_effective_group (MilterClient *client, const gchar *effective_group)
{
    MilterClientClass *klass;
    MilterManager *manager;
    MilterManagerPrivate *priv;
    MilterManagerConfiguration *configuration;

    manager = MILTER_MANAGER(client);
    priv = MILTER_MANAGER_GET_PRIVATE(manager);
    klass = MILTER_CLIENT_CLASS(milter_manager_parent_class);
    klass->set_effective_group(client, effective_group);

    configuration = priv->configuration;
    milter_manager_configuration_set_effective_group(configuration,
                                                     effective_group);
}

static guint
get_maintenance_interval (MilterClient *client)
{
    MilterManager *manager;
    MilterManagerPrivate *priv;
    MilterManagerConfiguration *configuration;

    manager = MILTER_MANAGER(client);
    priv = MILTER_MANAGER_GET_PRIVATE(manager);
    configuration = priv->configuration;
    return milter_manager_configuration_get_maintenance_interval(configuration);
}

static void
set_maintenance_interval (MilterClient *client, guint n_sessions)
{
    MilterManager *manager;
    MilterManagerPrivate *priv;
    MilterManagerConfiguration *configuration;

    manager = MILTER_MANAGER(client);
    priv = MILTER_MANAGER_GET_PRIVATE(manager);
    configuration = priv->configuration;
    milter_manager_configuration_set_maintenance_interval(configuration,
                                                          n_sessions);
}

static guint
get_default_packet_buffer_size (MilterClient *client)
{
    MilterManager *manager;
    MilterManagerPrivate *priv;
    MilterManagerConfiguration *configuration;

    manager = MILTER_MANAGER(client);
    priv = MILTER_MANAGER_GET_PRIVATE(manager);
    configuration = priv->configuration;
    return milter_manager_configuration_get_default_packet_buffer_size(configuration);
}

static void
set_default_packet_buffer_size (MilterClient *client, guint size)
{
    MilterManager *manager;
    MilterManagerPrivate *priv;
    MilterManagerConfiguration *configuration;

    manager = MILTER_MANAGER(client);
    priv = MILTER_MANAGER_GET_PRIVATE(manager);
    configuration = priv->configuration;
    milter_manager_configuration_set_default_packet_buffer_size(configuration,
                                                                size);
}

static const gchar *
get_pid_file (MilterClient *client)
{
    MilterManager *manager;
    MilterManagerPrivate *priv;
    MilterManagerConfiguration *configuration;
    const gchar *pid_file;

    manager = MILTER_MANAGER(client);
    priv = MILTER_MANAGER_GET_PRIVATE(manager);
    configuration = priv->configuration;
    pid_file = milter_manager_configuration_get_pid_file(configuration);
    if (!pid_file) {
        MilterClientClass *klass;

        klass = MILTER_CLIENT_CLASS(milter_manager_parent_class);
        pid_file = klass->get_pid_file(client);
    }
    return pid_file;
}

static void
set_pid_file (MilterClient *client, const gchar *pid_file)
{
    MilterClientClass *klass;
    MilterManager *manager;
    MilterManagerPrivate *priv;
    MilterManagerConfiguration *configuration;

    manager = MILTER_MANAGER(client);
    klass = MILTER_CLIENT_CLASS(milter_manager_parent_class);
    klass->set_pid_file(client, pid_file);

    priv = MILTER_MANAGER_GET_PRIVATE(manager);
    configuration = priv->configuration;
    milter_manager_configuration_set_pid_file(configuration, pid_file);
}

static void
maintain (MilterClient *client)
{
    MilterManagerPrivate *priv;

    milter_debug("[manager][maintain]");

    priv = MILTER_MANAGER_GET_PRIVATE(client);
    milter_manager_configuration_maintain(priv->configuration);
}

static void
sessions_finished (MilterClient *client, guint n_finished_sessions)
{
    MilterManagerPrivate *priv;

    milter_debug("[manager][sessions][finished] %u", n_finished_sessions);

    priv = MILTER_MANAGER_GET_PRIVATE(client);
    dispose_finished_leaders(priv);
}

static void
event_loop_created (MilterClient *client, MilterEventLoop *loop)
{
    MilterManagerPrivate *priv;

    milter_debug("[manager][event-loop-created]");

    priv = MILTER_MANAGER_GET_PRIVATE(client);
    milter_manager_configuration_event_loop_created(priv->configuration,
                                                    loop);
}

static guint
get_n_workers (MilterClient *client)
{
    MilterManager *manager;
    MilterManagerPrivate *priv;
    guint n_workers;

    manager = MILTER_MANAGER(client);
    priv = MILTER_MANAGER_GET_PRIVATE(manager);
    if (priv->is_custom_n_workers) {
        MilterClientClass *klass;

        klass = MILTER_CLIENT_CLASS(milter_manager_parent_class);
        n_workers = klass->get_n_workers(client);
    } else {
        MilterManagerConfiguration *configuration;

        configuration = priv->configuration;
        n_workers = milter_manager_configuration_get_n_workers(configuration);
    }
    return n_workers;
}

static void
set_n_workers (MilterClient *client, guint n_workers)
{
    MilterClientClass *klass;
    MilterManager *manager;
    MilterManagerPrivate *priv;

    manager = MILTER_MANAGER(client);

    priv = MILTER_MANAGER_GET_PRIVATE(manager);
    priv->is_custom_n_workers = TRUE;

    klass = MILTER_CLIENT_CLASS(milter_manager_parent_class);
    klass->set_n_workers(client, n_workers);
    milter_manager_configuration_set_n_workers(priv->configuration, n_workers);
}

static MilterClientEventLoopBackend
get_event_loop_backend (MilterClient *client)
{
    MilterClientClass *klass;
    MilterManager *manager;
    MilterClientEventLoopBackend backend;

    manager = MILTER_MANAGER(client);

    klass = MILTER_CLIENT_CLASS(milter_manager_parent_class);
    backend = klass->get_event_loop_backend(client);
    if (backend == MILTER_CLIENT_EVENT_LOOP_BACKEND_DEFAULT) {
        MilterManagerPrivate *priv;
        MilterManagerConfiguration *config;

        priv = MILTER_MANAGER_GET_PRIVATE(manager);
        config = priv->configuration;
        backend = milter_manager_configuration_get_event_loop_backend(config);
    }
    return backend;
}

static void
set_event_loop_backend (MilterClient *client,
                        MilterClientEventLoopBackend backend)
{
    MilterClientClass *klass;
    MilterManager *manager;
    MilterManagerPrivate *priv;

    manager = MILTER_MANAGER(client);

    klass = MILTER_CLIENT_CLASS(milter_manager_parent_class);
    klass->set_event_loop_backend(client, backend);

    priv = MILTER_MANAGER_GET_PRIVATE(manager);
    milter_manager_configuration_set_event_loop_backend(priv->configuration,
                                                        backend);
}

static gboolean
is_run_as_daemon (MilterClient *client)
{
    MilterManager *manager;
    MilterManagerPrivate *priv;
    gboolean run_as_daemon;

    manager = MILTER_MANAGER(client);
    priv = MILTER_MANAGER_GET_PRIVATE(manager);
    if (priv->is_custom_run_as_daemon) {
        MilterClientClass *klass;

        klass = MILTER_CLIENT_CLASS(milter_manager_parent_class);
        run_as_daemon = klass->is_run_as_daemon(client);
    } else {
        MilterManagerConfiguration *configuration;

        configuration = priv->configuration;
        run_as_daemon = milter_manager_configuration_is_daemon(configuration);
    }
    return run_as_daemon;
}

static void
set_run_as_daemon (MilterClient *client, gboolean run_as_daemon)
{
    MilterClientClass *klass;
    MilterManager *manager;
    MilterManagerPrivate *priv;

    manager = MILTER_MANAGER(client);

    priv = MILTER_MANAGER_GET_PRIVATE(manager);
    priv->is_custom_run_as_daemon = TRUE;

    klass = MILTER_CLIENT_CLASS(milter_manager_parent_class);
    klass->set_run_as_daemon(client, run_as_daemon);
    milter_manager_configuration_set_daemon(priv->configuration, run_as_daemon);
}

static GPid
fork_delegate (MilterClient *client)
{
    MilterManager *manager;
    MilterManagerPrivate *priv;

    manager = MILTER_MANAGER(client);
    priv = MILTER_MANAGER_GET_PRIVATE(manager);
    return milter_manager_configuration_fork(priv->configuration);
}

static guint
get_max_pending_finished_sessions (MilterClient *client)
{
    MilterManager *manager;
    MilterManagerPrivate *priv;
    guint n_sessions;

    manager = MILTER_MANAGER(client);
    priv = MILTER_MANAGER_GET_PRIVATE(manager);
    if (priv->is_custom_max_pending_finished_sessions) {
        MilterClientClass *klass;

        klass = MILTER_CLIENT_CLASS(milter_manager_parent_class);
        n_sessions = klass->get_max_pending_finished_sessions(client);
    } else {
        MilterManagerConfiguration *configuration;

        configuration = priv->configuration;
        n_sessions =
            milter_manager_configuration_get_max_pending_finished_sessions(configuration);
    }
    return n_sessions;
}

static void
set_max_pending_finished_sessions (MilterClient *client, guint n_sessions)
{
    MilterClientClass *klass;
    MilterManager *manager;
    MilterManagerPrivate *priv;

    manager = MILTER_MANAGER(client);

    priv = MILTER_MANAGER_GET_PRIVATE(manager);
    priv->is_custom_max_pending_finished_sessions = TRUE;

    klass = MILTER_CLIENT_CLASS(milter_manager_parent_class);
    klass->set_max_pending_finished_sessions(client, n_sessions);
    milter_manager_configuration_set_max_pending_finished_sessions(priv->configuration,
                                                                   n_sessions);
}

MilterManagerConfiguration *
milter_manager_get_configuration (MilterManager *manager)
{
    return MILTER_MANAGER_GET_PRIVATE(manager)->configuration;
}

const GList *
milter_manager_get_leaders (MilterManager *manager)
{
    return MILTER_MANAGER_GET_PRIVATE(manager)->leaders;
}

static void
apply_syslog_parameters (MilterManager *manager)
{
    MilterClient *client;
    MilterManagerPrivate *priv;
    MilterManagerConfiguration *configuration;

    client = MILTER_CLIENT(manager);
    priv = MILTER_MANAGER_GET_PRIVATE(manager);
    configuration = priv->configuration;

    if (milter_manager_configuration_get_use_syslog(configuration)) {
        if (milter_client_get_syslog_enabled(client)) {
            const gchar *facility;
            facility =
                milter_manager_configuration_get_syslog_facility(configuration);
            milter_client_set_syslog_facility(client, facility);
        } else {
            milter_client_start_syslog(client);
        }
    } else {
        milter_client_stop_syslog(client);
    }
}

static void
apply_custom_parameters (MilterManager *manager)
{
    MilterClient *client;
    MilterClientClass *klass;
    MilterManagerPrivate *priv;
    MilterManagerConfiguration *configuration;
    const gchar *spec;
    const gchar *pid_file;
    const gchar *effective_user;
    const gchar *effective_group;
    guint unix_socket_mode;
    const gchar *unix_socket_group;
    MilterClientEventLoopBackend backend;

    client = MILTER_CLIENT(manager);
    priv = MILTER_MANAGER_GET_PRIVATE(manager);
    configuration = priv->configuration;

    klass = MILTER_CLIENT_CLASS(milter_manager_parent_class);

    spec = klass->get_connection_spec(client);
    if (spec) {
        milter_manager_configuration_set_manager_connection_spec(configuration,
                                                                 spec);
        milter_manager_configuration_reset_location(configuration,
                                                    "manager.connection_spec");
    }

    pid_file = klass->get_pid_file(client);
    if (pid_file) {
        milter_manager_configuration_set_pid_file(configuration, pid_file);
        milter_manager_configuration_reset_location(configuration,
                                                    "manager.pid_file");
    }

    effective_user = klass->get_effective_user(client);
    if (effective_user) {
        milter_manager_configuration_set_effective_user(configuration,
                                                        effective_user);
        milter_manager_configuration_reset_location(configuration,
                                                    "security.effective_user");
    }

    effective_group = klass->get_effective_group(client);
    if (effective_group) {
        milter_manager_configuration_set_effective_group(configuration,
                                                         effective_group);
        milter_manager_configuration_reset_location(configuration,
                                                    "security.effective_grourp");
    }

    unix_socket_mode = klass->get_unix_socket_mode(client);
    if (unix_socket_mode) {
        milter_manager_configuration_set_manager_unix_socket_mode(configuration,
                                                                  unix_socket_mode);
        milter_manager_configuration_reset_location(configuration,
                                                    "manager.unix_socket_mode");
    }

    unix_socket_group = klass->get_unix_socket_group(client);
    if (unix_socket_group) {
        milter_manager_configuration_set_manager_unix_socket_group(configuration,
                                                                   unix_socket_group);
        milter_manager_configuration_reset_location(configuration,
                                                    "manager.unix_socket_group");
    }

    if (priv->is_custom_n_workers) {
        guint n_workers;

        n_workers = klass->get_n_workers(client);
        milter_manager_configuration_set_n_workers(configuration, n_workers);
        milter_manager_configuration_reset_location(configuration,
                                                    "manager.n_workers");
    }

    backend = klass->get_event_loop_backend(client);
    if (backend != MILTER_CLIENT_EVENT_LOOP_BACKEND_DEFAULT) {
        milter_manager_configuration_set_event_loop_backend(configuration,
                                                            backend);
        milter_manager_configuration_reset_location(configuration,
                                                    "manager.event_loop_backend");
    }

    if (priv->is_custom_max_pending_finished_sessions) {
        guint n_sessions;

        n_sessions = klass->get_max_pending_finished_sessions(client);
        milter_manager_configuration_set_max_pending_finished_sessions(configuration,
                                                                       n_sessions);
        milter_manager_configuration_reset_location(configuration,
                                                    "manager.max_pending_finished_sessions");
    }
}

gboolean
milter_manager_reload (MilterManager *manager, GError **error)
{
    MilterManagerPrivate *priv;
    gboolean success;

    priv = MILTER_MANAGER_GET_PRIVATE(manager);
    success = milter_manager_configuration_reload(priv->configuration, error);
    apply_syslog_parameters(manager);
    apply_custom_parameters(manager);
    return success;
}

void
milter_manager_set_launcher_channel (MilterManager *manager,
                                     GIOChannel *read_channel,
                                     GIOChannel *write_channel)
{
    MilterManagerPrivate *priv;

    priv = MILTER_MANAGER_GET_PRIVATE(manager);

    if (priv->launcher_write_channel)
        g_io_channel_unref(priv->launcher_write_channel);
    priv->launcher_write_channel = write_channel;
    if (priv->launcher_write_channel)
        g_io_channel_ref(priv->launcher_write_channel);

    if (priv->launcher_read_channel)
        g_io_channel_unref(priv->launcher_read_channel);
    priv->launcher_read_channel = read_channel;
    if (priv->launcher_read_channel)
        g_io_channel_ref(priv->launcher_read_channel);
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
