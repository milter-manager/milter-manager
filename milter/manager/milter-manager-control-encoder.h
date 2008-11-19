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

#ifndef __MILTER_MANAGER_CONTROL_ENCODER_H__
#define __MILTER_MANAGER_CONTROL_ENCODER_H__

#include <glib-object.h>

#include <milter/core.h>
#include <milter/manager/milter-manager-control-protocol.h>

G_BEGIN_DECLS

#define MILTER_TYPE_MANAGER_CONTROL_ENCODER            (milter_manager_control_encoder_get_type())
#define MILTER_MANAGER_CONTROL_ENCODER(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), MILTER_TYPE_MANAGER_CONTROL_ENCODER, MilterManagerControlEncoder))
#define MILTER_MANAGER_CONTROL_ENCODER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), MILTER_TYPE_MANAGER_CONTROL_ENCODER, MilterManagerControlEncoderClass))
#define MILTER_IS_MANAGER_CONTROL_ENCODER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), MILTER_TYPE_MANAGER_CONTROL_ENCODER))
#define MILTER_IS_MANAGER_CONTROL_ENCODER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), MILTER_TYPE_MANAGER_CONTROL_ENCODER))
#define MILTER_MANAGER_CONTROL_ENCODER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), MILTER_TYPE_MANAGER_CONTROL_ENCODER, MilterManagerControlEncoderClass))

typedef struct _MilterManagerControlEncoder         MilterManagerControlEncoder;
typedef struct _MilterManagerControlEncoderClass    MilterManagerControlEncoderClass;

struct _MilterManagerControlEncoder
{
    MilterEncoder object;
};

struct _MilterManagerControlEncoderClass
{
    MilterEncoderClass parent_class;
};

GType            milter_manager_control_encoder_get_type       (void) G_GNUC_CONST;

MilterEncoder   *milter_manager_control_encoder_new            (void);

void             milter_manager_control_encoder_encode_import_configuration
                                            (MilterManagerControlEncoder *encoder,
                                             gchar        **packet,
                                             gsize         *packet_size,
                                             const gchar   *configuration,
                                             gsize          configuration_size);

G_END_DECLS

#endif /* __MILTER_MANAGER_CONTROL_ENCODER_H__ */

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
