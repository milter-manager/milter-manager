/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Copyright (C) 2008-2010  Kouhei Sutou <kou@clear-code.com>
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

#ifdef HAVE_CONFIG_H
#  include "../../config.h"
#endif /* HAVE_CONFIG_H */

#include <milter/core.h>

static gboolean initialized = FALSE;

static guint glib_log_handler_id = 0;
static guint gobject_log_handler_id = 0;
static guint gthread_log_handler_id = 0;
static guint gmodule_log_handler_id = 0;
static guint milter_core_log_handler_id = 0;

static void
remove_glib_log_handlers (void)
{
#define REMOVE(domain, prefix)                                          \
    g_log_remove_handler(domain, prefix ## _log_handler_id)

    REMOVE("GLib", glib);
    REMOVE("GLib-GObject", gobject);
    REMOVE("GThread", gthread);
    REMOVE("GModule", gmodule);

#undef REMOVE
}

static void
delegate_glib_log_handlers (void)
{
    glib_log_handler_id = MILTER_GLIB_LOG_DELEGATE("GLib");
    gobject_log_handler_id = MILTER_GLIB_LOG_DELEGATE("GLib-GObject");
    gthread_log_handler_id = MILTER_GLIB_LOG_DELEGATE("GThread");
    gmodule_log_handler_id = MILTER_GLIB_LOG_DELEGATE("GModule");
}

void
milter_init (void)
{
    const gchar *memory_profile_env;

    if (initialized) {
        remove_glib_log_handlers();
        delegate_glib_log_handlers();
        return;
    }

    initialized = TRUE;

    milter_memory_profile_init();

    memory_profile_env = g_getenv("MILTER_MEMORY_PROFILE");
    if (memory_profile_env && g_str_equal(memory_profile_env, "yes")) {
        milter_memory_profile_enable();
    }

    if (!g_thread_supported())
        g_thread_init(NULL);

    g_type_init();

    delegate_glib_log_handlers();
    milter_core_log_handler_id = MILTER_GLIB_LOG_DELEGATE("milter-core");
}

void
milter_quit (void)
{
    if (!initialized)
        return;

    remove_glib_log_handlers();
    g_log_remove_handler("milter-core", milter_core_log_handler_id);

    milter_memory_profile_quit();

    initialized = FALSE;
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
