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

#ifndef __MILTER_PROTOCOL_H__
#define __MILTER_PROTOCOL_H__

G_BEGIN_DECLS

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
    MILTER_COMMAND_MAIL =			'M', /* MAIL from */
    MILTER_COMMAND_END_OF_HEADER =		'N', /* end of header */
    MILTER_COMMAND_OPTION_NEGOTIATION =		'O', /* Option negotiation */
    MILTER_COMMAND_QUIT =			'Q', /* QUIT */
    MILTER_COMMAND_RCPT =			'R', /* RCPT to */
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

G_END_DECLS

#endif /* __MILTER_PROTOCOL_H__ */

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
