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

class TestSocketAddresss < Test::Unit::TestCase
  include MilterTestUtils

  def test_ipv4_to_s
    address = Milter::SocketAddress::IPv4.new("127.0.0.1", 2929)

    assert_equal("inet:2929@127.0.0.1", address.to_s)
  end

  def test_ipv4_equal
    address1 = Milter::SocketAddress::IPv4.new("127.0.0.1", 2929)
    address2 = Milter::SocketAddress::IPv4.new("127.0.0.1", 9292)
    address3 = Milter::SocketAddress::IPv4.new("127.0.0.1", 2929)

    assert_equal(address1, address1)
    assert_not_equal(address1, address2)
    assert_equal(address1, address1)
  end

  def test_ipv6_to_s
    address = Milter::SocketAddress::IPv6.new("::1", 2929)

    assert_equal("inet6:2929@::1", address.to_s)
  end

  def test_ipv6_equal
    address1 = Milter::SocketAddress::IPv6.new("::1", 2929)
    address2 = Milter::SocketAddress::IPv6.new("::1", 9292)
    address3 = Milter::SocketAddress::IPv6.new("::1", 2929)

    assert_equal(address1, address1)
    assert_not_equal(address1, address2)
    assert_equal(address1, address1)
  end

  def test_unix_to_s
    address = Milter::SocketAddress::Unix.new("/tmp/local.sock")

    assert_equal("unix:/tmp/local.sock", address.to_s)
  end

  def test_unix_equal
    address1 = Milter::SocketAddress::Unix.new("/tmp/local.sock")
    address2 = Milter::SocketAddress::Unix.new("/tmp/other.sock")
    address3 = Milter::SocketAddress::Unix.new("/tmp/local.sock")

    assert_equal(address1, address1)
    assert_not_equal(address1, address2)
    assert_equal(address1, address1)
  end
end
