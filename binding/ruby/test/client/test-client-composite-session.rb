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

class TestClientCompositeSession < Test::Unit::TestCase
  include MilterTestUtils

  class ConnectOnlySession < Milter::ClientSession
    attr_reader :host, :address
    def connect(host, address)
      @host = host
      @address = address
    end
  end

  class HeloOnlySession < Milter::ClientSession
    attr_reader :helo
    def helo(fqdn)
      @fqdn = fqdn
    end
  end

  def setup
    @context = Milter::ClientContext.new
    @context.event_loop = Milter::GLibEventLoop.new
    @session_context = Milter::ClientSessionContext.new(@context)
    @session_classes = [ConnectOnlySession, HeloOnlySession]
    @session = Milter::Client::CompositeSession.new(@session_context,
                                                    @session_classes)
  end

  def test_negotiate
    option = Milter::Option.new
    option.step = Milter::StepFlags::NO_MASK | Milter::StepFlags::YES_MASK
    macros_requests = Milter::MacrosRequests.new
    @session.negotiate(option, macros_requests)
    expected_step =
      (Milter::StepFlags::NO_MASK | Milter::StepFlags::YES_MASK) &
      ~(Milter::StepFlags::NO_REPLY_MASK) &
      ~(Milter::StepFlags::HEADER_VALUE_WITH_LEADING_SPACE) &
      ~(Milter::StepFlags::NO_CONNECT | Milter::StepFlags::NO_HELO)
    assert_equal({
                   :status => Milter::Status::NOT_CHANGE,
                   :step => expected_step,
                 },
                 {
                   :status => @context.status,
                   :step => option.step,
                 })
  end
end
