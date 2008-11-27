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
#include <milter-test-utils.h>
#include <milter/core/milter-logger.h>
#include <milter/core/milter-enum-types.h>
#undef shutdown

#define MILTER_LOG_DOMAIN "logger-test"

void test_level_from_string (void);
void test_error (void);
void test_critical (void);
void test_message (void);
void test_warning (void);
void test_debug (void);
void test_info (void);

static const gchar *logged_domain;
static MilterLogLevelFlags logged_level;
static const gchar *logged_message;
static const gchar *logged_file;
static guint logged_line;
static const gchar *logged_function;

static void
cb_log (MilterLogger *logger,
        const gchar *domain, MilterLogLevelFlags level,
        const gchar *file, guint line, const gchar *function,
        GTimeVal *time_value, const gchar *message,
        gpointer user_data)
{
    logged_domain = cut_take_strdup(domain);
    logged_level = level;
    logged_file = cut_take_strdup(file);
    logged_line = line;
    logged_function = cut_take_strdup(function);

    logged_message = cut_take_strdup(message);
}


void
setup (void)
{
    logged_domain = NULL;
    logged_level = 0;
    logged_message = NULL;

    g_signal_connect(milter_logger(), "log", G_CALLBACK(cb_log), NULL);
}

void
teardown (void)
{
    g_signal_handlers_disconnect_by_func(milter_logger(),
                                         G_CALLBACK(cb_log), NULL);
}

#define milter_assert_equal_log_level(flags, name)                      \
    gcut_assert_equal_flags(MILTER_TYPE_LOG_LEVEL_FLAGS,                \
                            flags,                                      \
                            milter_log_level_flags_from_string(name))

void
test_level_from_string (void)
{
    milter_assert_equal_log_level(MILTER_LOG_LEVEL_ALL, "all");
    milter_assert_equal_log_level(MILTER_LOG_LEVEL_INFO, "info");
    milter_assert_equal_log_level(MILTER_LOG_LEVEL_ERROR |
                                  MILTER_LOG_LEVEL_CRITICAL,
                                  "error|critical");
}

#define milter_assert_equal_log(level, message)                     \
    cut_assert_equal_string(MILTER_LOG_DOMAIN, logged_domain);      \
    gcut_assert_equal_flags(MILTER_TYPE_LOG_LEVEL_FLAGS,            \
                            level,                                  \
                            logged_level);                          \
    cut_assert_equal_string(__FILE__, logged_file);                 \
    cut_assert_equal_string(__PRETTY_FUNCTION__, logged_function);  \
    cut_assert_equal_uint(line, logged_line);                       \
    cut_assert_equal_string(message, logged_message);

void
test_error (void)
{
    guint line;

    milter_error("error"); line = __LINE__;
    milter_assert_equal_log(MILTER_LOG_LEVEL_ERROR, "error");
}

void
test_critical (void)
{
    guint line;

    milter_critical("critical"); line = __LINE__;
    milter_assert_equal_log(MILTER_LOG_LEVEL_CRITICAL, "critical");
}

void
test_message (void)
{
    guint line;

    milter_message("message"); line = __LINE__;
    milter_assert_equal_log(MILTER_LOG_LEVEL_MESSAGE, "message");
}

void
test_warning (void)
{
    guint line;

    milter_warning("warning"); line = __LINE__;
    milter_assert_equal_log(MILTER_LOG_LEVEL_WARNING, "warning");
}

void
test_debug (void)
{
    guint line;

    milter_debug("debug"); line = __LINE__;
    milter_assert_equal_log(MILTER_LOG_LEVEL_DEBUG, "debug");
}

void
test_info (void)
{
    guint line;

    milter_info("info"); line = __LINE__;
    milter_assert_equal_log(MILTER_LOG_LEVEL_INFO, "info");
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
