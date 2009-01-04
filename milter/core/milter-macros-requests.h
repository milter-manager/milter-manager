/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Copyright (C) 2008  Kouhei Sutou <kou@cozmixng.org>
 *
 *  This library is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your macros_request) any later version.
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

#ifndef __MILTER_MACROS_REQUESTS_H__
#define __MILTER_MACROS_REQUESTS_H__

#include <glib-object.h>
#include <milter/core/milter-protocol.h>

G_BEGIN_DECLS

#define MILTER_TYPE_MACROS_REQUESTS            (milter_macros_requests_get_type())
#define MILTER_MACROS_REQUESTS(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), MILTER_TYPE_MACROS_REQUESTS, MilterMacrosRequests))
#define MILTER_MACROS_REQUESTS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), MILTER_TYPE_MACROS_REQUESTS, MilterMacrosRequestsClass))
#define MILTER_IS_MACROS_REQUESTS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), MILTER_TYPE_MACROS_REQUESTS))
#define MILTER_IS_MACROS_REQUESTS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), MILTER_TYPE_MACROS_REQUESTS))
#define MILTER_MACROS_REQUESTS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), MILTER_TYPE_MACROS_REQUESTS, MilterMacrosRequestsClass))

typedef struct _MilterMacrosRequests         MilterMacrosRequests;
typedef struct _MilterMacrosRequestsClass    MilterMacrosRequestsClass;

struct _MilterMacrosRequests
{
    GObject object;
};

struct _MilterMacrosRequestsClass
{
    GObjectClass parent_class;
};

GType                 milter_macros_requests_get_type (void) G_GNUC_CONST;

MilterMacrosRequests *milter_macros_requests_new      (void);

void                  milter_macros_requests_set_symbols
                                                      (MilterMacrosRequests *requests,
                                                       MilterCommand         command,
                                                       const gchar          *first_symbol,
                                                       ...) G_GNUC_NULL_TERMINATED;
void                  milter_macros_requests_set_symbols_va_list
                                                      (MilterMacrosRequests *requests,
                                                       MilterCommand         command,
                                                       const gchar          *first_symbol,
                                                       va_list               args);
void                  milter_macros_requests_set_symbols_string_array
                                                      (MilterMacrosRequests *requests,
                                                       MilterCommand         command,
                                                       const gchar         **strings);
GList                *milter_macros_requests_get_symbols
                                                      (MilterMacrosRequests *requests,
                                                       MilterCommand         command);
GHashTable           *milter_macros_requests_get_all_symbols
                                                      (MilterMacrosRequests *requests);
void                  milter_macros_requests_merge    (MilterMacrosRequests *dest,
                                                       MilterMacrosRequests *src);
void                  milter_macros_requests_foreach  (MilterMacrosRequests *requests,
                                                       GHFunc                func,
                                                       gpointer              user_data);

G_END_DECLS

#endif /* __MILTER_MACROS_REQUESTS_H__ */

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
