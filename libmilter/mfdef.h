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


#define SMFIP_NOCONNECT       0x00000001L
#define SMFIP_NOHELO          0x00000002L
#define SMFIP_NOMAIL          0x00000004L
#define SMFIP_NORCPT          0x00000008L
#define SMFIP_NOBODY          0x00000010L
#define SMFIP_NOHDRS          0x00000020L
#define SMFIP_NOEOH           0x00000040L
#define SMFIP_NR_HDR          0x00000080L
#define SMFIP_NOHREPL         SMFIP_NR_HDR
#define SMFIP_NOUNKNOWN       0x00000100L
#define SMFIP_NODATA          0x00000200L
#define SMFIP_SKIP            0x00000400L
#define SMFIP_RCPT_REJ        0x00000800L
#define SMFIP_NR_CONN         0x00001000L
#define SMFIP_NR_HELO         0x00002000L
#define SMFIP_NR_MAIL         0x00004000L
#define SMFIP_NR_RCPT         0x00008000L
#define SMFIP_NR_DATA         0x00010000L
#define SMFIP_NR_UNKN         0x00020000L
#define SMFIP_NR_EOH          0x00040000L
#define SMFIP_NR_BODY         0x00080000L
#define SMFIP_HDR_LEADSPC     0x00100000L


#endif /* __LIBMILTER_MFDEF_H__ */

/*
vi:nowrap:ai:expandtab:sw=4
*/
