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
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.dirname(__file__)))

import milter.client

class MilterReplace(milter.client.Session):
    def __init__(self, context, patterns):
        super().__init__(context)
        self._patterns = patterns

    def header(self, name, value):
        self._headers.append([name, value])

    def body(self, chunk):
        self._body += chunk.get_data()

    def end_of_message(self):
        header_indexes = {}
        for name, value in self._headers:
            if not name in header_indexes:
                header_indexes[name] = 0
            header_indexes[name] += 1
            for pattern, replaced in self._patterns.items():
                replaced_value, _ = pattern.subn(replaced, value)
                if value != replaced_value:
                    self._change_header(name,
                                        header_indexes[name],
                                        replaced_value)
                    break

        for pattern, replaced in self._patterns.items():
            body = self._body.decode("utf-8")
            replaced_body, _ = pattern.subn(replaced, body)
            if body != replaced_body:
                self._replace_body(replaced_body)

    def reset(self):
        self._headers = []
        self._body = b""

command_line = milter.client.CommandLine()
with command_line.run() as (client, options):
    patterns = {
        re.compile("viagra", re.IGNORECASE): "XXX",
    }
    client.register(MilterReplace, patterns)
    client.set_connection_spec('inet:10025')
