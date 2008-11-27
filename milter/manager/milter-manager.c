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

#include <locale.h>
#include <glib/gi18n.h>

#include <signal.h>
#include <errno.h>

#include "../manager.h"

static gboolean initialized = FALSE;
static MilterClient *current_client = NULL;

void
milter_manager_init (int *argc, char ***argv)
{
    if (initialized)
        return;

    initialized = TRUE;

    setlocale(LC_ALL, "");
    bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
    textdomain(GETTEXT_PACKAGE);

    g_type_init();
    if (!g_thread_supported())
        g_thread_init(NULL);

    _milter_manager_configuration_init();
}

void
milter_manager_quit (void)
{
    if (!initialized)
        return;

    _milter_manager_configuration_quit();
}

static MilterStatus
cb_client_negotiate (MilterClientContext *context, MilterOption *option,
                     gpointer user_data)
{
    MilterManagerLeader *leader = user_data;

    return milter_manager_leader_negotiate(leader, option);
}

static MilterStatus
cb_client_connect (MilterClientContext *context, const gchar *host_name,
                   struct sockaddr *address, socklen_t address_length,
                   gpointer user_data)
{
    MilterManagerLeader *leader = user_data;

    return milter_manager_leader_connect(leader, host_name,
                                         address, address_length);
}

static MilterStatus
cb_client_helo (MilterClientContext *context, const gchar *fqdn,
                gpointer user_data)
{
    MilterManagerLeader *leader = user_data;

    return milter_manager_leader_helo(leader, fqdn);
}

static MilterStatus
cb_client_envelope_from (MilterClientContext *context, const gchar *from,
                         gpointer user_data)
{
    MilterManagerLeader *leader = user_data;

    return milter_manager_leader_envelope_from(leader, from);
}

static MilterStatus
cb_client_envelope_recipient (MilterClientContext *context,
                              const gchar *recipient, gpointer user_data)
{
    MilterManagerLeader *leader = user_data;

    return milter_manager_leader_envelope_recipient(leader, recipient);
}

static MilterStatus
cb_client_data (MilterClientContext *context, gpointer user_data)
{
    MilterManagerLeader *leader = user_data;

    return milter_manager_leader_data(leader);
}

static MilterStatus
cb_client_unknown (MilterClientContext *context, const gchar *command,
                   gpointer user_data)
{
    MilterManagerLeader *leader = user_data;

    return milter_manager_leader_unknown(leader, command);
}

static MilterStatus
cb_client_header (MilterClientContext *context,
                  const gchar *name, const gchar *value,
                  gpointer user_data)
{
    MilterManagerLeader *leader = user_data;

    return milter_manager_leader_header(leader, name, value);
}

static MilterStatus
cb_client_end_of_header (MilterClientContext *context, gpointer user_data)
{
    MilterManagerLeader *leader = user_data;

    return milter_manager_leader_end_of_header(leader);
}

static MilterStatus
cb_client_body (MilterClientContext *context, const gchar *chunk, gsize size,
                gpointer user_data)
{
    MilterManagerLeader *leader = user_data;

    return milter_manager_leader_body(leader, chunk, size);
}

static MilterStatus
cb_client_end_of_message (MilterClientContext *context, gpointer user_data)
{
    MilterManagerLeader *leader = user_data;

    return milter_manager_leader_end_of_message(leader, NULL, 0);
}

static MilterStatus
cb_client_abort (MilterClientContext *context, gpointer user_data)
{
    MilterManagerLeader *leader = user_data;

    return milter_manager_leader_abort(leader);
}

static void
cb_client_define_macro (MilterClientContext *context, MilterCommand command,
                        GHashTable *macros, gpointer user_data)
{
    MilterManagerLeader *leader = user_data;

    milter_manager_leader_define_macro(leader, command, macros);
}

static void
cb_client_mta_timeout (MilterClientContext *context, gpointer user_data)
{
    MilterManagerLeader *leader = user_data;

    milter_manager_leader_mta_timeout(leader);
}

static MilterStatus
cb_client_quit (MilterClientContext *context, gpointer user_data)
{
    MilterManagerLeader *leader = user_data;
    MilterStatus status;

    status = milter_manager_leader_quit(leader);

#define DISCONNECT(name)                                                \
    g_signal_handlers_disconnect_by_func(context,                       \
                                         G_CALLBACK(cb_client_ ## name), \
                                         user_data)

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
    DISCONNECT(quit);
    DISCONNECT(abort);
    DISCONNECT(define_macro);
    DISCONNECT(mta_timeout);

#undef DISCONNECT

    g_object_unref(leader);

    return status;
}

static void
setup_context_signals (MilterClientContext *context,
                       MilterManagerConfiguration *configuration)
{
    MilterManagerLeader *leader;

    leader = milter_manager_leader_new(configuration, context);

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
    CONNECT(quit);
    CONNECT(abort);
    CONNECT(define_macro);
    CONNECT(mta_timeout);

#undef CONNECT
}

static void
shutdown_client (int signum)
{
    if (current_client)
        milter_client_shutdown(current_client);
}

static void
cb_connection_established (MilterClient *client, MilterClientContext *context,
                           gpointer user_data)
{
    MilterManagerConfiguration *configuration = user_data;
    setup_context_signals(context, configuration);
}

static void
cb_error (MilterAgent *agent, GError *error, gpointer user_data)
{
    g_print("MilterManager error: %s\n", error->message);
}

static gboolean
cb_free_controller (gpointer data)
{
    MilterManagerController *controller = data;

    g_object_unref(controller);
    return FALSE;
}

static void
cb_control_reader_finished (MilterReader *reader, gpointer data)
{
    MilterManagerController *controller = data;

    g_idle_add(cb_free_controller, controller);
}

static void
process_control_connection(MilterManagerConfiguration *configuration,
                           GIOChannel *agent_channel)
{
    MilterManagerController *controller;
    MilterWriter *writer;
    MilterReader *reader;

    controller = milter_manager_controller_new(configuration);

    writer = milter_writer_io_channel_new(agent_channel);
    milter_agent_set_writer(MILTER_AGENT(controller), writer);
    g_object_unref(writer);

    reader = milter_reader_io_channel_new(agent_channel);
    milter_agent_set_reader(MILTER_AGENT(controller), reader);
    g_object_unref(reader);

    g_signal_connect(reader, "finished",
                     G_CALLBACK(cb_control_reader_finished), controller);
}

static gboolean
accept_control_connection (gint control_fd,
                           MilterManagerConfiguration *configuration)
{
    gint agent_fd;
    GIOChannel *agent_channel;

    milter_info("start accepting...: %d", control_fd);
    agent_fd = accept(control_fd, NULL, NULL);
    if (agent_fd == -1) {
        milter_error("failed to accept(): %s", g_strerror(errno));
        return TRUE;
    }

    milter_info("accepted!: %d", agent_fd);
    agent_channel = g_io_channel_unix_new(agent_fd);
    g_io_channel_set_encoding(agent_channel, NULL, NULL);
    g_io_channel_set_flags(agent_channel, G_IO_FLAG_NONBLOCK, NULL);
    g_io_channel_set_close_on_unref(agent_channel, TRUE);
    process_control_connection(configuration, agent_channel);
    g_io_channel_unref(agent_channel);

    return TRUE;
}

static gboolean
control_watch_func (GIOChannel *channel, GIOCondition condition, gpointer data)
{
    MilterManagerConfiguration *configuration = data;
    gboolean keep_callback = TRUE;

    if (condition & G_IO_IN ||
        condition & G_IO_PRI) {
        guint fd;

        fd = g_io_channel_unix_get_fd(channel);
        keep_callback = accept_control_connection(fd, configuration);
    }

    if (condition & G_IO_ERR ||
        condition & G_IO_HUP ||
        condition & G_IO_NVAL) {
        gchar *message;

        message = milter_utils_inspect_io_condition_error(condition);
        milter_error("%s", message);
        g_free(message);
        keep_callback = FALSE;
    }

    return keep_callback;
}


static guint
setup_control_connection (MilterManagerConfiguration *configuration)
{
    const gchar *spec;
    GIOChannel *channel;
    GError *error = NULL;
    guint watch_id = 0;

    spec = milter_manager_configuration_get_control_connection_spec(configuration);
    if (!spec) {
        milter_info("control connection spec is missing. "
                    "control connection is disabled");
        return 0;
    }

    channel = milter_connection_listen(spec, 5, &error);
    if (!channel) {
        milter_error("failed to listen control connection: <%s>:<%s>",
                     spec, error->message);
        g_error_free(error);
        return 0;
    }

    watch_id = g_io_add_watch(channel,
                              G_IO_IN | G_IO_PRI |
                              G_IO_ERR | G_IO_HUP | G_IO_NVAL,
                              control_watch_func, configuration);

    return watch_id;
}

void
milter_manager_main (void)
{
    MilterClient *client;
    MilterManagerConfiguration *configuration;
    void (*sigint_handler) (int signum);
    const gchar *config_dir_env;
    guint control_connection_watch_id = 0;
    GError *error = NULL;

    configuration = milter_manager_configuration_new(NULL);
    config_dir_env = g_getenv("MILTER_MANAGER_CONFIG_DIR");
    if (config_dir_env)
        milter_manager_configuration_add_load_path(configuration,
                                                   config_dir_env);
    milter_manager_configuration_add_load_path(configuration, CONFIG_DIR);
    milter_manager_configuration_reload(configuration);

    /* FIXME */
    control_connection_watch_id = setup_control_connection(configuration);

    client = milter_client_new();
    g_signal_connect(client, "error",
                     G_CALLBACK(cb_error), NULL);

    if (!milter_client_set_connection_spec(client, "inet:10025", &error)) {
        g_object_unref(client);
        g_object_unref(configuration);
        g_print("%s\n", error->message);
        return;
    }

    g_signal_connect(client, "connection-established",
                     G_CALLBACK(cb_connection_established), configuration);

    current_client = client;
    sigint_handler = signal(SIGINT, shutdown_client);
    if (!milter_client_main(client))
        g_print("Failed to start milter-manager process.\n");
    signal(SIGINT, sigint_handler);

    current_client = NULL;
    g_object_unref(configuration);
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
