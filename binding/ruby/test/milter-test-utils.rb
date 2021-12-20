# Copyright (C) 2008-2021  Sutou Kouhei <kou@clear-code.com>
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

require 'fileutils'
require 'pathname'
require 'socket'
require 'tempfile'
require 'tmpdir'

module MilterTestUtils
  def fixture_path(*paths)
    Pathname(__FILE__).dirname.join("fixtures", *paths)
  end
end

Milter::Logger.default.target_level = Milter::LOG_LEVEL_NONE

require 'milter-encoder-test-utils'
require 'milter-manager-encoder-test-utils'
require 'milter-detector-test-utils'
require 'milter-parse-test-utils'
require 'milter-event-loop-test-utils'
require 'milter-multi-process-test-utils'
