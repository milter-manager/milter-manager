# Copyright (C) 2011  Kouhei Sutou <kou@clear-code.com>
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

class TestStatus < Test::Unit::TestCase
  include MilterTestUtils

  def test_compare_small
    assert_operator(Milter::Status::ACCEPT <=> Milter::Status::REJECT,
                    :<,
                    0)
  end

  def test_compare_equal
    assert_equal(0, Milter::Status::ACCEPT <=> Milter::Status::ACCEPT)
  end

  def test_compare_large
    assert_operator(Milter::Status::REJECT <=> Milter::Status::ACCEPT,
                    :>,
                    0)
  end
end
