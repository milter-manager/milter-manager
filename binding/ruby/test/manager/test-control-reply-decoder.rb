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

class TestControlReplyDecoder < Test::Unit::TestCase
  include MilterTestUtils

  def setup
    @encoder = Milter::Manager::ControlReplyEncoder.new
    @decoder = Milter::Manager::ControlReplyDecoder.new
  end

  def test_success
    success_emitted = false
    @decoder.signal_connect("success") do
      success_emitted = true
    end

    @decoder.decode(@encoder.encode_success)
    @decoder.end_decode
    assert_true(success_emitted)
  end

  def test_failure
    decoded_failure_message = nil
    @decoder.signal_connect("failure") do |_, message|
      decoded_failure_message = message
    end

    failure_message =  "Failure!"
    @decoder.decode(@encoder.encode_failure(failure_message))
    @decoder.end_decode
    assert_equal(failure_message, decoded_failure_message)
  end

  def test_error
    decoded_error_message = nil
    @decoder.signal_connect("error") do |_, message|
      decoded_error_message = message
    end

    error_message =  "Error!"
    @decoder.decode(@encoder.encode_error(error_message))
    @decoder.end_decode
    assert_equal(error_message, decoded_error_message)
  end

  def test_configuration
    decoded_configuration = nil
    @decoder.signal_connect("configuration") do |_, configuration|
      decoded_configuration = configuration
    end

    configuration = ["security.privilege_mode = true", "# comment"].join("\n")
    @decoder.decode(@encoder.encode_configuration(configuration))
    @decoder.end_decode
    assert_equal(configuration, decoded_configuration)
  end
end
