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


#include <milter/manager/milter-manager-configuration.h>
#include <milter/manager/milter-manager-children.h>

#include <milter-test-utils.h>
#include <milter-manager-test-utils.h>

#include <gcutter.h>

void test_children (void);
void test_privilege_mode (void);
void test_control_connection_spec (void);
void test_manager_connection_spec (void);
void test_fallback_status (void);
void test_applicable_condition (void);
void test_remove_applicable_condition (void);
void test_clear (void);
void test_load_paths (void);
void test_save_custom (void);
void test_to_xml_full (void);
void test_to_xml_signal (void);

static MilterManagerConfiguration *config;
static MilterManagerEgg *egg;
static MilterManagerApplicableCondition *condition;

static MilterManagerChildren *expected_children;
static MilterManagerChildren *actual_children;

static GList *expected_load_paths;

static GList *expected_applicable_conditions;

static gchar *actual_xml;

static gchar *tmp_dir;

void
setup (void)
{
    config = milter_manager_configuration_new(NULL);
    egg = NULL;
    condition = NULL;

    expected_children = milter_manager_children_new(config);
    actual_children = NULL;

    expected_load_paths = NULL;

    expected_applicable_conditions = NULL;

    actual_xml = NULL;

    tmp_dir = g_build_filename(milter_test_get_base_dir(),
                               "tmp",
                               NULL);
    cut_remove_path(tmp_dir, NULL);
    if (g_mkdir_with_parents(tmp_dir, 0700) == -1)
        cut_assert_errno();
}

void
teardown (void)
{
    if (config)
        g_object_unref(config);
    if (egg)
        g_object_unref(egg);
    if (condition)
        g_object_unref(condition);

    if (expected_children)
        g_object_unref(expected_children);
    if (actual_children)
        g_object_unref(actual_children);

    if (expected_load_paths) {
        g_list_foreach(expected_load_paths, (GFunc)g_free, NULL);
        g_list_free(expected_load_paths);
    }

    if (expected_applicable_conditions)
        g_list_free(expected_applicable_conditions);

    if (actual_xml)
        g_free(actual_xml);

    if (tmp_dir) {
        cut_remove_path(tmp_dir, NULL);
        g_free(tmp_dir);
    }
}

static gboolean
child_equal (gconstpointer a, gconstpointer b)
{
    MilterServerContext *context1, *context2;

    context1 = MILTER_SERVER_CONTEXT(a);
    context2 = MILTER_SERVER_CONTEXT(b);

    /* FIXME */
    return g_str_equal(milter_server_context_get_name(context1),
                       milter_server_context_get_name(context2));
}

#define milter_assert_equal_children(expected, actual)             \
    gcut_assert_equal_list_object_custom(                          \
        milter_manager_children_get_children(expected),            \
        milter_manager_children_get_children(actual),              \
        child_equal)

void
test_children (void)
{
    MilterManagerChild *child;
    GError *error = NULL;

    egg = milter_manager_egg_new("child-milter");
    milter_manager_egg_set_connection_spec(egg, "inet:2929@localhost", &error);
    gcut_assert_error(error);

    milter_manager_configuration_add_egg(config, egg);

    child = milter_manager_egg_hatch(egg);
    milter_manager_children_add_child(expected_children, child);

    actual_children = milter_manager_configuration_create_children(config);
    milter_assert_equal_children(expected_children, actual_children);
}

void
test_privilege_mode (void)
{
    cut_assert_false(milter_manager_configuration_is_privilege_mode(config));
    milter_manager_configuration_set_privilege_mode(config, TRUE);
    cut_assert_true(milter_manager_configuration_is_privilege_mode(config));
}

void
test_control_connection_spec (void)
{
    const gchar spec[] = "inet:2929@localhost";
    const gchar *actual_spec;

    actual_spec =
        milter_manager_configuration_get_control_connection_spec(config);
    cut_assert_equal_string(NULL, actual_spec);

    milter_manager_configuration_set_control_connection_spec(config, spec);

    actual_spec =
        milter_manager_configuration_get_control_connection_spec(config);
    cut_assert_equal_string(spec, actual_spec);
}

void
test_manager_connection_spec (void)
{
    const gchar spec[] = "inet:2929@localhost";
    const gchar *actual_spec;

    actual_spec =
        milter_manager_configuration_get_manager_connection_spec(config);
    cut_assert_equal_string(NULL, actual_spec);

    milter_manager_configuration_set_manager_connection_spec(config, spec);

    actual_spec =
        milter_manager_configuration_get_manager_connection_spec(config);
    cut_assert_equal_string(spec, actual_spec);
}

void
test_fallback_status (void)
{
    MilterStatus actual_status;

    actual_status = milter_manager_configuration_get_fallback_status(config);
    gcut_assert_equal_enum(MILTER_TYPE_STATUS,
                           MILTER_STATUS_CONTINUE, actual_status);

    milter_manager_configuration_set_fallback_status(config, MILTER_STATUS_REJECT);

    actual_status = milter_manager_configuration_get_fallback_status(config);
    gcut_assert_equal_enum(MILTER_TYPE_STATUS,
                           MILTER_STATUS_REJECT, actual_status);
}

static void
milter_assert_default_configuration_helper (MilterManagerConfiguration *config)
{
    const gchar *spec;
    MilterStatus status;

    cut_assert_false(milter_manager_configuration_is_privilege_mode(config));

    spec = milter_manager_configuration_get_control_connection_spec(config);
    cut_assert_equal_string(NULL, spec);

    status = milter_manager_configuration_get_fallback_status(config);
    gcut_assert_equal_enum(MILTER_TYPE_STATUS, MILTER_STATUS_CONTINUE, status);

    if (expected_children)
        g_object_unref(expected_children);
    expected_children = milter_manager_children_new(config);
    if (actual_children)
        g_object_unref(actual_children);
    actual_children = milter_manager_configuration_create_children(config);
    milter_assert_equal_children(expected_children, actual_children);
    g_object_unref(actual_children);
    actual_children = NULL;
}

void
test_applicable_condition (void)
{
    gcut_assert_equal_list_object_custom(
        NULL,
        milter_manager_configuration_get_applicable_conditions(config),
        milter_test_equal_manager_applicable_condition);

    condition = milter_manager_applicable_condition_new("S25R");
    expected_applicable_conditions =
        g_list_append(expected_applicable_conditions, condition);
    milter_manager_configuration_add_applicable_condition(config, condition);
    gcut_assert_equal_list_object_custom(
        expected_applicable_conditions,
        milter_manager_configuration_get_applicable_conditions(config),
        milter_test_equal_manager_applicable_condition);

    milter_manager_configuration_clear_applicable_conditions(config);
    gcut_assert_equal_list_object_custom(
        NULL,
        milter_manager_configuration_get_applicable_conditions(config),
        milter_test_equal_manager_applicable_condition);
}

void
test_remove_applicable_condition (void)
{
    MilterManagerApplicableCondition *s25r, *remote_network;

    s25r = milter_manager_applicable_condition_new("S25R");
    remote_network = milter_manager_applicable_condition_new("Remote Network");
    milter_manager_configuration_add_applicable_condition(config, s25r);
    milter_manager_configuration_add_applicable_condition(config,
                                                          remote_network);
    gcut_take_object(G_OBJECT(s25r));
    gcut_take_object(G_OBJECT(remote_network));

    expected_applicable_conditions =
        g_list_append(expected_applicable_conditions, s25r);
    expected_applicable_conditions =
        g_list_append(expected_applicable_conditions, remote_network);
    gcut_assert_equal_list_object_custom(
        expected_applicable_conditions,
        milter_manager_configuration_get_applicable_conditions(config),
        milter_test_equal_manager_applicable_condition);

    milter_manager_configuration_remove_applicable_condition_by_name(config,
                                                                     "S25R");
    g_list_free(expected_applicable_conditions);
    expected_applicable_conditions = g_list_append(NULL, remote_network);
    gcut_assert_equal_list_object_custom(
        expected_applicable_conditions,
        milter_manager_configuration_get_applicable_conditions(config),
        milter_test_equal_manager_applicable_condition);

    milter_manager_configuration_remove_applicable_condition(config,
                                                             remote_network);
    gcut_assert_equal_list_object_custom(
        NULL,
        milter_manager_configuration_get_applicable_conditions(config),
        milter_test_equal_manager_applicable_condition);
}

#define milter_assert_default_configuration(config)             \
    cut_trace_with_info_expression(                             \
        milter_assert_default_configuration_helper(config),     \
        milter_assert_default_configuration(config))

void
test_clear (void)
{
    milter_assert_default_configuration(config);

    test_privilege_mode();
    test_control_connection_spec();
    test_fallback_status();
    test_children();

    milter_manager_configuration_clear(config);
    milter_assert_default_configuration(config);
}

void
test_load_paths (void)
{
    const gchar *config_dir;

    config_dir = g_getenv("MILTER_MANAGER_CONFIG_DIR");
    if (config_dir)
        expected_load_paths = g_list_append(expected_load_paths,
                                            g_strdup(config_dir));
    expected_load_paths = g_list_append(expected_load_paths,
                                        g_strdup(CONFIG_DIR));
    gcut_assert_equal_list_string(
        expected_load_paths,
        milter_manager_configuration_get_load_paths(config));

    expected_load_paths = g_list_append(expected_load_paths,
                                        g_strdup("append/XXX"));
    milter_manager_configuration_append_load_path(config, "append/XXX");
    gcut_assert_equal_list_string(
        expected_load_paths,
        milter_manager_configuration_get_load_paths(config));

    expected_load_paths = g_list_prepend(expected_load_paths,
                                         g_strdup("prepend/XXX"));
    milter_manager_configuration_prepend_load_path(config, "prepend/XXX");
    gcut_assert_equal_list_string(
        expected_load_paths,
        milter_manager_configuration_get_load_paths(config));


    milter_manager_configuration_clear_load_paths(config);
    gcut_assert_equal_list_string(
        NULL,
        milter_manager_configuration_get_load_paths(config));
}

void
test_save_custom (void)
{
    GError *error = NULL;
    gchar *custom_config_path;

    custom_config_path = g_build_filename(tmp_dir,
                                          CUSTOM_CONFIG_FILE_NAME,
                                          NULL);
    cut_take_string(custom_config_path);

    milter_manager_configuration_prepend_load_path(config, tmp_dir);

    cut_assert_path_not_exist(custom_config_path);
    milter_manager_configuration_save_custom(config, "XXX", -1, &error);
    gcut_assert_error(error);
    cut_assert_path_exist(custom_config_path);
}

void
test_to_xml_full (void)
{
    GError *error = NULL;

    egg = milter_manager_egg_new("milter@10025");
    milter_manager_egg_set_connection_spec(egg, "inet:10025", &error);
    gcut_assert_error(error);

    milter_manager_egg_set_command(egg, "test-milter");

    milter_manager_configuration_add_egg(config, egg);


    condition = milter_manager_applicable_condition_new("S25R");
    milter_manager_applicable_condition_set_description(
        condition, "Selective SMTP Rejection");
    milter_manager_configuration_add_applicable_condition(config, condition);

    actual_xml = milter_manager_configuration_to_xml(config);
    cut_assert_equal_string(
        "<configuration>\n"
        "  <applicable-conditions>\n"
        "    <applicable-condition>\n"
        "      <name>S25R</name>\n"
        "      <description>Selective SMTP Rejection</description>\n"
        "    </applicable-condition>\n"
        "  </applicable-conditions>\n"
        "  <milters>\n"
        "    <milter>\n"
        "      <name>milter@10025</name>\n"
        "      <connection-spec>inet:10025</connection-spec>\n"
        "      <command>test-milter</command>\n"
        "    </milter>\n"
        "  </milters>\n"
        "</configuration>\n",
        actual_xml);
}

static void
cb_to_xml (MilterManagerConfiguration *configuration,
           GString *xml, guint indent, gpointer user_data)
{
    milter_utils_append_indent(xml, indent);
    g_string_append_printf(xml, "<additional-field>VALUE</additional-field>\n");
}

void
test_to_xml_signal (void)
{
    GError *error = NULL;

    egg = milter_manager_egg_new("milter@10025");
    milter_manager_egg_set_connection_spec(egg, "inet:10025", &error);
    gcut_assert_error(error);

    milter_manager_egg_set_command(egg, "test-milter");

    milter_manager_configuration_add_egg(config, egg);

    actual_xml = milter_manager_configuration_to_xml(config);
    cut_assert_equal_string(
        "<configuration>\n"
        "  <milters>\n"
        "    <milter>\n"
        "      <name>milter@10025</name>\n"
        "      <connection-spec>inet:10025</connection-spec>\n"
        "      <command>test-milter</command>\n"
        "    </milter>\n"
        "  </milters>\n"
        "</configuration>\n",
        actual_xml);

    g_signal_connect(config, "to-xml", G_CALLBACK(cb_to_xml), NULL);
    g_free(actual_xml);
    actual_xml = milter_manager_configuration_to_xml(config);
    cut_assert_equal_string(
        "<configuration>\n"
        "  <milters>\n"
        "    <milter>\n"
        "      <name>milter@10025</name>\n"
        "      <connection-spec>inet:10025</connection-spec>\n"
        "      <command>test-milter</command>\n"
        "    </milter>\n"
        "  </milters>\n"
        "  <additional-field>VALUE</additional-field>\n"
        "</configuration>\n",
        actual_xml);
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
