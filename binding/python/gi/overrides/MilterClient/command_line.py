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

import contextlib

import gi._ossighelper

import milter.core
from .client import Client

class CommandLine(object):
    def __init__(self, options={}):
        self._options = options

    @contextlib.contextmanager
    def run(self, argv=None):
        client = Client()
        def on_error(_client, error):
            milter.core.Logger.error(f"[client][error] {type(error)}: {error}")
        client.connect("error", on_error)
        client.event_loop = client.create_event_loop(True)
        yield client, self._options
        client.listen()
        client.drop_privilege()
        with gi._ossighelper.register_sigint_fallback(lambda: client.shutdown()):
            with gi._ossighelper.wakeup_on_signal():
                client.run()
