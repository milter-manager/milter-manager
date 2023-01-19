#!/usr/bin/env python3
#
# Copyright (C) 2022  Sutou Kouhei <kou@clear-code.com>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

import os
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.dirname(__file__)))

import milter.client

class MilterExternal(milter.client.Session):
    def __init__(self, context, timeout):
        super().__init__(context)
        self._timeout = timeout

    def end_of_message(self):
        command_line = [
            sys.executable,
            "-c",
            f"import time; time.sleep({self._timeout})",
        ]
        process = subprocess.Popen(command_line)
        def on_exit(pid, wait_status):
            self._remove_source(self._source_id)
            self._accept()
            self._context.emit("end_of_message_response",
                               self._context.status)
        self._source_id = self._watch_child(process.pid, on_exit)
        self._delay_response()

    def abort(self, status):
        if self._source_id is not None:
            self._remove_source(self._source_id)

    def reset(self):
        self._source_id = None

command_line = milter.client.CommandLine()
with command_line.run() as (client, options):
    client.register(MilterExternal, 1)
