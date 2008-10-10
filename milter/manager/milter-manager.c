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

#include "../manager.h"

static gboolean initialized = FALSE;
static MilterClient *current_client = NULL;

static void
cb_log (MilterLogger *logger,
        const gchar *domain, MilterLogLevelFlags level, const gchar *message,
        gpointer user_data)
{
    g_print("%s", message);
}

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

    g_signal_connect(milter_logger(), "log", G_CALLBACK(cb_log), NULL);

    milter_manager_controller_init();
}

void
milter_manager_quit (void)
{
    if (!initialized)
        return;

    milter_manager_controller_quit();

    g_signal_handlers_disconnect_by_func(milter_logger(),
                                         G_CALLBACK(cb_log), NULL);
}

static MilterStatus
cb_client_negotiate (MilterClientContext *context, MilterOption *option,
                     gpointer user_data)
{
    MilterManagerController *controller = user_data;

    milter_manager_controller_negotiate(controller, option);

    return MILTER_STATUS_DEFAULT;
}

static MilterStatus
cb_client_connect (MilterClientContext *context, const gchar *host_name,
                   struct sockaddr *address, socklen_t address_length,
                   gpointer user_data)
{
    MilterManagerController *controller = user_data;

    milter_manager_controller_connect(controller, host_name,
                                      address, address_length);

    return MILTER_STATUS_DEFAULT;
}

static MilterStatus
cb_client_helo (MilterClientContext *context, const gchar *fqdn,
                gpointer user_data)
{
    MilterManagerController *controller = user_data;

    milter_manager_controller_helo(controller, fqdn);

    return MILTER_STATUS_DEFAULT;
}

static MilterStatus
cb_client_envelope_from (MilterClientContext *context, const gchar *from,
                         gpointer user_data)
{
    MilterManagerController *controller = user_data;

    milter_manager_controller_envelope_from(controller, from);

    return MILTER_STATUS_DEFAULT;
}

static MilterStatus
cb_client_envelope_receipt (MilterClientContext *context, const gchar *receipt,
                            gpointer user_data)
{
    MilterManagerController *controller = user_data;

    milter_manager_controller_envelope_receipt(controller, receipt);

    return MILTER_STATUS_DEFAULT;
}

static MilterStatus
cb_client_data (MilterClientContext *context, gpointer user_data)
{
    MilterManagerController *controller = user_data;

    milter_manager_controller_data(controller);

    return MILTER_STATUS_DEFAULT;
}

static MilterStatus
cb_client_unknown (MilterClientContext *context, const gchar *command,
                   gpointer user_data)
{
    MilterManagerController *controller = user_data;

    milter_manager_controller_unknown(controller, command);

    return MILTER_STATUS_DEFAULT;
}

static MilterStatus
cb_client_header (MilterClientContext *context,
                  const gchar *name, const gchar *value,
                  gpointer user_data)
{
    MilterManagerController *controller = user_data;

    milter_manager_controller_header(controller, name, value);

    return MILTER_STATUS_DEFAULT;
}

static MilterStatus
cb_client_end_of_header (MilterClientContext *context, gpointer user_data)
{
    MilterManagerController *controller = user_data;

    milter_manager_controller_end_of_header(controller);

    return MILTER_STATUS_DEFAULT;
}

static MilterStatus
cb_client_body (MilterClientContext *context, const guchar *chunk, gsize size,
                gpointer user_data)
{
    MilterManagerController *controller = user_data;

    milter_manager_controller_body(controller, chunk, size);

    return MILTER_STATUS_DEFAULT;
}

static MilterStatus
cb_client_end_of_message (MilterClientContext *context, gpointer user_data)
{
    MilterManagerController *controller = user_data;

    milter_manager_controller_end_of_message(controller);

    return MILTER_STATUS_DEFAULT;
}

static MilterStatus
cb_client_abort (MilterClientContext *context, gpointer user_data)
{
    MilterManagerController *controller = user_data;

    milter_manager_controller_abort(controller);

    return MILTER_STATUS_DEFAULT;
}

static MilterStatus
cb_client_close (MilterClientContext *context, gpointer user_data)
{
    MilterManagerController *controller = user_data;

    milter_manager_controller_close(controller);

#define DISCONNECT(name)                                                \
    g_signal_handlers_disconnect_by_func(context,                       \
                                         G_CALLBACK(cb_client_ ## name), \
                                         user_data)

    DISCONNECT(negotiate);
    DISCONNECT(connect);
    DISCONNECT(helo);
    DISCONNECT(envelope_from);
    DISCONNECT(envelope_receipt);
    DISCONNECT(data);
    DISCONNECT(unknown);
    DISCONNECT(header);
    DISCONNECT(end_of_header);
    DISCONNECT(body);
    DISCONNECT(end_of_message);
    DISCONNECT(close);
    DISCONNECT(abort);

#undef DISCONNECT

    return MILTER_STATUS_DEFAULT;
}

static void
context_setup (MilterClientContext *context, gpointer user_data)
{
    MilterManagerController *controller = user_data;

#define CONNECT(name)                                   \
    g_signal_connect(context, #name,                    \
                     G_CALLBACK(cb_client_ ## name),    \
                     controller)

    CONNECT(negotiate);
    CONNECT(connect);
    CONNECT(helo);
    CONNECT(envelope_from);
    CONNECT(envelope_receipt);
    CONNECT(data);
    CONNECT(unknown);
    CONNECT(header);
    CONNECT(end_of_header);
    CONNECT(body);
    CONNECT(end_of_message);
    CONNECT(close);
    CONNECT(abort);

#undef CONNECT
}

static void
context_teardown (MilterClientContext *context, gpointer user_data)
{
}

static void
shutdown_client (int signum)
{
    if (current_client)
        milter_client_shutdown(current_client);
}

void
milter_manager_main (void)
{
    MilterClient *client;
    MilterManagerConfiguration *configuration;
    MilterManagerController *controller;
    void (*sigint_handler) (int signum);

    configuration = milter_manager_configuration_new();
    controller = milter_manager_controller_new("ruby",
                                               "configuration", configuration,
                                               NULL);
    milter_manager_controller_load(controller, "/tmp/milter-manager.conf");

    client = milter_client_new();
    milter_client_set_connection_spec(client, "inet:9999", NULL);
    milter_client_set_context_setup_func(client, context_setup, controller);
    milter_client_set_context_teardown_func(client, context_teardown, NULL);

    current_client = client;
    sigint_handler = signal(SIGINT, shutdown_client);
    milter_client_main(client);
    signal(SIGINT, sigint_handler);
    current_client = NULL;
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
