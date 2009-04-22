/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Copyright (C) 2008-2009  Kouhei Sutou <kou@clear-code.com>
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

#ifndef __MILTER_DECODER_H__
#define __MILTER_DECODER_H__

#include <sys/types.h>
#include <sys/socket.h>

#include <glib-object.h>

#include <milter/core/milter-protocol.h>
#include <milter/core/milter-option.h>

G_BEGIN_DECLS

#define MILTER_DECODER_ERROR           (milter_decoder_error_quark())

#define MILTER_TYPE_DECODER            (milter_decoder_get_type())
#define MILTER_DECODER(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), MILTER_TYPE_DECODER, MilterDecoder))
#define MILTER_DECODER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), MILTER_TYPE_DECODER, MilterDecoderClass))
#define MILTER_IS_DECODER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), MILTER_TYPE_DECODER))
#define MILTER_IS_DECODER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), MILTER_TYPE_DECODER))
#define MILTER_DECODER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), MILTER_TYPE_DECODER, MilterDecoderClass))

typedef enum
{
    MILTER_DECODER_ERROR_SHORT_COMMAND_LENGTH,
    MILTER_DECODER_ERROR_LONG_COMMAND_LENGTH,
    MILTER_DECODER_ERROR_UNEXPECTED_END,
    MILTER_DECODER_ERROR_UNEXPECTED_COMMAND,
    MILTER_DECODER_ERROR_MISSING_NULL
} MilterDecoderError;

typedef enum {
    MILTER_DECODER_COMPARE_AT_LEAST,
    MILTER_DECODER_COMPARE_EXACT
} MilterDecoderCompareType;

typedef struct _MilterDecoder         MilterDecoder;
typedef struct _MilterDecoderClass    MilterDecoderClass;

struct _MilterDecoder
{
    GObject object;
};

struct _MilterDecoderClass
{
    GObjectClass parent_class;

    gboolean (*decode)  (MilterDecoder *decoder,
                         GError       **error);
};

GQuark           milter_decoder_error_quark       (void);

GType            milter_decoder_get_type          (void) G_GNUC_CONST;

gboolean         milter_decoder_decode            (MilterDecoder   *decoder,
                                                   const gchar     *chunk,
                                                   gsize            size,
                                                   GError         **error);
gboolean         milter_decoder_end_decode        (MilterDecoder   *decoder,
                                                   GError         **error);
const gchar     *milter_decoder_get_buffer        (MilterDecoder   *decoder);
gint32           milter_decoder_get_command_length(MilterDecoder   *decoder);

/* utility functions */
gboolean         milter_decoder_check_command_length (const gchar *buffer,
                                                      gint length,
                                                      gint expected_length,
                                                      MilterDecoderCompareType compare_type,
                                                      GError **error,
                                                      const gchar *target);
gint             milter_decoder_decode_null_terminated_value
                                                     (const gchar *buffer,
                                                      gint length,
                                                      GError **error,
                                                      const gchar *message);
gboolean         milter_decoder_decode_header_content(const gchar *buffer,
                                                      gint length,
                                                      const gchar **name,
                                                      const gchar **value,
                                                      GError **error);
MilterOption    *milter_decoder_decode_negotiate     (const gchar *buffer,
                                                      gint length,
                                                      gint *processed_length,
                                                      GError **error);

guint            milter_decoder_get_tag              (MilterDecoder *decoder);
void             milter_decoder_set_tag              (MilterDecoder *decoder,
                                                      guint          tag);

G_END_DECLS

#endif /* __MILTER_DECODER_H__ */

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
