# Copyright (C) 2010  Kouhei Sutou <kou@clear-code.com>
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
  module ConditionTable
    class InvalidValueError < ParseError
      attr_reader :value, :detail, :line, :path, :line_no
      def initialize(value, detail, line, path, line_no)
        @value = value
        @detail = detail
        @line = line
        @path = path
        @line_no = line_no
        super("#{@path}:#{@line_no}: #{detail}: <#{value}>: <#{line}>")
      end
    end

    class InvalidFormatError < ParseError
      attr_reader :line, :path, :line_no
      def initialize(line, path, line_no)
        @line = line
        @path = path
        @line_no = line_no
        super("#{@path}:#{@line_no}: invalid format <#{line}>")
      end
    end
  end
end
