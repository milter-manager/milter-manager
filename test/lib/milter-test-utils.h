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

#ifndef __MILTER_TEST_UTILS_H__
#define __MILTER_TEST_UTILS_H__

#include <string.h>
#include <errno.h>

#include <gcutter.h>
#include "milter-assertions.h"
#include "milter-test-client.h"
#include "milter-test-server.h"

#ifndef cut_message
#  define cut_message(...) __VA_ARGS__
#endif

G_BEGIN_DECLS

typedef enum
{
    MILTER_TEST_ERRNO_ECONNREFUSED = ECONNREFUSED
} MilterTestErrno;

const gchar   *milter_test_get_base_dir    (void);

void           milter_test_write_data_to_io_channel
                                           (GIOChannel  *io_channel,
                                            const gchar *data,
                                            gsize        data_size);
gboolean       milter_test_read_data_from_io_channel
                                           (GIOChannel  *channel,
                                            gchar      **data,
                                            gsize       *data_size);
gboolean       milter_test_decode_io_channel
                                           (MilterDecoder  *decoder,
                                            GIOChannel     *channel);
guint          milter_test_io_add_decode_watch
                                           (GIOChannel     *channel,
                                            MilterDecoder  *decoder);

gboolean       milter_test_equal_symbols_table
                                           (GHashTable *table1,
                                            GHashTable *table2);
gchar         *milter_test_inspect_symbols_table
                                           (GHashTable *table);
void           milter_test_pump_all_events (void);

G_END_DECLS

#endif /* __MILTER_TEST_UTILS_H__ */

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
