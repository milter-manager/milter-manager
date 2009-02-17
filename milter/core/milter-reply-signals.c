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

#ifdef HAVE_CONFIG_H
#  include "../../config.h"
#endif /* HAVE_CONFIG_H */

#include "milter-reply-signals.h"
#include "milter-marshalers.h"

enum
{
    NEGOTIATE_REPLY,
    CONTINUE,
    REPLY_CODE,
    TEMPORARY_FAILURE,
    REJECT,
    ACCEPT,
    DISCARD,
    ADD_HEADER,
    INSERT_HEADER,
    CHANGE_HEADER,
    DELETE_HEADER,
    CHANGE_FROM,
    ADD_RECIPIENT,
    DELETE_RECIPIENT,
    REPLACE_BODY,
    PROGRESS,
    QUARANTINE,
    CONNECTION_FAILURE,
    SHUTDOWN,
    SKIP,

    ABORT,

    LAST_SIGNAL
};

static gint signals[LAST_SIGNAL] = {0};

static void
base_init (gpointer klass)
{
    static gboolean initialized = FALSE;

    if (initialized) return;

    signals[NEGOTIATE_REPLY] =
        g_signal_new("negotiate-reply",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterReplySignalsClass, negotiate_reply),
                     NULL, NULL,
                     _milter_marshal_VOID__OBJECT_OBJECT,
                     G_TYPE_NONE, 2, MILTER_TYPE_OPTION, MILTER_TYPE_MACROS_REQUESTS);

    signals[CONTINUE] =
        g_signal_new("continue",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterReplySignalsClass, _continue),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__VOID,
                     G_TYPE_NONE, 0);

    signals[REPLY_CODE] =
        g_signal_new("reply-code",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterReplySignalsClass, reply_code),
                     NULL, NULL,
                     _milter_marshal_VOID__UINT_STRING_STRING,
                     G_TYPE_NONE, 3, G_TYPE_UINT, G_TYPE_STRING, G_TYPE_STRING);

    signals[TEMPORARY_FAILURE] =
        g_signal_new("temporary-failure",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterReplySignalsClass, temporary_failure),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__VOID,
                     G_TYPE_NONE, 0);

    signals[REJECT] =
        g_signal_new("reject",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterReplySignalsClass, reject),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__VOID,
                     G_TYPE_NONE, 0);

    signals[ACCEPT] =
        g_signal_new("accept",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterReplySignalsClass, accept),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__VOID,
                     G_TYPE_NONE, 0);

    signals[DISCARD] =
        g_signal_new("discard",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterReplySignalsClass, discard),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__VOID,
                     G_TYPE_NONE, 0);

    signals[ADD_HEADER] =
        g_signal_new("add-header",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterReplySignalsClass, add_header),
                     NULL, NULL,
                     _milter_marshal_VOID__STRING_STRING,
                     G_TYPE_NONE, 2, G_TYPE_STRING, G_TYPE_STRING);

    signals[INSERT_HEADER] =
        g_signal_new("insert-header",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterReplySignalsClass, insert_header),
                     NULL, NULL,
                     _milter_marshal_VOID__UINT_STRING_STRING,
                     G_TYPE_NONE, 3, G_TYPE_UINT, G_TYPE_STRING, G_TYPE_STRING);

    signals[CHANGE_HEADER] =
        g_signal_new("change-header",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterReplySignalsClass, change_header),
                     NULL, NULL,
                     _milter_marshal_VOID__STRING_UINT_STRING,
                     G_TYPE_NONE, 3, G_TYPE_STRING, G_TYPE_UINT, G_TYPE_STRING);

    signals[DELETE_HEADER] =
        g_signal_new("delete-header",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterReplySignalsClass, delete_header),
                     NULL, NULL,
                     _milter_marshal_VOID__STRING_UINT,
                     G_TYPE_NONE, 2, G_TYPE_STRING, G_TYPE_UINT);

    signals[CHANGE_FROM] =
        g_signal_new("change-from",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterReplySignalsClass, change_from),
                     NULL, NULL,
                     _milter_marshal_VOID__STRING_STRING,
                     G_TYPE_NONE, 2, G_TYPE_STRING, G_TYPE_STRING);

    signals[ADD_RECIPIENT] =
        g_signal_new("add-recipient",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterReplySignalsClass, add_recipient),
                     NULL, NULL,
                     _milter_marshal_VOID__STRING_STRING,
                     G_TYPE_NONE, 2, G_TYPE_STRING, G_TYPE_STRING);

    signals[DELETE_RECIPIENT] =
        g_signal_new("delete-recipient",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterReplySignalsClass, delete_recipient),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__STRING,
                     G_TYPE_NONE, 1, G_TYPE_STRING);

    signals[REPLACE_BODY] =
        g_signal_new("replace-body",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterReplySignalsClass, replace_body),
                     NULL, NULL,
#if GLIB_SIZEOF_SIZE_T == 8
                     _milter_marshal_VOID__STRING_UINT64,
                     G_TYPE_NONE, 2, G_TYPE_STRING, G_TYPE_UINT64
#else
                     _milter_marshal_VOID__STRING_UINT,
                     G_TYPE_NONE, 2, G_TYPE_STRING, G_TYPE_UINT
#endif
                    );

    signals[PROGRESS] =
        g_signal_new("progress",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterReplySignalsClass, progress),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__VOID,
                     G_TYPE_NONE, 0);

    signals[QUARANTINE] =
        g_signal_new("quarantine",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterReplySignalsClass, quarantine),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__STRING,
                     G_TYPE_NONE, 1, G_TYPE_STRING);

    signals[CONNECTION_FAILURE] =
        g_signal_new("connection-failure",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterReplySignalsClass, connection_failure),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__VOID,
                     G_TYPE_NONE, 0);

    signals[SHUTDOWN] =
        g_signal_new("shutdown",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterReplySignalsClass, shutdown),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__VOID,
                     G_TYPE_NONE, 0);

    signals[SKIP] =
        g_signal_new("skip",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterReplySignalsClass, skip),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__VOID,
                     G_TYPE_NONE, 0);


    signals[ABORT] =
        g_signal_new("abort",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MilterReplySignalsClass, abort),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__VOID,
                     G_TYPE_NONE, 0);


    initialized = TRUE;
}

GType
milter_reply_signals_get_type (void)
{
    static GType reply_signals_type = 0;

    if (!reply_signals_type) {
        const GTypeInfo reply_signals_info = {
            sizeof(MilterReplySignalsClass), /* class_size */
            base_init,          /* base_init */
            NULL,               /* base_finalize */
        };

        reply_signals_type = g_type_register_static(G_TYPE_INTERFACE,
                                                    "MilterReplySignals",
                                                    &reply_signals_info, 0);
    }

    return reply_signals_type;
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
