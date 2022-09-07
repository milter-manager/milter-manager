#!/usr/bin/env ruby
#
# Copyright (C) 2008-2022  Sutou Kouhei <kou@clear-code.com>
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

test_dir = Pathname.new(__dir__).expand_path
top_dir = (test_dir + "..").expand_path

test_unit_dir = top_dir + "test-unit" + "lib"
$LOAD_PATH.unshift(test_unit_dir.to_s)
require 'test/unit'

lib_dir = top_dir + "lib"
$LOAD_PATH.unshift(lib_dir.to_s)
require 'milter/manager'

$LOAD_PATH.unshift(test_dir.to_s)
require 'milter-test-utils'

ARGV.unshift("--priority-mode")
exit(Test::Unit::AutoRunner.run(true, test_dir))
