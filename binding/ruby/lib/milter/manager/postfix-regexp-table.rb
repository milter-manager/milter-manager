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
require 'milter/manager/condition-table'
require 'milter/manager/postfix-condition-table-parser'

module Milter::Manager
  class PostfixRegexpTable
    include ConditionTable
    include PostfixConditionTableParser

    def initialize
      @table = []
    end

    def parse(io)
      current_table = @table
      tables = []
      each_line(io) do |line, line_no|
        case line
        when /\A\s*if\s+(!)?\/(.*)\/([imx]+)?$/
          not_flag = $1
          pattern = $2
          flag = $3
          regexp = create_regexp(pattern, flag, io, line, line_no)
          new_table = []
          current_table << [not_flag == "!", regexp, new_table]
          current_table = new_table
          tables << new_table
        when /\A\s*(!)?\/(.*)\/([imx]+)?\s+(.+)\s*$/
          not_flag = $1
          pattern = $2
          flag = $3
          action = $4
          regexp = create_regexp(pattern, flag, io, line, line_no)
          current_table << [not_flag == "!", regexp, action]
        when /\Aendif\s*$/
          current_table = tables.pop
        else
          raise InvalidFormatError.new(line, io.path, io.lineno)
        end
      end
      unless tables.empty?
        raise InvalidFormatError("endif isn't matched", io.path, io.lineno)
      end
    end

    def find(text)
      find_action(@table, text)
     end

    private
    def create_regexp(pattern, flag, io, line, line_no)
      regexp_flag = Regexp::IGNORECASE
      if flag
        regexp_flag &= Regexp::IGNORECASE if flag.index("i")
        regexp_flag |= Regexp::MULTILINE if flag.index("m")
      end
      begin
        Regexp.new(pattern, regexp_flag)
      rescue RegexpError
        raise InvalidValueError.new(pattern, $!.message, line,
                                    io.path, line_no)
      end
    end

    def find_action(table, text)
      table.each do |negative, regexp, table_or_action|
        match_data = nil
        if negative
          next if regexp =~ text
        else
          match_data = regexp.match(text)
          next unless match_data
        end
        if table_or_action.is_a?(Array)
          action = find_action(table_or_action, text)
          return expand_variables(action, match_data) if action
        else
          return expand_variables(table_or_action, match_data)
        end
      end
      nil
    end

    def expand_variables(action, match_data)
      return action if match_data.nil?
      action.gsub(/\$(?:(\$)|(\d+)|\((\d+)\)|\{(\d+)\})/) do |string|
        doller = $1
        n = $2 || $3 || $4
        if doller
          "$"
        else
          match_data[n.to_i]
        end
      end
    end
  end
end
