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

#include <milter-core.h>

#define SMFIR_ADDRCPT         MILTER_REPLY_ADD_RECEIPT
#define SMFIR_DELRCPT         MILTER_REPLY_DELETE_RECEIPT
#define SMFIR_ADDRCPT_PAR     MILTER_REPLY_ADD_RECEIPT_PARAMETERS
#define SMFIR_SHUTDOWN        MILTER_REPLY_SHUTDOWN
#define SMFIR_ACCEPT          MILTER_REPLY_ACCEPT
#define SMFIR_REPLBODY        MILTER_REPLY_REPLACE_BODY
#define SMFIR_CONTINUE        MILTER_REPLY_CONTINUE
#define SMFIR_DISCARD         MILTER_REPLY_DISCARD
#define SMFIR_CHGFROM         MILTER_REPLY_CHANGE_FROM
#define SMFIR_CONN_FAIL       MILTER_REPLY_CONNECTION_FAILURE
#define SMFIR_ADDHEADER       MILTER_REPLY_ADD_HEADER
#define SMFIR_INSHEADER       MILTER_REPLY_INSERT_HEADER
#define SMFIR_SETSYMLIST      MILTER_REPLY_SET_SYMBOL_LIST
#define SMFIR_CHGHEADER       MILTER_REPLY_CHANGE_HEADER
#define SMFIR_PROGRESS        MILTER_REPLY_PROGRESS
#define SMFIR_QUARANTINE      MILTER_REPLY_QUARANTINE
#define SMFIR_REJECT          MILTER_REPLY_REJECT
#define SMFIR_SKIP            MILTER_REPLY_SKIP
#define SMFIR_TEMPFAIL        MILTER_REPLY_TEMP_FAIL
#define SMFIR_REPLYCODE       MILTER_REPLY_REPLY_CODE


#define SMFIP_NOCONNECT       MILTER_STEP_NO_CONNECT
#define SMFIP_NOHELO          MILTER_STEP_NO_HELO
#define SMFIP_NOMAIL          MILTER_STEP_NO_MAIL
#define SMFIP_NORCPT          MILTER_STEP_NO_RCPT
#define SMFIP_NOBODY          MILTER_STEP_NO_BODY
#define SMFIP_NOHDRS          MILTER_STEP_NO_HEADERS
#define SMFIP_NOEOH           MILTER_STEP_NO_END_OF_HEADER
#define SMFIP_NR_HDR          MILTER_STEP_NO_REPLY_HEADER
#define SMFIP_NOHREPL         SMFIP_NR_HDR
#define SMFIP_NOUNKNOWN       MILTER_STEP_NO_UNKNOWN
#define SMFIP_NODATA          MILTER_STEP_NO_DATA
#define SMFIP_SKIP            MILTER_STEP_SKIP
#define SMFIP_RCPT_REJ        MILTER_STEP_RCPT_REJECTED
#define SMFIP_NR_CONN         MILTER_STEP_NO_REPLY_CONNECT
#define SMFIP_NR_HELO         MILTER_STEP_NO_REPLY_HELO
#define SMFIP_NR_MAIL         MILTER_STEP_NO_REPLY_MAIL
#define SMFIP_NR_RCPT         MILTER_STEP_NO_REPLY_RCPT
#define SMFIP_NR_DATA         MILTER_STEP_NO_REPLY_DATA
#define SMFIP_NR_UNKN         MILTER_STEP_NO_REPLY_UNKNOWN
#define SMFIP_NR_EOH          MILTER_STEP_NO_REPLY_END_OF_HEADER
#define SMFIP_NR_BODY         MILTER_STEP_NO_REPLY_BODY
#define SMFIP_HDR_LEADSPC     MILTER_STEP_HEADER_LEAD_SPACE


#endif /* __LIBMILTER_MFDEF_H__ */

/*
vi:nowrap:ai:expandtab:sw=4
*/
