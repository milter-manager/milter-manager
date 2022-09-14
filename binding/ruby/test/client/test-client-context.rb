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

  def test_mail_transaction_shelf
    assert_equal({}, @context.mail_transaction_shelf)
    @context.set_mail_transaction_shelf_value("key1", "value1")
    @context.set_mail_transaction_shelf_value("key2", "value2")
    @context.set_mail_transaction_shelf_value("key3", "value3")
    assert_equal("value1", @context.get_mail_transaction_shelf_value("key1"))
    assert_equal("value2", @context.get_mail_transaction_shelf_value("key2"))
    assert_equal("value3", @context.get_mail_transaction_shelf_value("key3"))
    assert_equal(["key1", "key2", "key3"], @context.mail_transaction_shelf.keys.sort)
    assert_equal(["value1", "value2", "value3"], @context.mail_transaction_shelf.values.sort)
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
      fqdn.force_encoding(Encoding::UTF_8)
      @context.signal_emit("helo", fqdn)
      assert_equal([fqdn, Encoding::ASCII_8BIT],
                   [received_fqdn, received_fqdn.encoding])
    end

    def test_envelope_from
      received_from = nil
      @context.signal_connect("envelope-from") do |_, from|
        received_from = from
        Milter::STATUS_CONTINUE
      end

      from = "from@example.com"
      from.force_encoding(Encoding::UTF_8)
      @context.signal_emit("envelope-from", from)
      assert_equal([from, Encoding::ASCII_8BIT],
                   [received_from, received_from.encoding])
    end

    def test_envelope_recipient
      received_to = nil
      @context.signal_connect("envelope-recipient") do |_, to|
        received_to = to
        Milter::STATUS_CONTINUE
      end

      to = "to@example.com"
      to.force_encoding(Encoding::UTF_8)
      @context.signal_emit("envelope-recipient", to)
      assert_equal([to, Encoding::ASCII_8BIT],
                   [received_to, received_to.encoding])
    end

    def test_unknown
      received_command = nil
      @context.signal_connect("unknown") do |_, command|
        received_command = command
        Milter::STATUS_CONTINUE
      end

      command = "UNKNOWN COMMAND WITH ARGUMENT"
      command.force_encoding(Encoding::UTF_8)
      @context.signal_emit("unknown", command)
      assert_equal([command, Encoding::ASCII_8BIT],
                   [received_command, received_command.encoding])
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
      name.force_encoding(Encoding::UTF_8)
      value = "This is test subject."
      value.force_encoding(Encoding::UTF_8)
      @context.signal_emit("header", name ,value)
      assert_equal([name, Encoding::ASCII_8BIT,
                     value, Encoding::ASCII_8BIT],
                   [received_name, received_name.encoding,
                     received_value, received_value.encoding])
    end

    def test_body_bytes
      received_chunk = nil
      @context.signal_connect("body-bytes") do |_, chunk|
        received_chunk = chunk
        Milter::STATUS_CONTINUE
      end

      chunk = GLib::Bytes.new("XXX\n\0YYY\n".freeze)
      @context.signal_emit("body-bytes", chunk)
      assert_equal(chunk.to_s, received_chunk.to_s)
    end
  end
end
