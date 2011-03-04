/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Copyright (C) 2008-2011  Kouhei Sutou <kou@clear-code.com>
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

#include <glib/gi18n.h>

#include "../client.h"
#include "milter-client-private.h"

static gboolean initialized = FALSE;
static guint milter_client_log_handler_id = 0;

void
milter_client_init (void)
{
    if (initialized)
        return;

    initialized = TRUE;

    milter_client_log_handler_id = MILTER_GLIB_LOG_DELEGATE("milter-client");
}

void
milter_client_quit (void)
{
    if (!initialized)
        return;

    g_log_remove_handler("milter-client", milter_client_log_handler_id);

    milter_client_log_handler_id = 0;
    initialized = FALSE;
}

static gboolean
parse_spec_arg (const gchar *option_name,
                const gchar *value,
                gpointer data,
                GError **error)
{
    MilterClient *client = data;
    GError *spec_error = NULL;
    gboolean success;

    success = milter_client_set_connection_spec(client, value, error);
    if (!success) {
        g_set_error(error,
                    G_OPTION_ERROR,
                    G_OPTION_ERROR_BAD_VALUE,
                    "%s",
                    spec_error->message);
        g_error_free(spec_error);
    }

    return success;
}

static gboolean
parse_verbose_arg (const gchar *option_name,
                   const gchar *value,
                   gpointer data,
                   GError **error)
{
    milter_logger_set_target_level(milter_logger(), MILTER_LOG_LEVEL_ALL);
    return TRUE;
}

static gboolean
parse_syslog_arg (const gchar *option_name,
                  const gchar *value,
                  gpointer data,
                  GError **error)
{
    MilterClient *client = data;

    milter_client_start_syslog(client,
                               g_get_prgname());
    return TRUE;
}

static const GOptionEntry option_entries[] =
{
    {"connection-spec", 's', 0, G_OPTION_ARG_CALLBACK, parse_spec_arg,
     N_("The spec of socket. (unix:PATH|inet:PORT[@HOST]|inet6:PORT[@HOST])"),
     "SPEC"},
    {"verbose", 'v', G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK,
     parse_verbose_arg,
     N_("Be verbose"), NULL},
    {"syslog", 0, G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK, parse_syslog_arg,
     N_("Use Syslog"), NULL},
    {NULL}
};

static gboolean
hook_pre_parse (GOptionContext *context, GOptionGroup *group,
                gpointer data, GError **error)
{
    return TRUE;
}

static gboolean
hook_post_parse (GOptionContext *context, GOptionGroup *group,
                 gpointer data, GError **error)
{
    return TRUE;
}

GOptionGroup *
milter_client_get_option_group (MilterClient *client)
{
    GOptionGroup *group;

    group = g_option_group_new("milter",
                               "Milter Common Options",
                               "Show milter help options",
                               g_object_ref(client),
                               (GDestroyNotify)g_object_unref);
    g_option_group_add_entries(group, option_entries);
    g_option_group_set_translation_domain(group, GETTEXT_PACKAGE);
    g_option_group_set_parse_hooks(group, hook_pre_parse, hook_post_parse);
    return group;
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
