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

#ifndef __MILTER_CLIENT_H__
#define __MILTER_CLIENT_H__

#include <milter/client/milter-client-context.h>
#include <milter/client/milter-client-enum-types.h>
#include <milter/core/milter-error-emittable.h>

G_BEGIN_DECLS

#define MILTER_TYPE_CLIENT            (milter_client_get_type())
#define MILTER_CLIENT(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), MILTER_TYPE_CLIENT, MilterClient))
#define MILTER_CLIENT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), MILTER_TYPE_CLIENT, MilterClientClass))
#define MILTER_CLIENT_IS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), MILTER_TYPE_CLIENT))
#define MILTER_CLIENT_IS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), MILTER_TYPE_CLIENT))
#define MILTER_CLIENT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), MILTER_TYPE_CLIENT, MilterClientClass))

typedef struct _MilterClient         MilterClient;
typedef struct _MilterClientClass    MilterClientClass;

struct _MilterClient
{
    GObject object;
};

struct _MilterClientClass
{
    GObjectClass parent_class;
    void (*connection_established)      (MilterClient *client,
                                         MilterClientContext *context);
};

GType                milter_client_get_type          (void) G_GNUC_CONST;

MilterClient        *milter_client_new               (void);

gboolean             milter_client_set_connection_spec
                                                     (MilterClient  *client,
                                                      const gchar   *spec,
                                                      GError       **error);
void                 milter_client_set_timeout       (MilterClient  *client,
                                                      guint          timeout);
gboolean             milter_client_main              (MilterClient  *client);
void                 milter_client_shutdown          (MilterClient  *client);


G_END_DECLS

#endif /* __MILTER_CLIENT_H__ */

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
