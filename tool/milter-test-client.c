/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Copyright (C) 2008-2011  Kouhei Sutou <kou@clear-code.com>
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
#include <sys/types.h>
#include <unistd.h>
#include <glib/gi18n.h>

#ifdef HAVE_LOCALE_H
#  include <locale.h>
#endif

#include <milter/client.h>

static gboolean report_request = TRUE;
static gboolean report_memory_profile = FALSE;
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

static const GOptionEntry option_entries[] =
{
    {"no-report-request", 0, G_OPTION_FLAG_REVERSE,
     G_OPTION_ARG_NONE, &report_request,
     N_("Don't report request values"), NULL},
    {"report-memory-profile", 0, 0, G_OPTION_ARG_NONE, &report_memory_profile,
     N_("Report memory profile. "
        "Need to set MILTER_MEMORY_PROFILE=yes environment variable."), NULL},
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
    if (report_request) {
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
    }

    milter_option_remove_step(option,
                              MILTER_STEP_ENVELOPE_RECIPIENT_REJECTED |
                              MILTER_STEP_HEADER_VALUE_WITH_LEADING_SPACE |
                              MILTER_STEP_NO_MASK);

    return MILTER_STATUS_CONTINUE;
}

static MilterStatus
cb_connect (MilterClientContext *context, const gchar *host_name,
            const struct sockaddr *address, socklen_t address_length,
            gpointer user_data)
{
    if (report_request) {
        gchar *spec;

        spec = milter_connection_address_to_spec(address);
        g_print("connect: host=<%s>, address=<%s>\n", host_name, spec);
        g_free(spec);

        print_macros(context);
    }

    return MILTER_STATUS_CONTINUE;
}

static MilterStatus
cb_helo (MilterClientContext *context, const gchar *fqdn, gpointer user_data)
{
    if (report_request) {
        g_print("helo: <%s>\n", fqdn);

        print_macros(context);
    }

    return MILTER_STATUS_CONTINUE;
}

static MilterStatus
cb_envelope_from (MilterClientContext *context, const gchar *from,
                  gpointer user_data)
{
    if (report_request) {
        g_print("envelope-from: <%s>\n", from);

        print_macros(context);
    }

    return MILTER_STATUS_CONTINUE;
}

static MilterStatus
cb_envelope_recipient (MilterClientContext *context, const gchar *to,
                       gpointer user_data)
{
    if (report_request) {
        g_print("envelope-recipient: <%s>\n", to);

        print_macros(context);
    }

    return MILTER_STATUS_CONTINUE;
}

static MilterStatus
cb_data (MilterClientContext *context, gpointer user_data)
{
    if (report_request) {
        g_print("data\n");

        print_macros(context);
    }

    return MILTER_STATUS_CONTINUE;
}

static MilterStatus
cb_header (MilterClientContext *context, const gchar *name, const gchar *value,
           gpointer user_data)
{
    if (report_request) {
        g_print("header: <%s: %s>\n", name, value);

        print_macros(context);
    }

    return MILTER_STATUS_CONTINUE;
}

static MilterStatus
cb_end_of_header (MilterClientContext *context, gpointer user_data)
{
    if (report_request) {
        g_print("end-of-header\n");

        print_macros(context);
    }

    return MILTER_STATUS_CONTINUE;
}

static MilterStatus
cb_body (MilterClientContext *context, const gchar *chunk, gsize length,
         gpointer user_data)
{
    if (report_request) {
        g_print("body: <%.*s>\n", (gint)length, chunk);

        print_macros(context);
    }

    return MILTER_STATUS_CONTINUE;
}

static MilterStatus
cb_end_of_message (MilterClientContext *context,
                   const gchar *chunk, gsize length,
                   gpointer user_data)
{
    if (report_request) {
        if (length > 0) {
            g_print("end-of-message: <%.*s>\n", (gint)length, chunk);
        } else {
            g_print("end-of-message\n");
        }

        print_macros(context);
    }

    return MILTER_STATUS_CONTINUE;
}

static MilterStatus
cb_abort (MilterClientContext *context, MilterClientContextState state,
          gpointer user_data)
{
    if (report_request) {
        g_print("abort\n");

        print_macros(context);
    }

    return MILTER_STATUS_CONTINUE;
}

static MilterStatus
cb_unknown (MilterClientContext *context, const gchar *command,
            gpointer user_data)
{
    if (report_request) {
        g_print("unknown: <%s>\n", command);

        print_macros(context);
    }

    return MILTER_STATUS_CONTINUE;
}

static void
cb_finished (MilterFinishedEmittable *emittable)
{
    if (report_request) {
        g_print("finished\n");
    }
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

static guint64
process_memory_usage_in_kb (void)
{
    guint64 usage = 0;
    gchar *command_line, *result;

    command_line = g_strdup_printf("ps -o rss= -p %u", getpid());
    if (g_spawn_command_line_sync(command_line, &result, NULL, NULL, NULL)) {
        usage = g_ascii_strtoull(result, NULL, 10);
        g_free(result);
    }
    g_free(command_line);

    return usage;
}

static void
cb_maintain (MilterClient *client, gpointer user_data)
{
    gboolean success;
    guint64 memory_usage_in_kb;
    gsize n_allocates, n_zero_initializes, n_frees;
    GString *report;

    report = g_string_new("[maintain][memory] ");
    memory_usage_in_kb = process_memory_usage_in_kb();
    g_string_append_printf(report,
                           "process: (%" G_GUINT64_FORMAT "KB) ",
                           memory_usage_in_kb);

    success = milter_memory_profile_get_data(&n_allocates,
                                             &n_zero_initializes,
                                             &n_frees);
    if (success) {
        guint64 n_used_in_kb;

        n_used_in_kb = (n_allocates - n_frees) / 1024;
        g_string_append_printf(report,
                               "GLib: (%" G_GUINT64_FORMAT "KB)",
                               n_used_in_kb);
        if (memory_usage_in_kb == 0) {
            g_string_append(report, "(NaN) ");
        } else {
            g_string_append_printf(report,
                                   "(%.2f%%) ",
                                   (double)n_used_in_kb / memory_usage_in_kb);
        }
        g_string_append_printf(report,
                               "allocated:%" G_GSIZE_FORMAT " ",
                               n_allocates);
        g_string_append_printf(report,
                               "freed:%" G_GSIZE_FORMAT "(%.2f%%)",
                               n_frees,
                               (double)n_frees / n_allocates * 100);
    }

    milter_statistics("%s", report->str);
    g_string_free(report, TRUE);
}

static void
setup_client_signals (MilterClient *client)
{
#define CONNECT(name)                                                   \
    g_signal_connect(client, #name, G_CALLBACK(cb_ ## name), NULL)

    CONNECT(connection_established);
    CONNECT(error);

    if (report_memory_profile)
        CONNECT(maintain);

#undef CONNECT
}

static void
cb_signal_shutdown_client (int signum)
{
    if (client)
        milter_client_shutdown(client);

    signal(signum, SIG_DFL);
}

int
main (int argc, char *argv[])
{
    gboolean success = TRUE;
    GError *error = NULL;
    GOptionContext *option_context;
    GOptionGroup *milter_group;

#ifdef HAVE_LOCALE_H
    setlocale(LC_ALL, "");
#endif

    milter_init();
    milter_client_init();

    option_context = g_option_context_new(NULL);
    g_option_context_add_main_entries(option_context, option_entries, NULL);

    client = milter_client_new();
    milter_group = milter_client_get_option_group(client);
    g_option_context_add_group(option_context, milter_group);

    if (!g_option_context_parse(option_context, &argc, &argv, &error)) {
        g_print("%s\n", error->message);
        g_error_free(error);
        g_option_context_free(option_context);
        g_object_unref(client);
        exit(EXIT_FAILURE);
    }

    if (success)
        success = milter_client_listen(client, &error);
    if (success)
        success = milter_client_drop_privilege(client, &error);
    if (success) {
        void (*sigint_handler) (int signum);
        void (*sigterm_handler) (int signum);

        setup_client_signals(client);
        sigint_handler = signal(SIGINT, cb_signal_shutdown_client);
        sigterm_handler = signal(SIGTERM, cb_signal_shutdown_client);
        success = milter_client_run(client, &error);
        if (!success) {
            g_print("%s\n", error->message);
            g_error_free(error);
        }
        signal(SIGTERM, sigterm_handler);
        signal(SIGINT, sigint_handler);
    } else {
        g_print("%s\n", error->message);
        g_error_free(error);
    }
    g_object_unref(client);

    milter_client_quit();
    milter_quit();

    g_option_context_free(option_context);

    exit(success ? EXIT_SUCCESS : EXIT_FAILURE);
}

/*
vi:nowrap:ai:expandtab:sw=4
*/
