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

#ifndef __MILTER_REPLY_SIGNALS_H__
#define __MILTER_REPLY_SIGNALS_H__

#include <glib-object.h>

#include <milter/core/milter-protocol.h>
#include <milter/core/milter-option.h>
#include <milter/core/milter-macros-requests.h>

G_BEGIN_DECLS

#define MILTER_TYPE_REPLY_SIGNALS             (milter_reply_signals_get_type ())
#define MILTER_REPLY_SIGNALS(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), MILTER_TYPE_REPLY_SIGNALS, MilterReplySignals))
#define MILTER_REPLY_SIGNALS_CLASS(vtable)    (G_TYPE_CHECK_CLASS_CAST ((vtable), MILTER_TYPE_REPLY_SIGNALS, MilterReplySignalsClass))
#define MILTER_IS_REPLY_SIGNALS(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MILTER_TYPE_REPLY_SIGNALS))
#define MILTER_IS_REPLY_SIGNALS_CLASS(vtable) (G_TYPE_CHECK_CLASS_TYPE ((vtable), MILTER_TYPE_REPLY_SIGNALS))
#define MILTER_REPLY_SIGNALS_GET_CLASS(inst)  (G_TYPE_INSTANCE_GET_INTERFACE ((inst), MILTER_TYPE_REPLY_SIGNALS, MilterReplySignalsClass))

typedef struct _MilterReplySignals         MilterReplySignals;
typedef struct _MilterReplySignalsClass    MilterReplySignalsClass;

struct _MilterReplySignalsClass
{
    GTypeInterface base_iface;

    void (*negotiate_reply)     (MilterReplySignals *reply,
                                 MilterOption       *option,
                                 MilterMacrosRequests *macros_requests);
    void (*_continue)           (MilterReplySignals *reply);
    void (*reply_code)          (MilterReplySignals *reply,
                                 guint                code,
                                 const gchar         *extended_code,
                                 const gchar         *message);
    void (*temporary_failure)   (MilterReplySignals *reply);
    void (*reject)              (MilterReplySignals *reply);
    void (*accept)              (MilterReplySignals *reply);
    void (*discard)             (MilterReplySignals *reply);
    void (*add_header)          (MilterReplySignals *reply,
                                 const gchar        *name,
                                 const gchar        *value);
    void (*insert_header)       (MilterReplySignals *reply,
                                 guint32             index,
                                 const gchar        *name,
                                 const gchar        *value);
    void (*change_header)       (MilterReplySignals *reply,
                                 const gchar        *name,
                                 guint32             index,
                                 const gchar        *value);
    void (*delete_header)       (MilterReplySignals *reply,
                                 const gchar        *name,
                                 guint32             index);
    void (*change_from)         (MilterReplySignals *reply,
                                 const gchar        *from,
                                 const gchar        *parameters);
    void (*add_recipient)       (MilterReplySignals *reply,
                                 const gchar        *recipient,
                                 const gchar        *parameters);
    void (*delete_recipient)    (MilterReplySignals *reply,
                                 const gchar        *recipient);
    void (*replace_body)        (MilterReplySignals *reply,
                                 const gchar        *body,
                                 gsize               body_size);
    void (*progress)            (MilterReplySignals *reply);
    void (*quarantine)          (MilterReplySignals *reply,
                                 const gchar        *reason);
    void (*connection_failure)  (MilterReplySignals *reply);
    void (*shutdown)            (MilterReplySignals *reply);
    void (*skip)                (MilterReplySignals *reply);

    void (*abort)               (MilterReplySignals *reply);
};

GType    milter_reply_signals_get_type          (void) G_GNUC_CONST;

G_END_DECLS

#endif /* __MILTER_REPLY_SIGNALS_H__ */

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
