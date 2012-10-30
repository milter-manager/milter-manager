# Copyright (C) 2008  Kouhei Sutou <kou@cozmixng.org>
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

class TestCommandDecoder < Test::Unit::TestCase
  include MilterTestUtils

  def setup
    @encoder = Milter::CommandEncoder.new
    @decoder = Milter::CommandDecoder.new
  end

  def test_negotiate
    decoded_option = nil
    @decoder.signal_connect("negotiate") do |_, option|
      decoded_option = option
    end

    option = Milter::Option.new
    @decoder.decode(@encoder.encode_negotiate(option))
    @decoder.end_decode
    assert_equal(option, decoded_option)
  end

  def test_define_macro
    decoded_context = nil
    decoded_macros = nil
    @decoder.signal_connect("define-macro") do |_, context, macros|
      decoded_context = context
      decoded_macros = macros
    end

    context = Milter::COMMAND_CONNECT
    macros = {"i" => "69FDD42DF4A"}
    @decoder.decode(@encoder.encode_define_macro(context, macros))
    @decoder.end_decode
    assert_equal([context, macros],
                 [decoded_context, decoded_macros])
  end

  def test_connect
    decoded_host_name = nil
    decoded_address = nil
    @decoder.signal_connect("connect") do |_, host_name, address|
        decoded_host_name = host_name
        decoded_address = address
    end

    host_name = "mx.local.net"
    address = Milter::SocketAddress::IPv4.new("192.168.123.123", 9999)
    @decoder.decode(@encoder.encode_connect(host_name, address))
    @decoder.end_decode
    assert_equal([host_name, address],
                 [decoded_host_name, decoded_address])
  end

  def test_helo
    decoded_fqdn = nil
    @decoder.signal_connect("helo") do |_, fqdn|
      decoded_fqdn = fqdn
    end

    fqdn = "delian"
    @decoder.decode(@encoder.encode_helo(fqdn))
    @decoder.end_decode
    if decoded_fqdn.respond_to?(:encoding)
      assert_equal([fqdn, Encoding::ASCII_8BIT], [decoded_fqdn, decoded_fqdn.encoding])
    else
      assert_equal(fqdn, decoded_fqdn)
    end
  end

  def test_envelope_from
    decoded_from = nil
    @decoder.signal_connect("envelope-from") do |_, from|
      decoded_from = from
    end

    from = "kou+sender@cozmixng.org"
    @decoder.decode(@encoder.encode_envelope_from(from))
    @decoder.end_decode
    if decoded_from.respond_to?(:encoding)
      assert_equal([from, Encoding::ASCII_8BIT], [decoded_from, decoded_from.encoding])
    else
      assert_equal(from, decoded_from)
    end
  end

  def test_envelope_recipient
    decoded_to = nil
    @decoder.signal_connect("envelope-recipient") do |_, to|
      decoded_to = to
    end

    to = "kou+sender@cozmixng.org"
    @decoder.decode(@encoder.encode_envelope_recipient(to))
    @decoder.end_decode
    if decoded_to.respond_to?(:encoding)
      assert_equal([to, Encoding::ASCII_8BIT], [decoded_to, decoded_to.encoding])
    else
      assert_equal(to, decoded_to)
    end
  end

  def test_data
    data_received = false
    @decoder.signal_connect("data") do
      data_received = true
    end

    @decoder.decode(@encoder.encode_data)
    @decoder.end_decode
    assert_true(data_received)
  end

  def test_header
    decoded_name = nil
    decoded_value = nil
    @decoder.signal_connect("header") do |_, name, value|
      decoded_name = name
      decoded_value = value
    end

    name = "name"
    value = "value"
    @decoder.decode(@encoder.encode_header(name, value))
    @decoder.end_decode
    if name.respond_to?(:encoding)
      assert_equal([name, Encoding::ASCII_8BIT, value, Encoding::ASCII_8BIT],
                   [decoded_name, decoded_name.encoding, decoded_value, decoded_value.encoding])
    else
      assert_equal([name, value],
                   [decoded_name, decoded_value])
    end
  end

  def test_end_of_header
    end_of_header_received = false
    @decoder.signal_connect("end-of-header") do
      end_of_header_received = true
    end

    @decoder.decode(@encoder.encode_end_of_header)
    @decoder.end_decode
    assert_true(end_of_header_received)
  end

  def test_body
    decoded_chunk = nil
    @decoder.signal_connect("body") do |_, chunk|
      decoded_chunk = chunk
    end

    chunk = "XXX\nYYY"
    @decoder.decode(@encoder.encode_body(chunk)[0])
    @decoder.end_decode
    assert_equal(chunk, decoded_chunk)
  end

  def test_end_of_message
    end_of_message_received = false
    decoded_chunk = nil
    @decoder.signal_connect("end-of-message") do |_, chunk|
      end_of_message_received = true
      decoded_chunk = chunk
    end

    @decoder.decode(@encoder.encode_end_of_message)
    @decoder.end_decode
    assert_equal([true, nil],
                 [end_of_message_received, decoded_chunk])
  end

  def test_end_of_message_with_data
    end_of_message_received = false
    decoded_chunk = nil
    @decoder.signal_connect("end-of-message") do |_, chunk|
      end_of_message_received = true
      decoded_chunk = chunk
    end

    chunk = "XXX\nYYY"
    @decoder.decode(@encoder.encode_end_of_message(chunk))
    @decoder.end_decode
    assert_equal([true, chunk],
                 [end_of_message_received, decoded_chunk])
  end

  def test_abort
    abort_received = false
    @decoder.signal_connect("abort") do
      abort_received = true
    end

    @decoder.decode(@encoder.encode_abort)
    @decoder.end_decode
    assert_true(abort_received)
  end

  def test_quit
    quit_received = false
    @decoder.signal_connect("quit") do
      quit_received = true
    end

    @decoder.decode(@encoder.encode_quit)
    @decoder.end_decode
    assert_true(quit_received)
  end

  def test_unknown
    decoded_command = nil
    @decoder.signal_connect("unknown") do |_, command|
      decoded_command = command
    end

    command = "UNKNOWN COMMAND WITH ARGUMENT"
    @decoder.decode(@encoder.encode_unknown(command))
    @decoder.end_decode
    if decoded_command.respond_to?(:encoding)
      assert_equal([command, Encoding::ASCII_8BIT], [decoded_command, decoded_command.encoding])
    else
      assert_equal(command, decoded_command)
    end
  end
end
