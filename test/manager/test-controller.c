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

#include <gcutter.h>

#define shutdown inet_shutdown
#include <milter-manager-test-utils.h>
#include <milter/manager/milter-manager-controller.h>
#undef shutdown

void test_negotiate (void);

static MilterManagerConfiguration *config;
static MilterManagerController *controller;

static void
add_load_path (const gchar *path)
{
    if (!path)
        return;
    milter_manager_controller_add_load_path(controller, path);
}

void
setup (void)
{
    config = milter_manager_configuration_new();
    controller = milter_manager_controller_new("ruby",
                                               "configuration", config,
                                               NULL);
    add_load_path(g_getenv("MILTER_MANAGER_CONFIG_DIR"));
    milter_manager_controller_load(controller, "milter-manager.conf");
}

void
teardown (void)
{
    if (config)
        g_object_unref(config);
    if (controller)
        g_object_unref(controller);
}

void
test_negotiate (void)
{
    cut_fail("WRITEME");
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
