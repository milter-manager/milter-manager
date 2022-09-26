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

import gi.module

import milter.core

MilterClient = gi.module.get_introspection_module("MilterClient")
ClientContext = MilterClient.ClientContext

def get_state(self):
    return self.get_state()
ClientContext.state = property(get_state)

def set_state(self, state):
    self.set_state(state)
ClientContext.state = ClientContext.state.setter(set_state)
