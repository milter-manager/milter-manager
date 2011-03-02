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

class TestCommandEncoder < Test::Unit::TestCase
  include MilterEncoderTestUtils

  def setup
    @encoder = Milter::CommandEncoder.new
  end

  def test_negotiate
    option = Milter::Option.new
    assert_equal(packet("O", [2, 0, 0].pack("N3")),
                 @encoder.encode_negotiate(option))
  end

  def test_define_macro
    assert_equal(packet("D", "C" + "{XXX}\0YYY\0"),
                 @encoder.encode_define_macro(Milter::Command::CONNECT,
                                              {"XXX" => "YYY"}))
  end

  def test_connect
    ip_address = "192.168.123.123"
    port = 9999
    address = Milter::SocketAddress::IPv4.new(ip_address, port)
    assert_equal(packet("C",
                        "delian\0" + "4" + [port].pack("n") + "#{ip_address}\0"),
                 @encoder.encode_connect("delian", address))
  end

  def test_connect_with_string_address
    ip_address = "192.168.123.123"
    port = 9999
    address = Milter::SocketAddress::IPv4.new(ip_address, port)
    assert_equal(packet("C",
                        "delian\0" + "4" + [port].pack("n") + "#{ip_address}\0"),
                 @encoder.encode_connect("delian", address.pack))
  end

  def test_helo
    assert_equal(packet("H", "delian\0"),
                 @encoder.encode_helo("delian"))
  end

  def test_envelope_from
    assert_equal(packet("M", "kou+sender@cozmixng.org\0"),
                 @encoder.encode_envelope_from("kou+sender@cozmixng.org"))
  end

  def test_envelope_recipient
    assert_equal(packet("R", "kou+receiver@cozmixng.org\0"),
                 @encoder.encode_envelope_recipient("kou+receiver@cozmixng.org"))
  end

  def test_data
    assert_equal(packet("T"), @encoder.encode_data)
  end

  def test_header
    assert_equal(packet("L", "name\0value\0"),
                 @encoder.encode_header("name", "value"))
  end

  def test_end_of_header
    assert_equal(packet("N"), @encoder.encode_end_of_header)
  end

  def test_body
    content = "content"
    assert_equal([packet("B", content), content.size],
                 @encoder.encode_body(content))
  end

  def test_end_of_message
    assert_equal(packet("E"),
                 @encoder.encode_end_of_message)

    content = "content"
    assert_equal(packet("E", content),
                 @encoder.encode_end_of_message(content))
  end

  def test_abort
    assert_equal(packet("A"), @encoder.encode_abort)
  end

  def test_quit
    assert_equal(packet("Q"), @encoder.encode_quit)
  end

  def test_unknown
    command = "UNKNOWN COMMAND"
    assert_equal(packet("U", command + "\0"),
                 @encoder.encode_unknown(command))
  end
end
