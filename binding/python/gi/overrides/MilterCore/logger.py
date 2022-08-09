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
Logger = MilterCore.Logger

Logger.domain = "milter"

def log(self, level, message, n_call_depth=None):
    if not isinstance(level, MilterCore.LogLevelFlags):
        level = getattr(MilterCore.LogLevelFlags, level.upper())
    if not self.get_interesting_level() & level:
        return
    frame_info = inspect.stack()[n_call_depth or 1]
    file = frame_info.filename
    line = frame_info.lineno
    function = frame_info.function
    if callable(message):
        message = message()
    if message is None:
        message = ""
    elif isinstance(message, Exception):
        output = io.StringIO()
        traceback.print_exception(message, file=output)
        message = output.getvalue()
    else:
        message = str(message)
    for message_line in io.StringIO(message.rstrip()):
        self.log_literal(self.domain,
                         level,
                         file,
                         line,
                         function,
                         message_line.rstrip())
Logger.log = log

def error(message=None):
    Logger.get_default().log("error", message, 1)
Logger.error = staticmethod(error)
