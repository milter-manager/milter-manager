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
  class EnmaSocketDetector
    def initialize(conf_file)
      @conf_file = conf_file
    end

    def detect
      return nil unless File.readable?(@conf_file)

      connection_spec = nil
      content = FileReader.read(@conf_file)
      content.each_line do |line|
        if /\A\s*milter\.socket\s*:\s*(.+)/ =~ line
          connection_spec = $1
        end
      end
      connection_spec
    end
  end
end
