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

#include <errno.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <signal.h>

#include <gcutter.h>
#include "milter-test-utils.h"
#include "milter-manager-test-utils.h"

void test_run (void);

static GCutEgg *manager;
static GArray *command;
static gchar *manager_path;
static GString *output_string;
static GString *error_string;
static gboolean reaped;
static GError *error;

static void
setup_manager_command (void)
{
    command = g_array_new(TRUE, TRUE, sizeof(gchar *));
    manager_path = g_build_filename(milter_test_get_base_dir(),
                                   "..",
                                   "src",
                                   "milter-manager",
                                   NULL);
    g_array_append_val(command, manager_path);
}

void
setup (void)
{
    manager = NULL;

    setup_manager_command();
    output_string = g_string_new(NULL);
    error_string = g_string_new(NULL);
    reaped = FALSE;

    error = NULL;
}

void
teardown (void)
{
    if (manager) {
        gcut_egg_kill(manager, SIGKILL);
        g_object_unref(manager);
    }
    if (command)
        g_array_free(command, TRUE);
    if (output_string)
        g_string_free(output_string, TRUE);
    if (error_string)
        g_string_free(error_string, TRUE);
    if (error)
        g_error_free(error);
}

static void
append_argument (GArray *array, const gchar *argument)
{
    gchar *dupped_argument;

    dupped_argument = g_strdup(argument);
    g_array_append_val(array, dupped_argument);
}

static void
cb_output_received (GCutEgg *egg, const gchar *chunk, gsize size,
                    gpointer user_data)
{
    g_string_append_len(output_string, chunk, size);
}

static void
cb_error_received (GCutEgg *egg, const gchar *chunk, gsize size,
                   gpointer user_data)
{
    g_string_append_len(error_string, chunk, size);
}

static void
cb_reaped (GCutEgg *egg, gint status, gpointer user_data)
{
    reaped = TRUE;
}

static void
setup_manager (const gchar *spec)
{
    append_argument(command, "-s");
    g_array_append_val(command, spec);

    manager = gcut_egg_new_array(command);

#define CONNECT(name)                                                   \
    g_signal_connect(manager, #name, G_CALLBACK(cb_ ## name), NULL)

    CONNECT(output_received);
    CONNECT(error_received);
    CONNECT(reaped);
#undef CONNECT
}

static void
wait_for_reaping (void)
{
    while (!reaped)
        g_main_context_iteration(NULL, TRUE);
}

void
test_run (void)
{
    setup_manager("inet:19999");
    gcut_egg_hatch(manager, &error);
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
