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

#include <gcutter.h>
#include "milter-manager-test-server.h"

#define MILTER_MANAGER_TEST_SERVER_GET_PRIVATE(obj)                    \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj),                          \
                                 MILTER_TYPE_MANAGER_TEST_SERVER,      \
                                 MilterManagerTestServerPrivate))

typedef struct _MilterManagerTestServerPrivate	MilterManagerTestServerPrivate;
struct _MilterManagerTestServerPrivate
{
    gchar *quarantine_reason;
};

enum
{
    PROP_0
};

static MilterReplySignals *reply_parent;
static void reply_init (MilterReplySignalsClass *reply);

G_DEFINE_TYPE_WITH_CODE(MilterManagerTestServer, milter_manager_test_server, MILTER_TYPE_SERVER_CONTEXT,
    G_IMPLEMENT_INTERFACE(MILTER_TYPE_REPLY_SIGNALS, reply_init))

static void dispose        (GObject         *object);
static void set_property   (GObject         *object,
                            guint            prop_id,
                            const GValue    *value,
                            GParamSpec      *pspec);
static void get_property   (GObject         *object,
                            guint            prop_id,
                            GValue          *value,
                            GParamSpec      *pspec);
static void quarantine     (MilterReplySignals *reply,
                            const gchar        *reason);

static void
milter_manager_test_server_class_init (MilterManagerTestServerClass *klass)
{
    GObjectClass *gobject_class;

    gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->dispose      = dispose;
    gobject_class->set_property = set_property;
    gobject_class->get_property = get_property;

    g_type_class_add_private(gobject_class,
                             sizeof(MilterManagerTestServerPrivate));
}

static void
reply_init (MilterReplySignalsClass *reply)
{
    reply_parent = g_type_interface_peek_parent(reply);

    reply->quarantine = quarantine;
}

static void
milter_manager_test_server_init (MilterManagerTestServer *milter)
{
    MilterManagerTestServerPrivate *priv;

    priv = MILTER_MANAGER_TEST_SERVER_GET_PRIVATE(milter);

    priv->quarantine_reason = NULL;
}

static void
dispose (GObject *object)
{
    MilterManagerTestServerPrivate *priv;

    priv = MILTER_MANAGER_TEST_SERVER_GET_PRIVATE(object);

    if (priv->quarantine_reason) {
        g_free(priv->quarantine_reason);
        priv->quarantine_reason = NULL;
    }

    G_OBJECT_CLASS(milter_manager_test_server_parent_class)->dispose(object);
}

static void
set_property (GObject      *object,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
    MilterManagerTestServerPrivate *priv;

    priv = MILTER_MANAGER_TEST_SERVER_GET_PRIVATE(object);
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
    MilterManagerTestServerPrivate *priv;

    priv = MILTER_MANAGER_TEST_SERVER_GET_PRIVATE(object);
    switch (prop_id) {
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

MilterManagerTestServer *
milter_manager_test_server_new (void)
{
    return  g_object_new(MILTER_TYPE_MANAGER_TEST_SERVER,
                         NULL);
}

static void
quarantine (MilterReplySignals *reply, const gchar *reason)
{
    MilterManagerTestServerPrivate *priv;

    priv = MILTER_MANAGER_TEST_SERVER_GET_PRIVATE(reply);

    if (priv->quarantine_reason)
        g_free(priv->quarantine_reason);

    priv->quarantine_reason = g_strdup(reason);
}

static gboolean
cb_timeout_waiting (gpointer data)
{
    gboolean *waiting = data;

    *waiting = FALSE;
    return FALSE;
}

const gchar *
milter_manager_test_server_get_quarantine_reason (MilterManagerTestServer *server)
{
    return MILTER_MANAGER_TEST_SERVER_GET_PRIVATE(server)->quarantine_reason;
}

void
milter_manager_test_server_wait_signal (MilterManagerTestServer *server)
{
    gboolean timeout_waiting = TRUE;
    guint timeout_waiting_id;
    MilterManagerTestServerPrivate *priv;

    priv = MILTER_MANAGER_TEST_SERVER_GET_PRIVATE(server);

    timeout_waiting_id = g_timeout_add_seconds(1, cb_timeout_waiting,
                                               &timeout_waiting);
    while (timeout_waiting && !priv->quarantine_reason) {
        g_main_context_iteration(NULL, TRUE);
    }
    g_source_remove(timeout_waiting_id);

    cut_assert_true(timeout_waiting, "timeout");
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
