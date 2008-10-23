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
#  include "../../config.h"
#endif /* HAVE_CONFIG_H */

#include "milter-manager-controller.h"

#define MILTER_MANAGER_CONTROLLER_GET_PRIVATE(obj)                   \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj),                              \
                                 MILTER_MANAGER_TYPE_CONTROLLER,     \
                                 MilterManagerControllerPrivate))

typedef struct _MilterManagerControllerPrivate MilterManagerControllerPrivate;
struct _MilterManagerControllerPrivate
{
    MilterManagerConfiguration *configuration;
};

enum
{
    PROP_0,
    PROP_CONFIGURATION
};

G_DEFINE_TYPE(MilterManagerController, milter_manager_controller,
              G_TYPE_OBJECT);

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
milter_manager_controller_class_init (MilterManagerControllerClass *klass)
{
    GObjectClass *gobject_class;
    GParamSpec *spec;

    gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->dispose      = dispose;
    gobject_class->set_property = set_property;
    gobject_class->get_property = get_property;

    spec = g_param_spec_object("configuration",
                               "Configuration",
                               "The configuration of the milter manager",
                               MILTER_MANAGER_TYPE_CONFIGURATION,
                               G_PARAM_READWRITE);
    g_object_class_install_property(gobject_class, PROP_CONFIGURATION, spec);

    g_type_class_add_private(gobject_class,
                             sizeof(MilterManagerControllerPrivate));
}

static void
milter_manager_controller_init (MilterManagerController *controller)
{
    MilterManagerControllerPrivate *priv;

    priv = MILTER_MANAGER_CONTROLLER_GET_PRIVATE(controller);
    priv->configuration = NULL;
}

static void
dispose (GObject *object)
{
    MilterManagerControllerPrivate *priv;

    priv = MILTER_MANAGER_CONTROLLER_GET_PRIVATE(object);

    if (priv->configuration) {
        g_object_unref(priv->configuration);
        priv->configuration = NULL;
    }

    G_OBJECT_CLASS(milter_manager_controller_parent_class)->dispose(object);
}

static void
set_property (GObject      *object,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
    MilterManagerControllerPrivate *priv;

    priv = MILTER_MANAGER_CONTROLLER_GET_PRIVATE(object);
    switch (prop_id) {
      case PROP_CONFIGURATION:
        if (priv->configuration)
            g_object_unref(priv->configuration);
        priv->configuration = g_value_get_object(value);
        if (priv->configuration)
            g_object_ref(priv->configuration);
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
    MilterManagerControllerPrivate *priv;

    priv = MILTER_MANAGER_CONTROLLER_GET_PRIVATE(object);
    switch (prop_id) {
      case PROP_CONFIGURATION:
        g_value_set_object(value, priv->configuration);
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

MilterManagerController *
milter_manager_controller_new (MilterManagerConfiguration *configuration)
{
    return g_object_new(MILTER_MANAGER_TYPE_CONTROLLER,
                        "configuration", configuration,
                        NULL);
}

static MilterStatus
cb_continue (MilterServerContext *context, gpointer user_data)
{
    gboolean *waiting = user_data;
    *waiting = FALSE;
    return MILTER_STATUS_NOT_CHANGE;
}

static MilterStatus
cb_negotiate_reply (MilterServerContext *context, MilterOption *option,
                    GList *macros_requests, gpointer user_data)
{
    gboolean *waiting = user_data;
    *waiting = FALSE;
    return MILTER_STATUS_NOT_CHANGE;
}

MilterStatus
milter_manager_controller_negotiate (MilterManagerController *controller, MilterOption *option)
{
    MilterManagerControllerPrivate *priv;
    const GList *milters;
    gboolean waiting = TRUE;
    MilterServerContext *context;
    GError *error = NULL;

    priv = MILTER_MANAGER_CONTROLLER_GET_PRIVATE(controller);
    milters = milter_manager_configuration_get_child_milters(priv->configuration);

    if (!milters)
        return MILTER_STATUS_NOT_CHANGE;

    context = milters->data;
    if (!milter_server_context_establish_connection(context, &error)) {
        gboolean priviledge;
        MilterStatus status;

        status =
            milter_manager_configuration_get_return_status_if_filter_unavailable(priv->configuration);

        priviledge =
            milter_manager_configuration_is_privilege_mode(priv->configuration);
        if (!priviledge ||
            error->code != MILTER_SERVER_CONTEXT_ERROR_CONNECTION_FAILURE) {
            milter_error("Error: %s", error->message);
            g_error_free(error);
            return status;
        }
        g_error_free(error);

        milter_manager_child_milter_start(MILTER_MANAGER_CHILD_MILTER(context),
                                          &error);
        if (error) {
            milter_error("Error: %s", error->message);
            g_error_free(error);
            return status;
        }
        return MILTER_STATUS_PROGRESS;
    }

    g_signal_connect(context, "continue", G_CALLBACK(cb_continue), &waiting);
    g_signal_connect(context, "negotiate_reply",
                     G_CALLBACK(cb_negotiate_reply), &waiting);
    milter_server_context_negotiate(context, option);
    while (waiting)
        g_main_context_iteration(NULL, TRUE);
    return MILTER_STATUS_CONTINUE;
}

MilterStatus
milter_manager_controller_connect (MilterManagerController *controller,
                                   const gchar             *host_name,
                                   struct sockaddr         *address,
                                   socklen_t                address_length)
{
    MilterManagerControllerPrivate *priv;
    const GList *milters;
    MilterServerContext *context;

    priv = MILTER_MANAGER_CONTROLLER_GET_PRIVATE(controller);
    milters = milter_manager_configuration_get_child_milters(priv->configuration);

    if (!milters)
        return MILTER_STATUS_NOT_CHANGE;

    context = milters->data;
    return milter_server_context_connect(context, host_name,
                                         address, address_length);
}

MilterStatus
milter_manager_controller_helo (MilterManagerController *controller, const gchar *fqdn)
{
    MilterManagerControllerPrivate *priv;
    const GList *milters;
    MilterServerContext *context;

    priv = MILTER_MANAGER_CONTROLLER_GET_PRIVATE(controller);
    milters = milter_manager_configuration_get_child_milters(priv->configuration);

    if (!milters)
        return MILTER_STATUS_NOT_CHANGE;

    context = milters->data;
    return milter_server_context_helo(context, fqdn);
}

MilterStatus
milter_manager_controller_envelope_from (MilterManagerController *controller,
                                         const gchar *from)
{
    MilterManagerControllerPrivate *priv;
    const GList *milters;
    MilterServerContext *context;

    priv = MILTER_MANAGER_CONTROLLER_GET_PRIVATE(controller);
    milters = milter_manager_configuration_get_child_milters(priv->configuration);

    if (!milters)
        return MILTER_STATUS_NOT_CHANGE;

    context = milters->data;
    return milter_server_context_envelope_from(context, from);
}

MilterStatus
milter_manager_controller_envelope_receipt (MilterManagerController *controller,
                                            const gchar *receipt)
{
    MilterManagerControllerPrivate *priv;
    const GList *milters;
    MilterServerContext *context;

    priv = MILTER_MANAGER_CONTROLLER_GET_PRIVATE(controller);
    milters = milter_manager_configuration_get_child_milters(priv->configuration);

    if (!milters)
        return MILTER_STATUS_NOT_CHANGE;

    context = milters->data;
    return milter_server_context_envelope_receipt(context, receipt);
}

MilterStatus
milter_manager_controller_data (MilterManagerController *controller)
{
    g_print("data");
    return MILTER_STATUS_NOT_CHANGE;
}

MilterStatus
milter_manager_controller_unknown (MilterManagerController *controller, const gchar *command)
{
    g_print("unknown");
    return MILTER_STATUS_NOT_CHANGE;
}

MilterStatus
milter_manager_controller_header (MilterManagerController *controller,
        const gchar *name, const gchar *value)
{
    g_print("header");
    return MILTER_STATUS_NOT_CHANGE;
}

MilterStatus
milter_manager_controller_end_of_header (MilterManagerController *controller)
{
    g_print("end-of-header");
    return MILTER_STATUS_NOT_CHANGE;
}

MilterStatus
milter_manager_controller_body (MilterManagerController *controller, const guchar *chunk, gsize size)
{
    g_print("body");
    return MILTER_STATUS_NOT_CHANGE;
}

MilterStatus
milter_manager_controller_end_of_message (MilterManagerController *controller)
{
    g_print("end-of-body");
    return MILTER_STATUS_NOT_CHANGE;
}

MilterStatus
milter_manager_controller_quit (MilterManagerController *controller)
{
    g_print("quit");
    return MILTER_STATUS_NOT_CHANGE;
}

MilterStatus
milter_manager_controller_abort (MilterManagerController *controller)
{
    g_print("abort");
    return MILTER_STATUS_NOT_CHANGE;
}

void
milter_manager_controller_mta_timeout (MilterManagerController *controller)
{
    g_print("mta_timeout");
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
