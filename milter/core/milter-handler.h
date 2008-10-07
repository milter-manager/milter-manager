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

#ifndef __MILTER_HANDLER_H__
#define __MILTER_HANDLER_H__

#include <glib-object.h>

#include <milter/core/milter-protocol.h>
#include <milter/core/milter-option.h>
#include <milter/core/milter-writer.h>
#include <milter/core/milter-encoder.h>
#include <milter/core/milter-decoder.h>

G_BEGIN_DECLS

#define MILTER_HANDLER_ERROR           (milter_handler_error_quark())

#define MILTER_TYPE_HANDLER            (milter_handler_get_type())
#define MILTER_HANDLER(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), MILTER_TYPE_HANDLER, MilterHandler))
#define MILTER_HANDLER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), MILTER_TYPE_HANDLER, MilterHandlerClass))
#define MILTER_IS_HANDLER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), MILTER_TYPE_HANDLER))
#define MILTER_IS_HANDLER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), MILTER_TYPE_HANDLER))
#define MILTER_HANDLER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), MILTER_TYPE_HANDLER, MilterHandlerClass))

typedef enum
{
    MILTER_HANDLER_ERROR_INVALID_CODE
} MilterHandlerError;

typedef struct _MilterHandler         MilterHandler;
typedef struct _MilterHandlerClass    MilterHandlerClass;

struct _MilterHandler
{
    GObject object;
};

struct _MilterHandlerClass
{
    GObjectClass parent_class;

    gboolean     (*reply)              (MilterHandler *handler,
                                        MilterStatus   status);
    MilterStatus (*negotiate)          (MilterHandler *handler,
                                        MilterOption  *option);
    MilterStatus (*negotiate_response) (MilterHandler *handler,
                                        MilterOption  *option,
                                        MilterStatus   status);
    MilterStatus (*connect)            (MilterHandler *handler,
                                        const gchar   *host_name,
                                        struct sockaddr *address,
                                        socklen_t      address_length);
    MilterStatus (*connect_response)   (MilterHandler *handler,
                                        const gchar   *host_name,
                                        struct sockaddr *address,
                                        socklen_t      address_length,
                                        MilterStatus   status);
    MilterStatus (*helo)               (MilterHandler *handler,
                                        const gchar   *fqdn);
    MilterStatus (*helo_response)      (MilterHandler *handler,
                                        const gchar   *fqdn,
                                        MilterStatus   status);
    MilterStatus (*envelope_from)      (MilterHandler *handler,
                                        const gchar   *from);
    MilterStatus (*envelope_from_response)
                                       (MilterHandler *handler,
                                        const gchar   *from,
                                        MilterStatus   status);
    MilterStatus (*envelope_receipt)   (MilterHandler *handler,
                                        const gchar   *receipt);
    MilterStatus (*envelope_receipt_response)
                                       (MilterHandler *handler,
                                        const gchar   *receipt,
                                        MilterStatus   status);
    MilterStatus (*data)               (MilterHandler *handler);
    MilterStatus (*data_response)      (MilterHandler *handler,
                                        MilterStatus   status);
    MilterStatus (*unknown)            (MilterHandler *handler,
                                        const gchar   *command);
    MilterStatus (*unknown_response)   (MilterHandler *handler,
                                        const gchar   *command,
                                        MilterStatus   status);
    MilterStatus (*header)             (MilterHandler *handler,
                                        const gchar   *name,
                                        const gchar   *value);
    MilterStatus (*header_response)    (MilterHandler *handler,
                                        const gchar   *name,
                                        const gchar   *value,
                                        MilterStatus   status);
    MilterStatus (*end_of_header)      (MilterHandler *handler);
    MilterStatus (*end_of_header_response)
                                       (MilterHandler *handler,
                                        MilterStatus   status);
    MilterStatus (*body)               (MilterHandler *handler,
                                        const guchar  *chunk,
                                        gsize          size);
    MilterStatus (*body_response)      (MilterHandler *handler,
                                        const guchar  *chunk,
                                        gsize          size,
                                        MilterStatus   status);
    MilterStatus (*end_of_message)     (MilterHandler *handler);
    MilterStatus (*end_of_message_response)
                                       (MilterHandler *handler,
                                        MilterStatus   status);
    MilterStatus (*close)              (MilterHandler *handler);
    MilterStatus (*close_response)     (MilterHandler *handler,
                                        MilterStatus   status);
    MilterStatus (*abort)              (MilterHandler *handler);
    MilterStatus (*abort_response)     (MilterHandler *handler,
                                        MilterStatus   status);
};

GQuark               milter_handler_error_quark       (void);

GType                milter_handler_get_type          (void) G_GNUC_CONST;

MilterEncoder       *milter_handler_get_encoder       (MilterHandler *handler);
MilterDecoder       *milter_handler_get_decoder       (MilterHandler *handler);

gboolean             milter_handler_feed              (MilterHandler *handler,
                                                       const gchar *chunk,
                                                       gsize size,
                                                       GError **error);
void                 milter_handler_set_writer        (MilterHandler *handler,
                                                       MilterWriter *writer);
gboolean             milter_handler_write_packet      (MilterHandler *handler,
                                                       const char *packet,
                                                       gsize packet_size,
                                                       GError **error);
GHashTable          *milter_handler_get_macros        (MilterHandler *handler);
const gchar         *milter_handler_get_macro         (MilterHandler *handler,
                                                       const gchar   *name);
void                 milter_handler_clear_macros      (MilterHandler *handler,
                                                       MilterCommand macro_context);
gboolean             milter_handler_reply             (MilterHandler *handler,
                                                       MilterStatus   status);

G_END_DECLS

#endif /* __MILTER_HANDLER_H__ */

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
