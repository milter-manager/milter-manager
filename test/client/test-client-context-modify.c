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

#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <arpa/inet.h>

#include <milter/client.h>
#include <milter-test-utils.h>

#include <gcutter.h>

void data_add_header (void);
void test_add_header (gconstpointer _data);

static MilterClientContext *context;

static GIOChannel *channel;

void
setup (void)
{
    MilterWriter *writer;

    context = milter_client_context_new();

    channel = gcut_string_io_channel_new(NULL);
    g_io_channel_set_encoding(channel, NULL, NULL);
    writer = milter_writer_io_channel_new(channel);
    milter_agent_set_writer(MILTER_AGENT(context), writer);
    g_object_unref(writer);
}

void
teardown (void)
{
    if (context)
        g_object_unref(context);

    if (channel)
        g_io_channel_unref(channel);
}

void
data_add_header (void)
{
    MilterReplyEncoder *reply_encoder;

    reply_encoder = MILTER_REPLY_ENCODER(milter_reply_encoder_new());

#define ADD(label, name, value, state, action_names, error) do          \
    {                                                                   \
        MilterActionFlags action;                                       \
        GError *parse_error = NULL;                                     \
        gchar *packet = NULL;                                           \
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
        if (packet)                                                     \
            g_free(packet);                                             \
    } while (0)

    ADD("success",
        "X-Name", "value", END_OF_MESSAGE, "add-headers", NULL);
    ADD("NULL name",
        NULL, "value", END_OF_MESSAGE, "add-headers",
        g_error_new(MILTER_CLIENT_CONTEXT_ERROR,
                    MILTER_CLIENT_CONTEXT_ERROR_NULL,
                    "both header name and value should not be NULL: "
                    "<NULL>=<value>"));
    ADD("NULL value",
        "X-Name", NULL, END_OF_MESSAGE, "add-headers",
        g_error_new(MILTER_CLIENT_CONTEXT_ERROR,
                    MILTER_CLIENT_CONTEXT_ERROR_NULL,
                    "both header name and value should not be NULL: "
                    "<X-Name>=<NULL>"));
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

    g_object_unref(reply_encoder);
}

void
test_add_header (gconstpointer _data)
{
    const GCutData *data = _data;
    MilterOption *option;
    const GError *expected_error;
    gboolean success;
    GError *actual_error = NULL;

    milter_client_context_set_state(context,
                                    gcut_data_get_enum(data, "state"));
    option = milter_option_new(2,
                               gcut_data_get_flags(data, "action"),
                               0);
    milter_client_context_set_option(context, option);
    g_object_unref(option);

    success =
        milter_client_context_add_header(context,
                                         gcut_data_get_string(data, "name"),
                                         gcut_data_get_string(data, "value"),
                                         &actual_error);
    expected_error = gcut_data_get_pointer(data, "error");
    if (expected_error) {
        gcut_take_error(actual_error);
        gcut_assert_equal_error(expected_error, actual_error);
        cut_assert_false(success);
    } else {
        GString *actual_data;

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
