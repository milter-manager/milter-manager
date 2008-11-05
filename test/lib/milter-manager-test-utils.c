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

#include <string.h>

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

void
milter_manager_test_header_inspect (GString *string,
                                    MilterManagerTestHeader *header,
                                    gpointer user_data)
{
    g_string_append_printf(string,
                           "<%s[%u] = %s>",
                           header->name,
                           header->index,
                           header->value);
}

void
milter_manager_test_header_inspect_without_index (GString *string,
                                                  MilterManagerTestHeader *header,
                                                  gpointer user_data)
{
    g_string_append_printf(string,
                           "<%s = %s>",
                           header->name, header->value);
}

gboolean
milter_manager_test_header_equal (MilterManagerTestHeader *header_a,
                                  MilterManagerTestHeader *header_b)
{
    if (!header_a->name || !header_b->name)
        return FALSE;

    return (header_a->index == header_b->index && 
            !strcmp(header_a->name, header_b->name) &&
            !g_strcmp0(header_a->value, header_b->value));
}

gint
milter_manager_test_header_compare (MilterManagerTestHeader *header_a,
                                    MilterManagerTestHeader *header_b)
{
    if (header_a->index != header_b->index)
        return (header_a->index - header_b->index);

    if (g_strcmp0(header_a->name, header_b->name))
        return g_strcmp0(header_a->name, header_b->name);

    return g_strcmp0(header_a->value, header_b->value);
}

MilterManagerTestHeader *
milter_manager_test_header_new (guint index, const gchar *name, const gchar *value)
{
    MilterManagerTestHeader *header;
    header = g_new0(MilterManagerTestHeader, 1);
    header->name = g_strdup(name);
    header->value = g_strdup(value);
    header->index = index;
    return header;
}

void
milter_manager_test_header_free (MilterManagerTestHeader *header)
{
    g_free(header->name);
    g_free(header->value);

    g_free(header);
}

void
milter_manager_test_value_with_param_inspect (GString *string,
                                              MilterManagerTestValueWithParam *value,
                                              gpointer user_data)
{
    g_string_append_printf(string,
                           "<%s(%s)>",
                           value->value, value->param);
}

gboolean
milter_manager_test_value_with_param_equal (MilterManagerTestValueWithParam *value_a,
                                            MilterManagerTestValueWithParam *value_b)
{
    if (!value_a->value || !value_b->value)
        return FALSE;

    return (!strcmp(value_a->value, value_b->value) &&
            !g_strcmp0(value_a->param, value_b->param));
}

MilterManagerTestValueWithParam *
milter_manager_test_value_with_param_new (const gchar *value_string, const gchar *param)
{
    MilterManagerTestValueWithParam *value;
    value = g_new0(MilterManagerTestValueWithParam, 1);
    value->value = g_strdup(value_string);
    value->param = g_strdup(param);
    return value;
}

void
milter_manager_test_value_with_param_free (MilterManagerTestValueWithParam *value)
{
    g_free(value->value);
    g_free(value->param);

    g_free(value);
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
