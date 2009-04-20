# Copyright (C) 2008-2009  Kouhei Sutou <kou@clear-code.com>
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

  def test_ipv4_accessor
    ipv4_address = ipv4("127.0.0.1", 2929)
    assert_equal(["127.0.0.1", 2929],
                 [ipv4_address.address, ipv4_address.port])
  end

  def test_ipv4_to_s
    address = ipv4("127.0.0.1", 2929)

    assert_equal("inet:2929@[127.0.0.1]", address.to_s)
  end

  def test_ipv4_equal
    address1 = ipv4("127.0.0.1", 2929)
    address2 = ipv4("127.0.0.1", 9292)
    address3 = ipv4("127.0.0.1", 2929)

    assert_equal(address1, address1)
    assert_not_equal(address1, address2)
    assert_equal(address1, address1)
  end

  def test_ipv4_local?
    assert_true(ipv4("127.0.0.1", 2929).local?)
    assert_true(ipv4("127.0.0.2", 2929).local?)

    assert_false(ipv4("9.255.255.254", 2929).local?)
    assert_true(ipv4("10.0.0.1", 2929).local?)
    assert_true(ipv4("10.255.255.254", 2929).local?)
    assert_false(ipv4("11.0.0.1", 2929).local?)

    assert_false(ipv4("172.15.255.254", 2929).local?)
    assert_true(ipv4("172.16.0.1", 2929).local?)
    assert_true(ipv4("172.31.255.254", 2929).local?)
    assert_false(ipv4("172.32.0.1", 2929).local?)

    assert_false(ipv4("192.167.255.254", 2929).local?)
    assert_true(ipv4("192.168.0.1", 2929).local?)
    assert_true(ipv4("192.168.255.254", 2929).local?)
    assert_false(ipv4("192.169.0.1", 2929).local?)
  end

  def test_ipv6_accessor
    ipv6_address = ipv6("::1", 2929)
    assert_equal(["::1", 2929],
                 [ipv6_address.address, ipv6_address.port])
  end

  def test_ipv6_to_s
    address = ipv6("::1", 2929)

    assert_equal("inet6:2929@[::1]", address.to_s)
  end

  def test_ipv6_equal
    address1 = ipv6("::1", 2929)
    address2 = ipv6("::1", 9292)
    address3 = ipv6("::1", 2929)

    assert_equal(address1, address1)
    assert_not_equal(address1, address2)
    assert_equal(address1, address1)
  end

  def test_ipv6_local?
    assert_true(ipv6("::1", 2929).local?)
    assert_false(ipv6("::2", 2929).local?)

    assert_true(ipv6("fe80::1", 2929).local?)
    assert_false(ipv6("fe81::1", 2929).local?)
  end

  def test_unix_accessor
    unix_address = unix("/tmp/local.sock")
    assert_equal(["/tmp/local.sock"],
                 [unix_address.path])
  end

  def test_unix_to_s
    address = unix("/tmp/local.sock")

    assert_equal("unix:/tmp/local.sock", address.to_s)
  end

  def test_unix_equal
    address1 = unix("/tmp/local.sock")
    address2 = unix("/tmp/other.sock")
    address3 = unix("/tmp/local.sock")

    assert_equal(address1, address1)
    assert_not_equal(address1, address2)
    assert_equal(address1, address3)
  end

  def test_unix_local?
    assert_true(unix("/tmp/local.sock").local?)
  end

  def test_unknown_to_s
    assert_equal("unknown", unknown.to_s)
  end

  def test_unknown_equal
    address = unknown

    assert_equal(address, address)
    assert_equal(address, unknown)
  end

  def test_unknown_local?
    assert_false(unknown.local?)
  end

  private
  def ipv4(address, port)
    Milter::SocketAddress::IPv4.new(address, port)
  end

  def ipv6(address, port)
    Milter::SocketAddress::IPv6.new(address, port)
  end

  def unix(path)
    Milter::SocketAddress::Unix.new(path)
  end

  def unknown
    Milter::SocketAddress::Unknown.new
  end
end
