/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Copyright (C) 2008-2013  Kouhei Sutou <kou@clear-code.com>
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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <glib/gstdio.h>

#include "milter-manager-module.h"
#include "milter-manager-configuration.h"
#include "milter-manager-leader.h"
#include "milter-manager-children.h"
#include <milter/core/milter-marshalers.h>

#define DEFAULT_FALLBACK_STATUS MILTER_STATUS_ACCEPT
#define DEFAULT_FALLBACK_STATUS_AT_DISCONNECT MILTER_STATUS_TEMPORARY_FAILURE
#define DEFAULT_MAINTENANCE_INTERVAL 10
#define DEFAULT_CONNECTION_CHECK_INTERVAL 0

#define MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(obj)                   \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj),                                 \
                                 MILTER_TYPE_MANAGER_CONFIGURATION,     \
                                 MilterManagerConfigurationPrivate))

typedef struct _MilterManagerConfigurationPrivate MilterManagerConfigurationPrivate;
struct _MilterManagerConfigurationPrivate
{
    GList *load_paths;
    GList *eggs;
    GList *applicable_conditions;
    gboolean privilege_mode;
    gchar *controller_connection_spec;
    gchar *manager_connection_spec;
    MilterStatus fallback_status;
    MilterStatus fallback_status_at_disconnect;
    gchar *package_platform;
    gchar *package_options;
    gchar *effective_user;
    gchar *effective_group;
    guint manager_unix_socket_mode;
    guint controller_unix_socket_mode;
    gchar *manager_unix_socket_group;
    gchar *controller_unix_socket_group;
    gboolean remove_manager_unix_socket_on_close;
    gboolean remove_controller_unix_socket_on_close;
    gboolean remove_manager_unix_socket_on_create;
    gboolean remove_controller_unix_socket_on_create;
    gboolean daemon;
    gchar *pid_file;
    guint maintenance_interval;
    guint suspend_time_on_unacceptable;
    guint max_connections;
    guint max_file_descriptors;
    gchar *custom_configuration_directory;
    GHashTable *locations;
    guint connection_check_interval;
    MilterClientEventLoopBackend event_loop_backend;
    guint n_workers;
    guint default_packet_buffer_size;
    gboolean use_syslog;
    gchar *syslog_facility;
    guint chunk_size;
    guint max_pending_finished_sessions;
};

enum
{
    PROP_0,
    PROP_PRIVILEGE_MODE,
    PROP_CONTROLLER_CONNECTION_SPEC,
    PROP_MANAGER_CONNECTION_SPEC,
    PROP_FALLBACK_STATUS,
    PROP_FALLBACK_STATUS_AT_DISCONNECT,
    PROP_PACKAGE_PLATFORM,
    PROP_PACKAGE_OPTIONS,
    PROP_EFFECTIVE_USER,
    PROP_EFFECTIVE_GROUP,
    PROP_MANAGER_UNIX_SOCKET_MODE,
    PROP_CONTROLLER_UNIX_SOCKET_MODE,
    PROP_MANAGER_UNIX_SOCKET_GROUP,
    PROP_CONTROLLER_UNIX_SOCKET_GROUP,
    PROP_REMOVE_MANAGER_UNIX_SOCKET_ON_CLOSE,
    PROP_REMOVE_CONTROLLER_UNIX_SOCKET_ON_CLOSE,
    PROP_REMOVE_MANAGER_UNIX_SOCKET_ON_CREATE,
    PROP_REMOVE_CONTROLLER_UNIX_SOCKET_ON_CREATE,
    PROP_DAEMON,
    PROP_PID_FILE,
    PROP_MAINTENANCE_INTERVAL,
    PROP_SUSPEND_TIME_ON_UNACCEPTABLE,
    PROP_MAX_CONNECTIONS,
    PROP_MAX_FILE_DESCRIPTORS,
    PROP_CUSTOM_CONFIGURATION_DIRECTORY,
    PROP_CONNECTION_CHECK_INTERVAL,
    PROP_EVENT_LOOP_BACKEND,
    PROP_N_WORKERS,
    PROP_DEFAULT_PACKET_BUFFER_SIZE,
    PROP_PREFIX,
    PROP_USE_SYSLOG,
    PROP_SYSLOG_FACILITY,
    PROP_CHUNK_SIZE,
    PROP_MAX_PENDING_FINISHED_SESSIONS
};

enum
{
    CONNECTED,
    TO_XML,
    LAST_SIGNAL
};

static gint signals[LAST_SIGNAL] = {0};

static GList *configurations = NULL;
static gchar *configuration_module_dir = NULL;

G_DEFINE_TYPE(MilterManagerConfiguration, milter_manager_configuration,
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
milter_manager_configuration_class_init (MilterManagerConfigurationClass *klass)
{
    GObjectClass *gobject_class;
    GParamSpec *spec;

    gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->dispose      = dispose;
    gobject_class->set_property = set_property;
    gobject_class->get_property = get_property;

    spec = g_param_spec_boolean("privilege-mode",
                                "Privilege mode",
                                "Whether MilterManager runs on privilege mode",
                                FALSE,
                                G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
    g_object_class_install_property(gobject_class, PROP_PRIVILEGE_MODE, spec);

    spec = g_param_spec_string("controller-connection-spec",
                               "Controller connection spec",
                               "The controller connection spec "
                               "of the milter-manager",
                               NULL,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
    g_object_class_install_property(gobject_class,
                                    PROP_CONTROLLER_CONNECTION_SPEC,
                                    spec);

    spec = g_param_spec_string("manager-connection-spec",
                               "Manager connection spec",
                               "The manager connection spec "
                               "of the milter-manager",
                               MILTER_MANAGER_DEFAULT_CONNECTION_SPEC,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
    g_object_class_install_property(gobject_class, PROP_MANAGER_CONNECTION_SPEC,
                                    spec);

    spec = g_param_spec_enum("fallback-status",
                             "Fallback status",
                             "The fallback status of the milter-manager",
                             MILTER_TYPE_STATUS,
                             DEFAULT_FALLBACK_STATUS,
                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
    g_object_class_install_property(gobject_class, PROP_FALLBACK_STATUS, spec);

    spec = g_param_spec_enum("fallback-status-at-disconnect",
                             "Fallback status at disconnect",
                             "The fallback status at disconnect "
                             "of the milter-manager",
                             MILTER_TYPE_STATUS,
                             DEFAULT_FALLBACK_STATUS_AT_DISCONNECT,
                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
    g_object_class_install_property(gobject_class,
                                    PROP_FALLBACK_STATUS_AT_DISCONNECT, spec);

    spec = g_param_spec_string("package-platform",
                               "Package platform",
                               "The package platform of the milter-manager",
                               MILTER_MANAGER_PACKAGE_PLATFORM,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
    g_object_class_install_property(gobject_class, PROP_PACKAGE_PLATFORM,
                                    spec);

    spec = g_param_spec_string("package-options",
                               "Package options",
                               "The package options of the milter-manager",
                               MILTER_MANAGER_PACKAGE_OPTIONS,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
    g_object_class_install_property(gobject_class, PROP_PACKAGE_OPTIONS,
                                    spec);

    spec = g_param_spec_string("effective-user",
                               "Effective user name",
                               "The effective user name of the milter-manager",
                               MILTER_MANAGER_DEFAULT_EFFECTIVE_USER,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
    g_object_class_install_property(gobject_class, PROP_EFFECTIVE_USER,
                                    spec);

    spec = g_param_spec_string("effective-group",
                               "Effective group name",
                               "The effective group name of the milter-manager",
                               MILTER_MANAGER_DEFAULT_EFFECTIVE_GROUP,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
    g_object_class_install_property(gobject_class, PROP_EFFECTIVE_GROUP,
                                    spec);

    spec = g_param_spec_uint("manager-unix-socket-mode",
                             "milter-manager's UNIX socket mode",
                             "The milter-manager's UNIX socket mode",
                             0000,
                             0777,
                             0660,
                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
    g_object_class_install_property(gobject_class,
                                    PROP_MANAGER_UNIX_SOCKET_MODE,
                                    spec);

    spec = g_param_spec_uint("controller-unix-socket-mode",
                             "milter-manager controller's UNIX socket mode",
                             "The milter-manager controller's UNIX socket mode",
                             0000,
                             0777,
                             0660,
                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
    g_object_class_install_property(gobject_class,
                                    PROP_CONTROLLER_UNIX_SOCKET_MODE,
                                    spec);

    spec = g_param_spec_string("manager-unix-socket-group",
                               "milter-manager's UNIX socket group",
                               "The milter-manager's UNIX socket group",
                               MILTER_MANAGER_DEFAULT_SOCKET_GROUP,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
    g_object_class_install_property(gobject_class,
                                    PROP_MANAGER_UNIX_SOCKET_GROUP,
                                    spec);

    spec = g_param_spec_string("controller-unix-socket-group",
                               "milter-manager's UNIX socket group",
                               "The milter-manager controller's UNIX socket group",
                               NULL,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
    g_object_class_install_property(gobject_class,
                                    PROP_CONTROLLER_UNIX_SOCKET_GROUP,
                                    spec);

    spec = g_param_spec_boolean("remove-manager-unix-socket-on-close",
                                "Remove unix socket for manager on close",
                                "Whether milter-manager removes "
                                "UNIX socket used for manager on close",
                                TRUE,
                                G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
    g_object_class_install_property(gobject_class,
                                    PROP_REMOVE_MANAGER_UNIX_SOCKET_ON_CLOSE,
                                    spec);

    spec = g_param_spec_boolean("remove-controller-unix-socket-on-close",
                                "Remove unix socket for controller on close",
                                "Whether milter-manager removes "
                                "UNIX socket used for controller on close",
                                TRUE,
                                G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
    g_object_class_install_property(gobject_class,
                                    PROP_REMOVE_MANAGER_UNIX_SOCKET_ON_CLOSE,
                                    spec);

    spec = g_param_spec_boolean("remove-manager-unix-socket-on-create",
                                "Remove unix socket for manager on create",
                                "Whether milter-manager removes "
                                "UNIX socket used for manager on create",
                                TRUE,
                                G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
    g_object_class_install_property(gobject_class,
                                    PROP_REMOVE_MANAGER_UNIX_SOCKET_ON_CREATE,
                                    spec);

    spec = g_param_spec_boolean("remove-controller-unix-socket-on-create",
                                "Remove unix socket for controller on create",
                                "Whether milter-manager removes "
                                "UNIX socket used for controller on create",
                                TRUE,
                                G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
    g_object_class_install_property(gobject_class,
                                    PROP_REMOVE_MANAGER_UNIX_SOCKET_ON_CREATE,
                                    spec);

    spec = g_param_spec_boolean("daemon",
                                "Run as daemon process",
                                "Whether milter-manager runs as daemon process",
                                FALSE,
                                G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
    g_object_class_install_property(gobject_class, PROP_DAEMON, spec);

    spec = g_param_spec_string("pid-file",
                               "File name to be saved process ID",
                               "The file name to be saved process ID",
                               MILTER_MANAGER_DEFAULT_PID_FILE,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
    g_object_class_install_property(gobject_class, PROP_PID_FILE,
                                    spec);

    spec = g_param_spec_uint("maintenance-interval",
                             "Maintenance interval in sessions",
                             "Run maintenance process in after each N sessions. "
                             "0 means that periodical maintenance "
                             "process isn't run.",
                             0,
                             G_MAXUINT,
                             DEFAULT_MAINTENANCE_INTERVAL,
                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
    g_object_class_install_property(gobject_class,
                                    PROP_MAINTENANCE_INTERVAL,
                                    spec);

    spec = g_param_spec_uint("suspend-time-on-unacceptable",
                             "Suspend time in sessions",
                             "Suspend time on unacceptable.",
                             0,
                             G_MAXUINT,
                             MILTER_CLIENT_DEFAULT_SUSPEND_TIME_ON_UNACCEPTABLE,
                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
    g_object_class_install_property(gobject_class,
                                    PROP_SUSPEND_TIME_ON_UNACCEPTABLE,
                                    spec);

    spec = g_param_spec_uint("max-connections",
                             "Max connections",
                             "Max number of concurrent connections.",
                             0,
                             G_MAXUINT,
                             MILTER_CLIENT_DEFAULT_MAX_CONNECTIONS,
                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
    g_object_class_install_property(gobject_class,
                                    PROP_MAX_CONNECTIONS,
                                    spec);

    spec = g_param_spec_uint("max-file-descriptors",
                             "Max file descriptors",
                             "Max number of concurrent file descriptors.",
                             0,
                             G_MAXUINT,
                             0,
                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
    g_object_class_install_property(gobject_class,
                                    PROP_MAX_FILE_DESCRIPTORS,
                                    spec);

    spec = g_param_spec_string("custom-configuration-directory",
                               "Directory for custom configuration",
                               "The directory for custom configuration",
                               NULL,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
    g_object_class_install_property(gobject_class,
                                    PROP_CUSTOM_CONFIGURATION_DIRECTORY,
                                    spec);

    spec = g_param_spec_uint("connection-check-interval",
                             "Connection check interval",
                             "The interval in seconds for checking whether "
                             "SMTP connection between SMTP client and server "
                             "is still connected or disconnected from "
                             "SMTP client.",
                             0,
                             G_MAXUINT,
                             DEFAULT_CONNECTION_CHECK_INTERVAL,
                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
    g_object_class_install_property(gobject_class,
                                    PROP_CONNECTION_CHECK_INTERVAL,
                                    spec);


    spec = g_param_spec_enum("event-loop-backend",
                             "Event loop backend",
                             "The event loop backend of the configuration",
                             MILTER_TYPE_CLIENT_EVENT_LOOP_BACKEND,
                             MILTER_CLIENT_EVENT_LOOP_BACKEND_GLIB,
                             G_PARAM_READWRITE);
    g_object_class_install_property(gobject_class,
                                    PROP_EVENT_LOOP_BACKEND,
                                    spec);

    spec = g_param_spec_uint("n-workers",
                             "Number of worker processes",
                             "The number of worker processes of the client",
                             0, MILTER_CLIENT_MAX_N_WORKERS, 0,
                             G_PARAM_READWRITE);
    g_object_class_install_property(gobject_class, PROP_N_WORKERS, spec);

    spec = g_param_spec_uint("default-packet-buffer-size",
                             "Default packet buffer size",
                             "The default packet buffer size of client contexts "
                             "of the client",
                             0, G_MAXUINT, 0,
                             G_PARAM_READWRITE);
    g_object_class_install_property(gobject_class,
                                    PROP_DEFAULT_PACKET_BUFFER_SIZE, spec);

    spec = g_param_spec_string("prefix",
                               "Prefix",
                               "The prefix of install directory",
                               NULL,
                               G_PARAM_READABLE);
    g_object_class_install_property(gobject_class, PROP_PREFIX, spec);

    spec = g_param_spec_boolean("use-syslog",
                                "Use Syslog",
                                "Whether milter-manager uses syslog",
                                TRUE,
                                G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
    g_object_class_install_property(gobject_class, PROP_USE_SYSLOG, spec);

    spec = g_param_spec_string("syslog-facility",
                               "Syslog Facility",
                               "The syslog facility of the milter-manager",
                               NULL,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
    g_object_class_install_property(gobject_class, PROP_SYSLOG_FACILITY, spec);

    spec = g_param_spec_uint("chunk-size",
                             "Chunk Size",
                             "The chunk size of the milter-manager",
                             1,
                             MILTER_CHUNK_SIZE,
                             MILTER_CHUNK_SIZE,
                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
    g_object_class_install_property(gobject_class, PROP_CHUNK_SIZE, spec);

    spec = g_param_spec_uint("max-pending-finished-sessions",
                             "Maximum number of pending finished sessions",
                             "The maximum number of pending finished sessions "
                             "of milter-manager",
                             0, G_MAXUINT, 0,
                             G_PARAM_READWRITE);
    g_object_class_install_property(gobject_class,
                                    PROP_MAX_PENDING_FINISHED_SESSIONS,
                                    spec);

    signals[CONNECTED] =
        g_signal_new("connected",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterManagerConfigurationClass, connected),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__OBJECT,
                     G_TYPE_NONE, 1, MILTER_TYPE_MANAGER_LEADER);

    signals[TO_XML] =
        g_signal_new("to-xml",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterManagerConfigurationClass, to_xml),
                     NULL, NULL,
                     _milter_marshal_VOID__POINTER_UINT,
                     G_TYPE_NONE, 2, G_TYPE_POINTER, G_TYPE_UINT);

    g_type_class_add_private(gobject_class,
                             sizeof(MilterManagerConfigurationPrivate));
}

static void
milter_manager_configuration_init (MilterManagerConfiguration *configuration)
{
    MilterManagerConfigurationPrivate *priv;
    const gchar *config_dir_env;

    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration);
    priv->load_paths = NULL;
    priv->eggs = NULL;
    priv->applicable_conditions = NULL;
    priv->controller_connection_spec = NULL;
    priv->manager_connection_spec = NULL;
    priv->effective_user = NULL;
    priv->effective_group = NULL;
    priv->pid_file = NULL;
    priv->maintenance_interval = DEFAULT_MAINTENANCE_INTERVAL;
    priv->suspend_time_on_unacceptable =
        MILTER_CLIENT_DEFAULT_SUSPEND_TIME_ON_UNACCEPTABLE;
    priv->max_connections =
        MILTER_CLIENT_DEFAULT_MAX_CONNECTIONS;
    priv->max_file_descriptors = 0;
    priv->custom_configuration_directory = NULL;
    priv->locations = g_hash_table_new_full(g_str_hash,
                                            g_str_equal,
                                            g_free,
                                            (GDestroyNotify)g_dataset_destroy);
    priv->connection_check_interval = DEFAULT_CONNECTION_CHECK_INTERVAL;
    priv->n_workers = 0;
    priv->default_packet_buffer_size = 0;
    priv->syslog_facility = NULL;
    priv->chunk_size = MILTER_CHUNK_SIZE;
    priv->max_pending_finished_sessions = 0;

    config_dir_env = g_getenv("MILTER_MANAGER_CONFIG_DIR");
    if (config_dir_env)
        priv->load_paths = g_list_append(priv->load_paths,
                                         g_strdup(config_dir_env));
    priv->load_paths = g_list_append(priv->load_paths, g_strdup(CONFIG_DIR));
}

static void
dispose (GObject *object)
{
    MilterManagerConfiguration *configuration;
    MilterManagerConfigurationPrivate *priv;
    GError *error = NULL;

    configuration = MILTER_MANAGER_CONFIGURATION(object);
    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration);

    milter_manager_configuration_clear_load_paths(configuration);
    milter_manager_configuration_clear(configuration, &error);
    if (error) {
        milter_error("[configuration][dispose][clear][error] %s",
                     error->message);
        g_error_free(error);
    }

    if (priv->manager_connection_spec) {
        g_free(priv->manager_connection_spec);
        priv->manager_connection_spec = NULL;
    }

    if (priv->locations) {
        g_hash_table_unref(priv->locations);
        priv->locations = NULL;
    }

    G_OBJECT_CLASS(milter_manager_configuration_parent_class)->dispose(object);
}

static void
set_property (GObject      *object,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
    MilterManagerConfiguration *config;

    config = MILTER_MANAGER_CONFIGURATION(object);
    switch (prop_id) {
    case PROP_PRIVILEGE_MODE:
        milter_manager_configuration_set_privilege_mode(
            config, g_value_get_boolean(value));
        break;
    case PROP_CONTROLLER_CONNECTION_SPEC:
        milter_manager_configuration_set_controller_connection_spec(
            config, g_value_get_string(value));
        break;
    case PROP_MANAGER_CONNECTION_SPEC:
        milter_manager_configuration_set_manager_connection_spec(
            config, g_value_get_string(value));
        break;
    case PROP_FALLBACK_STATUS:
        milter_manager_configuration_set_fallback_status(
            config, g_value_get_enum(value));
        break;
    case PROP_FALLBACK_STATUS_AT_DISCONNECT:
        milter_manager_configuration_set_fallback_status_at_disconnect(
            config, g_value_get_enum(value));
        break;
    case PROP_PACKAGE_PLATFORM:
        milter_manager_configuration_set_package_platform(
            config, g_value_get_string(value));
        break;
    case PROP_PACKAGE_OPTIONS:
        milter_manager_configuration_set_package_options(
            config, g_value_get_string(value));
        break;
    case PROP_EFFECTIVE_USER:
        milter_manager_configuration_set_effective_user(
            config, g_value_get_string(value));
        break;
    case PROP_EFFECTIVE_GROUP:
        milter_manager_configuration_set_effective_group(
            config, g_value_get_string(value));
        break;
    case PROP_MANAGER_UNIX_SOCKET_MODE:
        milter_manager_configuration_set_manager_unix_socket_mode(
            config, g_value_get_uint(value));
        break;
    case PROP_CONTROLLER_UNIX_SOCKET_MODE:
        milter_manager_configuration_set_controller_unix_socket_mode(
            config, g_value_get_uint(value));
        break;
    case PROP_MANAGER_UNIX_SOCKET_GROUP:
        milter_manager_configuration_set_manager_unix_socket_group(
            config, g_value_get_string(value));
        break;
    case PROP_CONTROLLER_UNIX_SOCKET_GROUP:
        milter_manager_configuration_set_controller_unix_socket_group(
            config, g_value_get_string(value));
        break;
    case PROP_REMOVE_MANAGER_UNIX_SOCKET_ON_CLOSE:
        milter_manager_configuration_set_remove_manager_unix_socket_on_close(
            config, g_value_get_boolean(value));
        break;
    case PROP_REMOVE_CONTROLLER_UNIX_SOCKET_ON_CLOSE:
        milter_manager_configuration_set_remove_controller_unix_socket_on_close(
            config, g_value_get_boolean(value));
        break;
    case PROP_REMOVE_MANAGER_UNIX_SOCKET_ON_CREATE:
        milter_manager_configuration_set_remove_manager_unix_socket_on_create(
            config, g_value_get_boolean(value));
        break;
    case PROP_REMOVE_CONTROLLER_UNIX_SOCKET_ON_CREATE:
        milter_manager_configuration_set_remove_controller_unix_socket_on_create(
            config, g_value_get_boolean(value));
        break;
    case PROP_DAEMON:
        milter_manager_configuration_set_daemon(
            config, g_value_get_boolean(value));
        break;
    case PROP_PID_FILE:
        milter_manager_configuration_set_pid_file(
            config, g_value_get_string(value));
        break;
    case PROP_MAINTENANCE_INTERVAL:
        milter_manager_configuration_set_maintenance_interval(
            config, g_value_get_uint(value));
        break;
    case PROP_SUSPEND_TIME_ON_UNACCEPTABLE:
        milter_manager_configuration_set_suspend_time_on_unacceptable(
            config, g_value_get_uint(value));
        break;
    case PROP_MAX_CONNECTIONS:
        milter_manager_configuration_set_max_connections(
            config, g_value_get_uint(value));
        break;
    case PROP_MAX_FILE_DESCRIPTORS:
        milter_manager_configuration_set_max_file_descriptors(
            config, g_value_get_uint(value));
        break;
    case PROP_CUSTOM_CONFIGURATION_DIRECTORY:
        milter_manager_configuration_set_custom_configuration_directory(
            config, g_value_get_string(value));
        break;
    case PROP_CONNECTION_CHECK_INTERVAL:
        milter_manager_configuration_set_connection_check_interval(
            config, g_value_get_uint(value));
        break;
    case PROP_EVENT_LOOP_BACKEND:
        milter_manager_configuration_set_event_loop_backend(
            config, g_value_get_enum(value));
        break;
    case PROP_N_WORKERS:
        milter_manager_configuration_set_n_workers(config, g_value_get_uint(value));
        break;
    case PROP_DEFAULT_PACKET_BUFFER_SIZE:
        milter_manager_configuration_set_default_packet_buffer_size(
            config,
            g_value_get_uint(value));
        break;
    case PROP_USE_SYSLOG:
        milter_manager_configuration_set_use_syslog(config,
                                                    g_value_get_boolean(value));
        break;
    case PROP_SYSLOG_FACILITY:
        milter_manager_configuration_set_syslog_facility(
            config, g_value_get_string(value));
        break;
    case PROP_CHUNK_SIZE:
        milter_manager_configuration_set_chunk_size(
            config, g_value_get_uint(value));
        break;
    case PROP_MAX_PENDING_FINISHED_SESSIONS:
        milter_manager_configuration_set_max_pending_finished_sessions(
            config, g_value_get_uint(value));
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
    MilterManagerConfigurationPrivate *priv;

    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(object);
    switch (prop_id) {
    case PROP_PRIVILEGE_MODE:
        g_value_set_boolean(value, priv->privilege_mode);
        break;
    case PROP_CONTROLLER_CONNECTION_SPEC:
        g_value_set_string(value, priv->controller_connection_spec);
        break;
    case PROP_MANAGER_CONNECTION_SPEC:
        g_value_set_string(value, priv->manager_connection_spec);
        break;
    case PROP_FALLBACK_STATUS:
        g_value_set_enum(value, priv->fallback_status);
        break;
    case PROP_FALLBACK_STATUS_AT_DISCONNECT:
        g_value_set_enum(value, priv->fallback_status_at_disconnect);
        break;
    case PROP_PACKAGE_PLATFORM:
        g_value_set_string(value, priv->package_platform);
        break;
    case PROP_PACKAGE_OPTIONS:
        g_value_set_string(value, priv->package_options);
        break;
    case PROP_EFFECTIVE_USER:
        g_value_set_string(value, priv->effective_user);
        break;
    case PROP_EFFECTIVE_GROUP:
        g_value_set_string(value, priv->effective_group);
        break;
    case PROP_MANAGER_UNIX_SOCKET_MODE:
        g_value_set_uint(value, priv->manager_unix_socket_mode);
        break;
    case PROP_CONTROLLER_UNIX_SOCKET_MODE:
        g_value_set_uint(value, priv->controller_unix_socket_mode);
        break;
    case PROP_MANAGER_UNIX_SOCKET_GROUP:
        g_value_set_string(value, priv->manager_unix_socket_group);
        break;
    case PROP_CONTROLLER_UNIX_SOCKET_GROUP:
        g_value_set_string(value, priv->controller_unix_socket_group);
        break;
    case PROP_REMOVE_MANAGER_UNIX_SOCKET_ON_CLOSE:
        g_value_set_boolean(value, priv->remove_manager_unix_socket_on_close);
        break;
    case PROP_REMOVE_CONTROLLER_UNIX_SOCKET_ON_CLOSE:
        g_value_set_boolean(value, priv->remove_controller_unix_socket_on_close);
        break;
    case PROP_REMOVE_MANAGER_UNIX_SOCKET_ON_CREATE:
        g_value_set_boolean(value, priv->remove_manager_unix_socket_on_create);
        break;
    case PROP_REMOVE_CONTROLLER_UNIX_SOCKET_ON_CREATE:
        g_value_set_boolean(value, priv->remove_controller_unix_socket_on_create);
        break;
    case PROP_DAEMON:
        g_value_set_boolean(value, priv->daemon);
        break;
    case PROP_PID_FILE:
        g_value_set_string(value, priv->pid_file);
        break;
    case PROP_MAINTENANCE_INTERVAL:
        g_value_set_uint(value, priv->maintenance_interval);
        break;
    case PROP_SUSPEND_TIME_ON_UNACCEPTABLE:
        g_value_set_uint(value, priv->suspend_time_on_unacceptable);
        break;
    case PROP_MAX_CONNECTIONS:
        g_value_set_uint(value, priv->max_connections);
        break;
    case PROP_MAX_FILE_DESCRIPTORS:
        g_value_set_uint(value, priv->max_file_descriptors);
        break;
    case PROP_CUSTOM_CONFIGURATION_DIRECTORY:
        g_value_set_string(value, priv->custom_configuration_directory);
        break;
    case PROP_CONNECTION_CHECK_INTERVAL:
        g_value_set_uint(value, priv->connection_check_interval);
        break;
    case PROP_EVENT_LOOP_BACKEND:
        g_value_set_enum(value, priv->event_loop_backend);
        break;
    case PROP_N_WORKERS:
        g_value_set_uint(value, priv->n_workers);
        break;
    case PROP_DEFAULT_PACKET_BUFFER_SIZE:
        g_value_set_uint(value, priv->default_packet_buffer_size);
        break;
    case PROP_PREFIX:
        g_value_set_string(value, PREFIX);
        break;
    case PROP_USE_SYSLOG:
        g_value_set_boolean(value, priv->use_syslog);
        break;
    case PROP_SYSLOG_FACILITY:
        g_value_set_string(value, priv->syslog_facility);
        break;
    case PROP_CHUNK_SIZE:
        g_value_set_uint(value, priv->chunk_size);
        break;
    case PROP_MAX_PENDING_FINISHED_SESSIONS:
        g_value_set_uint(value, priv->max_pending_finished_sessions);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static const gchar *
_milter_manager_configuration_module_dir (void)
{
    const gchar *dir;

    if (configuration_module_dir)
        return configuration_module_dir;

    dir = g_getenv("MILTER_MANAGER_CONFIGURATION_MODULE_DIR");
    if (dir)
        return dir;

    return CONFIGURATION_MODULE_DIR;
}

static MilterManagerModule *
milter_manager_configuration_load_module (const gchar *name)
{
    gchar *module_name;
    MilterManagerModule *module;

    module_name = g_strdup_printf("milter-manager-%s-configuration", name);
    module = milter_manager_module_find(configurations, module_name);
    if (!module) {
        const gchar *module_dir;

        module_dir = _milter_manager_configuration_module_dir();
        module = milter_manager_module_load_module(module_dir, module_name);
        if (module) {
            if (g_type_module_use(G_TYPE_MODULE(module))) {
                configurations = g_list_prepend(configurations, module);
            }
        }
    }

    g_free(module_name);
    return module;
}

void
_milter_manager_configuration_init (void)
{
}

static void
milter_manager_configuration_set_default_module_dir (const gchar *dir)
{
    if (configuration_module_dir)
        g_free(configuration_module_dir);
    configuration_module_dir = NULL;

    if (dir)
        configuration_module_dir = g_strdup(dir);
}

static void
milter_manager_configuration_unload (void)
{
    g_list_foreach(configurations, (GFunc)milter_manager_module_unload, NULL);
    g_list_free(configurations);
    configurations = NULL;
}

void
_milter_manager_configuration_quit (void)
{
    milter_manager_configuration_unload();
    milter_manager_configuration_set_default_module_dir(NULL);
}

GQuark
milter_manager_configuration_error_quark (void)
{
    return g_quark_from_static_string("milter-manager-configuration-error-quark");
}

static MilterManagerConfiguration *
milter_manager_configuration_instantiate_va_list (const gchar *first_property,
                                                  va_list var_args)
{
    MilterManagerModule *module;
    GObject *configuration;

    module = milter_manager_configuration_load_module("ruby");
    g_return_val_if_fail(module != NULL, NULL);

    configuration = milter_manager_module_instantiate(module,
                                                      first_property, var_args);
    return MILTER_MANAGER_CONFIGURATION(configuration);
}

MilterManagerConfiguration *
milter_manager_configuration_instantiate (const gchar *first_property,
                                          ...)
{
    MilterManagerConfiguration *configuration;
    va_list var_args;

    va_start(var_args, first_property);
    configuration =
        milter_manager_configuration_instantiate_va_list(first_property,
                                                         var_args);
    va_end(var_args);

    return configuration;
}

MilterManagerConfiguration *
milter_manager_configuration_new (const gchar *first_property,
                                  ...)
{
    MilterManagerConfiguration *configuration;
    va_list var_args;
    GError *error = NULL;

    va_start(var_args, first_property);
    configuration =
        milter_manager_configuration_instantiate_va_list(first_property,
                                                         var_args);
    va_end(var_args);

    milter_manager_configuration_clear(configuration, &error);
    if (error) {
        milter_error("[configuration][new][clear][error] %s",
                     error->message);
        g_error_free(error);
    }

    return configuration;
}

void
milter_manager_configuration_append_load_path (MilterManagerConfiguration *configuration,
                                               const gchar *path)
{
    MilterManagerConfigurationPrivate *priv;

    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration);
    priv->load_paths = g_list_append(priv->load_paths, g_strdup(path));
}

void
milter_manager_configuration_prepend_load_path (MilterManagerConfiguration *configuration,
                                                const gchar *path)
{
    MilterManagerConfigurationPrivate *priv;

    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration);
    priv->load_paths = g_list_prepend(priv->load_paths, g_strdup(path));
}

void
milter_manager_configuration_clear_load_paths (MilterManagerConfiguration *configuration)
{
    MilterManagerConfigurationPrivate *priv;

    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration);
    if (priv->load_paths) {
        g_list_foreach(priv->load_paths, (GFunc)g_free, NULL);
        g_list_free(priv->load_paths);
        priv->load_paths = NULL;
    }
}

const GList *
milter_manager_configuration_get_load_paths (MilterManagerConfiguration *configuration)
{
    return MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration)->load_paths;
}

static gchar *
find_file (MilterManagerConfiguration *configuration,
           const gchar *file_name, GError **error)
{
    MilterManagerConfigurationPrivate *priv;
    gchar *full_path = NULL;
    GList *node;

    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration);
    if (g_path_is_absolute(file_name)) {
        full_path = g_strdup(file_name);
    } else {
        for (node = priv->load_paths; node; node = g_list_next(node)) {
            const gchar *dir = node->data;

            full_path = g_build_filename(dir, file_name, NULL);
            if (g_file_test(full_path, G_FILE_TEST_IS_REGULAR)) {
                break;
            } else {
                g_free(full_path);
                full_path = NULL;
            }
        }
    }

    if (full_path) {
        milter_debug("[configuration][path][found] <%s>", full_path);
    } else {
        GError *local_error = NULL;
        GString *inspected_load_paths;

        inspected_load_paths = g_string_new("[");
        for (node = priv->load_paths; node; node = g_list_next(node)) {
            const gchar *dir = node->data;

            g_string_append_printf(inspected_load_paths, "\"%s\"", dir);
            if (g_list_next(node))
                g_string_append(inspected_load_paths, ", ");
        }
        g_string_append(inspected_load_paths, "]");
        g_set_error(&local_error,
                    MILTER_MANAGER_CONFIGURATION_ERROR,
                    MILTER_MANAGER_CONFIGURATION_ERROR_NOT_EXIST,
                    "path %s doesn't exist in load path (%s)",
                    file_name, inspected_load_paths->str);
        milter_debug("[configuration][path][nonexistent] %s",
                     local_error->message);
        g_string_free(inspected_load_paths, TRUE);
        g_propagate_error(error, local_error);
    }

    return full_path;
}

gboolean
milter_manager_configuration_load (MilterManagerConfiguration *configuration,
                                   const gchar *file_name, GError **error)
{
    MilterManagerConfigurationClass *configuration_class;

    configuration_class = MILTER_MANAGER_CONFIGURATION_GET_CLASS(configuration);
    if (configuration_class->load) {
        gboolean success;
        gchar *full_path;

        full_path = find_file(configuration, file_name, error);
        if (!full_path)
            return FALSE;

        success = configuration_class->load(configuration, full_path, error);
        g_free(full_path);
        return success;
    } else {
        g_set_error(error,
                    MILTER_MANAGER_CONFIGURATION_ERROR,
                    MILTER_MANAGER_CONFIGURATION_ERROR_NOT_IMPLEMENTED,
                    "load isn't implemented");
        return FALSE;
    }
}

gboolean
milter_manager_configuration_load_custom (MilterManagerConfiguration *configuration,
                                          const gchar *file_name, GError **error)
{
    MilterManagerConfigurationClass *configuration_class;

    configuration_class = MILTER_MANAGER_CONFIGURATION_GET_CLASS(configuration);
    if (configuration_class->load_custom) {
        gboolean success;
        gchar *full_path;

        full_path = find_file(configuration, file_name, error);
        if (!full_path)
            return FALSE;

        success = configuration_class->load_custom(configuration,
                                                   full_path,
                                                   error);
        g_free(full_path);
        return success;
    } else {
        g_set_error(error,
                    MILTER_MANAGER_CONFIGURATION_ERROR,
                    MILTER_MANAGER_CONFIGURATION_ERROR_NOT_IMPLEMENTED,
                    "load_custom isn't implemented");
        return FALSE;
    }
}

gboolean
milter_manager_configuration_load_if_exist (MilterManagerConfiguration *configuration,
                                            const gchar *file_name,
                                            GError **error)
{
    GError *local_error = NULL;

    if (milter_manager_configuration_load(configuration, file_name,
                                          &local_error))
        return TRUE;

    if (local_error->code == MILTER_MANAGER_CONFIGURATION_ERROR_NOT_EXIST) {
        g_error_free(local_error);
        return TRUE;
    }

    g_propagate_error(error, local_error);
    return FALSE;
}

gboolean
milter_manager_configuration_load_custom_if_exist (MilterManagerConfiguration *configuration,
                                                   const gchar *file_name,
                                                   GError **error)
{
    GError *local_error = NULL;

    if (milter_manager_configuration_load_custom(configuration, file_name,
                                                 &local_error))
        return TRUE;

    if (local_error->code == MILTER_MANAGER_CONFIGURATION_ERROR_NOT_EXIST) {
        g_error_free(local_error);
        return TRUE;
    }

    g_propagate_error(error, local_error);
    return FALSE;
}

gboolean
milter_manager_configuration_reload (MilterManagerConfiguration *configuration,
                                     GError **error)
{
    GError *local_error = NULL;

    if (!milter_manager_configuration_clear(configuration, &local_error)) {
        milter_error("[configuration][load][clear][error] <%s>: %s",
                     CONFIG_FILE_NAME, local_error->message);
        g_propagate_error(error, local_error);
        return FALSE;
    }

    if (!milter_manager_configuration_load(configuration, CONFIG_FILE_NAME,
                                           &local_error)) {
        milter_error("[configuration][load][error] <%s>: %s",
                     CONFIG_FILE_NAME, local_error->message);
        g_propagate_error(error, local_error);
        return FALSE;
    }

    if (!milter_manager_configuration_load_custom_if_exist(
            configuration, CUSTOM_CONFIG_FILE_NAME, &local_error)) {
        milter_error("[configuration][load][custom][error] <%s>: %s",
                     CUSTOM_CONFIG_FILE_NAME, local_error->message);
        g_propagate_error(error, local_error);
        return FALSE;
    }

    return TRUE;
}

gboolean
milter_manager_configuration_save_custom (MilterManagerConfiguration *configuration,
                                          const gchar                *content,
                                          gssize                      size,
                                          GError                    **error)
{
    MilterManagerConfigurationPrivate *priv;
    GError *local_error = NULL;
    GList *node;
    gboolean success = FALSE;
    GString *inspected_tried_paths;

    inspected_tried_paths = g_string_new("[");
    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration);
    for (node = priv->load_paths; node; node = g_list_next(node)) {
        const gchar *dir = node->data;
        gchar *full_path;

        full_path = g_build_filename(dir, CUSTOM_CONFIG_FILE_NAME, NULL);
        g_string_append_printf(inspected_tried_paths, "\"%s\"", full_path);
        if (g_file_set_contents(full_path, content, size, &local_error)) {
            success = TRUE;
        } else {
            g_string_append_printf(inspected_tried_paths, "(%s)",
                                   local_error->message);
            g_error_free(local_error);
            local_error = NULL;
            if (g_list_next(node))
                g_string_append(inspected_tried_paths, ", ");
        }
        g_free(full_path);
        if (success)
            break;
    }
    g_string_append(inspected_tried_paths, "]");

    if (!success) {
        g_set_error(error,
                    MILTER_MANAGER_CONFIGURATION_ERROR,
                    MILTER_MANAGER_CONFIGURATION_ERROR_SAVE,
                    "can't find writable custom configuration file in %s",
                    inspected_tried_paths->str);
        g_string_free(inspected_tried_paths, TRUE);
    }

    return success;
}

gboolean
milter_manager_configuration_is_privilege_mode (MilterManagerConfiguration *configuration)
{
    return MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration)->privilege_mode;
}

void
milter_manager_configuration_set_privilege_mode (MilterManagerConfiguration *configuration,
                                                 gboolean mode)
{
    MilterManagerConfigurationPrivate *priv;

    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration);
    priv->privilege_mode = mode;
}

const gchar *
milter_manager_configuration_get_controller_connection_spec (MilterManagerConfiguration *configuration)
{
    return MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration)->controller_connection_spec;
}

void
milter_manager_configuration_set_controller_connection_spec (MilterManagerConfiguration *configuration,
                                                             const gchar *spec)
{
    MilterManagerConfigurationPrivate *priv;

    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration);
    if (priv->controller_connection_spec)
        g_free(priv->controller_connection_spec);
    priv->controller_connection_spec = g_strdup(spec);
}

const gchar *
milter_manager_configuration_get_manager_connection_spec (MilterManagerConfiguration *configuration)
{
    return MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration)->manager_connection_spec;
}

void
milter_manager_configuration_set_manager_connection_spec (MilterManagerConfiguration *configuration,
                                                          const gchar *spec)
{
    MilterManagerConfigurationPrivate *priv;

    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration);
    if (priv->manager_connection_spec)
        g_free(priv->manager_connection_spec);
    priv->manager_connection_spec = g_strdup(spec);
}

const gchar *
milter_manager_configuration_get_effective_user (MilterManagerConfiguration *configuration)
{
    return MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration)->effective_user;
}

void
milter_manager_configuration_set_effective_user (MilterManagerConfiguration *configuration,
                                                 const gchar *effective_user)
{
    MilterManagerConfigurationPrivate *priv;

    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration);
    if (priv->effective_user)
        g_free(priv->effective_user);
    priv->effective_user = g_strdup(effective_user);
}

const gchar *
milter_manager_configuration_get_effective_group (MilterManagerConfiguration *configuration)
{
    return MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration)->effective_group;
}

void
milter_manager_configuration_set_effective_group (MilterManagerConfiguration *configuration,
                                                  const gchar *effective_group)
{
    MilterManagerConfigurationPrivate *priv;

    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration);
    if (priv->effective_group)
        g_free(priv->effective_group);
    priv->effective_group = g_strdup(effective_group);
}

guint
milter_manager_configuration_get_manager_unix_socket_mode (MilterManagerConfiguration *configuration)
{
    MilterManagerConfigurationPrivate *priv;

    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration);
    return priv->manager_unix_socket_mode;
}

void
milter_manager_configuration_set_manager_unix_socket_mode (MilterManagerConfiguration *configuration,
                                                           guint mode)
{
    MilterManagerConfigurationPrivate *priv;

    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration);
    priv->manager_unix_socket_mode = mode;
}

guint
milter_manager_configuration_get_controller_unix_socket_mode (MilterManagerConfiguration *configuration)
{
    MilterManagerConfigurationPrivate *priv;

    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration);
    return priv->controller_unix_socket_mode;
}

void
milter_manager_configuration_set_controller_unix_socket_mode (MilterManagerConfiguration *configuration,
                                                              guint mode)
{
    MilterManagerConfigurationPrivate *priv;

    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration);
    priv->controller_unix_socket_mode = mode;
}

const gchar *
milter_manager_configuration_get_manager_unix_socket_group (MilterManagerConfiguration *configuration)
{
    MilterManagerConfigurationPrivate *priv;

    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration);
    return priv->manager_unix_socket_group;
}

void
milter_manager_configuration_set_manager_unix_socket_group (MilterManagerConfiguration *configuration,
                                                            const gchar *group)
{
    MilterManagerConfigurationPrivate *priv;

    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration);
    if (priv->manager_unix_socket_group)
        g_free(priv->manager_unix_socket_group);
    priv->manager_unix_socket_group = g_strdup(group);
}

const gchar *
milter_manager_configuration_get_controller_unix_socket_group (MilterManagerConfiguration *configuration)
{
    MilterManagerConfigurationPrivate *priv;

    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration);
    return priv->controller_unix_socket_group;
}

void
milter_manager_configuration_set_controller_unix_socket_group (MilterManagerConfiguration *configuration,
                                                               const gchar *group)
{
    MilterManagerConfigurationPrivate *priv;

    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration);
    if (priv->controller_unix_socket_group)
        g_free(priv->controller_unix_socket_group);
    priv->controller_unix_socket_group = g_strdup(group);
}

gboolean
milter_manager_configuration_is_remove_manager_unix_socket_on_close (MilterManagerConfiguration *configuration)
{
    MilterManagerConfigurationPrivate *priv;

    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration);
    return priv->remove_manager_unix_socket_on_close;
}

void
milter_manager_configuration_set_remove_manager_unix_socket_on_close (MilterManagerConfiguration *configuration,
                                                                      gboolean remove)
{
    MilterManagerConfigurationPrivate *priv;

    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration);
    priv->remove_manager_unix_socket_on_close = remove;
}

gboolean
milter_manager_configuration_is_remove_controller_unix_socket_on_close (MilterManagerConfiguration *configuration)
{
    MilterManagerConfigurationPrivate *priv;

    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration);
    return priv->remove_controller_unix_socket_on_close;
}

void
milter_manager_configuration_set_remove_controller_unix_socket_on_close (MilterManagerConfiguration *configuration,
                                                                         gboolean remove)
{
    MilterManagerConfigurationPrivate *priv;

    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration);
    priv->remove_controller_unix_socket_on_close = remove;
}

gboolean
milter_manager_configuration_is_remove_manager_unix_socket_on_create (MilterManagerConfiguration *configuration)
{
    MilterManagerConfigurationPrivate *priv;

    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration);
    return priv->remove_manager_unix_socket_on_create;
}

void
milter_manager_configuration_set_remove_manager_unix_socket_on_create (MilterManagerConfiguration *configuration,
                                                                      gboolean remove)
{
    MilterManagerConfigurationPrivate *priv;

    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration);
    priv->remove_manager_unix_socket_on_create = remove;
}

gboolean
milter_manager_configuration_is_remove_controller_unix_socket_on_create (MilterManagerConfiguration *configuration)
{
    MilterManagerConfigurationPrivate *priv;

    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration);
    return priv->remove_controller_unix_socket_on_create;
}

void
milter_manager_configuration_set_remove_controller_unix_socket_on_create (MilterManagerConfiguration *configuration,
                                                                         gboolean remove)
{
    MilterManagerConfigurationPrivate *priv;

    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration);
    priv->remove_controller_unix_socket_on_create = remove;
}

gboolean
milter_manager_configuration_is_daemon (MilterManagerConfiguration *configuration)
{
    return MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration)->daemon;
}

void
milter_manager_configuration_set_daemon (MilterManagerConfiguration *configuration,
                                         gboolean daemon)
{
    MilterManagerConfigurationPrivate *priv;

    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration);
    priv->daemon = daemon;
}

const gchar *
milter_manager_configuration_get_pid_file (MilterManagerConfiguration *configuration)
{
    return MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration)->pid_file;
}

void
milter_manager_configuration_set_pid_file (MilterManagerConfiguration *configuration,
                                           const gchar *pid_file)
{
    MilterManagerConfigurationPrivate *priv;

    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration);
    if (priv->pid_file)
        g_free(priv->pid_file);
    priv->pid_file = g_strdup(pid_file);
}

guint
milter_manager_configuration_get_maintenance_interval (MilterManagerConfiguration *configuration)
{
    MilterManagerConfigurationPrivate *priv;

    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration);
    return priv->maintenance_interval;
}

void
milter_manager_configuration_set_maintenance_interval (MilterManagerConfiguration *configuration,
                                                       guint n_sessions)
{
    MilterManagerConfigurationPrivate *priv;

    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration);
    priv->maintenance_interval = n_sessions;
}

guint
milter_manager_configuration_get_suspend_time_on_unacceptable (MilterManagerConfiguration *configuration)
{
    MilterManagerConfigurationPrivate *priv;

    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration);
    return priv->suspend_time_on_unacceptable;
}

void
milter_manager_configuration_set_suspend_time_on_unacceptable (MilterManagerConfiguration *configuration,
                                                               guint suspend_time)
{
    MilterManagerConfigurationPrivate *priv;

    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration);
    priv->suspend_time_on_unacceptable = suspend_time;
}

guint
milter_manager_configuration_get_max_connections (MilterManagerConfiguration *configuration)
{
    MilterManagerConfigurationPrivate *priv;

    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration);
    return priv->max_connections;
}

void
milter_manager_configuration_set_max_connections (MilterManagerConfiguration *configuration,
                                                               guint suspend_time)
{
    MilterManagerConfigurationPrivate *priv;

    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration);
    priv->max_connections = suspend_time;
}

guint
milter_manager_configuration_get_max_file_descriptors (MilterManagerConfiguration *configuration)
{
    MilterManagerConfigurationPrivate *priv;

    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration);
    return priv->max_file_descriptors;
}

void
milter_manager_configuration_set_max_file_descriptors (MilterManagerConfiguration *configuration,
                                                               guint suspend_time)
{
    MilterManagerConfigurationPrivate *priv;

    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration);
    priv->max_file_descriptors = suspend_time;
}

const gchar *
milter_manager_configuration_get_custom_configuration_directory (MilterManagerConfiguration *configuration)
{
    return MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration)->custom_configuration_directory;
}

void
milter_manager_configuration_set_custom_configuration_directory (MilterManagerConfiguration *configuration,
                                                                 const gchar *custom_configuration_directory)
{
    MilterManagerConfigurationPrivate *priv;

    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration);
    if (priv->custom_configuration_directory)
        g_free(priv->custom_configuration_directory);
    priv->custom_configuration_directory = g_strdup(custom_configuration_directory);
}

static gboolean
cb_connected_dummy (MilterManagerConfiguration *configuration,
                    MilterManagerLeader *leader,
                    gpointer user_data)
{
    return TRUE;
}

void
milter_manager_configuration_clear_signal_handlers (MilterManagerConfiguration *configuration)
{
    gulong i, max_id;
    gboolean handler_id_overflowed;
    static gulong previous_max_id = 0;

    max_id = g_signal_connect(configuration, "connected",
                              G_CALLBACK(cb_connected_dummy), NULL);

    handler_id_overflowed = previous_max_id > max_id;
    if (handler_id_overflowed) {
        for (i = previous_max_id + 1; i <= G_MAXULONG; i++) {
            if (g_signal_handler_is_connected(configuration, i)) {
                g_signal_handler_disconnect(configuration, i);
            }
        }
    }
    previous_max_id = max_id;

    for (i = 0; i <= max_id; i++) {
        if (g_signal_handler_is_connected(configuration, i)) {
            g_signal_handler_disconnect(configuration, i);
        }
    }
}

void
milter_manager_configuration_add_egg (MilterManagerConfiguration *configuration,
                                      MilterManagerEgg         *egg)
{
    MilterManagerConfigurationPrivate *priv;

    g_return_if_fail(egg != NULL);

    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration);

    g_object_ref(egg);
    priv->eggs = g_list_append(priv->eggs, egg);
}

MilterManagerEgg *
milter_manager_configuration_find_egg (MilterManagerConfiguration *configuration,
                                       const gchar *name)
{
    MilterManagerConfigurationPrivate *priv;
    GList *node;

    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration);

    for (node = priv->eggs; node; node = g_list_next(node)) {
        MilterManagerEgg *egg = node->data;

        if (g_str_equal(name, milter_manager_egg_get_name(egg)))
            return egg;
    }

    return NULL;
}

const GList *
milter_manager_configuration_get_eggs (MilterManagerConfiguration *configuration)
{
    return MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration)->eggs;
}

void
milter_manager_configuration_remove_egg (MilterManagerConfiguration *configuration,
                                         MilterManagerEgg *egg)
{
    const gchar *name;

    name = milter_manager_egg_get_name(egg);
    milter_manager_configuration_remove_egg_by_name(configuration, name);
}

void
milter_manager_configuration_remove_egg_by_name (MilterManagerConfiguration *configuration,
                                                 const gchar *name)
{
    MilterManagerConfigurationPrivate *priv;
    GList *node;

    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration);

    for (node = priv->eggs; node; node = g_list_next(node)) {
        MilterManagerEgg *egg = node->data;

        if (g_str_equal(name, milter_manager_egg_get_name(egg))) {
            priv->eggs = g_list_delete_link(priv->eggs, node);
            g_object_unref(egg);
            break;
        }
    }
}

void
milter_manager_configuration_clear_eggs (MilterManagerConfiguration *configuration)
{
    MilterManagerConfigurationPrivate *priv;

    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration);

    if (priv->eggs) {
        g_list_foreach(priv->eggs, (GFunc)g_object_unref, NULL);
        g_list_free(priv->eggs);
        priv->eggs = NULL;
    }
}

void
milter_manager_configuration_setup_children (MilterManagerConfiguration *configuration,
                                             MilterManagerChildren *children,
                                             MilterClientContext *context)
{
    GList *node;
    MilterManagerConfigurationPrivate *priv;

    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration);

    for (node = priv->eggs; node; node = g_list_next(node)) {
        MilterManagerChild *child;
        MilterManagerEgg *egg = node->data;

        if (!milter_manager_egg_is_enabled(egg))
            continue;

        child = milter_manager_egg_hatch(egg);
        if (child) {
            milter_manager_children_add_child(children, child);
            milter_manager_egg_attach_applicable_conditions(egg,
                                                            child, children,
                                                            context);
            g_object_unref(child);
        }
    }
}

MilterStatus
milter_manager_configuration_get_fallback_status
                                     (MilterManagerConfiguration *configuration)
{
    MilterManagerConfigurationPrivate *priv;

    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration);
    return priv->fallback_status;
}

void
milter_manager_configuration_set_fallback_status
                                     (MilterManagerConfiguration *configuration,
                                      MilterStatus status)
{
    MilterManagerConfigurationPrivate *priv;

    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration);
    priv->fallback_status = status;
}

MilterStatus
milter_manager_configuration_get_fallback_status_at_disconnect
                                     (MilterManagerConfiguration *configuration)
{
    MilterManagerConfigurationPrivate *priv;

    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration);
    return priv->fallback_status_at_disconnect;
}

void
milter_manager_configuration_set_fallback_status_at_disconnect
                                     (MilterManagerConfiguration *configuration,
                                      MilterStatus status)
{
    MilterManagerConfigurationPrivate *priv;

    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration);
    priv->fallback_status_at_disconnect = status;
}

const gchar *
milter_manager_configuration_get_package_platform
                                     (MilterManagerConfiguration *configuration)
{
    return MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration)->package_platform;
}

void
milter_manager_configuration_set_package_platform
                                     (MilterManagerConfiguration *configuration,
                                      const gchar *platform)
{
    MilterManagerConfigurationPrivate *priv;

    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration);
    if (priv->package_platform)
        g_free(priv->package_platform);
    priv->package_platform = g_strdup(platform);
}

const gchar *
milter_manager_configuration_get_package_options
                                     (MilterManagerConfiguration *configuration)
{
    return MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration)->package_options;
}

void
milter_manager_configuration_set_package_options
                                     (MilterManagerConfiguration *configuration,
                                      const gchar *options)
{
    MilterManagerConfigurationPrivate *priv;

    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration);
    if (priv->package_options)
        g_free(priv->package_options);
    priv->package_options = g_strdup(options);
}

void
milter_manager_configuration_add_applicable_condition (MilterManagerConfiguration *configuration,
                                                       MilterManagerApplicableCondition *condition)
{
    MilterManagerConfigurationPrivate *priv;

    g_return_if_fail(condition != NULL);

    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration);

    g_object_ref(condition);
    priv->applicable_conditions =
        g_list_append(priv->applicable_conditions, condition);
}

MilterManagerApplicableCondition *
milter_manager_configuration_find_applicable_condition (MilterManagerConfiguration *configuration,
                                                        const gchar *name)
{
    MilterManagerConfigurationPrivate *priv;
    GList *node;

    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration);

    for (node = priv->applicable_conditions; node; node = g_list_next(node)) {
        MilterManagerApplicableCondition *condition = node->data;
        const gchar *condition_name;

        condition_name = milter_manager_applicable_condition_get_name(condition);
        if (g_str_equal(name, condition_name))
            return condition;
    }

    return NULL;
}

const GList *
milter_manager_configuration_get_applicable_conditions (MilterManagerConfiguration *configuration)
{
    MilterManagerConfigurationPrivate *priv;

    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration);
    return priv->applicable_conditions;
}

void
milter_manager_configuration_remove_applicable_condition (MilterManagerConfiguration *configuration,
                                                          MilterManagerApplicableCondition *condition)
{
    const gchar *name;

    name = milter_manager_applicable_condition_get_name(condition);
    milter_manager_configuration_remove_applicable_condition_by_name(configuration,
                                                                     name);
}

void
milter_manager_configuration_remove_applicable_condition_by_name (MilterManagerConfiguration *configuration,
                                                                  const gchar *name)
{
    MilterManagerConfigurationPrivate *priv;
    GList *node;

    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration);

    for (node = priv->applicable_conditions; node; node = g_list_next(node)) {
        MilterManagerApplicableCondition *condition = node->data;
        const gchar *condition_name;

        condition_name = milter_manager_applicable_condition_get_name(condition);
        if (g_str_equal(name, condition_name)) {
            priv->applicable_conditions =
                g_list_delete_link(priv->applicable_conditions, node);
            g_object_unref(condition);
            break;
        }
    }
}

void
milter_manager_configuration_clear_applicable_conditions (MilterManagerConfiguration *configuration)
{
    MilterManagerConfigurationPrivate *priv;

    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration);

    if (priv->applicable_conditions) {
        g_list_foreach(priv->applicable_conditions, (GFunc)g_object_unref, NULL);
        g_list_free(priv->applicable_conditions);
        priv->applicable_conditions = NULL;
    }
}

static void
clear_controller (MilterManagerConfigurationPrivate *priv)
{
    if (priv->controller_connection_spec) {
        g_free(priv->controller_connection_spec);
        priv->controller_connection_spec = NULL;
    }

    if (priv->controller_unix_socket_group) {
        g_free(priv->controller_unix_socket_group);
        priv->controller_unix_socket_group = NULL;
    }
}

static void
clear_manager (MilterManagerConfigurationPrivate *priv)
{
    if (priv->manager_connection_spec) {
        g_free(priv->manager_connection_spec);
        priv->manager_connection_spec = NULL;
    }
    priv->manager_connection_spec =
        g_strdup(MILTER_MANAGER_DEFAULT_CONNECTION_SPEC);

    priv->privilege_mode = FALSE;
    priv->daemon = FALSE;
    priv->fallback_status = DEFAULT_FALLBACK_STATUS;
    priv->fallback_status_at_disconnect = DEFAULT_FALLBACK_STATUS_AT_DISCONNECT;
    priv->maintenance_interval = DEFAULT_MAINTENANCE_INTERVAL;
    priv->suspend_time_on_unacceptable =
        MILTER_CLIENT_DEFAULT_SUSPEND_TIME_ON_UNACCEPTABLE;
    priv->max_connections = 0;
    priv->max_file_descriptors = 0;
    priv->connection_check_interval = DEFAULT_CONNECTION_CHECK_INTERVAL;
    priv->event_loop_backend = MILTER_CLIENT_EVENT_LOOP_BACKEND_GLIB;
    priv->n_workers = 0;
    priv->default_packet_buffer_size = 0;
    priv->chunk_size = MILTER_CHUNK_SIZE;
    priv->max_pending_finished_sessions = 0;
}

static void
clear_package (MilterManagerConfigurationPrivate *priv)
{
    if (priv->package_platform) {
        g_free(priv->package_platform);
        priv->package_platform = g_strdup(MILTER_MANAGER_PACKAGE_PLATFORM);
    }

    if (priv->package_options) {
        g_free(priv->package_options);
        priv->package_options = g_strdup(MILTER_MANAGER_PACKAGE_OPTIONS);
    }
}

static void
clear_account (MilterManagerConfigurationPrivate *priv)
{
    if (priv->effective_user) {
        g_free(priv->effective_user);
        priv->effective_user = NULL;
    }
    priv->effective_user = g_strdup(MILTER_MANAGER_DEFAULT_EFFECTIVE_USER);

    if (priv->effective_group) {
        g_free(priv->effective_group);
        priv->effective_group = NULL;
    }
    priv->effective_group = g_strdup(MILTER_MANAGER_DEFAULT_EFFECTIVE_GROUP);
}

static void
clear_process (MilterManagerConfigurationPrivate *priv)
{
    if (priv->pid_file) {
        g_free(priv->pid_file);
        priv->pid_file = NULL;
    }
    priv->pid_file = g_strdup(MILTER_MANAGER_DEFAULT_PID_FILE);
}

static void
clear_configuration (MilterManagerConfigurationPrivate *priv)
{
    if (priv->custom_configuration_directory) {
        g_free(priv->custom_configuration_directory);
        priv->custom_configuration_directory = NULL;
    }

    if (priv->locations)
        g_hash_table_remove_all(priv->locations);
}

static void
clear_socket (MilterManagerConfigurationPrivate *priv)
{
    if (priv->manager_unix_socket_group) {
        g_free(priv->manager_unix_socket_group);
        priv->manager_unix_socket_group = NULL;
    }
    priv->manager_unix_socket_group =
        g_strdup(MILTER_MANAGER_DEFAULT_SOCKET_GROUP);

    priv->manager_unix_socket_mode = 0660;
    priv->controller_unix_socket_mode = 0660;
    priv->remove_manager_unix_socket_on_close = TRUE;
    priv->remove_controller_unix_socket_on_close = TRUE;
    priv->remove_manager_unix_socket_on_create = TRUE;
    priv->remove_controller_unix_socket_on_create = TRUE;
}

static void
clear_log (MilterManagerConfigurationPrivate *priv)
{
    priv->use_syslog = TRUE;
    if (priv->syslog_facility) {
        g_free(priv->syslog_facility);
        priv->syslog_facility = NULL;
    }
}

gboolean
milter_manager_configuration_clear (MilterManagerConfiguration *configuration,
                                    GError **error)
{
    MilterManagerConfigurationPrivate *priv;
    MilterManagerConfigurationClass *configuration_class;

    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration);

    milter_manager_configuration_clear_signal_handlers(configuration);
    milter_manager_configuration_clear_eggs(configuration);
    milter_manager_configuration_clear_applicable_conditions(configuration);
    clear_controller(priv);
    clear_manager(priv);
    clear_package(priv);
    clear_account(priv);
    clear_process(priv);
    clear_configuration(priv);
    clear_socket(priv);
    clear_log(priv);

    configuration_class = MILTER_MANAGER_CONFIGURATION_GET_CLASS(configuration);
    if (configuration_class->clear) {
        GError *local_error = NULL;

        milter_debug("[configuration][clear][custom]");
        if (!configuration_class->clear(configuration, &local_error)) {
            milter_error("[configuration][clear][custom][error] %s",
                         local_error->message);
            g_propagate_error(error, local_error);
            return FALSE;
        }
    }

    return TRUE;
}

GPid
milter_manager_configuration_fork (MilterManagerConfiguration *configuration)
{
    MilterManagerConfigurationClass *configuration_class;

    configuration_class = MILTER_MANAGER_CONFIGURATION_GET_CLASS(configuration);
    if (configuration_class->fork) {
        return configuration_class->fork(configuration);
    } else {
        return fork();
    }
}

void
milter_manager_configuration_maintain (MilterManagerConfiguration *configuration)
{
    MilterManagerConfigurationClass *configuration_class;

    configuration_class = MILTER_MANAGER_CONFIGURATION_GET_CLASS(configuration);
    if (configuration_class->maintain) {
        GError *error = NULL;
        milter_debug("[configuration][maintain][start]");
        if (!configuration_class->maintain(configuration, &error)) {
            milter_error("[configuration][maintain][error] %s", error->message);
            g_error_free(error);
        }
        milter_debug("[configuration][maintain][end]");
    }
}

void
milter_manager_configuration_event_loop_created (MilterManagerConfiguration *configuration,
                                                 MilterEventLoop *loop)
{
    MilterManagerConfigurationClass *configuration_class;

    configuration_class = MILTER_MANAGER_CONFIGURATION_GET_CLASS(configuration);
    if (configuration_class->event_loop_created) {
        GError *error = NULL;
        milter_debug("[configuration][event-loop-created][start]");
        if (!configuration_class->event_loop_created(configuration, loop,
                                                     &error)) {
            milter_error("[configuration][event-loop-created][error] %s",
                         error->message);
            g_error_free(error);
        }
        milter_debug("[configuration][event-loop-created][end]");
    }
}

gchar *
milter_manager_configuration_to_xml (MilterManagerConfiguration *configuration)
{
    GString *string;

    string = g_string_new(NULL);
    milter_manager_configuration_to_xml_string(configuration, string, 0);
    return g_string_free(string, FALSE);
}

void
milter_manager_configuration_to_xml_string (MilterManagerConfiguration *configuration,
                                            GString *string, guint indent)
{
    MilterManagerConfigurationPrivate *priv;
    gchar *full_path;

    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration);

    milter_utils_append_indent(string, indent);
    g_string_append(string, "<configuration>\n");

    full_path = find_file(configuration, CUSTOM_CONFIG_FILE_NAME, NULL);
    if (full_path) {
        struct stat status;
        if (g_stat(full_path, &status) == 0) {
            gchar *content;

            content = g_strdup_printf("%" G_GINT64_FORMAT,
                                      (gint64)status.st_mtime);
            milter_utils_xml_append_text_element(string,
                                                 "last-modified", content,
                                                 indent + 2);
        }
        g_free(full_path);
    }

    if (priv->applicable_conditions) {
        GList *node = priv->applicable_conditions;

        milter_utils_append_indent(string, indent + 2);
        g_string_append(string, "<applicable-conditions>\n");
        for (; node; node = g_list_next(node)) {
            MilterManagerApplicableCondition *condition = node->data;
            const gchar *name, *description;

            milter_utils_append_indent(string, indent + 4);
            g_string_append(string, "<applicable-condition>\n");

            name = milter_manager_applicable_condition_get_name(condition);
            milter_utils_xml_append_text_element(string,
                                                 "name", name,
                                                 indent + 6);
            description =
                milter_manager_applicable_condition_get_description(condition);
            if (description)
                milter_utils_xml_append_text_element(string,
                                                     "description", description,
                                                     indent + 6);

            milter_utils_append_indent(string, indent + 4);
            g_string_append(string, "</applicable-condition>\n");
        }
        milter_utils_append_indent(string, indent + 2);
        g_string_append(string, "</applicable-conditions>\n");
    }

    if (priv->eggs) {
        GList *node;

        milter_utils_append_indent(string, indent + 2);
        g_string_append(string, "<milters>\n");
        for (node = priv->eggs; node; node = g_list_next(node)) {
            MilterManagerEgg *egg = node->data;
            milter_manager_egg_to_xml_string(egg, string, indent + 2 + 2);
        }
        milter_utils_append_indent(string, indent + 2);
        g_string_append(string, "</milters>\n");
    }

    g_signal_emit(configuration, signals[TO_XML], 0, string, indent + 2);

    milter_utils_append_indent(string, indent);
    g_string_append(string, "</configuration>\n");
}

gchar *
milter_manager_configuration_dump (MilterManagerConfiguration *configuration)
{
    MilterManagerConfigurationClass *configuration_class;

    configuration_class = MILTER_MANAGER_CONFIGURATION_GET_CLASS(configuration);
    if (configuration_class->dump) {
        return configuration_class->dump(configuration);
    } else {
        return milter_manager_configuration_to_xml(configuration);
    }
}

void
milter_manager_configuration_set_location (MilterManagerConfiguration *configuration,
                                           const gchar *key,
                                           const gchar *file,
                                           gint line)
{
    MilterManagerConfigurationPrivate *priv;
    gconstpointer location;

    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration);
    if (!priv->locations)
        return;

    if (!file) {
        g_hash_table_remove(priv->locations, key);
        return;
    }

    location = g_hash_table_lookup(priv->locations, key);
    if (!location) {
        gchar *duped_key;
        duped_key = g_strdup(key);
        location = duped_key;
        g_hash_table_insert(priv->locations, duped_key, (gpointer)location);
    }
    g_dataset_set_data_full(location, "file", g_strdup(file), g_free);
    g_dataset_set_data(location, "line", GINT_TO_POINTER(line));
}

void
milter_manager_configuration_reset_location (MilterManagerConfiguration *configuration,
                                             const gchar *key)
{
    milter_manager_configuration_set_location(configuration, key, NULL, -1);
}

gconstpointer
milter_manager_configuration_get_location (MilterManagerConfiguration *configuration,
                                           const gchar *key)
{
    MilterManagerConfigurationPrivate *priv;

    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration);
    if (!priv->locations)
        return NULL;

    return g_hash_table_lookup(priv->locations, key);
}

GHashTable *
milter_manager_configuration_get_locations (MilterManagerConfiguration *configuration)
{
    return MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration)->locations;
}

guint
milter_manager_configuration_get_connection_check_interval (MilterManagerConfiguration *configuration)
{
    MilterManagerConfigurationPrivate *priv;

    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration);
    return priv->connection_check_interval;
}

void
milter_manager_configuration_set_connection_check_interval (MilterManagerConfiguration *configuration,
                                                            guint                       interval_in_seconds)
{
    MilterManagerConfigurationPrivate *priv;

    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration);
    priv->connection_check_interval = interval_in_seconds;
}

MilterClientEventLoopBackend
milter_manager_configuration_get_event_loop_backend (MilterManagerConfiguration *configuration)
{
    MilterManagerConfigurationPrivate *priv;

    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration);
    return priv->event_loop_backend;
}

void
milter_manager_configuration_set_event_loop_backend (MilterManagerConfiguration *configuration,
                                                     MilterClientEventLoopBackend backend)
{
    MilterManagerConfigurationPrivate *priv;

    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration);
    priv->event_loop_backend = backend;
}

guint
milter_manager_configuration_get_n_workers (MilterManagerConfiguration *configuration)
{
    MilterManagerConfigurationPrivate *priv;

    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration);
    return priv->n_workers;
}

void
milter_manager_configuration_set_n_workers (MilterManagerConfiguration *configuration,
                                            guint                       n_workers)
{
    MilterManagerConfigurationPrivate *priv;

    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration);
    priv->n_workers = n_workers;
}

guint
milter_manager_configuration_get_default_packet_buffer_size (MilterManagerConfiguration *configuration)
{
    MilterManagerConfigurationPrivate *priv;

    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration);
    return priv->default_packet_buffer_size;
}

void
milter_manager_configuration_set_default_packet_buffer_size (MilterManagerConfiguration *configuration,
                                                             guint                       size)
{
    MilterManagerConfigurationPrivate *priv;

    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration);
    priv->default_packet_buffer_size = size;
}

const gchar *
milter_manager_configuration_get_prefix (MilterManagerConfiguration *configuration)
{
    return PREFIX;
}

gboolean
milter_manager_configuration_get_use_syslog (MilterManagerConfiguration *configuration)
{
    MilterManagerConfigurationPrivate *priv;

    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration);
    return priv->use_syslog;
}

void
milter_manager_configuration_set_use_syslog (MilterManagerConfiguration *configuration,
                                             gboolean                    use_syslog)
{
    MilterManagerConfigurationPrivate *priv;

    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration);
    priv->use_syslog = use_syslog;
}

const gchar *
milter_manager_configuration_get_syslog_facility (MilterManagerConfiguration *configuration)
{
    MilterManagerConfigurationPrivate *priv;

    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration);
    return priv->syslog_facility;
}

void
milter_manager_configuration_set_syslog_facility (MilterManagerConfiguration *configuration,
                                                  const gchar                *facility)
{
    MilterManagerConfigurationPrivate *priv;

    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration);
    if (priv->syslog_facility) {
        g_free(priv->syslog_facility);
    }
    priv->syslog_facility = g_strdup(facility);
}

guint
milter_manager_configuration_get_chunk_size (MilterManagerConfiguration *configuration)
{
    MilterManagerConfigurationPrivate *priv;

    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration);
    return priv->chunk_size;
}

void
milter_manager_configuration_set_chunk_size (MilterManagerConfiguration *configuration,
                                             guint size)
{
    MilterManagerConfigurationPrivate *priv;

    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration);
    priv->chunk_size = MIN(size, MILTER_CHUNK_SIZE);
}

guint
milter_manager_configuration_get_max_pending_finished_sessions (MilterManagerConfiguration *configuration)
{
    MilterManagerConfigurationPrivate *priv;

    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration);
    return priv->max_pending_finished_sessions;
}

void
milter_manager_configuration_set_max_pending_finished_sessions (MilterManagerConfiguration *configuration,
                                                                guint                       n_sessions)
{
    MilterManagerConfigurationPrivate *priv;

    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration);
    priv->max_pending_finished_sessions = n_sessions;
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
