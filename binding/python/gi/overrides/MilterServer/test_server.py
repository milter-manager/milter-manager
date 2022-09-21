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

import subprocess

class TestServer(object):
    def __init__(self):
        self._milter_test_server = self._guess_milter_test_server()
        self.options = []

    def run(self, *options):
        command_line = [
            self._milter_test_server,
            "--output-message",
            "--color", "no",
            *self.options,
            *options,
        ]
        completed_process = subprocess.run(command_line,
                                           check=True,
                                           capture_output=True)
        return self._parse(completed_process.stdout)

    def _guess_milter_test_server(self):
        import os
        path = os.environ.get("MILTER_TEST_SERVER")
        if not path:
            try:
                import pkgconfig
            except ModuleNotFoundError:
                pass
            else:
                path = pkgconfig.variables("milter-server")["milter_test_server"]
        return path or "milter-test-server"

    def _parse(self, output):
        result = TestServerResult()
        lines = output.splitlines(keepends=True)
        # "status: accept\n" -> "accept"
        result.status = lines.pop(0).split(b": ", 2)[1].rstrip().decode()
        # "elapsed-time: 0.003769 seconds" -> 0.003769
        result.elapsed_time = \
            float(lines.pop(0).split(b": ", 2)[1].split()[0].rstrip().decode())
        lines.pop(0)

        mode = None
        while len(lines) > 0:
            line = lines.pop(0)
            if line.startswith(b"Envelope:"):
                mode = "envelope"
            elif line.startswith(b"Header:"):
                mode = "headers"
            elif line.startswith(b"Body:"):
                break
            else:
                if mode == "envelope":
                    if line[0] == b"-":
                        continue
                    target, value = line[1:].strip().split(b":", 2)
                    if target == b"MAIL FROM":
                        result.envelope_from = value
                    elif target == b"RCPT TO":
                        result.envelope_recipients.append(value)
                elif mode == "headers":
                    name, value = line.split(b":", 2)
                    result.headers.append([name.strip().decode(),
                                           value.strip().decode()])
        result.body = b"".join(lines)

        return result


class TestServerResult(object):
    def __init__(self):
        self.status = None
        self.elapsed_time = None
        self.envelope_from = None
        self.envelope_recipients = []
        self.headers = []
        self.body = None

    def to_dict(self):
        return {
            "status": self.status,
            "elapsed_time": self.elapsed_time,
            "envelope_from": self.envelope_from,
            "envelope_recipients": self.envelope_recipients,
            "headers": self.headers,
            "body": self.body,
        }
