# Copyright (C) 2022  Sutou Kouhei <kou@clear-code.com>
#
# This library is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this library.  If not, see <http://www.gnu.org/licenses/>.

import inspect
import io
import traceback

import gi.module

MilterCore = gi.module.get_introspection_module("MilterCore")
Agent = MilterCore.Agent

def get_event_loop(self):
    return self.get_event_loop()
Agent.event_loop = property(get_event_loop)

def set_event_loop(self, value):
    self.set_event_loop(value)
Agent.event_loop = Agent.event_loop.setter(set_event_loop)
