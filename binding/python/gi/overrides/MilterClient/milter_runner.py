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

class MilterRunError(Exception):
    def __init__(self, return_code, command_line, stdout, stderr):
        self.return_code = return_code
        self.command_line = command_line
        self.stdout = stdout
        self.stderr = stderr

    def __str__(self):
        return f"""
Command line: {self.command_line}
Return code: {self.return_code}
Standard output:
=================================
{self.stdout.strip()}
=================================
Standard error:
=================================
{self.stderr.strip()}
=================================
"""

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
        self.stdout = tempfile.NamedTemporaryFile(mode="w+",
                                                  suffix=".stdout.log")
        self.stderr = tempfile.NamedTemporaryFile(mode="w+",
                                                  suffix=".stderr.log")
        self.process = subprocess.Popen(command_line,
                                        env=self._env,
                                        stdout=self.stdout,
                                        stderr=self.stderr,
                                        text=True)
        start = time.time()
        while True:
            tcp_client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            try:
                tcp_client.connect((self._host, self._port))
                break
            except ConnectionRefusedError:
                if self.process.poll() is not None:
                    self.stdout.seek(0)
                    self.stderr.seek(0)
                    raise MilterRunError(self.process.returncode,
                                         self.process.args,
                                         self.stdout.read(),
                                         self.stderr.read())
                if time.time() - start > self._start_timeout:
                    raise
            finally:
                tcp_client.close()

    def __enter__(self):
        return self

    def __exit__(self, exc_type, value, traceback):
        self.process.terminate()
