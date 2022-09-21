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

import socket
import subprocess
import sys
import tempfile
import time

class MilterRunner(object):
    def __init__(self,
                 path,
                 *args,
                 env=None,
                 start_timeout=5,
                 host="127.0.0.1",
                 port=20025):
        self._path = path
        self._args = args
        self._env = env
        self._start_timeout = start_timeout
        self._host = host
        self._port = port
        self.connection_spec = f"inet:{self._port}@{self._host}"
        command_line = [
            sys.executable,
            self._path,
            "--connection-spec", self.connection_spec,
            *self._args,
        ]
        self.stdout = tempfile.TemporaryFile()
        self.stderr = tempfile.TemporaryFile()
        self.process = subprocess.Popen(command_line,
                                        stdout=subprocess.PIPE,
                                        stderr=subprocess.PIPE,
                                        text=True)
        start = time.time()
        while True:
            tcp_client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            try:
                tcp_client.connect((self._host, self._port))
                break
            except ConnectionRefusedError:
                if time.time() - start > self._start_timeout:
                    raise
            finally:
                tcp_client.close()

    def __enter__(self):
        return self

    def __exit__(self, exc_type, value, traceback):
        self.process.terminate()
