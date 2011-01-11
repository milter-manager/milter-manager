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

#ifndef __MILTER_CORE_H__
#define __MILTER_CORE_H__

#include <glib.h>

#include <milter/core/milter-version.h>
#include <milter/core/milter-protocol.h>
#include <milter/core/milter-esmtp.h>
#include <milter/core/milter-encoder.h>
#include <milter/core/milter-command-encoder.h>
#include <milter/core/milter-reply-encoder.h>
#include <milter/core/milter-decoder.h>
#include <milter/core/milter-command-decoder.h>
#include <milter/core/milter-reply-decoder.h>
#include <milter/core/milter-writer.h>
#include <milter/core/milter-reader.h>
#include <milter/core/milter-utils.h>
#include <milter/core/milter-connection.h>
#include <milter/core/milter-agent.h>
#include <milter/core/milter-protocol-agent.h>
#include <milter/core/milter-macros-requests.h>
#include <milter/core/milter-headers.h>
#include <milter/core/milter-logger.h>
#include <milter/core/milter-syslog-logger.h>
#include <milter/core/milter-error-emittable.h>
#include <milter/core/milter-finished-emittable.h>
#include <milter/core/milter-reply-signals.h>
#include <milter/core/milter-message-result.h>
#include <milter/core/milter-memory-profile.h>
#include <milter/core/milter-event-loop.h>
#include <milter/core/milter-glib-event-loop.h>
#include <milter/core/milter-libev-event-loop.h>
#include <milter/core/milter-enum-types.h>

G_BEGIN_DECLS

/**
 * SECTION: core
 * @title: Core library
 * @short_description: The core library to be used by
 *                     milter-manager libraries.
 *
 * The milter-core library includes common features that are
 * used by the milter-client, milter-server and
 * milter-manager libraries.
 *
 * Applications and libraries that use the milter-core
 * library need to call milter_init() at the beginning and
 * milter_quit() at the end:
 *
 * |[
 * #include <milter/core.h>
 *
 * int
 * main (int argc, char **argv)
 * {
 *     milter_init();
 *     main_codes_that_use_the_milter_core_library();
 *     milter_quit();
 *
 *     return 0;
 * }
 * ]|
 */

/**
 * milter_init:
 *
 * Call this function before using any other the milter-core
 * library functions.
 */
void milter_init (void);

/**
 * milter_quit:
 *
 * Call this function after the milter-core library use.
 */
void milter_quit (void);

G_END_DECLS

#endif /* __MILTER_CORE_H__ */

/*
vi:nowrap:ai:expandtab:sw=4
*/
