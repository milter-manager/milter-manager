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

#include "rb-milter-core-private.h"

void
Init_milter_core (void)
{
    rb_mMilterCore = rb_define_module_under(rb_mMilter, "Core");

    Init_milter_socket_address();
    Init_milter_utils();
    Init_milter_connection();
    Init_milter_protocol();
    Init_milter_option();
    Init_milter_macros_requests();
    Init_milter_encoder();
    Init_milter_command_encoder();
    Init_milter_reply_encoder();
    Init_milter_decoder();
    Init_milter_protocol_agent();
}
