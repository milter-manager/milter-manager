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

#include <milter/core/milter-syslog-logger.h>

#include <gcutter.h>
#include <glib/gstdio.h>

#define MILTER_LOG_DOMAIN "MilterSyslogTestSyslogLogger"

void test_info (void);
void test_statistics (void);

static MilterSyslogLogger *logger;
static GIOChannel *syslog;
static gchar *actual;
static gchar *syslog_file_name;
static gsize first_log_file_size;

static MilterLogLevelFlags original_log_level;
static GPrintFunc original_print_hander;

static void
setup_syslog (void)
{
    struct stat buf;
    gint i;
    gchar *candidates[] = {
        "/var/log/mail.log",
        "/var/log/maillog",
        "/var/log/syslog",
        "/var/log/messages",
        NULL
    };

    for (i = 0; candidates[i]; i++) {
        if (g_file_test(candidates[i], G_FILE_TEST_EXISTS)) {
            syslog_file_name = g_strdup(candidates[i]);
            break;
        }
    }
    cut_set_message("syslog file name candidates: [%s]",
                    cut_take_string(g_strjoinv(", ", candidates)));
    cut_assert_not_null(syslog_file_name);

    cut_assert_equal_int(0, g_lstat(syslog_file_name, &buf));
    first_log_file_size = buf.st_size;
}

void
setup (void)
{
    original_log_level = milter_get_log_level();
    original_print_hander = NULL;

    logger = milter_syslog_logger_new(MILTER_LOG_DOMAIN);
    actual = NULL;
    syslog_file_name = NULL;
    cut_trace(setup_syslog());
}

void
teardown (void)
{
    if (logger)
        g_object_unref(logger);

    if (syslog)
        g_io_channel_unref(syslog);

    if (actual)
        g_free(actual);

    if (syslog_file_name)
        g_free(syslog_file_name);

    if (original_print_hander)
        g_set_print_handler(original_print_hander);

    milter_set_log_level(original_log_level);
}

static void
collect_log_message (void)
{
    gsize read_length;
    GError *error = NULL;

    syslog = g_io_channel_new_file(syslog_file_name, "r", &error);
    gcut_assert_error(error);

    g_io_channel_seek_position(syslog, first_log_file_size, G_SEEK_SET, &error);
    gcut_assert_error(error);

    g_io_channel_read_to_end(syslog, &actual, &read_length, &error);
    gcut_assert_error(error);
}

static void
print_handler (const gchar *string)
{
}

void
test_info (void)
{
    original_print_hander = g_set_print_handler(print_handler);
    milter_set_log_level(MILTER_LOG_LEVEL_INFO);
    milter_info("This is informative message.");
    g_set_print_handler(original_print_hander);
    original_print_hander = NULL;

    cut_trace(collect_log_message());

    cut_assert_match(".* " MILTER_LOG_DOMAIN "\\[\\d+\\]: "
                     "This is informative message.$",
                     actual);

}

void
test_statistics (void)
{
    milter_statistics("This is statistics message.");
    cut_trace(collect_log_message());

    cut_assert_match(".* " MILTER_LOG_DOMAIN "\\[\\d+\\]: "
                     "\\[statistics\\] "
                     "This is statistics message.$",
                     actual);

}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
