# Copyright (C) 2009  Kouhei Sutou <kou@clear-code.com>
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
  class MilterGreylistConfigParser
    attr_reader :max_tarpit_time
    def initialize
      @variables = {}
      @recipient_acls = []
      @data_acls = []
      @max_tarpit_time = nil
    end

    def socket
      if @variables["socket"].is_a?(Array)
        @variables["socket"].first
      else
        @variables["socket"]
      end
    end

    def parse(conf_file)
      return unless File.readable?(conf_file)

      lines = normalize_lines(conf_file)
      lines.each do |line|
        case line.chomp
        when /\A\s*r?acl\s+/
          recipient_acl = $POSTMATCH
          @recipient_acls << recipient_acl
          parse_recipient_acl(recipient_acl)
        when /\A\s*d?acl\s+/
          @data_acls << $POSTMATCH
        when /\A\s*([\w_]+)\s+"(.+?)"\s*\z/
          @variables[$1] = $2
        when /\A\s*([\w_]+)\s+"(.+?)"\s*(\d+)\z/
          @variables[$1] = [$2, $3.to_i]
        when /\A\s*([\w_]+)\s+/
          @variables[$1] = $POSTMATCH
        end
      end
    end

    def apply(milter)
      if @max_tarpit_time
        milter.writing_timeout += @max_tarpit_time
        milter.reading_timeout += @max_tarpit_time
      end
    end

    private
    def normalize_lines(conf_file)
      lines = []
      continued_line = false
      File.open(conf_file) do |conf|
        conf.each_line do |line|
          case line
          when /\A\s*#/
            continued_line = false
          else
            line = lines.pop.rstrip + " " + line if continued_line
            lines << line
            if /\\\z/ =~ line
              continued_line = true
            else
              continued_line = false
            end
          end
        end
      end
      lines
    end

    def parse_recipient_acl(acl)
      case acl
      when /\btarpit\s+(\d+)([smhdw]?)\b/
        time = resolve_time($1.to_i, $2)
        @max_tarpit_time = [@max_tarpit_time || 0, time].max
      end
    end

    def resolve_time(number, unit)
      case unit
      when "m"
        number * 60
      when "h"
        number * 60 * 60
      when "d"
        number * 60 * 60 * 24
      when "w"
        number * 60 * 60 * 24 * 7
      else
        number
      end
    end
  end
end
