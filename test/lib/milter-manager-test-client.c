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

#include <string.h>

#include <gcutter.h>

#include "milter-test-utils.h"
#include "milter-manager-test-utils.h"
#include "milter-manager-test-client.h"

#define MILTER_MANAGER_TEST_CLIENT_GET_PRIVATE(obj)                     \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj),                                 \
                                 MILTER_TYPE_MANAGER_TEST_CLIENT,       \
                                 MilterManagerTestClientPrivate))

typedef struct _MilterManagerTestClientPrivate MilterManagerTestClientPrivate;
struct _MilterManagerTestClientPrivate
{
    GCutProcess *process;
    guint port;
    gchar *name;
    MilterEventLoop *loop;
    GArray *command;
    GString *output_received;

    gboolean ready;
    gboolean reaped;

    guint n_negotiate_received;
    guint n_define_macro_received;
    guint n_connect_received;
    guint n_helo_received;
    guint n_envelope_from_received;
    guint n_envelope_recipient_received;
    guint n_data_received;
    guint n_header_received;
    guint n_end_of_header_received;
    guint n_body_received;
    guint n_end_of_message_received;
    guint n_quit_received;
    guint n_abort_received;
    guint n_unknown_received;

    MilterOption *negotiate_option;
    GHashTable *macros;
    gchar *connect_host;
    gchar *connect_address;
    gchar *helo_fqdn;
    gchar *envelope_from;
    gchar *envelope_recipient;
    gchar *header_name;
    gchar *header_value;
    GString *body_chunk;
    gchar *end_of_message_chunk;
    gchar *unknown_command;

    gboolean have_output;
};

G_DEFINE_TYPE(MilterManagerTestClient, milter_manager_test_client, G_TYPE_OBJECT)

static void dispose        (GObject         *object);

static void
milter_manager_test_client_class_init (MilterManagerTestClientClass *klass)
{
    GObjectClass *gobject_class;

    gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->dispose      = dispose;

    g_type_class_add_private(gobject_class,
                             sizeof(MilterManagerTestClientPrivate));
}

static void
milter_manager_test_client_init (MilterManagerTestClient *test_client)
{
    MilterManagerTestClientPrivate *priv;

    priv = MILTER_MANAGER_TEST_CLIENT_GET_PRIVATE(test_client);

    priv->process = NULL;
    priv->port = 0;
    priv->name = NULL;
    priv->command = NULL;
    priv->output_received = g_string_new(NULL);

    priv->ready = FALSE;
    priv->reaped = FALSE;

    priv->negotiate_option = NULL;
    priv->macros = NULL;
    priv->connect_host = NULL;
    priv->connect_address = NULL;
    priv->helo_fqdn = NULL;
    priv->envelope_from = NULL;
    priv->envelope_recipient = NULL;
    priv->header_name = NULL;
    priv->header_value = NULL;
    priv->body_chunk = g_string_new(NULL);
    priv->end_of_message_chunk = NULL;
    priv->unknown_command = NULL;

    priv->have_output = TRUE;

    milter_manager_test_client_clear_data(test_client);
}

static gchar *
receive_additional_info (const gchar *chunk, gsize size, const gchar *prefix)
{
    gsize info_start_position;

    info_start_position = strlen(prefix);
    if (info_start_position < size)
        return g_strndup(chunk + info_start_position,
                         size - info_start_position - 1);
    else
        return NULL;
}

static void
parse_negotiate_info (MilterManagerTestClientPrivate *priv,
                      const gchar *chunk, gsize size)
{
    gchar **items;

    items = g_strsplit_set(chunk, " \n", -1);
    if (items[0] && items[1] && items[2] && items[3] && items[4]) {
        guint32 version;
        MilterActionFlags action;
        MilterStepFlags step;
        GError *error = NULL;

        if (priv->negotiate_option) {
            g_object_unref(priv->negotiate_option);
            priv->negotiate_option = NULL;
        }

        version = atoi(items[2]);
        action = gcut_flags_parse(MILTER_TYPE_ACTION_FLAGS,
                                  items[3],
                                  &error);
        if (!error)
            step = gcut_flags_parse(MILTER_TYPE_STEP_FLAGS,
                                    items[4],
                                    &error);
        if (error) {
            gchar *inspected_error;

            inspected_error = gcut_error_inspect(error);
            g_error_free(error);
            g_print("failed to parse received negotiated option: %s\n",
                    inspected_error);
            g_free(inspected_error);
        } else {
            priv->negotiate_option =
                milter_option_new(version, action, step);
        }
    }
    g_strfreev(items);
}

static void
parse_define_macro_info (MilterManagerTestClientPrivate *priv,
                         const gchar *chunk, gsize size)
{
    gchar *info;
    gchar **items;
    MilterCommand context;
    GError *error = NULL;

    info = receive_additional_info(chunk, size, "receive: define-macro: ");
    if (!info)
        return;

    items = g_strsplit_set(info, "\n", -1);
    if (items[0])
        context = gcut_enum_parse(MILTER_TYPE_COMMAND, items[0], &error);

    if (items[0] && !error) {
        gchar **macro;
        GHashTable *macros;

        macros = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
        for (macro = items + 1; *macro; macro++) {
            gchar **name_and_value;

            name_and_value = g_strsplit(*macro, ": ", 2);
            g_hash_table_insert(macros,
                                g_strdup(name_and_value[0]),
                                g_strdup(name_and_value[1]));
            g_strfreev(name_and_value);
        }
        g_hash_table_insert(priv->macros, GUINT_TO_POINTER(context), macros);
    }
    g_strfreev(items);
    g_free(info);

    gcut_assert_error(error);
}

static void
parse_connect_info (MilterManagerTestClientPrivate *priv,
                    const gchar *chunk, gsize size)
{
    gchar **items;

    items = g_strsplit_set(chunk, " \n", -1);
    if (items[0] && items[1] && items[2] && items[3]) {
        if (priv->connect_host)
            g_free(priv->connect_host);
        if (priv->connect_address)
            g_free(priv->connect_address);
        priv->connect_host = g_strdup(items[2]);
        priv->connect_address = g_strdup(items[3]);
    }
    g_strfreev(items);
}

static void
parse_helo_info (MilterManagerTestClientPrivate *priv,
                 const gchar *chunk, gsize size)
{
    if (priv->helo_fqdn)
        g_free(priv->helo_fqdn);
    priv->helo_fqdn = receive_additional_info(chunk, size, "receive: helo: ");
}

static void
parse_envelope_from_info (MilterManagerTestClientPrivate *priv,
                          const gchar *chunk, gsize size)
{
    if (priv->envelope_from)
        g_free(priv->envelope_from);
    priv->envelope_from =
        receive_additional_info(chunk, size, "receive: envelope-from: ");
}

static void
parse_envelope_recipient_info (MilterManagerTestClientPrivate *priv,
                               const gchar *chunk, gsize size)
{
    if (priv->envelope_recipient)
        g_free(priv->envelope_recipient);
    priv->envelope_recipient =
        receive_additional_info(chunk, size, "receive: envelope-recipient: ");
}

static void
parse_header_info (MilterManagerTestClientPrivate *priv,
                   const gchar *chunk, gsize size)
{
    gchar **items;

    items = g_strsplit_set(chunk, " ", 3);
    if (items[0] && items[1] && items[2]) {
        gchar **header_info;

        header_info = g_strsplit(items[2], " ", 2);
        if (header_info[0] && header_info[1]) {
            if (priv->header_name)
                g_free(priv->header_name);
            if (priv->header_value)
                g_free(priv->header_value);
            priv->header_name = g_strdup(header_info[0]);
            priv->header_value =
                g_strndup(header_info[1],
                          strstr(header_info[1], "\n") - header_info[1]);
        }
        g_strfreev(header_info);
    }
    g_strfreev(items);
}

static void
parse_body_info (MilterManagerTestClientPrivate *priv,
                 const gchar *chunk, gsize size)
{
    gchar *body;

    if (!priv->body_chunk)
        priv->body_chunk = g_string_new(NULL);
    body = receive_additional_info(chunk, size, "receive: body: ");
    g_string_append(priv->body_chunk, body);
    g_free(body);
}

static void
parse_end_of_message_info (MilterManagerTestClientPrivate *priv,
                           const gchar *chunk, gsize size)
{
    if (priv->end_of_message_chunk)
        g_free(priv->end_of_message_chunk);
    priv->end_of_message_chunk =
        receive_additional_info(chunk, size, "receive: end-of-message: ");
}

static void
parse_unknown_info (MilterManagerTestClientPrivate *priv,
                    const gchar *chunk, gsize size)
{
    if (priv->unknown_command)
        g_free(priv->unknown_command);
    priv->unknown_command =
        receive_additional_info(chunk, size, "receive: unknown: ");
}

typedef struct _reponse_data
{
    gchar *string;
    gsize size;
} ResponseData;

static ResponseData *
response_data_new (const gchar *string, gsize size)
{
    ResponseData *response;

    response = g_new0(ResponseData, 1);
    response->string = g_memdup(string, size);
    response->size = size;

    return response;
}

static void
free_response_data (ResponseData *data)
{
    g_free(data->string);
    g_free(data);
}

static GList *
split_response (const gchar *response, gsize size)
{
    GList *list = NULL;

    while (size > 0) {
        const gchar *found_point;
        ResponseData *response_data;
        gsize length;

        found_point = g_strstr_len(response, size, "receive: ");
        if (!found_point) {
            length = size;
        } else if (found_point != response) {
            length = found_point - response;
        } else {
            found_point = g_strstr_len(response + 1, size - 1, "receive: ");
            if (found_point)
                length = found_point - response;
            else
                length = size;
        }

        response_data = response_data_new(response, length);
        list = g_list_append(list, response_data);

        if (!found_point)
            break;

        response = found_point;
        size -= length;
    }

    return list;
}

static void
cb_output_received (GCutProcess *process, const gchar *chunk, gsize size,
                    gpointer user_data)
{
    MilterManagerTestClient *client = user_data;
    MilterManagerTestClientPrivate *priv;
    GList *responses, *node;
    GIOChannel *output_channel;

    priv = MILTER_MANAGER_TEST_CLIENT_GET_PRIVATE(client);

    g_string_append_len(priv->output_received, chunk, size);
    output_channel = gcut_process_get_output_channel(process);
    if (chunk[size - 1] != '\n' ||
        size == 4096 /* FIXME: 4096 is GCutProcess's buffer size */ ||
        g_io_channel_get_buffer_condition(output_channel) == G_IO_IN) {
        return;
    }

    priv->have_output = TRUE;
    responses = split_response(priv->output_received->str,
                               priv->output_received->len);
    for (node = g_list_first(responses); node; node = g_list_next(node)) {
        ResponseData *response = (ResponseData*)node->data;
        if (g_str_has_prefix(response->string, "ready")) {
            priv->ready = TRUE;
        } else if (g_str_has_prefix(response->string, "receive: negotiate")) {
            priv->n_negotiate_received++;
            parse_negotiate_info(priv, response->string, response->size);
        } else if (g_str_has_prefix(response->string, "receive: define-macro")) {
            priv->n_define_macro_received++;
            parse_define_macro_info(priv, response->string, response->size);
        } else if (g_str_has_prefix(response->string, "receive: connect")) {
            priv->n_connect_received++;
            parse_connect_info(priv, response->string, response->size);
        } else if (g_str_has_prefix(response->string, "receive: helo")) {
            priv->n_helo_received++;
            parse_helo_info(priv, response->string, response->size);
        } else if (g_str_has_prefix(response->string,
                                    "receive: envelope-from")) {
            priv->n_envelope_from_received++;
            parse_envelope_from_info(priv, response->string, response->size);
        } else if (g_str_has_prefix(response->string,
                                    "receive: envelope-recipient")) {
            priv->n_envelope_recipient_received++;
            parse_envelope_recipient_info(priv,
                                          response->string, response->size);
        } else if (g_str_has_prefix(response->string, "receive: data")) {
            priv->n_data_received++;
        } else if (g_str_has_prefix(response->string, "receive: header")) {
            priv->n_header_received++;
            parse_header_info(priv, response->string, response->size);
        } else if (g_str_has_prefix(response->string,
                                    "receive: end-of-header")) {
            priv->n_end_of_header_received++;
        } else if (g_str_has_prefix(response->string, "receive: body")) {
            priv->n_body_received++;
            parse_body_info(priv, response->string, response->size);
        } else if (g_str_has_prefix(response->string,
                                    "receive: end-of-message")) {
            priv->n_end_of_message_received++;
            parse_end_of_message_info(priv, response->string, response->size);
        } else if (g_str_has_prefix(response->string, "receive: quit")) {
            priv->n_quit_received++;
        } else if (g_str_has_prefix(response->string, "receive: abort")) {
            priv->n_abort_received++;
        } else if (g_str_has_prefix(response->string, "receive: unknown")) {
            priv->n_unknown_received++;
            parse_unknown_info(priv, response->string, response->size);
        } else {
            GString *string;

            string = g_string_new_len(response->string, size);
            g_print("[OUTPUT] <%s>\n", string->str);
            g_string_free(string, TRUE);
        }
    }
    g_string_truncate(priv->output_received, 0);
    g_list_foreach(responses, (GFunc)free_response_data, NULL);
    g_list_free(responses);
}

static void
cb_error_received (GCutProcess *process, const gchar *chunk, gsize size,
                   gpointer user_data)
{
    GString *string;

    string = g_string_new_len(chunk, size);
    g_print("[ERROR] <%s>\n", string->str);
    g_string_free(string, TRUE);
}

static void
cb_reaped (GCutProcess *process, gint status, gpointer user_data)
{
    MilterManagerTestClient *client = user_data;
    MilterManagerTestClientPrivate *priv;

    priv = MILTER_MANAGER_TEST_CLIENT_GET_PRIVATE(client);
    priv->reaped = TRUE;
}

static void
setup_process_signals (GCutProcess *process, MilterManagerTestClient *client)
{
#define CONNECT(name)                                                   \
    g_signal_connect(process, #name, G_CALLBACK(cb_ ## name), client)

    CONNECT(output_received);
    CONNECT(error_received);
    CONNECT(reaped);

#undef CONNECT
}

static void
teardown_process_signals (GCutProcess *process, MilterManagerTestClient *client)
{
#define DISCONNECT(name)                                                \
    g_signal_handlers_disconnect_by_func(process,                           \
                                         G_CALLBACK(cb_ ## name),       \
                                         client)

    DISCONNECT(output_received);
    DISCONNECT(error_received);
    DISCONNECT(reaped);

#undef DISCONNECT
}

static void
process_free (GCutProcess *process, MilterManagerTestClient *client)
{
    teardown_process_signals(process, client);
    g_object_unref(process);
}

static void
command_free (GArray *command)
{
    guint i;

    for (i = 0; i < command->len; i++) {
        g_free(g_array_index(command, gchar *, i));
    }

    g_array_free(command, TRUE);
}

void
milter_manager_test_client_clear_data (MilterManagerTestClient *client)
{
    MilterManagerTestClientPrivate *priv;

    priv = MILTER_MANAGER_TEST_CLIENT_GET_PRIVATE(client);

    priv->n_negotiate_received = 0;
    priv->n_define_macro_received = 0;
    priv->n_connect_received = 0;
    priv->n_helo_received = 0;
    priv->n_envelope_from_received = 0;
    priv->n_envelope_recipient_received = 0;
    priv->n_data_received = 0;
    priv->n_header_received = 0;
    priv->n_end_of_header_received = 0;
    priv->n_body_received = 0;
    priv->n_end_of_message_received = 0;
    priv->n_quit_received = 0;
    priv->n_abort_received = 0;
    priv->n_unknown_received = 0;

    if (priv->negotiate_option) {
        g_object_unref(priv->negotiate_option);
        priv->negotiate_option = NULL;
    }

    if (priv->macros)
        g_hash_table_unref(priv->macros);
    priv->macros = g_hash_table_new_full(g_direct_hash, g_direct_equal,
                                         NULL,
                                         (GDestroyNotify)g_hash_table_unref);

    if (priv->connect_host) {
        g_free(priv->connect_host);
        priv->connect_host = NULL;
    }

    if (priv->connect_address) {
        g_free(priv->connect_address);
        priv->connect_address = NULL;
    }

    if (priv->helo_fqdn) {
        g_free(priv->helo_fqdn);
        priv->helo_fqdn = NULL;
    }

    if (priv->envelope_from) {
        g_free(priv->envelope_from);
        priv->envelope_from = NULL;
    }

    if (priv->envelope_recipient) {
        g_free(priv->envelope_recipient);
        priv->envelope_recipient = NULL;
    }

    if (priv->header_name) {
        g_free(priv->header_name);
        priv->header_name = NULL;
    }

    if (priv->header_value) {
        g_free(priv->header_value);
        priv->header_value = NULL;
    }

    if (priv->body_chunk) {
        g_string_free(priv->body_chunk, TRUE);
        priv->body_chunk = NULL;
    }

    if (priv->end_of_message_chunk) {
        g_free(priv->end_of_message_chunk);
        priv->end_of_message_chunk = NULL;
    }

    if (priv->unknown_command) {
        g_free(priv->unknown_command);
        priv->unknown_command = NULL;
    }
}

static void
dispose (GObject *object)
{
    MilterManagerTestClient *client;
    MilterManagerTestClientPrivate *priv;

    client = MILTER_MANAGER_TEST_CLIENT(object);
    priv = MILTER_MANAGER_TEST_CLIENT_GET_PRIVATE(object);

    if (priv->name) {
        g_free(priv->name);
        priv->name = NULL;
    }

    if (priv->process) {
        process_free(priv->process, client);
        priv->process = NULL;
    }

    if (priv->command) {
        command_free(priv->command);
        priv->command = NULL;
    }

    if (priv->output_received) {
        g_string_free(priv->output_received, TRUE);
        priv->output_received = NULL;
    }

    milter_manager_test_client_clear_data(client);

    if (priv->macros) {
        g_hash_table_unref(priv->macros);
        priv->macros = NULL;
    }

    if (priv->loop) {
        g_object_unref(priv->loop);
        priv->loop = NULL;
    }

    G_OBJECT_CLASS(milter_manager_test_client_parent_class)->dispose(object);
}

MilterManagerTestClient *
milter_manager_test_client_new (guint port, const gchar *name,
                                MilterEventLoop *loop)
{
    MilterManagerTestClient *client;
    MilterManagerTestClientPrivate *priv;

    client = g_object_new(MILTER_TYPE_MANAGER_TEST_CLIENT, NULL);
    priv = MILTER_MANAGER_TEST_CLIENT_GET_PRIVATE(client);
    priv->port = port;
    priv->name = g_strdup(name);
    priv->loop = loop;
    g_object_ref(priv->loop);

    return client;
}

const gchar *
milter_manager_test_client_get_name (MilterManagerTestClient *client)
{
    return MILTER_MANAGER_TEST_CLIENT_GET_PRIVATE(client)->name;
}

gboolean
milter_manager_test_client_run (MilterManagerTestClient *client, GError **error)
{
    MilterManagerTestClientPrivate *priv;
    GCutEventLoop *gcut_event_loop;

    priv = MILTER_MANAGER_TEST_CLIENT_GET_PRIVATE(client);

    if (priv->process)
        process_free(priv->process, client);

    if (!priv->command)
        milter_manager_test_client_set_arguments(client, NULL);

    priv->process = gcut_process_new_array(priv->command);

    gcut_event_loop = gcut_milter_event_loop_new(priv->loop);
    gcut_process_set_event_loop(priv->process, gcut_event_loop);
    g_object_unref(gcut_event_loop);

    setup_process_signals(priv->process, client);
    priv->ready = FALSE;
    priv->reaped = FALSE;
    if (!gcut_process_run(priv->process, error))
        return FALSE;

    while (!priv->ready && !priv->reaped)
        milter_event_loop_iterate(priv->loop, TRUE);

    return !priv->reaped;
}

static void
append_command (GArray *command, const gchar *argument)
{
    gchar *dupped_argument;

    dupped_argument = g_strdup(argument);
    g_array_append_val(command, dupped_argument);
}

void
milter_manager_test_client_set_arguments (MilterManagerTestClient *client,
                                          GArray *arguments)
{
    MilterManagerTestClientPrivate *priv;
    gchar *ruby_path, *test_client_path, *port;
    guint i;

    priv = MILTER_MANAGER_TEST_CLIENT_GET_PRIVATE(client);

    if (priv->command)
        command_free(priv->command);

    priv->command = g_array_new(TRUE, TRUE, sizeof(gchar *));

    ruby_path = g_strdup(g_getenv("RUBY"));
    g_array_append_val(priv->command, ruby_path);
    test_client_path = g_build_filename(milter_test_get_source_dir(),
                                        "test",
                                        "lib",
                                        "milter-test-client.rb",
                                        NULL);
    g_array_append_val(priv->command, test_client_path);
    append_command(priv->command, "--print-status");
    append_command(priv->command, "--timeout");
    append_command(priv->command, "1.5");
    append_command(priv->command, "--host");
    append_command(priv->command, "127.0.0.1");
    append_command(priv->command, "--port");
    port = g_strdup_printf("%u", priv->port);
    g_array_append_val(priv->command, port);

    if (!arguments)
        return;
    for (i = 0; i < arguments->len; i++) {
        append_command(priv->command, g_array_index(arguments, gchar *, i));
    }
}

void
milter_manager_test_client_set_argument_strings (MilterManagerTestClient *client,
                                                 const gchar **argument_strings)
{
    GArray *arguments;
    const gchar **argument;

    arguments = g_array_new(TRUE, TRUE, sizeof(gchar *));
    if (argument_strings) {
        for (argument = argument_strings; *argument; argument++) {
            g_array_append_val(arguments, *argument);
        }
    }
    milter_manager_test_client_set_arguments(client, arguments);
    g_array_free(arguments, TRUE);
}

const GArray *
milter_manager_test_client_get_command (MilterManagerTestClient *client)
{
    return MILTER_MANAGER_TEST_CLIENT_GET_PRIVATE(client)->command;
}

guint
milter_manager_test_client_get_n_negotiate_received (MilterManagerTestClient *client)
{
    return MILTER_MANAGER_TEST_CLIENT_GET_PRIVATE(client)->n_negotiate_received;
}

MilterOption *
milter_manager_test_client_get_negotiate_option (MilterManagerTestClient *client)
{
    return MILTER_MANAGER_TEST_CLIENT_GET_PRIVATE(client)->negotiate_option;
}

guint
milter_manager_test_client_get_n_define_macro_received (MilterManagerTestClient *client)
{
    return MILTER_MANAGER_TEST_CLIENT_GET_PRIVATE(client)->n_define_macro_received;
}

const GHashTable *
milter_manager_test_client_get_macros (MilterManagerTestClient *client)
{
    return MILTER_MANAGER_TEST_CLIENT_GET_PRIVATE(client)->macros;
}

guint
milter_manager_test_client_get_n_connect_received (MilterManagerTestClient *client)
{
    return MILTER_MANAGER_TEST_CLIENT_GET_PRIVATE(client)->n_connect_received;
}

const gchar *
milter_manager_test_client_get_connect_host (MilterManagerTestClient *client)
{
    return MILTER_MANAGER_TEST_CLIENT_GET_PRIVATE(client)->connect_host;
}

const gchar *
milter_manager_test_client_get_connect_address (MilterManagerTestClient *client)
{
    return MILTER_MANAGER_TEST_CLIENT_GET_PRIVATE(client)->connect_address;
}

guint
milter_manager_test_client_get_n_helo_received (MilterManagerTestClient *client)
{
    return MILTER_MANAGER_TEST_CLIENT_GET_PRIVATE(client)->n_helo_received;
}

const gchar *
milter_manager_test_client_get_helo_fqdn (MilterManagerTestClient *client)
{
    return MILTER_MANAGER_TEST_CLIENT_GET_PRIVATE(client)->helo_fqdn;
}

guint
milter_manager_test_client_get_n_envelope_from_received (MilterManagerTestClient *client)
{
    return MILTER_MANAGER_TEST_CLIENT_GET_PRIVATE(client)->n_envelope_from_received;
}

const gchar *
milter_manager_test_client_get_envelope_from (MilterManagerTestClient *client)
{
    return MILTER_MANAGER_TEST_CLIENT_GET_PRIVATE(client)->envelope_from;
}

guint
milter_manager_test_client_get_n_envelope_recipient_received (MilterManagerTestClient *client)
{
    return MILTER_MANAGER_TEST_CLIENT_GET_PRIVATE(client)->n_envelope_recipient_received;
}

const gchar *
milter_manager_test_client_get_envelope_recipient (MilterManagerTestClient *client)
{
    return MILTER_MANAGER_TEST_CLIENT_GET_PRIVATE(client)->envelope_recipient;
}

guint
milter_manager_test_client_get_n_data_received (MilterManagerTestClient *client)
{
    return MILTER_MANAGER_TEST_CLIENT_GET_PRIVATE(client)->n_data_received;
}

guint
milter_manager_test_client_get_n_header_received (MilterManagerTestClient *client)
{
    return MILTER_MANAGER_TEST_CLIENT_GET_PRIVATE(client)->n_header_received;
}

const gchar *
milter_manager_test_client_get_header_name (MilterManagerTestClient *client)
{
    return MILTER_MANAGER_TEST_CLIENT_GET_PRIVATE(client)->header_name;
}

const gchar *
milter_manager_test_client_get_header_value (MilterManagerTestClient *client)
{
    return MILTER_MANAGER_TEST_CLIENT_GET_PRIVATE(client)->header_value;
}

guint
milter_manager_test_client_get_n_end_of_header_received (MilterManagerTestClient *client)
{
    return MILTER_MANAGER_TEST_CLIENT_GET_PRIVATE(client)->n_end_of_header_received;
}

guint
milter_manager_test_client_get_n_body_received (MilterManagerTestClient *client)
{
    return MILTER_MANAGER_TEST_CLIENT_GET_PRIVATE(client)->n_body_received;
}

const gchar *
milter_manager_test_client_get_body_chunk (MilterManagerTestClient *client)
{
    GString *body_chunk;

    body_chunk = MILTER_MANAGER_TEST_CLIENT_GET_PRIVATE(client)->body_chunk;
    if (!body_chunk)
        return NULL;
    return body_chunk->len == 0 ? NULL : body_chunk->str;
}

guint
milter_manager_test_client_get_n_end_of_message_received (MilterManagerTestClient *client)
{
    return MILTER_MANAGER_TEST_CLIENT_GET_PRIVATE(client)->n_end_of_message_received;
}

const gchar *
milter_manager_test_client_get_end_of_message_chunk (MilterManagerTestClient *client)
{
    return MILTER_MANAGER_TEST_CLIENT_GET_PRIVATE(client)->end_of_message_chunk;
}

guint
milter_manager_test_client_get_n_quit_received (MilterManagerTestClient *client)
{
    return MILTER_MANAGER_TEST_CLIENT_GET_PRIVATE(client)->n_quit_received;
}

guint
milter_manager_test_client_get_n_abort_received (MilterManagerTestClient *client)
{
    return MILTER_MANAGER_TEST_CLIENT_GET_PRIVATE(client)->n_abort_received;
}

guint
milter_manager_test_client_get_n_unknown_received (MilterManagerTestClient *client)
{
    return MILTER_MANAGER_TEST_CLIENT_GET_PRIVATE(client)->n_unknown_received;
}

const gchar *
milter_manager_test_client_get_unknown_command (MilterManagerTestClient *client)
{
    return MILTER_MANAGER_TEST_CLIENT_GET_PRIVATE(client)->unknown_command;
}

gboolean
milter_manager_test_client_is_reaped (MilterManagerTestClient *client)
{
    return MILTER_MANAGER_TEST_CLIENT_GET_PRIVATE(client)->reaped;
}

static gboolean
cb_timeout_waiting (gpointer data)
{
    gboolean *waiting = data;

    *waiting = FALSE;
    return FALSE;
}

void
milter_manager_test_client_wait_reply (MilterManagerTestClient *client,
                                       MilterManagerTestClientGetNReceived getter)
{
    MilterManagerTestClientPrivate *priv;
    gboolean timeout_waiting = TRUE;
    guint timeout_waiting_id;

    priv = MILTER_MANAGER_TEST_CLIENT_GET_PRIVATE(client);
    timeout_waiting_id = milter_event_loop_add_timeout(priv->loop, 0.1,
                                                       cb_timeout_waiting,
                                                       &timeout_waiting);
    while (timeout_waiting && getter(client) == 0)
        milter_event_loop_iterate(priv->loop, TRUE);
    milter_event_loop_remove(priv->loop, timeout_waiting_id);

    cut_assert_true(timeout_waiting, cut_message("timeout"));
}

void
milter_manager_test_clients_wait_reply (const GList *clients,
                                        MilterManagerTestClientGetNReceived getter)
{
    milter_manager_test_clients_wait_n_replies(clients, getter,
                                               g_list_length((GList *)clients));
}

void
milter_manager_test_clients_wait_n_replies(const GList *clients,
                                           MilterManagerTestClientGetNReceived getter,
                                           guint n_replies)
{
    MilterManagerTestClientPrivate *priv;
    gboolean timeout_waiting = TRUE;
    guint timeout_waiting_id;

    priv = MILTER_MANAGER_TEST_CLIENT_GET_PRIVATE(clients->data);
    timeout_waiting_id = milter_event_loop_add_timeout(priv->loop, 1,
                                                       cb_timeout_waiting,
                                                       &timeout_waiting);
    while (timeout_waiting &&
           n_replies > milter_manager_test_clients_collect_n_received(clients,
                                                                      getter)) {
        milter_event_loop_iterate(priv->loop, TRUE);
    }
    milter_event_loop_remove(priv->loop, timeout_waiting_id);

    cut_assert_true(timeout_waiting, cut_message("timeout"));
    cut_assert_equal_uint(n_replies,
                          milter_manager_test_clients_collect_n_received(clients,
                                                                         getter));
}

void
milter_manager_test_clients_wait_n_alive(const GList *clients, guint n_alive)
{
    MilterManagerTestClientPrivate *priv;
    gboolean timeout_waiting = TRUE;
    guint timeout_waiting_id;

    priv = MILTER_MANAGER_TEST_CLIENT_GET_PRIVATE(clients->data);
    timeout_waiting_id = milter_event_loop_add_timeout(priv->loop, 1,
                                                       cb_timeout_waiting,
                                                       &timeout_waiting);
    while (timeout_waiting &&
           n_alive != milter_manager_test_clients_collect_n_alive(clients)) {
        milter_event_loop_iterate(priv->loop, TRUE);
    }
    milter_event_loop_remove(priv->loop, timeout_waiting_id);

    cut_assert_true(timeout_waiting, cut_message("timeout"));
    cut_assert_equal_uint(n_alive,
                          milter_manager_test_clients_collect_n_alive(clients));
}

void
milter_manager_test_client_assert_nothing_output (MilterManagerTestClient *client)
{
    MilterManagerTestClientPrivate *priv;
    gboolean timeout_waiting = TRUE;
    guint timeout_waiting_id;

    priv = MILTER_MANAGER_TEST_CLIENT_GET_PRIVATE(client);
    priv->have_output = FALSE;
    timeout_waiting_id = milter_event_loop_add_timeout(priv->loop, 0.1,
                                                       cb_timeout_waiting,
                                                       &timeout_waiting);
    while (timeout_waiting && !priv->have_output)
        milter_event_loop_iterate(priv->loop, TRUE);
    milter_event_loop_remove(priv->loop, timeout_waiting_id);

    cut_assert_false(timeout_waiting);
    cut_assert_false(priv->have_output);
}


MilterManagerTestClient *
milter_manager_test_clients_find (const GList *clients, const gchar *name)
{
    const GList *node;

    for (node = clients; node; node = g_list_next(node)) {
        MilterManagerTestClientPrivate *priv;

        priv = MILTER_MANAGER_TEST_CLIENT_GET_PRIVATE(node->data);

        if (g_str_equal(priv->name, name))
            return MILTER_MANAGER_TEST_CLIENT(node->data);
    }

    return NULL;
}

guint
milter_manager_test_clients_collect_n_received (const GList *clients,
                                                MilterManagerTestClientGetNReceived getter)
{
    guint n_received = 0;
    const GList *node;

    for (node = clients; node; node = g_list_next(node)) {
        MilterManagerTestClient *client = node->data;

        n_received += getter(client);
    }

    return n_received;
}

guint
milter_manager_test_clients_collect_n_alive (const GList *clients)
{
    guint n_alive = 0;
    const GList *node;

    for (node = clients; node; node = g_list_next(node)) {
        MilterManagerTestClient *client = node->data;

        if (!milter_manager_test_client_is_reaped(client))
            n_alive++;
    }

    return n_alive;
}

const GList *
milter_manager_test_clients_collect_strings (const GList *clients,
                                             MilterManagerTestClientGetString getter)
{
    GList *collected_strings = NULL;
    const GList *node;

    for (node = clients; node; node = g_list_next(node)) {
        MilterManagerTestClient *client = node->data;

        collected_strings = g_list_append(collected_strings,
                                          g_strdup(getter(client)));
    }

    return gcut_take_list(collected_strings, g_free);
}

const GList *
milter_manager_test_clients_collect_pairs (const GList *clients,
                                           MilterManagerTestClientGetString name_getter,
                                           MilterManagerTestClientGetString value_getter)
{
    GList *collected_pairs = NULL;
    const GList *node;

    for (node = clients; node; node = g_list_next(node)) {
        MilterManagerTestClient *client = node->data;

        collected_pairs =
            g_list_append(collected_pairs,
                          milter_manager_test_pair_new(name_getter(client),
                                                       value_getter(client)));
    }

    return gcut_take_list(collected_pairs,
                          (CutDestroyFunction)milter_manager_test_pair_free);
}

static void
safe_g_object_unref (void *data)
{
    if (data)
        g_object_unref(data);
}

const GList *
milter_manager_test_clients_collect_negotiate_options (const GList *clients)
{
    GList *collected_options = NULL;
    const GList *node;

    for (node = clients; node; node = g_list_next(node)) {
        MilterManagerTestClient *client = node->data;
        MilterOption *option;

        option = milter_manager_test_client_get_negotiate_option(client);
        if (option)
            g_object_ref(option);
        collected_options = g_list_append(collected_options, option);
    }

    return gcut_take_list(collected_options, safe_g_object_unref);
}

const GList *
milter_manager_test_clients_collect_macros (const GList *clients,
                                            MilterCommand command)
{
    GList *collected_macros = NULL;
    const GList *node;

    for (node = clients; node; node = g_list_next(node)) {
        MilterManagerTestClient *client = node->data;
        GHashTable *macros;
        GHashTable *context_macro;
        GList *macro_list = NULL;
        GList *macro_names, *node;

        macros = (GHashTable*)milter_manager_test_client_get_macros(client);
        context_macro = g_hash_table_lookup(macros,
                                            GUINT_TO_POINTER(command));
        if (!context_macro)
            continue;

        macro_names = g_hash_table_get_keys(context_macro);
        macro_names = g_list_sort(macro_names, (GCompareFunc)g_utf8_collate);
        for (node = macro_names; node; node = g_list_next(node)) {
            const gchar *macro_name = node->data;
            const gchar *macro_value;
            MilterManagerTestPair *pair;

            macro_value = g_hash_table_lookup(context_macro, macro_name);
            pair = milter_manager_test_pair_new(macro_name, macro_value);
            macro_list = g_list_append(macro_list, pair);
        }
        collected_macros = g_list_concat(collected_macros, macro_list);
    }

    return gcut_take_list(collected_macros,
                          (CutDestroyFunction)milter_manager_test_pair_free);
}

const GList *
milter_manager_test_clients_collect_all_macros (const GList *clients)
{
    GList *collected_macros = NULL;
    const GList *node;

    for (node = clients; node; node = g_list_next(node)) {
        MilterManagerTestClient *client = node->data;
        GHashTable *macros;

        macros = (GHashTable *)milter_manager_test_client_get_macros(client);
        g_hash_table_ref(macros);
        collected_macros = g_list_append(collected_macros, macros);
    }

    return gcut_take_list(collected_macros,
                          (CutDestroyFunction)g_hash_table_unref);
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
