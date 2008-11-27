#!/usr/bin/env ruby

require 'pathname'

base = Pathname.new(__FILE__).dirname.expand_path
top = (base + "..").expand_path
test_unit_dir = top + "test-unit" + "lib"

$LOAD_PATH.unshift(test_unit_dir.to_s)
require 'test/unit'

if system("which make > /dev/null")
  system("cd #{top.to_s.dump} && make > /dev/null") or exit(1)
end

$LOAD_PATH.unshift((top + "src").to_s)
$LOAD_PATH.unshift((top + "src" + "lib").to_s)

$LOAD_PATH.unshift(base.to_s)
require 'milter-test-utils'

require 'milter'

ARGV.unshift("--priority-mode")
exit Test::Unit::AutoRunner.run(true, base)
