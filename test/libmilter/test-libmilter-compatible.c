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
void test_settimeout (void);
void test_setconn (void);
void test_stop (void);
void test_version (void);
void test_convert_status_to (void);
void test_convert_status_from (void);
void test_mi_stop (void);

static gchar *tmp_dir;
static guint idle_id;

static MilterEventLoop *loop;

static MilterLogLevelFlags original_log_level;

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
cut_setup (void)
{
    original_log_level = milter_get_log_level();

    tmp_dir = g_build_filename(milter_test_get_base_dir(),
                               "tmp",
                               NULL);
    cut_remove_path(tmp_dir, NULL);
    if (g_mkdir_with_parents(tmp_dir, 0700) == -1)
        cut_assert_errno();

    idle_id = 0;

    loop = milter_test_event_loop_new();
}

void
cut_teardown (void)
{
    milter_set_log_level(original_log_level);

    libmilter_compatible_reset();

    if (tmp_dir) {
        cut_remove_path(tmp_dir, NULL);
        g_free(tmp_dir);
    }

    if (idle_id > 0)
        milter_event_loop_remove(loop, idle_id);

    if (loop)
        g_object_unref(loop);
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
    milter_assert_equal_target_level(MILTER_LOG_LEVEL_NONE);

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

void
test_settimeout (void)
{
    milter_assert_success(smfi_settimeout(7210));
}

void
test_setconn (void)
{
    milter_assert_fail(smfi_setconn(NULL));
    milter_assert_fail(smfi_setconn("unknown:/path"));
    milter_assert_success(smfi_setconn("unix:/tmp/nonexistent"));
    milter_assert_success(smfi_setconn("inet:12345"));
    milter_assert_success(smfi_setconn("inet6:12345@::1"));
}

static gboolean
cb_idle_stop (gpointer data)
{
    gboolean *stopped = data;

    milter_assert_success(smfi_stop());
    *stopped = TRUE;

    idle_id = 0;

    return FALSE;
}

void
test_stop (void)
{
    const gchar *socket_path;
    gboolean stopped = FALSE;

    milter_assert_success(smfi_register(smfilter));

    socket_path = cut_take_printf("unix:%s/sock", tmp_dir);
    milter_assert_success(smfi_setconn((gchar *)socket_path));

    idle_id = milter_event_loop_add_idle(loop, cb_idle_stop, &stopped);

    milter_assert_success(smfi_main());
    cut_assert_true(stopped);
}

void
test_version (void)
{
    unsigned int major, minor, patch_level;

    milter_assert_success(smfi_version(&major, &minor, &patch_level));
    cut_assert_equal_uint(1, major);
    cut_assert_equal_uint(0, minor);
    cut_assert_equal_uint(1, patch_level);
}

void
test_convert_status_to (void)
{
#define milter_assert_convert_status_to(expected, input)                \
    gcut_assert_equal_enum(MILTER_TYPE_STATUS,                          \
                           expected,                                    \
                           libmilter_compatible_convert_status_to(input))

    milter_assert_convert_status_to(MILTER_STATUS_CONTINUE,
                                    SMFIS_CONTINUE);
    milter_assert_convert_status_to(MILTER_STATUS_REJECT,
                                    SMFIS_REJECT);
    milter_assert_convert_status_to(MILTER_STATUS_DISCARD,
                                    SMFIS_DISCARD);
    milter_assert_convert_status_to(MILTER_STATUS_ACCEPT,
                                    SMFIS_ACCEPT);
    milter_assert_convert_status_to(MILTER_STATUS_TEMPORARY_FAILURE,
                                    SMFIS_TEMPFAIL);
    milter_assert_convert_status_to(MILTER_STATUS_NO_REPLY,
                                    SMFIS_NOREPLY);
    milter_assert_convert_status_to(MILTER_STATUS_SKIP,
                                    SMFIS_SKIP);
    milter_assert_convert_status_to(MILTER_STATUS_ALL_OPTIONS,
                                    SMFIS_ALL_OPTS);

#undef milter_assert_convert_status_to
}

void
test_convert_status_from (void)
{
#define milter_assert_convert_status_from(expected, input)              \
    cut_assert_equal_int(expected,                                      \
                         libmilter_compatible_convert_status_from(input))

    milter_assert_convert_status_from(SMFIS_CONTINUE,
                                      MILTER_STATUS_CONTINUE);
    milter_assert_convert_status_from(SMFIS_REJECT,
                                      MILTER_STATUS_REJECT);
    milter_assert_convert_status_from(SMFIS_DISCARD,
                                      MILTER_STATUS_DISCARD);
    milter_assert_convert_status_from(SMFIS_ACCEPT,
                                      MILTER_STATUS_ACCEPT);
    milter_assert_convert_status_from(SMFIS_TEMPFAIL,
                                      MILTER_STATUS_TEMPORARY_FAILURE);
    milter_assert_convert_status_from(SMFIS_NOREPLY,
                                      MILTER_STATUS_NO_REPLY);
    milter_assert_convert_status_from(SMFIS_SKIP,
                                      MILTER_STATUS_SKIP);
    milter_assert_convert_status_from(SMFIS_ALL_OPTS,
                                      MILTER_STATUS_ALL_OPTIONS);

#undef milter_assert_convert_status_from
}

void
test_mi_stop (void)
{
    int mi_stop(void);
    cut_assert_equal_int(0, mi_stop());
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
