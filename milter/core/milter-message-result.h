/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Copyright (C) 2009  Kouhei Sutou <kou@clear-code.com>
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

#ifndef __MILTER_MESSAGE_RESULT_H__
#define __MILTER_MESSAGE_RESULT_H__

#include <glib-object.h>
#include <milter/core/milter-protocol.h>
#include <milter/core/milter-headers.h>

G_BEGIN_DECLS

#define MILTER_TYPE_MESSAGE_RESULT            (milter_message_result_get_type())
#define MILTER_MESSAGE_RESULT(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), MILTER_TYPE_MESSAGE_RESULT, MilterMessageResult))
#define MILTER_MESSAGE_RESULT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), MILTER_TYPE_MESSAGE_RESULT, MilterMessageResultClass))
#define MILTER_IS_MESSAGE_RESULT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), MILTER_TYPE_MESSAGE_RESULT))
#define MILTER_IS_MESSAGE_RESULT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), MILTER_TYPE_MESSAGE_RESULT))
#define MILTER_MESSAGE_RESULT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), MILTER_TYPE_MESSAGE_RESULT, MilterMessageResultClass))

typedef enum
{
    MILTER_STATE_INVALID,
    MILTER_STATE_START,
    MILTER_STATE_NEGOTIATE,
    MILTER_STATE_NEGOTIATE_REPLIED,
    MILTER_STATE_CONNECT,
    MILTER_STATE_CONNECT_REPLIED,
    MILTER_STATE_HELO,
    MILTER_STATE_HELO_REPLIED,
    MILTER_STATE_ENVELOPE_FROM,
    MILTER_STATE_ENVELOPE_FROM_REPLIED,
    MILTER_STATE_ENVELOPE_RECIPIENT,
    MILTER_STATE_ENVELOPE_RECIPIENT_REPLIED,
    MILTER_STATE_DATA,
    MILTER_STATE_DATA_REPLIED,
    MILTER_STATE_UNKNOWN,
    MILTER_STATE_UNKNOWN_REPLIED,
    MILTER_STATE_HEADER,
    MILTER_STATE_HEADER_REPLIED,
    MILTER_STATE_END_OF_HEADER,
    MILTER_STATE_END_OF_HEADER_REPLIED,
    MILTER_STATE_BODY,
    MILTER_STATE_BODY_REPLIED,
    MILTER_STATE_END_OF_MESSAGE,
    MILTER_STATE_END_OF_MESSAGE_REPLIED,
    MILTER_STATE_QUIT,
    MILTER_STATE_QUIT_REPLIED,
    MILTER_STATE_ABORT,
    MILTER_STATE_ABORT_REPLIED,
    MILTER_STATE_FINISHED
} MilterState;

typedef struct _MilterMessageResult         MilterMessageResult;
typedef struct _MilterMessageResultClass    MilterMessageResultClass;

struct _MilterMessageResult
{
    GObject object;
};

struct _MilterMessageResultClass
{
    GObjectClass parent_class;
};

GType          milter_message_result_get_type    (void) G_GNUC_CONST;

MilterMessageResult *milter_message_result_new   (void);

void           milter_message_result_start       (MilterMessageResult *result);
void           milter_message_result_stop        (MilterMessageResult *result);

const gchar   *milter_message_result_get_from    (MilterMessageResult *result);
void           milter_message_result_set_from    (MilterMessageResult *result,
                                                  const gchar         *from);

GList         *milter_message_result_get_recipients
                                                 (MilterMessageResult *result);
void           milter_message_result_set_recipients
                                                 (MilterMessageResult *result,
                                                  const GList         *recipients);
void           milter_message_result_add_recipient
                                                 (MilterMessageResult *result,
                                                  const gchar         *recipient);
void           milter_message_result_remove_recipient
                                                 (MilterMessageResult *result,
                                                  const gchar         *recipient);

GList         *milter_message_result_get_temporary_failed_recipients
                                                 (MilterMessageResult *result);
void           milter_message_result_set_temporary_failed_recipients
                                                 (MilterMessageResult *result,
                                                  const GList         *recipients);
void           milter_message_result_add_temporary_failed_recipient
                                                 (MilterMessageResult *result,
                                                  const gchar         *recipient);

GList         *milter_message_result_get_rejected_recipients
                                                 (MilterMessageResult *result);
void           milter_message_result_set_rejected_recipients
                                                 (MilterMessageResult *result,
                                                  const GList         *recipients);
void           milter_message_result_add_rejected_recipient
                                                 (MilterMessageResult *result,
                                                  const gchar         *recipient);

MilterHeaders *milter_message_result_get_headers (MilterMessageResult *result);
void           milter_message_result_set_headers (MilterMessageResult *result,
                                                  MilterHeaders       *headers);

guint64        milter_message_result_get_body_size
                                                 (MilterMessageResult *result);
void           milter_message_result_set_body_size
                                                 (MilterMessageResult *result,
                                                  guint64              body_size);
void           milter_message_result_add_body_size
                                                 (MilterMessageResult *result,
                                                  guint64              body_size);

MilterState    milter_message_result_get_state   (MilterMessageResult *result);
void           milter_message_result_set_state   (MilterMessageResult *result,
                                                  MilterState          state);

MilterStatus   milter_message_result_get_status  (MilterMessageResult *result);
void           milter_message_result_set_status  (MilterMessageResult *result,
                                                  MilterStatus         status);

MilterHeaders *milter_message_result_get_added_headers
                                                 (MilterMessageResult *result);
void           milter_message_result_set_added_headers
                                                 (MilterMessageResult *result,
                                                  MilterHeaders       *added_headers);

MilterHeaders *milter_message_result_get_removed_headers
                                                 (MilterMessageResult *result);
void           milter_message_result_set_removed_headers
                                                 (MilterMessageResult *result,
                                                  MilterHeaders       *removed_headers);

gboolean       milter_message_result_is_quarantine
                                                 (MilterMessageResult *result);
void           milter_message_result_set_quarantine
                                                 (MilterMessageResult *result,
                                                  gboolean             quarantine);
GTimeVal      *milter_message_result_get_start_time
                                                 (MilterMessageResult *result);
void           milter_message_result_set_start_time
                                                 (MilterMessageResult *result,
                                                  GTimeVal            *time);

GTimeVal      *milter_message_result_get_end_time(MilterMessageResult *result);
void           milter_message_result_set_end_time(MilterMessageResult *result,
                                                  GTimeVal            *time);

gdouble        milter_message_result_get_elapsed_time
                                                 (MilterMessageResult *result);
void           milter_message_result_set_elapsed_time
                                                 (MilterMessageResult *result,
                                                  gdouble              elapsed_time);

G_END_DECLS

#endif /* __MILTER_MESSAGE_RESULT_H__ */

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
