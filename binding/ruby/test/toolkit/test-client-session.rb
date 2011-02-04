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

class TestClientSession < Test::Unit::TestCase
  include MilterTestUtils

  def setup
    @context = Milter::ClientContext.new
    @context.event_loop = Milter::GLibEventLoop.new
    @session_context = Milter::ClientSessionContext.new(@context)
    @session = Milter::ClientSession.new(@session_context)
  end

  def test_add_header
    @context.option = Milter::Option.new
    @context.option.add_action(Milter::ACTION_ADD_HEADERS)
    @context.state = Milter::ClientContext::STATE_END_OF_MESSAGE
    assert_nothing_raised do
      @session.send(:add_header, "X-Tag", "spam")
    end
  end

  def test_insert_header
    @context.option = Milter::Option.new
    @context.option.add_action(Milter::ACTION_ADD_HEADERS)
    @context.state = Milter::ClientContext::STATE_END_OF_MESSAGE
    assert_nothing_raised do
      @session.send(:insert_header, 0, "X-Tag", "Ruby")
    end
  end

  def test_change_header
    @context.option = Milter::Option.new
    @context.option.add_action(Milter::ACTION_CHANGE_HEADERS)
    @context.state = Milter::ClientContext::STATE_END_OF_MESSAGE
    assert_nothing_raised do
      @session.send(:change_header, "X-Tag", 1, "Ruby")
    end
  end

  def test_delete_header
    @context.option = Milter::Option.new
    @context.option.add_action(Milter::ACTION_CHANGE_HEADERS)
    @context.state = Milter::ClientContext::STATE_END_OF_MESSAGE
    assert_nothing_raised do
      @session.send(:delete_header, "X-Tag", 1)
    end
  end

  def test_replace_body
    @context.option = Milter::Option.new
    @context.option.add_action(Milter::ACTION_CHANGE_BODY)
    @context.state = Milter::ClientContext::STATE_END_OF_MESSAGE
    assert_nothing_raised do
      @session.send(:replace_body, "Hello")
    end
  end

  def test_change_from
    @context.option = Milter::Option.new
    @context.option.add_action(Milter::ACTION_CHANGE_ENVELOPE_FROM)
    @context.state = Milter::ClientContext::STATE_END_OF_MESSAGE
    assert_nothing_raised do
      @session.send(:change_from, "<info@example.com>")
    end
  end

  def test_add_recipient
    @context.option = Milter::Option.new
    @context.option.add_action(Milter::ACTION_ADD_ENVELOPE_RECIPIENT)
    @context.state = Milter::ClientContext::STATE_END_OF_MESSAGE
    assert_nothing_raised do
      @session.send(:add_recipient, "<webmaster@example.com>")
    end
  end

  def test_delete_recipient
    @context.option = Milter::Option.new
    @context.option.add_action(Milter::ACTION_DELETE_ENVELOPE_RECIPIENT)
    @context.state = Milter::ClientContext::STATE_END_OF_MESSAGE
    assert_nothing_raised do
      @session.send(:delete_recipient, "<webmaster@example.com>")
    end
  end

  def test_accept
    assert_equal(Milter::Status::DEFAULT, @session_context.status)
    @session.send(:accept)
    assert_equal(Milter::Status::ACCEPT, @session_context.status)
  end

  def test_reject
    assert_equal(Milter::Status::DEFAULT, @session_context.status)
    @session.send(:reject)
    assert_equal(Milter::Status::REJECT, @session_context.status)
  end

  def test_temporary_failure
    assert_equal(Milter::Status::DEFAULT, @session_context.status)
    @session.send(:temporary_failure)
    assert_equal(Milter::Status::TEMPORARY_FAILURE, @session_context.status)
  end

  def test_quarantine
    assert_equal(Milter::Status::DEFAULT, @session_context.status)
    @session.send(:quarantine, "a virus is detected.")
    assert_equal("a virus is detected.", @context.quarantine_reason)
    assert_equal(Milter::Status::ACCEPT, @session_context.status)
  end

  def test_delay_response
    assert_equal(Milter::Status::DEFAULT, @session_context.status)
    @session.send(:delay_response)
    assert_equal(Milter::Status::PROGRESS, @session_context.status)
  end

  def test_progress
    @context.option = Milter::Option.new
    @context.state = Milter::ClientContext::STATE_END_OF_MESSAGE
    assert_nothing_raised do
      @session.send(:progress)
    end
  end
end
