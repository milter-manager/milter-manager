/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Copyright (C) 2008-2009  Kouhei Sutou <kou@clear-code.com>
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

#ifndef __MILTER_PROTOCOL_H__
#define __MILTER_PROTOCOL_H__

#include <sys/types.h>
#include <sys/socket.h>

#include <glib-object.h>

#include <milter/core/milter-option.h>

G_BEGIN_DECLS

#define MILTER_CHUNK_SIZE 65535

typedef enum
{
    MILTER_COMMAND_ABORT =			'A', /* Abort */
    MILTER_COMMAND_BODY =			'B', /* Body chunk */
    MILTER_COMMAND_CONNECT =			'C', /* Connection information */
    MILTER_COMMAND_DEFINE_MACRO =		'D', /* Define macro */
    MILTER_COMMAND_END_OF_MESSAGE =		'E', /* final body chunk (End) */
    MILTER_COMMAND_HELO =			'H', /* HELO/EHLO */
    MILTER_COMMAND_QUIT_NEW_CONNECTION =	'K', /* QUIT but
                                                        new connection follows */
    MILTER_COMMAND_HEADER =			'L', /* Header */
    MILTER_COMMAND_ENVELOPE_FROM =		'M', /* MAIL from */
    MILTER_COMMAND_END_OF_HEADER =		'N', /* end of header */
    MILTER_COMMAND_NEGOTIATE =			'O', /* Option negotiation */
    MILTER_COMMAND_QUIT =			'Q', /* QUIT */
    MILTER_COMMAND_ENVELOPE_RECIPIENT =		'R', /* RCPT to */
    MILTER_COMMAND_DATA =			'T', /* DATA */
    MILTER_COMMAND_UNKNOWN =			'U'  /* Any unknown command */
} MilterCommand;

typedef enum
{
    MILTER_SOCKET_FAMILY_UNKNOWN =	'U',
    MILTER_SOCKET_FAMILY_UNIX =		'L',
    MILTER_SOCKET_FAMILY_INET =		'4',
    MILTER_SOCKET_FAMILY_INET6 =	'6'
} MilterSocketFamily;

typedef enum
{
    MILTER_REPLY_ADD_RECIPIENT =		'+', /* add recipient */
    MILTER_REPLY_DELETE_RECIPIENT	=	'-', /* remove recipient */
    /* add recipient (incl. ESMTP args) */
    MILTER_REPLY_ADD_RECIPIENT_WITH_PARAMETERS ='2',
    /* 421: shutdown (internal to MTA) */
    MILTER_REPLY_SHUTDOWN =			'4',
    MILTER_REPLY_ACCEPT =			'a', /* accept */
    MILTER_REPLY_REPLACE_BODY =			'b', /* replace body (chunk) */
    MILTER_REPLY_CONTINUE =			'c', /* continue */
    MILTER_REPLY_DISCARD =			'd', /* discard */
    /* change envelope sender (from) */
    MILTER_REPLY_CHANGE_FROM =			'e',
    /* cause a connection failure */
    MILTER_REPLY_CONNECTION_FAILURE =		'f',
    MILTER_REPLY_ADD_HEADER =			'h', /* add header */
    MILTER_REPLY_INSERT_HEADER =		'i', /* insert header */
    /* set list of symbols (macros) */
    MILTER_REPLY_SET_SYMBOL_LIST =		'l',
    MILTER_REPLY_CHANGE_HEADER =		'm', /* change header */
    MILTER_REPLY_PROGRESS =			'p', /* progress */
    MILTER_REPLY_QUARANTINE =			'q', /* quarantine */
    MILTER_REPLY_REJECT =			'r', /* reject */
    MILTER_REPLY_SKIP =				's', /* skip */
    MILTER_REPLY_TEMPORARY_FAILURE =		't', /* tempfail */
    MILTER_REPLY_REPLY_CODE =			'y'  /* reply code etc */
} MilterReply;

typedef enum
{
    MILTER_STATUS_DEFAULT,
    MILTER_STATUS_NOT_CHANGE,
    MILTER_STATUS_CONTINUE,
    MILTER_STATUS_REJECT,
    MILTER_STATUS_DISCARD,
    MILTER_STATUS_ACCEPT,
    MILTER_STATUS_TEMPORARY_FAILURE,
    MILTER_STATUS_NO_REPLY,
    MILTER_STATUS_SKIP,
    MILTER_STATUS_ALL_OPTIONS,
    MILTER_STATUS_PROGRESS,
    MILTER_STATUS_ABORT,
    MILTER_STATUS_QUARANTINE,
    MILTER_STATUS_STOP,
    MILTER_STATUS_ERROR
} MilterStatus;

#define MILTER_STATUS_IS_PASS(status)           \
    (MILTER_STATUS_DEFAULT <= (status) &&       \
     (status) <= MILTER_STATUS_CONTINUE)

typedef enum
{
    MILTER_MACRO_STAGE_CONNECT = 0,
    MILTER_MACRO_STAGE_HELO = 1,
    MILTER_MACRO_STAGE_ENVELOPE_FROM = 2,
    MILTER_MACRO_STAGE_ENVELOPE_RECIPIENT = 3,
    MILTER_MACRO_STAGE_DATA = 4,
    MILTER_MACRO_STAGE_END_OF_MESSAGE = 5,
    MILTER_MACRO_STAGE_END_OF_HEADER = 6
} MilterMacroStage;

gint       milter_status_compare        (MilterStatus status1,
                                         MilterStatus status2);

G_END_DECLS

#endif /* __MILTER_PROTOCOL_H__ */

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
