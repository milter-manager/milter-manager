#!/usr/bin/env ruby
#
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

require 'pathname'

base = Pathname.new(__FILE__).dirname.expand_path
top = (base + "..").expand_path

test_unit_dir = top + "test-unit" + "lib"
$LOAD_PATH.unshift(test_unit_dir.to_s)
require 'test/unit'

require 'milter/manager'

$LOAD_PATH.unshift(base.to_s)
require 'milter-test-utils'

ARGV.unshift("--priority-mode")
exit Test::Unit::AutoRunner.run(true, base)
