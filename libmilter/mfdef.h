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

#ifndef __LIBMILTER_MFDEF_H__
#define __LIBMILTER_MFDEF_H__

#define SMFIR_ADDRCPT         '+'
#define SMFIR_DELRCPT         '-'
#define SMFIR_ADDRCPT_PAR     '2'
#define SMFIR_SHUTDOWN        '4'
#define SMFIR_ACCEPT          'a'
#define SMFIR_REPLBODY        'b'
#define SMFIR_CONTINUE        'c'
#define SMFIR_DISCARD         'd'
#define SMFIR_CHGFROM         'e'
#define SMFIR_CONN_FAIL       'f'
#define SMFIR_ADDHEADER       'h'
#define SMFIR_INSHEADER       'i'
#define SMFIR_SETSYMLIST      'l'
#define SMFIR_CHGHEADER       'm'
#define SMFIR_PROGRESS        'p'
#define SMFIR_QUARANTINE      'q'
#define SMFIR_REJECT          'r'
#define SMFIR_SKIP            's'
#define SMFIR_TEMPFAIL        't'
#define SMFIR_REPLYCODE       'y'


#define SMFIM_FIRST           SMFIM_CONNECT
#define SMFIM_LAST            SMFIM_EOH

#endif /* __LIBMILTER_MFDEF_H__ */

/*
vi:nowrap:ai:expandtab:sw=4
*/
