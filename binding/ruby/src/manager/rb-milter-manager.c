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

#include "rb-milter-manager-private.h"

VALUE rb_mMilterManager;

void
Init_milter_manager (void)
{
    rb_mMilterManager = rb_define_module_under(rb_mMilter, "Manager");
    Init_milter_manager_gstring();
    Init_milter_manager_configuration();
    Init_milter_manager_child();
    Init_milter_manager_applicable_condition();
    Init_milter_manager_egg();
    Init_milter_manager_children();
    Init_milter_manager_control_command_encoder();
    Init_milter_manager_control_reply_encoder();
    Init_milter_manager_control_decoder();
}
