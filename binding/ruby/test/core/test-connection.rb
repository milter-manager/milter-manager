# Copyright (C) 2008  Kouhei Sutou <kou@cozmixng.org>
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

class TestConnection < Test::Unit::TestCase
  include MilterTestUtils

  def test_parse_spec_ipv4
    assert_equal(Milter::SocketAddress::IPv4.new("127.0.0.1", 9999),
                 Milter::Connection.parse_spec("inet:9999@127.0.0.1"))
  end

  def test_parse_spec_ipv6
    assert_equal(Milter::SocketAddress::IPv6.new("::1", 9999),
                 Milter::Connection.parse_spec("inet6:9999@::1"))
  end

  def test_parse_spec_unix
    assert_equal(Milter::SocketAddress::Unix.new("/tmp/socket"),
                 Milter::Connection.parse_spec("unix:/tmp/socket"))
  end
end
