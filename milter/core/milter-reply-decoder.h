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

#ifndef __MILTER_REPLY_DECODER_H__
#define __MILTER_REPLY_DECODER_H__

#include <sys/types.h>
#include <sys/socket.h>

#include <glib-object.h>

#include <milter/core/milter-decoder.h>
#include <milter/core/milter-reply-signals.h>

G_BEGIN_DECLS

#define MILTER_REPLY_DECODER_ERROR           (milter_reply_decoder_error_quark())

#define MILTER_TYPE_REPLY_DECODER            (milter_reply_decoder_get_type())
#define MILTER_REPLY_DECODER(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), MILTER_TYPE_REPLY_DECODER, MilterReplyDecoder))
#define MILTER_REPLY_DECODER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), MILTER_TYPE_REPLY_DECODER, MilterReplyDecoderClass))
#define MILTER_IS_REPLY_DECODER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), MILTER_TYPE_REPLY_DECODER))
#define MILTER_IS_REPLY_DECODER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), MILTER_TYPE_REPLY_DECODER))
#define MILTER_REPLY_DECODER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), MILTER_TYPE_REPLY_DECODER, MilterReplyDecoderClass))

typedef enum
{
    MILTER_REPLY_DECODER_ERROR_UNEXPECTED_MACRO_STAGE
} MilterReplyDecoderError;

typedef struct _MilterReplyDecoder         MilterReplyDecoder;
typedef struct _MilterReplyDecoderClass    MilterReplyDecoderClass;

struct _MilterReplyDecoder
{
    MilterDecoder object;
};

struct _MilterReplyDecoderClass
{
    MilterDecoderClass parent_class;
};

GQuark         milter_reply_decoder_error_quark (void);

GType          milter_reply_decoder_get_type          (void) G_GNUC_CONST;

MilterDecoder *milter_reply_decoder_new               (void);

G_END_DECLS

#endif /* __MILTER_REPLY_DECODER_H__ */

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
