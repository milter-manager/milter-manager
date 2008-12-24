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

#ifdef HAVE_CONFIG_H
#  include "../config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

#include <sys/socket.h>
#include <sys/un.h>

#include <glib/gstdio.h>

#include "libmilter-compatible.h"

static struct smfiDesc *filter_description;
static gchar *connection_spec = NULL;
static GIOChannel *listen_channel = NULL;
static gint listen_backlog = -1;
static guint timeout = 7210;

#define SMFI_CONTEXT_GET_PRIVATE(obj)                   \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj),                 \
                                 SMFI_TYPE_CONTEXT,     \
                                 SmfiContextPrivate))

typedef struct _SmfiContextPrivate	SmfiContextPrivate;
struct _SmfiContextPrivate
{
    MilterClientContext *client_context;
    gpointer private_data;
};

G_DEFINE_TYPE(SmfiContext, smfi_context, G_TYPE_OBJECT);

static void dispose        (GObject         *object);
static void set_property   (GObject         *object,
                            guint            prop_id,
                            const GValue    *value,
                            GParamSpec      *pspec);
static void get_property   (GObject         *object,
                            guint            prop_id,
                            GValue          *value,
                            GParamSpec      *pspec);

static void
smfi_context_class_init (SmfiContextClass *klass)
{
    GObjectClass *gobject_class;

    gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->dispose      = dispose;
    gobject_class->set_property = set_property;
    gobject_class->get_property = get_property;

    g_type_class_add_private(gobject_class, sizeof(SmfiContextPrivate));
}

static void
smfi_context_init (SmfiContext *context)
{
    SmfiContextPrivate *priv;

    priv = SMFI_CONTEXT_GET_PRIVATE(context);
    priv->client_context = NULL;
    priv->private_data = NULL;
}

static void
dispose (GObject *object)
{
    SmfiContext *context;

    context = SMFI_CONTEXT(object);

    G_OBJECT_CLASS(smfi_context_parent_class)->dispose(object);
}

static void
set_property (GObject      *object,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
    switch (prop_id) {
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static void
get_property (GObject    *object,
              guint       prop_id,
              GValue     *value,
              GParamSpec *pspec)
{
    switch (prop_id) {
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

GQuark
smfi_context_error_quark (void)
{
    return g_quark_from_static_string("smfi-context-error-quark");
}

SmfiContext *
smfi_context_new (void)
{
    return g_object_new(SMFI_TYPE_CONTEXT,
                        NULL);
}

int
smfi_opensocket (bool remove_socket)
{
    GError *error = NULL;
    GIOChannel *channel;
    struct sockaddr *address = NULL;
    socklen_t address_size;
    gboolean success = TRUE;

    if (!filter_description)
        return MI_FAILURE;

    if (!connection_spec)
        return MI_FAILURE;

    if (!milter_connection_parse_spec(connection_spec,
                                      NULL, &address, &address_size,
                                      &error)) {
        milter_error("%s", error->message);
        g_error_free(error);
        return MI_FAILURE;
    }

    if (address->sa_family == AF_UNIX && remove_socket) {
        struct sockaddr_un *address_unix = (struct sockaddr_un *)address;
        gchar *path;

        path = address_unix->sun_path;
        if (g_file_test(path, G_FILE_TEST_EXISTS)) {
            if (g_unlink(path) == -1) {
                milter_error("can't remove existing socket: <%s>: %s",
                             path, g_strerror(errno));
                success = FALSE;
            }
        }
    }
    g_free(address);
    if (!success)
        return MI_FAILURE;

    channel = milter_connection_listen(connection_spec, listen_backlog, &error);
    if (error) {
        milter_error("%s", error->message);
        g_error_free(error);
        return MI_FAILURE;
    }

    if (listen_channel)
        g_io_channel_unref(listen_channel);
    listen_channel = channel;
    return MI_SUCCESS;
}

static void
filter_description_free (void)
{
    if (!filter_description)
        return;

    if (filter_description->xxfi_name)
        g_free(filter_description->xxfi_name);

    g_free(filter_description);
    filter_description = NULL;
}

int
smfi_register (struct smfiDesc description)
{
    if (description.xxfi_version != SMFI_VERSION &&
        description.xxfi_version != 2 &&
        description.xxfi_version != 3 &&
        description.xxfi_version != 4) {
        return MI_FAILURE;
    }

    if (!description.xxfi_name)
        description.xxfi_name = "Unknown";

    filter_description_free();
    filter_description = g_memdup(&description, sizeof(struct smfiDesc));
    filter_description->xxfi_name = g_strdup(filter_description->xxfi_name);

    return MI_SUCCESS;
}

static MilterStatus
cb_negotiate (MilterClientContext *context, MilterOption *option,
              gpointer user_data)
{
    SmfiContext *smfi_context = user_data;
    MilterStatus status;
    gulong action, step, preserve1 = 0, preserve2 = 0;
    gulong action_out, step_out, preserve1_out = 0, preserve2_out = 0;

    if (!filter_description->xxfi_negotiate)
        return MILTER_STATUS_DEFAULT;

    action = action_out = milter_option_get_action(option);
    step = step_out = milter_option_get_step(option);
    status = filter_description->xxfi_negotiate(smfi_context,
                                               action, step,
                                               preserve1, preserve2,
                                               &action_out, &step_out,
                                               &preserve1_out, &preserve2_out);
    if (status == MILTER_STATUS_CONTINUE) {
        milter_option_set_action(option, action_out);
        milter_option_set_step(option, step_out);
    }
    return status;
}

static MilterStatus
cb_connect (MilterClientContext *context, const gchar *host_name,
            struct sockaddr *address, socklen_t address_length,
            gpointer user_data)
{
    SmfiContext *smfi_context = user_data;

    if (!filter_description->xxfi_connect)
        return MILTER_STATUS_DEFAULT;

    return filter_description->xxfi_connect(smfi_context,
                                           (gchar *)host_name,
                                           address);
}

static MilterStatus
cb_helo (MilterClientContext *context, const gchar *fqdn, gpointer user_data)
{
    SmfiContext *smfi_context = user_data;

    if (!filter_description->xxfi_helo)
        return MILTER_STATUS_DEFAULT;

    return filter_description->xxfi_helo(smfi_context, (gchar *)fqdn);
}

static MilterStatus
cb_envelope_from (MilterClientContext *context, const gchar *from,
                  gpointer user_data)
{
    SmfiContext *smfi_context = user_data;
    gchar *addresses[2];

    if (!filter_description->xxfi_envfrom)
        return MILTER_STATUS_DEFAULT;

    addresses[0] = (gchar *)from;
    addresses[1] = NULL;
    return filter_description->xxfi_envfrom(smfi_context, addresses);
}

static MilterStatus
cb_envelope_recipient (MilterClientContext *context, const gchar *recipient,
                       gpointer user_data)
{
    SmfiContext *smfi_context = user_data;
    gchar *addresses[2];

    if (!filter_description->xxfi_envrcpt)
        return MILTER_STATUS_DEFAULT;

    addresses[0] = (gchar *)recipient;
    addresses[1] = NULL;
    return filter_description->xxfi_envrcpt(smfi_context, addresses);
}

static MilterStatus
cb_data (MilterClientContext *context, gpointer user_data)
{
    SmfiContext *smfi_context = user_data;

    if (!filter_description->xxfi_data)
        return MILTER_STATUS_DEFAULT;

    return filter_description->xxfi_data(smfi_context);
}

static MilterStatus
cb_unknown (MilterClientContext *context, const gchar *command,
            gpointer user_data)
{
    SmfiContext *smfi_context = user_data;

    if (!filter_description->xxfi_unknown)
        return MILTER_STATUS_DEFAULT;

    return filter_description->xxfi_unknown(smfi_context, command);
}

static MilterStatus
cb_header (MilterClientContext *context, const gchar *name, const gchar *value,
           gpointer user_data)
{
    SmfiContext *smfi_context = user_data;

    if (!filter_description->xxfi_header)
        return MILTER_STATUS_DEFAULT;

    return filter_description->xxfi_header(smfi_context,
                                          (gchar *)name,
                                          (gchar *)value);
}

static MilterStatus
cb_end_of_header (MilterClientContext *context, gpointer user_data)
{
    SmfiContext *smfi_context = user_data;

    if (!filter_description->xxfi_eoh)
        return MILTER_STATUS_DEFAULT;

    return filter_description->xxfi_eoh(smfi_context);
}

static MilterStatus
cb_body (MilterClientContext *context, const guchar *chunk, gsize size,
         gpointer user_data)
{
    SmfiContext *smfi_context = user_data;

    if (!filter_description->xxfi_body)
        return MILTER_STATUS_DEFAULT;

    return filter_description->xxfi_body(smfi_context, (guchar *)chunk, size);
}

static MilterStatus
cb_end_of_message (MilterClientContext *context, gpointer user_data)
{
    SmfiContext *smfi_context = user_data;

    if (!filter_description->xxfi_eom)
        return MILTER_STATUS_DEFAULT;

    return filter_description->xxfi_eom(smfi_context);
}

static MilterStatus
cb_abort (MilterClientContext *context, gpointer user_data)
{
    SmfiContext *smfi_context = user_data;

    if (!filter_description->xxfi_abort)
        return MILTER_STATUS_DEFAULT;

    return filter_description->xxfi_abort(smfi_context);
}

static void
cb_finished (MilterFinishedEmittable *emittable, gpointer user_data)
{
    MilterClientContext *context;
    SmfiContext *smfi_context = user_data;

    if (filter_description->xxfi_close)
        filter_description->xxfi_close(smfi_context);

    context = MILTER_CLIENT_CONTEXT(emittable);
    smfi_context_detach_from_client_context(smfi_context, context);
}

void
smfi_context_attach_to_client_context (SmfiContext *context,
                                       MilterClientContext *client_context)
{
    SmfiContextPrivate *priv;

    priv = SMFI_CONTEXT_GET_PRIVATE(context);
    priv->client_context = g_object_ref(client_context);

#define CONNECT(name, smfi_name)                        \
    if (filter_description->xxfi_ ## smfi_name)         \
        g_signal_connect(client_context, #name,         \
                         G_CALLBACK(cb_ ## name),       \
                         context)

    CONNECT(negotiate, negotiate);
    CONNECT(connect, connect);
    CONNECT(helo, helo);
    CONNECT(envelope_from, envfrom);
    CONNECT(envelope_recipient, envrcpt);
    CONNECT(data, data);
    CONNECT(unknown, unknown);
    CONNECT(header, header);
    CONNECT(end_of_header, eoh);
    CONNECT(body, body);
    CONNECT(end_of_message, eom);
    CONNECT(abort, abort);

#undef CONNECT

    g_signal_connect_after(client_context, "finished",
                           G_CALLBACK(cb_finished), context);
}

void
smfi_context_detach_from_client_context (SmfiContext *context,
                                         MilterClientContext *client_context)
{
    SmfiContextPrivate *priv;

    priv = SMFI_CONTEXT_GET_PRIVATE(context);

#define DISCONNECT(name)                                                \
    g_signal_handlers_disconnect_by_func(client_context,                \
                                         G_CALLBACK(cb_ ## name),       \
                                         context)

    DISCONNECT(negotiate);
    DISCONNECT(connect);
    DISCONNECT(helo);
    DISCONNECT(envelope_from);
    DISCONNECT(envelope_recipient);
    DISCONNECT(data);
    DISCONNECT(unknown);
    DISCONNECT(header);
    DISCONNECT(end_of_header);
    DISCONNECT(body);
    DISCONNECT(end_of_message);
    DISCONNECT(abort);

    DISCONNECT(finished);

#undef DISCONNECT

    if (priv->client_context) {
        g_object_unref(priv->client_context);
        priv->client_context = NULL;
    }
}

static void
cb_connection_established (MilterClient *client, MilterClientContext *context,
                           gpointer user_data)
{
    smfi_context_attach_to_client_context(user_data, context);
}

static void
setup_milter_client (MilterClient *client, SmfiContext *context)
{
    milter_client_set_connection_spec(client, connection_spec, NULL);
    milter_client_set_listen_channel(client, listen_channel);
    milter_client_set_listen_backlog(client, listen_backlog);
    milter_client_set_timeout(client, timeout);
    g_signal_connect(client, "connection-established",
                     G_CALLBACK(cb_connection_established), context);
}

int
smfi_main (void)
{
    gboolean success;
    MilterClient *client;
    SmfiContext *smfi_context;

    milter_init();

    client = milter_client_new();
    smfi_context = smfi_context_new();
    setup_milter_client(client, smfi_context);
    success = milter_client_main(client);
    g_object_unref(smfi_context);
    g_object_unref(client);

    milter_quit();

    libmilter_compatible_reset();

    if (success)
        return MI_SUCCESS;
    else
        return MI_FAILURE;
}

int
smfi_setbacklog (int backlog)
{
    if (backlog <= 0)
        return MI_FAILURE;

    listen_backlog = backlog;
    return MI_SUCCESS;
}

int
smfi_setdbg (int level)
{
    MilterLogLevelFlags target_level = 0;

    if (level > 0)
        target_level |= MILTER_LOG_LEVEL_CRITICAL;
    if (level > 1)
        target_level |= MILTER_LOG_LEVEL_ERROR;
    if (level > 2)
        target_level |= MILTER_LOG_LEVEL_WARNING;
    if (level > 3)
        target_level |= MILTER_LOG_LEVEL_MESSAGE;
    if (level > 4)
        target_level |= MILTER_LOG_LEVEL_INFO;
    if (level > 5)
        target_level |= MILTER_LOG_LEVEL_DEBUG;

    milter_logger_set_target_level(milter_logger(), target_level);
    return MI_SUCCESS;
}

int
smfi_settimeout (int timeout)
{
    timeout = timeout;

    return MI_SUCCESS;
}

int
smfi_setconn (char *spec)
{
    GError *error = NULL;

    if (!spec) {
        milter_error("must not be NULL");
        return MI_FAILURE;
    }

    if (!milter_connection_parse_spec(spec, NULL, NULL, NULL, &error)) {
        milter_error("%s", error->message);
        g_error_free(error);
        return MI_FAILURE;
    }

    if (connection_spec)
        g_free(connection_spec);
    connection_spec = g_strdup(spec);
    return MI_SUCCESS;
}

int
smfi_stop (void)
{
    return MI_SUCCESS;
}

int
smfi_version (unsigned int *major, unsigned int *minor,
              unsigned int *patch_level)
{
    return MI_SUCCESS;
}

char *
smfi_getsymval (SMFICTX *context, char *name)
{
    SmfiContextPrivate *priv;
    MilterProtocolAgent *agent;

    priv = SMFI_CONTEXT_GET_PRIVATE(context);
    if (!priv->client_context)
        return NULL;

    agent = MILTER_PROTOCOL_AGENT(priv->client_context);
    return (char *)milter_protocol_agent_get_macro(agent, name);
}

int
smfi_setreply (SMFICTX *context,
               char *return_code, char *extended_code, char *message)
{
    SmfiContextPrivate *priv;

    priv = SMFI_CONTEXT_GET_PRIVATE(context);
    if (!priv->client_context)
        return MI_FAILURE;

    if (milter_client_context_set_reply(priv->client_context,
                                        atoi(return_code),
                                        extended_code,
                                        message,
                                        NULL))
        return MI_SUCCESS;
    else
        return MI_FAILURE;
}

#define MAXREPLYLEN 980

int
smfi_setmlreply (SMFICTX *context,
                 const char *return_code, const char *extended_code, ...)
{
    gboolean error = FALSE;
    int ret = MI_SUCCESS;
    GString *string = g_string_new(NULL);
    va_list var_args;
    const gchar *message;

    va_start(var_args, extended_code);

    message = va_arg(var_args, gchar*);
    while (message) {
        if (strlen(message) > MAXREPLYLEN ||
            !strchr(message, '\r')) {
            error = TRUE;
            break;
        }
        g_string_append(string, message);
        message = va_arg(var_args, gchar*);
    }
    va_end(var_args);
    
    if (error) {
        g_string_free(string, TRUE);
        return MI_FAILURE;
    }

    if (string->len > 0) {
        ret = smfi_setreply(context,
                            (char*)return_code, (char*)extended_code,
                            string->str);
    }
    g_string_free(string, TRUE);

    return ret;
}

int
smfi_addheader (SMFICTX *context, char *name, char *value)
{
    SmfiContextPrivate *priv;

    priv = SMFI_CONTEXT_GET_PRIVATE(context);
    if (!priv->client_context)
        return MI_FAILURE;

    if (milter_client_context_add_header(priv->client_context, name, value))
        return MI_SUCCESS;
    else
        return MI_FAILURE;
}

int
smfi_chgheader (SMFICTX *context, char *name, int index, char *value)
{
    SmfiContextPrivate *priv;

    priv = SMFI_CONTEXT_GET_PRIVATE(context);
    if (!priv->client_context)
        return MI_FAILURE;

    if (milter_client_context_change_header(priv->client_context,
                                            name, index, value))
        return MI_SUCCESS;
    else
        return MI_FAILURE;
}

int
smfi_insheader (SMFICTX *context, int index, char *name, char *value)
{
    SmfiContextPrivate *priv;

    priv = SMFI_CONTEXT_GET_PRIVATE(context);
    if (!priv->client_context)
        return MI_FAILURE;

    if (milter_client_context_insert_header(priv->client_context,
                                            index, name, value))
        return MI_SUCCESS;
    else
        return MI_FAILURE;
}

int
smfi_chgfrom (SMFICTX *context, char *mail, char *args)
{
    SmfiContextPrivate *priv;

    priv = SMFI_CONTEXT_GET_PRIVATE(context);
    if (!priv->client_context)
        return MI_FAILURE;

    if (milter_client_context_change_from(priv->client_context, mail, args))
        return MI_SUCCESS;
    else
        return MI_FAILURE;
}

int
smfi_addrcpt (SMFICTX *context, char *recipient)
{
    SmfiContextPrivate *priv;

    priv = SMFI_CONTEXT_GET_PRIVATE(context);
    if (!priv->client_context)
        return MI_FAILURE;

    if (milter_client_context_add_recipient(priv->client_context, recipient, NULL))
        return MI_SUCCESS;
    else
        return MI_FAILURE;
}

int
smfi_addrcpt_par (SMFICTX *context, char *recipient, char *args)
{
    SmfiContextPrivate *priv;

    priv = SMFI_CONTEXT_GET_PRIVATE(context);
    if (!priv->client_context)
        return MI_FAILURE;

    if (milter_client_context_add_recipient(priv->client_context, recipient, args))
        return MI_SUCCESS;
    else
        return MI_FAILURE;
}

int
smfi_delrcpt (SMFICTX *context, char *recipient)
{
    SmfiContextPrivate *priv;

    priv = SMFI_CONTEXT_GET_PRIVATE(context);
    if (!priv->client_context)
        return MI_FAILURE;

    if (milter_client_context_delete_recipient(priv->client_context, recipient))
        return MI_SUCCESS;
    else
        return MI_FAILURE;
}

int
smfi_progress (SMFICTX *context)
{
    SmfiContextPrivate *priv;

    priv = SMFI_CONTEXT_GET_PRIVATE(context);
    if (!priv->client_context)
        return MI_FAILURE;

    if (milter_client_context_progress(priv->client_context))
        return MI_SUCCESS;
    else
        return MI_FAILURE;
}

int
smfi_replacebody (SMFICTX *context, unsigned char *new_body, int new_body_size)
{
    SmfiContextPrivate *priv;

    priv = SMFI_CONTEXT_GET_PRIVATE(context);
    if (!priv->client_context)
        return MI_FAILURE;

    if (milter_client_context_replace_body(priv->client_context,
                                           (char *)new_body, new_body_size))
        return MI_SUCCESS;
    else
        return MI_FAILURE;
}

int
smfi_quarantine (SMFICTX *context, char *reason)
{
    SmfiContextPrivate *priv;

    priv = SMFI_CONTEXT_GET_PRIVATE(context);
    if (!priv->client_context)
        return MI_FAILURE;

    if (milter_client_context_quarantine(priv->client_context, reason))
        return MI_SUCCESS;
    else
        return MI_FAILURE;
}

int
smfi_setpriv (SMFICTX *context, void *data)
{
    SMFI_CONTEXT_GET_PRIVATE(context)->private_data = data;
    return MI_SUCCESS;
}

void *
smfi_getpriv (SMFICTX *context)
{
    return SMFI_CONTEXT_GET_PRIVATE(context)->private_data;
}

int
smfi_setsymlist (SMFICTX *context, int where, char *macros)
{
    SmfiContextPrivate *priv;
    MilterProtocolAgent *agent;
    MilterMacrosRequests *macros_requests;
    gchar **macro_array;

    priv = SMFI_CONTEXT_GET_PRIVATE(context);
    if (!priv->client_context)
        return MI_FAILURE;

    agent = MILTER_PROTOCOL_AGENT(priv->client_context);
    macros_requests = milter_protocol_agent_get_macros_requests(agent);
    if (!macros_requests)
        macros_requests = milter_macros_requests_new();

    macro_array = g_strsplit(macros, " ", -1);
    milter_macros_requests_set_symbols_string_array(macros_requests,
                                                    where,
                                                    (const gchar**)macro_array);
    g_strfreev(macro_array);

    return MI_SUCCESS;
}

void
libmilter_compatible_reset (void)
{
    filter_description_free();
    if (connection_spec)
        g_free(connection_spec);
    connection_spec = NULL;
    if (listen_channel)
        g_io_channel_unref(listen_channel);
    listen_channel = NULL;
    listen_backlog = -1;
    timeout = 7210;
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
