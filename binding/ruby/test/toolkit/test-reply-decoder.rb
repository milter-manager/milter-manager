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

class TestReplyDecoder < Test::Unit::TestCase
  include MilterTestUtils

  def setup
    @encoder = Milter::ReplyEncoder.new
    @decoder = Milter::ReplyDecoder.new
  end

  def test_negotiate_reply
    decoded_option = nil
    decoded_requests = nil
    @decoder.signal_connect("negotiate-reply") do |_, option, requests|
      decoded_option = option
      decoded_requests = requests
    end

    encoded_option = Milter::Option.new
    @decoder.decode(@encoder.encode_negotiate(encoded_option))
    @decoder.end_decode
    assert_equal([encoded_option, nil],
                 [decoded_option, decoded_requests])
  end

  def test_continue
    connected = false
    @decoder.signal_connect("continue") do
      connected = true
    end
    @decoder.decode(@encoder.encode_continue)
    @decoder.end_decode
    assert_true(connected)
  end

  def test_reply_code
    decoded_code = nil
    decoded_extended_code = nil
    decoded_message = nil
    @decoder.signal_connect("reply-code") do |_, code, extended_code, message|
      decoded_code = code
      decoded_extended_code = extended_code
      decoded_message = message
    end

    code = 554
    extended_code = "5.7.1"
    message = "1% 2%% 3%%%"
    reply_code = "#{code} #{extended_code} #{message}"
    @decoder.decode(@encoder.encode_reply_code(reply_code))
    @decoder.end_decode
    assert_equal([code, extended_code, message],
                 [decoded_code, decoded_extended_code, decoded_message])
  end

  def test_temporary_failure
    failed = false
    @decoder.signal_connect("temporary-failure") do
      failed = true
    end

    @decoder.decode(@encoder.encode_temporary_failure)
    @decoder.end_decode
    assert_true(failed)
  end

  def test_reject
    rejected = false
    @decoder.signal_connect("reject") do
      rejected = true
    end

    @decoder.decode(@encoder.encode_reject)
    @decoder.end_decode
    assert_true(rejected)
  end

  def test_accept
    accepted = false
    @decoder.signal_connect("accept") do
      accepted = true
    end

    @decoder.decode(@encoder.encode_accept)
    @decoder.end_decode
    assert_true(accepted)
  end

  def test_discard
    discarded = false
    @decoder.signal_connect("discard") do
      discarded = true
    end

    @decoder.decode(@encoder.encode_discard)
    @decoder.end_decode
    assert_true(discarded)
  end

  def test_add_header
    decoded_name = nil
    decoded_value = nil
    @decoder.signal_connect("add-header") do |_, name, value|
      decoded_name = name
      decoded_value = value
    end

    name = "name"
    value = "value"
    @decoder.decode(@encoder.encode_add_header(name, value))
    @decoder.end_decode
    assert_equal([name, value],
                 [decoded_name, decoded_value])
  end

  def test_insert_header
    decoded_index = nil
    decoded_name = nil
    decoded_value = nil
    @decoder.signal_connect("insert-header") do |_, index, name, value|
      decoded_index = index
      decoded_name = name
      decoded_value = value
    end

    index = 5
    name = "name"
    value = "value"
    @decoder.decode(@encoder.encode_insert_header(index, name, value))
    @decoder.end_decode
    assert_equal([index, name, value],
                 [decoded_index, decoded_name, decoded_value])
  end

  def test_change_header
    decoded_name = nil
    decoded_index = nil
    decoded_value = nil
    @decoder.signal_connect("change-header") do |_, name, index, value|
      decoded_name = name
      decoded_index = index
      decoded_value = value
    end

    name = "name"
    index = 5
    value = "value"
    @decoder.decode(@encoder.encode_change_header(name, index, value))
    @decoder.end_decode
    assert_equal([name, index, value],
                 [decoded_name, decoded_index, decoded_value])
  end

  def test_delete_header
    decoded_name = nil
    decoded_index = nil
    @decoder.signal_connect("delete-header") do |_, name, index|
      decoded_name = name
      decoded_index = index
    end

    name = "name"
    index = 5
    @decoder.decode(@encoder.encode_delete_header(name, index))
    @decoder.end_decode
    assert_equal([name, index],
                 [decoded_name, decoded_index])
  end

  def test_change_from
    decoded_from = nil
    decoded_parameters = nil
    @decoder.signal_connect("change-from") do |_, from, parameters|
      decoded_from = from
      decoded_parameters = parameters
    end

    from = "kou+sender@cozmixng.org"
    parameters = "PARAMETER=VALUE"
    @decoder.decode(@encoder.encode_change_from(from, parameters))
    @decoder.end_decode
    assert_equal([from, parameters],
                 [decoded_from, decoded_parameters])
  end

  def test_add_recipient
    decoded_to = nil
    decoded_parameters = nil
    @decoder.signal_connect("add-recipient") do |_, to, parameters|
      decoded_to = to
      decoded_parameters = parameters
    end

    to = "kou+receiver@cozmixng.org"
    parameters = "PARAMETER=VALUE"
    @decoder.decode(@encoder.encode_add_recipient(to, parameters))
    @decoder.end_decode
    assert_equal([to, parameters],
                 [decoded_to, decoded_parameters])
  end

  def test_delete_recipient
    decoded_to = nil
    @decoder.signal_connect("delete-recipient") do |_, to|
      decoded_to = to
    end

    to = "kou+receiver@cozmixng.org"
    @decoder.decode(@encoder.encode_delete_recipient(to))
    @decoder.end_decode
    assert_equal(to, decoded_to)
  end

  def test_replace_body
    decoded_chunk = nil
    @decoder.signal_connect("replace-body") do |_, chunk|
      decoded_chunk = chunk
    end

    chunk = "Hi,\n\nThanks,"
    @decoder.decode(@encoder.encode_replace_body(chunk)[0])
    @decoder.end_decode
    assert_equal(chunk, decoded_chunk)
  end

  def test_progress
    progressed = false
    @decoder.signal_connect("progress") do
      progressed = true
    end

    @decoder.decode(@encoder.encode_progress)
    @decoder.end_decode
    assert_true(progressed)
  end

  def test_quarantine
    decoded_reason = nil
    @decoder.signal_connect("quarantine") do |_, reason|
      decoded_reason = reason
    end

    reason = "NO REASON"
    @decoder.decode(@encoder.encode_quarantine(reason))
    @decoder.end_decode
    assert_equal(reason, decoded_reason)
  end

  def test_connection_failure
    failed = false
    @decoder.signal_connect("connection-failure") do
      failed = true
    end

    @decoder.decode(@encoder.encode_connection_failure)
    @decoder.end_decode
    assert_true(failed)
  end

  def test_shutdown
    shutdowned = false
    @decoder.signal_connect("shutdown") do
      shutdowned = true
    end

    @decoder.decode(@encoder.encode_shutdown)
    @decoder.end_decode
    assert_true(shutdowned)
  end

  def test_skip
    skipped = false
    @decoder.signal_connect("skip") do
      skipped = true
    end

    @decoder.decode(@encoder.encode_skip)
    @decoder.end_decode
    assert_true(skipped)
  end
end
