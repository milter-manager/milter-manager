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
#  include "../../config.h"
#endif /* HAVE_CONFIG_H */

#include <string.h>

#include "milter-manager-controller-context.h"
#include "milter-manager-enum-types.h"
#include "milter-manager-control-command-decoder.h"
#include "milter-manager-control-reply-encoder.h"

#define MILTER_MANAGER_CONTROLLER_CONTEXT_GET_PRIVATE(obj)              \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj),                                 \
                                 MILTER_TYPE_MANAGER_CONTROLLER_CONTEXT, \
                                 MilterManagerControllerContextPrivate))

typedef struct _MilterManagerControllerContextPrivate MilterManagerControllerContextPrivate;
struct _MilterManagerControllerContextPrivate
{
    MilterManager *manager;
};

enum
{
    PROP_0,
    PROP_MANAGER
};

G_DEFINE_TYPE(MilterManagerControllerContext, milter_manager_controller_context,
              MILTER_TYPE_AGENT)

static void dispose        (GObject         *object);
static void set_property   (GObject         *object,
                            guint            prop_id,
                            const GValue    *value,
                            GParamSpec      *pspec);
static void get_property   (GObject         *object,
                            guint            prop_id,
                            GValue          *value,
                            GParamSpec      *pspec);

static MilterDecoder *decoder_new    (MilterAgent *agent);
static MilterEncoder *encoder_new    (MilterAgent *agent);

static void
milter_manager_controller_context_class_init (MilterManagerControllerContextClass *klass)
{
    GObjectClass *gobject_class;
    MilterAgentClass *agent_class;
    GParamSpec *spec;

    gobject_class = G_OBJECT_CLASS(klass);
    agent_class = MILTER_AGENT_CLASS(klass);

    gobject_class->dispose      = dispose;
    gobject_class->set_property = set_property;
    gobject_class->get_property = get_property;

    agent_class->decoder_new   = decoder_new;
    agent_class->encoder_new   = encoder_new;

    spec = g_param_spec_object("manager",
                               "Manager",
                               "The manager of the milter controller context",
                               MILTER_TYPE_MANAGER,
                               G_PARAM_READWRITE);
    g_object_class_install_property(gobject_class, PROP_MANAGER, spec);

    g_type_class_add_private(gobject_class,
                             sizeof(MilterManagerControllerContextPrivate));
}

static void
cb_decoder_set_configuration (MilterManagerControlCommandDecoder *decoder,
                              const gchar *configuration, gsize size,
                              gpointer user_data)
{
    MilterManagerControllerContext *context = user_data;
    MilterManagerControllerContextPrivate *priv;
    MilterManagerConfiguration *config;
    GError *error = NULL;
    MilterAgent *agent;
    MilterEncoder *_encoder;
    MilterManagerControlReplyEncoder *encoder;
    const gchar *packet;
    gsize packet_size;

    priv = MILTER_MANAGER_CONTROLLER_CONTEXT_GET_PRIVATE(context);
    config = milter_manager_get_configuration(priv->manager);

    agent = MILTER_AGENT(context);
    _encoder = milter_agent_get_encoder(agent);
    encoder = MILTER_MANAGER_CONTROL_REPLY_ENCODER(_encoder);
    if (milter_manager_configuration_save_custom(config,
                                                 configuration, size,
                                                 &error)) {
        milter_manager_control_reply_encoder_encode_success(encoder,
                                                            &packet,
                                                            &packet_size);
        if (!milter_agent_write_packet(agent, packet, packet_size, &error)) {
            milter_error("[controller][error][write][success] %s",
                         error->message);
            g_error_free(error);
        }
    } else {
        gchar *message;

        message = g_strdup_printf("failed to save custom configuration file: %s",
                                  error->message);
        g_error_free(error);
        error = NULL;

        milter_error("[controller][error][save] %s", message);
        milter_manager_control_reply_encoder_encode_error(encoder,
                                                          &packet,
                                                          &packet_size,
                                                          message);
        if (!milter_agent_write_packet(agent, packet, packet_size, &error)) {
            milter_error("[controller][error][write][error] %s",
                         error->message);
            g_error_free(error);
        }
    }
}

static void
cb_decoder_get_configuration (MilterManagerControlCommandDecoder *decoder,
                              gpointer user_data)
{
    MilterManagerControllerContext *context = user_data;
    MilterManagerControllerContextPrivate *priv;
    MilterManagerConfiguration *config;
    GError *error = NULL;
    MilterAgent *agent;
    MilterEncoder *base_encoder;
    MilterManagerControlReplyEncoder *encoder;
    const gchar *packet;
    gsize packet_size;
    gchar *config_xml;

    priv = MILTER_MANAGER_CONTROLLER_CONTEXT_GET_PRIVATE(context);
    config = milter_manager_get_configuration(priv->manager);

    agent = MILTER_AGENT(context);
    base_encoder = milter_agent_get_encoder(agent);
    encoder = MILTER_MANAGER_CONTROL_REPLY_ENCODER(base_encoder);
    config_xml = milter_manager_configuration_to_xml(config);
    milter_manager_control_reply_encoder_encode_configuration(
        encoder, &packet, &packet_size,
        config_xml, strlen(config_xml));
    g_free(config_xml);
    if (!milter_agent_write_packet(agent, packet, packet_size, &error)) {
        milter_error("[controller][error][write][configuration] %s",
                     error->message);
        g_error_free(error);
    }
}

static void
cb_decoder_reload (MilterManagerControlCommandDecoder *decoder,
                   gpointer user_data)
{
    MilterManagerControllerContext *context = user_data;
    MilterManagerControllerContextPrivate *priv;
    MilterManagerConfiguration *config;
    GError *error = NULL;
    MilterAgent *agent;
    MilterEncoder *base_encoder;
    MilterManagerControlReplyEncoder *encoder;
    const gchar *packet;
    gsize packet_size;

    priv = MILTER_MANAGER_CONTROLLER_CONTEXT_GET_PRIVATE(context);
    config = milter_manager_get_configuration(priv->manager);
    if (!milter_manager_configuration_reload(config, &error)) {
        milter_error("[controller][reload][error] %s",
                     error->message);
        g_error_free(error);
        error = NULL;
    }

    agent = MILTER_AGENT(context);
    base_encoder = milter_agent_get_encoder(agent);
    encoder = MILTER_MANAGER_CONTROL_REPLY_ENCODER(base_encoder);
    milter_manager_control_reply_encoder_encode_success(encoder,
                                                        &packet,
                                                        &packet_size);
    if (!milter_agent_write_packet(agent, packet, packet_size, &error)) {
        milter_error("[controller][error][write][success] %s",
                     error->message);
        g_error_free(error);
    }
}

static void
collect_status (MilterManagerControllerContext *context, GString *status)
{
    g_string_append(status, "FIXME");
}

static void
cb_decoder_get_status (MilterManagerControlCommandDecoder *decoder,
                       gpointer user_data)
{
    MilterManagerControllerContext *context = user_data;
    GString *status;
    GError *error = NULL;
    MilterAgent *agent;
    MilterEncoder *base_encoder;
    MilterManagerControlReplyEncoder *encoder;
    const gchar *packet;
    gsize packet_size;

    status = g_string_new(NULL);
    collect_status(context, status);
    agent = MILTER_AGENT(context);
    base_encoder = milter_agent_get_encoder(agent);
    encoder = MILTER_MANAGER_CONTROL_REPLY_ENCODER(base_encoder);
    milter_manager_control_reply_encoder_encode_status(encoder,
                                                       &packet,
                                                       &packet_size,
                                                       status->str,
                                                       status->len);
    g_string_free(status, TRUE);
    if (!milter_agent_write_packet(agent, packet, packet_size, &error)) {
        milter_error("[controller][error][write][status] %s",
                     error->message);
        g_error_free(error);
    }
}

static MilterDecoder *
decoder_new (MilterAgent *agent)
{
    MilterDecoder *decoder;

    decoder = milter_manager_control_command_decoder_new();

#define CONNECT(name)                                                   \
    g_signal_connect(decoder, #name, G_CALLBACK(cb_decoder_ ## name),   \
                     agent)

    CONNECT(set_configuration);
    CONNECT(get_configuration);
    CONNECT(reload);
    CONNECT(get_status);

#undef CONNECT

    return decoder;
}

static MilterEncoder *
encoder_new (MilterAgent *agent)
{
    return milter_manager_control_reply_encoder_new();
}

static void
milter_manager_controller_context_init (MilterManagerControllerContext *context)
{
    MilterManagerControllerContextPrivate *priv;

    priv = MILTER_MANAGER_CONTROLLER_CONTEXT_GET_PRIVATE(context);
    priv->manager = NULL;
}

static void
dispose (GObject *object)
{
    MilterManagerControllerContextPrivate *priv;

    priv = MILTER_MANAGER_CONTROLLER_CONTEXT_GET_PRIVATE(object);

    if (priv->manager) {
        g_object_unref(priv->manager);
        priv->manager = NULL;
    }

    G_OBJECT_CLASS(milter_manager_controller_context_parent_class)->dispose(object);
}

static void
set_property (GObject      *object,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
    MilterManagerControllerContextPrivate *priv;

    priv = MILTER_MANAGER_CONTROLLER_CONTEXT_GET_PRIVATE(object);
    switch (prop_id) {
      case PROP_MANAGER:
        if (priv->manager)
            g_object_unref(priv->manager);
        priv->manager = g_value_get_object(value);
        if (priv->manager)
            g_object_ref(priv->manager);
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
    MilterManagerControllerContextPrivate *priv;

    priv = MILTER_MANAGER_CONTROLLER_CONTEXT_GET_PRIVATE(object);
    switch (prop_id) {
      case PROP_MANAGER:
        g_value_set_object(value, priv->manager);
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

MilterManagerControllerContext *
milter_manager_controller_context_new (MilterManager *manager)
{
    return g_object_new(MILTER_TYPE_MANAGER_CONTROLLER_CONTEXT,
                        "manager", manager,
                        NULL);
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
