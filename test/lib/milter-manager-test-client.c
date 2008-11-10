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

#include <gcutter.h>

#include "milter-manager-test-utils.h"
#include "milter-manager-test-client.h"

#define MILTER_MANAGER_TEST_CLIENT_GET_PRIVATE(obj)                     \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj),                                 \
                                 MILTER_TYPE_MANAGER_TEST_CLIENT,       \
                                 MilterManagerTestClientPrivate))

typedef struct _MilterManagerTestClientPrivate MilterManagerTestClientPrivate;
struct _MilterManagerTestClientPrivate
{
    GCutEgg *egg;
    guint port;
    GArray *command;

    gboolean ready;
    gboolean reaped;

    guint n_negotiate_received;
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
    gchar *connect_host;
    gchar *connect_address;
    gchar *helo_fqdn;
    gchar *envelope_from;
    gchar *envelope_recipient;
    gchar *header_name;
    gchar *header_value;
    gchar *body_chunk;
    gchar *end_of_message_chunk;
    gchar *unknown_command;
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

    priv->egg = NULL;
    priv->port = 0;
    priv->command = NULL;

    priv->ready = FALSE;
    priv->reaped = FALSE;

    priv->n_negotiate_received = 0;
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

    priv->negotiate_option = NULL;
    priv->connect_host = NULL;
    priv->connect_address = NULL;
    priv->helo_fqdn = NULL;
    priv->envelope_from = NULL;
    priv->envelope_recipient = NULL;
    priv->header_name = NULL;
    priv->header_value = NULL;
    priv->body_chunk = NULL;
    priv->end_of_message_chunk = NULL;
    priv->unknown_command = NULL;
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
cb_output_received (GCutEgg *egg, const gchar *chunk, gsize size,
                    gpointer user_data)
{
    MilterManagerTestClient *client = user_data;
    MilterManagerTestClientPrivate *priv;

    priv = MILTER_MANAGER_TEST_CLIENT_GET_PRIVATE(client);
    if (g_str_has_prefix(chunk, "ready")) {
        priv->ready = TRUE;
    } else if (g_str_has_prefix(chunk, "receive: negotiate")) {
        gchar **items;

        priv->n_negotiate_received++;

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
    } else if (g_str_has_prefix(chunk, "receive: connect")) {
        gchar **items;

        priv->n_connect_received++;

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
    } else if (g_str_has_prefix(chunk, "receive: helo")) {
        priv->n_helo_received++;

        if (priv->helo_fqdn)
            g_free(priv->helo_fqdn);
        priv->helo_fqdn =
            receive_additional_info(chunk, size, "receive: helo: ");
    } else if (g_str_has_prefix(chunk, "receive: mail")) {
        priv->n_envelope_from_received++;

        if (priv->envelope_from)
            g_free(priv->envelope_from);
        priv->envelope_from =
            receive_additional_info(chunk, size, "receive: mail: ");
    } else if (g_str_has_prefix(chunk, "receive: rcpt")) {
        priv->n_envelope_recipient_received++;

        if (priv->envelope_recipient)
            g_free(priv->envelope_recipient);
        priv->envelope_recipient =
            receive_additional_info(chunk, size, "receive: rcpt: ");
    } else if (g_str_has_prefix(chunk, "receive: data")) {
        priv->n_data_received++;
    } else if (g_str_has_prefix(chunk, "receive: header")) {
        gchar **items;

        priv->n_header_received++;

        items = g_strsplit_set(chunk, " \n", -1);
        if (items[0] && items[1] && items[2] && items[3]) {
            if (priv->header_name)
                g_free(priv->header_name);
            if (priv->header_value)
                g_free(priv->header_value);
            priv->header_name = g_strdup(items[2]);
            priv->header_value = g_strdup(items[3]);
        }
        g_strfreev(items);
    } else if (g_str_has_prefix(chunk, "receive: end-of-header")) {
        priv->n_end_of_header_received++;
    } else if (g_str_has_prefix(chunk, "receive: body")) {
        priv->n_body_received++;

        if (priv->body_chunk)
            g_free(priv->body_chunk);
        priv->body_chunk =
            receive_additional_info(chunk, size, "receive: body: ");
    } else if (g_str_has_prefix(chunk, "receive: end-of-message")) {
        priv->n_end_of_message_received++;

        if (priv->end_of_message_chunk)
            g_free(priv->end_of_message_chunk);
        priv->end_of_message_chunk =
            receive_additional_info(chunk, size, "receive: end-of-message: ");
    } else if (g_str_has_prefix(chunk, "receive: quit")) {
        priv->n_quit_received++;
    } else if (g_str_has_prefix(chunk, "receive: abort")) {
        priv->n_abort_received++;
    } else if (g_str_has_prefix(chunk, "receive: unknown")) {
        priv->n_unknown_received++;

        if (priv->unknown_command)
            g_free(priv->unknown_command);
        priv->unknown_command =
            receive_additional_info(chunk, size, "receive: unknown: ");
    } else {
        GString *string;

        string = g_string_new_len(chunk, size);
        g_print("[OUTPUT] <%s>\n", string->str);
        g_string_free(string, TRUE);
    }
}

static void
cb_error_received (GCutEgg *egg, const gchar *chunk, gsize size,
                   gpointer user_data)
{
    GString *string;

    string = g_string_new_len(chunk, size);
    g_print("[ERROR] <%s>\n", string->str);
    g_string_free(string, TRUE);
}

static void
cb_reaped (GCutEgg *egg, gint status, gpointer user_data)
{
    MilterManagerTestClient *client = user_data;
    MilterManagerTestClientPrivate *priv;

    priv = MILTER_MANAGER_TEST_CLIENT_GET_PRIVATE(client);
    priv->reaped = TRUE;
}

static void
setup_egg_signals (GCutEgg *egg, MilterManagerTestClient *client)
{
#define CONNECT(name)                                                   \
    g_signal_connect(egg, #name, G_CALLBACK(cb_ ## name), client)

    CONNECT(output_received);
    CONNECT(error_received);
    CONNECT(reaped);

#undef CONNECT
}

static void
teardown_egg_signals (GCutEgg *egg, MilterManagerTestClient *client)
{
#define DISCONNECT(name)                                                \
    g_signal_handlers_disconnect_by_func(egg,                           \
                                         G_CALLBACK(cb_ ## name),       \
                                         client)

    DISCONNECT(output_received);
    DISCONNECT(error_received);
    DISCONNECT(reaped);

#undef DISCONNECT
}

static void
egg_free (GCutEgg *egg, MilterManagerTestClient *client)
{
    teardown_egg_signals(egg, client);
    g_object_unref(egg);
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

static void
clear_data (MilterManagerTestClient *client)
{
    MilterManagerTestClientPrivate *priv;

    priv = MILTER_MANAGER_TEST_CLIENT_GET_PRIVATE(client);

    priv->n_negotiate_received = 0;
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
        g_free(priv->body_chunk);
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

    if (priv->egg) {
        egg_free(priv->egg, client);
        priv->egg = NULL;
    }

    if (priv->command) {
        command_free(priv->command);
        priv->command = NULL;
    }

    clear_data(client);

    G_OBJECT_CLASS(milter_manager_test_client_parent_class)->dispose(object);
}

MilterManagerTestClient *
milter_manager_test_client_new (guint port)
{
    MilterManagerTestClient *client;
    MilterManagerTestClientPrivate *priv;

    client = g_object_new(MILTER_TYPE_MANAGER_TEST_CLIENT, NULL);
    priv = MILTER_MANAGER_TEST_CLIENT_GET_PRIVATE(client);
    priv->port = port;

    return client;
}

gboolean
milter_manager_test_client_run (MilterManagerTestClient *client, GError **error)
{
    MilterManagerTestClientPrivate *priv;

    priv = MILTER_MANAGER_TEST_CLIENT_GET_PRIVATE(client);

    if (priv->egg)
        egg_free(priv->egg, client);

    if (!priv->command)
        milter_manager_test_client_set_arguments(client, NULL);

    priv->egg = gcut_egg_new_array(priv->command);

    setup_egg_signals(priv->egg, client);
    priv->ready = FALSE;
    priv->reaped = FALSE;
    if (!gcut_egg_hatch(priv->egg, error))
        return FALSE;

    while (!priv->ready && !priv->reaped)
        g_main_context_iteration(NULL, TRUE);

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
    gchar *test_client_path, *port;
    guint i;

    priv = MILTER_MANAGER_TEST_CLIENT_GET_PRIVATE(client);

    if (priv->command)
        command_free(priv->command);

    priv->command = g_array_new(TRUE, TRUE, sizeof(gchar *));

    test_client_path = g_build_filename(milter_manager_test_get_base_dir(),
                                        "lib",
                                        "milter-test-client.rb",
                                        NULL);
    g_array_append_val(priv->command, test_client_path);
    append_command(priv->command, "--print-status");
    append_command(priv->command, "--timeout");
    append_command(priv->command, "0.5");
    append_command(priv->command, "--port");
    port = g_strdup_printf("%u", priv->port);
    g_array_append_val(priv->command, port);

    if (!arguments)
        return;
    for (i = 0; i < arguments->len; i++) {
        append_command(priv->command, g_array_index(arguments, gchar *, i));
    }
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
    return MILTER_MANAGER_TEST_CLIENT_GET_PRIVATE(client)->body_chunk;
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

void
milter_manager_test_client_clear_data (MilterManagerTestClient *client)
{
    clear_data(client);
}

static gboolean
cb_timeout_waiting (gpointer data)
{
    gboolean *waiting = data;

    *waiting = FALSE;
    return FALSE;
}

void
milter_manager_test_clients_wait_reply (GList *clients,
                                        MilterManagerTestClientGetNReceived getter)
{
    gboolean timeout_waiting = TRUE;
    guint timeout_waiting_id;
    guint n_clients;

    n_clients = g_list_length(clients);
    timeout_waiting_id = g_timeout_add(10, cb_timeout_waiting,
                                       &timeout_waiting);
    while (timeout_waiting &&
           n_clients > milter_manager_test_clients_collect_n_received(clients,
                                                                      getter)) {
        g_main_context_iteration(NULL, TRUE);
    }
    g_source_remove(timeout_waiting_id);

    cut_assert_true(timeout_waiting, "timeout");
    cut_assert_equal_uint(n_clients,
                          milter_manager_test_clients_collect_n_received(clients,
                                                                         getter));
}

guint
milter_manager_test_clients_collect_n_received (GList *clients,
                                                MilterManagerTestClientGetNReceived getter)
{
    guint n_received = 0;
    GList *node;

    for (node = clients; node; node = g_list_next(node)) {
        MilterManagerTestClient *client = node->data;

        n_received += getter(client);
    }

    return n_received;
}

const GList *
milter_manager_test_clients_collect_strings (GList *clients,
                                             MilterManagerTestClientGetString getter)
{
    GList *collected_strings = NULL;
    GList *node;

    for (node = clients; node; node = g_list_next(node)) {
        MilterManagerTestClient *client = node->data;

        collected_strings = g_list_append(collected_strings,
                                          g_strdup(getter(client)));
    }

    return gcut_take_list(collected_strings, g_free);
}

const GList *
milter_manager_test_clients_collect_pairs (GList *clients,
                                           MilterManagerTestClientGetString name_getter,
                                           MilterManagerTestClientGetString value_getter)
{
    GList *collected_pairs = NULL;
    GList *node;

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
milter_manager_test_clients_collect_negotiate_options (GList *clients)
{
    GList *collected_options = NULL;
    GList *node;

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

/*
vi:nowrap:ai:expandtab:sw=4
*/
