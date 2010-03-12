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

#ifndef __MILTER_CLIENT_PRIVATE_H__
#define __MILTER_CLIENT_PRIVATE_H__

#include <milter/client.h>

G_BEGIN_DECLS

void                 milter_client_set_n_processing_sessions
                                                     (MilterClient *client,
                                                      guint n_processing_sessions);
void                 milter_client_set_n_processed_sessions
                                                     (MilterClient *client,
                                                      guint n_processed_sessions);
gboolean             milter_client_need_maintain     (MilterClient  *client,
                                                      guint          n_finished_sessions);

G_END_DECLS

#endif /* __MILTER_CLIENT_PRIVATE_H__ */

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
