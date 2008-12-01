#!/usr/bin/env ruby

require 'pathname'

require 'rubygems'
gem 'rr'
require 'rr'

base = Pathname.new(__FILE__).dirname.expand_path
top = (base + "..").expand_path
test_unit_dir = top + "test-unit" + "lib"

$LOAD_PATH.unshift(test_unit_dir.to_s)
require 'test/unit'

if ENV["NO_MAKE"] != "make" and system("which make > /dev/null")
  system("cd #{top.to_s.dump} && make > /dev/null") or exit(1)
end

$LOAD_PATH.unshift((top + "src" + "toolkit" + ".libs").to_s)
$LOAD_PATH.unshift((top + "src" + "manager" + ".libs").to_s)
$LOAD_PATH.unshift((top + "lib").to_s)

$LOAD_PATH.unshift(base.to_s)
require 'milter-test-utils'

require 'milter/manager'

ARGV.unshift("--priority-mode")
exit Test::Unit::AutoRunner.run(true, base)
