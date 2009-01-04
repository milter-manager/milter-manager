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
 *  along with this library.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef __MILTER_MANAGER_CONTROL_COMMAND_DECODER_H__
#define __MILTER_MANAGER_CONTROL_COMMAND_DECODER_H__

#include <glib-object.h>

#include <milter/core.h>
#include <milter/manager/milter-manager-control-protocol.h>

G_BEGIN_DECLS

#define MILTER_MANAGER_CONTROL_COMMAND_DECODER_ERROR           (milter_manager_control_command_decoder_error_quark())

#define MILTER_TYPE_MANAGER_CONTROL_COMMAND_DECODER            (milter_manager_control_command_decoder_get_type())
#define MILTER_MANAGER_CONTROL_COMMAND_DECODER(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), MILTER_TYPE_MANAGER_CONTROL_COMMAND_DECODER, MilterManagerControlCommandDecoder))
#define MILTER_MANAGER_CONTROL_COMMAND_DECODER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), MILTER_TYPE_MANAGER_CONTROL_COMMAND_DECODER, MilterManagerControlCommandDecoderClass))
#define MILTER_MANAGER_IS_CONTROL_COMMAND_DECODER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), MILTER_TYPE_MANAGER_CONTROL_COMMAND_DECODER))
#define MILTER_MANAGER_IS_CONTROL_COMMAND_DECODER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), MILTER_TYPE_MANAGER_CONTROL_COMMAND_DECODER))
#define MILTER_MANAGER_CONTROL_COMMAND_DECODER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), MILTER_TYPE_MANAGER_CONTROL_COMMAND_DECODER, MilterManagerControlCommandDecoderClass))

typedef enum
{
    MILTER_MANAGER_CONTROL_COMMAND_DECODER_ERROR_UNEXPECTED_COMMAND
} MilterManagerControlCommandDecoderError;

typedef struct _MilterManagerControlCommandDecoder         MilterManagerControlCommandDecoder;
typedef struct _MilterManagerControlCommandDecoderClass    MilterManagerControlCommandDecoderClass;

struct _MilterManagerControlCommandDecoder
{
    MilterDecoder object;
};

struct _MilterManagerControlCommandDecoderClass
{
    MilterDecoderClass parent_class;

    void (*set_configuration)    (MilterManagerControlCommandDecoder *decoder,
                                  const gchar *configuration,
                                  gsize size);
    void (*get_configuration)    (MilterManagerControlCommandDecoder *decoder);
    void (*reload)               (MilterManagerControlCommandDecoder *decoder);
    void (*stop_child)           (MilterManagerControlCommandDecoder *decoder,
                                  const gchar *name);
    void (*get_status)           (MilterManagerControlCommandDecoder *decoder);
};

GQuark         milter_manager_control_command_decoder_error_quark (void);
GType          milter_manager_control_command_decoder_get_type    (void) G_GNUC_CONST;

MilterDecoder *milter_manager_control_command_decoder_new         (void);

G_END_DECLS

#endif /* __MILTER_MANAGER_CONTROL_COMMAND_DECODER_H__ */

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
