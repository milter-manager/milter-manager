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

#include <milter-client/milter-client-handler.h>

#if NETINET || NETINET6 || NETUNIX
union bigsockaddr
{
	struct sockaddr		sa;	/* general version */
# if NETUNIX
	struct sockaddr_un	sunix;	/* UNIX family */
# endif /* NETUNIX */
# if NETINET
	struct sockaddr_in	sin;	/* INET family */
# endif /* NETINET */
# if NETINET6
	struct sockaddr_in6	sin6;	/* INET/IPv6 */
# endif /* NETINET6 */
};
#endif

#endif /* __MILTER_CLIENT_H__ */

/*
vi:nowrap:ai:expandtab:sw=4
*/
