/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Copyright (C) 2010  Kouhei Sutou <kou@clear-code.com>
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

#ifndef __MILTER_SESSION_RESULT_H__
#define __MILTER_SESSION_RESULT_H__

#include <glib-object.h>
#include <milter/core/milter-message-result.h>

G_BEGIN_DECLS

#define MILTER_TYPE_SESSION_RESULT            (milter_session_result_get_type())
#define MILTER_SESSION_RESULT(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), MILTER_TYPE_SESSION_RESULT, MilterSessionResult))
#define MILTER_SESSION_RESULT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), MILTER_TYPE_SESSION_RESULT, MilterSessionResultClass))
#define MILTER_IS_SESSION_RESULT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), MILTER_TYPE_SESSION_RESULT))
#define MILTER_IS_SESSION_RESULT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), MILTER_TYPE_SESSION_RESULT))
#define MILTER_SESSION_RESULT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), MILTER_TYPE_SESSION_RESULT, MilterSessionResultClass))

typedef struct _MilterSessionResult         MilterSessionResult;
typedef struct _MilterSessionResultClass    MilterSessionResultClass;

struct _MilterSessionResult
{
    GObject object;
};

struct _MilterSessionResultClass
{
    GObjectClass parent_class;
};

GType          milter_session_result_get_type    (void) G_GNUC_CONST;

MilterSessionResult *milter_session_result_new   (void);

void           milter_session_result_start       (MilterSessionResult *result);
void           milter_session_result_stop        (MilterSessionResult *result);

GList         *milter_session_result_get_message_results
                                                 (MilterSessionResult *result);
void           milter_session_result_set_message_results
                                                 (MilterSessionResult *result,
                                                  const GList         *message_results);
void           milter_session_result_add_message_result
                                                 (MilterSessionResult *result,
                                                  MilterMessageResult *message_result);
void           milter_session_result_remove_message_result
                                                 (MilterSessionResult *result,
                                                  MilterMessageResult *message_result);

gboolean       milter_session_result_is_disconnected
                                                 (MilterSessionResult *result);
void           milter_session_result_set_disconnected
                                                 (MilterSessionResult *result,
                                                  gboolean             disconnected);
GTimeVal      *milter_session_result_get_start_time
                                                 (MilterSessionResult *result);
void           milter_session_result_set_start_time
                                                 (MilterSessionResult *result,
                                                  GTimeVal            *time);

GTimeVal      *milter_session_result_get_end_time(MilterSessionResult *result);
void           milter_session_result_set_end_time(MilterSessionResult *result,
                                                  GTimeVal            *time);

gdouble        milter_session_result_get_elapsed_time
                                                 (MilterSessionResult *result);
void           milter_session_result_set_elapsed_time
                                                 (MilterSessionResult *result,
                                                  gdouble              elapsed_time);

G_END_DECLS

#endif /* __MILTER_SESSION_RESULT_H__ */

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
