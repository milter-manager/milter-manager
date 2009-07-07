/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Copyright (C) 2008-2009  Kouhei Sutou <kou@clear-code.com>
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
    MilterSyslogLogger *logger;
    GList *leaders;

    GIOChannel *launcher_read_channel;
    GIOChannel *launcher_write_channel;
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
static gchar *get_default_connection_spec (MilterClient *client);
static guint  get_unix_socket_mode        (MilterClient *client);
static const gchar *get_unix_socket_group (MilterClient *client);
static gboolean is_remove_unix_socket_on_close (MilterClient *client);
static guint  get_suspend_time_on_unacceptable
                                          (MilterClient *client);
static void   set_suspend_time_on_unacceptable
                                          (MilterClient *client,
                                           guint         suspend_time);
static void   cb_leader_finished          (MilterFinishedEmittable *emittable,
                                           gpointer user_data);

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
    client_class->get_default_connection_spec = get_default_connection_spec;
    client_class->get_unix_socket_mode        = get_unix_socket_mode;
    client_class->get_unix_socket_group       = get_unix_socket_group;
    client_class->is_remove_unix_socket_on_close = is_remove_unix_socket_on_close;
    client_class->get_suspend_time_on_unacceptable = get_suspend_time_on_unacceptable;
    client_class->set_suspend_time_on_unacceptable = set_suspend_time_on_unacceptable;

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
    priv->logger = milter_syslog_logger_new("milter-manager");

    priv->launcher_read_channel = NULL;
    priv->launcher_write_channel = NULL;
}

static void
dispose (GObject *object)
{
    MilterManagerPrivate *priv;

    priv = MILTER_MANAGER_GET_PRIVATE(object);

    if (priv->configuration) {
        g_object_unref(priv->configuration);
        priv->configuration = NULL;
    }

    if (priv->leaders) {
        g_list_foreach(priv->leaders, (GFunc)g_object_unref, NULL);
        g_list_free(priv->leaders);
        priv->leaders = NULL;
    }

    if (priv->logger) {
        g_object_unref(priv->logger);
        priv->logger = NULL;
    }
    milter_manager_set_launcher_channel(MILTER_MANAGER(object), NULL, NULL);

    G_OBJECT_CLASS(milter_manager_parent_class)->dispose(object);
}

static void
set_property (GObject      *object,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
    MilterManagerPrivate *priv;

    priv = MILTER_MANAGER_GET_PRIVATE(object);
    switch (prop_id) {
      case PROP_CONFIGURATION:
        if (priv->configuration)
            g_object_unref(priv->configuration);
        priv->configuration = g_value_get_object(value);
        if (priv->configuration)
            g_object_ref(priv->configuration);
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
                        NULL);
}

static MilterStatus
cb_client_negotiate (MilterClientContext *context, MilterOption *option,
                     MilterMacrosRequests *macros_requests, gpointer user_data)
{
    MilterManagerLeader *leader = user_data;
    gchar *inspected_option, *inspected_macros_requests;

    inspected_option = milter_option_inspect(option);
    inspected_macros_requests = milter_utils_inspect_object(G_OBJECT(macros_requests));
    milter_debug("[%u] [manager][receive][negotiate] <%s>:<%s>",
                 milter_agent_get_tag(MILTER_AGENT(context)),
                 inspected_option, inspected_macros_requests);
    g_free(inspected_option);
    g_free(inspected_macros_requests);

    return milter_manager_leader_negotiate(leader, option, macros_requests);
}

static MilterStatus
cb_client_connect (MilterClientContext *context, const gchar *host_name,
                   struct sockaddr *address, socklen_t address_length,
                   gpointer user_data)
{
    MilterManagerLeader *leader = user_data;
    gchar *spec;

    spec = milter_connection_address_to_spec(address);
    milter_debug("[%u] [manager][receive][connect] <%s>:<%s>",
                 milter_agent_get_tag(MILTER_AGENT(context)),
                 host_name, spec);
    g_free(spec);

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
    gchar *state_name;

    state_name =
        milter_utils_get_enum_nick_name(MILTER_TYPE_CLIENT_CONTEXT_STATE,
                                        state);
    milter_debug("[%u] [manager][receive][abort][%s]",
                 milter_agent_get_tag(MILTER_AGENT(context)),
                 state_name);
    g_free(state_name);

    return milter_manager_leader_abort(leader);
}

static void
cb_client_define_macro (MilterClientContext *context, MilterCommand command,
                        GHashTable *macros, gpointer user_data)
{
    MilterManagerLeader *leader = user_data;
    gchar *inspected_macros;

    inspected_macros = milter_utils_inspect_hash_string_string(macros);
    milter_debug("[%u] [manager][receive][define-macro] <%s>",
                 milter_agent_get_tag(MILTER_AGENT(context)),
                 inspected_macros);
    g_free(inspected_macros);

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
    GTimer *timer;
    gdouble elapsed;
    MilterClientContextState state, last_state;
    MilterStatus status;
    gchar *state_name, *last_state_name;
    gchar *status_name, *statistics_status_name;
    guint tag;

    timer = milter_client_context_get_private_data(context);
    g_timer_stop(timer);
    elapsed = g_timer_elapsed(timer, NULL);


    state = milter_client_context_get_state(context);
    state_name =
        milter_utils_get_enum_nick_name(MILTER_TYPE_CLIENT_CONTEXT_STATE,
                                        state);
    last_state = milter_client_context_get_last_state(context);
    last_state_name =
        milter_utils_get_enum_nick_name(MILTER_TYPE_CLIENT_CONTEXT_STATE,
                                        last_state);
    status = milter_client_context_get_status(context);
    status_name = milter_utils_get_enum_nick_name(MILTER_TYPE_STATUS,
                                                  status);
    tag = milter_agent_get_tag(MILTER_AGENT(context));
    milter_debug("[%u] [manager][finish][%s][%s][%g]",
                 tag, state_name, status_name, elapsed);
    if (MILTER_STATUS_IS_PASS(status)) {
        statistics_status_name = "pass";
    } else {
        statistics_status_name = status_name;
    }
    milter_statistics("[session][end][%s][%s][%g](%u)",
                      last_state_name, statistics_status_name, elapsed, tag);
    g_free(status_name);
    g_free(state_name);
    g_free(last_state_name);

    milter_manager_leader_quit(leader);
}

static void
teardown_client_context_signals (MilterClientContext *context,
                                 MilterManagerLeader *leader)
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
                                         context);
}

static gboolean
cb_idle_notify_finish_session_to_configuration (gpointer data)
{
    MilterManagerConfiguration *config = data;

    milter_manager_configuration_session_finished(config);

    return FALSE;
}

static gboolean
cb_idle_unref_leader (gpointer data)
{
    MilterManagerLeader *leader = data;

    g_object_unref(leader);

    return FALSE;
}

static void
cb_leader_finished (MilterFinishedEmittable *emittable, gpointer user_data)
{
    MilterClientContext *client_context = user_data;
    GTimer *timer;
    MilterManagerLeader *leader;
    MilterManagerConfiguration *config;

    timer = milter_client_context_get_private_data(client_context);
    g_timer_stop(timer);

    leader = MILTER_MANAGER_LEADER(emittable);
    teardown_client_context_signals(client_context, leader);

    config = milter_manager_leader_get_configuration(leader);
    g_idle_add(cb_idle_notify_finish_session_to_configuration, config);
    g_idle_add(cb_idle_unref_leader, leader);
}

static void
setup_context_signals (MilterClientContext *context,
                       MilterManager *manager)
{
    MilterManagerLeader *leader;
    MilterManagerPrivate *priv;

    priv = MILTER_MANAGER_GET_PRIVATE(manager);

    leader = milter_manager_leader_new(priv->configuration, context);

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
    g_signal_connect(leader, "finished",
                     G_CALLBACK(cb_leader_finished), context);
    milter_manager_leader_set_launcher_channel(leader,
                                               priv->launcher_read_channel,
                                               priv->launcher_write_channel);
}

static void
connection_established (MilterClient *client, MilterClientContext *context)
{
    GTimer *timer;
    guint tag;

    setup_context_signals(context, MILTER_MANAGER(client));

    timer = g_timer_new();
    milter_client_context_set_private_data(context, timer,
                                           (GDestroyNotify)g_timer_destroy);

    tag = milter_agent_get_tag(MILTER_AGENT(context));
    milter_debug("[%u] [manager][session][start]", tag),
    milter_statistics("[session][start](%u)", tag);
}

static gchar *
get_default_connection_spec (MilterClient *client)
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
        return g_strdup(spec);
    } else {
        MilterClientClass *klass;

        klass = MILTER_CLIENT_CLASS(milter_manager_parent_class);
        return klass->get_default_connection_spec(MILTER_CLIENT(manager));
    }
}

static guint
get_unix_socket_mode (MilterClient *client)
{
    MilterManager *manager;
    MilterManagerPrivate *priv;
    MilterManagerConfiguration *configuration;

    manager = MILTER_MANAGER(client);
    priv = MILTER_MANAGER_GET_PRIVATE(manager);
    configuration = priv->configuration;
    return milter_manager_configuration_get_manager_unix_socket_mode(configuration);
}

static const gchar *
get_unix_socket_group (MilterClient *client)
{
    MilterManager *manager;
    MilterManagerPrivate *priv;
    MilterManagerConfiguration *configuration;

    manager = MILTER_MANAGER(client);
    priv = MILTER_MANAGER_GET_PRIVATE(manager);
    configuration = priv->configuration;
    return milter_manager_configuration_get_manager_unix_socket_group(configuration);
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

MilterManagerConfiguration *
milter_manager_get_configuration (MilterManager *manager)
{
    return MILTER_MANAGER_GET_PRIVATE(manager)->configuration;
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
