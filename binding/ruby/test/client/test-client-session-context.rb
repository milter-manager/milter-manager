# Copyright (C) 2016  Kouhei Sutou <kou@clear-code.com>
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

class TestClientSessionContext < Test::Unit::TestCase
  include MilterTestUtils

  def setup
    @client = Milter::Client.new
    @context = Milter::ClientContext.new(@client)
    @context.event_loop = Milter::GLibEventLoop.new
    @session_context = Milter::ClientSessionContext.new(@context)
  end

  class TestStatus < self
    def test_snake_case
      @context.status = :temporary_failure
      assert_equal(Milter::Status::TEMPORARY_FAILURE, @context.status)
    end

    def test_hyphen
      @context.status = "temporary-failure"
      assert_equal(Milter::Status::TEMPORARY_FAILURE, @context.status)
    end
  end
end
