/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Copyright (C) 2009  Kouhei Sutou <kou@cozmixng.org>
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

#ifndef __MILTER_ESMTP_H__
#define __MILTER_ESMTP_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define MILTER_ESMTP_ERROR                  (milter_esmtp_error_quark())

typedef enum
{
    MILTER_ESMTP_ERROR_INVALID_FORMAT
} MilterEsmtpError;

GQuark           milter_esmtp_error_quark              (void);

gboolean         milter_esmtp_parse_mail_from_argument (const gchar  *argument,
                                                        gchar       **path,
                                                        GHashTable  **parameters,
                                                        GError      **error);
gboolean         milter_esmtp_parse_rcpt_to_argument   (const gchar  *argument,
                                                        gchar       **path,
                                                        GHashTable  **parameters,
                                                        GError      **error);
G_END_DECLS

#endif /* __MILTER_ESMTP_H__ */

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
