# Copyright (C) 2011  Kouhei Sutou <kou@clear-code.com>
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

require 'pathname'

module Milter
  module PathResolver
    def resolve_path(path)
      return [path] if Pathname(path).absolute?
      load_paths.collect do |load_path|
        full_path = File.join(load_path, path)
        if File.directory?(full_path)
          paths = []
          Dir.open(full_path) do |dir|
            dir.each do |sub_path|
              next if sub_path == "." or sub_path == ".."
              full_sub_path = File.join(full_path, sub_path)
              next if File.directory?(full_sub_path)
              paths << full_sub_path
            end
          end
          return paths
        elsif File.exist?(full_path)
          return [full_path]
        else
          Dir.glob(full_path).reject do |expanded_full_path|
            File.directory?(expanded_full_path)
          end
        end
      end.flatten
    end
  end
end
