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

class TestReportGraphGenerator < Test::Unit::TestCase
  include RR::Adapters::RRMethods

  RR.trim_backtrace = true
  setup :setup_rr
  def setup_rr
    RR.reset
  end

  def run_test
    super
    RR.verify
  end

  def setup
    base = Pathname(__FILE__).dirname
    @data_dir = base + "fixtures"
    @tmp_dir = base + "tmp"
    FileUtils.rm_rf(@tmp_dir.to_s)
    FileUtils.mkdir_p(@tmp_dir.to_s)

    generator_class = MilterManagerLogAnalyzer::MilterManagerReportGraphGenerator
    @generator = generator_class.new(@tmp_dir, nil)
  end

  def teardown
    FileUtils.rm_rf(@tmp_dir.to_s)
  end

  priority :must
  def test_parse_x_greylist
    assert_parse_headers(["spf-pass"],
                         [["X-Greylist",
                           "Sender is SPF-compliant, " +
                           "not delayed by milter-greylist-3.0 " +
                           "(mail.example.com []); " +
                           "Mon, 27 Jul 2009 12:44:06 +0900 (JST)"],
                          ["X-Greylist",
                           "Sender succeeded SMTP AUTH authentication, " +
                           "not delayed by milter-greylist-3.0 " +
                           "(mail.example.com []); " +
                           "Wed, 12 Aug 2009 13:03:25 +0900 (JST)"]])

    assert_parse_headers(["greylisting-pass"],
                         [["X-Greylist",
                           "IP, sender and recipient auto-whitelisted, " +
                           "not delayed by milter-greylist-3.0 " +
                           "(mail.example.com [0.0.0.0]); " +
                           "Sat, 02 May 2009 05:33:56 +0900 (JST)"],
                          ["X-Greylist",
                           "Delayed for 00:31:56 by milter-greylist-3.0 " +
                           "(mail.example.com []); " +
                           "Thu, 06 Aug 2009 22:16:34 +0900 (JST)"]])
  end

  private
  def assert_parse_headers(expected, headers)
    results = []
    headers.each do |name, value|
      @generator.send(:report_key, name, value, nil) do |result|
        results << result
      end
    end
    assert_equal(expected, results)
  end
end
