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

MilterCore = gi.module.get_introspection_module("MilterCore")

class SessionContext(object):
    def __init__(self, context):
        self._context = context
        self.clear()

    def clear(self):
        self._macros = None
        self._status = MilterCore.Status.DEFAULT

    @property
    def status(self):
        return self._status

    def _normalize_status(self, value):
        if isinstance(value, MilterCore.Status):
            return value
        return getattr(MilterCore.Status, value.replace("-", "_").upper())

    @status.setter
    def status(self, value):
        self._status = self._normalize_status(value)

    def set_reply(self, code, extended_code, reason):
        self._context.set_reply(code, extended_code, reason)
        if 400 <= code < 500:
            self.status = "temporary_failure"
        elif 500 <= code < 600:
            self.status = "reject"

    def __getitem__(self, name):
        if self._macros is None:
            self._macros = self._context.get_available_macros() or {}
        return self._macros.get(name)

    def __getattr__(self, name):
        return getattr(self._context, name)
