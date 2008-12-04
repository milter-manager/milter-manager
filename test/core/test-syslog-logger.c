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

#include <string.h>

#include <milter/core/milter-syslog-logger.h>

#include <gcutter.h>
#include <glib/gstdio.h>

void test_info (void);

static MilterSyslogLogger *logger;
static GIOChannel *syslog;
static GError *error;
static gchar *actual;
static gsize first_log_file_size;

#define MILTER_LOG_DOMAIN "MilterSyslogTestSyslogLogger"
#define SYSLOG_NAME "/var/log/syslog"

static void
setup_syslog (void)
{
    struct stat buf;
    cut_assert_true(g_file_test(SYSLOG_NAME, G_FILE_TEST_EXISTS),
                    SYSLOG_NAME " does not exists.");
    cut_assert_equal_int(0, g_lstat(SYSLOG_NAME, &buf));
    first_log_file_size = buf.st_size;
}

void
setup (void)
{
    logger = milter_syslog_logger_new(MILTER_LOG_DOMAIN);
    actual = NULL;

    error = NULL;
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
}

static void
collect_log_message (void)
{
    gsize read_length;

    syslog = g_io_channel_new_file(SYSLOG_NAME, "r", &error);
    gcut_assert_error(error);

    g_io_channel_seek_position(syslog, first_log_file_size, G_SEEK_SET, &error);
    gcut_assert_error(error);

    g_io_channel_read_to_end(syslog, &actual, &read_length, &error);
    gcut_assert_error(error);
}

void
test_info (void)
{
    milter_info("This is informative message.");
    cut_trace(collect_log_message());

    cut_assert_match(".* " MILTER_LOG_DOMAIN "\\[\\d+\\]: "
                     "This is informative message.$",
                     actual);

}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
