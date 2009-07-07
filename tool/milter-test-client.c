/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Copyright (C) 2008-2009  Kouhei Sutou <kou@clear-code.com>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
    {"connection-spec", 's', 0, G_OPTION_ARG_CALLBACK, parse_spec_arg,
     N_("The spec of socket. (unix:PATH|inet:PORT[@HOST]|inet6:PORT[@HOST])"),
     "SPEC"},
    {"verbose", 'v', G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_NONE, &verbose,
     N_("Be verbose"), NULL},
    {"version", 0, G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK, print_version,
     N_("Show version"), NULL},
    {NULL}
};

static void
print_macro (gpointer _key, gpointer _value, gpointer user_data)
{
    const gchar *key = _key;
    const gchar *value = _value;

    g_print("  %s=%s\n", key, value);
}

static void
print_macros (MilterClientContext *context)
{
    GHashTable *macros;

    macros = milter_protocol_agent_get_macros(MILTER_PROTOCOL_AGENT(context));
    if (!macros)
        return;

    g_print("macros:\n");
    g_hash_table_foreach(macros, print_macro, NULL);
}

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

    print_macros(context);

    milter_option_remove_step(option,
                              MILTER_STEP_NO_CONNECT |
                              MILTER_STEP_NO_HELO |
                              MILTER_STEP_NO_ENVELOPE_FROM |
                              MILTER_STEP_NO_ENVELOPE_RECIPIENT |
                              MILTER_STEP_NO_BODY |
                              MILTER_STEP_NO_HEADERS |
                              MILTER_STEP_NO_END_OF_HEADER |
                              MILTER_STEP_NO_REPLY_HEADER |
                              MILTER_STEP_NO_UNKNOWN |
                              MILTER_STEP_NO_DATA |
                              MILTER_STEP_NO_REPLY_CONNECT |
                              MILTER_STEP_NO_REPLY_HELO |
                              MILTER_STEP_NO_REPLY_ENVELOPE_FROM |
                              MILTER_STEP_NO_REPLY_ENVELOPE_RECIPIENT |
                              MILTER_STEP_NO_REPLY_DATA |
                              MILTER_STEP_NO_REPLY_UNKNOWN |
                              MILTER_STEP_NO_REPLY_END_OF_HEADER |
                              MILTER_STEP_NO_REPLY_BODY);

    return MILTER_STATUS_CONTINUE;
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

    print_macros(context);

    return MILTER_STATUS_CONTINUE;
}

static MilterStatus
cb_helo (MilterClientContext *context, const gchar *fqdn, gpointer user_data)
{
    g_print("helo: <%s>\n", fqdn);

    print_macros(context);

    return MILTER_STATUS_CONTINUE;
}

static MilterStatus
cb_envelope_from (MilterClientContext *context, const gchar *from,
                  gpointer user_data)
{
    g_print("envelope-from: <%s>\n", from);

    print_macros(context);

    return MILTER_STATUS_CONTINUE;
}

static MilterStatus
cb_envelope_recipient (MilterClientContext *context, const gchar *to,
                       gpointer user_data)
{
    g_print("envelope-recipient: <%s>\n", to);

    print_macros(context);

    return MILTER_STATUS_CONTINUE;
}

static MilterStatus
cb_data (MilterClientContext *context, gpointer user_data)
{
    g_print("data\n");

    print_macros(context);

    return MILTER_STATUS_CONTINUE;
}

static MilterStatus
cb_header (MilterClientContext *context, const gchar *name, const gchar *value,
           gpointer user_data)
{
    g_print("header: <%s: %s>\n", name, value);

    print_macros(context);

    return MILTER_STATUS_CONTINUE;
}

static MilterStatus
cb_end_of_header (MilterClientContext *context, gpointer user_data)
{
    g_print("end-of-header\n");

    print_macros(context);

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

    print_macros(context);

    return MILTER_STATUS_CONTINUE;
}

static MilterStatus
cb_end_of_message (MilterClientContext *context,
                   const gchar *chunk, gsize size,
                   gpointer user_data)
{
    if (chunk && size > 0) {
        GString *null_terminated_chunk;

        null_terminated_chunk = g_string_new_len(chunk, size);
        g_print("end-of-message: <%s>\n", null_terminated_chunk->str);
        g_string_free(null_terminated_chunk, TRUE);
    } else {
        g_print("end-of-message\n");
    }

    print_macros(context);

    return MILTER_STATUS_CONTINUE;
}

static MilterStatus
cb_abort (MilterClientContext *context, MilterClientContextState state,
          gpointer user_data)
{
    g_print("abort\n");

    print_macros(context);

    return MILTER_STATUS_CONTINUE;
}

static MilterStatus
cb_unknown (MilterClientContext *context, const gchar *command,
            gpointer user_data)
{
    g_print("unknown: <%s>\n", command);

    print_macros(context);

    return MILTER_STATUS_CONTINUE;
}

static void
cb_finished (MilterFinishedEmittable *emittable)
{
    g_print("finished\n");
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
    CONNECT(unknown);

    CONNECT(finished);

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
cb_sigint_shutdown_client (int signum)
{
    if (client)
        milter_client_shutdown(client);

    signal(SIGINT, SIG_DFL);
}

int
main (int argc, char *argv[])
{
    gboolean success = TRUE;
    GError *error = NULL;
    GOptionContext *option_context;
    GOptionGroup *main_group;

    milter_init();
    milter_client_init();

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

    if (spec)
        success = milter_client_set_connection_spec(client, spec, &error);
    if (success) {
        void (*sigint_handler) (int signum);

        setup_client_signals(client);
        sigint_handler = signal(SIGINT, cb_sigint_shutdown_client);
        success = milter_client_main(client);
        signal(SIGINT, sigint_handler);
    } else {
        g_print("%s\n", error->message);
        g_error_free(error);
    }
    g_object_unref(client);
    if (logger)
        g_object_unref(logger);

    milter_client_quit();
    milter_quit();

    g_option_context_free(option_context);

    exit(success ? EXIT_SUCCESS : EXIT_FAILURE);
}

/*
vi:nowrap:ai:expandtab:sw=4
*/
