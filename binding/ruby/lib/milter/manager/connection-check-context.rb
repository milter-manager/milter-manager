# Copyright (C) 2010-2011  Kouhei Sutou <kou@clear-code.com>
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

module Milter::Manager
  class ConnectionCheckContext
    include Milter::MacroNameNormalizer

    def initialize(leader)
      @leader = leader
    end

    def smtp_client_address
      children.smtp_client_address
    end

    def smtp_server_address
      client_context.socket_address
    end

    def [](macro_name)
      macros[normalize_macro_name(macro_name)]
    end

    def smtp_server_name
      self["v"]
    end

    private
    def children
      @children ||= @leader.children
    end

    def client_context
      @client_context ||= @leader.client_context
    end

    def macros
      @macros ||= client_context.available_macros || {}
    end
  end
end
