# Copyright (C) 2009-2014  Kouhei Sutou <kou@clear-code.com>
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

require "milter/manager/file-reader"

module Milter::Manager
  class ClamAVMilterConfigParser
    def initialize
      @variables = {}
    end

    def milter_socket
      @variables["MilterSocket"]
    end

    def example?
      @variables.has_key?("Example")
    end

    def parse(conf_file)
      return unless File.readable?(conf_file)

      content = FileReader.read(conf_file)
      content.each_line do |line|
        if /\A\s*(\w+)(?:\s+(.+))?\s*\z/ =~ line
          @variables[$1] = $2
        end
      end
    end
  end
end
