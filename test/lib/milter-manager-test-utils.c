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

#include "milter-manager-test-utils.h"

static gchar *base_dir = NULL;
const gchar *
milter_manager_test_get_base_dir (void)
{
    const gchar *dir;

    if (base_dir)
        return base_dir;

    dir = g_getenv("BASE_DIR");
    if (!dir)
        dir = ".";

    if (g_path_is_absolute(dir)) {
        base_dir = g_strdup(dir);
    } else {
        gchar *current_dir;

        current_dir = g_get_current_dir();
        base_dir = g_build_filename(current_dir, dir, NULL);
        g_free(current_dir);
    }

    return base_dir;
}

gchar *
milter_manager_test_header_inspect (MilterManagerTestHeader *header)
{
    return g_strdup_printf("<%s = %s>", header->name, header->value);
}

MilterManagerTestHeader *
milter_manager_test_header_new (const gchar *name, const gchar *value)
{
    MilterManagerTestHeader *header;
    header = g_new0(MilterManagerTestHeader, 1);
    header->name = g_strdup(name);
    header->value = g_strdup(value);
    return header;
}

void
milter_manager_test_header_free (MilterManagerTestHeader *header)
{
    g_free(header->name);
    g_free(header->value);

    g_free(header);
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
