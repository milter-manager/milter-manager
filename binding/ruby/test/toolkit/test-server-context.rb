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

class TestServerContext < Test::Unit::TestCase
  include MilterTestUtils

  def setup
    @context = Milter::ServerContext.new
  end

  def test_connection_spec
    assert_nothing_raised do
      @context.connection_spec = "inet:9999@localhost"
    end
  end

  def test_connection_spec_with_invalid_format
    exception = Milter::ConnectionError.new("spec doesn't have colon: <inet>")
    assert_raise(exception) do
      @context.connection_spec = "inet"
    end
  end

  def test_establish_connection
    TCPServer.open(9999) do
      @context.connection_spec = "inet:9999@localhost"
      assert_nothing_raised do
        @context.establish_connection
      end
    end
  end

  def test_establish_connection_without_spec
    assert_raise(Milter::ServerContextError.new("No spec set yet.")) do
      @context.establish_connection
    end
  end

  def test_check_connect_pass
    assert_check_pass do
      received_host = nil
      received_address = nil
      @context.signal_connect("check-connect") do |_, host, address|
        received_host = host
        received_address = address
        true
      end

      host = "localhost"
      address = Milter::SocketAddress::IPv4.new("127.0.0.1", 9999)
      @context.connect(host, address)

      assert_equal(host, received_host)
      assert_equal(address, received_address)
    end
  end

  def test_check_connect_not_pass
    assert_check_not_pass do
      @context.signal_connect("check-connect") do
        false
      end

      host = "localhost"
      address = Milter::SocketAddress::IPv4.new("127.0.0.1", 9999)
      @context.connect(host, address)
    end
  end

  def test_check_helo_pass
    assert_check_pass do
      received_fqdn = nil
      @context.signal_connect("check-helo") do |_, fqdn|
        received_fqdn = fqdn
        true
      end

      fqdn = "mx.local.net"
      @context.helo(fqdn)

      assert_equal(fqdn, received_fqdn)
    end
  end

  def test_check_helo_not_pass
    assert_check_not_pass do
      @context.signal_connect("check-helo") do
        false
      end

      @context.helo("mx.local.net")
    end
  end

  def test_check_envelope_from_pass
    assert_check_pass do
      received_from = nil
      @context.signal_connect("check-envelope-from") do |_, from|
        received_from = from
        true
      end

      from = "kou+sender@cozmixng.org"
      @context.envelope_from(from)

      assert_equal(from, received_from)
    end
  end

  def test_check_envelope_from_not_pass
    assert_check_not_pass do
      @context.signal_connect("check-envelope-from") do
        false
      end

      @context.envelope_from("kou+sender@cozmixng.org")
    end
  end

  def test_check_envelope_recipient_pass
    assert_check_pass do
      received_recipient = nil
      @context.signal_connect("check-envelope-recipient") do |_, recipient|
        received_recipient = recipient
        true
      end

      recipient = "kou+receiver@cozmixng.org"
      @context.envelope_recipient(recipient)

      assert_equal(recipient, received_recipient)
    end
  end

  def test_check_envelope_recipient_not_pass
    assert_check_not_pass do
      @context.signal_connect("check-envelope-recipient") do
        false
      end

      @context.envelope_recipient("kou+receiver@cozmixng.org")
    end
  end

  def test_check_header_pass
    assert_check_pass do
      received_name = nil
      received_value = nil
      @context.signal_connect("check-header") do |_, name, value|
        received_name = name
        received_value = value
        true
      end

      name = "X-HEADER"
      value = "header value"
      @context.header(name, value)

      assert_equal([name, value], [received_name, received_value])
    end
  end

  def test_check_header_not_pass
    assert_check_not_pass do
      @context.signal_connect("check-header") do
        false
      end

      @context.header("X-HEADER", "header value")
    end
  end

  def test_check_body_pass
    assert_check_pass do
      received_chunk = nil
      @context.signal_connect("check-body") do |_, chunk|
        received_chunk = chunk
        true
      end

      chunk = "Hi\n\nI'm here."
      @context.body(chunk)

      assert_equal(chunk, received_chunk)
    end
  end

  def test_check_body_not_pass
    assert_check_not_pass do
      @context.signal_connect("check-body") do
        false
      end

      @context.body("Hi,\n\nI'm there\n")
    end
  end

  def test_check_end_of_message_pass
    assert_check_pass do
      received_chunk = nil
      @context.signal_connect("check-end-of-message") do |_, chunk|
        received_chunk = chunk
        true
      end

      @context.end_of_message

      assert_nil(received_chunk)
    end
  end

  def test_check_end_of_message_pass_with_chunk
    assert_check_pass do
      received_chunk = nil
      @context.signal_connect("check-end-of-message") do |_, chunk|
        received_chunk = chunk
        true
      end

      chunk = "Hi\n\nI'm here."
      @context.end_of_message(chunk)

      assert_equal(chunk, received_chunk)
    end
  end

  def test_check_end_of_message_not_pass
    assert_check_not_pass do
      @context.signal_connect("check-end-of-message") do
        false
      end

      @context.end_of_message
    end
  end

  def test_check_end_of_message_not_pass_with_chunk
    assert_check_not_pass do
      @context.signal_connect("check-end-of-message") do
        false
      end

      @context.end_of_message("Hi,\n\nI'm there\n")
    end
  end

  private
  def connect
    TCPServer.open(9999) do
      @context.connection_spec = "inet:9999@localhost"
      @context.establish_connection

      @passed = false
      @context.signal_connect("passed") do
        @passed = true
      end

      yield
    end
  end

  def assert_check_pass
    connect do
      yield
      assert_true(@passed)
    end
  end

  def assert_check_not_pass
    connect do
      yield
      assert_false(@passed)
    end
  end
end
