# Copyright (C) 2011  Kouhei Sutou <kou@clear-code.com>
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

require 'milter/core'

begin
  require "milter_server.so"
rescue LoadError
  require "gobject-introspection"
  MilterServer = GObjectIntrospection.load("MilterServer")
  module Milter
    ServerContext = MilterServer::ServerContext
  end
end

module Milter
  autoload :TestServer, 'milter/server/testing'
end
