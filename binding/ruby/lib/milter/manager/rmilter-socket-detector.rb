# Copyright (C) 2017  Kouhei Sutou <kou@clear-code.com>
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
  # Rmilter has been obsolete since Rspamd 1.6
  class RmilterSocketDetector
    def initialize(conf_file)
      @conf_file = conf_file
    end

    def detect
      return nil unless File.readable?(@conf_file)

      parse(@conf_file)
    end

    private
    def parse(path)
      connection_spec = nil
      content = FileReader.read(path)
      content.each_line do |line|
        case line
        when /\A\s*\.(?:try_)?include\s+(.+)$/
          sub_path_pattern = $1.strip
          Dir.glob(sub_path_pattern).each do |sub_path|
            connection_spec = parse(sub_path) || connection_spec
          end
        when /\A\s*bind_socket\s*=\s*(.+);$/
          connection_spec = $1.strip
        end
      end
      connection_spec
    end
  end
end
