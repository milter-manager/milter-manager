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

class TestLogAnalyzer < Test::Unit::TestCase
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

    @analyzer = MilterManagerLogAnalyzer.new
    @analyzer.now = Time.parse("#{Time.now.year}/09/29 15:48:12")
  end

  def teardown
    FileUtils.rm_rf(@tmp_dir.to_s)
  end

  priority :must
  def test_update
    @analyzer.output_directory = @tmp_dir.to_s
    @analyzer.prepare

    input = (@data_dir + "mail-20090715-1.log").open
    @analyzer.log = input

    session_rrd = (@tmp_dir + "session.rrd").to_s
    status_rrd = (@tmp_dir + "milter-manager.status.rrd").to_s
    report_rrd = (@tmp_dir + "milter-manager.report.rrd").to_s
    milter_status_rrd = (@tmp_dir + "milter.status.milter-greylist.rrd").to_s
    milter_report_rrd = (@tmp_dir + "milter.report.milter-greylist.rrd").to_s

    @analyzer.update

    suffix = "1"
    assert_equal((@data_dir + "session-#{suffix}.dump").read,
                 dump(session_rrd))
    assert_equal((@data_dir + "milter-manager.status-#{suffix}.dump").read,
                 dump(status_rrd))
    assert_equal((@data_dir + "milter-manager.report-#{suffix}.dump").read,
                 dump(report_rrd))
    assert_equal((@data_dir + "milter.status.milter-greylist-#{suffix}.dump").read,
                 dump(milter_status_rrd))
    assert_equal((@data_dir + "milter.report.milter-greylist-#{suffix}.dump").read,
                 dump(milter_report_rrd))
  end

  def test_update_incrementally
    session_rrd = (@tmp_dir + "session.rrd").to_s
    status_rrd = (@tmp_dir + "milter-manager.status.rrd").to_s
    report_rrd = (@tmp_dir + "milter-manager.report.rrd").to_s
    milter_status_rrd = (@tmp_dir + "milter.status.milter-greylist.rrd").to_s
    milter_report_rrd = (@tmp_dir + "milter.report.milter-greylist.rrd").to_s

    (@data_dir + "mail-20090715-1.log").open do |log|
      lines = ""
      i = 0
      log.each_line do |line|
        i += 1
        lines << line
        if (i % 1000).zero?
          analyzer = MilterManagerLogAnalyzer.new
          analyzer.output_directory = @tmp_dir.to_s
          analyzer.prepare
          analyzer.log = StringIO.new(lines)
          analyzer.update
          lines = ""
        end
      end

      unless lines.empty?
        analyzer = MilterManagerLogAnalyzer.new
        analyzer.output_directory = @tmp_dir.to_s
        analyzer.prepare
        analyzer.log = StringIO.new(lines)
        analyzer.update
      end
    end

    suffix = "1-incrementally"
    assert_equal((@data_dir + "session-#{suffix}.dump").read,
                 dump(session_rrd))
    assert_equal((@data_dir + "milter-manager.status-#{suffix}.dump").read,
                 dump(status_rrd))
    assert_equal((@data_dir + "milter-manager.report-#{suffix}.dump").read,
                 dump(report_rrd))
    assert_equal((@data_dir + "milter.status.milter-greylist-#{suffix}.dump").read,
                 dump(milter_status_rrd))
    assert_equal((@data_dir + "milter.report.milter-greylist-#{suffix}.dump").read,
                 dump(milter_report_rrd))
  end

  def test_update_with_distance
    @analyzer.output_directory = @tmp_dir.to_s
    @analyzer.prepare
    FileUtils.cp(Dir.glob((@data_dir + "*.rrd").to_s), @tmp_dir)

    input = (@data_dir + "mail-20090715-2.log").open
    @analyzer.log = input

    session_rrd = (@tmp_dir + "session.rrd").to_s
    status_rrd = (@tmp_dir + "milter-manager.status.rrd").to_s
    report_rrd = (@tmp_dir + "milter-manager.report.rrd").to_s
    milter_status_rrd = (@tmp_dir + "milter.status.milter-greylist.rrd").to_s
    milter_report_rrd = (@tmp_dir + "milter.report.milter-greylist.rrd").to_s

    @analyzer.update

    suffix = "2"
    assert_equal((@data_dir + "session-#{suffix}.dump").read,
                 dump(session_rrd))
    assert_equal((@data_dir + "milter-manager.status-#{suffix}.dump").read,
                 dump(status_rrd))
    assert_equal((@data_dir + "milter-manager.report-#{suffix}.dump").read,
                 dump(report_rrd))
    assert_equal((@data_dir + "milter.status.milter-greylist-#{suffix}.dump").read,
                 dump(milter_status_rrd))
    assert_equal((@data_dir + "milter.report.milter-greylist-#{suffix}.dump").read,
                 dump(milter_report_rrd))
  end

  private
  def assert_received(subject, &block)
    block.call(received(subject)).call
  end

  def dump(path)
    `rrdtool dump '#{path}'`
  end
end
