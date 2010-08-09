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

require 'English'

module Milter::Manager
  module PostfixConditionTableParser
    private
    def each_line(io)
      current_line = nil
      io.each_line do |line|
        case line
        when /\A\s*#/, /\A\s*$/
        when /\A\s+/
          target_line = $POSTMATCH.chomp
          if current_line
            current_line << " "
          else
            current_line = ""
          end
          current_line << target_line
        else
          yield(current_line, io.lineno - 1) if current_line
          current_line = line.chomp
        end
      end
      yield(current_line, io.lineno) if current_line
    end
  end
end
