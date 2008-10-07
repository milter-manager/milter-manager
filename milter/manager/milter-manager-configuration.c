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

#include "milter-manager-configuration.h"

#define MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(obj)                   \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj),                                 \
                                 MILTER_MANAGER_TYPE_CONFIGURATION,     \
                                 MilterManagerConfigurationPrivate))

typedef struct _MilterManagerConfigurationPrivate MilterManagerConfigurationPrivate;
struct _MilterManagerConfigurationPrivate
{
    gpointer private_data;
};

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

    gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->dispose      = dispose;
    gobject_class->set_property = set_property;
    gobject_class->get_property = get_property;

    g_type_class_add_private(gobject_class,
                             sizeof(MilterManagerConfigurationPrivate));
}

static void
milter_manager_configuration_init (MilterManagerConfiguration *configuration)
{
    MilterManagerConfigurationPrivate *priv;

    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(configuration);
    priv->private_data = NULL;
}

static void
dispose (GObject *object)
{
    MilterManagerConfigurationPrivate *priv;

    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(object);

    if (priv->private_data) {
        priv->private_data = NULL;
    }

    G_OBJECT_CLASS(milter_manager_configuration_parent_class)->dispose(object);
}

static void
set_property (GObject      *object,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
    MilterManagerConfigurationPrivate *priv;

    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(object);
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
    MilterManagerConfigurationPrivate *priv;

    priv = MILTER_MANAGER_CONFIGURATION_GET_PRIVATE(object);
    switch (prop_id) {
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

MilterManagerConfiguration *
milter_manager_configuration_new (void)
{
    return g_object_new(MILTER_MANAGER_TYPE_CONFIGURATION,
                        NULL);
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
