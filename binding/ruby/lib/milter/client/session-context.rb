# Copyright (C) 2009-2013  Kouhei Sutou <kou@clear-code.com>
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

module Milter
  class ClientSessionContext
    include MacroPredicates

    def initialize(context)
      @context = context
      clear
    end

    def status=(value)
      case value
      when String, Symbol
        status_name = value.to_s.upcase.gsub(/-/, "_")
        unless Milter::Status.const_defined?(status_name)
          raise ArgumentError, "unknown status: <#{value.inspect}>"
        end
        status = Milter::Status.const_get(status_name)
      when Milter::Status, nil
      else
        message =
          "should be one of [String, Symbol, Milter::Status, nil]: " +
          "<#{value.inspect}>"
        raise ArgumentError, message
      end
      @status = value
    end

    def status
      @status || Milter::Status::DEFAULT
    end

    def set_reply(code, extended_code, reason)
      @context.set_reply(code, extended_code, reason)
      if 400 <= code and code < 500
        self.status = :temporary_failure
      elsif 500 <= code and code < 600
        self.status = :reject
      end
    end

    def [](name)
      (@macros ||= @context.available_macros || {})[name]
    end

    def clear
      @macros = nil
      @status = nil
    end

    def method_missing(name, *args, &block)
      if @context.respond_to?(name)
        @context.send(name, *args, &block)
      else
        super
      end
    end
  end
end
