# Copyright (C) 2009  Kouhei Sutou <kou@clear-code.com>
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

class TestDetector < Test::Unit::TestCase
  include MilterTestUtils

  class MockDetector
    include Milter::Manager::Detector
  end

  def setup
    @configuration = Milter::Manager::Configuration.new
  end

  def test_package_options
    assert_equal({}, detector.package_options)
    @configuration.package_options = "prefix=/etc,name=test"
    assert_equal({
                   "prefix" => "/etc",
                   "name" => "test"
                 },
                 detector.package_options)
  end

  private
  def detector
    MockDetector.new(@configuration, "test-milter")
  end
end
