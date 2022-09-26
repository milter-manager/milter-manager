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

import functools
import inspect
import io
import traceback

import gi.module

MilterCore = gi.module.get_introspection_module("MilterCore")
Logger = MilterCore.Logger
LogLevelFlags = MilterCore.LogLevelFlags

Logger.domain = "milter"
LogLevelFlags.ALL = functools.reduce(lambda x, y: x | y,
                                     LogLevelFlags.__flags_values__.values())

Logger.default = Logger.get_default()

def resolve_log_level_flags(level):
    if level is None:
        return LogLevelFlags.NONE
    if not isinstance(level, LogLevelFlags):
        level = getattr(LogLevelFlags, level.upper())
    return level

def get_target_level(self):
    return self.get_target_level()
Logger.target_level = property(get_target_level)

def set_target_level(self, level):
    self.set_target_level(resolve_log_level_flags(level))
Logger.target_level = Logger.target_level.setter(set_target_level)

def get_path(self):
    return self.get_path()
Logger.path = property(get_path)

def set_path(self, path):
    self.set_path(path)
Logger.path = Logger.path.setter(set_path)

def log(self, level, message, n_call_depth=None):
    level = resolve_log_level_flags(level)
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

for log_level_flag in LogLevelFlags.__flags_values__.values():
    name = log_level_flag.first_value_nick
    if name in ["default", "none"]:
        continue
    def create_log_method(name):
        def log_method(self, message=None):
            self.log(name, message, 1)
        return log_method
    setattr(Logger, name, create_log_method(name))
