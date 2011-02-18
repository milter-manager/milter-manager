/* -*- c-file-style: "ruby" -*- */
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

#ifndef __RB_MILTER_MANAGER_PRIVATE_H__
#define __RB_MILTER_MANAGER_PRIVATE_H__

#include "rb-milter-manager.h"

extern void Init_milter_manager (void);
extern void Init_milter_manager_gstring (void);
extern void Init_milter_manager_configuration (void);
extern void Init_milter_manager_child (void);
extern void Init_milter_manager_applicable_condition (void);
extern void Init_milter_manager_egg (void);
extern void Init_milter_manager_children (void);
extern void Init_milter_manager_control_command_encoder (void);
extern void Init_milter_manager_control_reply_encoder (void);
extern void Init_milter_manager_control_decoder (void);

extern VALUE rb_milter_manager_gstring_handle_to_xml_signal (guint num, const GValue *values);

#endif
