/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Copyright (C) 2008-2010  Kouhei Sutou <kou@clear-code.com>
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

#ifndef __MILTER_READER_H__
#define __MILTER_READER_H__

#include <glib-object.h>

#include <milter/core/milter-protocol.h>
#include <milter/core/milter-option.h>
#include <milter/core/milter-error-emittable.h>
#include <milter/core/milter-finished-emittable.h>
#include <milter/core/milter-event-loop.h>

G_BEGIN_DECLS

#define MILTER_READER_ERROR           (milter_reader_error_quark())

#define MILTER_TYPE_READER            (milter_reader_get_type())
#define MILTER_READER(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), MILTER_TYPE_READER, MilterReader))
#define MILTER_READER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), MILTER_TYPE_READER, MilterReaderClass))
#define MILTER_IS_READER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), MILTER_TYPE_READER))
#define MILTER_IS_READER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), MILTER_TYPE_READER))
#define MILTER_READER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), MILTER_TYPE_READER, MilterReaderClass))

typedef enum
{
    MILTER_READER_ERROR_NO_CHANNEL,
    MILTER_READER_ERROR_IO_ERROR
} MilterReaderError;

typedef struct _MilterReader         MilterReader;
typedef struct _MilterReaderClass    MilterReaderClass;

struct _MilterReader
{
    GObject object;
};

struct _MilterReaderClass
{
    GObjectClass parent_class;

    void     (*flow)               (MilterReader *reader,
                                    const gchar  *data,
                                    gsize         data_size);
};

GQuark           milter_reader_error_quark    (void);

GType            milter_reader_get_type       (void) G_GNUC_CONST;

MilterReader    *milter_reader_io_channel_new (GIOChannel       *channel);

void             milter_reader_start          (MilterReader     *reader,
                                               MilterEventLoop  *loop);
gboolean         milter_reader_is_watching    (MilterReader     *reader);
void             milter_reader_shutdown       (MilterReader     *reader);

guint            milter_reader_get_tag        (MilterReader     *reader);
void             milter_reader_set_tag        (MilterReader     *reader,
                                               guint             tag);

G_END_DECLS

#endif /* __MILTER_READER_H__ */

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
