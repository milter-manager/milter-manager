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

#ifndef __MILTER_MANAGER_UMBILICAL_COMMAND_DECODER_H__
#define __MILTER_MANAGER_UMBILICAL_COMMAND_DECODER_H__

#include <glib-object.h>

#include <milter/core.h>
#include <milter/manager/milter-manager-umbilical-protocol.h>

G_BEGIN_DECLS

#define MILTER_MANAGER_UMBILICAL_COMMAND_DECODER_ERROR           (milter_manager_umbilical_command_decoder_error_quark())

#define MILTER_TYPE_MANAGER_UMBILICAL_COMMAND_DECODER            (milter_manager_umbilical_command_decoder_get_type())
#define MILTER_MANAGER_UMBILICAL_COMMAND_DECODER(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), MILTER_TYPE_MANAGER_CONTROL_COMMAND_DECODER, MilterManagerUmbilicalCommandDecoder))
#define MILTER_MANAGER_UMBILICAL_COMMAND_DECODER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), MILTER_TYPE_MANAGER_CONTROL_COMMAND_DECODER, MilterManagerUmbilicalCommandDecoderClass))
#define MILTER_MANAGER_IS_UMBILICAL_COMMAND_DECODER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), MILTER_TYPE_MANAGER_CONTROL_COMMAND_DECODER))
#define MILTER_MANAGER_IS_UMBILICAL_COMMAND_DECODER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), MILTER_TYPE_MANAGER_CONTROL_COMMAND_DECODER))
#define MILTER_MANAGER_UMBILICAL_COMMAND_DECODER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), MILTER_TYPE_MANAGER_CONTROL_COMMAND_DECODER, MilterManagerUmbilicalCommandDecoderClass))

typedef enum
{
    MILTER_MANAGER_UMBILICAL_COMMAND_DECODER_ERROR_UNEXPECTED_COMMAND
} MilterManagerUmbilicalCommandDecoderError;

typedef struct _MilterManagerUmbilicalCommandDecoder         MilterManagerUmbilicalCommandDecoder;
typedef struct _MilterManagerUmbilicalCommandDecoderClass    MilterManagerUmbilicalCommandDecoderClass;

struct _MilterManagerUmbilicalCommandDecoder
{
    MilterDecoder object;
};

struct _MilterManagerUmbilicalCommandDecoderClass
{
    MilterDecoderClass parent_class;

    void (*spawn_child)    (MilterManagerUmbilicalCommandDecoder *decoder,
                            const gchar *command_line,
                            const gchar *user_name);
};

GQuark         milter_manager_umbilical_command_decoder_error_quark (void);
GType          milter_manager_umbilical_command_decoder_get_type    (void) G_GNUC_CONST;

MilterDecoder *milter_manager_umbilical_command_decoder_new         (void);

G_END_DECLS

#endif /* __MILTER_MANAGER_UMBILICAL_COMMAND_DECODER_H__ */

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
