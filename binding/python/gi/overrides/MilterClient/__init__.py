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

MilterClient = gi.module.get_introspection_module("MilterClient")
MilterClient.Client.init()

from .client import Client
from .client_context import ClientContext
from .command_line import CommandLine
from .milter_runner import MilterRunner
from .session import Session
from .session_context import SessionContext

ClientEventLoopBackend = MilterClient.ClientEventLoopBackend

__all__ = [
    "Client",
    "ClientContext",
    "ClientEventLoopBackend",
    "CommandLine",
    "MilterRunner",
    "Session",
    "SessionContext",
]
