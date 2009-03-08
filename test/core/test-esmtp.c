/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Copyright (C) 2009  Kouhei Sutou <kou@cozmixng.org>
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

#include <milter/core/milter-esmtp.h>
#include <milter-test-utils.h>

#include <gcutter.h>

#if defined(CUTTER_CHECK_VERSION)
#  if CUTTER_CHECK_VERSION(1, 0, 7)

void data_parse_mail_from_argument (void);
void data_parse_rcpt_to_argument (void);
void test_parse_mail_from_argument (gconstpointer data);
void test_parse_rcpt_to_argument (gconstpointer data);

static gchar *actual_path;
static GHashTable *actual_parameters;

static GError *expected_error;
static GError *actual_error;

void
setup (void)
{
    actual_path = NULL;
    actual_parameters = NULL;

    expected_error = NULL;
    actual_error = NULL;
}

void
teardown (void)
{
    if (actual_path)
        g_free(actual_path);
    if (actual_parameters)
        g_hash_table_unref(actual_parameters);

    if (expected_error)
        g_error_free(expected_error);
    if (actual_error)
        g_error_free(actual_error);
}

#define ADD_DATUM(label, argument, error, path, parameters)     \
    gcut_add_datum(label,                                       \
                   "argument", G_TYPE_STRING, argument,         \
                   "error", GCUT_TYPE_ERROR, error,             \
                   "path", G_TYPE_STRING, path,                 \
                   "parameters", G_TYPE_HASH_TABLE, parameters, \
                   NULL)
#define ERROR(...)                                      \
    g_error_new(MILTER_ESMTP_ERROR,                     \
                MILTER_ESMTP_ERROR_INVALID_FORMAT,      \
                __VA_ARGS__)

static void
data_parse_envelope_command_argument (void)
{
    ADD_DATUM("NULL argument", NULL, ERROR("argument should not be NULL"),
              NULL, NULL);
    ADD_DATUM("not started with '<'", " <user@example.com>",
              ERROR("argument should start with '<': <|@| <user@example.com>>"),
              NULL, NULL);

    ADD_DATUM("domain: not started with alphabet nor digit",
              "<@.example.org:user@example.com>",
              ERROR("domain should start with alphabet or digit: "
                    "<<@|@|.example.org:user@example.com>>"),
              NULL, NULL);
    ADD_DATUM("domain: invalid quoted character",
              "<@[192.168\\\n.1.1]:user@example.com>",
              ERROR("invalid quoted character in domain: "
                    "<<@[192.168\\|@|\n.1.1]:user@example.com>>"),
              NULL, NULL);
    ADD_DATUM("domain: terminated ']' is missing",
              "<@[192.168.1.1 :user@example.com>",
              ERROR("terminate ']' is missing in domain: "
                    "<<@[192.168.1.1|@| :user@example.com>>"),
              NULL, NULL);

    ADD_DATUM("source route: no ':'",
              "<@example.org user@example.com>",
              ERROR("separator ':' is missing after source route: "
                    "<<@example.org|@| user@example.com>>"),
              NULL, NULL);

    ADD_DATUM("local part: invalid quoted character",
              "<\"us\\\ner\"@example.com>",
              ERROR("invalid quoted character in local part: "
                    "<<\"us\\|@|\ner\"@example.com>>"),
              NULL, NULL);
    ADD_DATUM("local part: end quote is missing",
              "<\"user @example.com>",
              ERROR("end quote for local part is missing: "
                    "<<\"user|@| @example.com>>"),
              NULL, NULL);

    ADD_DATUM("mailbox: '@' is missing",
              "<user example.com>",
              ERROR("'@' is missing in mailbox: "
                    "<<user|@| example.com>>"),
              NULL, NULL);
    ADD_DATUM("mailbox: domain is missing",
              "<user@",
              ERROR("domain is missing in mailbox: "
                    "<<user@|@|>"),
              NULL, NULL);
    ADD_DATUM("mailbox: domain is missing",
              "<user@>",
              ERROR("domain is missing in mailbox: "
                    "<<user@|@|>>"),
              NULL, NULL);
    ADD_DATUM("mailbox: domain: not started with alphabet nor digit",
              "<user@.example.com>",
              ERROR("domain should start with alphabet or digit: "
                    "<<user@|@|.example.com>>"),
              NULL, NULL);

    ADD_DATUM("path: terminate '>' is missing",
              "<user@example.com",
              ERROR("terminate '>' is missing in path: "
                    "<<user@example.com|@|>"),
              NULL, NULL);
    ADD_DATUM("path: garbage at the last",
              "<user@example.com>x",
              ERROR("there is a garbage at the last: "
                    "<<user@example.com>|@|x>"),
              NULL, NULL);

    ADD_DATUM("parameter: keyword is missing",
              "<user@example.com> ",
              ERROR("parameter keyword is missing: "
                    "<<user@example.com> |@|>"),
              NULL, NULL);
    ADD_DATUM("parameter: keyword is started without alphabet nor digit",
              "<user@example.com> .",
              ERROR("parameter keyword should start with alphabet or digit: "
                    "<<user@example.com> |@|.>"),
              NULL, NULL);

    ADD_DATUM("null reverse-path", "<>", NULL, NULL, NULL);
    ADD_DATUM("reverse-path only", "<user@example.com>",
              NULL, "<user@example.com>", NULL);
    ADD_DATUM("with dot", "<first.family@example.com>",
              NULL, "<first.family@example.com>", NULL);
    ADD_DATUM("quoted local part", "<\"u<se>r(s)+\\\\x\\ y\"@example.com>",
              NULL, "<\"u<se>r(s)+\\\\x\\ y\"@example.com>", NULL);
    ADD_DATUM("IPv4 address", "<user@[192.168.1.1]>",
              NULL, "<user@[192.168.1.1]>", NULL);
    ADD_DATUM("IPv6 address", "<user@[IPv6:::1]>",
              NULL, "<user@[IPv6:::1]>", NULL);
    ADD_DATUM("full IPv6 address", "<user@[IPv6:ff02:0:0:0:0:0:0:1]>",
              NULL, "<user@[IPv6:ff02:0:0:0:0:0:0:1]>", NULL);
    ADD_DATUM("general address", "<user@[tag:address\\]with-escape]>",
              NULL, "<user@[tag:address\\]with-escape]>", NULL);

    ADD_DATUM("single source route", "<@example.com:user@example.org>",
              NULL, "<@example.com:user@example.org>", NULL);
    ADD_DATUM("multi source route",
              "<@example.com,@example.net:user@example.org>",
              NULL, "<@example.com,@example.net:user@example.org>", NULL);

    ADD_DATUM("single parameter",
              "<user@example.org> SOLICIT=org.example:ADV:ADLT",
              NULL,
              "<user@example.org>",
              gcut_hash_table_string_string_new("SOLICIT",
                                                "org.example:ADV:ADLT",
                                                NULL));
    ADD_DATUM("multi parameters",
              "<user@example.org> RET=HDRS ENVID=QQ314159",
              NULL,
              "<user@example.org>",
              gcut_hash_table_string_string_new("RET", "HDRS",
                                                "ENVID", "QQ314159",
                                                NULL));
    ADD_DATUM("keyword only",
              "<user@example.org> KEY-WORD",
              NULL,
              "<user@example.org>",
              gcut_hash_table_string_string_new("KEY-WORD", NULL,
                                                NULL));
}

void
data_parse_mail_from_argument (void)
{
    data_parse_envelope_command_argument();
    ADD_DATUM("postmaster path",
              "<Postmaster>",
              ERROR("'@' is missing in mailbox: <<Postmaster|@|>>"),
              NULL, NULL);
}

void
data_parse_rcpt_to_argument (void)
{
    data_parse_envelope_command_argument();
    ADD_DATUM("postmaster path",
              "<Postmaster>",
              NULL,
              "<Postmaster>",
              NULL);
}
#undef ERROR
#undef ADD_DATUM

static void
assert_parse_envelope_command_argument (gconstpointer data, gboolean success)
{
    const gchar *path;
    GError *error;
    GHashTable *parameters;

    error = (GError *)gcut_data_get_boxed(data, "error");
    path = gcut_data_get_string(data, "path");
    parameters = (GHashTable *)gcut_data_get_boxed(data, "parameters");

    gcut_assert_equal_error(error, actual_error);
    if (error)
        cut_assert_false(success);
    else
        cut_assert_true(success);
    cut_assert_equal_string(path, actual_path);
    gcut_assert_equal_hash_table_string_string(parameters, actual_parameters);
}

void
test_parse_mail_from_argument (gconstpointer data)
{
    gboolean success;
    const gchar *argument;

    argument = gcut_data_get_string(data, "argument");
    success = milter_esmtp_parse_mail_from_argument(argument,
                                                    &actual_path,
                                                    &actual_parameters,
                                                    &actual_error);
    cut_trace(assert_parse_envelope_command_argument(data, success));
}

void
test_parse_rcpt_to_argument (gconstpointer data)
{
    gboolean success;
    const gchar *argument;

    argument = gcut_data_get_string(data, "argument");
    success = milter_esmtp_parse_rcpt_to_argument(argument,
                                                  &actual_path,
                                                  &actual_parameters,
                                                  &actual_error);
    cut_trace(assert_parse_envelope_command_argument(data, success));
}

#  endif
#endif

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
