# Copyright (C) 2011  Nobuyoshi Nakada <nakada@clear-code.com>
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

class TestGString < Test::Unit::TestCase
  def setup
    @gstring = Milter::Manager::GString.new
  end

  def test_puts
    @gstring.puts("foo")
    assert_equal("foo\n", @gstring.to_s)
  end

  def test_puts_with_newline
    @gstring.puts("foo\n")
    assert_equal("foo\n", @gstring.to_s)
  end

  def test_print
    @gstring.print("foo")
    assert_equal("foo", @gstring.to_s)
  end

  def test_write
    @gstring.write("foo")
    assert_equal("foo", @gstring.to_s)
  end

  def test_argument_error
    assert_raises(TypeError) do
      @gstring.write(false)
    end
    assert_equal("", @gstring.to_s)
  end
end
