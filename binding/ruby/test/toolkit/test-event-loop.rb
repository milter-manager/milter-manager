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

class TestEventLoop < Test::Unit::TestCase
  include MilterTestUtils
  include MilterEventLoopTestUtils

  def setup
    @loop = create_event_loop
    @tag = nil
  end

  def teardown
    @loop.remove(@tag) if @tag
  end

  def test_timeout
    timeouted = false
    interval = 0.001
    @tag = @loop.add_timeout(interval) do
      timeouted = true
      false
    end
    sleep(interval)
    assert_true(@loop.iterate(:may_block => false))
    assert_true(timeouted)
  end

  def test_timeout_not_timeouted
    timeouted = false
    interval = 1
    @tag = @loop.add_timeout(interval) do
      timeouted = true
      false
    end
    assert_false(@loop.iterate(:may_block => false))
    assert_false(timeouted)
  end

  def test_timeout_without_block
    assert_raise(ArgumentError.new("timeout block is missing")) do
      @loop.add_timeout(1)
    end
  end

  def test_idle
    idled = false
    @tag = @loop.add_idle do
      idled = true
      false
    end
    assert_true(@loop.iterate(:may_block => false))
    assert_true(idled)
  end

  def test_idle_without_block
    assert_raise(ArgumentError.new("idle block is missing")) do
      @loop.add_idle
    end
  end
end
