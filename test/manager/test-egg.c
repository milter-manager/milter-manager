/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Copyright (C) 2008-2010  Kouhei Sutou <kou@clear-code.com>
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

#include <milter-test-utils.h>
#include <milter-manager-test-utils.h>
#include <milter/manager/milter-manager-egg.h>

#include <gcutter.h>

#define TIMEOUT_LEEWAY 3
#define DEFAULT_CONNECTION_TIMEOUT \
    (MILTER_SERVER_CONTEXT_DEFAULT_CONNECTION_TIMEOUT - TIMEOUT_LEEWAY)
#define DEFAULT_WRITING_TIMEOUT \
    (MILTER_SERVER_CONTEXT_DEFAULT_WRITING_TIMEOUT - TIMEOUT_LEEWAY)
#define DEFAULT_READING_TIMEOUT \
    (MILTER_SERVER_CONTEXT_DEFAULT_READING_TIMEOUT - TIMEOUT_LEEWAY)
#define DEFAULT_END_OF_MESSAGE_TIMEOUT \
    (MILTER_SERVER_CONTEXT_DEFAULT_END_OF_MESSAGE_TIMEOUT - TIMEOUT_LEEWAY)

void test_new (void);
void test_hatch (void);
void test_name (void);
void test_description (void);
void test_enabled (void);
void test_connection_spec (void);
void test_connection_spec_error (void);
void test_connection_timeout (void);
void test_writing_timeout (void);
void test_reading_timeout (void);
void test_end_of_message_timeout (void);
void test_user_name (void);
void test_command (void);
void test_command_options (void);
void test_fallback_status (void);
void test_evaluation_mode (void);
void test_merge (void);
void test_applicable_condition (void);
void test_attach_applicable_conditions (void);
void test_to_xml (void);

static MilterManagerEgg *egg;
static MilterManagerEgg *merged_egg;
static MilterManagerChild *child;
static MilterManagerChild *hatched_child;
static MilterManagerChildren *children;
static MilterManagerApplicableCondition *condition;

static GError *expected_error;
static GError *actual_error;

static GList *expected_applicable_conditions;

static gchar *actual_xml;


static const gchar *milter_log_level;

void
setup (void)
{
    egg = NULL;
    merged_egg = NULL;
    child = NULL;
    hatched_child = NULL;
    children = NULL;
    condition = NULL;

    expected_error = NULL;
    actual_error = NULL;

    expected_applicable_conditions = NULL;

    actual_xml = NULL;

    milter_log_level = g_getenv("MILTER_LOG_LEVEL");
}

void
teardown (void)
{
    if (egg)
        g_object_unref(egg);
    if (merged_egg)
        g_object_unref(merged_egg);
    if (child)
        g_object_unref(child);
    if (hatched_child)
        g_object_unref(hatched_child);
    if (children)
        g_object_unref(children);
    if (condition)
        g_object_unref(condition);

    if (expected_error)
        g_error_free(expected_error);
    if (actual_error)
        g_error_free(actual_error);

    if (expected_applicable_conditions) {
        g_list_foreach(expected_applicable_conditions,
                       (GFunc)g_object_unref, NULL);
        g_list_free(expected_applicable_conditions);
    }

    if (actual_xml)
        g_free(actual_xml);

    if (milter_log_level)
        g_setenv("MILTER_LOG_LEVEL", milter_log_level, TRUE);
}

void
test_new (void)
{
    const gchar name[] = "child-milter";

    egg = milter_manager_egg_new(name);
    cut_assert_equal_string(name, milter_manager_egg_get_name(egg));
    cut_assert_equal_double(DEFAULT_CONNECTION_TIMEOUT, 0.0,
                            milter_manager_egg_get_connection_timeout(egg));
    cut_assert_equal_double(DEFAULT_WRITING_TIMEOUT, 0.0,
                            milter_manager_egg_get_writing_timeout(egg));
    cut_assert_equal_double(DEFAULT_READING_TIMEOUT, 0.0,
                            milter_manager_egg_get_reading_timeout(egg));
    cut_assert_equal_double(DEFAULT_END_OF_MESSAGE_TIMEOUT, 0.0,
                            milter_manager_egg_get_end_of_message_timeout(egg));
}

static void
cb_hatched (MilterManagerEgg *egg, MilterManagerChild *child, gpointer user_data)
{
    gboolean *hatched = user_data;

    *hatched = TRUE;

    if (hatched_child)
        g_object_unref(hatched_child);
    hatched_child = child;
    if (hatched_child)
        g_object_ref(hatched_child);
}

static void
cb_attach_to (MilterManagerApplicableCondition *condition,
              MilterManagerChild *child, MilterManagerChildren *children,
              MilterClientContext *client_context, gpointer user_data)
{
    gboolean *attached_to = user_data;

    *attached_to = TRUE;
}

void
test_hatch (void)
{
    const gchar spec[] = "inet:9999@127.0.0.1";
    gboolean hatched = FALSE;
    gboolean attached_to = FALSE;
    GError *error = NULL;

    egg = milter_manager_egg_new("child-milter");
    g_signal_connect(egg, "hatched", G_CALLBACK(cb_hatched), &hatched);

    milter_manager_egg_set_connection_spec(egg, spec, &error);
    gcut_assert_error(error);

    milter_manager_egg_set_fallback_status(egg, MILTER_STATUS_TEMPORARY_FAILURE);

    condition = milter_manager_applicable_condition_new("S25R");
    g_signal_connect(condition, "attach-to",
                     G_CALLBACK(cb_attach_to), &attached_to);
    milter_manager_egg_add_applicable_condition(egg, condition);

    child = milter_manager_egg_hatch(egg);
    cut_assert_not_null(child);
    cut_assert_equal_string("child-milter",
                            milter_server_context_get_name(MILTER_SERVER_CONTEXT(child)));
    cut_assert_true(hatched);
    cut_assert_false(attached_to);

    gcut_assert_equal_enum(
        MILTER_TYPE_STATUS,
        MILTER_STATUS_TEMPORARY_FAILURE,
        milter_manager_child_get_fallback_status(hatched_child));
}

void
test_name (void)
{
    const gchar name[] = "child-milter";
    const gchar new_name[] = "new-child-milter";

    egg = milter_manager_egg_new(name);
    cut_assert_equal_string(name, milter_manager_egg_get_name(egg));
    milter_manager_egg_set_name(egg, new_name);
    cut_assert_equal_string(new_name, milter_manager_egg_get_name(egg));
}

void
test_description (void)
{
    egg = milter_manager_egg_new("child-milter");

    cut_assert_equal_string(NULL,
                            milter_manager_egg_get_description(egg));
    milter_manager_egg_set_description(egg, "Description");
    cut_assert_equal_string("Description",
                            milter_manager_egg_get_description(egg));
}

void
test_enabled (void)
{
    egg = milter_manager_egg_new("child-milter");
    cut_assert_true(milter_manager_egg_is_enabled(egg));
    milter_manager_egg_set_enabled(egg, FALSE);
    cut_assert_false(milter_manager_egg_is_enabled(egg));
}

void
test_connection_spec (void)
{
    const gchar spec[] = "inet:9999@127.0.0.1";
    GError *error = NULL;

    egg = milter_manager_egg_new("child-milter");

    milter_manager_egg_set_connection_spec(egg, spec, &error);
    gcut_assert_error(error);
    cut_assert_equal_string(spec, milter_manager_egg_get_connection_spec(egg));
}

void
test_connection_spec_error (void)
{
    const gchar spec[] = "unknown:9999@127.0.0.1";

    egg = milter_manager_egg_new("child-milter");

    expected_error = g_error_new(MILTER_MANAGER_EGG_ERROR,
                                 MILTER_MANAGER_EGG_ERROR_INVALID,
                                 "<child-milter>: invalid connection spec: "
                                 "milter-connection-error-quark:%d: "
                                 "protocol must be 'unix', 'inet' or 'inet6': "
                                 "<%s>: <unknown>",
                                 MILTER_CONNECTION_ERROR_INVALID_FORMAT,
                                 spec);
    g_unsetenv("MILTER_LOG_LEVEL");
    milter_manager_egg_set_connection_spec(egg, spec, &actual_error);
    gcut_assert_equal_error(expected_error, actual_error);
}

void
test_connection_timeout (void)
{
    gdouble new_connection_timeout = 100;

    egg = milter_manager_egg_new("child-milter");

    milter_manager_egg_set_connection_timeout(egg, new_connection_timeout);
    cut_assert_equal_double(new_connection_timeout, 0.0,
                            milter_manager_egg_get_connection_timeout(egg));
}

void
test_writing_timeout (void)
{
    gdouble new_writing_timeout = 100;

    egg = milter_manager_egg_new("child-milter");

    milter_manager_egg_set_writing_timeout(egg, new_writing_timeout);
    cut_assert_equal_double(new_writing_timeout, 0.0,
                            milter_manager_egg_get_writing_timeout(egg));
}

void
test_reading_timeout (void)
{
    gdouble new_reading_timeout = 100;

    egg = milter_manager_egg_new("child-milter");

    milter_manager_egg_set_reading_timeout(egg, new_reading_timeout);
    cut_assert_equal_double(new_reading_timeout, 0.0,
                            milter_manager_egg_get_reading_timeout(egg));
}

void
test_end_of_message_timeout (void)
{
    gdouble new_end_of_message_timeout = 100;

    egg = milter_manager_egg_new("child-milter");

    milter_manager_egg_set_end_of_message_timeout(egg,
                                                    new_end_of_message_timeout);
    cut_assert_equal_double(new_end_of_message_timeout, 0.0, 
                            milter_manager_egg_get_end_of_message_timeout(egg));
}

void
test_user_name (void)
{
    const gchar user_name[] = "milter-user";

    egg = milter_manager_egg_new("child-milter");
    cut_assert_equal_string(NULL,
                            milter_manager_egg_get_user_name(egg));
    milter_manager_egg_set_user_name(egg, user_name);
    cut_assert_equal_string(user_name,
                            milter_manager_egg_get_user_name(egg));
}

void
test_command (void)
{
    const gchar command[] = "/usr/bin/my-milter";

    egg = milter_manager_egg_new("child-milter");
    cut_assert_equal_string(NULL,
                            milter_manager_egg_get_command(egg));
    milter_manager_egg_set_command(egg, command);
    cut_assert_equal_string(command, milter_manager_egg_get_command(egg));
}

void
test_command_options (void)
{
    const gchar command_options[] = "--port=10026 --print-status";

    egg = milter_manager_egg_new("child-milter");
    cut_assert_equal_string(NULL,
                            milter_manager_egg_get_command_options(egg));
    milter_manager_egg_set_command_options(egg, command_options);
    cut_assert_equal_string(command_options, milter_manager_egg_get_command_options(egg));
}

void
test_fallback_status (void)
{
    egg = milter_manager_egg_new("child-milter");
    gcut_assert_equal_enum(MILTER_TYPE_STATUS,
                           MILTER_STATUS_ACCEPT,
                           milter_manager_egg_get_fallback_status(egg));

    milter_manager_egg_set_fallback_status(egg, MILTER_STATUS_TEMPORARY_FAILURE);
    gcut_assert_equal_enum(MILTER_TYPE_STATUS,
                           MILTER_STATUS_TEMPORARY_FAILURE,
                           milter_manager_egg_get_fallback_status(egg));
}

void
test_evaluation_mode (void)
{
    const gchar spec[] = "inet:9999@127.0.0.1";
    GError *error = NULL;

    egg = milter_manager_egg_new("child-milter");
    milter_manager_egg_set_connection_spec(egg, spec, &error);
    gcut_assert_error(error);

    cut_assert_false(milter_manager_egg_is_evaluation_mode(egg));
    child = milter_manager_egg_hatch(egg);
    cut_assert_not_null(child);
    cut_assert_false(milter_manager_child_is_evaluation_mode(child));

    milter_manager_egg_set_evaluation_mode(egg, TRUE);
    cut_assert_true(milter_manager_egg_is_evaluation_mode(egg));

    g_object_unref(child);
    child = milter_manager_egg_hatch(egg);
    cut_assert_not_null(child);
    cut_assert_true(milter_manager_child_is_evaluation_mode(child));
}

void
test_applicable_condition (void)
{
    MilterManagerApplicableCondition *condition;

    egg = milter_manager_egg_new("child-milter");
    gcut_assert_equal_list_object_custom(
        NULL,
        milter_manager_egg_get_applicable_conditions(egg),
        milter_manager_test_applicable_condition_equal);

    condition = milter_manager_applicable_condition_new("S25R");
    expected_applicable_conditions =
        g_list_append(expected_applicable_conditions, condition);
    milter_manager_egg_add_applicable_condition(egg, condition);
    gcut_assert_equal_list_object_custom(
        expected_applicable_conditions,
        milter_manager_egg_get_applicable_conditions(egg),
        milter_manager_test_applicable_condition_equal);

    milter_manager_egg_clear_applicable_conditions(egg);
    gcut_assert_equal_list_object_custom(
        NULL,
        milter_manager_egg_get_applicable_conditions(egg),
        milter_manager_test_applicable_condition_equal);
}

void
test_attach_applicable_conditions (void)
{
    gboolean attached_to = FALSE;

    egg = milter_manager_egg_new("child-milter");

    condition = milter_manager_applicable_condition_new("S25R");
    g_signal_connect(condition, "attach-to",
                     G_CALLBACK(cb_attach_to), &attached_to);
    milter_manager_egg_add_applicable_condition(egg, condition);

    child = milter_manager_child_new("child-milter");
    children = milter_manager_children_new(NULL, NULL);

    milter_manager_egg_attach_applicable_conditions(egg, child, children, NULL);
    cut_assert_true(attached_to);
}

void
test_merge (void)
{
    MilterManagerApplicableCondition *s25r, *remote_network;
    GError *error = NULL;

    egg = milter_manager_egg_new("milter");
    milter_manager_egg_set_description(egg, "Description");
    milter_manager_egg_set_enabled(egg, FALSE);
    milter_manager_egg_set_connection_timeout(egg, 2.9);
    milter_manager_egg_set_writing_timeout(egg, 2.929);
    milter_manager_egg_set_reading_timeout(egg, 2.92929);
    milter_manager_egg_set_end_of_message_timeout(egg, 29.29);
    milter_manager_egg_set_user_name(egg, "milter-user");
    milter_manager_egg_set_command(egg, "milter-test-client");
    milter_manager_egg_set_command_options(egg, "-s inet:2929@localhost");
    milter_manager_egg_set_connection_spec(egg, "inet:2929@localhost", &error);
    gcut_assert_error(error);

    s25r = milter_manager_applicable_condition_new("S25R");
    remote_network = milter_manager_applicable_condition_new("remote-network");
    milter_manager_egg_add_applicable_condition(egg, s25r);
    milter_manager_egg_add_applicable_condition(egg, remote_network);

    expected_applicable_conditions =
        g_list_append(expected_applicable_conditions, s25r);
    expected_applicable_conditions =
        g_list_append(expected_applicable_conditions, remote_network);


    merged_egg = milter_manager_egg_new("merged-milter");
    milter_manager_egg_merge(merged_egg, egg, &error);
    gcut_assert_error(error);

    cut_assert_equal_string("merged-milter",
                            milter_manager_egg_get_name(merged_egg));
    cut_assert_equal_string("Description",
                            milter_manager_egg_get_description(merged_egg));
    cut_assert_false(milter_manager_egg_is_enabled(merged_egg));
    cut_assert_equal_double(2.9,
                            milter_manager_egg_get_connection_timeout(merged_egg),
                            0.01);
    cut_assert_equal_double(2.929,
                            milter_manager_egg_get_writing_timeout(merged_egg),
                            0.0001);
    cut_assert_equal_double(2.92929,
                            milter_manager_egg_get_reading_timeout(merged_egg),
                            0.000001);
    cut_assert_equal_double(29.29,
                            milter_manager_egg_get_end_of_message_timeout(merged_egg),
                            0.0001);
    cut_assert_equal_string("milter-user",
                            milter_manager_egg_get_user_name(merged_egg));
    cut_assert_equal_string("milter-test-client",
                            milter_manager_egg_get_command(merged_egg));
    cut_assert_equal_string("-s inet:2929@localhost",
                            milter_manager_egg_get_command_options(merged_egg));
    cut_assert_equal_string("inet:2929@localhost",
                            milter_manager_egg_get_connection_spec(merged_egg));

    gcut_assert_equal_list_object_custom(
        expected_applicable_conditions,
        milter_manager_egg_get_applicable_conditions(merged_egg),
        milter_manager_test_applicable_condition_equal);
}

static void
cb_to_xml (MilterManagerEgg *egg, GString *xml, guint indent, gpointer user_data)
{
    milter_utils_append_indent(xml, indent);
    g_string_append_printf(xml, "<additional-field>VALUE</additional-field>\n");
}

void
test_to_xml (void)
{
    egg = milter_manager_egg_new("child-milter");

    actual_xml = milter_manager_egg_to_xml(egg);
    cut_assert_equal_string("<milter>\n"
                            "  <name>child-milter</name>\n"
                            "  <enabled>true</enabled>\n"
                            "  <fallback-status>accept</fallback-status>\n"
                            "  <evaluation-mode>false</evaluation-mode>\n"
                            "</milter>\n",
                            actual_xml);

    g_signal_connect(egg, "to-xml", G_CALLBACK(cb_to_xml), NULL);
    g_free(actual_xml);
    actual_xml = milter_manager_egg_to_xml(egg);
    cut_assert_equal_string("<milter>\n"
                            "  <name>child-milter</name>\n"
                            "  <enabled>true</enabled>\n"
                            "  <fallback-status>accept</fallback-status>\n"
                            "  <evaluation-mode>false</evaluation-mode>\n"
                            "  <additional-field>VALUE</additional-field>\n"
                            "</milter>\n",
                            actual_xml);
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
