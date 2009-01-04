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

class TestReplyEncoder < Test::Unit::TestCase
  include MilterEncoderTestUtils

  def setup
    @encoder = Milter::ReplyEncoder.new
  end

  def test_negotiate
    option = Milter::Option.new
    macros_requests = Milter::MacrosRequests.new
    assert_equal(packet("O", [2, 0, 0].pack("N3")),
                 @encoder.encode_negotiate(option, macros_requests))
  end

  def test_continue
    assert_equal(packet("c"), @encoder.encode_continue)
  end

  def test_reply_code
    code = "554 5.7.1 1% 2%% 3%%%"
    assert_equal(packet("y", code + "\0"),
                 @encoder.encode_reply_code(code))
  end

  def test_temporary_failure
    assert_equal(packet("t"), @encoder.encode_temporary_failure)
  end

  def test_reject
    assert_equal(packet("r"), @encoder.encode_reject)
  end

  def test_accept
    assert_equal(packet("a"), @encoder.encode_accept)
  end

  def test_discard
    assert_equal(packet("d"), @encoder.encode_discard)
  end

  def test_add_Header
    assert_equal(packet("h", "name\0value\0"),
                 @encoder.encode_add_header("name", "value"))
  end

  def test_insert_Header
    assert_equal(packet("i", [5].pack("N") + "name\0value\0"),
                 @encoder.encode_insert_header(5, "name", "value"))
  end

  def test_change_Header
    assert_equal(packet("m", [5].pack("N") + "name\0value\0"),
                 @encoder.encode_change_header("name", 5, "value"))
  end

  def test_change_from
    assert_equal(packet("e", "kou+sender@cozmixng.org\0"),
                 @encoder.encode_change_from("kou+sender@cozmixng.org"))
  end

  def test_add_recipient
    assert_equal(packet("2", "kou+receiver@cozmixng.org\0PARAMETER=VALUE\0"),
                 @encoder.encode_add_recipient("kou+receiver@cozmixng.org",
                                                     "PARAMETER=VALUE"))
  end

  def test_delete_recipient
    to = "kou+receiver@cozmixng.org"
    assert_equal(packet("-", "#{to}\0"),
                 @encoder.encode_delete_recipient(to))
  end

  def test_replace_body
    chunk = "Hi,\n\nThanks,"
    assert_equal([packet("b", chunk), chunk.size],
                 @encoder.encode_replace_body(chunk))
  end

  def test_replace_body_type_check
    chunk = "Hi,\n\nThanks,"
    input = [chunk]
    message = "chunk should be a String: #{input.inspect}"
    assert_raise(ArgumentError.new(message)) do
      @encoder.encode_replace_body(input)
    end
  end

  def test_progress
    assert_equal(packet("p"), @encoder.encode_progress)
  end

  def test_quarantine
    assert_equal(packet("q", "NO REASON\0"),
                 @encoder.encode_quarantine("NO REASON"))
  end

  def test_connection_failure
    assert_equal(packet("f"), @encoder.encode_connection_failure)
  end

  def test_shutdown
    assert_equal(packet("4"), @encoder.encode_shutdown)
  end

  def test_skip
    assert_equal(packet("s"), @encoder.encode_skip)
  end
end
