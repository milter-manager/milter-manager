/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Copyright (C) 2008  Kouhei Sutou <kou@cozmixng.org>
 *
 *  This program is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License version 3 as published by the
 *  Free Software Foundation with the addition of the following permission added
 *  to Section 15 as permitted in Section 7(a): FOR ANY PART OF THE COVERED WORK
 *  IN WHICH THE COPYRIGHT IS OWNED BY SUGARCRM, SUGARCRM DISCLAIMS THE WARRANTY
 *  OF NON INFRINGEMENT OF THIRD PARTY RIGHTS.
 *
 *  This program is distributed in the hope that it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 *  details.
 *
 *  You should have received a copy of the GNU General Public License along with
 *  this program; if not, see http://www.gnu.org/licenses or write to the Free
 *  Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 *  02110-1301 USA.
 *
 */

#ifdef HAVE_CONFIG_H
#  include "../config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <signal.h>
#include <glib/gi18n.h>

#include <milter/client.h>

static gchar *spec = NULL;
static gboolean verbose = FALSE;
static MilterSyslogLogger *logger = NULL;
static MilterClient *client = NULL;

static gboolean
print_version (const gchar *option_name,
               const gchar *value,
               gpointer data,
               GError **error)
{
    g_print("%s\n", VERSION);
    exit(EXIT_SUCCESS);
    return TRUE;
}

static gboolean
parse_spec_arg (const gchar *option_name,
                const gchar *value,
                gpointer data,
                GError **error)
{
    GError *spec_error = NULL;
    gboolean success;

    success = milter_connection_parse_spec(value, NULL, NULL, NULL, &spec_error);
    if (success) {
        spec = g_strdup(value);
    } else {
        g_set_error(error,
                    G_OPTION_ERROR,
                    G_OPTION_ERROR_BAD_VALUE,
                    _("%s"), spec_error->message);
        g_error_free(spec_error);
    }

    return success;
}

static const GOptionEntry option_entries[] =
{
    {"spec", 's', 0, G_OPTION_ARG_CALLBACK, parse_spec_arg,
     N_("The address of the desired communication socket."), "PROTOCOL:ADDRESS"},
    {"verbose", 'v', G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_NONE, &verbose,
     N_("Be verbose"), NULL},
    {"version", 0, G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK, print_version,
     N_("Show version"), NULL},
    {NULL}
};

static MilterStatus
cb_negotiate (MilterClientContext *context, MilterOption *option,
              gpointer user_data)
{
    MilterActionFlags action;
    MilterStepFlags step;
    gchar *action_names;
    gchar *step_names;

    action = milter_option_get_action(option);
    step = milter_option_get_step(option);

    action_names = milter_utils_get_flags_names(MILTER_TYPE_ACTION_FLAGS,
                                                action);
    step_names = milter_utils_get_flags_names(MILTER_TYPE_STEP_FLAGS, step);

    g_print("negotiate: version=<%u>, action=<%s>, step=<%s>\n",
            milter_option_get_version(option), action_names, step_names);

    g_free(action_names);
    g_free(step_names);

    return MILTER_STATUS_ALL_OPTIONS;
}

static MilterStatus
cb_connect (MilterClientContext *context, const gchar *host_name,
            const struct sockaddr *address, socklen_t address_length,
            gpointer user_data)
{
    gchar *spec;

    spec = milter_connection_address_to_spec(address);
    g_print("connect: host=<%s>, address=<%s>\n", host_name, spec);
    g_free(spec);

    return MILTER_STATUS_CONTINUE;
}

static MilterStatus
cb_helo (MilterClientContext *context, const gchar *fqdn, gpointer user_data)
{
    g_print("helo: <%s>\n", fqdn);

    return MILTER_STATUS_CONTINUE;
}

static MilterStatus
cb_envelope_from (MilterClientContext *context, const gchar *from,
                  gpointer user_data)
{
    g_print("envelope-from: <%s>\n", from);

    return MILTER_STATUS_CONTINUE;
}

static MilterStatus
cb_envelope_recipient (MilterClientContext *context, const gchar *to,
                       gpointer user_data)
{
    g_print("envelope-recipient: <%s>\n", to);

    return MILTER_STATUS_CONTINUE;
}

static MilterStatus
cb_data (MilterClientContext *context, gpointer user_data)
{
    g_print("data\n");

    return MILTER_STATUS_CONTINUE;
}

static MilterStatus
cb_header (MilterClientContext *context, const gchar *name, const gchar *value,
           gpointer user_data)
{
    g_print("header: <%s: %s>\n", name, value);

    return MILTER_STATUS_CONTINUE;
}

static MilterStatus
cb_end_of_header (MilterClientContext *context, gpointer user_data)
{
    g_print("end-of-header\n");

    return MILTER_STATUS_CONTINUE;
}

static MilterStatus
cb_body (MilterClientContext *context, const gchar *chunk, gsize length,
         gpointer user_data)
{
    GString *null_terminated_chunk;

    null_terminated_chunk = g_string_new_len(chunk, length);
    g_print("body: <%s>\n", null_terminated_chunk->str);
    g_string_free(null_terminated_chunk, TRUE);

    return MILTER_STATUS_CONTINUE;
}

static MilterStatus
cb_end_of_message (MilterClientContext *context, gpointer user_data)
{
    g_print("end-of-message\n");

    return MILTER_STATUS_CONTINUE;
}

static MilterStatus
cb_abort (MilterClientContext *context, gpointer user_data)
{
    g_print("abort\n");

    return MILTER_STATUS_CONTINUE;
}

static MilterStatus
cb_quit (MilterClientContext *context, gpointer user_data)
{
    g_print("quit\n");

    return MILTER_STATUS_CONTINUE;
}

static MilterStatus
cb_unknown (MilterClientContext *context, const gchar *command,
            gpointer user_data)
{
    g_print("unknown: <%s>\n", command);

    return MILTER_STATUS_CONTINUE;
}

static void
setup_context_signals (MilterClientContext *context)
{
#define CONNECT(name)                                                   \
    g_signal_connect(context, #name, G_CALLBACK(cb_ ## name), NULL)

    CONNECT(negotiate);
    CONNECT(connect);
    CONNECT(helo);
    CONNECT(envelope_from);
    CONNECT(envelope_recipient);
    CONNECT(data);
    CONNECT(header);
    CONNECT(end_of_header);
    CONNECT(body);
    CONNECT(end_of_message);
    CONNECT(abort);
    CONNECT(quit);
    CONNECT(unknown);

#undef CONNECT
}

static void
cb_connection_established (MilterClient *client, MilterClientContext *context,
                           gpointer user_data)
{
    setup_context_signals(context);
}

static void
cb_error (MilterErrorEmittable *emittable, GError *error,
          gpointer user_data)
{
    g_print("ERROR: %s", error->message);
}

static void
setup_client_signals (MilterClient *client)
{
#define CONNECT(name)                                                   \
    g_signal_connect(client, #name, G_CALLBACK(cb_ ## name), NULL)

    CONNECT(connection_established);
    CONNECT(error);

#undef CONNECT
}

static void
shutdown_client (int signum)
{
    if (client)
        milter_client_shutdown(client);
}

int
main (int argc, char *argv[])
{
    gboolean success = TRUE;
    GError *error = NULL;
    GOptionContext *option_context;
    GOptionGroup *main_group;

    milter_init();

    option_context = g_option_context_new(NULL);
    g_option_context_add_main_entries(option_context, option_entries, NULL);
    main_group = g_option_context_get_main_group(option_context);

    if (!g_option_context_parse(option_context, &argc, &argv, &error)) {
        g_print("%s\n", error->message);
        g_error_free(error);
        g_option_context_free(option_context);
        exit(EXIT_FAILURE);
    }

    if (verbose)
        logger = milter_syslog_logger_new("milter-test-client");
    client = milter_client_new();
    success = milter_client_set_connection_spec(client, spec, &error);
    if (success) {
        void (*sigint_handler) (int signum);

        setup_client_signals(client);
        sigint_handler = signal(SIGINT, shutdown_client);
        success = milter_client_main(client);
        signal(SIGINT, sigint_handler);
    } else {
        g_print("%s\n", error->message);
        g_error_free(error);
    }
    g_object_unref(client);
    if (logger)
        g_object_unref(logger);

    milter_quit();

    g_option_context_free(option_context);

    exit(success ? EXIT_SUCCESS : EXIT_FAILURE);
}

/*
vi:nowrap:ai:expandtab:sw=4
*/
