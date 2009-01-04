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

class TestControlReplyEncoder < Test::Unit::TestCase
  include MilterManagerEncoderTestUtils

  def setup
    @encoder = Milter::Manager::ControlReplyEncoder.new
  end

  def test_success
    assert_equal(packet("success"), @encoder.encode_success)
  end

  def test_failure
    message = "Failure!"
    assert_equal(packet("failure", message),
                 @encoder.encode_failure(message))
  end

  def test_error
    message = "Error!"
    assert_equal(packet("error", message),
                 @encoder.encode_error(message))
  end

  def test_configuration
    configuration = ["security.privilege_mode = true", "# comment"].join("\n")
    assert_equal(packet("configuration", configuration),
                 @encoder.encode_configuration(configuration))
  end
end
