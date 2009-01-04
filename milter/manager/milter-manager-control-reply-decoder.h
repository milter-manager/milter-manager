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

#ifndef __MILTER_MANAGER_CONTROL_REPLY_DECODER_H__
#define __MILTER_MANAGER_CONTROL_REPLY_DECODER_H__

#include <glib-object.h>

#include <milter/core.h>
#include <milter/manager/milter-manager-control-protocol.h>
#include <milter/manager/milter-manager-reply-decoder.h>

G_BEGIN_DECLS

#define MILTER_MANAGER_CONTROL_REPLY_DECODER_ERROR           (milter_manager_control_reply_decoder_error_quark())

#define MILTER_TYPE_MANAGER_CONTROL_REPLY_DECODER            (milter_manager_control_reply_decoder_get_type())
#define MILTER_MANAGER_CONTROL_REPLY_DECODER(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), MILTER_TYPE_MANAGER_CONTROL_REPLY_DECODER, MilterManagerControlReplyDecoder))
#define MILTER_MANAGER_CONTROL_REPLY_DECODER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), MILTER_TYPE_MANAGER_CONTROL_REPLY_DECODER, MilterManagerControlReplyDecoderClass))
#define MILTER_MANAGER_IS_CONTROL_REPLY_DECODER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), MILTER_TYPE_MANAGER_CONTROL_REPLY_DECODER))
#define MILTER_MANAGER_IS_CONTROL_REPLY_DECODER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), MILTER_TYPE_MANAGER_CONTROL_REPLY_DECODER))
#define MILTER_MANAGER_CONTROL_REPLY_DECODER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), MILTER_TYPE_MANAGER_CONTROL_REPLY_DECODER, MilterManagerControlReplyDecoderClass))

typedef enum
{
    MILTER_MANAGER_CONTROL_REPLY_DECODER_ERROR_UNEXPECTED_REPLY
} MilterManagerControlReplyDecoderError;

typedef struct _MilterManagerControlReplyDecoder         MilterManagerControlReplyDecoder;
typedef struct _MilterManagerControlReplyDecoderClass    MilterManagerControlReplyDecoderClass;

struct _MilterManagerControlReplyDecoder
{
    MilterManagerReplyDecoder object;
};

struct _MilterManagerControlReplyDecoderClass
{
    MilterManagerReplyDecoderClass parent_class;

    void (*configuration)        (MilterManagerControlReplyDecoder *decoder,
                                  const gchar *configuration,
                                  gsize configuration_size);
};

GQuark         milter_manager_control_reply_decoder_error_quark (void);
GType          milter_manager_control_reply_decoder_get_type    (void) G_GNUC_CONST;

MilterDecoder *milter_manager_control_reply_decoder_new         (void);

G_END_DECLS

#endif /* __MILTER_MANAGER_CONTROL_REPLY_DECODER_H__ */

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
