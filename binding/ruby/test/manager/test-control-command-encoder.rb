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

class TestControlCommandEncoder < Test::Unit::TestCase
  include MilterManagerEncoderTestUtils

  def setup
    @encoder = Milter::Manager::ControlCommandEncoder.new
  end

  def test_set_configuration
    configuration = "XXX"
    assert_equal(packet("set-configuration", configuration),
                 @encoder.encode_set_configuration(configuration))
  end
end
