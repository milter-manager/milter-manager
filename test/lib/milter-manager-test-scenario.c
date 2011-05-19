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

#include <string.h>

#include <gcutter.h>

#include "milter-manager-test-utils.h"
#include "milter-manager-test-client.h"
#include "milter-manager-test-scenario.h"

#include <milter/manager/milter-manager-egg.h>

#define MILTER_MANAGER_TEST_SCENARIO_GET_PRIVATE(obj)                   \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj),                                 \
                                 MILTER_TYPE_MANAGER_TEST_SCENARIO,     \
                                 MilterManagerTestScenarioPrivate))

typedef struct _MilterManagerTestScenarioPrivate MilterManagerTestScenarioPrivate;
struct _MilterManagerTestScenarioPrivate
{
    GKeyFile *key_file;
    GList *imported_scenarios;
    GList *started_clients;
    GList *eggs;
};

G_DEFINE_TYPE(MilterManagerTestScenario,
              milter_manager_test_scenario,
              G_TYPE_OBJECT)

static void dispose        (GObject         *object);

static void
milter_manager_test_scenario_class_init (MilterManagerTestScenarioClass *klass)
{
    GObjectClass *gobject_class;

    gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->dispose      = dispose;

    g_type_class_add_private(gobject_class,
                             sizeof(MilterManagerTestScenarioPrivate));
}

static void
milter_manager_test_scenario_init (MilterManagerTestScenario *scenario)
{
    MilterManagerTestScenarioPrivate *priv;

    priv = MILTER_MANAGER_TEST_SCENARIO_GET_PRIVATE(scenario);

    priv->key_file = NULL;
    priv->imported_scenarios = NULL;
    priv->started_clients = NULL;
    priv->eggs = NULL;
}

static void
dispose (GObject *object)
{
    MilterManagerTestScenarioPrivate *priv;

    priv = MILTER_MANAGER_TEST_SCENARIO_GET_PRIVATE(object);

    if (priv->key_file) {
        g_key_file_free(priv->key_file);
        priv->key_file = NULL;
    }

    if (priv->imported_scenarios) {
        g_list_foreach(priv->imported_scenarios, (GFunc)g_object_unref, NULL);
        g_list_free(priv->imported_scenarios);
        priv->imported_scenarios = NULL;
    }

    if (priv->started_clients) {
        g_list_foreach(priv->started_clients, (GFunc)g_object_unref, NULL);
        g_list_free(priv->started_clients);
        priv->started_clients = NULL;
    }

    if (priv->eggs) {
        g_list_foreach(priv->eggs, (GFunc)g_object_unref, NULL);
        priv->eggs = NULL;
    }

    G_OBJECT_CLASS(milter_manager_test_scenario_parent_class)->dispose(object);
}

static void load_scenario (MilterManagerTestScenario *scenario,
                           const gchar *base_dir,
                           const gchar *scenario_name);

static void
import_scenario (MilterManagerTestScenario *scenario,
                 const gchar *base_dir, const gchar *scenario_name)
{
    MilterManagerTestScenarioPrivate *priv;
    MilterManagerTestScenario *imported_scenario = NULL;

    priv = MILTER_MANAGER_TEST_SCENARIO_GET_PRIVATE(scenario);
    imported_scenario = milter_manager_test_scenario_new();
    priv->imported_scenarios =
        g_list_append(priv->imported_scenarios, imported_scenario);
    cut_trace(milter_manager_test_scenario_load(imported_scenario,
                                                base_dir, scenario_name));
}

static void
import_scenarios (MilterManagerTestScenario *scenario, const gchar *base_dir)
{
    const gchar key[] = "import";
    const gchar **scenario_names;
    gsize length, i;

    if (!milter_manager_test_scenario_has_key(
            scenario, MILTER_MANAGER_TEST_SCENARIO_GROUP_NAME, key))
        return;

    scenario_names =
        milter_manager_test_scenario_get_string_list(
            scenario, MILTER_MANAGER_TEST_SCENARIO_GROUP_NAME, key, &length);
    for (i = 0; i < length; i++) {
        cut_trace(import_scenario(scenario, base_dir, scenario_names[i]));
    }
}

static void
load_scenario (MilterManagerTestScenario *scenario,
               const gchar *base_dir, const gchar *scenario_name)
{
    MilterManagerTestScenarioPrivate *priv;
    GError *error = NULL;
    gchar *scenario_path;

    priv = MILTER_MANAGER_TEST_SCENARIO_GET_PRIVATE(scenario);
    priv->key_file = g_key_file_new();
    scenario_path = g_build_filename(base_dir, scenario_name, NULL);
    if (!g_key_file_load_from_file(priv->key_file, scenario_path,
                                   G_KEY_FILE_KEEP_COMMENTS |
                                   G_KEY_FILE_KEEP_TRANSLATIONS,
                                   &error)) {
        g_key_file_free(priv->key_file);
        priv->key_file = NULL;
    }
    gcut_assert_error(error,
                      cut_message("<%s>", cut_take_string(scenario_path)));

    cut_trace(import_scenarios(scenario, base_dir));
}

MilterManagerTestScenario *
milter_manager_test_scenario_new (void)
{
    return g_object_new(MILTER_TYPE_MANAGER_TEST_SCENARIO, NULL);
}

void
milter_manager_test_scenario_load (MilterManagerTestScenario *scenario,
                                   const gchar *base_dir,
                                   const gchar *scenario_name)
{
    cut_trace(load_scenario(scenario, base_dir, scenario_name));
}

const GList *
milter_manager_test_scenario_get_imported_scenarios (MilterManagerTestScenario *scenario)
{
    return MILTER_MANAGER_TEST_SCENARIO_GET_PRIVATE(scenario)->imported_scenarios;
}

static void
add_egg (MilterManagerTestScenarioPrivate *priv,
         MilterManagerTestClient *client, const gchar *name, guint port)
{
    MilterManagerEgg *egg;
    GError *error = NULL;
    gchar *connection_spec;
    const GArray *command_line;
    gchar *command;

    egg = milter_manager_egg_new(name);
    priv->eggs = g_list_append(priv->eggs, egg);
    connection_spec = g_strdup_printf("inet:%u@localhost", port);
    milter_manager_egg_set_connection_spec(egg, connection_spec, &error);
    g_free(connection_spec);
    gcut_assert_error(error);

    command_line = milter_manager_test_client_get_command(client);
    command = g_array_index(command_line, gchar *, 0);
    if (command) {
        gchar *command_options;

        milter_manager_egg_set_command(egg, command);
        command_options = g_strjoinv(" ", ((gchar **)(command_line->data)) + 1);
        milter_manager_egg_set_command_options(egg, command_options);
        g_free(command_options);
    }
    milter_manager_egg_set_user_name(egg, g_get_user_name());
}


static void
start_client (MilterManagerTestScenario *scenario, const gchar *group,
              MilterEventLoop *loop)
{
    MilterManagerTestScenarioPrivate *priv;
    MilterManagerTestClient *client;
    GError *error = NULL;
    guint port;
    gsize length;
    const gchar **arguments = NULL;

    priv = MILTER_MANAGER_TEST_SCENARIO_GET_PRIVATE(scenario);
    port = milter_manager_test_scenario_get_integer(scenario, group, "port");
    if (milter_manager_test_scenario_has_key(scenario, group, "arguments")) {
        arguments =
            milter_manager_test_scenario_get_string_list(
                scenario, group, "arguments", &length);
    }

    client = milter_manager_test_client_new(port, group, loop);
    milter_manager_test_client_set_argument_strings(client, arguments);
    priv->started_clients = g_list_append(priv->started_clients, client);

    add_egg(priv, client, group, port);

    if (milter_manager_test_scenario_has_key(scenario, group, "run") &&
        !milter_manager_test_scenario_get_boolean(scenario, group, "run"))
        return;

    if (!milter_manager_test_client_run(client, &error)) {
        gcut_assert_error(error);
        cut_fail("couldn't run test client on port <%u>", port);
    }
}

void
milter_manager_test_scenario_start_clients (MilterManagerTestScenario *scenario,
                                            MilterEventLoop *loop)
{
    const gchar **clients;
    gsize length, i;

    clients = milter_manager_test_scenario_get_string_list(
        scenario, MILTER_MANAGER_TEST_SCENARIO_GROUP_NAME, "clients", &length);
    for (i = 0; i < length; i++) {
        cut_trace(start_client(scenario, clients[i], loop));
    }
}

const GList *
milter_manager_test_scenario_get_started_clients (MilterManagerTestScenario *scenario)
{
    return MILTER_MANAGER_TEST_SCENARIO_GET_PRIVATE(scenario)->started_clients;
}

const GList *
milter_manager_test_scenario_get_eggs (MilterManagerTestScenario *scenario)
{
    return MILTER_MANAGER_TEST_SCENARIO_GET_PRIVATE(scenario)->eggs;
}

gboolean
milter_manager_test_scenario_has_key (MilterManagerTestScenario *scenario,
                                      const gchar *group,
                                      const gchar *key)
{
    MilterManagerTestScenarioPrivate *priv;
    GError *error = NULL;
    gboolean result;

    priv = MILTER_MANAGER_TEST_SCENARIO_GET_PRIVATE(scenario);
    result = g_key_file_has_key(priv->key_file, group, key, &error);
    gcut_assert_error(error, cut_message("[%s]:[%s]", group, key));
    return result;
}

gboolean
milter_manager_test_scenario_has_group (MilterManagerTestScenario *scenario,
                                        const gchar *group)
{
    MilterManagerTestScenarioPrivate *priv;

    priv = MILTER_MANAGER_TEST_SCENARIO_GET_PRIVATE(scenario);
    return g_key_file_has_group(priv->key_file, group);
}

MilterOption *
milter_manager_test_scenario_get_option (MilterManagerTestScenario *scenario,
                                         const gchar *group,
                                         MilterManagerTestScenario *base_scenario)
{
    guint32 version;
    MilterActionFlags action;
    MilterStepFlags step;

#define GET_VERSION(_scenario)                                          \
    milter_manager_test_scenario_get_integer(_scenario, group, "version")
#define GET_ACTION(_scenario)                                           \
    milter_manager_test_scenario_get_flags(_scenario, group, "action",  \
                                           MILTER_TYPE_ACTION_FLAGS)
#define GET_STEP(_scenario)                                             \
    milter_manager_test_scenario_get_flags(_scenario, group, "step",    \
                                           MILTER_TYPE_STEP_FLAGS)
#define HAS_VERSION(_scenario)                                          \
    milter_manager_test_scenario_has_key(_scenario, group, "version")
#define HAS_ACTION(_scenario)                                           \
    milter_manager_test_scenario_has_key(_scenario, group, "action")
#define HAS_STEP(_scenario)                                             \
    milter_manager_test_scenario_has_key(_scenario, group, "step")

    version = GET_VERSION(scenario);
    action = GET_ACTION(scenario);
    step = GET_STEP(scenario);

    if (base_scenario &&
        scenario != base_scenario &&
        milter_manager_test_scenario_has_group(base_scenario, group)) {
        if (HAS_VERSION(base_scenario))
            version = GET_VERSION(base_scenario);
        if (HAS_ACTION(base_scenario))
            action = GET_ACTION(base_scenario);
        if (HAS_STEP(base_scenario))
            step = GET_STEP(base_scenario);
    }

#undef GET_VERSION
#undef GET_ACTION
#undef GET_STEP
#undef HAS_VERSION
#undef HAS_ACTION
#undef HAS_STEP

    return milter_option_new(version, action, step);
}

gint
milter_manager_test_scenario_get_integer (MilterManagerTestScenario *scenario,
                                          const gchar *group, const gchar *key)
{
    MilterManagerTestScenarioPrivate *priv;
    GError *error = NULL;
    gint value;

    priv = MILTER_MANAGER_TEST_SCENARIO_GET_PRIVATE(scenario);
    value = g_key_file_get_integer(priv->key_file, group, key, &error);
    gcut_assert_error(error, cut_message("[%s][%s]", group, key));
    return value;
}

gdouble
milter_manager_test_scenario_get_double (MilterManagerTestScenario *scenario,
                                         const gchar *group, const gchar *key)
{
    MilterManagerTestScenarioPrivate *priv;
    GError *error = NULL;
    gdouble value;

    priv = MILTER_MANAGER_TEST_SCENARIO_GET_PRIVATE(scenario);
    value = g_key_file_get_double(priv->key_file, group, key, &error);
    gcut_assert_error(error, cut_message("[%s]", group));
    return value;
}

gboolean
milter_manager_test_scenario_get_boolean (MilterManagerTestScenario *scenario,
                                          const gchar *group, const gchar *key)
{
    MilterManagerTestScenarioPrivate *priv;
    GError *error = NULL;
    gboolean value;

    priv = MILTER_MANAGER_TEST_SCENARIO_GET_PRIVATE(scenario);
    value = g_key_file_get_boolean(priv->key_file, group, key, &error);
    gcut_assert_error(error, cut_message("[%s]", group));
    return value;
}

const gchar *
milter_manager_test_scenario_get_string (MilterManagerTestScenario *scenario,
                                         const gchar *group, const gchar *key)
{
    MilterManagerTestScenarioPrivate *priv;
    GError *error = NULL;
    gchar *value;

    priv = MILTER_MANAGER_TEST_SCENARIO_GET_PRIVATE(scenario);
    value = g_key_file_get_string(priv->key_file, group, key, &error);
    gcut_assert_error(error, cut_message("[%s]", group));
    return cut_take_string(value);
}

const gchar *
milter_manager_test_scenario_get_string_with_sub_key (MilterManagerTestScenario *scenario,
                                                      const gchar *group,
                                                      const gchar *key,
                                                      const gchar *sub_key)
{
    MilterManagerTestScenarioPrivate *priv;
    GError *error = NULL;
    gchar *value;

    priv = MILTER_MANAGER_TEST_SCENARIO_GET_PRIVATE(scenario);
    value = g_key_file_get_locale_string(priv->key_file, group, key, sub_key, &error);
    gcut_assert_error(error, cut_message("[%s]", group));
    return cut_take_string(value);
}

const gchar **
milter_manager_test_scenario_get_string_list (MilterManagerTestScenario *scenario,
                                              const gchar *group,
                                              const gchar *key,
                                              gsize *length)
{
    MilterManagerTestScenarioPrivate *priv;
    GError *error = NULL;
    gchar **value;

    priv = MILTER_MANAGER_TEST_SCENARIO_GET_PRIVATE(scenario);
    value = g_key_file_get_string_list(priv->key_file, group, key, length,
                                       &error);
    gcut_assert_error(error, cut_message("[%s]", group));
    return cut_take_string_array(value);
}

const GList *
milter_manager_test_scenario_get_string_g_list (MilterManagerTestScenario *scenario,
                                                const gchar *group,
                                                const gchar *key)
{
    GList *list = NULL;
    const gchar **strings;
    gsize length, i;

    if (milter_manager_test_scenario_has_key(scenario, group, key)) {
        strings = milter_manager_test_scenario_get_string_list(scenario,
                                                               group, key,
                                                               &length);
        for (i = 0; i < length; i++) {
            if (strings[i] && strings[i][0] == '\0')
                list = g_list_append(list, NULL);
            else
                list = g_list_append(list, g_strdup(strings[i]));
        }
    }

    return gcut_take_list(list, g_free);
}

gint
milter_manager_test_scenario_get_enum (MilterManagerTestScenario *scenario,
                                       const gchar *group,
                                       const gchar *key,
                                       GType enum_type)
{
    MilterManagerTestScenarioPrivate *priv;
    GError *error = NULL;
    gint value;

    priv = MILTER_MANAGER_TEST_SCENARIO_GET_PRIVATE(scenario);
    value = gcut_key_file_get_enum(priv->key_file, group, key, enum_type,
                                   &error);
    gcut_assert_error(error, cut_message("[%s]:[%s]", group, key));
    return value;
}

guint
milter_manager_test_scenario_get_flags (MilterManagerTestScenario *scenario,
                                        const gchar *group,
                                        const gchar *key,
                                        GType flags_type)
{
    MilterManagerTestScenarioPrivate *priv;
    GError *error = NULL;
    guint value;

    priv = MILTER_MANAGER_TEST_SCENARIO_GET_PRIVATE(scenario);
    value = gcut_key_file_get_flags(priv->key_file, group, key, flags_type,
                                    &error);
    gcut_assert_error(error, cut_message("[%s]", group));
    return value;
}

const GList *
milter_manager_test_scenario_get_pair_list (MilterManagerTestScenario *scenario,
                                            const gchar *group,
                                            const gchar *key)
{
    const GList *pairs;

    pairs = milter_manager_test_scenario_get_pair_list_without_sort(scenario, group, key);

    return g_list_sort((GList*)pairs, milter_manager_test_pair_compare);
}

const GList *
milter_manager_test_scenario_get_pair_list_without_sort (MilterManagerTestScenario *scenario,
                                                         const gchar *group,
                                                         const gchar *key)
{
    GList *pairs = NULL;

    if (milter_manager_test_scenario_has_key(scenario, group, key)) {
        const gchar **pair_strings;
        gsize length, i;

        pair_strings =
            milter_manager_test_scenario_get_string_list(scenario, group, key,
                                                         &length);
        for (i = 0; i < length; i += 2) {
            MilterManagerTestPair *pair;
            const gchar *name, *value;

            name = pair_strings[i];
            if (name && name[0] == '\0')
                name = NULL;
            value = pair_strings[i + 1];
            if (value && value[0] == '\0')
                value = NULL;
            pair = milter_manager_test_pair_new(name, value);
            pairs = g_list_append(pairs, pair);
        }
    }

    return gcut_take_list(pairs,
                          (CutDestroyFunction)milter_manager_test_pair_free);
}

const GList *
milter_manager_test_scenario_get_option_list (MilterManagerTestScenario *scenario,
                                              const gchar *group,
                                              const gchar *key)
{
    const GList *option_list = NULL;

    if (milter_manager_test_scenario_has_key(scenario, group, key)) {
        const gchar **option_strings;
        GList *options = NULL;
        gsize length, i;
        GError *error = NULL;

        option_strings =
            milter_manager_test_scenario_get_string_list(scenario, group, key,
                                                         &length);
        for (i = 0; i < length; i += 3) {
            MilterOption *option;
            guint32 version;
            MilterActionFlags action;
            MilterStepFlags step;

            if (option_strings[i][0] == '\0') {
                option = NULL;
            } else {
                version = atoi(option_strings[i]);
                action = gcut_flags_parse(MILTER_TYPE_ACTION_FLAGS,
                                          option_strings[i + 1],
                                          &error);
                if (error)
                    break;
                step = gcut_flags_parse(MILTER_TYPE_STEP_FLAGS,
                                        option_strings[i + 2],
                                        &error);
                if (error)
                    break;
                option = milter_option_new(version, action, step);
            }
            options = g_list_append(options, option);
        }
        option_list = gcut_take_list(options,
                                     (CutDestroyFunction)g_object_unref);
        gcut_assert_error(error);
    }

    return option_list;
}

const GList *
milter_manager_test_scenario_get_header_list (MilterManagerTestScenario *scenario,
                                              const gchar *group,
                                              const gchar *key)
{
    GList *headers = NULL;

    if (milter_manager_test_scenario_has_key(scenario, group, key)) {
        const gchar **header_strings;
        gsize length;

        header_strings =
            milter_manager_test_scenario_get_string_list(scenario,
                                                         group, key, &length);
        for (; *header_strings; header_strings++) {
            MilterHeader *header;
            gchar **components;

            components = g_strsplit(*header_strings, ":", 2);
            header = milter_header_new(components[0], components[1]);
            g_strfreev(components);
            headers = g_list_append(headers, header);
        }
    }

    return gcut_take_list(headers, (CutDestroyFunction)milter_header_free);
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
