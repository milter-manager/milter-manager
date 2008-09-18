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

#ifndef __MILTER_DECODER_H__
#define __MILTER_DECODER_H__

#include <sys/types.h>
#include <sys/socket.h>

#include <glib-object.h>

#include <milter-core/milter-option.h>

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
    MILTER_DECODER_ERROR_UNKNOWN_MACRO_CONTEXT,
    MILTER_DECODER_ERROR_UNEXPECTED_END,
    MILTER_DECODER_ERROR_UNEXPECTED_COMMAND,
    MILTER_DECODER_ERROR_MISSING_NULL,
    MILTER_DECODER_ERROR_INVALID_FORMAT,
    MILTER_DECODER_ERROR_UNKNOWN_FAMILY
} MilterDecoderError;

typedef enum
{
    MILTER_CONTEXT_TYPE_CONNECT,
    MILTER_CONTEXT_TYPE_HELO,
    MILTER_CONTEXT_TYPE_MAIL,
    MILTER_CONTEXT_TYPE_RCPT,
    MILTER_CONTEXT_TYPE_HEADER,
    MILTER_CONTEXT_TYPE_END_OF_HEADER,
    MILTER_CONTEXT_TYPE_BODY,
    MILTER_CONTEXT_TYPE_END_OF_MESSAGE,
    MILTER_CONTEXT_TYPE_ABORT,
    MILTER_CONTEXT_TYPE_QUIT,
    MILTER_CONTEXT_TYPE_UNKNOWN
} MilterContextType;

typedef struct _MilterDecoder         MilterDecoder;
typedef struct _MilterDecoderClass    MilterDecoderClass;

struct _MilterDecoder
{
    GObject object;
};

struct _MilterDecoderClass
{
    GObjectClass parent_class;

    void (*option_negotiation)  (MilterDecoder *decoder,
                                 MilterOption *option);
    void (*define_macro)        (MilterDecoder *decoder,
                                 MilterContextType context,
                                 GHashTable *macros);
    void (*connect)             (MilterDecoder *decoder,
                                 const gchar *host_name,
                                 const struct sockaddr *address,
                                 socklen_t address_length);
    void (*helo)                (MilterDecoder *decoder,
                                 const gchar *fqdn);
    void (*mail)                (MilterDecoder *decoder,
                                 const gchar *from);
    void (*rcpt)                (MilterDecoder *decoder,
                                 const gchar *to);
    void (*header)              (MilterDecoder *decoder,
                                 const gchar *name,
                                 const gchar *value);
    void (*end_of_header)       (MilterDecoder *decoder);
    void (*body)                (MilterDecoder *decoder,
                                 const gchar *chunk,
                                 gsize length);
    void (*end_of_message)      (MilterDecoder *decoder);
    void (*abort)               (MilterDecoder *decoder);
    void (*quit)                (MilterDecoder *decoder);
    void (*unknown)             (MilterDecoder *decoder,
                                 const gchar *command);
};

GQuark           milter_decoder_error_quark       (void);

GType            milter_decoder_get_type          (void) G_GNUC_CONST;

MilterDecoder    *milter_decoder_new              (void);

gboolean         milter_decoder_decode            (MilterDecoder    *decoder,
                                                   const gchar     *text,
                                                   gsize            text_len,
                                                   GError         **error);
gboolean         milter_decoder_end_decode        (MilterDecoder    *decoder,
                                                   GError         **error);

G_END_DECLS

#endif /* __MILTER_DECODER_H__ */

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
