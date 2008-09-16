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

#include <milter-core/milter-parser.h>
#include "milter-client.h"

#define MILTER_CLIENT_HANDLER_GET_PRIVATE(obj)                          \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj),                                 \
                                 MILTER_CLIENT_TYPE_HANDLER,            \
                                 MilterClientHandlerPrivate))

typedef struct _MilterClientHandlerPrivate	MilterClientHandlerPrivate;
struct _MilterClientHandlerPrivate
{
    MilterParser *parser;
};

enum
{
    SOME_SIGNAL,
    LAST_SIGNAL
};

static gint signals[LAST_SIGNAL] = {0};

G_DEFINE_TYPE(MilterClientHandler, milter_client_handler, G_TYPE_OBJECT);

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
milter_client_handler_class_init (MilterClientHandlerClass *klass)
{
    GObjectClass *gobject_class;

    gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->dispose      = dispose;
    gobject_class->set_property = set_property;
    gobject_class->get_property = get_property;

    g_type_class_add_private(gobject_class, sizeof(MilterClientHandlerPrivate));
}

static void
milter_client_handler_init (MilterClientHandler *handler)
{
    MilterClientHandlerPrivate *priv;

    priv = MILTER_CLIENT_HANDLER_GET_PRIVATE(handler);
    priv->parser = NULL;
}

static void
dispose (GObject *object)
{
    MilterClientHandlerPrivate *priv;

    priv = MILTER_CLIENT_HANDLER_GET_PRIVATE(object);
    if (priv->parser) {
        g_object_unref(priv->parser);
        priv->parser = NULL;
    }

    G_OBJECT_CLASS(milter_client_handler_parent_class)->dispose(object);
}

static void
set_property (GObject      *object,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
    MilterClientHandlerPrivate *priv;

    priv = MILTER_CLIENT_HANDLER_GET_PRIVATE(object);
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
    MilterClientHandlerPrivate *priv;

    priv = MILTER_CLIENT_HANDLER_GET_PRIVATE(object);
    switch (prop_id) {
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

MilterClientHandler *
milter_client_handler_new (void)
{
    return g_object_new(MILTER_CLIENT_TYPE_HANDLER,
                        NULL);
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
