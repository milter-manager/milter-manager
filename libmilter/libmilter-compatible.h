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

#ifndef __LIBMILTER_COMPATIBLE_H__
#define __LIBMILTER_COMPATIBLE_H__

#include <libmilter/mfapi.h>

#include <milter/client.h>

G_BEGIN_DECLS

#define SMFI_CONTEXT_ERROR           (smfi_context_error_quark())

#define SMFI_TYPE_CONTEXT            (smfi_context_get_type())
#define SMFI_CONTEXT(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), SMFI_TYPE_CONTEXT, SmfiContext))
#define SMFI_CONTEXT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), SMFI_TYPE_CONTEXT, SmfiContextClass))
#define SMFI_IS_CONTEXT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), SMFI_TYPE_CONTEXT))
#define SMFI_IS_CONTEXT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), SMFI_TYPE_CONTEXT))
#define SMFI_CONTEXT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), SMFI_TYPE_CONTEXT, SmfiContextClass))

typedef enum
{
    SMFI_CONTEXT_ERROR_INVALID_CODE
} SmfiContextError;

#define _SmfiContext smfi_str
typedef struct _SmfiContext         SmfiContext;
typedef struct _SmfiContextClass    SmfiContextClass;

struct _SmfiContext
{
    GObject object;
};

struct _SmfiContextClass
{
    GObjectClass parent_class;
};

GQuark               smfi_context_error_quark       (void);

GType                smfi_context_get_type          (void) G_GNUC_CONST;

SmfiContext         *smfi_context_new               (MilterClientContext *client_context);

void                 libmilter_compatible_reset     (void);

MilterStatus         libmilter_compatible_convert_status_to
                                                    (sfsistat     status);
sfsistat             libmilter_compatible_convert_status_from
                                                    (MilterStatus status);

G_END_DECLS

#endif /* __LIBMILTER_COMPATIBLE_H__ */

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
