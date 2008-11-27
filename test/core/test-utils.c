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

#include <gcutter.h>

#define shutdown inet_shutdown
#include <milter/core/milter-utils.h>
#include <milter/core/milter-enum-types.h>
#undef shutdown

void data_inspect_io_condition_error (void);
void test_inspect_io_condition_error (gconstpointer data);
void test_inspect_enum (void);
void test_inspect_flags (void);
void test_inspect_object (void);
void data_command_to_macro_stage (void);
void test_command_to_macro_stage (gconstpointer data);
void data_macro_stage_to_command (void);
void test_macro_stage_to_command (gconstpointer data);

static GIOCondition io_condition;
static const gchar *expected_inspected_io_condition;

static gchar *expected_inspected_enum;
static gchar *actual_inspected_enum;

static gchar *expected_inspected_flags;
static gchar *actual_inspected_flags;

static gchar *expected_inspected_object;
static gchar *actual_inspected_object;

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

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
