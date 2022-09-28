# -*- ruby -*-
#
# Copyright (C) 2022  Sutou Kouhei <kou@clear-code.com>
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

def version
  meson_build = File.join(__dir__, "meson.build")
  File.read(meson_build).scan(/version: '([^']+)'/)[0][0]
end

desc "Tag #{version} and push"
task :tag do
  cd(__dir__) do
    sh("git", "tag", "-a", version, "-m", "Released #{version}!!!")
    sh("git", "push", "--tags")
  end
end
