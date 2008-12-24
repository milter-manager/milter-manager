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


#include <libmilter/mfapi.h>
#include <libmilter/libmilter-compatible.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <arpa/inet.h>

#include <milter-test-utils.h>

#include <gcutter.h>

void test_opensocket (void);
void test_opensocket_with_remove_socket (void);
void test_setbacklog (void);
void test_setdbg (void);

static gchar *tmp_dir;

static struct smfiDesc smfilter = {
    "test milter",
    SMFI_VERSION,
    SMFIF_ADDHDRS | SMFIF_CHGHDRS,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
};

void
setup (void)
{
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
    libmilter_compatible_reset();

    if (tmp_dir) {
        cut_remove_path(tmp_dir, NULL);
        g_free(tmp_dir);
    }
}

#define milter_assert_success(expression)               \
    cut_assert_equal_int(MI_SUCCESS, expression)

#define milter_assert_fail(expression)                  \
    cut_assert_equal_int(MI_FAILURE, expression)

void
test_opensocket (void)
{
    const gchar *socket_path;

    milter_assert_fail(smfi_opensocket(FALSE));

    milter_assert_success(smfi_register(smfilter));
    milter_assert_fail(smfi_opensocket(FALSE));

    socket_path = cut_take_printf("%s/%s", tmp_dir, "milter.sock");
    smfi_setconn((char *)cut_take_printf("unix:%s", socket_path));
    milter_assert_success(smfi_opensocket(FALSE));

    cut_assert_path_exist(socket_path);
}

void
test_opensocket_with_remove_socket (void)
{
    GError *error = NULL;
    const gchar *socket_path;

    milter_assert_success(smfi_register(smfilter));
    socket_path = cut_take_printf("%s/%s", tmp_dir, "milter.sock");
    smfi_setconn((char *)cut_take_printf("unix:%s", socket_path));
    g_file_set_contents(socket_path, "XXX", -1, &error);
    gcut_assert_error(error);

    milter_assert_success(smfi_opensocket(TRUE));
    cut_assert_path_exist(socket_path);
}

void
test_setbacklog (void)
{
    milter_assert_fail(smfi_setbacklog(-1));
    milter_assert_fail(smfi_setbacklog(0));
    milter_assert_success(smfi_setbacklog(1));
}

#define milter_assert_equal_target_level(expected)              \
    gcut_assert_equal_flags(                                    \
        MILTER_TYPE_LOG_LEVEL_FLAGS,                            \
        expected,                                               \
        milter_logger_get_target_level(milter_logger()))

void
test_setdbg (void)
{
    milter_assert_equal_target_level(0);

    milter_assert_success(smfi_setdbg(1));
    milter_assert_equal_target_level(MILTER_LOG_LEVEL_CRITICAL);

    milter_assert_success(smfi_setdbg(2));
    milter_assert_equal_target_level(MILTER_LOG_LEVEL_CRITICAL |
                                     MILTER_LOG_LEVEL_ERROR);

    milter_assert_success(smfi_setdbg(3));
    milter_assert_equal_target_level(MILTER_LOG_LEVEL_CRITICAL |
                                     MILTER_LOG_LEVEL_ERROR |
                                     MILTER_LOG_LEVEL_WARNING);

    milter_assert_success(smfi_setdbg(4));
    milter_assert_equal_target_level(MILTER_LOG_LEVEL_CRITICAL |
                                     MILTER_LOG_LEVEL_ERROR |
                                     MILTER_LOG_LEVEL_WARNING |
                                     MILTER_LOG_LEVEL_MESSAGE);

    milter_assert_success(smfi_setdbg(5));
    milter_assert_equal_target_level(MILTER_LOG_LEVEL_CRITICAL |
                                     MILTER_LOG_LEVEL_ERROR |
                                     MILTER_LOG_LEVEL_WARNING |
                                     MILTER_LOG_LEVEL_MESSAGE |
                                     MILTER_LOG_LEVEL_INFO);

    milter_assert_success(smfi_setdbg(6));
    milter_assert_equal_target_level(MILTER_LOG_LEVEL_CRITICAL |
                                     MILTER_LOG_LEVEL_ERROR |
                                     MILTER_LOG_LEVEL_WARNING |
                                     MILTER_LOG_LEVEL_MESSAGE |
                                     MILTER_LOG_LEVEL_INFO |
                                     MILTER_LOG_LEVEL_DEBUG);

    milter_assert_success(smfi_setdbg(-1));
    milter_assert_equal_target_level(0);
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
