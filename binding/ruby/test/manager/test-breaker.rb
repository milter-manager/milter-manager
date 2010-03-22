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

class TestBreaker < Test::Unit::TestCase
  include MilterTestUtils

  def test_stressing_n_connections
    assert_not_stressing(10, 9)
    assert_stressing(10, 10)
    assert_stressing(10, 11)
  end

  def test_stressing_n_connections_no_threshold
    assert_not_stressing(0, 0)
    assert_not_stressing(0, 100)
  end

  def test_detect_postfix_threshold_n_connections
    omit("TODO: WRITE ME")
  end

  private
  def breaker(threshold_n_connections=nil)
    _breaker = Milter::Manager::Breaker.new
    _breaker.threshold_n_connections = threshold_n_connections
    _breaker
  end

  def assert_stressing(threshold_n_connections, n_processing_sessions)
    _breaker = breaker(threshold_n_connections)
    assert_true(_breaker.stressing?(context(n_processing_sessions)),
                [_breaker, n_processing_sessions].inspect)
  end

  def assert_not_stressing(threshold_n_connections, n_processing_sessions)
    _breaker = breaker(threshold_n_connections)
    assert_false(_breaker.stressing?(context(n_processing_sessions)),
                 [_breaker, n_processing_sessions].inspect)
  end

  def context(n_processing_sessions)
    _context = OpenStruct.new
    _context.n_processing_sessions = n_processing_sessions
    _context
  end
end
