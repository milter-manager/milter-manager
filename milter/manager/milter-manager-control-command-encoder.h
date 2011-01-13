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

#ifndef __MILTER_MANAGER_CONTROL_COMMAND_ENCODER_H__
#define __MILTER_MANAGER_CONTROL_COMMAND_ENCODER_H__

#include <glib-object.h>

#include <milter/core.h>
#include <milter/manager/milter-manager-control-protocol.h>

G_BEGIN_DECLS

#define MILTER_TYPE_MANAGER_CONTROL_COMMAND_ENCODER            (milter_manager_control_command_encoder_get_type())
#define MILTER_MANAGER_CONTROL_COMMAND_ENCODER(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), MILTER_TYPE_MANAGER_CONTROL_COMMAND_ENCODER, MilterManagerControlCommandEncoder))
#define MILTER_MANAGER_CONTROL_COMMAND_ENCODER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), MILTER_TYPE_MANAGER_CONTROL_COMMAND_ENCODER, MilterManagerControlCommandEncoderClass))
#define MILTER_IS_MANAGER_CONTROL_COMMAND_ENCODER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), MILTER_TYPE_MANAGER_CONTROL_COMMAND_ENCODER))
#define MILTER_IS_MANAGER_CONTROL_COMMAND_ENCODER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), MILTER_TYPE_MANAGER_CONTROL_COMMAND_ENCODER))
#define MILTER_MANAGER_CONTROL_COMMAND_ENCODER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), MILTER_TYPE_MANAGER_CONTROL_COMMAND_ENCODER, MilterManagerControlCommandEncoderClass))

typedef struct _MilterManagerControlCommandEncoder         MilterManagerControlCommandEncoder;
typedef struct _MilterManagerControlCommandEncoderClass    MilterManagerControlCommandEncoderClass;

struct _MilterManagerControlCommandEncoder
{
    MilterEncoder object;
};

struct _MilterManagerControlCommandEncoderClass
{
    MilterEncoderClass parent_class;
};

GType            milter_manager_control_command_encoder_get_type       (void) G_GNUC_CONST;

MilterEncoder   *milter_manager_control_command_encoder_new            (void);

void             milter_manager_control_command_encoder_encode_set_configuration
                                            (MilterManagerControlCommandEncoder *encoder,
                                             const gchar  **packet,
                                             gsize         *packet_size,
                                             const gchar   *configuration,
                                             gsize          configuration_size);
void             milter_manager_control_command_encoder_encode_reload
                                            (MilterManagerControlCommandEncoder *encoder,
                                             const gchar  **packet,
                                             gsize         *packet_size);
void             milter_manager_control_command_encoder_encode_get_status
                                            (MilterManagerControlCommandEncoder *encoder,
                                             const gchar  **packet,
                                             gsize         *packet_size);

G_END_DECLS

#endif /* __MILTER_MANAGER_CONTROL_COMMAND_ENCODER_H__ */

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
