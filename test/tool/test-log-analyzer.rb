# Copyright (C) 2009-2010  Kouhei Sutou <kou@clear-code.com>
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

    @analyzer = analyzer
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
    start_time = Time.parse("2009/07/15 00:00:00")
    end_time = Time.parse("2009/07/15 02:00:00")
    assert_equal(read_fetch_file("session-#{suffix}.fetch"),
                 fetch(session_rrd, start_time, end_time))
    assert_equal(read_fetch_file("milter-manager.status-#{suffix}.fetch"),
                 fetch(status_rrd, start_time, end_time))
    assert_equal(read_fetch_file("milter-manager.report-#{suffix}.fetch"),
                 fetch(report_rrd, start_time, end_time))
    assert_equal(read_fetch_file("milter.status.milter-greylist-#{suffix}.fetch"),
                 fetch(milter_status_rrd, start_time, end_time))
    assert_equal(read_fetch_file("milter.report.milter-greylist-#{suffix}.fetch"),
                 fetch(milter_report_rrd, start_time, end_time))
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
          _analyzer = analyzer
          _analyzer.output_directory = @tmp_dir.to_s
          _analyzer.prepare
          _analyzer.log = StringIO.new(lines)
          _analyzer.update
          lines = ""
        end
      end

      unless lines.empty?
        _analyzer = analyzer
        _analyzer.output_directory = @tmp_dir.to_s
        _analyzer.prepare
        _analyzer.log = StringIO.new(lines)
        _analyzer.update
      end
    end

    suffix = "1-incrementally"
    start_time = Time.parse("2009/07/15 00:00:00")
    end_time = Time.parse("2009/07/15 02:00:00")
    assert_equal(read_fetch_file("session-#{suffix}.fetch"),
                 fetch(session_rrd, start_time, end_time))
    assert_equal(read_fetch_file("milter-manager.status-#{suffix}.fetch"),
                 fetch(status_rrd, start_time, end_time))
    assert_equal(read_fetch_file("milter-manager.report-#{suffix}.fetch"),
                 fetch(report_rrd, start_time, end_time))
    assert_equal(read_fetch_file("milter.status.milter-greylist-#{suffix}.fetch"),
                 fetch(milter_status_rrd, start_time, end_time))
    assert_equal(read_fetch_file("milter.report.milter-greylist-#{suffix}.fetch"),
                 fetch(milter_report_rrd, start_time, end_time))
  end

  def test_update_with_distance
    FileUtils.cp(Dir.glob((@data_dir + "*.rrd").to_s), @tmp_dir)

    @analyzer.output_directory = @tmp_dir.to_s
    @analyzer.prepare

    input = (@data_dir + "mail-20090715-2.log").open
    @analyzer.log = input

    session_rrd = (@tmp_dir + "session.rrd").to_s
    status_rrd = (@tmp_dir + "milter-manager.status.rrd").to_s
    report_rrd = (@tmp_dir + "milter-manager.report.rrd").to_s
    milter_status_rrd = (@tmp_dir + "milter.status.milter-greylist.rrd").to_s
    milter_report_rrd = (@tmp_dir + "milter.report.milter-greylist.rrd").to_s

    @analyzer.update

    suffix = "2"
    start_time = Time.parse("2009/07/15 06:40:00")
    end_time = Time.parse("2009/07/15 09:40:00")
    assert_equal(read_fetch_file("session-#{suffix}.fetch"),
                 fetch(session_rrd, start_time, end_time))
    assert_equal(read_fetch_file("milter-manager.status-#{suffix}.fetch"),
                 fetch(status_rrd, start_time, end_time))
    assert_equal(read_fetch_file("milter-manager.report-#{suffix}.fetch"),
                 fetch(report_rrd, start_time, end_time))
    assert_equal(read_fetch_file("milter.status.milter-greylist-#{suffix}.fetch"),
                 fetch(milter_status_rrd, start_time, end_time))
    assert_equal(read_fetch_file("milter.report.milter-greylist-#{suffix}.fetch"),
                 fetch(milter_report_rrd, start_time, end_time))
  end

  def test_update_invalid_byte_sequence
    @analyzer.output_directory = @tmp_dir.to_s
    @analyzer.prepare

    input = (@data_dir + "mail-20090715-invalid-byte-sequence.log").open
    @analyzer.log = input

    assert_nothing_raised do
      @analyzer.update
    end
  end

  def test_parse_authentication_results
    report_graph_generator = MilterManagerLogAnalyzer::MilterManagerReportGraphGenerator.new(nil, Time.now)
    value =<<EOH
Authentication-Results: mail.example.com;
     spf=none smtp.mailfrom=list@lists.example.com;
     sender-id=none header.From=foo@example.com;
     dkim=pass;
     dkim-adsp=pass header.From=foo@example.com
EOH
    expected = ["dkim-pass", "dkim-adsp-pass"]
    results = []
    report_graph_generator.__send__(:parse_authentication_results, value) do |result|
      results << result
    end
    assert_equal(expected, results)
  end

  private
  def assert_received(subject, &block)
    block.call(received(subject)).call
  end

  def fetch(path, start_time, end_time)
    start_time = rrdtool_time_format(start_time)
    end_time = rrdtool_time_format(end_time)
    result = `rrdtool fetch '#{path}' MAX -s '#{start_time}' -e '#{end_time}'`
    parse_fetched_data(result)
  end

  def parse_fetched_data(raw_data)
    raw_data = StringIO.new(raw_data)
    headers = raw_data.gets.split
    raw_data.gets
    fetched_data = []
    raw_data.each_line do |line|
      case line
      when /\A(\d+): /
        row = {}
        time = $1
        $POSTMATCH.split.each_with_index do |data, i|
          row[headers[i]] = data
        end
        row["time"] = Time.at(time.to_i).strftime("%m/%d %H:%M:%S")
        fetched_data << row
      else
        raise "invalid line: <#{line}>: <#{raw_data}>"
      end
    end
    fetched_data
  end

  def rrdtool_time_format(time)
    time.strftime("%Y%m%d %H:%M")
  end

  def read_fetch_file(base_name)
    parse_fetched_data((@data_dir + base_name).read)
  end

  def analyzer
    _analyzer = MilterManagerLogAnalyzer.new
    _analyzer.now = Time.parse("2009/09/29 15:48:12")
    _analyzer
  end
end
