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
#include <glib/gi18n.h>

#include <milter/client.h>

static gchar *spec = NULL;
static gboolean verbose = FALSE;

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

int
main (int argc, char *argv[])
{
    gboolean success = TRUE;
    MilterClient *client;
    GError *error = NULL;
    GOptionContext *option_context;
    GOptionGroup *main_group;

    option_context = g_option_context_new(NULL);
    g_option_context_add_main_entries(option_context, option_entries, NULL);
    main_group = g_option_context_get_main_group(option_context);

    if (!g_option_context_parse(option_context, &argc, &argv, &error)) {
        g_print("%s\n", error->message);
        g_error_free(error);
        g_option_context_free(option_context);
        exit(EXIT_FAILURE);
    }

    milter_init();

    client = milter_client_new();
    success = milter_client_set_connection_spec(client, spec, &error);
    if (success) {
        success = milter_client_main(client);
    } else {
        g_print("%s\n", error->message);
        g_error_free(error);
    }
    g_object_unref(client);

    milter_quit();

    g_option_context_free(option_context);

    exit(success ? EXIT_SUCCESS : EXIT_FAILURE);
}

/*
vi:nowrap:ai:expandtab:sw=4
*/
