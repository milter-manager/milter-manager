# Copyright (C) 2009  Yuto Hayamizu <y.hayamizu@gmail.com>
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

class TestClient < Test::Unit::TestCase
  include MilterTestUtils

  def setup
    @client = Milter::Client.new
  end

  def test_connection_spec
    @client.connection_spec = "inet:12345"
    assert_equal("inet:12345", @client.connection_spec)
  end

  def test_effective_user
    @client.effective_user = "nobody"
    assert_equal("nobody", @client.effective_user)
  end

  def test_effective_group
    @client.effective_group = "nogroup"
    assert_equal("nogroup", @client.effective_group)
  end

  def test_unix_socket_group
    @client.unix_socket_group = "nogroup"
    assert_equal("nogroup", @client.unix_socket_group)
  end
end
