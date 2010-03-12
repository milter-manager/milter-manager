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

class TestLocalAddressMatcher < Test::Unit::TestCase
  def setup
    @matcher = Milter::Manager::LocalAddressMatcher.new
  end

  def test_default_ipv4_local_address
    assert_local_address(ipv4("192.168.1.10"))
  end

  def test_default_ipv6_local_address
    assert_local_address(ipv6("::1"))
  end

  def test_unix
    assert_local_address(unix("/tmp/milter-manager.sock"))
  end

  def test_unknown
    assert_not_local_address(unknown)
  end

  def test_add_ipv4
    @matcher.add("160.29.167.0/24")
    assert_local_address(ipv4("160.29.167.10"))
    assert_not_local_address(ipv4("160.29.168.10"))
    assert_not_local_address(unknown)
  end

  def test_add_ipv6
    @matcher.add("2001:2f8:c2:201::0/64")
    assert_local_address(ipv6("2001:2f8:c2:201::fff0"))
    assert_not_local_address(ipv6("2001:2f8:c2:202::fff0"))
    assert_not_local_address(unknown)
  end

  private
  def ipv4(address, port=2929)
    Milter::SocketAddress::IPv4.new(address, port)
  end

  def ipv6(address, port=2929)
    Milter::SocketAddress::IPv6.new(address, port)
  end

  def unix(path)
    Milter::SocketAddress::Unix.new(path)
  end

  def unknown
    Milter::SocketAddress::Unknown.new
  end

  def assert_local_address(address)
    assert_true(@matcher.local_address?(address), address)
  end

  def assert_not_local_address(address)
    assert_false(@matcher.local_address?(address), address)
  end
end
