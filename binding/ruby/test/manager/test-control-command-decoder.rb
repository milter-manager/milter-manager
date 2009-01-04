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

class TestControlCommandDecoder < Test::Unit::TestCase
  include MilterTestUtils

  def setup
    @encoder = Milter::Manager::ControlCommandEncoder.new
    @decoder = Milter::Manager::ControlCommandDecoder.new
  end

  def test_set_configuration
    decoded_configuration = nil
    @decoder.signal_connect("set-configuration") do |_, configuration|
      decoded_configuration = configuration
    end

    configuration = ["security.privilege_mode = true", "# comment"].join("\n")
    @decoder.decode(@encoder.encode_set_configuration(configuration))
    @decoder.end_decode
    assert_equal(configuration, decoded_configuration)
  end
end
