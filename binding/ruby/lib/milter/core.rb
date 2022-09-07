# Copyright (C) 2008-2022  Sutou Kouhei <kou@clear-code.com>
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

require 'glib2'
GLib::Log.cancel_handler

begin
  require "gobject-introspection"
  Milter = GObjectIntrospection.load("MilterCore")
  module Milter
    class Error < StandardError
    end
  end
rescue LoadError
  require "milter_core.so"
end

require 'milter/core/compatible'

require 'milter/core/logger'
require 'milter/core/socket-address'
require 'milter/core/macro'
require 'milter/core/path'
require 'milter/core/callback'
