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
MilterCore.init()

from .agent import Agent
from .logger import Logger

MilterCore.StepFlags.NO_REPLY_MASK = \
    MilterCore.StepFlags.NO_REPLY_CONNECT | \
    MilterCore.StepFlags.NO_REPLY_HELO | \
    MilterCore.StepFlags.NO_REPLY_ENVELOPE_FROM | \
    MilterCore.StepFlags.NO_REPLY_ENVELOPE_RECIPIENT | \
    MilterCore.StepFlags.NO_REPLY_DATA | \
    MilterCore.StepFlags.NO_REPLY_HEADER | \
    MilterCore.StepFlags.NO_REPLY_UNKNOWN | \
    MilterCore.StepFlags.NO_REPLY_END_OF_HEADER | \
    MilterCore.StepFlags.NO_REPLY_BODY

MilterCore.VERSION = ".".join([str(MilterCore.TOOLKIT_VERSION_MAJOR),
                               str(MilterCore.TOOLKIT_VERSION_MINOR),
                               str(MilterCore.TOOLKIT_VERSION_MICRO)])
