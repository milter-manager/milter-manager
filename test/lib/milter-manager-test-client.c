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
    gchar *test_client_path;
    guint port;

    gboolean ready;
    gboolean reaped;

    guint n_negotiate_received;
    guint n_connect_received;
    guint n_helo_received;
    guint n_envelope_from_received;
    guint n_envelope_receipt_received;
    guint n_data_received;
    guint n_header_received;
    guint n_end_of_header_received;
    guint n_body_received;
    guint n_end_of_message_received;
    guint n_quit_received;
    guint n_abort_received;
    guint n_unknown_received;

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
    priv->test_client_path = NULL;
    priv->port = 0;

    priv->ready = FALSE;
    priv->reaped = FALSE;

    priv->n_negotiate_received = 0;
    priv->n_connect_received = 0;
    priv->n_helo_received = 0;
    priv->n_envelope_from_received = 0;
    priv->n_envelope_receipt_received = 0;
    priv->n_data_received = 0;
    priv->n_header_received = 0;
    priv->n_end_of_header_received = 0;
    priv->n_body_received = 0;
    priv->n_end_of_message_received = 0;
    priv->n_quit_received = 0;
    priv->n_abort_received = 0;
    priv->n_unknown_received = 0;

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
        priv->n_negotiate_received++;
    } else if (g_str_has_prefix(chunk, "receive: connect")) {
        priv->n_connect_received++;
    } else if (g_str_has_prefix(chunk, "receive: helo")) {
        priv->n_helo_received++;
    } else if (g_str_has_prefix(chunk, "receive: mail")) {
        priv->n_envelope_from_received++;
    } else if (g_str_has_prefix(chunk, "receive: rcpt")) {
        priv->n_envelope_receipt_received++;
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

    if (priv->test_client_path) {
        g_free(priv->test_client_path);
        priv->test_client_path = NULL;
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

    G_OBJECT_CLASS(milter_manager_test_client_parent_class)->dispose(object);
}

MilterManagerTestClient *
milter_manager_test_client_new (const guint port)
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
    gchar *port_string;

    priv = MILTER_MANAGER_TEST_CLIENT_GET_PRIVATE(client);

    if (priv->egg)
        egg_free(priv->egg, client);

    if (!priv->test_client_path)
        priv->test_client_path =
            g_build_filename(milter_manager_test_get_base_dir(),
                             "lib",
                             "milter-test-client.rb",
                             NULL);

    port_string = g_strdup_printf("%u", priv->port);
    priv->egg = gcut_egg_new(priv->test_client_path,
                             "--print-status",
                             "--timeout", "1.0",
                             "--port", port_string,
                             NULL);
    g_free(port_string);

    setup_egg_signals(priv->egg, client);
    priv->ready = FALSE;
    priv->reaped = FALSE;
    if (!gcut_egg_hatch(priv->egg, error))
        return FALSE;

    while (!priv->ready && !priv->reaped)
        g_main_context_iteration(NULL, TRUE);

    return !priv->reaped;
}

guint
milter_manager_test_client_get_n_negotiate_received (MilterManagerTestClient *client)
{
    return MILTER_MANAGER_TEST_CLIENT_GET_PRIVATE(client)->n_negotiate_received;
}

guint
milter_manager_test_client_get_n_connect_received (MilterManagerTestClient *client)
{
    return MILTER_MANAGER_TEST_CLIENT_GET_PRIVATE(client)->n_connect_received;
}

guint
milter_manager_test_client_get_n_helo_received (MilterManagerTestClient *client)
{
    return MILTER_MANAGER_TEST_CLIENT_GET_PRIVATE(client)->n_helo_received;
}

guint
milter_manager_test_client_get_n_envelope_from_received (MilterManagerTestClient *client)
{
    return MILTER_MANAGER_TEST_CLIENT_GET_PRIVATE(client)->n_envelope_from_received;
}

guint
milter_manager_test_client_get_n_envelope_receipt_received (MilterManagerTestClient *client)
{
    return MILTER_MANAGER_TEST_CLIENT_GET_PRIVATE(client)->n_envelope_receipt_received;
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


/*
vi:nowrap:ai:expandtab:sw=4
*/
