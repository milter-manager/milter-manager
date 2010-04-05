/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Copyright (C) 2010  Kouhei Sutou <kou@clear-code.com>
 *
 * Based on GLib's gmem.c. The following is the original header:
 *
 * Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#  include "../../config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "milter-memory-profile.h"
#include "milter-logger.h"

#define MEM_PROFILE_TABLE_SIZE 4096
#define	PROFILE_TABLE(f1, f2, f3) \
    ((((f3) << 2) | ((f2) << 1) | (f1)) * (MEM_PROFILE_TABLE_SIZE + 1))
#define N_PROFILE_DATA (MEM_PROFILE_TABLE_SIZE + 1) * 8 * sizeof(guint)

typedef enum {
  PROFILER_FREE		= 0,
  PROFILER_ALLOC	= 1,
  PROFILER_RELOC	= 2,
  PROFILER_ZINIT	= 4
} ProfilerJob;

static gboolean profile_enabled = FALSE;
static gboolean profile_verbose = FALSE;
static guint profile_data[N_PROFILE_DATA];
static gsize profile_allocs = 0;
static gsize profile_zinit = 0;
static gsize profile_frees = 0;
static GStaticMutex profile_mutex = G_STATIC_MUTEX_INIT;

static void
profiler_log (ProfilerJob job,
	      gsize       n_bytes,
	      gboolean    success)
{
    g_static_mutex_lock(&profile_mutex);

    if (n_bytes < MEM_PROFILE_TABLE_SIZE) {
        profile_data[n_bytes + PROFILE_TABLE((job & PROFILER_ALLOC) != 0,
                                             (job & PROFILER_RELOC) != 0,
                                             success != 0)] += 1;
    } else {
        profile_data[MEM_PROFILE_TABLE_SIZE +
                     PROFILE_TABLE((job & PROFILER_ALLOC) != 0,
                                   (job & PROFILER_RELOC) != 0,
                                   success != 0)] += 1;
    }

    if (success) {
        if (job & PROFILER_ALLOC) {
            profile_allocs += n_bytes;
            if (job & PROFILER_ZINIT)
                profile_zinit += n_bytes;
        } else {
            profile_frees += n_bytes;
        }
    }

    g_static_mutex_unlock(&profile_mutex);
}

static void
profile_print_locked (guint   *local_data,
		      gboolean success)
{
    gboolean need_header = TRUE;
    guint i;

    for (i = 0; i <= MEM_PROFILE_TABLE_SIZE; i++) {
        glong t_malloc = local_data[i + PROFILE_TABLE(1, 0, success)];
        glong t_realloc = local_data[i + PROFILE_TABLE(1, 1, success)];
        glong t_free = local_data[i + PROFILE_TABLE(0, 0, success)];
        glong t_refree = local_data[i + PROFILE_TABLE(0, 1, success)];

        if (!t_malloc && !t_realloc && !t_free && !t_refree)
            continue;

        if (need_header) {
            need_header = FALSE;
            milter_profile(" blocks of | allocated  | freed      | allocated  | freed      | n_bytes   ");
            milter_profile("  n_bytes  | n_times by | n_times by | n_times by | n_times by | remaining ");
            milter_profile("           | malloc()   | free()     | realloc()  | realloc()  |           ");
            milter_profile("===========|============|============|============|============|===========");
	}
        if (i < MEM_PROFILE_TABLE_SIZE) {
            milter_profile("%10u | %10ld | %10ld | %10ld | %10ld |%+11ld",
                           i, t_malloc, t_free, t_realloc, t_refree,
                           (t_malloc - t_free + t_realloc - t_refree) * i);
        } else if (i >= MEM_PROFILE_TABLE_SIZE) {
            milter_profile("   >%6u | %10ld | %10ld | %10ld | %10ld |        ***",
                           i, t_malloc, t_free, t_realloc, t_refree);
        }
    }

    if (need_header)
        milter_profile(" --- none ---");
}

void
milter_memory_profile_report (void)
{
    guint local_data[N_PROFILE_DATA];
    gsize local_allocs;
    gsize local_zinit;
    gsize local_frees;

    if (!profile_enabled)
        return;

    g_static_mutex_lock(&profile_mutex);
    local_allocs = profile_allocs;
    local_zinit = profile_zinit;
    local_frees = profile_frees;
    memcpy(local_data, profile_data, N_PROFILE_DATA);
    g_static_mutex_unlock(&profile_mutex);

    if (profile_verbose) {
        milter_profile("Memory statistics (successful operations):");
        profile_print_locked(local_data, TRUE);
        milter_profile("Memory statistics (failing operations):");
        profile_print_locked(local_data, FALSE);
    }
    milter_profile("Total bytes: allocated=%"G_GSIZE_FORMAT", "
                   "zero-initialized=%"G_GSIZE_FORMAT" (%.2f%%), "
                   "freed=%"G_GSIZE_FORMAT" (%.2f%%), "
                   "remaining=%"G_GSIZE_FORMAT,
                   local_allocs,
                   local_zinit,
                   ((gdouble)local_zinit) / local_allocs * 100.0,
                   local_frees,
                   ((gdouble)local_frees) / local_allocs * 100.0,
                   local_allocs - local_frees);
}

gboolean
milter_memory_profile_get_data (gsize *n_allocates,
                                gsize *n_zero_initializes,
                                gsize *n_frees)
{
    if (!profile_enabled)
        return FALSE;

    g_static_mutex_lock(&profile_mutex);
    *n_allocates = profile_allocs;
    *n_zero_initializes = profile_zinit;
    *n_frees = profile_frees;
    g_static_mutex_unlock(&profile_mutex);

    return TRUE;
}

static gpointer
profiler_try_malloc (gsize n_bytes)
{
    gsize *p;

    p = malloc(sizeof(gsize) * 1 + n_bytes);

    if (p) {
        p[0] = n_bytes;	/* length */
        profiler_log(PROFILER_ALLOC, n_bytes, TRUE);
        p += 1;
    } else {
        profiler_log(PROFILER_ALLOC, n_bytes, FALSE);
    }

    return p;
}

static gpointer
profiler_malloc (gsize n_bytes)
{
    gpointer mem = profiler_try_malloc(n_bytes);

    if (!mem)
        milter_memory_profile_report();

    return mem;
}

static gpointer
profiler_calloc (gsize n_blocks,
		 gsize n_block_bytes)
{
    gsize l = n_blocks * n_block_bytes;
    gsize *p;

    p = calloc(1, sizeof(gsize) * 1 + l);

    if (p) {
        p[0] = l;		/* length */
        profiler_log(PROFILER_ALLOC | PROFILER_ZINIT, l, TRUE);
        p += 1;
    } else {
        profiler_log(PROFILER_ALLOC | PROFILER_ZINIT, l, FALSE);
        milter_memory_profile_report();
    }

    return p;
}

static void
profiler_free (gpointer mem)
{
    gsize *p = mem;

    p -= 1;
    profiler_log(PROFILER_FREE,
                 p[0],	/* length */
                 TRUE);
    free(p);
}

static gpointer
profiler_try_realloc (gpointer mem,
		      gsize    n_bytes)
{
    gsize *p = mem;

    p = realloc(mem ? p - 1: NULL, sizeof(gsize) * 1 + n_bytes);

    if (p) {
        if (mem)
            profiler_log(PROFILER_FREE | PROFILER_RELOC, p[0], TRUE);
        p[0] = n_bytes;
        profiler_log(PROFILER_ALLOC | PROFILER_RELOC, p[0], TRUE);
        p += 1;
    } else {
        profiler_log(PROFILER_ALLOC | PROFILER_RELOC, n_bytes, FALSE);
    }

    return p;
}

static gpointer
profiler_realloc (gpointer mem,
		  gsize    n_bytes)
{
    mem = profiler_try_realloc(mem, n_bytes);

    if (!mem)
        milter_memory_profile_report();

    return mem;
}

static GMemVTable profiler_table = {
    profiler_malloc,
    profiler_realloc,
    profiler_free,
    profiler_calloc,
    profiler_try_malloc,
    profiler_try_realloc,
};

void
milter_memory_profile_init (void)
{
    memset(profile_data, 0, N_PROFILE_DATA);
}

void
milter_memory_profile_quit (void)
{
}

void
milter_memory_profile_enable (void)
{
    const gchar *memory_profile_verbose_env;

    g_mem_set_vtable(&profiler_table);
    profile_enabled = TRUE;
    memory_profile_verbose_env = g_getenv("MILTER_MEMORY_PROFILE_VERBOSE");
    if (memory_profile_verbose_env &&
        g_str_equal(memory_profile_verbose_env, "yes")) {
        profile_verbose = TRUE;
    }
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
