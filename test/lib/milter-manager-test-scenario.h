/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Copyright (C) 2008-2011  Kouhei Sutou <kou@clear-code.com>
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

#ifndef __MILTER_MANAGER_TEST_SCENARIO_H__
#define __MILTER_MANAGER_TEST_SCENARIO_H__

#include <gcutter.h>

G_BEGIN_DECLS

#define MILTER_TYPE_MANAGER_TEST_SCENARIO            (milter_manager_test_scenario_get_type())
#define MILTER_MANAGER_TEST_SCENARIO(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), MILTER_TYPE_MANAGER_TEST_SCENARIO, MilterManagerTestScenario))
#define MILTER_MANAGER_TEST_SCENARIO_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), MILTER_TYPE_MANAGER_TEST_SCENARIO, MilterManagerTestScenarioClass))
#define MILTER_MANAGER_IS_TEST_SCENARIO(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), MILTER_TYPE_MANAGER_TEST_SCENARIO))
#define MILTER_MANAGER_IS_TEST_SCENARIO_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), MILTER_TYPE_MANAGER_TEST_SCENARIO))
#define MILTER_MANAGER_TEST_SCENARIO_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), MILTER_TYPE_MANAGER_TEST_SCENARIO, MilterManagerTestScenarioClass))

typedef struct _MilterManagerTestScenario         MilterManagerTestScenario;
typedef struct _MilterManagerTestScenarioClass    MilterManagerTestScenarioClass;

struct _MilterManagerTestScenario
{
    GObject object;
};

struct _MilterManagerTestScenarioClass
{
    GObjectClass parent_class;
};

#define MILTER_MANAGER_TEST_SCENARIO_GROUP_NAME "scenario"

GType                      milter_manager_test_scenario_get_type (void) G_GNUC_CONST;

MilterManagerTestScenario *milter_manager_test_scenario_new
                                              (void);
void                       milter_manager_test_scenario_load
                                              (MilterManagerTestScenario *scenario,
                                               const gchar *base_dir,
                                               const gchar *scenario_name);

const GList               *milter_manager_test_scenario_get_imported_scenarios
                                              (MilterManagerTestScenario *scenario);

void                       milter_manager_test_scenario_start_clients
                                              (MilterManagerTestScenario *scenario,
                                               MilterEventLoop           *loop);
const GList               *milter_manager_test_scenario_get_started_clients
                                              (MilterManagerTestScenario *scenario);
const GList               *milter_manager_test_scenario_get_eggs
                                              (MilterManagerTestScenario *scenario);

gboolean                   milter_manager_test_scenario_has_group
                                              (MilterManagerTestScenario *scenario,
                                               const gchar *group);
gboolean                   milter_manager_test_scenario_has_key
                                              (MilterManagerTestScenario *scenario,
                                               const gchar *group,
                                               const gchar *key);
MilterOption              *milter_manager_test_scenario_get_option
                                              (MilterManagerTestScenario *scenario,
                                               const gchar *group,
                                               MilterManagerTestScenario *base_scenario);
gint                       milter_manager_test_scenario_get_integer
                                              (MilterManagerTestScenario *scenario,
                                               const gchar *group,
                                               const gchar *key);
gdouble                    milter_manager_test_scenario_get_double
                                              (MilterManagerTestScenario *scenario,
                                               const gchar *group,
                                               const gchar *key);
gboolean                   milter_manager_test_scenario_get_boolean
                                              (MilterManagerTestScenario *scenario,
                                               const gchar *group,
                                               const gchar *key);
const gchar               *milter_manager_test_scenario_get_string
                                              (MilterManagerTestScenario *scenario,
                                               const gchar *group,
                                               const gchar *key);
const gchar               *milter_manager_test_scenario_get_string_with_sub_key
                                              (MilterManagerTestScenario *scenario,
                                               const gchar *group,
                                               const gchar *key,
                                               const gchar *sub_key);
const gchar              **milter_manager_test_scenario_get_string_list
                                              (MilterManagerTestScenario *scenario,
                                               const gchar *group,
                                               const gchar *key,
                                               gsize *length);
const GList               *milter_manager_test_scenario_get_string_g_list
                                              (MilterManagerTestScenario *scenario,
                                               const gchar *group,
                                               const gchar *key);
gint                       milter_manager_test_scenario_get_enum
                                              (MilterManagerTestScenario *scenario,
                                               const gchar *group,
                                               const gchar *key,
                                               GType enum_type);
guint                      milter_manager_test_scenario_get_flags
                                              (MilterManagerTestScenario *scenario,
                                               const gchar *group,
                                               const gchar *key,
                                               GType flags_type);
const GList               *milter_manager_test_scenario_get_pair_list
                                              (MilterManagerTestScenario *scenario,
                                               const gchar *group,
                                               const gchar *key);
const GList               *milter_manager_test_scenario_get_pair_list_without_sort
                                              (MilterManagerTestScenario *scenario,
                                               const gchar *group,
                                               const gchar *key);
const GList               *milter_manager_test_scenario_get_option_list
                                              (MilterManagerTestScenario *scenario,
                                               const gchar *group,
                                               const gchar *key);
const GList               *milter_manager_test_scenario_get_header_list
                                              (MilterManagerTestScenario *scenario,
                                               const gchar *group,
                                               const gchar *key);

G_END_DECLS

#endif /* __MILTER_MANAGER_TEST_SCENARIO_H__ */

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
