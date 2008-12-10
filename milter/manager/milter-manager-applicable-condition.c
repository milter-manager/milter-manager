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

#include "milter-manager-applicable-condition.h"
#include "milter-manager-enum-types.h"
#include <milter/core/milter-marshalers.h>

#define MILTER_MANAGER_APPLICABLE_CONDITION_GET_PRIVATE(obj)            \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj),                                 \
                                 MILTER_TYPE_MANAGER_APPLICABLE_CONDITION, \
                                 MilterManagerApplicableConditionPrivate))

typedef struct _MilterManagerApplicableConditionPrivate MilterManagerApplicableConditionPrivate;
struct _MilterManagerApplicableConditionPrivate
{
    gchar *name;
    gchar *description;
};

enum
{
    PROP_0,
    PROP_NAME,
    PROP_DESCRIPTION
};

enum
{
    ATTACH_TO,
    LAST_SIGNAL
};

static gint signals[LAST_SIGNAL] = {0};

G_DEFINE_TYPE(MilterManagerApplicableCondition,
              milter_manager_applicable_condition,
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
milter_manager_applicable_condition_class_init (MilterManagerApplicableConditionClass *klass)
{
    GObjectClass *gobject_class;
    GParamSpec *spec;

    gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->dispose      = dispose;
    gobject_class->set_property = set_property;
    gobject_class->get_property = get_property;

    spec = g_param_spec_string("name",
                               "Name",
                               "The name of the applicable condition",
                               NULL,
                               G_PARAM_READWRITE);
    g_object_class_install_property(gobject_class, PROP_NAME, spec);

    spec = g_param_spec_string("description",
                               "Description",
                               "The description of the applicable condition",
                               NULL,
                               G_PARAM_READWRITE);
    g_object_class_install_property(gobject_class, PROP_DESCRIPTION, spec);

    signals[ATTACH_TO] =
        g_signal_new("attach-to",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterManagerApplicableConditionClass,
                                     attach_to),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__OBJECT,
                     G_TYPE_NONE, 1, MILTER_TYPE_MANAGER_CHILD);

    g_type_class_add_private(gobject_class,
                             sizeof(MilterManagerApplicableConditionPrivate));
}

static void
milter_manager_applicable_condition_init (MilterManagerApplicableCondition *applicable_condition)
{
    MilterManagerApplicableConditionPrivate *priv;

    priv = MILTER_MANAGER_APPLICABLE_CONDITION_GET_PRIVATE(applicable_condition);
    priv->name = NULL;
    priv->description = NULL;
}

static void
dispose (GObject *object)
{
    MilterManagerApplicableConditionPrivate *priv;

    priv = MILTER_MANAGER_APPLICABLE_CONDITION_GET_PRIVATE(object);

    if (priv->name) {
        g_free(priv->name);
        priv->name = NULL;
    }

    if (priv->description) {
        g_free(priv->description);
        priv->description = NULL;
    }

    G_OBJECT_CLASS(milter_manager_applicable_condition_parent_class)->dispose(object);
}

static void
set_property (GObject      *object,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
    MilterManagerApplicableCondition *applicable_condition;
    MilterManagerApplicableConditionPrivate *priv;

    applicable_condition = MILTER_MANAGER_APPLICABLE_CONDITION(object);
    priv = MILTER_MANAGER_APPLICABLE_CONDITION_GET_PRIVATE(object);

    switch (prop_id) {
      case PROP_NAME:
        milter_manager_applicable_condition_set_name(applicable_condition,
                                                     g_value_get_string(value));
        break;
      case PROP_DESCRIPTION:
        milter_manager_applicable_condition_set_description(applicable_condition,
                                                            g_value_get_string(value));
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
    MilterManagerApplicableConditionPrivate *priv;

    priv = MILTER_MANAGER_APPLICABLE_CONDITION_GET_PRIVATE(object);
    switch (prop_id) {
      case PROP_NAME:
        g_value_set_string(value, priv->name);
        break;
      case PROP_DESCRIPTION:
        g_value_set_string(value, priv->description);
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

MilterManagerApplicableCondition *
milter_manager_applicable_condition_new (const gchar *name)
{
    return g_object_new(MILTER_TYPE_MANAGER_APPLICABLE_CONDITION,
                        "name", name,
                        NULL);
}

void
milter_manager_applicable_condition_set_name (MilterManagerApplicableCondition *condition,
                                              const gchar *name)
{
    MilterManagerApplicableConditionPrivate *priv;

    priv = MILTER_MANAGER_APPLICABLE_CONDITION_GET_PRIVATE(condition);
    if (priv->name)
        g_free(priv->name);
    priv->name = g_strdup(name);
}

const gchar *
milter_manager_applicable_condition_get_name (MilterManagerApplicableCondition *condition)
{
    return MILTER_MANAGER_APPLICABLE_CONDITION_GET_PRIVATE(condition)->name;
}

void
milter_manager_applicable_condition_set_description (MilterManagerApplicableCondition *condition,
                                                     const gchar *description)
{
    MilterManagerApplicableConditionPrivate *priv;

    priv = MILTER_MANAGER_APPLICABLE_CONDITION_GET_PRIVATE(condition);
    if (priv->description)
        g_free(priv->description);
    priv->description = g_strdup(description);
}

const gchar *
milter_manager_applicable_condition_get_description (MilterManagerApplicableCondition *condition)
{
    return MILTER_MANAGER_APPLICABLE_CONDITION_GET_PRIVATE(condition)->description;
}

void
milter_manager_applicable_condition_attach_to (MilterManagerApplicableCondition *condition,
                                               MilterManagerChild               *child)
{
    g_signal_emit(condition, signals[ATTACH_TO], 0, child);
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
