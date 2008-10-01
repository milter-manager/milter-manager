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

#include "libmilter-compatible.h"

static struct smfiDesc filter_description = {0};
static gchar *connection_spec = NULL;

#define SMFI_CONTEXT_GET_PRIVATE(obj)                   \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj),                 \
                                 SMFI_TYPE_CONTEXT,     \
                                 SmfiContextPrivate))

typedef struct _SmfiContextPrivate	SmfiContextPrivate;
struct _SmfiContextPrivate
{
    MilterClientContext *client_context;
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
    return MI_SUCCESS;
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

    if (filter_description.xxfi_name)
        g_free(filter_description.xxfi_name);
    filter_description = description;
    filter_description.xxfi_name = g_strdup(filter_description.xxfi_name);
    if (!filter_description.xxfi_name)
        return MI_FAILURE;

    return MI_SUCCESS;
}

static MilterStatus
cb_option_negotiation (MilterClientContext *context, MilterOption *option,
                       gpointer user_data)
{
    SmfiContext *smfi_context = user_data;
    MilterStatus status;
    gulong action, step, preserve1 = 0, preserve2 = 0;
    gulong action_out, step_out, preserve1_out = 0, preserve2_out = 0;

    if (!filter_description.xxfi_negotiate)
        return MILTER_STATUS_DEFAULT;

    action = action_out = milter_option_get_action(option);
    step = step_out = milter_option_get_step(option);
    status = filter_description.xxfi_negotiate(smfi_context,
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

    if (!filter_description.xxfi_connect)
        return MILTER_STATUS_DEFAULT;

    return filter_description.xxfi_connect(smfi_context,
                                           (gchar *)host_name,
                                           address);
}

static MilterStatus
cb_helo (MilterClientContext *context, const gchar *fqdn, gpointer user_data)
{
    SmfiContext *smfi_context = user_data;

    if (!filter_description.xxfi_helo)
        return MILTER_STATUS_DEFAULT;

    return filter_description.xxfi_helo(smfi_context, (gchar *)fqdn);
}

static MilterStatus
cb_envelope_from (MilterClientContext *context, const gchar *from,
                  gpointer user_data)
{
    SmfiContext *smfi_context = user_data;
    gchar *addresses[2];

    if (!filter_description.xxfi_envfrom)
        return MILTER_STATUS_DEFAULT;

    addresses[0] = (gchar *)from;
    addresses[1] = NULL;
    return filter_description.xxfi_envfrom(smfi_context, addresses);
}

static MilterStatus
cb_envelope_receipt (MilterClientContext *context, const gchar *receipt,
                     gpointer user_data)
{
    SmfiContext *smfi_context = user_data;
    gchar *addresses[2];

    if (!filter_description.xxfi_envrcpt)
        return MILTER_STATUS_DEFAULT;

    addresses[0] = (gchar *)receipt;
    addresses[1] = NULL;
    return filter_description.xxfi_envrcpt(smfi_context, addresses);
}

static MilterStatus
cb_data (MilterClientContext *context, gpointer user_data)
{
    SmfiContext *smfi_context = user_data;

    if (!filter_description.xxfi_data)
        return MILTER_STATUS_DEFAULT;

    return filter_description.xxfi_data(smfi_context);
}

static MilterStatus
cb_unknown (MilterClientContext *context, const gchar *command,
            gpointer user_data)
{
    SmfiContext *smfi_context = user_data;

    if (!filter_description.xxfi_unknown)
        return MILTER_STATUS_DEFAULT;

    return filter_description.xxfi_unknown(smfi_context, command);
}

static MilterStatus
cb_header (MilterClientContext *context, const gchar *name, const gchar *value,
           gpointer user_data)
{
    SmfiContext *smfi_context = user_data;

    if (!filter_description.xxfi_header)
        return MILTER_STATUS_DEFAULT;

    return filter_description.xxfi_header(smfi_context,
                                          (gchar *)name,
                                          (gchar *)value);
}

static MilterStatus
cb_end_of_header (MilterClientContext *context, gpointer user_data)
{
    SmfiContext *smfi_context = user_data;

    if (!filter_description.xxfi_eoh)
        return MILTER_STATUS_DEFAULT;

    return filter_description.xxfi_eoh(smfi_context);
}

static MilterStatus
cb_body (MilterClientContext *context, const guchar *chunk, gsize size,
         gpointer user_data)
{
    SmfiContext *smfi_context = user_data;

    if (!filter_description.xxfi_body)
        return MILTER_STATUS_DEFAULT;

    return filter_description.xxfi_body(smfi_context, (guchar *)chunk, size);
}

static MilterStatus
cb_end_of_message (MilterClientContext *context, gpointer user_data)
{
    SmfiContext *smfi_context = user_data;

    if (!filter_description.xxfi_eom)
        return MILTER_STATUS_DEFAULT;

    return filter_description.xxfi_eom(smfi_context);
}

static MilterStatus
cb_close (MilterClientContext *context, gpointer user_data)
{
    SmfiContext *smfi_context = user_data;

    if (!filter_description.xxfi_close)
        return MILTER_STATUS_DEFAULT;

    return filter_description.xxfi_close(smfi_context);
}

static MilterStatus
cb_abort (MilterClientContext *context, gpointer user_data)
{
    SmfiContext *smfi_context = user_data;

    if (!filter_description.xxfi_abort)
        return MILTER_STATUS_DEFAULT;

    return filter_description.xxfi_abort(smfi_context);
}

static void
setup_client_context (MilterClientContext *context, gpointer user_data)
{
    SmfiContext *smfi_context = user_data;
    SmfiContextPrivate *priv;

    priv = SMFI_CONTEXT_GET_PRIVATE(smfi_context);
    priv->client_context = g_object_ref(context);

#define CONNECT(name, smfi_name)                                        \
    if (filter_description.xxfi_ ## smfi_name)                          \
        g_signal_connect(context, #name, G_CALLBACK(cb_ ## name),       \
                         user_data)

    CONNECT(option_negotiation, negotiate);
    CONNECT(connect, connect);
    CONNECT(helo, helo);
    CONNECT(envelope_from, envfrom);
    CONNECT(envelope_receipt, envrcpt);
    CONNECT(data, data);
    CONNECT(unknown, unknown);
    CONNECT(header, header);
    CONNECT(end_of_header, eoh);
    CONNECT(body, body);
    CONNECT(end_of_message, eom);
    CONNECT(close, close);
    CONNECT(abort, abort);

#undef CONNECT
}

static void
teardown_client_context (MilterClientContext *context, gpointer user_data)
{
    SmfiContext *smfi_context = user_data;
    SmfiContextPrivate *priv;

    priv = SMFI_CONTEXT_GET_PRIVATE(smfi_context);

#define DISCONNECT(name)                                                \
    g_signal_handlers_disconnect_by_func(context,                       \
                                         G_CALLBACK(cb_ ## name),       \
                                         user_data)

    DISCONNECT(option_negotiation);
    DISCONNECT(connect);
    DISCONNECT(helo);
    DISCONNECT(envelope_from);
    DISCONNECT(envelope_receipt);
    DISCONNECT(data);
    DISCONNECT(unknown);
    DISCONNECT(header);
    DISCONNECT(end_of_header);
    DISCONNECT(body);
    DISCONNECT(end_of_message);
    DISCONNECT(close);
    DISCONNECT(abort);

#undef IDSCONNECT

    g_object_unref(priv->client_context);
    priv->client_context = NULL;
}

int
smfi_main (void)
{
    MilterClient *client;
    SmfiContext *smfi_context;

    g_type_init();

    client = milter_client_new();
    milter_client_set_connection_spec(client, connection_spec, NULL);
    smfi_context = smfi_context_new();
    milter_client_set_context_setup_func(client, setup_client_context,
                                         smfi_context);
    milter_client_set_context_teardown_func(client, teardown_client_context,
                                            smfi_context);
    milter_client_main(client);
    g_object_unref(smfi_context);
    g_object_unref(client);

    return MI_SUCCESS;
}

int
smfi_setbacklog (int backlog)
{
    return MI_SUCCESS;
}

int
smfi_setdbg (int level)
{
    return MI_SUCCESS;
}

int
smfi_settimeout (int timeout)
{
    return MI_SUCCESS;
}

int
smfi_setconn (char *spec)
{
    if (connection_spec)
        g_free(connection_spec);
    connection_spec = g_strdup(spec);
    if (connection_spec)
        return MI_SUCCESS;
    else
        return MI_FAILURE;
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
    return NULL;
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

int
smfi_setmlreply (SMFICTX *context,
                 const char *return_code, const char *extended_code, ...)
{
    return MI_SUCCESS;
}

int
smfi_addheader (SMFICTX *context, char *name, char *value)
{
    return MI_SUCCESS;
}

int
smfi_chgheader (SMFICTX *context, char *name, int index, char *value)
{
    return MI_SUCCESS;
}

int
smfi_insheader (SMFICTX *context, int index, char *name, char *value)
{
    return MI_SUCCESS;
}

int
smfi_chgfrom (SMFICTX *context, char *mail, char *args)
{
    return MI_SUCCESS;
}

int
smfi_addrcpt (SMFICTX *context, char *receipt)
{
    return MI_SUCCESS;
}

int
smfi_addrcpt_par (SMFICTX *context, char *receipt, char *args)
{
    return MI_SUCCESS;
}

int
smfi_delrcpt (SMFICTX *context, char *receipt)
{
    return MI_SUCCESS;
}

int
smfi_progress (SMFICTX *context)
{
    return MI_SUCCESS;
}

int
smfi_replacebody (SMFICTX *context, unsigned char *new_body, int new_body_size)
{
    return MI_SUCCESS;
}

int
smfi_quarantine (SMFICTX *context, char *reason)
{
    return MI_SUCCESS;
}

int
smfi_setpriv (SMFICTX *context, void *data)
{
    return MI_SUCCESS;
}

void *
smfi_getpriv (SMFICTX *context)
{
    return NULL;
}

int
smfi_setsymlist (SMFICTX *context, int where, char *macros)
{
    return MI_SUCCESS;
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
