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

#include <milter/core/milter-logger.h>
#include <milter/core/milter-utils.h>
#include <milter/core/milter-enum-types.h>

#include <gcutter.h>

void data_inspect_io_condition_error (void);
void test_inspect_io_condition_error (gconstpointer data);
void test_inspect_enum (void);
void test_inspect_flags (void);
void test_inspect_object (void);
void test_inspect_hash_string_string (void);
void test_merge_hash_string_string (void);
void test_inspect_list_pointer (void);
void data_command_to_macro_stage (void);
void test_command_to_macro_stage (gconstpointer data);
void data_macro_stage_to_command (void);
void test_macro_stage_to_command (gconstpointer data);
void data_flags_from_string_success (void);
void test_flags_from_string_success (gconstpointer data);
void data_flags_from_string_error (void);
void test_flags_from_string_error (gconstpointer data);
void data_enum_from_string_success (void);
void test_enum_from_string_success (gconstpointer data);
void data_enum_from_string_error (void);
void test_enum_from_string_error (gconstpointer data);
void data_flags_names (void);
void test_flags_names (gconstpointer data);
void test_append_index (void);
void test_xml_append_text_element (void);
void test_xml_append_boolean_element (void);
void test_xml_append_enum_element (void);

static GIOCondition io_condition;
static const gchar *expected_inspected_io_condition;

static gchar *expected_inspected_enum;
static gchar *actual_inspected_enum;

static gchar *expected_inspected_flags;
static gchar *actual_inspected_flags;

static gchar *expected_inspected_object;
static gchar *actual_inspected_object;

static GString *actual_string;

void
setup (void)
{
    io_condition = 0;
    expected_inspected_io_condition = NULL;

    expected_inspected_enum = NULL;
    actual_inspected_enum = NULL;

    expected_inspected_flags = NULL;
    actual_inspected_flags = NULL;

    expected_inspected_object = NULL;
    actual_inspected_object = NULL;

    actual_string = g_string_new(NULL);
}

void
teardown (void)
{
    if (expected_inspected_enum)
        g_free(expected_inspected_enum);
    if (actual_inspected_enum)
        g_free(actual_inspected_enum);

    if (expected_inspected_flags)
        g_free(expected_inspected_flags);
    if (actual_inspected_flags)
        g_free(actual_inspected_flags);

    if (expected_inspected_object)
        g_free(expected_inspected_object);
    if (actual_inspected_object)
        g_free(actual_inspected_object);

    if (actual_string)
        g_string_free(actual_string, TRUE);
}

static void
setup_io_condition_err_data (void)
{
    io_condition = G_IO_ERR;
    expected_inspected_io_condition = "Error";
}

static void
setup_io_condition_hup_data (void)
{
    io_condition = G_IO_HUP;
    expected_inspected_io_condition = "Hung up";
}

static void
setup_io_condition_nval_data (void)
{
    io_condition = G_IO_NVAL;
    expected_inspected_io_condition = "Invalid request";
}

static void
setup_io_condition_err_hup_data (void)
{
    io_condition = G_IO_ERR | G_IO_HUP;
    expected_inspected_io_condition = "Error | Hung up";
}

static void
setup_io_condition_err_nval_data (void)
{
    io_condition = G_IO_ERR | G_IO_NVAL;
    expected_inspected_io_condition = "Error | Invalid request";
}

static void
setup_io_condition_err_hup_nval_data (void)
{
    io_condition = G_IO_ERR | G_IO_HUP | G_IO_NVAL;
    expected_inspected_io_condition = "Error | Hung up | Invalid request";
}

static void
setup_io_condition_hup_nval_data (void)
{
    io_condition = G_IO_HUP | G_IO_NVAL;
    expected_inspected_io_condition = "Hung up | Invalid request";
}

void
data_inspect_io_condition_error (void)
{
    cut_add_data("G_IO_ERR", setup_io_condition_err_data, NULL,
                 "G_IO_HUP", setup_io_condition_hup_data, NULL,
                 "G_IO_NVAL", setup_io_condition_nval_data, NULL,
                 "G_IO_ERR | G_IO_HUP", setup_io_condition_err_hup_data, NULL,
                 "G_IO_ERR | G_IO_NVAL", setup_io_condition_err_nval_data, NULL,
                 "G_IO_ERR | G_IO_HUP | G_IO_NVAL",
                 setup_io_condition_err_hup_nval_data, NULL,
                 "G_IO_HUP | G_IO_NVAL", setup_io_condition_hup_nval_data, NULL);
}

void
test_inspect_io_condition_error (gconstpointer data)
{
    GCallback setup_func = (GCallback)data;
    const gchar *actual_inspected_io_condition;

    setup_func();

    actual_inspected_io_condition =
        cut_take_string(milter_utils_inspect_io_condition_error(io_condition));
    cut_assert_equal_string(expected_inspected_io_condition,
                            actual_inspected_io_condition);
}

void
test_inspect_enum (void)
{
    expected_inspected_enum = gcut_enum_inspect(MILTER_TYPE_STATUS,
                                                MILTER_STATUS_CONTINUE);
    actual_inspected_enum = milter_utils_inspect_enum(MILTER_TYPE_STATUS,
                                                      MILTER_STATUS_CONTINUE);
    cut_assert_equal_string(expected_inspected_enum,
                            actual_inspected_enum);
}

void
test_inspect_flags (void)
{
    expected_inspected_flags =
        gcut_flags_inspect(MILTER_TYPE_ACTION_FLAGS,
                           MILTER_ACTION_ADD_HEADERS |
                           MILTER_ACTION_CHANGE_ENVELOPE_FROM);
    actual_inspected_flags =
        milter_utils_inspect_flags(MILTER_TYPE_ACTION_FLAGS,
                                   MILTER_ACTION_ADD_HEADERS |
                                   MILTER_ACTION_CHANGE_ENVELOPE_FROM);
    cut_assert_equal_string(expected_inspected_flags,
                            actual_inspected_flags);
}

void
test_inspect_object (void)
{
    MilterOption *option;

    option = milter_option_new(2,
                               MILTER_ACTION_ADD_HEADERS |
                               MILTER_ACTION_ADD_ENVELOPE_RECIPIENT |
                               MILTER_ACTION_CHANGE_ENVELOPE_FROM,
                               MILTER_STEP_NO_HELO |
                               MILTER_STEP_NO_HEADERS);

    expected_inspected_object = gcut_object_inspect(G_OBJECT(option));
    actual_inspected_object = milter_utils_inspect_object(G_OBJECT(option));
    cut_assert_equal_string(expected_inspected_object,
                            actual_inspected_object);
}

void
test_inspect_hash_string_string (void)
{
    GHashTable *hash;
    gchar *inspected;

    hash = gcut_take_new_hash_table_string_string(NULL, NULL, NULL);
    inspected = milter_utils_inspect_hash_string_string(hash);
    cut_assert_equal_string_with_free("{}", inspected);

    hash = gcut_take_new_hash_table_string_string("name", "value",
                                                  NULL);
    inspected = milter_utils_inspect_hash_string_string(hash);
    cut_assert_equal_string_with_free("{\"name\" => \"value\"}", inspected);

    hash = gcut_take_new_hash_table_string_string("name1", "value1",
                                                  "name2", "value2",
                                                  NULL);
    inspected = milter_utils_inspect_hash_string_string(hash);
    cut_assert_equal_string_with_free("{"
                                      "\"name1\" => \"value1\", "
                                      "\"name2\" => \"value2\""
                                      "}",
                                      inspected);
}

void
test_merge_hash_string_string (void)
{
    GHashTable *expected;
    GHashTable *dest;
    GHashTable *src;

    dest = gcut_take_new_hash_table_string_string("name1", "value1",
                                                  "name2", "value2",
                                                  NULL);
    src = gcut_take_new_hash_table_string_string("name1", "new value1",
                                                 "name3", "new value3",
                                                 NULL);
    milter_utils_merge_hash_string_string(dest, src);
    expected = gcut_take_new_hash_table_string_string("name1", "new value1",
                                                      "name2", "value2",
                                                      "name3", "new value3",
                                                      NULL);
    gcut_assert_equal_hash_table_string_string(expected, dest);

    milter_utils_merge_hash_string_string(dest, NULL);
    gcut_assert_equal_hash_table_string_string(expected, dest);
}

void
test_inspect_list_pointer (void)
{
    const GList *list;
    gchar *inspected;

    list = gcut_take_new_list_string(NULL, NULL);
    inspected = milter_utils_inspect_list_pointer(list);
    cut_assert_equal_string_with_free("[]", inspected);

    list = gcut_take_new_list_string("abc", NULL);
    inspected = milter_utils_inspect_list_pointer(list);
    cut_assert_equal_string_with_free(cut_take_printf("[<%p>]", list->data),
                                      inspected);

    list = gcut_take_new_list_string("abc", "def", NULL);
    inspected = milter_utils_inspect_list_pointer(list);
    cut_assert_equal_string_with_free(cut_take_printf("[<%p>, <%p>]",
                                                      list->data,
                                                      list->next->data),
                                      inspected);
}

typedef struct _CommandAndMacroStageTestData
{
    MilterCommand command;
    MilterMacroStage macro_stage;
} CommandAndMacroStageTestData;

static CommandAndMacroStageTestData *
command_and_macro_stage_test_data_new (MilterCommand command,
                                       MilterMacroStage macro_stage)
{
    CommandAndMacroStageTestData *data;

    data = g_new(CommandAndMacroStageTestData, 1);
    data->command = command;
    data->macro_stage = macro_stage;

    return data;
}

void
data_command_to_macro_stage (void)
{
#define DATA_VALID(name)                                                \
    command_and_macro_stage_test_data_new(MILTER_COMMAND_ ## name,      \
                                          MILTER_MACRO_STAGE_ ## name)
#define DATA_INVALID(name)                                              \
    command_and_macro_stage_test_data_new(MILTER_COMMAND_ ## name, -1)

    cut_add_data("negotiate", DATA_INVALID(NEGOTIATE), g_free,
                 "define macro", DATA_INVALID(DEFINE_MACRO), g_free,
                 "connect", DATA_VALID(CONNECT), g_free,
                 "helo", DATA_VALID(HELO), g_free,
                 "envelope from", DATA_VALID(ENVELOPE_FROM), g_free,
                 "envelope recipient", DATA_VALID(ENVELOPE_RECIPIENT), g_free,
                 "header", DATA_INVALID(HEADER), g_free,
                 "end of header", DATA_VALID(END_OF_HEADER), g_free,
                 "data", DATA_VALID(DATA), g_free,
                 "body", DATA_INVALID(BODY), g_free,
                 "end of message", DATA_VALID(END_OF_MESSAGE), g_free,
                 "quit", DATA_INVALID(QUIT), g_free,
                 "quit new connection",
                 DATA_INVALID(QUIT_NEW_CONNECTION), g_free,
                 "abort", DATA_INVALID(ABORT), g_free,
                 "unknown", DATA_INVALID(UNKNOWN), g_free);
#undef DATA_VALID
#undef DATA_INVALID
}

void
test_command_to_macro_stage (gconstpointer data)
{
    const CommandAndMacroStageTestData *test_data = data;
    MilterMacroStage stage;

    stage = milter_utils_command_to_macro_stage(test_data->command);
    gcut_assert_equal_enum(MILTER_TYPE_MACRO_STAGE,
                           test_data->macro_stage,
                           stage);
}

void
data_macro_stage_to_command (void)
{
#define DATA_VALID(name)                                                \
    command_and_macro_stage_test_data_new(MILTER_COMMAND_ ## name,      \
                                          MILTER_MACRO_STAGE_ ## name)
#define DATA_INVALID(value)                                             \
    command_and_macro_stage_test_data_new('\0', value)

    cut_add_data("connect", DATA_VALID(CONNECT), g_free,
                 "helo", DATA_VALID(HELO), g_free,
                 "envelope from", DATA_VALID(ENVELOPE_FROM), g_free,
                 "envelope recipient", DATA_VALID(ENVELOPE_RECIPIENT), g_free,
                 "end of header", DATA_VALID(END_OF_HEADER), g_free,
                 "data", DATA_VALID(DATA), g_free,
                 "end of message", DATA_VALID(END_OF_MESSAGE), g_free,
                 "unknown value", DATA_INVALID(-1), g_free);
#undef DATA_VALID
#undef DATA_INVALID
}

void
test_macro_stage_to_command (gconstpointer data)
{
    const CommandAndMacroStageTestData *test_data = data;
    MilterCommand command;

    command = milter_utils_macro_stage_to_command(test_data->macro_stage);
    gcut_assert_equal_enum(MILTER_TYPE_COMMAND,
                           test_data->command,
                           command);
}

void
data_flags_from_string_success (void)
{
#define ADD(label, expected, input, base_flags)                         \
    gcut_add_datum(label,                                               \
                   "/expected", MILTER_TYPE_LOG_LEVEL_FLAGS, expected,  \
                   "/input", G_TYPE_STRING, input,                      \
                   "/type", G_TYPE_GTYPE, MILTER_TYPE_LOG_LEVEL_FLAGS,  \
                   "/base-flags", MILTER_TYPE_LOG_LEVEL_FLAGS, base_flags, \
                   NULL)

    ADD("one",
        MILTER_LOG_LEVEL_ERROR,
        "error",
        0);
    ADD("none",
        0,
        "",
        0);
    ADD("multi",
        MILTER_LOG_LEVEL_ERROR | MILTER_LOG_LEVEL_INFO,
        "error|info",
        0);
    ADD("all",
        MILTER_LOG_LEVEL_ALL | MILTER_LOG_LEVEL_NONE,
        "all",
        0);
    ADD("multi - all",
        MILTER_LOG_LEVEL_ALL | MILTER_LOG_LEVEL_NONE,
        "error|all|info",
        0);
    ADD("append",
        MILTER_LOG_LEVEL_CRITICAL |
        MILTER_LOG_LEVEL_WARNING |
        MILTER_LOG_LEVEL_ERROR,
        "+warning|error",
        MILTER_LOG_LEVEL_CRITICAL);
    ADD("remove",
        MILTER_LOG_LEVEL_CRITICAL,
        "-warning|error",
        MILTER_LOG_LEVEL_CRITICAL |
        MILTER_LOG_LEVEL_WARNING |
        MILTER_LOG_LEVEL_ERROR);
    ADD("remove - all",
        0,
        "-all",
        MILTER_LOG_LEVEL_CRITICAL |
        MILTER_LOG_LEVEL_WARNING |
        MILTER_LOG_LEVEL_ERROR);
    ADD("remove - multi - all",
        0,
        "-warning|all|error",
        MILTER_LOG_LEVEL_CRITICAL |
        MILTER_LOG_LEVEL_WARNING |
        MILTER_LOG_LEVEL_ERROR);
    ADD("local append",
        MILTER_LOG_LEVEL_CRITICAL | MILTER_LOG_LEVEL_DEBUG,
        "-warning|error|+debug",
        MILTER_LOG_LEVEL_CRITICAL |
        MILTER_LOG_LEVEL_WARNING |
        MILTER_LOG_LEVEL_ERROR);
    ADD("local remove",
        (MILTER_LOG_LEVEL_ALL | MILTER_LOG_LEVEL_NONE) & ~MILTER_LOG_LEVEL_TRACE,
        "all|-trace",
        0);

#undef ADD
}

void
test_flags_from_string_success (gconstpointer data)
{
    const gchar *input;
    GType type;
    MilterLogLevelFlags base_flags, expected, actual;
    GError *error = NULL;

    input = gcut_data_get_string(data, "/input");
    type = gcut_data_get_type(data, "/type");
    base_flags = gcut_data_get_flags(data, "/base-flags");
    expected = gcut_data_get_flags(data, "/expected");
    actual = milter_utils_flags_from_string(type, input, base_flags, &error);
    gcut_assert_equal_flags(type, expected, actual);
    gcut_assert_error(error);
}

void
data_flags_from_string_error (void)
{
#define ADD(label, input, expected)                                     \
    gcut_add_datum(label,                                               \
                   "/input", G_TYPE_STRING, input,                      \
                   "/type", G_TYPE_GTYPE, MILTER_TYPE_LOG_LEVEL_FLAGS,  \
                   "/expected", G_TYPE_POINTER, expected, g_error_free, \
                   NULL)

    ADD("null",
        NULL,
        g_error_new(MILTER_FLAGS_ERROR,
                    MILTER_FLAGS_ERROR_NULL_NAME,
                    "flags name is NULL (<MilterLogLevelFlags>)"));
    ADD("invalid",
        "unknown|nonexistent",
        g_error_new(MILTER_FLAGS_ERROR,
                    MILTER_FLAGS_ERROR_UNKNOWN_NAMES,
                    "unknown flag names: [<unknown>|<nonexistent>]"
                    "(<MilterLogLevelFlags>)"));

#undef ADD
}

void
test_flags_from_string_error (gconstpointer data)
{
    const gchar *input;
    GType type;
    MilterLogLevelFlags base_flags;
    GError *error = NULL;

    input = gcut_data_get_string(data, "/input");
    type = gcut_data_get_type(data, "/type");
    base_flags = MILTER_LOG_LEVEL_DEFAULT;
    gcut_assert_equal_flags(type,
                            0,
                            milter_utils_flags_from_string(type, input,
                                                           base_flags, &error));
    gcut_assert_equal_error(gcut_data_get_pointer(data, "/expected"),
                            error);
}

void
data_enum_from_string_success (void)
{
#define ADD(label, input, expected)                                     \
    gcut_add_datum(label,                                               \
                   "/input", G_TYPE_STRING, input,                      \
                   "/type", G_TYPE_GTYPE, MILTER_TYPE_LOG_COLORIZE,     \
                   "/expected", MILTER_TYPE_LOG_COLORIZE, expected,     \
                   NULL)

    ADD("one", "console", MILTER_LOG_COLORIZE_CONSOLE);

#undef ADD
}

void
test_enum_from_string_success (gconstpointer data)
{
    const gchar *input;
    GType type;
    GError *error = NULL;

    input = gcut_data_get_string(data, "/input");
    type = gcut_data_get_type(data, "/type");
    gcut_assert_equal_enum(type,
                           gcut_data_get_enum(data, "/expected"),
                           milter_utils_enum_from_string(type, input, &error));
    gcut_assert_error(error);
}

void
data_enum_from_string_error (void)
{
#define ADD(label, input, expected)                                     \
    gcut_add_datum(label,                                               \
                   "/input", G_TYPE_STRING, input,                      \
                   "/type", G_TYPE_GTYPE, MILTER_TYPE_LOG_COLORIZE,     \
                   "/expected", G_TYPE_POINTER, expected, g_error_free, \
                   NULL)

    ADD("null",
        NULL,
        g_error_new(MILTER_ENUM_ERROR,
                    MILTER_ENUM_ERROR_NULL_NAME,
                    "enum name is NULL (<MilterLogColorize>)"));
    ADD("none",
        "",
        g_error_new(MILTER_ENUM_ERROR,
                    MILTER_ENUM_ERROR_UNKNOWN_NAME,
                    "unknown enum name: <>(<MilterLogColorize>)"));
    ADD("invalid",
        "unknown",
        g_error_new(MILTER_ENUM_ERROR,
                    MILTER_ENUM_ERROR_UNKNOWN_NAME,
                    "unknown enum name: <unknown>(<MilterLogColorize>)"));

#undef ADD
}

void
test_enum_from_string_error (gconstpointer data)
{
    const gchar *input;
    GType type;
    GError *error = NULL;

    input = gcut_data_get_string(data, "/input");
    type = gcut_data_get_type(data, "/type");
    gcut_assert_equal_enum(type,
                           0,
                           milter_utils_enum_from_string(type, input, &error));
    gcut_assert_equal_error(gcut_data_get_pointer(data, "/expected"),
                            error);
}

void
data_flags_names (void)
{
#define ADD(label, input, expected)                                     \
    gcut_add_datum(label,                                               \
                   "/input", MILTER_TYPE_LOG_LEVEL_FLAGS, input,        \
                   "/type", G_TYPE_GTYPE, MILTER_TYPE_LOG_LEVEL_FLAGS,  \
                   "/expected", G_TYPE_STRING, expected,                \
                   NULL)

    ADD("one", MILTER_LOG_LEVEL_ERROR, "error");
    ADD("none", 0, "");
    ADD("multi", MILTER_LOG_LEVEL_CRITICAL | MILTER_LOG_LEVEL_INFO,
        "critical|info");
    ADD("all", MILTER_LOG_LEVEL_ALL,
        "critical|error|warning|message|info|debug|trace|statistics|profile");
    ADD("unknown", MILTER_LOG_LEVEL_PROFILE << 1,
        "(unknown flags: MilterLogLevelFlags:0x400)");

#undef ADD
}

void
test_flags_names (gconstpointer data)
{
    MilterLogLevelFlags input;
    GType type;

    input = gcut_data_get_flags(data, "/input");
    type = gcut_data_get_type(data, "/type");
    cut_assert_equal_string_with_free(gcut_data_get_string(data, "/expected"),
                                      milter_utils_get_flags_names(type, input));
}

void
test_append_index (void)
{
    milter_utils_append_indent(actual_string, 3);
    cut_assert_equal_string("   ", actual_string->str);
}

void
test_xml_append_text_element (void)
{
    milter_utils_xml_append_text_element(actual_string, "NAME", "CONTENT<>", 2);
    cut_assert_equal_string("  <NAME>CONTENT&lt;&gt;</NAME>\n",
                            actual_string->str);
}

void
test_xml_append_boolean_element (void)
{
    milter_utils_xml_append_boolean_element(actual_string, "NAME", TRUE, 2);
    cut_assert_equal_string("  <NAME>true</NAME>\n", actual_string->str);

    g_string_truncate(actual_string, 0);
    milter_utils_xml_append_boolean_element(actual_string, "NAME", FALSE, 2);
    cut_assert_equal_string("  <NAME>false</NAME>\n", actual_string->str);
}

void
test_xml_append_enum_element (void)
{
    milter_utils_xml_append_enum_element(actual_string, "NAME",
                                         MILTER_TYPE_STATUS,
                                         MILTER_STATUS_CONTINUE,
                                         2);
    cut_assert_equal_string("  <NAME>continue</NAME>\n", actual_string->str);
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
