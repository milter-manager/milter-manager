/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Copyright (C) 2010-2011  Kouhei Sutou <kou@clear-code.com>
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
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/types.h>
#include <unistd.h>

#include <glib.h>
#include <glib/gi18n.h>

#ifdef HAVE_LOCALE_H
#  include <locale.h>
#endif

#include <libmilter/mfapi.h>

static gchar *spec = NULL;
static gboolean verbose = FALSE;
static gboolean report_request = TRUE;
static gboolean report_memory_profile = FALSE;

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
    {"connection-spec", 's', 0, G_OPTION_ARG_STRING, &spec,
     N_("The spec of socket. (unix:PATH|inet:PORT[@HOST]|inet6:PORT[@HOST])"),
     "SPEC"},
    {"verbose", 'v', 0, G_OPTION_ARG_NONE, &verbose,
     N_("Be verbose"), NULL},
    {"no-report-request", 0, G_OPTION_FLAG_REVERSE, G_OPTION_ARG_NONE,
     &report_request,
     N_("Don't report request values"), NULL},
    {"report-memory-profile", 0, 0, G_OPTION_ARG_NONE, &report_memory_profile,
     N_("Report memory profile. "
        "Need to set MILTER_MEMORY_PROFILE=yes environment variable."), NULL},
    {"version", 0, G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK, print_version,
     N_("Show version"), NULL},
    {NULL}
};

static gchar *
address_to_spec (_SOCK_ADDR *address)
{
    gchar *spec = NULL;

    if (!address)
        return NULL;

    switch (address->sa_family) {
    case AF_UNIX:
    {
        struct sockaddr_un *address_unix = (struct sockaddr_un *)address;
        spec = g_strdup_printf("unix:%s", address_unix->sun_path);
        break;
    }
    case AF_INET:
    {
        struct sockaddr_in *address_inet = (struct sockaddr_in *)address;
        gchar ip_address_string[INET_ADDRSTRLEN];

        if (inet_ntop(AF_INET, &address_inet->sin_addr,
                      (gchar *)ip_address_string, INET_ADDRSTRLEN)) {
            spec = g_strdup_printf("inet:%d@%s",
                                   g_ntohs(address_inet->sin_port),
                                   ip_address_string);
        }
        break;
    }
    case AF_INET6:
    {
        struct sockaddr_in6 *address_inet6 = (struct sockaddr_in6 *)address;
        gchar ip_address_string[INET6_ADDRSTRLEN];

        if (inet_ntop(AF_INET6, &address_inet6->sin6_addr,
                      (gchar *)ip_address_string, INET6_ADDRSTRLEN)) {
            spec = g_strdup_printf("inet6:%d@%s",
                                   g_ntohs(address_inet6->sin6_port),
                                   ip_address_string);
        }
        break;
    }
    case AF_UNSPEC:
        spec = g_strdup("unknown");
        break;
    default:
        spec = g_strdup_printf("unexpected:%d", address->sa_family);
        break;
    }

    return spec;
}

static sfsistat
mlfi_connect (SMFICTX *context, char *host_name, _SOCK_ADDR  *address)
{
    if (report_request) {
        gchar *spec;

        spec = address_to_spec(address);
        g_print("connect: host=<%s>, address=<%s>\n", host_name, spec);
        g_free(spec);
    }

    return SMFIS_CONTINUE;
}

static sfsistat
mlfi_helo (SMFICTX *context, char *fqdn)
{
    if (report_request) {
        g_print("helo: <%s>\n", fqdn);
    }

    return SMFIS_NOREPLY;
}

static sfsistat
mlfi_envfrom (SMFICTX *context, char **arguments)
{
    if (report_request) {
        gint i;
        GString *envelope_from;

        envelope_from = g_string_new(NULL);
        for (i = 0; arguments[i]; i++) {
            g_string_append_printf(envelope_from, " <%s>", arguments[i]);
        }
        g_print("envelope-from:%s\n", envelope_from->str);
        g_string_free(envelope_from, TRUE);
    }

    return SMFIS_NOREPLY;
}

static sfsistat
mlfi_envrcpt (SMFICTX *context, char **arguments)
{
    if (report_request) {
        gint i;
        GString *envelope_recipient;

        envelope_recipient = g_string_new(NULL);
        for (i = 0; arguments[i]; i++) {
            g_string_append_printf(envelope_recipient, " <%s>", arguments[i]);
        }
        g_print("envelope-recipient:%s\n", envelope_recipient->str);
        g_string_free(envelope_recipient, TRUE);
    }

    return SMFIS_CONTINUE;
}

static sfsistat
mlfi_header (SMFICTX *context, char *name, char *value)
{
    if (report_request) {
        g_print("header: <%s: %s>\n", name, value);
    }

    return SMFIS_CONTINUE;
}

static sfsistat
mlfi_eoh (SMFICTX *context)
{
    if (report_request) {
        g_print("end-of-header\n");
    }

    return SMFIS_CONTINUE;
}

static sfsistat
mlfi_body (SMFICTX *context, unsigned char *data, size_t data_size)
{
    if (report_request) {
        g_print("body: <%.*s>\n", (gint)data_size, data);
    }

    return SMFIS_NOREPLY;
}


static sfsistat
mlfi_eom (SMFICTX *context)
{
    if (report_request) {
        g_print("end-of-message\n");
    }

    return SMFIS_CONTINUE;
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
report_memory_usage (void)
{
    guint64 memory_usage_in_kb;

    memory_usage_in_kb = process_memory_usage_in_kb();
    g_print("process: (%" G_GUINT64_FORMAT "KB) ", memory_usage_in_kb);
}

static sfsistat
mlfi_close (SMFICTX *context)
{
    if (report_request) {
        g_print("close\n");
    }

    if (report_memory_profile) {
        report_memory_usage();
    }

    return SMFIS_ACCEPT;
}

static sfsistat
mlfi_abort (SMFICTX *context)
{
    if (report_request) {
        g_print("abort\n");
    }

    return SMFIS_CONTINUE;
}

static sfsistat
mlfi_unknown (SMFICTX *context, const char *command)
{
    if (report_request) {
        g_print("unknown: <%s>\n", command);
    }

    return SMFIS_CONTINUE;
}

static sfsistat
mlfi_data (SMFICTX *context)
{
    if (report_request) {
        g_print("data\n");
    }

    return SMFIS_CONTINUE;
}

static sfsistat
mlfi_negotiate (SMFICTX *context,
                unsigned long  actions,
                unsigned long  steps,
                unsigned long  unused0,
                unsigned long  unused1,
                unsigned long *actions_output,
                unsigned long *steps_output,
                unsigned long *unused0_output,
                unsigned long *unused1_output)
{
    if (report_request) {
        g_print("negotiate: actions=<%lu>, step=<%lu>\n",
                actions, steps);
    }

    *actions_output =
        actions &
        (SMFIF_ADDHDRS |
         SMFIF_CHGBODY |
         SMFIF_ADDRCPT |
         SMFIF_DELRCPT |
         SMFIF_CHGHDRS |
         SMFIF_QUARANTINE |
         SMFIF_CHGFROM |
         SMFIF_ADDRCPT_PAR |
         SMFIF_SETSYMLIST);

    *steps_output = steps & (SMFIP_NR_HELO | SMFIP_NR_MAIL | SMFIP_NR_BODY);

    return SMFIS_CONTINUE;
}

struct smfiDesc smfilter =
{
    "milter-test-client-libmilter",
    SMFI_VERSION,
    SMFIF_ADDHDRS,
    mlfi_connect,
    mlfi_helo,
    mlfi_envfrom,
    mlfi_envrcpt,
    mlfi_header,
    mlfi_eoh,
    mlfi_body,
    mlfi_eom,
    mlfi_abort,
    mlfi_close,
    mlfi_unknown,
    mlfi_data,
    mlfi_negotiate
};

int
main (int argc, char *argv[])
{
    gint status = MI_SUCCESS;
    GError *error = NULL;
    GOptionContext *option_context;
    unsigned int major, minor, patch_level;

#ifdef HAVE_LOCALE_H
    setlocale(LC_ALL, "");
#endif

    /*
     * workaround for memory profiler for GLib memory
     * profiler, we need to call g_mem_set_vtable prior to
     * any other GLib functions. smfi_version() calls
     * g_mem_set_vtable() internally.
     */
    smfi_version(&major, &minor, &patch_level);

    option_context = g_option_context_new(NULL);
    g_option_context_add_main_entries(option_context, option_entries, NULL);

    if (!g_option_context_parse(option_context, &argc, &argv, &error)) {
        g_print("%s\n", error->message);
        g_error_free(error);
        g_option_context_free(option_context);
        exit(EXIT_FAILURE);
    }

    if (verbose)
        smfi_setdbg(6);

    status = smfi_setconn(spec);
    if (status == MI_SUCCESS)
        status = smfi_register(smfilter);
    if (status == MI_SUCCESS)
        return smfi_main() == MI_SUCCESS ? EXIT_SUCCESS : EXIT_FAILURE;

    return EXIT_FAILURE;
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
