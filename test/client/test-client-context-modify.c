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

#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <arpa/inet.h>

#include <milter/client.h>
#include <milter-test-utils.h>

#include <gcutter.h>

void data_add_header (void);
void test_add_header (gconstpointer data);
void data_insert_header (void);
void test_insert_header (gconstpointer data);
void data_change_header (void);
void test_change_header (gconstpointer data);
void data_delete_header (void);
void test_delete_header (gconstpointer data);

static MilterEventLoop *loop;

static MilterReplyEncoder *reply_encoder;

static MilterClientContext *context;

static GIOChannel *channel;

void
cut_startup (void)
{
    reply_encoder = MILTER_REPLY_ENCODER(milter_reply_encoder_new());
}

void
cut_shutdown (void)
{
    g_object_unref(reply_encoder);
}

void
cut_setup (void)
{
    MilterWriter *writer;
    GError *error = NULL;

    loop = milter_test_event_loop_new();

    context = milter_client_context_new(NULL);
    milter_agent_set_event_loop(MILTER_AGENT(context), loop);

    channel = gcut_string_io_channel_new(NULL);
    g_io_channel_set_encoding(channel, NULL, NULL);
    writer = milter_writer_io_channel_new(channel);
    milter_agent_set_writer(MILTER_AGENT(context), writer);
    milter_agent_start(MILTER_AGENT(context), &error);
    g_object_unref(writer);
    gcut_assert_error(error);
}

void
cut_teardown (void)
{
    if (context)
        g_object_unref(context);

    if (channel)
        g_io_channel_unref(channel);

    if (loop)
        g_object_unref(loop);
}

static void
set_option (gint version, MilterActionFlags action, MilterStepFlags step)
{
    MilterOption *option;

    option = milter_option_new(version, action, step);
    milter_client_context_set_option(context, option);
    g_object_unref(option);
}

void
data_add_header (void)
{
#define ADD(label, name, value, state, action_names, error) do          \
    {                                                                   \
        MilterActionFlags action;                                       \
        GError *parse_error = NULL;                                     \
        const gchar *packet = NULL;                                     \
        gsize packet_size = 0;                                          \
                                                                        \
        action = gcut_flags_parse(MILTER_TYPE_ACTION_FLAGS,             \
                                  action_names, &parse_error);          \
        gcut_assert_error(parse_error);                                 \
                                                                        \
        if (!error) {                                                   \
            milter_reply_encoder_encode_add_header(reply_encoder,       \
                                                   &packet,             \
                                                   &packet_size,        \
                                                   name, value);        \
        }                                                               \
                                                                        \
        gcut_add_datum(label,                                           \
                       "name", G_TYPE_STRING, name,                     \
                       "value", G_TYPE_STRING, value,                   \
                       "state", MILTER_TYPE_CLIENT_CONTEXT_STATE,       \
                       MILTER_CLIENT_CONTEXT_STATE_ ## state,           \
                       "action", MILTER_TYPE_ACTION_FLAGS, action,      \
                       "error", G_TYPE_POINTER, error, g_error_free,    \
                       "packet", G_TYPE_POINTER,                        \
                       g_memdup(packet, packet_size), g_free,           \
                       "packet-size", G_TYPE_UINT, packet_size,         \
                       NULL);                                           \
    } while (0)

    ADD("success",
        "X-Name", "value", END_OF_MESSAGE, "add-headers", NULL);
    ADD("NULL name",
        NULL, "value", END_OF_MESSAGE, "add-headers",
        g_error_new(MILTER_CLIENT_CONTEXT_ERROR,
                    MILTER_CLIENT_CONTEXT_ERROR_NULL,
                    "both header name and value should not be NULL: "
                    "<(null)>=<value>"));
    ADD("NULL value",
        "X-Name", NULL, END_OF_MESSAGE, "add-headers",
        g_error_new(MILTER_CLIENT_CONTEXT_ERROR,
                    MILTER_CLIENT_CONTEXT_ERROR_NULL,
                    "both header name and value should not be NULL: "
                    "<X-Name>=<(null)>"));
    ADD("invalid state",
        "X-Name", "value", BODY, "add-headers",
        g_error_new(MILTER_CLIENT_CONTEXT_ERROR,
                    MILTER_CLIENT_CONTEXT_ERROR_INVALID_STATE,
                    "add-header can only be done on end-of-message state: "
                    "<body>"));
    ADD("invalid action",
        "X-Name", "value", END_OF_MESSAGE, "",
        g_error_new(MILTER_CLIENT_CONTEXT_ERROR,
                    MILTER_CLIENT_CONTEXT_ERROR_INVALID_ACTION,
                    "add-header can only be done with "
                    "<add-headers> action flags: <>"));
#undef ADD
}

void
test_add_header (gconstpointer data)
{
    const GError *expected_error;
    gboolean success;
    GError *actual_error = NULL;

    milter_client_context_set_state(context,
                                    gcut_data_get_enum(data, "state"));
    set_option(2, gcut_data_get_flags(data, "action"), 0);

    success =
        milter_client_context_add_header(context,
                                         gcut_data_get_string(data, "name"),
                                         gcut_data_get_string(data, "value"),
                                         &actual_error);
    if (MILTER_IS_LIBEV_EVENT_LOOP(loop))
        cut_omit("MilterLibevEventLoop doesn't support GCutStringIOChannel.");
    milter_test_pump_all_events(loop);

    expected_error = gcut_data_get_pointer(data, "error");
    if (expected_error) {
        gcut_take_error(actual_error);
        gcut_assert_equal_error(expected_error, actual_error);
        cut_assert_false(success);
    } else {
        GString *actual_data;

        gcut_assert_error(actual_error);

        milter_agent_flush(MILTER_AGENT(context), &actual_error);
        gcut_assert_error(actual_error);

        actual_data = gcut_string_io_channel_get_string(channel);
        cut_assert_equal_memory(gcut_data_get_pointer(data, "packet"),
                                gcut_data_get_uint(data, "packet-size"),
                                actual_data->str,
                                actual_data->len);
        cut_assert_true(success);
    }
}

void
data_insert_header (void)
{
#define ADD(label, index, name, value, state, action_names, error) do   \
    {                                                                   \
        MilterActionFlags action;                                       \
        GError *parse_error = NULL;                                     \
        const gchar *packet = NULL;                                     \
        gsize packet_size = 0;                                          \
                                                                        \
        action = gcut_flags_parse(MILTER_TYPE_ACTION_FLAGS,             \
                                  action_names, &parse_error);          \
        gcut_assert_error(parse_error);                                 \
                                                                        \
        if (!error) {                                                   \
            milter_reply_encoder_encode_insert_header(reply_encoder,    \
                                                      &packet,          \
                                                      &packet_size,     \
                                                      index,            \
                                                      name, value);     \
        }                                                               \
                                                                        \
        gcut_add_datum(label,                                           \
                       "index", G_TYPE_UINT, index,                     \
                       "name", G_TYPE_STRING, name,                     \
                       "value", G_TYPE_STRING, value,                   \
                       "state", MILTER_TYPE_CLIENT_CONTEXT_STATE,       \
                       MILTER_CLIENT_CONTEXT_STATE_ ## state,           \
                       "action", MILTER_TYPE_ACTION_FLAGS, action,      \
                       "error", G_TYPE_POINTER, error, g_error_free,    \
                       "packet", G_TYPE_POINTER,                        \
                       g_memdup(packet, packet_size), g_free,           \
                       "packet-size", G_TYPE_UINT, packet_size,         \
                       NULL);                                           \
    } while (0)

    ADD("success",
        0, "X-Name", "value", END_OF_MESSAGE, "add-headers", NULL);
    ADD("NULL name",
        0, NULL, "value", END_OF_MESSAGE, "add-headers",
        g_error_new(MILTER_CLIENT_CONTEXT_ERROR,
                    MILTER_CLIENT_CONTEXT_ERROR_NULL,
                    "both header name and value should not be NULL: "
                    "<(null)>=<value>"));
    ADD("NULL value",
        0, "X-Name", NULL, END_OF_MESSAGE, "add-headers",
        g_error_new(MILTER_CLIENT_CONTEXT_ERROR,
                    MILTER_CLIENT_CONTEXT_ERROR_NULL,
                    "both header name and value should not be NULL: "
                    "<X-Name>=<(null)>"));
    ADD("invalid state",
        0, "X-Name", "value", BODY, "add-headers",
        g_error_new(MILTER_CLIENT_CONTEXT_ERROR,
                    MILTER_CLIENT_CONTEXT_ERROR_INVALID_STATE,
                    "insert-header can only be done on end-of-message state: "
                    "<body>"));
    ADD("invalid action",
        0, "X-Name", "value", END_OF_MESSAGE, "",
        g_error_new(MILTER_CLIENT_CONTEXT_ERROR,
                    MILTER_CLIENT_CONTEXT_ERROR_INVALID_ACTION,
                    "insert-header can only be done with "
                    "<add-headers> action flags: <>"));
#undef ADD
}

void
test_insert_header (gconstpointer data)
{
    const GError *expected_error;
    gboolean success;
    GError *actual_error = NULL;

    milter_client_context_set_state(context,
                                    gcut_data_get_enum(data, "state"));
    set_option(2, gcut_data_get_flags(data, "action"), 0);

    success =
        milter_client_context_insert_header(context,
                                            gcut_data_get_uint(data, "index"),
                                            gcut_data_get_string(data, "name"),
                                            gcut_data_get_string(data, "value"),
                                            &actual_error);
    if (MILTER_IS_LIBEV_EVENT_LOOP(loop))
        cut_omit("MilterLibevEventLoop doesn't support GCutStringIOChannel.");
    milter_test_pump_all_events(loop);

    expected_error = gcut_data_get_pointer(data, "error");
    if (expected_error) {
        gcut_take_error(actual_error);
        gcut_assert_equal_error(expected_error, actual_error);
        cut_assert_false(success);
    } else {
        GString *actual_data;

        gcut_assert_error(actual_error);

        milter_agent_flush(MILTER_AGENT(context), &actual_error);
        gcut_assert_error(actual_error);

        actual_data = gcut_string_io_channel_get_string(channel);
        cut_assert_equal_memory(gcut_data_get_pointer(data, "packet"),
                                gcut_data_get_uint(data, "packet-size"),
                                actual_data->str,
                                actual_data->len);
        cut_assert_true(success);
    }
}

void
data_change_header (void)
{
#define ADD(label, name, index, value, state, action_names, error) do   \
    {                                                                   \
        MilterActionFlags action;                                       \
        GError *parse_error = NULL;                                     \
        const gchar *packet = NULL;                                     \
        gsize packet_size = 0;                                          \
                                                                        \
        action = gcut_flags_parse(MILTER_TYPE_ACTION_FLAGS,             \
                                  action_names, &parse_error);          \
        gcut_assert_error(parse_error);                                 \
                                                                        \
        if (!error) {                                                   \
            milter_reply_encoder_encode_change_header(reply_encoder,    \
                                                      &packet,          \
                                                      &packet_size,     \
                                                      name, index,      \
                                                      value);           \
        }                                                               \
                                                                        \
        gcut_add_datum(label,                                           \
                       "name", G_TYPE_STRING, name,                     \
                       "index", G_TYPE_UINT, index,                     \
                       "value", G_TYPE_STRING, value,                   \
                       "state", MILTER_TYPE_CLIENT_CONTEXT_STATE,       \
                       MILTER_CLIENT_CONTEXT_STATE_ ## state,           \
                       "action", MILTER_TYPE_ACTION_FLAGS, action,      \
                       "error", G_TYPE_POINTER, error, g_error_free,    \
                       "packet", G_TYPE_POINTER,                        \
                       g_memdup(packet, packet_size), g_free,           \
                       "packet-size", G_TYPE_UINT, packet_size,         \
                       NULL);                                           \
    } while (0)

    ADD("success",
        "X-Name", 2, "value", END_OF_MESSAGE, "change-headers", NULL);
    ADD("NULL name",
        NULL, 2, "value", END_OF_MESSAGE, "change-headers",
        g_error_new(MILTER_CLIENT_CONTEXT_ERROR,
                    MILTER_CLIENT_CONTEXT_ERROR_NULL,
                    "header name should not be NULL: value=<value>"));
    ADD("NULL value",
        "X-Name", 2, NULL, END_OF_MESSAGE, "change-headers", NULL);
    ADD("invalid state",
        "X-Name", 0, "value", BODY, "change-headers",
        g_error_new(MILTER_CLIENT_CONTEXT_ERROR,
                    MILTER_CLIENT_CONTEXT_ERROR_INVALID_STATE,
                    "change-header can only be done on end-of-message state: "
                    "<body>"));
    ADD("invalid action",
        "X-Name", 0, "value", END_OF_MESSAGE, "",
        g_error_new(MILTER_CLIENT_CONTEXT_ERROR,
                    MILTER_CLIENT_CONTEXT_ERROR_INVALID_ACTION,
                    "change-header can only be done with "
                    "<change-headers> action flags: <>"));
#undef ADD
}

void
test_change_header (gconstpointer data)
{
    const GError *expected_error;
    gboolean success;
    GError *actual_error = NULL;

    milter_client_context_set_state(context,
                                    gcut_data_get_enum(data, "state"));
    set_option(2, gcut_data_get_flags(data, "action"), 0);

    success =
        milter_client_context_change_header(context,
                                            gcut_data_get_string(data, "name"),
                                            gcut_data_get_uint(data, "index"),
                                            gcut_data_get_string(data, "value"),
                                            &actual_error);
    if (MILTER_IS_LIBEV_EVENT_LOOP(loop))
        cut_omit("MilterLibevEventLoop doesn't support GCutStringIOChannel.");
    milter_test_pump_all_events(loop);

    expected_error = gcut_data_get_pointer(data, "error");
    if (expected_error) {
        gcut_take_error(actual_error);
        gcut_assert_equal_error(expected_error, actual_error);
        cut_assert_false(success);
    } else {
        GString *actual_data;

        gcut_assert_error(actual_error);

        milter_agent_flush(MILTER_AGENT(context), &actual_error);
        gcut_assert_error(actual_error);

        actual_data = gcut_string_io_channel_get_string(channel);
        cut_assert_equal_memory(gcut_data_get_pointer(data, "packet"),
                                gcut_data_get_uint(data, "packet-size"),
                                actual_data->str,
                                actual_data->len);
        cut_assert_true(success);
    }
}

void
data_delete_header (void)
{
#define ADD(label, name, index, state, action_names, error) do          \
    {                                                                   \
        MilterActionFlags action;                                       \
        GError *parse_error = NULL;                                     \
        const gchar *packet = NULL;                                     \
        gsize packet_size = 0;                                          \
                                                                        \
        action = gcut_flags_parse(MILTER_TYPE_ACTION_FLAGS,             \
                                  action_names, &parse_error);          \
        gcut_assert_error(parse_error);                                 \
                                                                        \
        if (!error) {                                                   \
            milter_reply_encoder_encode_delete_header(reply_encoder,    \
                                                      &packet,          \
                                                      &packet_size,     \
                                                      name, index);     \
        }                                                               \
                                                                        \
        gcut_add_datum(label,                                           \
                       "name", G_TYPE_STRING, name,                     \
                       "index", G_TYPE_UINT, index,                     \
                       "state", MILTER_TYPE_CLIENT_CONTEXT_STATE,       \
                       MILTER_CLIENT_CONTEXT_STATE_ ## state,           \
                       "action", MILTER_TYPE_ACTION_FLAGS, action,      \
                       "error", G_TYPE_POINTER, error, g_error_free,    \
                       "packet", G_TYPE_POINTER,                        \
                       g_memdup(packet, packet_size), g_free,           \
                       "packet-size", G_TYPE_UINT, packet_size,         \
                       NULL);                                           \
    } while (0)

    ADD("success",
        "X-Name", 2, END_OF_MESSAGE, "change-headers", NULL);
    ADD("NULL name",
        NULL, 2, END_OF_MESSAGE, "change-headers",
        g_error_new(MILTER_CLIENT_CONTEXT_ERROR,
                    MILTER_CLIENT_CONTEXT_ERROR_NULL,
                    "header name should not be NULL: index=<2>"));
    ADD("invalid state",
        "X-Name", 2, BODY, "change-headers",
        g_error_new(MILTER_CLIENT_CONTEXT_ERROR,
                    MILTER_CLIENT_CONTEXT_ERROR_INVALID_STATE,
                    "delete-header can only be done on end-of-message state: "
                    "<body>"));
    ADD("invalid action",
        "X-Name", 2, END_OF_MESSAGE, "",
        g_error_new(MILTER_CLIENT_CONTEXT_ERROR,
                    MILTER_CLIENT_CONTEXT_ERROR_INVALID_ACTION,
                    "delete-header can only be done with "
                    "<change-headers> action flags: <>"));
#undef ADD
}

void
test_delete_header (gconstpointer data)
{
    const GError *expected_error;
    gboolean success;
    GError *actual_error = NULL;

    milter_client_context_set_state(context,
                                    gcut_data_get_enum(data, "state"));
    set_option(2, gcut_data_get_flags(data, "action"), 0);

    success =
        milter_client_context_delete_header(context,
                                            gcut_data_get_string(data, "name"),
                                            gcut_data_get_uint(data, "index"),
                                            &actual_error);
    if (MILTER_IS_LIBEV_EVENT_LOOP(loop))
        cut_omit("MilterLibevEventLoop doesn't support GCutStringIOChannel.");
    milter_test_pump_all_events(loop);

    expected_error = gcut_data_get_pointer(data, "error");
    if (expected_error) {
        gcut_take_error(actual_error);
        gcut_assert_equal_error(expected_error, actual_error);
        cut_assert_false(success);
    } else {
        GString *actual_data;

        gcut_assert_error(actual_error);

        milter_agent_flush(MILTER_AGENT(context), &actual_error);
        gcut_assert_error(actual_error);

        actual_data = gcut_string_io_channel_get_string(channel);
        cut_assert_equal_memory(gcut_data_get_pointer(data, "packet"),
                                gcut_data_get_uint(data, "packet-size"),
                                actual_data->str,
                                actual_data->len);
        cut_assert_true(success);
    }
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
