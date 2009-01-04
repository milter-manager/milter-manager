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

#ifndef __MILTER_COMMAND_DECODER_H__
#define __MILTER_COMMAND_DECODER_H__

#include <sys/types.h>
#include <sys/socket.h>

#include <glib-object.h>

#include <milter/core/milter-protocol.h>
#include <milter/core/milter-option.h>
#include <milter/core/milter-decoder.h>

G_BEGIN_DECLS

#define MILTER_COMMAND_DECODER_ERROR           (milter_command_decoder_error_quark())

#define MILTER_TYPE_COMMAND_DECODER            (milter_command_decoder_get_type())
#define MILTER_COMMAND_DECODER(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), MILTER_TYPE_COMMAND_DECODER, MilterCommandDecoder))
#define MILTER_COMMAND_DECODER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), MILTER_TYPE_COMMAND_DECODER, MilterCommandDecoderClass))
#define MILTER_IS_COMMAND_DECODER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), MILTER_TYPE_COMMAND_DECODER))
#define MILTER_IS_COMMAND_DECODER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), MILTER_TYPE_COMMAND_DECODER))
#define MILTER_COMMAND_DECODER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), MILTER_TYPE_COMMAND_DECODER, MilterCommandDecoderClass))

typedef enum
{
    MILTER_COMMAND_DECODER_ERROR_UNKNOWN_MACRO_CONTEXT,
    MILTER_COMMAND_DECODER_ERROR_INVALID_FORMAT,
    MILTER_COMMAND_DECODER_ERROR_UNKNOWN_SOCKET_FAMILY
} MilterCommandDecoderError;

typedef struct _MilterCommandDecoder         MilterCommandDecoder;
typedef struct _MilterCommandDecoderClass    MilterCommandDecoderClass;

struct _MilterCommandDecoder
{
    MilterDecoder object;
};

struct _MilterCommandDecoderClass
{
    MilterDecoderClass parent_class;

    void (*negotiate)           (MilterCommandDecoder *decoder,
                                 MilterOption  *option);
    void (*define_macro)        (MilterCommandDecoder *decoder,
                                 MilterCommand context,
                                 GHashTable *macros);
    void (*connect)             (MilterCommandDecoder *decoder,
                                 const gchar *host_name,
                                 const struct sockaddr *address,
                                 socklen_t address_length);
    void (*helo)                (MilterCommandDecoder *decoder,
                                 const gchar *fqdn);
    void (*envelope_from)       (MilterCommandDecoder *decoder,
                                 const gchar *from);
    void (*envelope_recipient)  (MilterCommandDecoder *decoder,
                                 const gchar *recipient);
    void (*data)                (MilterCommandDecoder *decoder);
    void (*header)              (MilterCommandDecoder *decoder,
                                 const gchar *name,
                                 const gchar *value);
    void (*end_of_header)       (MilterCommandDecoder *decoder);
    void (*body)                (MilterCommandDecoder *decoder,
                                 const gchar *chunk,
                                 gsize size);
    void (*end_of_message)      (MilterCommandDecoder *decoder,
                                 const gchar *chunk,
                                 gsize size);
    void (*abort)               (MilterCommandDecoder *decoder);
    void (*quit)                (MilterCommandDecoder *decoder);
    void (*unknown)             (MilterCommandDecoder *decoder,
                                 const gchar *command);
};

GQuark         milter_command_decoder_error_quark (void);

GType          milter_command_decoder_get_type    (void) G_GNUC_CONST;

MilterDecoder *milter_command_decoder_new         (void);

G_END_DECLS

#endif /* __MILTER_COMMAND_DECODER_H__ */

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
