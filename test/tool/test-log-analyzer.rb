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
  include RR::Adapters::TestUnit

  def setup
    @data_dir = Pathname(__FILE__).dirname + "fixtures"
    @input = (@data_dir + "mail.log").open
    @analyzer = MilterManagerLogAnalyzer.new
    @analyzer.log = @input
  end

  def test_update
    mock(RRD).create("./milter-log.session.rrd",
                     "--start", "1242562259", "--step", 60,
                     "RRA:MAX:0.5:8:200",
                     "RRA:AVERAGE:0.5:8:200",
                     "RRA:MAX:0.5:56:200",
                     "RRA:AVERAGE:0.5:56:200",
                     "RRA:MAX:0.5:280:200",
                     "RRA:AVERAGE:0.5:280:200",
                     "RRA:MAX:0.5:3360:200",
                     "RRA:AVERAGE:0.5:3360:200",
                     "DS:smtp:GAUGE:180:0:U",
                     "DS:child:GAUGE:180:0:U")
    mock(RRD).update("./milter-log.session.rrd", "1242562260:1:5")
    mock(RRD).update("./milter-log.session.rrd", "1242562320:17:85")
    mock(RRD).update("./milter-log.session.rrd", "1242562380:150:750")

    mock(RRD).create("./milter-log.mail.rrd",
                     "--start", "1242562259",
                     "--step", 60,
                     "RRA:MAX:0.5:8:200",
                     "RRA:AVERAGE:0.5:8:200",
                     "RRA:MAX:0.5:56:200",
                     "RRA:AVERAGE:0.5:56:200",
                     "RRA:MAX:0.5:280:200",
                     "RRA:AVERAGE:0.5:280:200",
                     "RRA:MAX:0.5:3360:200",
                     "RRA:AVERAGE:0.5:3360:200",
                     "DS:pass:GAUGE:180:0:U",
                     "DS:accept:GAUGE:180:0:U",
                     "DS:reject:GAUGE:180:0:U",
                     "DS:discard:GAUGE:180:0:U",
                     "DS:temporary-failure:GAUGE:180:0:U",
                     "DS:quarantine:GAUGE:180:0:U",
                     "DS:abort:GAUGE:180:0:U")
    mock(RRD).update("./milter-log.mail.rrd", "1242562260:1::::::")
    mock(RRD).update("./milter-log.mail.rrd", "1242562320:17::::::")
    mock(RRD).update("./milter-log.mail.rrd", "1242562380:150::::::")

    @analyzer.update
  end
end
