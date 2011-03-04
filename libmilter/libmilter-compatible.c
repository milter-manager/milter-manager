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

static MilterClient *client = NULL;
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

enum
{
    PROP_0,
    PROP_CLIENT_CONTEXT
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

static void smfi_context_attach_to_client_context   (SmfiContext *context);
static void smfi_context_detach_from_client_context (SmfiContext *context);

static void
smfi_context_class_init (SmfiContextClass *klass)
{
    GObjectClass *gobject_class;
    GParamSpec *spec;

    gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->dispose      = dispose;
    gobject_class->set_property = set_property;
    gobject_class->get_property = get_property;

    spec = g_param_spec_object("client-context",
                               "client context",
                               "A MilterClientContext object",
                               MILTER_TYPE_CLIENT_CONTEXT,
                               G_PARAM_READWRITE);
    g_object_class_install_property(gobject_class, PROP_CLIENT_CONTEXT, spec);

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
smfi_context_set_client_context (SmfiContext *context,
                                 MilterClientContext *client_context)
{
    SmfiContextPrivate *priv;

    priv = SMFI_CONTEXT_GET_PRIVATE(context);
    if (priv->client_context) {
        smfi_context_detach_from_client_context(context);
        g_object_unref(priv->client_context);
    }

    priv->client_context = client_context;
    if (priv->client_context) {
        g_object_ref(priv->client_context);
        smfi_context_attach_to_client_context(context);
    }
}

static void
dispose (GObject *object)
{
    SmfiContext *context;

    context = SMFI_CONTEXT(object);

    smfi_context_set_client_context(context, NULL);

    G_OBJECT_CLASS(smfi_context_parent_class)->dispose(object);
}

static void
set_property (GObject      *object,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
    SmfiContext *context;

    context = SMFI_CONTEXT(object);
    switch (prop_id) {
    case PROP_CLIENT_CONTEXT:
        smfi_context_set_client_context(context, g_value_get_object(value));
        break;
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
    SmfiContextPrivate *priv;

    priv = SMFI_CONTEXT_GET_PRIVATE(object);
    switch (prop_id) {
    case PROP_CLIENT_CONTEXT:
        g_value_set_object(value, priv->client_context);
        break;
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
smfi_context_new (MilterClientContext *client_context)
{
    return g_object_new(SMFI_TYPE_CONTEXT,
                        "client-context", client_context,
                        NULL);
}

static void
libmilter_compatible_initialize (void)
{
    static gboolean initialized = FALSE;

    if (initialized)
        return;

    initialized = TRUE;
    milter_init();
}

int
smfi_opensocket (bool remove_socket)
{
    GError *error = NULL;
    GIOChannel *channel;

    if (!filter_description)
        return MI_FAILURE;

    if (!connection_spec)
        return MI_FAILURE;

    libmilter_compatible_initialize();

    channel = milter_connection_listen(connection_spec, listen_backlog,
                                       NULL, NULL, remove_socket, &error);
    if (error) {
        milter_error("[%s] %s", filter_description->xxfi_name, error->message);
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
    sfsistat status;
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
    if (status == SMFIS_CONTINUE) {
        milter_option_set_action(option, action_out);
        milter_option_set_step(option, step_out);
    }
    return libmilter_compatible_convert_status_to(status);
}

static MilterStatus
cb_connect (MilterClientContext *context, const gchar *host_name,
            struct sockaddr *address, socklen_t address_length,
            gpointer user_data)
{
    SmfiContext *smfi_context = user_data;
    sfsistat status;

    if (!filter_description->xxfi_connect)
        return MILTER_STATUS_DEFAULT;

    status = filter_description->xxfi_connect(smfi_context,
                                              (gchar *)host_name,
                                              address);
    return libmilter_compatible_convert_status_to(status);
}

static MilterStatus
cb_helo (MilterClientContext *context, const gchar *fqdn, gpointer user_data)
{
    SmfiContext *smfi_context = user_data;
    sfsistat status;

    if (!filter_description->xxfi_helo)
        return MILTER_STATUS_DEFAULT;

    status = filter_description->xxfi_helo(smfi_context, (gchar *)fqdn);
    return libmilter_compatible_convert_status_to(status);
}

static MilterStatus
cb_envelope_from (MilterClientContext *context, const gchar *from,
                  gpointer user_data)
{
    SmfiContext *smfi_context = user_data;
    sfsistat status;
    gchar *arguments[2];

    if (!filter_description->xxfi_envfrom)
        return MILTER_STATUS_DEFAULT;

    arguments[0] = (gchar *)from;
    arguments[1] = NULL;

    status = filter_description->xxfi_envfrom(smfi_context, arguments);
    return libmilter_compatible_convert_status_to(status);
}

static MilterStatus
cb_envelope_recipient (MilterClientContext *context, const gchar *recipient,
                       gpointer user_data)
{
    SmfiContext *smfi_context = user_data;
    sfsistat status;
    gchar *arguments[2];

    if (!filter_description->xxfi_envrcpt)
        return MILTER_STATUS_DEFAULT;

    arguments[0] = (gchar *)recipient;
    arguments[1] = NULL;

    status = filter_description->xxfi_envrcpt(smfi_context, arguments);
    return libmilter_compatible_convert_status_to(status);
}

static MilterStatus
cb_data (MilterClientContext *context, gpointer user_data)
{
    SmfiContext *smfi_context = user_data;
    sfsistat status;

    if (!filter_description->xxfi_data)
        return MILTER_STATUS_DEFAULT;

    status = filter_description->xxfi_data(smfi_context);
    return libmilter_compatible_convert_status_to(status);
}

static MilterStatus
cb_unknown (MilterClientContext *context, const gchar *command,
            gpointer user_data)
{
    SmfiContext *smfi_context = user_data;
    sfsistat status;

    if (!filter_description->xxfi_unknown)
        return MILTER_STATUS_DEFAULT;

    status = filter_description->xxfi_unknown(smfi_context, command);
    return libmilter_compatible_convert_status_to(status);
}

static MilterStatus
cb_header (MilterClientContext *context, const gchar *name, const gchar *value,
           gpointer user_data)
{
    SmfiContext *smfi_context = user_data;
    sfsistat status;

    if (!filter_description->xxfi_header)
        return MILTER_STATUS_DEFAULT;

    status = filter_description->xxfi_header(smfi_context,
                                             (gchar *)name,
                                             (gchar *)value);
    return libmilter_compatible_convert_status_to(status);
}

static MilterStatus
cb_end_of_header (MilterClientContext *context, gpointer user_data)
{
    SmfiContext *smfi_context = user_data;
    sfsistat status;

    if (!filter_description->xxfi_eoh)
        return MILTER_STATUS_DEFAULT;

    status = filter_description->xxfi_eoh(smfi_context);
    return libmilter_compatible_convert_status_to(status);
}

static MilterStatus
cb_body (MilterClientContext *context, const guchar *chunk, gsize size,
         gpointer user_data)
{
    SmfiContext *smfi_context = user_data;
    sfsistat status;

    if (!filter_description->xxfi_body)
        return MILTER_STATUS_DEFAULT;

    status = filter_description->xxfi_body(smfi_context, (guchar *)chunk, size);
    return libmilter_compatible_convert_status_to(status);
}

static MilterStatus
cb_end_of_message (MilterClientContext *context,
                   const gchar *chunk, gsize size,
                   gpointer user_data)
{
    SmfiContext *smfi_context = user_data;
    sfsistat status;

    if (chunk && size > 0 && filter_description->xxfi_body) {
        status = filter_description->xxfi_body(smfi_context,
                                               (guchar *)chunk, size);
        switch (status) {
        case SMFIS_REJECT:
        case SMFIS_DISCARD:
        case SMFIS_ACCEPT:
        case SMFIS_TEMPFAIL:
            return libmilter_compatible_convert_status_to(status);
            break;
        default:
            break;
        }
    }

    if (!filter_description->xxfi_eom)
        return MILTER_STATUS_DEFAULT;

    status = filter_description->xxfi_eom(smfi_context);
    return libmilter_compatible_convert_status_to(status);
}

static MilterStatus
cb_abort (MilterClientContext *context, MilterClientContextState state,
          gpointer user_data)
{
    SmfiContext *smfi_context = user_data;
    sfsistat status;

    if (!filter_description->xxfi_abort)
        return MILTER_STATUS_DEFAULT;

    if (!MILTER_CLIENT_CONTEXT_STATE_IN_MESSAGE_PROCESSING(state))
        return MILTER_STATUS_DEFAULT;

    status = filter_description->xxfi_abort(smfi_context);
    return libmilter_compatible_convert_status_to(status);
}

static void
cb_finished (MilterFinishedEmittable *emittable, gpointer user_data)
{
    SmfiContext *smfi_context = user_data;

    if (filter_description->xxfi_close)
        filter_description->xxfi_close(smfi_context);

    g_object_unref(smfi_context);
}

static void
smfi_context_attach_to_client_context (SmfiContext *context)
{
    SmfiContextPrivate *priv;
    MilterClientContext *client_context;

    priv = SMFI_CONTEXT_GET_PRIVATE(context);
    client_context = priv->client_context;

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

    g_signal_connect(client_context, "finished",
                     G_CALLBACK(cb_finished), context);
}

static void
smfi_context_detach_from_client_context (SmfiContext *context)
{
    SmfiContextPrivate *priv;
    MilterClientContext *client_context;

    priv = SMFI_CONTEXT_GET_PRIVATE(context);
    client_context = priv->client_context;

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
}

static void
cb_connection_established (MilterClient *client, MilterClientContext *context,
                           gpointer user_data)
{
    smfi_context_new(context);
}

static void
setup_milter_client_event_loop_backend (MilterClient *client)
{
    const gchar *backend_env;
    MilterClientEventLoopBackend backend;
    GError *error = NULL;

    backend_env = g_getenv("MILTER_EVENT_LOOP_BACKEND");
    if (!backend_env)
        return;

    backend = milter_utils_enum_from_string(MILTER_TYPE_CLIENT_EVENT_LOOP_BACKEND,
                                            backend_env,
                                            &error);
    if (error) {
        milter_error("invalid MILTER_EVENT_LOOP_BACKEND value: <%s>: <%s>",
                     backend_env, error->message);
        g_error_free(error);
        return;
    }

    milter_client_set_event_loop_backend(client, backend);
}

static void
setup_milter_client (MilterClient *client)
{
    setup_milter_client_event_loop_backend(client);
    milter_client_set_connection_spec(client, connection_spec, NULL);
    milter_client_set_listen_channel(client, listen_channel);
    milter_client_set_listen_backlog(client, listen_backlog);
    milter_client_set_timeout(client, timeout);
    g_signal_connect(client, "connection-established",
                     G_CALLBACK(cb_connection_established), NULL);
}

int
smfi_main (void)
{
    gboolean success;
    GError *error = NULL;

    libmilter_compatible_initialize();

    if (client)
        g_object_unref(client);
    client = milter_client_new();
    setup_milter_client(client);
    success = milter_client_run(client, &error);
    if (!success) {
        milter_error("failed to run main loop: %s", error->message);
        g_error_free(error);
    }
    g_object_unref(client);
    client = NULL;

    libmilter_compatible_reset();

    milter_quit();

    if (success)
        return MI_SUCCESS;
    else
        return MI_FAILURE;
}

int
smfi_setbacklog (int backlog)
{
    libmilter_compatible_initialize();

    if (backlog <= 0)
        return MI_FAILURE;

    listen_backlog = backlog;
    return MI_SUCCESS;
}

int
smfi_setdbg (int level)
{
    MilterLogLevelFlags target_level = 0;

    libmilter_compatible_initialize();

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
    libmilter_compatible_initialize();

    timeout = timeout;

    return MI_SUCCESS;
}

int
smfi_setconn (char *spec)
{
    GError *error = NULL;
    gchar *normalized_spec;

    libmilter_compatible_initialize();

    if (!spec) {
        milter_error("must not be NULL");
        return MI_FAILURE;
    }

    if (strchr(spec, ':'))
        normalized_spec = g_strdup(spec);
    else
        normalized_spec = g_strdup_printf("unix:%s", spec);

    if (!milter_connection_parse_spec(normalized_spec,
                                      NULL, NULL, NULL,
                                      &error)) {
        milter_error("%s", error->message);
        g_error_free(error);
        g_free(normalized_spec);
        return MI_FAILURE;
    }

    if (connection_spec)
        g_free(connection_spec);
    connection_spec = normalized_spec;
    return MI_SUCCESS;
}

int
smfi_stop (void)
{
    libmilter_compatible_initialize();

    if (client)
        milter_client_shutdown(client);
    return MI_SUCCESS;
}

int
smfi_version (unsigned int *major, unsigned int *minor,
              unsigned int *patch_level)
{
    libmilter_compatible_initialize();

    if (major)
        *major = SM_LM_VRS_MAJOR(SMFI_VERSION);
    if (minor)
        *minor = SM_LM_VRS_MINOR(SMFI_VERSION);
    if (patch_level)
        *patch_level = SM_LM_VRS_PLVL(SMFI_VERSION);
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
    GError *error = NULL;

    priv = SMFI_CONTEXT_GET_PRIVATE(context);
    if (!priv->client_context)
        return MI_FAILURE;

    if (milter_client_context_set_reply(priv->client_context,
                                        atoi(return_code),
                                        extended_code,
                                        message,
                                        &error)) {
        return MI_SUCCESS;
    } else {
        milter_error("failed to set reply: %s", error->message);
        g_error_free(error);
        return MI_FAILURE;
    }
}

#define MAXREPLYLEN 980

int
smfi_setmlreply (SMFICTX *context,
                 const char *return_code, const char *extended_code, ...)
{
    int result = MI_SUCCESS;
    GString *string = g_string_new(NULL);
    va_list var_args;
    const gchar *message;

    va_start(var_args, extended_code);

    message = va_arg(var_args, gchar *);
    while (message) {
        size_t length;

        length = strlen(message);
        if (length > MAXREPLYLEN) {
            milter_error("a line is too long: <%" G_GSIZE_FORMAT ">: "
                         "max: <%d>: <%s>",
                         length, MAXREPLYLEN, message);
            result = MI_FAILURE;
        } else if (strchr(message, '\r')) {
            milter_error("a line should not include '\\r': <%s>", message);
            result = MI_FAILURE;
        }
        if (result == MI_FAILURE)
            break;
        g_string_append(string, message);
        message = va_arg(var_args, gchar *);
        if (message)
            g_string_append(string, "\n");
    }
    va_end(var_args);

    if (result == MI_SUCCESS) {
        if (string->len > 0)
            result = smfi_setreply(context,
                                   (char *)return_code,
                                   (char *)extended_code,
                                   string->str);
    }
    g_string_free(string, TRUE);

    return result;
}

int
smfi_addheader (SMFICTX *context, char *name, char *value)
{
    SmfiContextPrivate *priv;
    GError *error = NULL;

    priv = SMFI_CONTEXT_GET_PRIVATE(context);
    if (!priv->client_context)
        return MI_FAILURE;

    if (milter_client_context_add_header(priv->client_context, name, value,
                                         &error)) {
        return MI_SUCCESS;
    } else {
        if (error) {
            milter_error("failed to add header: %s", error->message);
            g_error_free(error);
        }
        return MI_FAILURE;
    }
}

int
smfi_chgheader (SMFICTX *context, char *name, int index, char *value)
{
    SmfiContextPrivate *priv;
    GError *error = NULL;

    priv = SMFI_CONTEXT_GET_PRIVATE(context);
    if (!priv->client_context)
        return MI_FAILURE;

    if (milter_client_context_change_header(priv->client_context,
                                            name, index, value, &error)) {
        return MI_SUCCESS;
    } else {
        if (error) {
            milter_error("failed to change header: %s", error->message);
            g_error_free(error);
        }
        return MI_FAILURE;
    }
}

int
smfi_insheader (SMFICTX *context, int index, char *name, char *value)
{
    SmfiContextPrivate *priv;
    GError *error = NULL;

    priv = SMFI_CONTEXT_GET_PRIVATE(context);
    if (!priv->client_context)
        return MI_FAILURE;

    if (milter_client_context_insert_header(priv->client_context,
                                            index, name, value, &error)) {
        return MI_SUCCESS;
    } else {
        if (error) {
            milter_error("failed to insert header: %s", error->message);
            g_error_free(error);
        }
        return MI_FAILURE;
    }
}

int
smfi_chgfrom (SMFICTX *context, char *mail, char *arguments)
{
    SmfiContextPrivate *priv;
    GError *error = NULL;

    priv = SMFI_CONTEXT_GET_PRIVATE(context);
    if (!priv->client_context)
        return MI_FAILURE;

    if (milter_client_context_change_from(priv->client_context, mail, arguments,
                                          &error)) {
        return MI_SUCCESS;
    } else {
        if (error) {
            milter_error("failed to change from: %s", error->message);
            g_error_free(error);
        }
        return MI_FAILURE;
    }
}

int
smfi_addrcpt (SMFICTX *context, char *recipient)
{
    SmfiContextPrivate *priv;
    GError *error = NULL;

    priv = SMFI_CONTEXT_GET_PRIVATE(context);
    if (!priv->client_context)
        return MI_FAILURE;

    if (milter_client_context_add_recipient(priv->client_context,
                                            recipient, NULL, &error)) {
        return MI_SUCCESS;
    } else {
        if (error) {
            milter_error("failed to add recipient: %s", error->message);
            g_error_free(error);
        }
        return MI_FAILURE;
    }
}

int
smfi_addrcpt_par (SMFICTX *context, char *recipient, char *args)
{
    SmfiContextPrivate *priv;
    GError *error = NULL;

    priv = SMFI_CONTEXT_GET_PRIVATE(context);
    if (!priv->client_context)
        return MI_FAILURE;

    if (milter_client_context_add_recipient(priv->client_context,
                                            recipient, args, &error)) {
        return MI_SUCCESS;
    } else {
        if (error) {
            milter_error("failed to add recipient: %s", error->message);
            g_error_free(error);
        }
        return MI_FAILURE;
    }
}

int
smfi_delrcpt (SMFICTX *context, char *recipient)
{
    SmfiContextPrivate *priv;
    GError *error = NULL;

    priv = SMFI_CONTEXT_GET_PRIVATE(context);
    if (!priv->client_context)
        return MI_FAILURE;

    if (milter_client_context_delete_recipient(priv->client_context, recipient,
                                               &error)) {
        return MI_SUCCESS;
    } else {
        if (error) {
            milter_error("failed to delete recipient: %s", error->message);
            g_error_free(error);
        }
        return MI_FAILURE;
    }
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
    GError *error = NULL;

    priv = SMFI_CONTEXT_GET_PRIVATE(context);
    if (!priv->client_context)
        return MI_FAILURE;

    if (milter_client_context_replace_body(priv->client_context,
                                           (char *)new_body, new_body_size,
                                           &error)) {
        return MI_SUCCESS;
    } else {
        if (error) {
            milter_error("failed to replace body: %s", error->message);
            g_error_free(error);
        }
        return MI_FAILURE;
    }
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
smfi_setsymlist (SMFICTX *context, int state, char *macros)
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
                                                    state,
                                                    (const gchar**)macro_array);
    g_strfreev(macro_array);

    return MI_SUCCESS;
}

void
libmilter_compatible_reset (void)
{
    if (client)
        g_object_unref(client);
    client = NULL;
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

MilterStatus
libmilter_compatible_convert_status_to (sfsistat status)
{
    switch (status) {
    case SMFIS_CONTINUE:
        return MILTER_STATUS_CONTINUE;
    case SMFIS_REJECT:
        return MILTER_STATUS_REJECT;
    case SMFIS_DISCARD:
        return MILTER_STATUS_DISCARD;
    case SMFIS_ACCEPT:
        return MILTER_STATUS_ACCEPT;
    case SMFIS_TEMPFAIL:
        return MILTER_STATUS_TEMPORARY_FAILURE;
    case SMFIS_NOREPLY:
        return MILTER_STATUS_NO_REPLY;
    case SMFIS_SKIP:
        return MILTER_STATUS_SKIP;
    case SMFIS_ALL_OPTS:
        return MILTER_STATUS_ALL_OPTIONS;
    default:
        milter_error("invalid SMFIS_* value: <%d>", status);
        return MILTER_STATUS_DEFAULT;
    }
}

sfsistat
libmilter_compatible_convert_status_from (MilterStatus status)
{
    switch (status) {
    case MILTER_STATUS_CONTINUE:
        return SMFIS_CONTINUE;
    case MILTER_STATUS_REJECT:
        return SMFIS_REJECT;
    case MILTER_STATUS_DISCARD:
        return SMFIS_DISCARD;
    case MILTER_STATUS_ACCEPT:
        return SMFIS_ACCEPT;
    case MILTER_STATUS_TEMPORARY_FAILURE:
        return SMFIS_TEMPFAIL;
    case MILTER_STATUS_NO_REPLY:
        return SMFIS_NOREPLY;
    case MILTER_STATUS_SKIP:
        return SMFIS_SKIP;
    case MILTER_STATUS_ALL_OPTIONS:
        return SMFIS_ALL_OPTS;
    default:
        milter_error("invalid MILTER_STATUS_* value: <%d>", status);
        return 0;
    }
}


int mi_stop (void);
int
mi_stop (void)
{
    /* export this symbol because spamass-milter use it for
       detecting -lmilter. :< */
    return 0;
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
