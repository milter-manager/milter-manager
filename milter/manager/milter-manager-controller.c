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
#include <errno.h>
#include <unistd.h>
#include <grp.h>

#include <glib/gstdio.h>

#include "milter-manager-controller.h"
#include "milter-manager-enum-types.h"

#define MILTER_MANAGER_CONTROLLER_GET_PRIVATE(obj)                   \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj),                              \
                                 MILTER_TYPE_MANAGER_CONTROLLER,     \
                                 MilterManagerControllerPrivate))

typedef struct _MilterManagerControllerPrivate MilterManagerControllerPrivate;
struct _MilterManagerControllerPrivate
{
    MilterManager *manager;
    MilterEventLoop *event_loop;
    guint watch_id;
    gchar *spec;
};

enum
{
    PROP_0,
    PROP_MANAGER,
    PROP_EVENT_LOOP
};

G_DEFINE_TYPE(MilterManagerController, milter_manager_controller,
              G_TYPE_OBJECT)

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

    spec = g_param_spec_object("manager",
                               "Manager",
                               "The manager of the milter controller",
                               MILTER_TYPE_MANAGER,
                               G_PARAM_READWRITE);
    g_object_class_install_property(gobject_class, PROP_MANAGER, spec);

    spec = g_param_spec_object("event-loop",
                               "Event Loop",
                               "The event loop of the milter controller",
                               MILTER_TYPE_EVENT_LOOP,
                               G_PARAM_READWRITE);
    g_object_class_install_property(gobject_class, PROP_EVENT_LOOP, spec);

    g_type_class_add_private(gobject_class,
                             sizeof(MilterManagerControllerPrivate));
}

static void
milter_manager_controller_init (MilterManagerController *controller)
{
    MilterManagerControllerPrivate *priv;

    priv = MILTER_MANAGER_CONTROLLER_GET_PRIVATE(controller);
    priv->manager = NULL;
    priv->event_loop = NULL;
    priv->watch_id = 0;
    priv->spec = NULL;
}

static void
dispose_spec (MilterManagerControllerPrivate *priv)
{
    MilterManagerConfiguration *config;

    if (!priv->spec)
        return;

    config = milter_manager_get_configuration(priv->manager);
    if (milter_manager_configuration_is_remove_controller_unix_socket_on_close(config)) {
        struct sockaddr *address = NULL;
        socklen_t address_size = 0;
        GError *error = NULL;

        if (milter_connection_parse_spec(priv->spec,
                                         NULL, &address, &address_size,
                                         &error)) {
            if (address->sa_family == AF_UNIX) {
                struct sockaddr_un *address_un;

                address_un = (struct sockaddr_un *)address;
                if (g_unlink(address_un->sun_path) == -1) {
                    milter_error("[controller][error][unix] "
                                 "failed to remove used UNIX socket: %s: %s",
                                 address_un->sun_path, g_strerror(errno));
                }
            }
            g_free(address);
        } else {
            milter_error("[controller][error][unix] %s", error->message);
            g_error_free(error);
        }
    }

    g_free(priv->spec);
    priv->spec = NULL;
}

static void
dispose (GObject *object)
{
    MilterManagerControllerPrivate *priv;

    priv = MILTER_MANAGER_CONTROLLER_GET_PRIVATE(object);

    if (priv->watch_id > 0) {
        milter_event_loop_remove(priv->event_loop, priv->watch_id);
        priv->watch_id = 0;
    }

    dispose_spec(priv);

    if (priv->manager) {
        g_object_unref(priv->manager);
        priv->manager = NULL;
    }

    if (priv->event_loop) {
        g_object_unref(priv->event_loop);
        priv->event_loop = NULL;
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
    case PROP_MANAGER:
        if (priv->manager)
            g_object_unref(priv->manager);
        priv->manager = g_value_get_object(value);
        if (priv->manager)
            g_object_ref(priv->manager);
        break;
    case PROP_EVENT_LOOP:
        if (priv->event_loop)
            g_object_unref(priv->event_loop);
        priv->event_loop = g_value_dup_object(value);
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
    case PROP_MANAGER:
        g_value_set_object(value, priv->manager);
        break;
    case PROP_EVENT_LOOP:
        g_value_set_object(value, priv->event_loop);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

GQuark
milter_manager_controller_error_quark (void)
{
    return g_quark_from_static_string("milter-manager-controller-error-quark");
}

MilterManagerController *
milter_manager_controller_new (MilterManager *manager,
                               MilterEventLoop *event_loop)
{
    return g_object_new(MILTER_TYPE_MANAGER_CONTROLLER,
                        "manager", manager,
                        "event-loop", event_loop,
                        NULL);
}

static gboolean
cb_free_context (gpointer data)
{
    MilterManagerControllerContext *context = data;

    g_object_unref(context);
    return FALSE;
}

static void
cb_context_reader_finished (MilterReader *reader, gpointer data)
{
    MilterManagerControllerContext *context = data;
    MilterEventLoop *loop;

    loop = milter_agent_get_event_loop(MILTER_AGENT(context));
    milter_event_loop_add_idle_full(loop, G_PRIORITY_DEFAULT,
                                    cb_free_context, context,
                                    NULL);
}

static void
process_connection(MilterManagerController *controller,
                   GIOChannel *agent_channel)
{
    MilterManagerControllerPrivate *priv;
    MilterManagerControllerContext *context;
    MilterWriter *writer;
    MilterReader *reader;
    GError *error = NULL;

    priv = MILTER_MANAGER_CONTROLLER_GET_PRIVATE(controller);
    context = milter_manager_controller_context_new(priv->manager);
    milter_agent_set_event_loop(MILTER_AGENT(context), priv->event_loop);

    writer = milter_writer_io_channel_new(agent_channel);
    milter_agent_set_writer(MILTER_AGENT(context), writer);
    g_object_unref(writer);

    reader = milter_reader_io_channel_new(agent_channel);
    milter_agent_set_reader(MILTER_AGENT(context), reader);
    g_object_unref(reader);

    g_signal_connect(reader, "finished",
                     G_CALLBACK(cb_context_reader_finished), context);

    if (!milter_agent_start(MILTER_AGENT(context), &error)) {
        milter_error("[controller][error][start] %s", error->message);
        milter_error_emittable_emit(MILTER_ERROR_EMITTABLE(context), error);
        g_error_free(error);
    }
}

static gboolean
accept_connection (gint controller_fd, MilterManagerController *controller)
{
    gint agent_fd;
    MilterGenericSocketAddress address;
    socklen_t address_size;
    GIOChannel *agent_channel;

    address_size = sizeof(address);
    memset(&address, '\0', address_size);
    agent_fd = accept(controller_fd,
                      (struct sockaddr *)(&address), &address_size);
    if (agent_fd == -1) {
        milter_error("[controller][error][accept] %s", g_strerror(errno));
        return TRUE;
    }

    if (milter_need_debug_log()) {
        gchar *spec;
        spec = milter_connection_address_to_spec(&(address.address.base));
        milter_debug("[controller][accept] %d: %s", agent_fd, spec);
        g_free(spec);
    }

    agent_channel = g_io_channel_unix_new(agent_fd);
    g_io_channel_set_encoding(agent_channel, NULL, NULL);
    g_io_channel_set_flags(agent_channel, G_IO_FLAG_NONBLOCK, NULL);
    g_io_channel_set_close_on_unref(agent_channel, TRUE);
    process_connection(controller, agent_channel);
    g_io_channel_unref(agent_channel);

    return TRUE;
}

static gboolean
watch_func (GIOChannel *channel, GIOCondition condition, gpointer data)
{
    MilterManagerController *controller = data;
    MilterManagerControllerPrivate *priv;
    gboolean keep_callback = TRUE;

    priv = MILTER_MANAGER_CONTROLLER_GET_PRIVATE(controller);

    if (condition & G_IO_IN ||
        condition & G_IO_PRI) {
        guint fd;

        fd = g_io_channel_unix_get_fd(channel);
        keep_callback = accept_connection(fd, controller);
    }

    if (condition & G_IO_ERR ||
        condition & G_IO_HUP ||
        condition & G_IO_NVAL) {
        gchar *message;

        message = milter_utils_inspect_io_condition_error(condition);
        milter_error("[controller][error][watch] %s", message);
        g_free(message);
        keep_callback = FALSE;
    }

    if (!keep_callback) {
        priv->watch_id = 0;
    }

    return keep_callback;
}

static void
change_unix_socket_group (MilterManagerController *controller,
                          struct sockaddr_un *address_un,
                          MilterManagerConfiguration *config)
{
    const gchar *socket_group;
    struct group *group;

    socket_group = milter_manager_configuration_get_controller_unix_socket_group(config);
    if (!socket_group)
        return;

    errno = 0;
    group = getgrnam(socket_group);
    if (!group) {
        if (errno == 0) {
            milter_error("[controller][error][unix] "
                         "failed to find group entry for UNIX socket group: "
                         "<%s>: <%s>",
                         address_un->sun_path, socket_group);
        } else {
            milter_error("[controller][error][unix] "
                         "failed to get group entry for UNIX socket group: "
                         "<%s>: <%s>: %s",
                         address_un->sun_path, socket_group, g_strerror(errno));
        }
        return;
    }

    if (chown(address_un->sun_path, -1, group->gr_gid) == -1) {
        milter_error("[controller][error][unix] "
                     "failed to change UNIX socket group: <%s>: <%s>: %s",
                     address_un->sun_path, socket_group, g_strerror(errno));
    }
}

static void
change_unix_socket_mode (MilterManagerController *controller,
                         struct sockaddr_un *address_un,
                         MilterManagerConfiguration *config)
{
    guint mode;

    mode = milter_manager_configuration_get_controller_unix_socket_mode(config);
    if (g_chmod(address_un->sun_path, mode) == -1) {
        milter_error("[controller][error][unix] "
                     "failed to change the mode of UNIX socket: %s(%o): %s",
                     address_un->sun_path, mode, g_strerror(errno));
    }
}

static void
listen_started (MilterManagerController *controller,
                struct sockaddr *address, socklen_t address_size,
                MilterManagerConfiguration *config)
{
    struct sockaddr_un *address_un;

    if (address->sa_family != AF_UNIX)
        return;

    address_un = (struct sockaddr_un *)address;

    change_unix_socket_group(controller, address_un, config);
    change_unix_socket_mode(controller, address_un, config);
}

gboolean
milter_manager_controller_listen (MilterManagerController *controller,
                                  GError **error)
{
    MilterManagerControllerPrivate *priv;
    MilterManagerConfiguration *config;
    const gchar *spec;
    GIOChannel *channel;
    struct sockaddr *address = NULL;
    socklen_t address_size = 0;
    gboolean remove_socket;
    GError *local_error = NULL;

    priv = MILTER_MANAGER_CONTROLLER_GET_PRIVATE(controller);
    if (priv->watch_id > 0) {
        local_error = g_error_new(MILTER_MANAGER_CONTROLLER_ERROR,
                                  MILTER_MANAGER_CONTROLLER_ERROR_LISTING,
                                  "already listening");
        milter_error("[controller][error][listen] %s", local_error->message);
        g_propagate_error(error, local_error);
        return FALSE;
    }

    dispose_spec(priv);

    config = milter_manager_get_configuration(priv->manager);
    spec = milter_manager_configuration_get_controller_connection_spec(config);
    if (!spec) {
        milter_info("[controller][disabled] connection spec isn't specified");
        return TRUE;
    }

    remove_socket = milter_manager_configuration_is_remove_controller_unix_socket_on_create(config);
    channel = milter_connection_listen(spec, -1, &address, &address_size,
                                       remove_socket, &local_error);
    if (address) {
        listen_started(controller, address, address_size, config);
        g_free(address);
    }

    if (!channel) {
        milter_error("[controller][error][listen] <%s>: %s",
                     spec, local_error->message);
        g_propagate_error(error, local_error);
        return FALSE;
    }

    priv->spec = g_strdup(spec);
    priv->watch_id =
        milter_event_loop_watch_io(priv->event_loop, channel,
                                   G_IO_IN | G_IO_PRI |
                                   G_IO_ERR | G_IO_HUP | G_IO_NVAL,
                                   watch_func, controller);
    g_io_channel_unref(channel);

    return TRUE;
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
