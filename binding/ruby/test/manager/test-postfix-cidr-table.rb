# Copyright (C) 2010  Kouhei Sutou <kou@clear-code.com>
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

class TestPostfixCIDRTable < Test::Unit::TestCase

  include MilterParseTestUtils

  def setup
    @table = Milter::Manager::PostfixCIDRTable.new
  end

  def test_empty
    assert_nil(@table.find(ipv4("127.0.0.1")))
  end

  def test_exact_match_ipv4
    @table.parse(create_input(<<-EOC))
# Rule order matters. Put more specific whitelist entries
# before more general blacklist entries.
192.168.1.1             OK
192.168.1.0/24          REJECT
EOC
    assert_equal("OK", @table.find(ipv4("192.168.1.1")))
  end

  def test_network_match_ipv4
    @table.parse(create_input(<<-EOC))
# Rule order matters. Put more specific whitelist entries
# before more general blacklist entries.
192.168.1.1             OK
192.168.1.0/24          REJECT
EOC
    assert_equal("REJECT", @table.find(ipv4("192.168.1.29")))
  end

  def test_all_match_ipv4
    @table.parse(create_input(<<-EOC))
# Rule order matters. Put more specific whitelist entries
# before more general blacklist entries.
0.0.0.0/0               OK
192.168.1.1             REJECT
192.168.1.0/24          REJECT
EOC
    assert_equal("OK", @table.find(ipv4("192.168.1.1")))
  end

  def test_exact_match_ipv6
    @table.parse(create_input(<<-EOC))
# Rule order matters. Put more specific whitelist entries
# before more general blacklist entries.
2001:2f8:c2:201::fff0 OK
2001:2f8:c2:201::0/64 REJECT
EOC
    assert_equal("OK", @table.find(ipv6("2001:2f8:c2:201::fff0")))
  end

  def test_network_match_ipv6
    @table.parse(create_input(<<-EOC))
# Rule order matters. Put more specific whitelist entries
# before more general blacklist entries.
2001:2f8:c2:201::fff0 OK
2001:2f8:c2:201::0/64 REJECT
EOC
    assert_equal("REJECT", @table.find(ipv4("2001:2f8:c2:201::1")))
  end

  def test_all_match_ipv6
    @table.parse(create_input(<<-EOC))
# Rule order matters. Put more specific whitelist entries
# before more general blacklist entries.
::/0                  OK
2001:2f8:c2:201::fff0 REJECT
2001:2f8:c2:201::0/64 REJECT
EOC
    assert_equal("OK", @table.find(ipv6("2001:2f8:c2:201::fff0")))
  end

  def test_logical_line
    @table.parse(create_input(<<-EOC))
192.168.1.1
  OK
EOC
    assert_equal("OK", @table.find(ipv4("192.168.1.1")))
  end

  def test_logical_line_with_empty_line
    @table.parse(create_input(<<-EOC))
192.168.1.1

  OK
EOC
    assert_equal("OK", @table.find(ipv4("192.168.1.1")))
  end

  def test_logical_line_with_comment_line
    @table.parse(create_input(<<-EOC))
192.168.1.1
# comment line
  OK
EOC
    assert_equal("OK", @table.find(ipv4("192.168.1.1")))
  end

  def test_invalid_address_ipv4
    error = invalid_value_error("192.168.1",
                                "invalid address: 192.168.1",
                                "192.168.1 OK",
                                nil,
                                1)
    pp error
    assert_raise(error) do
      @table.parse(create_input(<<-EOC))
192.168.1 OK
EOC
    end
    assert_nil(@table.find(ipv4("192.168.1.1")))
  end

  def test_invalid_address_ipv6
    error = invalid_value_error("f:",
                                "invalid address: f:",
                                "f: OK",
                                nil,
                                1)
    assert_raise(error) do
      @table.parse(create_input(<<-EOC))
f: OK
EOC
    end
    assert_nil(@table.find(ipv4("192.168.1.1")))
  end

  def test_invalid_format
    error = invalid_format_error("x:: OK",
                                 nil,
                                 1)
    assert_raise(error) do
      @table.parse(create_input(<<-EOC))
x:: OK
EOC
    end
    assert_nil(@table.find(ipv4("192.168.1.1")))
  end

  def test_result_with_spaces
    @table.parse(create_input(<<-EOC))
192.168.1.1 550 mail fromo black is rejected
EOC
    assert_equal("550 mail fromo black is rejected",
                 @table.find(ipv4("192.168.1.1")))
  end

  private
  def ipv4(address, port=2929)
    Milter::SocketAddress::IPv4.new(address, port)
  end

  def ipv6(address, port=2929)
    Milter::SocketAddress::IPv6.new(address, port)
  end

  def invalid_value_error(value, detail, line, path, line_no)
    Milter::Manager::ConditionTable::InvalidValueError.new(value,
                                                           detail,
                                                           line,
                                                           path,
                                                           line_no)
  end

  def invalid_format_error(line, path, line_no)
    Milter::Manager::ConditionTable::InvalidFormatError.new(line, path, line_no)
  end

end
