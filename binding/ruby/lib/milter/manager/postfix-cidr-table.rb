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
  class PostfixCIDRTable
    include ConditionTable
    include PostfixConditionTableParser

    def initialize
      @table = []
    end

    def parse(io)
      each_line(io) do |line, line_no|
        case line
        when /\A\s*([\d\.]+|[\da-fA-F:]+)(?:\/(\d+))?\s+(.+)\s*$/
          address = $1
          network = $2
          action = $3
          address << "/#{network}" unless network.nil?
          ip_address = nil
          begin
            ip_address = IPAddr.new(address)
          rescue ArgumentError
            raise InvalidValueError.new(address, $!.message, line,
                                        io.path, line_no)
          end
          @table << [ip_address, action]
        else
          raise InvalidFormatError.new(line, io.path, line_no)
        end
      end
    end

    def find(address)
      address = address.to_ip_address if address.respond_to?(:to_ip_address)
      @table.each do |match_address, action|
        return action if match_address === address
      end
      nil
    end
  end
end
