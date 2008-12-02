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
#include "milter-manager-test-scenario.h"

void test_version (void);
void test_invalid_spec (void);

void data_scenario (void);
void test_scenario (gconstpointer data);

static gchar *scenario_dir;
static MilterManagerTestScenario *main_scenario;

typedef struct _EggData 
{
    GCutEgg *egg;
    gchar *command_path;
    GString *output_string;
    GString *error_string;
    gboolean reaped;
} EggData;

static EggData *manager_data;
static EggData *server_data;

static GError *error;

static gchar *
build_manager_path (void)
{
    return g_build_filename(milter_test_get_base_dir(),
                            "..",
                            "src",
                            "milter-manager",
                            NULL);
}

static gchar *
build_server_path (void)
{
    return g_build_filename(milter_test_get_base_dir(),
                            "..",
                            "tool",
                            "milter-test-server",
                            NULL);
}

typedef gchar *(*BuildPathFunc) (void);

static EggData *
egg_data_new (BuildPathFunc path_func)
{
    EggData *data;

    data = g_new0(EggData, 1);
    data->egg = NULL;
    data->command_path = path_func();
    data->output_string = g_string_new(NULL);
    data->error_string = g_string_new(NULL);
    data->reaped = FALSE;

    return data;
}

#define manager_egg manager_data->egg
#define server_egg server_data->egg

#define wait_for_server_reaping()   \
    wait_for_reaping(server_data)

#define wait_for_manager_reaping()  \
    wait_for_reaping(manager_data)

static void
wait_for_reaping (EggData *data)
{
    while (!data->reaped)
        g_main_context_iteration(NULL, TRUE);
}

static void
egg_data_free (EggData *data)
{
    if (data->egg) {
        if (!data->reaped && gcut_egg_get_pid(data->egg) > 0)
            gcut_egg_kill(data->egg, SIGINT);
        wait_for_manager_reaping();
        g_object_unref(data->egg);
    }

    if (data->command_path)
        g_free(data->command_path);
    if (data->output_string)
        g_string_free(data->output_string, TRUE);
    if (data->error_string)
        g_string_free(data->error_string, TRUE);
    g_free(data);
}

void
setup (void)
{
    scenario_dir = g_build_filename(milter_test_get_base_dir(),
                                    "fixtures",
                                    "manager",
                                    NULL);

    main_scenario = NULL;

    manager_data = egg_data_new(build_manager_path);
    server_data = egg_data_new(build_server_path);

    error = NULL;
}

void
teardown (void)
{
    if (scenario_dir)
        g_free(scenario_dir);
    if (main_scenario)
        g_object_unref(main_scenario);

    if (manager_data)
        egg_data_free(manager_data);
    if (server_data)
        egg_data_free(server_data);

    if (error)
        g_error_free(error);
}

static void
cb_output_received (GCutEgg *egg, const gchar *chunk, gsize size,
                    gpointer user_data)
{
    EggData *data = user_data;
    g_string_append_len(data->output_string, chunk, size);
}

static void
cb_error_received (GCutEgg *egg, const gchar *chunk, gsize size,
                   gpointer user_data)
{
    EggData *data = user_data;
    g_string_append_len(data->error_string, chunk, size);
}

static void
cb_reaped (GCutEgg *egg, gint status, gpointer user_data)
{
    EggData *data = user_data;
    data->reaped = TRUE;
}

static void
setup_egg (EggData *data, const gchar *first_arg, ...)
{
    va_list var_args;
    const gchar *arg;
    gint argc = 1, i;
    gchar **argv;
    GList *strings = NULL, *node;

    va_start(var_args, first_arg);
    arg = first_arg;
    while (arg) {
        argc++;
        strings = g_list_append(strings, g_strdup(arg));
        arg = va_arg(var_args, gchar *);
    }
    va_end(var_args);

    argv = g_new0(gchar*, argc + 1);
    argv[0] = g_strdup(data->command_path);
    argv[argc] = NULL;
    for (node = strings, i = 1; node; node = g_list_next(node), i++) {
        argv[i] = node->data;
    }
    if (strings)
        g_list_free(strings);

    data->egg = gcut_egg_new_argv(argc, argv);
    g_strfreev(argv);

#define CONNECT(name)                                                   \
    g_signal_connect(data->egg, #name, G_CALLBACK(cb_ ## name), data)

    CONNECT(output_received);
    CONNECT(error_received);
    CONNECT(reaped);
#undef CONNECT
}

static gboolean
cb_timeout_waiting (gpointer data)
{
    gboolean *waiting = data;

    *waiting = FALSE;
    return FALSE;
}

static void
wait_for_manager_ready (const gchar *spec)
{
    gboolean timeout_waiting = TRUE;
    guint timeout_waiting_id;
    gint sock_fd;
    gint domain;
    struct sockaddr *address;
    socklen_t address_size;

    if (!milter_connection_parse_spec(spec,
                                      &domain,
                                      &address,
                                      &address_size,
                                      NULL)) {
        cut_error("Error");
    }

    sock_fd = socket(domain, SOCK_STREAM, 0);

    timeout_waiting_id = g_timeout_add(500, cb_timeout_waiting,
                                       &timeout_waiting);
    while (timeout_waiting &&
           connect(sock_fd, address, address_size) != 0) {
        g_main_context_iteration(NULL, FALSE);
    }

    g_source_remove(timeout_waiting_id);
    close(sock_fd);
    cut_assert_true(timeout_waiting, "Error");
}

static void
start_manager (void)
{
    const gchar spec[] = "inet:19999@localhost";
    setup_egg(manager_data, "-s", spec, NULL);
    gcut_egg_set_env(manager_egg,
                     "MILTER_MANAGER_CONFIG_DIR", scenario_dir,
                     NULL);
    gcut_egg_hatch(manager_egg, &error);
    wait_for_manager_ready(spec);
}

static void
start_server (void)
{
    const gchar spec[] = "inet:19999@localhost";
    setup_egg(server_data, "-s", spec, NULL);

    gcut_egg_hatch(server_egg, &error);
}

void
test_version (void)
{
    setup_egg(manager_data, "--version", NULL);
    gcut_egg_hatch(manager_egg, &error);
    gcut_assert_error(error);

    wait_for_manager_reaping();

    cut_assert_equal_string("milter-manager " VERSION "\n",
                            manager_data->output_string->str);
}

void
test_invalid_spec (void)
{
    setup_egg(manager_data, "-s", "XXXX@localhost", NULL);
    gcut_egg_hatch(manager_egg, &error);
    gcut_assert_error(error);

    wait_for_manager_reaping();

    cut_assert_equal_string("spec doesn't have colon: <XXXX@localhost>\n",
                            manager_data->output_string->str);
}

#define has_key(scenario, group, key)                                     \
    milter_manager_test_scenario_has_key(scenario, group, key)

#define get_integer(scenario, group, key)                               \
    milter_manager_test_scenario_get_integer(scenario, group, key)

#define get_boolean(scenario, group, key)                               \
    milter_manager_test_scenario_get_boolean(scenario, group, key)

#define get_string(scenario, group, key)                                \
    milter_manager_test_scenario_get_string(scenario, group, key)

#define get_string_with_sub_key(scenario, group, key, sub_key)          \
    milter_manager_test_scenario_get_string_with_sub_key(scenario, group, key, sub_key)

#define get_string_list(scenario, group, key, length)                   \
    milter_manager_test_scenario_get_string_list(scenario, group, key, length)

#define get_string_g_list(scenario, group, key)                         \
    milter_manager_test_scenario_get_string_g_list(scenario, group, key)

#define get_enum(scenario, group, key, enum_type)                       \
    milter_manager_test_scenario_get_enum(scenario, group, key, enum_type)

#define get_option_list(scenario, group, key)                           \
    milter_manager_test_scenario_get_option_list(scenario, group, key)

#define get_header_list(scenario, group, key)                           \
    milter_manager_test_scenario_get_header_list(scenario, group, key)

#define get_pair_list(scenario, group, key)                             \
    milter_manager_test_scenario_get_pair_list(scenario, group, key)

#define get_pair_list_without_sort(scenario, group, key)                         \
    milter_manager_test_scenario_get_pair_list_without_sort(scenario, group, key)

static void
do_actions (MilterManagerTestScenario *scenario)
{
    const gchar *expected;
    gchar **lines;

    wait_for_server_reaping();
    expected = get_string(scenario, "scenario", "expected");
    lines = g_strsplit(server_data->output_string->str, "\n", 2);
    cut_take_string_array(lines);
    cut_assert_equal_string(expected, lines[0]);
    cut_assert_match("^Finished in [\\d.]+sec.\\s*$", lines[1]);
}

void
data_scenario (void)
{
    cut_add_data("normal",
                 g_strdup("normal.txt"), g_free);
    cut_add_data("reject on envelope-from",
                 g_strdup("reject-on-envelope-from.txt"), g_free);
    cut_add_data("reject on envelope-recipient",
                 g_strdup("reject-on-envelope-recipient.txt"), g_free);
    cut_add_data("skip on body",
                 g_strdup("skip-on-body.txt"), g_free);
}

void
test_scenario (gconstpointer data)
{
    const gchar *scenario_name = data;
    const gchar omit_key[] = "omit";

    start_manager();

    main_scenario = milter_manager_test_scenario_new();
    cut_trace(milter_manager_test_scenario_load(main_scenario,
                                                scenario_dir,
                                                scenario_name));

    if (has_key(main_scenario,
                MILTER_MANAGER_TEST_SCENARIO_GROUP_NAME, omit_key))
        cut_omit("%s", get_string(main_scenario,
                                  MILTER_MANAGER_TEST_SCENARIO_GROUP_NAME,
                                  omit_key));

    cut_trace(milter_manager_test_scenario_start_clients(main_scenario));

    start_server();

    cut_trace(do_actions(main_scenario));
    wait_for_server_reaping();
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
