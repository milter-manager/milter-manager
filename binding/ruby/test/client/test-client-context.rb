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
    assert_equal(0, @context.packet_buffer_size)
    @context.packet_buffer_size = 4096
    assert_equal(4096, @context.packet_buffer_size)
  end

  class TestSignal < self
    def test_helo
      received_fqdn = nil
      @context.signal_connect("helo") do |_, fqdn|
        received_fqdn = fqdn
        Milter::STATUS_CONTINUE
      end

      fqdn = "delian"
      if fqdn.respond_to?(:force_encoding)
        fqdn.force_encoding(Encoding::UTF_8)
      end
      @context.signal_emit("helo", fqdn)
      if received_fqdn.respond_to?(:encoding)
        assert_equal([fqdn, Encoding::ASCII_8BIT],
                     [received_fqdn, received_fqdn.encoding])
      else
        assert_equal(fqdn, received_fqdn)
      end
    end

    def test_envelope_from
      received_from = nil
      @context.signal_connect("envelope-from") do |_, from|
        received_from = from
        Milter::STATUS_CONTINUE
      end

      from = "from@example.com"
      if from.respond_to?(:force_encoding)
        from.force_encoding(Encoding::UTF_8)
      end
      @context.signal_emit("envelope-from", from)
      if received_from.respond_to?(:encoding)
        assert_equal([from, Encoding::ASCII_8BIT],
                     [received_from, received_from.encoding])
      else
        assert_equal(from, received_from)
      end
    end

    def test_envelope_recipient
      received_to = nil
      @context.signal_connect("envelope-recipient") do |_, to|
        received_to = to
        Milter::STATUS_CONTINUE
      end

      to = "to@example.com"
      if to.respond_to?(:force_encoding)
        to.force_encoding(Encoding::UTF_8)
      end
      @context.signal_emit("envelope-recipient", to)
      if received_to.respond_to?(:encoding)
        assert_equal([to, Encoding::ASCII_8BIT],
                     [received_to, received_to.encoding])
      else
        assert_equal(to, received_to)
      end
    end

    def test_unknown
      received_command = nil
      @context.signal_connect("unknown") do |_, command|
        received_command = command
        Milter::STATUS_CONTINUE
      end

      command = "UNKNOWN COMMAND WITH ARGUMENT"
      if command.respond_to?(:force_encoding)
        command.force_encoding(Encoding::UTF_8)
      end
      @context.signal_emit("unknown", command)
      if received_command.respond_to?(:encoding)
        assert_equal([command, Encoding::ASCII_8BIT],
                     [received_command, received_command.encoding])
      else
        assert_equal(command, received_command)
      end
    end

    def test_header
      received_name = nil
      received_value = nil
      @context.signal_connect("header") do |_, name, value|
        received_name = name
        received_value = value
        Milter::STATUS_CONTINUE
      end

      name = "Subject"
      if name.respond_to?(:force_encoding)
        name.force_encoding(Encoding::UTF_8)
      end
      value = "This is test subject."
      if value.respond_to?(:force_encoding)
        value.force_encoding(Encoding::UTF_8)
      end
      @context.signal_emit("header", name ,value)
      if received_name.respond_to?(:encoding)
        assert_equal([name, Encoding::ASCII_8BIT,
                      value, Encoding::ASCII_8BIT],
                     [received_name, received_name.encoding,
                      received_value, received_value.encoding])
      else
        assert_equal([name, value], [received_name, received_value])
      end
    end

    def test_body
      received_chunk = nil
      @context.signal_connect("body") do |_, chunk|
        received_chunk = chunk
        Milter::STATUS_CONTINUE
      end

      chunk = "XXX\nYYY\n"
      if chunk.respond_to?(:force_encoding)
        chunk.force_encoding(Encoding::UTF_8)
      end
      @context.signal_emit("body", chunk, chunk.bytesize)
      if received_chunk.respond_to?(:encoding)
        assert_equal([chunk, Encoding::ASCII_8BIT],
                     [received_chunk, received_chunk.encoding])
      else
        assert_equal(chunk, received_chunk)
      end
    end
  end
end
