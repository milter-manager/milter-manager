/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Copyright (C) 2011  Kouhei Sutou <kou@clear-code.com>
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

#include "milter-client-runner.h"

#define MILTER_CLIENT_RUNNER_GET_PRIVATE(obj)                   \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj),                         \
                                 MILTER_TYPE_CLIENT_RUNNER,     \
                                 MilterClientRunnerPrivate))

typedef struct _MilterClientRunnerPrivate	MilterClientRunnerPrivate;
struct _MilterClientRunnerPrivate
{
    MilterClient *client;
};

enum
{
    PROP_0,
    PROP_CLIENT
};

G_DEFINE_ABSTRACT_TYPE(MilterClientRunner, milter_client_runner, G_TYPE_OBJECT)


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
milter_client_runner_class_init (MilterClientRunnerClass *klass)
{
    GObjectClass *gobject_class;
    GParamSpec *spec;

    gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->dispose      = dispose;
    gobject_class->set_property = set_property;
    gobject_class->get_property = get_property;

    spec = g_param_spec_object("client",
                               "Client",
                               "The client of the runner",
                               MILTER_TYPE_CLIENT,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    g_object_class_install_property(gobject_class, PROP_CLIENT, spec);

    g_type_class_add_private(gobject_class, sizeof(MilterClientRunnerPrivate));
}

static void
milter_client_runner_init (MilterClientRunner *agent)
{
    MilterClientRunnerPrivate *priv;

    priv = MILTER_CLIENT_RUNNER_GET_PRIVATE(agent);
    priv->client = NULL;
}

static void
dispose (GObject *object)
{
    MilterClientRunner *runner;
    MilterClientRunnerPrivate *priv;

    runner = MILTER_CLIENT_RUNNER(object);
    priv = MILTER_CLIENT_RUNNER_GET_PRIVATE(runner);

    if (priv->client) {
        g_object_unref(priv->client);
        priv->client = NULL;
    }

    G_OBJECT_CLASS(milter_client_runner_parent_class)->dispose(object);
}

static void
set_property (GObject      *object,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
    MilterClientRunnerPrivate *priv;

    priv = MILTER_CLIENT_RUNNER_GET_PRIVATE(object);
    switch (prop_id) {
    case PROP_CLIENT:
        if (priv->client)
            g_object_unref(priv->client);
        priv->client = g_value_dup_object(value);
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
    MilterClientRunner *runner;

    runner = MILTER_CLIENT_RUNNER(object);
    switch (prop_id) {
    case PROP_CLIENT:
        g_value_set_object(value, milter_client_runner_get_client(runner));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

GQuark
milter_client_runner_error_quark (void)
{
    return g_quark_from_static_string("milter-client-runner-error-quark");
}

gboolean
milter_client_runner_run (MilterClientRunner *runner, GError **error)
{
    MilterClientRunnerClass *runner_class;

    runner_class = MILTER_CLIENT_RUNNER_GET_CLASS(runner);
    return runner_class->run(runner, error);
}

void
milter_client_runner_quit (MilterClientRunner *runner)
{
    MilterClientRunnerClass *runner_class;

    runner_class = MILTER_CLIENT_RUNNER_GET_CLASS(runner);
    runner_class->quit(runner);
}

MilterClient *
milter_client_runner_get_client (MilterClientRunner *runner)
{
    MilterClientRunnerPrivate *priv;

    priv = MILTER_CLIENT_RUNNER_GET_PRIVATE(runner);
    return priv->client;
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
