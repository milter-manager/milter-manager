# Copyright (C) 2013  Kouhei Sutou <kou@clear-code.com>
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

require "milter/core"
require "milter_client.so"

module Milter
  class ClientContextState
    def negotiate?
      self == NEGOTIATE
    end

    def connect?
      self == CONNECT
    end

    def helo?
      self == HELO
    end

    def envelope_from?
      self == ENVELOPE_FROM
    end

    def envelope_recipient?
      self == ENVELOPE_RECIPIENT
    end

    def data?
      self == DATA
    end

    def unknown?
      self == UNKNOWN
    end

    def header?
      self == HEADER
    end

    def end_of_header?
      self == END_OF_HEADER
    end

    def body?
      self == BODY
    end

    def end_of_message?
      self == END_OF_MESSAGE
    end

    def quit?
      self == QUIT
    end

    def abort?
      self == ABORT
    end
  end
end
