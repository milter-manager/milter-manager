# Copyright (C) 2008-2011  Kouhei Sutou <kou@clear-code.com>
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

class TestClientContext < Test::Unit::TestCase
  include MilterTestUtils

  def setup
    @context = Milter::ClientContext.new
    @context.event_loop = Milter::GLibEventLoop.new
  end

  def test_feed
    received_option = nil
    @context.signal_connect("negotiate") do |_, option|
      received_option = option
      Milter::STATUS_CONTINUE
    end
    encoder = Milter::CommandEncoder.new
    option = Milter::Option.new
    @context.feed(encoder.encode_negotiate(option))
    assert_equal(option, received_option)
  end

  def test_progress
    assert_true(@context.progress)
  end

  def test_add_header
    @context.option = Milter::Option.new
    @context.option.add_action(Milter::ACTION_ADD_HEADERS)
    @context.state = Milter::ClientContext::STATE_END_OF_MESSAGE
    assert_nothing_raised do
      @context.add_header("foo", "bar")
    end
  end

  def test_insert_header
    @context.option = Milter::Option.new
    @context.option.add_action(Milter::ACTION_ADD_HEADERS)
    @context.state = Milter::ClientContext::STATE_END_OF_MESSAGE
    assert_nothing_raised do
      @context.insert_header(0, "X-Tag", "Ruby")
    end
  end

  def test_change_header
    @context.option = Milter::Option.new
    @context.option.add_action(Milter::ACTION_CHANGE_HEADERS)
    @context.state = Milter::ClientContext::STATE_END_OF_MESSAGE
    assert_nothing_raised do
      @context.change_header("X-Tag", 1, "Ruby")
    end
  end

  def test_delete_header
    @context.option = Milter::Option.new
    @context.option.add_action(Milter::ACTION_CHANGE_HEADERS)
    @context.state = Milter::ClientContext::STATE_END_OF_MESSAGE
    assert_nothing_raised do
      @context.delete_header("X-Tag", 1)
    end
  end

  def test_replace_body
    @context.option = Milter::Option.new
    @context.option.add_action(Milter::ACTION_CHANGE_BODY)
    @context.state = Milter::ClientContext::STATE_END_OF_MESSAGE
    assert_nothing_raised do
      @context.replace_body("Hello")
    end
  end

  def test_change_from
    @context.option = Milter::Option.new
    @context.option.add_action(Milter::ACTION_CHANGE_ENVELOPE_FROM)
    @context.state = Milter::ClientContext::STATE_END_OF_MESSAGE
    assert_nothing_raised do
      @context.change_from("<kou@clear-code.com>")
    end
  end

  def test_add_recipient
    @context.option = Milter::Option.new
    @context.option.add_action(Milter::ACTION_ADD_ENVELOPE_RECIPIENT)
    @context.state = Milter::ClientContext::STATE_END_OF_MESSAGE
    assert_nothing_raised do
      @context.add_recipient("<kou@clear-code.com>")
    end
  end

  def test_delete_recipient
    @context.option = Milter::Option.new
    @context.option.add_action(Milter::ACTION_DELETE_ENVELOPE_RECIPIENT)
    @context.state = Milter::ClientContext::STATE_END_OF_MESSAGE
    assert_nothing_raised do
      @context.delete_recipient("<kou@clear-code.com>")
    end
  end

  def test_set_reply
    assert_nil(@context.format_reply)
    @context.set_reply(451, "4.7.0", "DNS timeout")
    assert_equal("451 4.7.0 DNS timeout", @context.format_reply)
  end

  def test_n_processing_sessions
    assert_equal(0, @context.n_processing_sessions)
  end

  def test_packet_buffer_size
    assert_equal(Milter::ClientContext::DEFAULT_PACKET_BUFFER_SIZE,
                 @context.packet_buffer_size)
    @context.packet_buffer_size = 29
    assert_equal(29, @context.packet_buffer_size)
  end
end
