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
    base = Pathname(__FILE__).dirname
    @data_dir = base + "fixtures"
    @tmp_dir = base + "tmp"
    FileUtils.rm_rf(@tmp_dir.to_s)
    FileUtils.mkdir_p(@tmp_dir.to_s)

    @analyzer = MilterManagerLogAnalyzer.new
  end

  def teardown
    FileUtils.rm_rf(@tmp_dir.to_s)
  end

  def test_update
    input = (@data_dir + "mail.log").open
    @analyzer.log = input

    mock(RRD).create("./milter-log.session.rrd",
                     "--start", "1242831599", "--step", 60,
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
    mock(RRD).update("./milter-log.session.rrd", "1242831600:2:12")
    mock(RRD).update("./milter-log.session.rrd", "1242831660::")
    mock(RRD).update("./milter-log.session.rrd", "1242831720:3:18")
    mock(RRD).update("./milter-log.session.rrd", "1242831780:2:12")
    mock(RRD).update("./milter-log.session.rrd", "1242831840:2:12")
    mock(RRD).update("./milter-log.session.rrd", "1242831900:2:12")
    mock(RRD).update("./milter-log.session.rrd", "1242831960:5:30")
    mock(RRD).update("./milter-log.session.rrd", "1242832020:1:6")
    mock(RRD).update("./milter-log.session.rrd", "1242832080:1:6")
    mock(RRD).update("./milter-log.session.rrd", "1242832140::")
    mock(RRD).update("./milter-log.session.rrd", "1242832200:4:29")
    mock(RRD).update("./milter-log.session.rrd", "1242832260:1:1")
    mock(RRD).update("./milter-log.session.rrd", "1242832320::")
    mock(RRD).update("./milter-log.session.rrd", "1242832380:1:6")
    mock(RRD).update("./milter-log.session.rrd", "1242832440::")
    mock(RRD).update("./milter-log.session.rrd", "1242832500:1:6")
    mock(RRD).update("./milter-log.session.rrd", "1242832560:1:6")
    mock(RRD).update("./milter-log.session.rrd", "1242832620:1:6")
    mock(RRD).update("./milter-log.session.rrd", "1242832680:3:18")
    mock(RRD).update("./milter-log.session.rrd", "1242832740:1:6")
    mock(RRD).update("./milter-log.session.rrd", "1242832800:3:18")
    mock(RRD).update("./milter-log.session.rrd", "1242832860:1:6")
    mock(RRD).update("./milter-log.session.rrd", "1242832920:4:24")
    mock(RRD).update("./milter-log.session.rrd", "1242832980::")
    mock(RRD).update("./milter-log.session.rrd", "1242833040:3:23")
    mock(RRD).update("./milter-log.session.rrd", "1242833100:2:7")
    mock(RRD).update("./milter-log.session.rrd", "1242833160::")
    mock(RRD).update("./milter-log.session.rrd", "1242833220::")
    mock(RRD).update("./milter-log.session.rrd", "1242833280:2:12")
    mock(RRD).update("./milter-log.session.rrd", "1242833340:1:6")
    mock(RRD).update("./milter-log.session.rrd", "1242833400:2:12")
    mock(RRD).update("./milter-log.session.rrd", "1242833460:5:30")
    mock(RRD).update("./milter-log.session.rrd", "1242833520::")
    mock(RRD).update("./milter-log.session.rrd", "1242833580:1:6")
    mock(RRD).update("./milter-log.session.rrd", "1242833640:1:6")
    mock(RRD).update("./milter-log.session.rrd", "1242833700:3:18")
    mock(RRD).update("./milter-log.session.rrd", "1242833760:3:18")
    mock(RRD).update("./milter-log.session.rrd", "1242833820::")
    mock(RRD).update("./milter-log.session.rrd", "1242833880:1:6")
    mock(RRD).update("./milter-log.session.rrd", "1242833940:2:12")
    mock(RRD).update("./milter-log.session.rrd", "1242834000:1:11")
    mock(RRD).update("./milter-log.session.rrd", "1242834060:2:7")
    mock(RRD).update("./milter-log.session.rrd", "1242834120:3:18")
    mock(RRD).update("./milter-log.session.rrd", "1242834180:2:12")
    mock(RRD).update("./milter-log.session.rrd", "1242834240:2:12")
    mock(RRD).update("./milter-log.session.rrd", "1242834300:5:30")
    mock(RRD).update("./milter-log.session.rrd", "1242834360:2:12")
    mock(RRD).update("./milter-log.session.rrd", "1242834420:4:24")
    mock(RRD).update("./milter-log.session.rrd", "1242834480:1:6")
    mock(RRD).update("./milter-log.session.rrd", "1242834540::4")
    mock(RRD).update("./milter-log.session.rrd", "1242834600:2:8")
    mock(RRD).update("./milter-log.session.rrd", "1242834660:1:6")
    mock(RRD).update("./milter-log.session.rrd", "1242834720:2:12")
    mock(RRD).update("./milter-log.session.rrd", "1242834780::")
    mock(RRD).update("./milter-log.session.rrd", "1242834840:1:6")
    mock(RRD).update("./milter-log.session.rrd", "1242834900:3:18")
    mock(RRD).update("./milter-log.session.rrd", "1242834960:3:18")
    mock(RRD).update("./milter-log.session.rrd", "1242835020:3:18")
    mock(RRD).update("./milter-log.session.rrd", "1242835080::")
    mock(RRD).update("./milter-log.session.rrd", "1242835140:1:6")
    mock(RRD).update("./milter-log.session.rrd", "1242835200::1")
    mock(RRD).update("./milter-log.session.rrd", "1242835260:3:17")
    mock(RRD).update("./milter-log.session.rrd", "1242835320:2:12")
    mock(RRD).update("./milter-log.session.rrd", "1242835380:1:6")
    mock(RRD).update("./milter-log.session.rrd", "1242835440:5:30")
    mock(RRD).update("./milter-log.session.rrd", "1242835500:3:18")
    mock(RRD).update("./milter-log.session.rrd", "1242835560:1:6")
    mock(RRD).update("./milter-log.session.rrd", "1242835620::")
    mock(RRD).update("./milter-log.session.rrd", "1242835680:5:30")
    mock(RRD).update("./milter-log.session.rrd", "1242835740:1:6")
    mock(RRD).update("./milter-log.session.rrd", "1242835800:4:24")
    mock(RRD).update("./milter-log.session.rrd", "1242835860:2:13")
    mock(RRD).update("./milter-log.session.rrd", "1242835920:1:5")
    mock(RRD).update("./milter-log.session.rrd", "1242835980::")
    mock(RRD).update("./milter-log.session.rrd", "1242836040:3:18")
    mock(RRD).update("./milter-log.session.rrd", "1242836100:1:6")
    mock(RRD).update("./milter-log.session.rrd", "1242836160:4:24")
    mock(RRD).update("./milter-log.session.rrd", "1242836220:2:12")
    mock(RRD).update("./milter-log.session.rrd", "1242836280:3:18")
    mock(RRD).update("./milter-log.session.rrd", "1242836340::")
    mock(RRD).update("./milter-log.session.rrd", "1242836400:2:12")
    mock(RRD).update("./milter-log.session.rrd", "1242836460:1:6")
    mock(RRD).update("./milter-log.session.rrd", "1242836520:3:23")
    mock(RRD).update("./milter-log.session.rrd", "1242836580:2:13")
    mock(RRD).update("./milter-log.session.rrd", "1242836640:3:18")
    mock(RRD).update("./milter-log.session.rrd", "1242836700:1:6")
    mock(RRD).update("./milter-log.session.rrd", "1242836760:1:6")
    mock(RRD).update("./milter-log.session.rrd", "1242836820:1:11")
    mock(RRD).update("./milter-log.session.rrd", "1242836880:4:13")
    mock(RRD).update("./milter-log.session.rrd", "1242836940:1:6")
    mock(RRD).update("./milter-log.session.rrd", "1242837000:3:18")
    mock(RRD).update("./milter-log.session.rrd", "1242837060:2:13")

    mock(RRD).create("./milter-log.mail.rrd",
                     "--start", "1242831599", "--step", 60,
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
    mock(RRD).update("./milter-log.mail.rrd", "1242831600:2::::::")
    mock(RRD).update("./milter-log.mail.rrd", "1242831660:::::::")
    mock(RRD).update("./milter-log.mail.rrd", "1242831720:::::2::")
    mock(RRD).update("./milter-log.mail.rrd", "1242831780:2::::::")
    mock(RRD).update("./milter-log.mail.rrd", "1242831840:::::2::")
    mock(RRD).update("./milter-log.mail.rrd", "1242831900:1::::1::")
    mock(RRD).update("./milter-log.mail.rrd", "1242831960:1::::4::")
    mock(RRD).update("./milter-log.mail.rrd", "1242832020:1::::::")
    mock(RRD).update("./milter-log.mail.rrd", "1242832080:::::1::")
    mock(RRD).update("./milter-log.mail.rrd", "1242832140:::::::")
    mock(RRD).update("./milter-log.mail.rrd", "1242832200:3::::::")
    mock(RRD).update("./milter-log.mail.rrd", "1242832260:1::::::")
    mock(RRD).update("./milter-log.mail.rrd", "1242832320:::::::")
    mock(RRD).update("./milter-log.mail.rrd", "1242832380:1::::::")
    mock(RRD).update("./milter-log.mail.rrd", "1242832440:::::::")
    mock(RRD).update("./milter-log.mail.rrd", "1242832500:::::1::")
    mock(RRD).update("./milter-log.mail.rrd", "1242832560:::::1::")
    mock(RRD).update("./milter-log.mail.rrd", "1242832620:::::1::")
    mock(RRD).update("./milter-log.mail.rrd", "1242832680:::::::")
    mock(RRD).update("./milter-log.mail.rrd", "1242832740:1::::::")
    mock(RRD).update("./milter-log.mail.rrd", "1242832800:1::::1::")
    mock(RRD).update("./milter-log.mail.rrd", "1242832860:1::::::")
    mock(RRD).update("./milter-log.mail.rrd", "1242832920:1::::3::")
    mock(RRD).update("./milter-log.mail.rrd", "1242832980:::::::")
    mock(RRD).update("./milter-log.mail.rrd", "1242833040:3::::::")
    mock(RRD).update("./milter-log.mail.rrd", "1242833100:1::::::")
    mock(RRD).update("./milter-log.mail.rrd", "1242833160:::::::")
    mock(RRD).update("./milter-log.mail.rrd", "1242833220:::::::")
    mock(RRD).update("./milter-log.mail.rrd", "1242833280:1::::1::")
    mock(RRD).update("./milter-log.mail.rrd", "1242833340:1::::::")
    mock(RRD).update("./milter-log.mail.rrd", "1242833400:1::::1::")
    mock(RRD).update("./milter-log.mail.rrd", "1242833460:::::4::")
    mock(RRD).update("./milter-log.mail.rrd", "1242833520:::::::")
    mock(RRD).update("./milter-log.mail.rrd", "1242833580:::::1::")
    mock(RRD).update("./milter-log.mail.rrd", "1242833640:::::::")
    mock(RRD).update("./milter-log.mail.rrd", "1242833700:::::3::")
    mock(RRD).update("./milter-log.mail.rrd", "1242833760:1::::2::")
    mock(RRD).update("./milter-log.mail.rrd", "1242833820:::::::")
    mock(RRD).update("./milter-log.mail.rrd", "1242833880:::::1::")
    mock(RRD).update("./milter-log.mail.rrd", "1242833940:::::2::")
    mock(RRD).update("./milter-log.mail.rrd", "1242834000:::::::")
    mock(RRD).update("./milter-log.mail.rrd", "1242834060:1::::1::")
    mock(RRD).update("./milter-log.mail.rrd", "1242834120:2::::1::")
    mock(RRD).update("./milter-log.mail.rrd", "1242834180:::::2::")
    mock(RRD).update("./milter-log.mail.rrd", "1242834240:::::1::")
    mock(RRD).update("./milter-log.mail.rrd", "1242834300:2::::1::")
    mock(RRD).update("./milter-log.mail.rrd", "1242834360:::::2::")
    mock(RRD).update("./milter-log.mail.rrd", "1242834420:::::3::")
    mock(RRD).update("./milter-log.mail.rrd", "1242834480:1::::::")
    mock(RRD).update("./milter-log.mail.rrd", "1242834540:::::::")
    mock(RRD).update("./milter-log.mail.rrd", "1242834600:1::::::")
    mock(RRD).update("./milter-log.mail.rrd", "1242834660:1::::::")
    mock(RRD).update("./milter-log.mail.rrd", "1242834720:2::::::")
    mock(RRD).update("./milter-log.mail.rrd", "1242834780:::::::")
    mock(RRD).update("./milter-log.mail.rrd", "1242834840:::::1::")
    mock(RRD).update("./milter-log.mail.rrd", "1242834900:1::::2::")
    mock(RRD).update("./milter-log.mail.rrd", "1242834960:::::1::")
    mock(RRD).update("./milter-log.mail.rrd", "1242835020:::::3::")
    mock(RRD).update("./milter-log.mail.rrd", "1242835080:::::::")
    mock(RRD).update("./milter-log.mail.rrd", "1242835140:1::::::")
    mock(RRD).update("./milter-log.mail.rrd", "1242835200:::::::")
    mock(RRD).update("./milter-log.mail.rrd", "1242835260:1::::1::")
    mock(RRD).update("./milter-log.mail.rrd", "1242835320:::::2::")
    mock(RRD).update("./milter-log.mail.rrd", "1242835380:::::::")
    mock(RRD).update("./milter-log.mail.rrd", "1242835440:::::5::")
    mock(RRD).update("./milter-log.mail.rrd", "1242835500:1::::2::")
    mock(RRD).update("./milter-log.mail.rrd", "1242835560:1::::::")
    mock(RRD).update("./milter-log.mail.rrd", "1242835620:::::::")
    mock(RRD).update("./milter-log.mail.rrd", "1242835680:1::::4::")
    mock(RRD).update("./milter-log.mail.rrd", "1242835740:::::1::")
    mock(RRD).update("./milter-log.mail.rrd", "1242835800:1::::1::")
    mock(RRD).update("./milter-log.mail.rrd", "1242835860:::::2::")
    mock(RRD).update("./milter-log.mail.rrd", "1242835920:1::::::")
    mock(RRD).update("./milter-log.mail.rrd", "1242835980:::::::")
    mock(RRD).update("./milter-log.mail.rrd", "1242836040:::::3::")
    mock(RRD).update("./milter-log.mail.rrd", "1242836100:::::1::")
    mock(RRD).update("./milter-log.mail.rrd", "1242836160:::::4::")
    mock(RRD).update("./milter-log.mail.rrd", "1242836220:::::2::")
    mock(RRD).update("./milter-log.mail.rrd", "1242836280:::::3::")
    mock(RRD).update("./milter-log.mail.rrd", "1242836340:::::::")
    mock(RRD).update("./milter-log.mail.rrd", "1242836400:::::1::")
    mock(RRD).update("./milter-log.mail.rrd", "1242836460:::::::")
    mock(RRD).update("./milter-log.mail.rrd", "1242836520:1:1:::::")
    mock(RRD).update("./milter-log.mail.rrd", "1242836580:2::::2::")
    mock(RRD).update("./milter-log.mail.rrd", "1242836640:::::1::")
    mock(RRD).update("./milter-log.mail.rrd", "1242836700:::::1::")
    mock(RRD).update("./milter-log.mail.rrd", "1242836760:::::3::")
    mock(RRD).update("./milter-log.mail.rrd", "1242836820:1::::::")
    mock(RRD).update("./milter-log.mail.rrd", "1242836880:1::::2::")
    mock(RRD).update("./milter-log.mail.rrd", "1242836940:::::::")
    mock(RRD).update("./milter-log.mail.rrd", "1242837000:::::3::")
    mock(RRD).update("./milter-log.mail.rrd", "1242837060:::::2::")

    mock(RRD).create("./milter-log.stop.rrd",
                     "--start", "1242831599", "--step", 60,
                     "RRA:MAX:0.5:8:200",
                     "RRA:AVERAGE:0.5:8:200",
                     "RRA:MAX:0.5:56:200",
                     "RRA:AVERAGE:0.5:56:200",
                     "RRA:MAX:0.5:280:200",
                     "RRA:AVERAGE:0.5:280:200",
                     "RRA:MAX:0.5:3360:200",
                     "RRA:AVERAGE:0.5:3360:200",
                     "DS:connect:GAUGE:180:0:U",
                     "DS:helo:GAUGE:180:0:U",
                     "DS:envelope-from:GAUGE:180:0:U",
                     "DS:envelope-recipient:GAUGE:180:0:U",
                     "DS:header:GAUGE:180:0:U",
                     "DS:body:GAUGE:180:0:U",
                     "DS:end-of-message:GAUGE:180:0:U")
    mock(RRD).update("./milter-log.stop.rrd", "1242831600:2::::::")
    mock(RRD).update("./milter-log.stop.rrd", "1242831660:::::::")
    mock(RRD).update("./milter-log.stop.rrd", "1242831720:4::::::")
    mock(RRD).update("./milter-log.stop.rrd", "1242831780:1::::::")
    mock(RRD).update("./milter-log.stop.rrd", "1242831840:::::::")
    mock(RRD).update("./milter-log.stop.rrd", "1242831900:1::::::")
    mock(RRD).update("./milter-log.stop.rrd", "1242831960:1::::::")
    mock(RRD).update("./milter-log.stop.rrd", "1242832020:1::::::")
    mock(RRD).update("./milter-log.stop.rrd", "1242832080:::::::")
    mock(RRD).update("./milter-log.stop.rrd", "1242832140:::::::")
    mock(RRD).update("./milter-log.stop.rrd", "1242832200:7::::::")
    mock(RRD).update("./milter-log.stop.rrd", "1242832260:::::::")
    mock(RRD).update("./milter-log.stop.rrd", "1242832320:::::::")
    mock(RRD).update("./milter-log.stop.rrd", "1242832380:1::::::")
    mock(RRD).update("./milter-log.stop.rrd", "1242832440:::::::")
    mock(RRD).update("./milter-log.stop.rrd", "1242832500:::::::")
    mock(RRD).update("./milter-log.stop.rrd", "1242832560:::::::")
    mock(RRD).update("./milter-log.stop.rrd", "1242832620:::::::")
    mock(RRD).update("./milter-log.stop.rrd", "1242832680:8::::::")
    mock(RRD).update("./milter-log.stop.rrd", "1242832740:1::::::")
    mock(RRD).update("./milter-log.stop.rrd", "1242832800:5::::::")
    mock(RRD).update("./milter-log.stop.rrd", "1242832860:1::::::")
    mock(RRD).update("./milter-log.stop.rrd", "1242832920:1::::::")
    mock(RRD).update("./milter-log.stop.rrd", "1242832980:::::::")
    mock(RRD).update("./milter-log.stop.rrd", "1242833040:4::::::")
    mock(RRD).update("./milter-log.stop.rrd", "1242833100:::::::")
    mock(RRD).update("./milter-log.stop.rrd", "1242833160:::::::")
    mock(RRD).update("./milter-log.stop.rrd", "1242833220:::::::")
    mock(RRD).update("./milter-log.stop.rrd", "1242833280:1::::::")
    mock(RRD).update("./milter-log.stop.rrd", "1242833340:1::::::")
    mock(RRD).update("./milter-log.stop.rrd", "1242833400:1::::::")
    mock(RRD).update("./milter-log.stop.rrd", "1242833460:4::::::")
    mock(RRD).update("./milter-log.stop.rrd", "1242833520:::::::")
    mock(RRD).update("./milter-log.stop.rrd", "1242833580:::::::")
    mock(RRD).update("./milter-log.stop.rrd", "1242833640:::::::")
    mock(RRD).update("./milter-log.stop.rrd", "1242833700:::::::")
    mock(RRD).update("./milter-log.stop.rrd", "1242833760:1::::::")
    mock(RRD).update("./milter-log.stop.rrd", "1242833820:::::::")
    mock(RRD).update("./milter-log.stop.rrd", "1242833880:::::::")
    mock(RRD).update("./milter-log.stop.rrd", "1242833940:::::::")
    mock(RRD).update("./milter-log.stop.rrd", "1242834000:5::::::")
    mock(RRD).update("./milter-log.stop.rrd", "1242834060:::::::")
    mock(RRD).update("./milter-log.stop.rrd", "1242834120:2::::::")
    mock(RRD).update("./milter-log.stop.rrd", "1242834180:::::::")
    mock(RRD).update("./milter-log.stop.rrd", "1242834240:::::::")
    mock(RRD).update("./milter-log.stop.rrd", "1242834300:2::::::")
    mock(RRD).update("./milter-log.stop.rrd", "1242834360:::::::")
    mock(RRD).update("./milter-log.stop.rrd", "1242834420:::::::")
    mock(RRD).update("./milter-log.stop.rrd", "1242834480:1::::::")
    mock(RRD).update("./milter-log.stop.rrd", "1242834540:::::::")
    mock(RRD).update("./milter-log.stop.rrd", "1242834600:4::::::")
    mock(RRD).update("./milter-log.stop.rrd", "1242834660:1::::::")
    mock(RRD).update("./milter-log.stop.rrd", "1242834720:2::::::")
    mock(RRD).update("./milter-log.stop.rrd", "1242834780:::::::")
    mock(RRD).update("./milter-log.stop.rrd", "1242834840:::::::")
    mock(RRD).update("./milter-log.stop.rrd", "1242834900:1::::::")
    mock(RRD).update("./milter-log.stop.rrd", "1242834960:8::::::")
    mock(RRD).update("./milter-log.stop.rrd", "1242835020:::::::")
    mock(RRD).update("./milter-log.stop.rrd", "1242835080:::::::")
    mock(RRD).update("./milter-log.stop.rrd", "1242835140:1::::::")
    mock(RRD).update("./milter-log.stop.rrd", "1242835200:1::::::")
    mock(RRD).update("./milter-log.stop.rrd", "1242835260:4::::::")
    mock(RRD).update("./milter-log.stop.rrd", "1242835320:::::::")
    mock(RRD).update("./milter-log.stop.rrd", "1242835380:::::::")
    mock(RRD).update("./milter-log.stop.rrd", "1242835440:::::::")
    mock(RRD).update("./milter-log.stop.rrd", "1242835500:1::::::")
    mock(RRD).update("./milter-log.stop.rrd", "1242835560:1::::::")
    mock(RRD).update("./milter-log.stop.rrd", "1242835620:::::::")
    mock(RRD).update("./milter-log.stop.rrd", "1242835680:1::::::")
    mock(RRD).update("./milter-log.stop.rrd", "1242835740:::::::")
    mock(RRD).update("./milter-log.stop.rrd", "1242835800:5::::::")
    mock(RRD).update("./milter-log.stop.rrd", "1242835860:1::::::")
    mock(RRD).update("./milter-log.stop.rrd", "1242835920:::::::")
    mock(RRD).update("./milter-log.stop.rrd", "1242835980:::::::")
    mock(RRD).update("./milter-log.stop.rrd", "1242836040:::::::")
    mock(RRD).update("./milter-log.stop.rrd", "1242836100:::::::")
    mock(RRD).update("./milter-log.stop.rrd", "1242836160:::::::")
    mock(RRD).update("./milter-log.stop.rrd", "1242836220:::::::")
    mock(RRD).update("./milter-log.stop.rrd", "1242836280:::::::")
    mock(RRD).update("./milter-log.stop.rrd", "1242836340:::::::")
    mock(RRD).update("./milter-log.stop.rrd", "1242836400:4::::::")
    mock(RRD).update("./milter-log.stop.rrd", "1242836460:::::::")
    mock(RRD).update("./milter-log.stop.rrd", "1242836520:5::::::")
    mock(RRD).update("./milter-log.stop.rrd", "1242836580:1::::::")
    mock(RRD).update("./milter-log.stop.rrd", "1242836640:::::::")
    mock(RRD).update("./milter-log.stop.rrd", "1242836700:::::::")
    mock(RRD).update("./milter-log.stop.rrd", "1242836760:::::::")
    mock(RRD).update("./milter-log.stop.rrd", "1242836820:2::::::")
    mock(RRD).update("./milter-log.stop.rrd", "1242836880:::::::")
    mock(RRD).update("./milter-log.stop.rrd", "1242836940:1::::::")
    mock(RRD).update("./milter-log.stop.rrd", "1242837000:::::::")
    mock(RRD).update("./milter-log.stop.rrd", "1242837060:1::::::")

    @analyzer.update
  end

  def test_output_all_graph
    @analyzer.output_directory = @tmp_dir.to_s

    FileUtils.cp(Dir.glob((@data_dir + "*.rrd").to_s), @tmp_dir)

    rows = 3 / 600.to_f
    now = Time.now.to_i

    day_start_time = -(60 * 60 * 24)
    day_end_time = now - (now % (day_start_time.abs * rows).to_i)

    week_start_time = day_start_time * 7
    week_end_time = now - (now % (week_start_time.abs * rows).to_i)

    month_start_time = week_start_time * 5
    month_end_time = now - (now % (month_start_time.abs * rows).to_i)

    year_start_time = month_start_time * 12
    year_end_time = now - (now % (year_start_time.abs * rows).to_i)

    session_rrd = (@tmp_dir + "milter-log.session.rrd").to_s
    session_day_png = (@tmp_dir + "session.day.png").to_s
    session_week_png = (@tmp_dir + "session.week.png").to_s
    session_month_png = (@tmp_dir + "session.month.png").to_s
    session_year_png = (@tmp_dir + "session.year.png").to_s

    mail_rrd = (@tmp_dir + "milter-log.mail.rrd").to_s
    mail_day_png = (@tmp_dir + "mail.day.png").to_s
    mail_week_png = (@tmp_dir + "mail.week.png").to_s
    mail_month_png = (@tmp_dir + "mail.month.png").to_s
    mail_year_png = (@tmp_dir + "mail.year.png").to_s

    stop_rrd = (@tmp_dir + "milter-log.stop.rrd").to_s
    stop_day_png = (@tmp_dir + "stop.day.png").to_s
    stop_week_png = (@tmp_dir + "stop.week.png").to_s
    stop_month_png = (@tmp_dir + "stop.month.png").to_s
    stop_year_png = (@tmp_dir + "stop.year.png").to_s

    mock(RRD).last(session_rrd) {Time.at(1242837060)}
    mock(RRD).graph(session_day_png,
                    "--title", "Sessions",
                    "--vertical-label", "sessions/min",
                    "--start", "-86400",
                    "--end", day_end_time.to_s,
                    "--width", "600",
                    "--height", "200",
                    "--alt-y-grid",
                    "--units-exponent", "0",
                    "DEF:smtp=#{session_rrd}:smtp:AVERAGE",
                    "DEF:max_smtp=#{session_rrd}:smtp:MAX",
                    "CDEF:n_smtp=smtp",
                    "CDEF:real_smtp=smtp",
                    "CDEF:real_max_smtp=max_smtp",
                    "CDEF:real_n_smtp=n_smtp,8,*",
                    "CDEF:total_smtp=PREV,UN,real_n_smtp,PREV,IF,real_n_smtp,+",
                    "DEF:child=#{session_rrd}:child:AVERAGE",
                    "DEF:max_child=#{session_rrd}:child:MAX",
                    "CDEF:n_child=child",
                    "CDEF:real_child=child",
                    "CDEF:real_max_child=max_child",
                    "CDEF:real_n_child=n_child,8,*",
                    "CDEF:total_child=PREV,UN,real_n_child,PREV,IF,real_n_child,+",
                    "AREA:n_smtp#0000ff:SMTP  ",
                    "GPRINT:total_smtp:MAX:total\\: %8.0lf sessions",
                    "GPRINT:smtp:AVERAGE:avg\\: %6.2lf sessions/min",
                    "GPRINT:max_smtp:MAX:max\\: %4.0lf sessions/min\\l",
                    "LINE2:n_child#00ff00:milter",
                    "GPRINT:total_child:MAX:total\\: %8.0lf sessions",
                    "GPRINT:child:AVERAGE:avg\\: %6.2lf sessions/min",
                    "GPRINT:max_child:MAX:max\\: %4.0lf sessions/min\\l",
                    "COMMENT:[2009-05-21T01\\:31\\:00+09\\:00]\\r")

    mock(RRD).last(mail_rrd) {Time.at(1242837060)}
    mock(RRD).graph(mail_day_png,
                    "--title", "Processed mails",
                    "--vertical-label", "mails/min",
                    "--start", "-86400",
                    "--end", day_end_time.to_s,
                    "--width", "600",
                    "--height", "200",
                    "--alt-y-grid",
                    "--units-exponent", "0",
                    "DEF:pass=#{mail_rrd}:pass:AVERAGE",
                    "DEF:max_pass=#{mail_rrd}:pass:MAX",
                    "CDEF:n_pass=pass",
                    "CDEF:real_pass=pass",
                    "CDEF:real_max_pass=max_pass",
                    "CDEF:real_n_pass=n_pass,8,*",
                    "CDEF:total_pass=PREV,UN,real_n_pass,PREV,IF,real_n_pass,+",
                    "DEF:accept=#{mail_rrd}:accept:AVERAGE",
                    "DEF:max_accept=#{mail_rrd}:accept:MAX",
                    "CDEF:n_accept=accept",
                    "CDEF:real_accept=accept",
                    "CDEF:real_max_accept=max_accept",
                    "CDEF:real_n_accept=n_accept,8,*",
                    "CDEF:total_accept=PREV,UN,real_n_accept,PREV,IF,real_n_accept,+",
                    "DEF:reject=#{mail_rrd}:reject:AVERAGE",
                    "DEF:max_reject=#{mail_rrd}:reject:MAX",
                    "CDEF:n_reject=reject",
                    "CDEF:real_reject=reject",
                    "CDEF:real_max_reject=max_reject",
                    "CDEF:real_n_reject=n_reject,8,*",
                    "CDEF:total_reject=PREV,UN,real_n_reject,PREV,IF,real_n_reject,+",
                    "DEF:discard=#{mail_rrd}:discard:AVERAGE",
                    "DEF:max_discard=#{mail_rrd}:discard:MAX",
                    "CDEF:n_discard=discard",
                    "CDEF:real_discard=discard",
                    "CDEF:real_max_discard=max_discard",
                    "CDEF:real_n_discard=n_discard,8,*",
                    "CDEF:total_discard=PREV,UN,real_n_discard,PREV,IF,real_n_discard,+",
                    "DEF:temporary-failure=#{mail_rrd}:temporary-failure:AVERAGE",
                    "DEF:max_temporary-failure=#{mail_rrd}:temporary-failure:MAX",
                    "CDEF:n_temporary-failure=temporary-failure",
                    "CDEF:real_temporary-failure=temporary-failure",
                    "CDEF:real_max_temporary-failure=max_temporary-failure",
                    "CDEF:real_n_temporary-failure=n_temporary-failure,8,*",
                    "CDEF:total_temporary-failure=PREV,UN,real_n_temporary-failure,PREV,IF,real_n_temporary-failure,+",
                    "DEF:quarantine=#{mail_rrd}:quarantine:AVERAGE",
                    "DEF:max_quarantine=#{mail_rrd}:quarantine:MAX",
                    "CDEF:n_quarantine=quarantine",
                    "CDEF:real_quarantine=quarantine",
                    "CDEF:real_max_quarantine=max_quarantine",
                    "CDEF:real_n_quarantine=n_quarantine,8,*",
                    "CDEF:total_quarantine=PREV,UN,real_n_quarantine,PREV,IF,real_n_quarantine,+",
                    "DEF:abort=#{mail_rrd}:abort:AVERAGE",
                    "DEF:max_abort=#{mail_rrd}:abort:MAX",
                    "CDEF:n_abort=abort",
                    "CDEF:real_abort=abort",
                    "CDEF:real_max_abort=max_abort",
                    "CDEF:real_n_abort=n_abort,8,*",
                    "CDEF:total_abort=PREV,UN,real_n_abort,PREV,IF,real_n_abort,+",
                    "AREA:pass#0000ff:Pass      ",
                    "GPRINT:total_pass:MAX:total\\: %11.0lf mails",
                    "GPRINT:pass:AVERAGE:avg\\: %9.2lf mails/min",
                    "GPRINT:max_pass:MAX:max\\: %7.0lf mails/min\\l",
                    "STACK:accept#00ff00:Accept    ",
                    "GPRINT:total_accept:MAX:total\\: %11.0lf mails",
                    "GPRINT:accept:AVERAGE:avg\\: %9.2lf mails/min",
                    "GPRINT:max_accept:MAX:max\\: %7.0lf mails/min\\l",
                    "STACK:reject#ff0000:Reject    ",
                    "GPRINT:total_reject:MAX:total\\: %11.0lf mails",
                    "GPRINT:reject:AVERAGE:avg\\: %9.2lf mails/min",
                    "GPRINT:max_reject:MAX:max\\: %7.0lf mails/min\\l",
                    "STACK:discard#ffd400:Discard   ",
                    "GPRINT:total_discard:MAX:total\\: %11.0lf mails",
                    "GPRINT:discard:AVERAGE:avg\\: %9.2lf mails/min",
                    "GPRINT:max_discard:MAX:max\\: %7.0lf mails/min\\l",
                    "STACK:temporary-failure#888888:Temp-Fail ",
                    "GPRINT:total_temporary-failure:MAX:total\\: %11.0lf mails",
                    "GPRINT:temporary-failure:AVERAGE:avg\\: %9.2lf mails/min",
                    "GPRINT:max_temporary-failure:MAX:max\\: %7.0lf mails/min\\l",
                    "STACK:quarantine#a52a2a:Quarantine",
                    "GPRINT:total_quarantine:MAX:total\\: %11.0lf mails",
                    "GPRINT:quarantine:AVERAGE:avg\\: %9.2lf mails/min",
                    "GPRINT:max_quarantine:MAX:max\\: %7.0lf mails/min\\l",
                    "STACK:abort#ff9999:Abort     ",
                    "GPRINT:total_abort:MAX:total\\: %11.0lf mails",
                    "GPRINT:abort:AVERAGE:avg\\: %9.2lf mails/min",
                    "GPRINT:max_abort:MAX:max\\: %7.0lf mails/min\\l",
                    "DEF:smtp=#{session_rrd}:smtp:AVERAGE",
                    "DEF:max_smtp=#{session_rrd}:smtp:MAX",
                    "CDEF:n_smtp=smtp",
                    "CDEF:real_smtp=smtp",
                    "CDEF:real_max_smtp=max_smtp",
                    "CDEF:real_n_smtp=n_smtp,8,*",
                    "CDEF:total_smtp=PREV,UN,real_n_smtp,PREV,IF,real_n_smtp,+",
                    "LINE2:n_smtp#000000:SMTP      ",
                    "GPRINT:total_smtp:MAX:total\\: %8.0lf sessions",
                    "GPRINT:smtp:AVERAGE:avg\\: %6.2lf sessions/min",
                    "GPRINT:max_smtp:MAX:max\\: %4.0lf sessions/min\\l",
                    "COMMENT:[2009-05-21T01\\:31\\:00+09\\:00]\\r")

    mock(RRD).last(stop_rrd) {Time.at(1242837060)}
    mock(RRD).graph(stop_day_png,
                    "--title", "Stopped milters",
                    "--vertical-label", "milters/min",
                    "--start", "-86400",
                    "--end", day_end_time.to_s,
                    "--width", "600",
                    "--height", "200",
                    "--alt-y-grid",
                    "--units-exponent", "0",
                    "DEF:connect=#{stop_rrd}:connect:AVERAGE",
                    "DEF:max_connect=#{stop_rrd}:connect:MAX",
                    "CDEF:n_connect=connect",
                    "CDEF:real_connect=connect",
                    "CDEF:real_max_connect=max_connect",
                    "CDEF:real_n_connect=n_connect,8,*",
                    "CDEF:total_connect=PREV,UN,real_n_connect,PREV,IF,real_n_connect,+",
                    "DEF:helo=#{stop_rrd}:helo:AVERAGE",
                    "DEF:max_helo=#{stop_rrd}:helo:MAX",
                    "CDEF:n_helo=helo",
                    "CDEF:real_helo=helo",
                    "CDEF:real_max_helo=max_helo",
                    "CDEF:real_n_helo=n_helo,8,*",
                    "CDEF:total_helo=PREV,UN,real_n_helo,PREV,IF,real_n_helo,+",
                    "DEF:envelope-from=#{stop_rrd}:envelope-from:AVERAGE",
                    "DEF:max_envelope-from=#{stop_rrd}:envelope-from:MAX",
                    "CDEF:n_envelope-from=envelope-from",
                    "CDEF:real_envelope-from=envelope-from",
                    "CDEF:real_max_envelope-from=max_envelope-from",
                    "CDEF:real_n_envelope-from=n_envelope-from,8,*",
                    "CDEF:total_envelope-from=PREV,UN,real_n_envelope-from,PREV,IF,real_n_envelope-from,+",
                    "DEF:envelope-recipient=#{stop_rrd}:envelope-recipient:AVERAGE",
                    "DEF:max_envelope-recipient=#{stop_rrd}:envelope-recipient:MAX",
                    "CDEF:n_envelope-recipient=envelope-recipient",
                    "CDEF:real_envelope-recipient=envelope-recipient",
                    "CDEF:real_max_envelope-recipient=max_envelope-recipient",
                    "CDEF:real_n_envelope-recipient=n_envelope-recipient,8,*",
                    "CDEF:total_envelope-recipient=PREV,UN,real_n_envelope-recipient,PREV,IF,real_n_envelope-recipient,+",
                     "DEF:header=#{stop_rrd}:header:AVERAGE",
                     "DEF:max_header=#{stop_rrd}:header:MAX",
                     "CDEF:n_header=header",
                     "CDEF:real_header=header",
                     "CDEF:real_max_header=max_header",
                     "CDEF:real_n_header=n_header,8,*",
                     "CDEF:total_header=PREV,UN,real_n_header,PREV,IF,real_n_header,+",
                     "DEF:body=#{stop_rrd}:body:AVERAGE",
                     "DEF:max_body=#{stop_rrd}:body:MAX",
                     "CDEF:n_body=body",
                     "CDEF:real_body=body",
                     "CDEF:real_max_body=max_body",
                     "CDEF:real_n_body=n_body,8,*",
                     "CDEF:total_body=PREV,UN,real_n_body,PREV,IF,real_n_body,+",
                     "DEF:end-of-message=#{stop_rrd}:end-of-message:AVERAGE",
                     "DEF:max_end-of-message=#{stop_rrd}:end-of-message:MAX",
                     "CDEF:n_end-of-message=end-of-message",
                     "CDEF:real_end-of-message=end-of-message",
                     "CDEF:real_max_end-of-message=max_end-of-message",
                     "CDEF:real_n_end-of-message=n_end-of-message,8,*",
                     "CDEF:total_end-of-message=PREV,UN,real_n_end-of-message,PREV,IF,real_n_end-of-message,+",
                     "AREA:connect#0000ff:connect           ",
                     "GPRINT:total_connect:MAX:total\\: %8.0lf milters",
                     "GPRINT:connect:AVERAGE:avg\\: %6.2lf milters/min",
                     "GPRINT:max_connect:MAX:max\\: %4.0lf milters/min\\l",
                     "STACK:helo#ff00ff:helo              ",
                     "GPRINT:total_helo:MAX:total\\: %8.0lf milters",
                     "GPRINT:helo:AVERAGE:avg\\: %6.2lf milters/min",
                     "GPRINT:max_helo:MAX:max\\: %4.0lf milters/min\\l",
                     "STACK:envelope-from#00ffff:envelope-from     ",
                     "GPRINT:total_envelope-from:MAX:total\\: %8.0lf milters",
                     "GPRINT:envelope-from:AVERAGE:avg\\: %6.2lf milters/min",
                     "GPRINT:max_envelope-from:MAX:max\\: %4.0lf milters/min\\l",
                     "STACK:envelope-recipient#ffff00:envelope-recipient",
                     "GPRINT:total_envelope-recipient:MAX:total\\: %8.0lf milters",
                     "GPRINT:envelope-recipient:AVERAGE:avg\\: %6.2lf milters/min",
                     "GPRINT:max_envelope-recipient:MAX:max\\: %4.0lf milters/min\\l",
                     "STACK:header#a52a2a:header            ",
                     "GPRINT:total_header:MAX:total\\: %8.0lf milters",
                     "GPRINT:header:AVERAGE:avg\\: %6.2lf milters/min",
                     "GPRINT:max_header:MAX:max\\: %4.0lf milters/min\\l",
                     "STACK:body#ff0000:body              ",
                     "GPRINT:total_body:MAX:total\\: %8.0lf milters",
                     "GPRINT:body:AVERAGE:avg\\: %6.2lf milters/min",
                     "GPRINT:max_body:MAX:max\\: %4.0lf milters/min\\l",
                     "STACK:end-of-message#00ff00:end-of-message    ",
                     "GPRINT:total_end-of-message:MAX:total\\: %8.0lf milters",
                     "GPRINT:end-of-message:AVERAGE:avg\\: %6.2lf milters/min",
                     "GPRINT:max_end-of-message:MAX:max\\: %4.0lf milters/min\\l",
                     "DEF:milter=#{session_rrd}:child:AVERAGE",
                     "DEF:max_milter=#{session_rrd}:child:MAX",
                     "CDEF:n_milter=milter",
                     "CDEF:real_milter=milter",
                     "CDEF:real_max_milter=max_milter",
                     "CDEF:real_n_milter=n_milter,8,*",
                     "CDEF:total_milter=PREV,UN,real_n_milter,PREV,IF,real_n_milter,+",
                     "LINE2:n_milter#000000:total             ",
                     "GPRINT:total_milter:MAX:total\\: %8.0lf milters",
                     "GPRINT:milter:AVERAGE:avg\\: %6.2lf milters/min",
                     "GPRINT:max_milter:MAX:max\\: %4.0lf milters/min\\l",
                     "COMMENT:[2009-05-21T01\\:31\\:00+09\\:00]\\r")

    mock(RRD).last(session_rrd) {Time.at(1242837060)}
    mock(RRD).graph(session_week_png,
                    "--title", "Sessions",
                    "--vertical-label", "sessions/min",
                    "--start", "-604800",
                    "--end", week_end_time.to_s,
                    "--width", "600",
                    "--height", "200",
                    "--alt-y-grid",
                    "--units-exponent", "0",
                    "DEF:smtp=#{session_rrd}:smtp:AVERAGE",
                    "DEF:max_smtp=#{session_rrd}:smtp:MAX",
                    "CDEF:n_smtp=smtp",
                    "CDEF:real_smtp=smtp",
                    "CDEF:real_max_smtp=max_smtp",
                    "CDEF:real_n_smtp=n_smtp,56,*",
                    "CDEF:total_smtp=PREV,UN,real_n_smtp,PREV,IF,real_n_smtp,+",
                    "DEF:child=#{session_rrd}:child:AVERAGE",
                    "DEF:max_child=#{session_rrd}:child:MAX",
                    "CDEF:n_child=child",
                    "CDEF:real_child=child",
                    "CDEF:real_max_child=max_child",
                    "CDEF:real_n_child=n_child,56,*",
                    "CDEF:total_child=PREV,UN,real_n_child,PREV,IF,real_n_child,+",
                    "AREA:n_smtp#0000ff:SMTP  ",
                    "GPRINT:total_smtp:MAX:total\\: %8.0lf sessions",
                    "GPRINT:smtp:AVERAGE:avg\\: %6.2lf sessions/min",
                    "GPRINT:max_smtp:MAX:max\\: %4.0lf sessions/min\\l",
                    "LINE2:n_child#00ff00:milter",
                    "GPRINT:total_child:MAX:total\\: %8.0lf sessions",
                    "GPRINT:child:AVERAGE:avg\\: %6.2lf sessions/min",
                    "GPRINT:max_child:MAX:max\\: %4.0lf sessions/min\\l",
                    "COMMENT:[2009-05-21T01\\:31\\:00+09\\:00]\\r")

    mock(RRD).last(mail_rrd) {Time.at(1242837060)}
    mock(RRD).graph(mail_week_png,
                    "--title", "Processed mails",
                    "--vertical-label", "mails/min",
                    "--start", "-604800",
                    "--end", week_end_time.to_s,
                    "--width", "600",
                    "--height", "200",
                    "--alt-y-grid",
                    "--units-exponent", "0",
                    "DEF:pass=#{mail_rrd}:pass:AVERAGE",
                    "DEF:max_pass=#{mail_rrd}:pass:MAX",
                    "CDEF:n_pass=pass",
                    "CDEF:real_pass=pass",
                    "CDEF:real_max_pass=max_pass",
                    "CDEF:real_n_pass=n_pass,56,*",
                    "CDEF:total_pass=PREV,UN,real_n_pass,PREV,IF,real_n_pass,+",
                    "DEF:accept=#{mail_rrd}:accept:AVERAGE",
                    "DEF:max_accept=#{mail_rrd}:accept:MAX",
                    "CDEF:n_accept=accept",
                    "CDEF:real_accept=accept",
                    "CDEF:real_max_accept=max_accept",
                    "CDEF:real_n_accept=n_accept,56,*",
                    "CDEF:total_accept=PREV,UN,real_n_accept,PREV,IF,real_n_accept,+",
                    "DEF:reject=#{mail_rrd}:reject:AVERAGE",
                    "DEF:max_reject=#{mail_rrd}:reject:MAX",
                    "CDEF:n_reject=reject",
                    "CDEF:real_reject=reject",
                    "CDEF:real_max_reject=max_reject",
                    "CDEF:real_n_reject=n_reject,56,*",
                    "CDEF:total_reject=PREV,UN,real_n_reject,PREV,IF,real_n_reject,+",
                    "DEF:discard=#{mail_rrd}:discard:AVERAGE",
                    "DEF:max_discard=#{mail_rrd}:discard:MAX",
                    "CDEF:n_discard=discard",
                    "CDEF:real_discard=discard",
                    "CDEF:real_max_discard=max_discard",
                    "CDEF:real_n_discard=n_discard,56,*",
                    "CDEF:total_discard=PREV,UN,real_n_discard,PREV,IF,real_n_discard,+",
                    "DEF:temporary-failure=#{mail_rrd}:temporary-failure:AVERAGE",
                    "DEF:max_temporary-failure=#{mail_rrd}:temporary-failure:MAX",
                    "CDEF:n_temporary-failure=temporary-failure",
                    "CDEF:real_temporary-failure=temporary-failure",
                    "CDEF:real_max_temporary-failure=max_temporary-failure",
                    "CDEF:real_n_temporary-failure=n_temporary-failure,56,*",
                    "CDEF:total_temporary-failure=PREV,UN,real_n_temporary-failure,PREV,IF,real_n_temporary-failure,+",
                    "DEF:quarantine=#{mail_rrd}:quarantine:AVERAGE",
                    "DEF:max_quarantine=#{mail_rrd}:quarantine:MAX",
                    "CDEF:n_quarantine=quarantine",
                    "CDEF:real_quarantine=quarantine",
                    "CDEF:real_max_quarantine=max_quarantine",
                    "CDEF:real_n_quarantine=n_quarantine,56,*",
                    "CDEF:total_quarantine=PREV,UN,real_n_quarantine,PREV,IF,real_n_quarantine,+",
                    "DEF:abort=#{mail_rrd}:abort:AVERAGE",
                    "DEF:max_abort=#{mail_rrd}:abort:MAX",
                    "CDEF:n_abort=abort",
                    "CDEF:real_abort=abort",
                    "CDEF:real_max_abort=max_abort",
                    "CDEF:real_n_abort=n_abort,56,*",
                    "CDEF:total_abort=PREV,UN,real_n_abort,PREV,IF,real_n_abort,+",
                    "AREA:pass#0000ff:Pass      ",
                    "GPRINT:total_pass:MAX:total\\: %11.0lf mails",
                    "GPRINT:pass:AVERAGE:avg\\: %9.2lf mails/min",
                    "GPRINT:max_pass:MAX:max\\: %7.0lf mails/min\\l",
                    "STACK:accept#00ff00:Accept    ",
                    "GPRINT:total_accept:MAX:total\\: %11.0lf mails",
                    "GPRINT:accept:AVERAGE:avg\\: %9.2lf mails/min",
                    "GPRINT:max_accept:MAX:max\\: %7.0lf mails/min\\l",
                    "STACK:reject#ff0000:Reject    ",
                    "GPRINT:total_reject:MAX:total\\: %11.0lf mails",
                    "GPRINT:reject:AVERAGE:avg\\: %9.2lf mails/min",
                    "GPRINT:max_reject:MAX:max\\: %7.0lf mails/min\\l",
                    "STACK:discard#ffd400:Discard   ",
                    "GPRINT:total_discard:MAX:total\\: %11.0lf mails",
                    "GPRINT:discard:AVERAGE:avg\\: %9.2lf mails/min",
                    "GPRINT:max_discard:MAX:max\\: %7.0lf mails/min\\l",
                    "STACK:temporary-failure#888888:Temp-Fail ",
                    "GPRINT:total_temporary-failure:MAX:total\\: %11.0lf mails",
                    "GPRINT:temporary-failure:AVERAGE:avg\\: %9.2lf mails/min",
                    "GPRINT:max_temporary-failure:MAX:max\\: %7.0lf mails/min\\l",
                    "STACK:quarantine#a52a2a:Quarantine",
                    "GPRINT:total_quarantine:MAX:total\\: %11.0lf mails",
                    "GPRINT:quarantine:AVERAGE:avg\\: %9.2lf mails/min",
                    "GPRINT:max_quarantine:MAX:max\\: %7.0lf mails/min\\l",
                    "STACK:abort#ff9999:Abort     ",
                    "GPRINT:total_abort:MAX:total\\: %11.0lf mails",
                    "GPRINT:abort:AVERAGE:avg\\: %9.2lf mails/min",
                    "GPRINT:max_abort:MAX:max\\: %7.0lf mails/min\\l",
                    "DEF:smtp=#{session_rrd}:smtp:AVERAGE",
                    "DEF:max_smtp=#{session_rrd}:smtp:MAX",
                    "CDEF:n_smtp=smtp",
                    "CDEF:real_smtp=smtp",
                    "CDEF:real_max_smtp=max_smtp",
                    "CDEF:real_n_smtp=n_smtp,56,*",
                    "CDEF:total_smtp=PREV,UN,real_n_smtp,PREV,IF,real_n_smtp,+",
                    "LINE2:n_smtp#000000:SMTP      ",
                    "GPRINT:total_smtp:MAX:total\\: %8.0lf sessions",
                    "GPRINT:smtp:AVERAGE:avg\\: %6.2lf sessions/min",
                    "GPRINT:max_smtp:MAX:max\\: %4.0lf sessions/min\\l",
                    "COMMENT:[2009-05-21T01\\:31\\:00+09\\:00]\\r")

    mock(RRD).last(stop_rrd) {Time.at(1242837060)}
    mock(RRD).graph(stop_week_png,
                    "--title", "Stopped milters",
                    "--vertical-label", "milters/min",
                    "--start", "-604800",
                    "--end", week_end_time.to_s,
                    "--width", "600",
                    "--height", "200",
                    "--alt-y-grid",
                    "--units-exponent", "0",
                    "DEF:connect=#{stop_rrd}:connect:AVERAGE",
                    "DEF:max_connect=#{stop_rrd}:connect:MAX",
                    "CDEF:n_connect=connect",
                    "CDEF:real_connect=connect",
                    "CDEF:real_max_connect=max_connect",
                    "CDEF:real_n_connect=n_connect,56,*",
                    "CDEF:total_connect=PREV,UN,real_n_connect,PREV,IF,real_n_connect,+",
                    "DEF:helo=#{stop_rrd}:helo:AVERAGE",
                    "DEF:max_helo=#{stop_rrd}:helo:MAX",
                    "CDEF:n_helo=helo",
                    "CDEF:real_helo=helo",
                    "CDEF:real_max_helo=max_helo",
                    "CDEF:real_n_helo=n_helo,56,*",
                    "CDEF:total_helo=PREV,UN,real_n_helo,PREV,IF,real_n_helo,+",
                    "DEF:envelope-from=#{stop_rrd}:envelope-from:AVERAGE",
                    "DEF:max_envelope-from=#{stop_rrd}:envelope-from:MAX",
                    "CDEF:n_envelope-from=envelope-from",
                    "CDEF:real_envelope-from=envelope-from",
                    "CDEF:real_max_envelope-from=max_envelope-from",
                    "CDEF:real_n_envelope-from=n_envelope-from,56,*",
                    "CDEF:total_envelope-from=PREV,UN,real_n_envelope-from,PREV,IF,real_n_envelope-from,+",
                    "DEF:envelope-recipient=#{stop_rrd}:envelope-recipient:AVERAGE",
                    "DEF:max_envelope-recipient=#{stop_rrd}:envelope-recipient:MAX",
                    "CDEF:n_envelope-recipient=envelope-recipient",
                    "CDEF:real_envelope-recipient=envelope-recipient",
                    "CDEF:real_max_envelope-recipient=max_envelope-recipient",
                    "CDEF:real_n_envelope-recipient=n_envelope-recipient,56,*",
                    "CDEF:total_envelope-recipient=PREV,UN,real_n_envelope-recipient,PREV,IF,real_n_envelope-recipient,+",
                    "DEF:header=#{stop_rrd}:header:AVERAGE",
                    "DEF:max_header=#{stop_rrd}:header:MAX",
                    "CDEF:n_header=header",
                    "CDEF:real_header=header",
                    "CDEF:real_max_header=max_header",
                    "CDEF:real_n_header=n_header,56,*",
                    "CDEF:total_header=PREV,UN,real_n_header,PREV,IF,real_n_header,+",
                    "DEF:body=#{stop_rrd}:body:AVERAGE",
                    "DEF:max_body=#{stop_rrd}:body:MAX",
                    "CDEF:n_body=body",
                    "CDEF:real_body=body",
                    "CDEF:real_max_body=max_body",
                    "CDEF:real_n_body=n_body,56,*",
                    "CDEF:total_body=PREV,UN,real_n_body,PREV,IF,real_n_body,+",
                    "DEF:end-of-message=#{stop_rrd}:end-of-message:AVERAGE",
                    "DEF:max_end-of-message=#{stop_rrd}:end-of-message:MAX",
                    "CDEF:n_end-of-message=end-of-message",
                    "CDEF:real_end-of-message=end-of-message",
                    "CDEF:real_max_end-of-message=max_end-of-message",
                    "CDEF:real_n_end-of-message=n_end-of-message,56,*",
                    "CDEF:total_end-of-message=PREV,UN,real_n_end-of-message,PREV,IF,real_n_end-of-message,+",
                    "AREA:connect#0000ff:connect           ",
                    "GPRINT:total_connect:MAX:total\\: %8.0lf milters",
                    "GPRINT:connect:AVERAGE:avg\\: %6.2lf milters/min",
                    "GPRINT:max_connect:MAX:max\\: %4.0lf milters/min\\l",
                    "STACK:helo#ff00ff:helo              ",
                    "GPRINT:total_helo:MAX:total\\: %8.0lf milters",
                    "GPRINT:helo:AVERAGE:avg\\: %6.2lf milters/min",
                    "GPRINT:max_helo:MAX:max\\: %4.0lf milters/min\\l",
                    "STACK:envelope-from#00ffff:envelope-from     ",
                    "GPRINT:total_envelope-from:MAX:total\\: %8.0lf milters",
                    "GPRINT:envelope-from:AVERAGE:avg\\: %6.2lf milters/min",
                    "GPRINT:max_envelope-from:MAX:max\\: %4.0lf milters/min\\l",
                    "STACK:envelope-recipient#ffff00:envelope-recipient",
                    "GPRINT:total_envelope-recipient:MAX:total\\: %8.0lf milters",
                    "GPRINT:envelope-recipient:AVERAGE:avg\\: %6.2lf milters/min",
                    "GPRINT:max_envelope-recipient:MAX:max\\: %4.0lf milters/min\\l",
                    "STACK:header#a52a2a:header            ",
                    "GPRINT:total_header:MAX:total\\: %8.0lf milters",
                    "GPRINT:header:AVERAGE:avg\\: %6.2lf milters/min",
                    "GPRINT:max_header:MAX:max\\: %4.0lf milters/min\\l",
                    "STACK:body#ff0000:body              ",
                    "GPRINT:total_body:MAX:total\\: %8.0lf milters",
                    "GPRINT:body:AVERAGE:avg\\: %6.2lf milters/min",
                    "GPRINT:max_body:MAX:max\\: %4.0lf milters/min\\l",
                    "STACK:end-of-message#00ff00:end-of-message    ",
                    "GPRINT:total_end-of-message:MAX:total\\: %8.0lf milters",
                    "GPRINT:end-of-message:AVERAGE:avg\\: %6.2lf milters/min",
                    "GPRINT:max_end-of-message:MAX:max\\: %4.0lf milters/min\\l",
                    "DEF:milter=#{session_rrd}:child:AVERAGE",
                    "DEF:max_milter=#{session_rrd}:child:MAX",
                    "CDEF:n_milter=milter",
                    "CDEF:real_milter=milter",
                    "CDEF:real_max_milter=max_milter",
                    "CDEF:real_n_milter=n_milter,56,*",
                    "CDEF:total_milter=PREV,UN,real_n_milter,PREV,IF,real_n_milter,+",
                    "LINE2:n_milter#000000:total             ",
                    "GPRINT:total_milter:MAX:total\\: %8.0lf milters",
                    "GPRINT:milter:AVERAGE:avg\\: %6.2lf milters/min",
                    "GPRINT:max_milter:MAX:max\\: %4.0lf milters/min\\l",
                    "COMMENT:[2009-05-21T01\\:31\\:00+09\\:00]\\r")

    mock(RRD).last(session_rrd) {Time.at(1242837060)}
    mock(RRD).graph(session_month_png,
                    "--title", "Sessions",
                    "--vertical-label", "sessions/min",
                    "--start", "-3024000",
                    "--end", month_end_time.to_s,
                    "--width", "600",
                    "--height", "200",
                    "--alt-y-grid",
                    "--units-exponent", "0",
                    "DEF:smtp=#{session_rrd}:smtp:AVERAGE",
                    "DEF:max_smtp=#{session_rrd}:smtp:MAX",
                    "CDEF:n_smtp=smtp",
                    "CDEF:real_smtp=smtp",
                    "CDEF:real_max_smtp=max_smtp",
                    "CDEF:real_n_smtp=n_smtp,280,*",
                    "CDEF:total_smtp=PREV,UN,real_n_smtp,PREV,IF,real_n_smtp,+",
                    "DEF:child=#{session_rrd}:child:AVERAGE",
                    "DEF:max_child=#{session_rrd}:child:MAX",
                    "CDEF:n_child=child",
                    "CDEF:real_child=child",
                    "CDEF:real_max_child=max_child",
                    "CDEF:real_n_child=n_child,280,*",
                    "CDEF:total_child=PREV,UN,real_n_child,PREV,IF,real_n_child,+",
                    "AREA:n_smtp#0000ff:SMTP  ",
                    "GPRINT:total_smtp:MAX:total\\: %8.0lf sessions",
                    "GPRINT:smtp:AVERAGE:avg\\: %6.2lf sessions/min",
                    "GPRINT:max_smtp:MAX:max\\: %4.0lf sessions/min\\l",
                    "LINE2:n_child#00ff00:milter",
                    "GPRINT:total_child:MAX:total\\: %8.0lf sessions",
                    "GPRINT:child:AVERAGE:avg\\: %6.2lf sessions/min",
                    "GPRINT:max_child:MAX:max\\: %4.0lf sessions/min\\l",
                    "COMMENT:[2009-05-21T01\\:31\\:00+09\\:00]\\r")

    mock(RRD).last(mail_rrd) {Time.at(1242837060)}
    mock(RRD).graph(mail_month_png,
                    "--title", "Processed mails",
                    "--vertical-label", "mails/min",
                    "--start", "-3024000",
                    "--end", month_end_time.to_s,
                    "--width", "600",
                    "--height", "200",
                    "--alt-y-grid",
                    "--units-exponent", "0",
                    "DEF:pass=#{mail_rrd}:pass:AVERAGE",
                    "DEF:max_pass=#{mail_rrd}:pass:MAX",
                    "CDEF:n_pass=pass",
                    "CDEF:real_pass=pass",
                    "CDEF:real_max_pass=max_pass",
                    "CDEF:real_n_pass=n_pass,280,*",
                    "CDEF:total_pass=PREV,UN,real_n_pass,PREV,IF,real_n_pass,+",
                    "DEF:accept=#{mail_rrd}:accept:AVERAGE",
                    "DEF:max_accept=#{mail_rrd}:accept:MAX",
                    "CDEF:n_accept=accept",
                    "CDEF:real_accept=accept",
                    "CDEF:real_max_accept=max_accept",
                    "CDEF:real_n_accept=n_accept,280,*",
                    "CDEF:total_accept=PREV,UN,real_n_accept,PREV,IF,real_n_accept,+",
                    "DEF:reject=#{mail_rrd}:reject:AVERAGE",
                    "DEF:max_reject=#{mail_rrd}:reject:MAX",
                    "CDEF:n_reject=reject",
                    "CDEF:real_reject=reject",
                    "CDEF:real_max_reject=max_reject",
                    "CDEF:real_n_reject=n_reject,280,*",
                    "CDEF:total_reject=PREV,UN,real_n_reject,PREV,IF,real_n_reject,+",
                    "DEF:discard=#{mail_rrd}:discard:AVERAGE",
                    "DEF:max_discard=#{mail_rrd}:discard:MAX",
                    "CDEF:n_discard=discard",
                    "CDEF:real_discard=discard",
                    "CDEF:real_max_discard=max_discard",
                    "CDEF:real_n_discard=n_discard,280,*",
                    "CDEF:total_discard=PREV,UN,real_n_discard,PREV,IF,real_n_discard,+",
                    "DEF:temporary-failure=#{mail_rrd}:temporary-failure:AVERAGE",
                    "DEF:max_temporary-failure=#{mail_rrd}:temporary-failure:MAX",
                    "CDEF:n_temporary-failure=temporary-failure",
                    "CDEF:real_temporary-failure=temporary-failure",
                    "CDEF:real_max_temporary-failure=max_temporary-failure",
                    "CDEF:real_n_temporary-failure=n_temporary-failure,280,*",
                    "CDEF:total_temporary-failure=PREV,UN,real_n_temporary-failure,PREV,IF,real_n_temporary-failure,+",
                    "DEF:quarantine=#{mail_rrd}:quarantine:AVERAGE",
                    "DEF:max_quarantine=#{mail_rrd}:quarantine:MAX",
                    "CDEF:n_quarantine=quarantine",
                    "CDEF:real_quarantine=quarantine",
                    "CDEF:real_max_quarantine=max_quarantine",
                    "CDEF:real_n_quarantine=n_quarantine,280,*",
                    "CDEF:total_quarantine=PREV,UN,real_n_quarantine,PREV,IF,real_n_quarantine,+",
                    "DEF:abort=#{mail_rrd}:abort:AVERAGE",
                    "DEF:max_abort=#{mail_rrd}:abort:MAX",
                    "CDEF:n_abort=abort",
                    "CDEF:real_abort=abort",
                    "CDEF:real_max_abort=max_abort",
                    "CDEF:real_n_abort=n_abort,280,*",
                    "CDEF:total_abort=PREV,UN,real_n_abort,PREV,IF,real_n_abort,+",
                    "AREA:pass#0000ff:Pass      ",
                    "GPRINT:total_pass:MAX:total\\: %11.0lf mails",
                    "GPRINT:pass:AVERAGE:avg\\: %9.2lf mails/min",
                    "GPRINT:max_pass:MAX:max\\: %7.0lf mails/min\\l",
                    "STACK:accept#00ff00:Accept    ",
                    "GPRINT:total_accept:MAX:total\\: %11.0lf mails",
                    "GPRINT:accept:AVERAGE:avg\\: %9.2lf mails/min",
                    "GPRINT:max_accept:MAX:max\\: %7.0lf mails/min\\l",
                    "STACK:reject#ff0000:Reject    ",
                    "GPRINT:total_reject:MAX:total\\: %11.0lf mails",
                    "GPRINT:reject:AVERAGE:avg\\: %9.2lf mails/min",
                    "GPRINT:max_reject:MAX:max\\: %7.0lf mails/min\\l",
                    "STACK:discard#ffd400:Discard   ",
                    "GPRINT:total_discard:MAX:total\\: %11.0lf mails",
                    "GPRINT:discard:AVERAGE:avg\\: %9.2lf mails/min",
                    "GPRINT:max_discard:MAX:max\\: %7.0lf mails/min\\l",
                    "STACK:temporary-failure#888888:Temp-Fail ",
                    "GPRINT:total_temporary-failure:MAX:total\\: %11.0lf mails",
                    "GPRINT:temporary-failure:AVERAGE:avg\\: %9.2lf mails/min",
                    "GPRINT:max_temporary-failure:MAX:max\\: %7.0lf mails/min\\l",
                    "STACK:quarantine#a52a2a:Quarantine",
                    "GPRINT:total_quarantine:MAX:total\\: %11.0lf mails",
                    "GPRINT:quarantine:AVERAGE:avg\\: %9.2lf mails/min",
                    "GPRINT:max_quarantine:MAX:max\\: %7.0lf mails/min\\l",
                    "STACK:abort#ff9999:Abort     ",
                    "GPRINT:total_abort:MAX:total\\: %11.0lf mails",
                    "GPRINT:abort:AVERAGE:avg\\: %9.2lf mails/min",
                    "GPRINT:max_abort:MAX:max\\: %7.0lf mails/min\\l",
                    "DEF:smtp=#{session_rrd}:smtp:AVERAGE",
                    "DEF:max_smtp=#{session_rrd}:smtp:MAX",
                    "CDEF:n_smtp=smtp",
                    "CDEF:real_smtp=smtp",
                    "CDEF:real_max_smtp=max_smtp",
                    "CDEF:real_n_smtp=n_smtp,280,*",
                    "CDEF:total_smtp=PREV,UN,real_n_smtp,PREV,IF,real_n_smtp,+",
                    "LINE2:n_smtp#000000:SMTP      ",
                    "GPRINT:total_smtp:MAX:total\\: %8.0lf sessions",
                    "GPRINT:smtp:AVERAGE:avg\\: %6.2lf sessions/min",
                    "GPRINT:max_smtp:MAX:max\\: %4.0lf sessions/min\\l",
                    "COMMENT:[2009-05-21T01\\:31\\:00+09\\:00]\\r")

    mock(RRD).last(stop_rrd) {Time.at(1242837060)}
    mock(RRD).graph(stop_month_png,
                    "--title", "Stopped milters",
                    "--vertical-label", "milters/min",
                    "--start", "-3024000",
                    "--end", month_end_time.to_s,
                    "--width", "600",
                    "--height", "200",
                    "--alt-y-grid",
                    "--units-exponent", "0",
                    "DEF:connect=#{stop_rrd}:connect:AVERAGE",
                    "DEF:max_connect=#{stop_rrd}:connect:MAX",
                    "CDEF:n_connect=connect",
                    "CDEF:real_connect=connect",
                    "CDEF:real_max_connect=max_connect",
                    "CDEF:real_n_connect=n_connect,280,*",
                    "CDEF:total_connect=PREV,UN,real_n_connect,PREV,IF,real_n_connect,+",
                    "DEF:helo=#{stop_rrd}:helo:AVERAGE",
                    "DEF:max_helo=#{stop_rrd}:helo:MAX",
                    "CDEF:n_helo=helo",
                    "CDEF:real_helo=helo",
                    "CDEF:real_max_helo=max_helo",
                    "CDEF:real_n_helo=n_helo,280,*",
                    "CDEF:total_helo=PREV,UN,real_n_helo,PREV,IF,real_n_helo,+",
                    "DEF:envelope-from=#{stop_rrd}:envelope-from:AVERAGE",
                    "DEF:max_envelope-from=#{stop_rrd}:envelope-from:MAX",
                    "CDEF:n_envelope-from=envelope-from",
                    "CDEF:real_envelope-from=envelope-from",
                    "CDEF:real_max_envelope-from=max_envelope-from",
                    "CDEF:real_n_envelope-from=n_envelope-from,280,*",
                    "CDEF:total_envelope-from=PREV,UN,real_n_envelope-from,PREV,IF,real_n_envelope-from,+",
                    "DEF:envelope-recipient=#{stop_rrd}:envelope-recipient:AVERAGE",
                    "DEF:max_envelope-recipient=#{stop_rrd}:envelope-recipient:MAX",
                    "CDEF:n_envelope-recipient=envelope-recipient",
                    "CDEF:real_envelope-recipient=envelope-recipient",
                    "CDEF:real_max_envelope-recipient=max_envelope-recipient",
                    "CDEF:real_n_envelope-recipient=n_envelope-recipient,280,*",
                    "CDEF:total_envelope-recipient=PREV,UN,real_n_envelope-recipient,PREV,IF,real_n_envelope-recipient,+",
                    "DEF:header=#{stop_rrd}:header:AVERAGE",
                    "DEF:max_header=#{stop_rrd}:header:MAX",
                    "CDEF:n_header=header",
                    "CDEF:real_header=header",
                    "CDEF:real_max_header=max_header",
                    "CDEF:real_n_header=n_header,280,*",
                    "CDEF:total_header=PREV,UN,real_n_header,PREV,IF,real_n_header,+",
                    "DEF:body=#{stop_rrd}:body:AVERAGE",
                    "DEF:max_body=#{stop_rrd}:body:MAX",
                    "CDEF:n_body=body",
                    "CDEF:real_body=body",
                    "CDEF:real_max_body=max_body",
                    "CDEF:real_n_body=n_body,280,*",
                    "CDEF:total_body=PREV,UN,real_n_body,PREV,IF,real_n_body,+",
                    "DEF:end-of-message=#{stop_rrd}:end-of-message:AVERAGE",
                    "DEF:max_end-of-message=#{stop_rrd}:end-of-message:MAX",
                    "CDEF:n_end-of-message=end-of-message",
                    "CDEF:real_end-of-message=end-of-message",
                    "CDEF:real_max_end-of-message=max_end-of-message",
                    "CDEF:real_n_end-of-message=n_end-of-message,280,*",
                    "CDEF:total_end-of-message=PREV,UN,real_n_end-of-message,PREV,IF,real_n_end-of-message,+",
                    "AREA:connect#0000ff:connect           ",
                    "GPRINT:total_connect:MAX:total\\: %8.0lf milters",
                    "GPRINT:connect:AVERAGE:avg\\: %6.2lf milters/min",
                    "GPRINT:max_connect:MAX:max\\: %4.0lf milters/min\\l",
                    "STACK:helo#ff00ff:helo              ",
                    "GPRINT:total_helo:MAX:total\\: %8.0lf milters",
                    "GPRINT:helo:AVERAGE:avg\\: %6.2lf milters/min",
                    "GPRINT:max_helo:MAX:max\\: %4.0lf milters/min\\l",
                    "STACK:envelope-from#00ffff:envelope-from     ",
                    "GPRINT:total_envelope-from:MAX:total\\: %8.0lf milters",
                    "GPRINT:envelope-from:AVERAGE:avg\\: %6.2lf milters/min",
                    "GPRINT:max_envelope-from:MAX:max\\: %4.0lf milters/min\\l",
                    "STACK:envelope-recipient#ffff00:envelope-recipient",
                    "GPRINT:total_envelope-recipient:MAX:total\\: %8.0lf milters",
                    "GPRINT:envelope-recipient:AVERAGE:avg\\: %6.2lf milters/min",
                    "GPRINT:max_envelope-recipient:MAX:max\\: %4.0lf milters/min\\l",
                    "STACK:header#a52a2a:header            ",
                    "GPRINT:total_header:MAX:total\\: %8.0lf milters",
                    "GPRINT:header:AVERAGE:avg\\: %6.2lf milters/min",
                    "GPRINT:max_header:MAX:max\\: %4.0lf milters/min\\l",
                    "STACK:body#ff0000:body              ",
                    "GPRINT:total_body:MAX:total\\: %8.0lf milters",
                    "GPRINT:body:AVERAGE:avg\\: %6.2lf milters/min",
                    "GPRINT:max_body:MAX:max\\: %4.0lf milters/min\\l",
                    "STACK:end-of-message#00ff00:end-of-message    ",
                    "GPRINT:total_end-of-message:MAX:total\\: %8.0lf milters",
                    "GPRINT:end-of-message:AVERAGE:avg\\: %6.2lf milters/min",
                    "GPRINT:max_end-of-message:MAX:max\\: %4.0lf milters/min\\l",
                    "DEF:milter=#{session_rrd}:child:AVERAGE",
                    "DEF:max_milter=#{session_rrd}:child:MAX",
                    "CDEF:n_milter=milter",
                    "CDEF:real_milter=milter",
                    "CDEF:real_max_milter=max_milter",
                    "CDEF:real_n_milter=n_milter,280,*",
                    "CDEF:total_milter=PREV,UN,real_n_milter,PREV,IF,real_n_milter,+",
                    "LINE2:n_milter#000000:total             ",
                    "GPRINT:total_milter:MAX:total\\: %8.0lf milters",
                    "GPRINT:milter:AVERAGE:avg\\: %6.2lf milters/min",
                    "GPRINT:max_milter:MAX:max\\: %4.0lf milters/min\\l",
                    "COMMENT:[2009-05-21T01\\:31\\:00+09\\:00]\\r")

    mock(RRD).last(session_rrd) {Time.at(1242837060)}
    mock(RRD).graph(session_year_png,
                    "--title", "Sessions",
                    "--vertical-label", "sessions/min",
                    "--start", "-36288000",
                    "--end", year_end_time.to_s,
                    "--width", "600",
                    "--height", "200",
                    "--alt-y-grid",
                    "--units-exponent", "0",
                    "DEF:smtp=#{session_rrd}:smtp:AVERAGE",
                    "DEF:max_smtp=#{session_rrd}:smtp:MAX",
                    "CDEF:n_smtp=smtp",
                    "CDEF:real_smtp=smtp",
                    "CDEF:real_max_smtp=max_smtp",
                    "CDEF:real_n_smtp=n_smtp,3360,*",
                    "CDEF:total_smtp=PREV,UN,real_n_smtp,PREV,IF,real_n_smtp,+",
                    "DEF:child=#{session_rrd}:child:AVERAGE",
                    "DEF:max_child=#{session_rrd}:child:MAX",
                    "CDEF:n_child=child",
                    "CDEF:real_child=child",
                    "CDEF:real_max_child=max_child",
                    "CDEF:real_n_child=n_child,3360,*",
                    "CDEF:total_child=PREV,UN,real_n_child,PREV,IF,real_n_child,+",
                    "AREA:n_smtp#0000ff:SMTP  ",
                    "GPRINT:total_smtp:MAX:total\\: %8.0lf sessions",
                    "GPRINT:smtp:AVERAGE:avg\\: %6.2lf sessions/min",
                    "GPRINT:max_smtp:MAX:max\\: %4.0lf sessions/min\\l",
                    "LINE2:n_child#00ff00:milter",
                    "GPRINT:total_child:MAX:total\\: %8.0lf sessions",
                    "GPRINT:child:AVERAGE:avg\\: %6.2lf sessions/min",
                    "GPRINT:max_child:MAX:max\\: %4.0lf sessions/min\\l",
                    "COMMENT:[2009-05-21T01\\:31\\:00+09\\:00]\\r")

    mock(RRD).last(mail_rrd) {Time.at(1242837060)}
    mock(RRD).graph(mail_year_png,
                    "--title", "Processed mails",
                    "--vertical-label", "mails/min",
                    "--start", "-36288000",
                    "--end", year_end_time.to_s,
                    "--width", "600",
                    "--height", "200",
                    "--alt-y-grid",
                    "--units-exponent", "0",
                    "DEF:pass=#{mail_rrd}:pass:AVERAGE",
                    "DEF:max_pass=#{mail_rrd}:pass:MAX",
                    "CDEF:n_pass=pass",
                    "CDEF:real_pass=pass",
                    "CDEF:real_max_pass=max_pass",
                    "CDEF:real_n_pass=n_pass,3360,*",
                    "CDEF:total_pass=PREV,UN,real_n_pass,PREV,IF,real_n_pass,+",
                    "DEF:accept=#{mail_rrd}:accept:AVERAGE",
                    "DEF:max_accept=#{mail_rrd}:accept:MAX",
                    "CDEF:n_accept=accept",
                    "CDEF:real_accept=accept",
                    "CDEF:real_max_accept=max_accept",
                    "CDEF:real_n_accept=n_accept,3360,*",
                    "CDEF:total_accept=PREV,UN,real_n_accept,PREV,IF,real_n_accept,+",
                    "DEF:reject=#{mail_rrd}:reject:AVERAGE",
                    "DEF:max_reject=#{mail_rrd}:reject:MAX",
                    "CDEF:n_reject=reject",
                    "CDEF:real_reject=reject",
                    "CDEF:real_max_reject=max_reject",
                    "CDEF:real_n_reject=n_reject,3360,*",
                    "CDEF:total_reject=PREV,UN,real_n_reject,PREV,IF,real_n_reject,+",
                    "DEF:discard=#{mail_rrd}:discard:AVERAGE",
                    "DEF:max_discard=#{mail_rrd}:discard:MAX",
                    "CDEF:n_discard=discard",
                    "CDEF:real_discard=discard",
                    "CDEF:real_max_discard=max_discard",
                    "CDEF:real_n_discard=n_discard,3360,*",
                    "CDEF:total_discard=PREV,UN,real_n_discard,PREV,IF,real_n_discard,+",
                    "DEF:temporary-failure=#{mail_rrd}:temporary-failure:AVERAGE",
                    "DEF:max_temporary-failure=#{mail_rrd}:temporary-failure:MAX",
                    "CDEF:n_temporary-failure=temporary-failure",
                    "CDEF:real_temporary-failure=temporary-failure",
                    "CDEF:real_max_temporary-failure=max_temporary-failure",
                    "CDEF:real_n_temporary-failure=n_temporary-failure,3360,*",
                    "CDEF:total_temporary-failure=PREV,UN,real_n_temporary-failure,PREV,IF,real_n_temporary-failure,+",
                    "DEF:quarantine=#{mail_rrd}:quarantine:AVERAGE",
                    "DEF:max_quarantine=#{mail_rrd}:quarantine:MAX",
                    "CDEF:n_quarantine=quarantine",
                    "CDEF:real_quarantine=quarantine",
                    "CDEF:real_max_quarantine=max_quarantine",
                    "CDEF:real_n_quarantine=n_quarantine,3360,*",
                    "CDEF:total_quarantine=PREV,UN,real_n_quarantine,PREV,IF,real_n_quarantine,+",
                    "DEF:abort=#{mail_rrd}:abort:AVERAGE",
                    "DEF:max_abort=#{mail_rrd}:abort:MAX",
                    "CDEF:n_abort=abort",
                    "CDEF:real_abort=abort",
                    "CDEF:real_max_abort=max_abort",
                    "CDEF:real_n_abort=n_abort,3360,*",
                    "CDEF:total_abort=PREV,UN,real_n_abort,PREV,IF,real_n_abort,+",
                    "AREA:pass#0000ff:Pass      ",
                    "GPRINT:total_pass:MAX:total\\: %11.0lf mails",
                    "GPRINT:pass:AVERAGE:avg\\: %9.2lf mails/min",
                    "GPRINT:max_pass:MAX:max\\: %7.0lf mails/min\\l",
                    "STACK:accept#00ff00:Accept    ",
                    "GPRINT:total_accept:MAX:total\\: %11.0lf mails",
                    "GPRINT:accept:AVERAGE:avg\\: %9.2lf mails/min",
                    "GPRINT:max_accept:MAX:max\\: %7.0lf mails/min\\l",
                    "STACK:reject#ff0000:Reject    ",
                    "GPRINT:total_reject:MAX:total\\: %11.0lf mails",
                    "GPRINT:reject:AVERAGE:avg\\: %9.2lf mails/min",
                    "GPRINT:max_reject:MAX:max\\: %7.0lf mails/min\\l",
                    "STACK:discard#ffd400:Discard   ",
                    "GPRINT:total_discard:MAX:total\\: %11.0lf mails",
                    "GPRINT:discard:AVERAGE:avg\\: %9.2lf mails/min",
                    "GPRINT:max_discard:MAX:max\\: %7.0lf mails/min\\l",
                    "STACK:temporary-failure#888888:Temp-Fail ",
                    "GPRINT:total_temporary-failure:MAX:total\\: %11.0lf mails",
                    "GPRINT:temporary-failure:AVERAGE:avg\\: %9.2lf mails/min",
                    "GPRINT:max_temporary-failure:MAX:max\\: %7.0lf mails/min\\l",
                    "STACK:quarantine#a52a2a:Quarantine",
                    "GPRINT:total_quarantine:MAX:total\\: %11.0lf mails",
                    "GPRINT:quarantine:AVERAGE:avg\\: %9.2lf mails/min",
                    "GPRINT:max_quarantine:MAX:max\\: %7.0lf mails/min\\l",
                    "STACK:abort#ff9999:Abort     ",
                    "GPRINT:total_abort:MAX:total\\: %11.0lf mails",
                    "GPRINT:abort:AVERAGE:avg\\: %9.2lf mails/min",
                    "GPRINT:max_abort:MAX:max\\: %7.0lf mails/min\\l",
                    "DEF:smtp=#{session_rrd}:smtp:AVERAGE",
                    "DEF:max_smtp=#{session_rrd}:smtp:MAX",
                    "CDEF:n_smtp=smtp",
                    "CDEF:real_smtp=smtp",
                    "CDEF:real_max_smtp=max_smtp",
                    "CDEF:real_n_smtp=n_smtp,3360,*",
                    "CDEF:total_smtp=PREV,UN,real_n_smtp,PREV,IF,real_n_smtp,+",
                    "LINE2:n_smtp#000000:SMTP      ",
                    "GPRINT:total_smtp:MAX:total\\: %8.0lf sessions",
                    "GPRINT:smtp:AVERAGE:avg\\: %6.2lf sessions/min",
                    "GPRINT:max_smtp:MAX:max\\: %4.0lf sessions/min\\l",
                    "COMMENT:[2009-05-21T01\\:31\\:00+09\\:00]\\r")

    mock(RRD).last(stop_rrd) {Time.at(1242837060)}
    mock(RRD).graph(stop_year_png,
                    "--title", "Stopped milters",
                    "--vertical-label", "milters/min",
                    "--start", "-36288000",
                    "--end", year_end_time.to_s,
                    "--width", "600",
                    "--height", "200",
                    "--alt-y-grid",
                    "--units-exponent", "0",
                    "DEF:connect=#{stop_rrd}:connect:AVERAGE",
                    "DEF:max_connect=#{stop_rrd}:connect:MAX",
                    "CDEF:n_connect=connect",
                    "CDEF:real_connect=connect",
                    "CDEF:real_max_connect=max_connect",
                    "CDEF:real_n_connect=n_connect,3360,*",
                    "CDEF:total_connect=PREV,UN,real_n_connect,PREV,IF,real_n_connect,+",
                    "DEF:helo=#{stop_rrd}:helo:AVERAGE",
                    "DEF:max_helo=#{stop_rrd}:helo:MAX",
                    "CDEF:n_helo=helo",
                    "CDEF:real_helo=helo",
                    "CDEF:real_max_helo=max_helo",
                    "CDEF:real_n_helo=n_helo,3360,*",
                    "CDEF:total_helo=PREV,UN,real_n_helo,PREV,IF,real_n_helo,+",
                    "DEF:envelope-from=#{stop_rrd}:envelope-from:AVERAGE",
                    "DEF:max_envelope-from=#{stop_rrd}:envelope-from:MAX",
                    "CDEF:n_envelope-from=envelope-from",
                    "CDEF:real_envelope-from=envelope-from",
                    "CDEF:real_max_envelope-from=max_envelope-from",
                    "CDEF:real_n_envelope-from=n_envelope-from,3360,*",
                    "CDEF:total_envelope-from=PREV,UN,real_n_envelope-from,PREV,IF,real_n_envelope-from,+",
                    "DEF:envelope-recipient=#{stop_rrd}:envelope-recipient:AVERAGE",
                    "DEF:max_envelope-recipient=#{stop_rrd}:envelope-recipient:MAX",
                    "CDEF:n_envelope-recipient=envelope-recipient",
                    "CDEF:real_envelope-recipient=envelope-recipient",
                    "CDEF:real_max_envelope-recipient=max_envelope-recipient",
                    "CDEF:real_n_envelope-recipient=n_envelope-recipient,3360,*",
                    "CDEF:total_envelope-recipient=PREV,UN,real_n_envelope-recipient,PREV,IF,real_n_envelope-recipient,+",
                    "DEF:header=#{stop_rrd}:header:AVERAGE",
                    "DEF:max_header=#{stop_rrd}:header:MAX",
                    "CDEF:n_header=header",
                    "CDEF:real_header=header",
                    "CDEF:real_max_header=max_header",
                    "CDEF:real_n_header=n_header,3360,*",
                    "CDEF:total_header=PREV,UN,real_n_header,PREV,IF,real_n_header,+",
                    "DEF:body=#{stop_rrd}:body:AVERAGE",
                    "DEF:max_body=#{stop_rrd}:body:MAX",
                    "CDEF:n_body=body",
                    "CDEF:real_body=body",
                    "CDEF:real_max_body=max_body",
                    "CDEF:real_n_body=n_body,3360,*",
                    "CDEF:total_body=PREV,UN,real_n_body,PREV,IF,real_n_body,+",
                    "DEF:end-of-message=#{stop_rrd}:end-of-message:AVERAGE",
                    "DEF:max_end-of-message=#{stop_rrd}:end-of-message:MAX",
                    "CDEF:n_end-of-message=end-of-message",
                    "CDEF:real_end-of-message=end-of-message",
                    "CDEF:real_max_end-of-message=max_end-of-message",
                    "CDEF:real_n_end-of-message=n_end-of-message,3360,*",
                    "CDEF:total_end-of-message=PREV,UN,real_n_end-of-message,PREV,IF,real_n_end-of-message,+",
                    "AREA:connect#0000ff:connect           ",
                    "GPRINT:total_connect:MAX:total\\: %8.0lf milters",
                    "GPRINT:connect:AVERAGE:avg\\: %6.2lf milters/min",
                    "GPRINT:max_connect:MAX:max\\: %4.0lf milters/min\\l",
                    "STACK:helo#ff00ff:helo              ",
                    "GPRINT:total_helo:MAX:total\\: %8.0lf milters",
                    "GPRINT:helo:AVERAGE:avg\\: %6.2lf milters/min",
                    "GPRINT:max_helo:MAX:max\\: %4.0lf milters/min\\l",
                    "STACK:envelope-from#00ffff:envelope-from     ",
                    "GPRINT:total_envelope-from:MAX:total\\: %8.0lf milters",
                    "GPRINT:envelope-from:AVERAGE:avg\\: %6.2lf milters/min",
                    "GPRINT:max_envelope-from:MAX:max\\: %4.0lf milters/min\\l",
                    "STACK:envelope-recipient#ffff00:envelope-recipient",
                    "GPRINT:total_envelope-recipient:MAX:total\\: %8.0lf milters",
                    "GPRINT:envelope-recipient:AVERAGE:avg\\: %6.2lf milters/min",
                    "GPRINT:max_envelope-recipient:MAX:max\\: %4.0lf milters/min\\l",
                    "STACK:header#a52a2a:header            ",
                    "GPRINT:total_header:MAX:total\\: %8.0lf milters",
                    "GPRINT:header:AVERAGE:avg\\: %6.2lf milters/min",
                    "GPRINT:max_header:MAX:max\\: %4.0lf milters/min\\l",
                    "STACK:body#ff0000:body              ",
                    "GPRINT:total_body:MAX:total\\: %8.0lf milters",
                    "GPRINT:body:AVERAGE:avg\\: %6.2lf milters/min",
                    "GPRINT:max_body:MAX:max\\: %4.0lf milters/min\\l",
                    "STACK:end-of-message#00ff00:end-of-message    ",
                    "GPRINT:total_end-of-message:MAX:total\\: %8.0lf milters",
                    "GPRINT:end-of-message:AVERAGE:avg\\: %6.2lf milters/min",
                    "GPRINT:max_end-of-message:MAX:max\\: %4.0lf milters/min\\l",
                    "DEF:milter=#{session_rrd}:child:AVERAGE",
                    "DEF:max_milter=#{session_rrd}:child:MAX",
                    "CDEF:n_milter=milter",
                    "CDEF:real_milter=milter",
                    "CDEF:real_max_milter=max_milter",
                    "CDEF:real_n_milter=n_milter,3360,*",
                    "CDEF:total_milter=PREV,UN,real_n_milter,PREV,IF,real_n_milter,+",
                    "LINE2:n_milter#000000:total             ",
                    "GPRINT:total_milter:MAX:total\\: %8.0lf milters",
                    "GPRINT:milter:AVERAGE:avg\\: %6.2lf milters/min",
                    "GPRINT:max_milter:MAX:max\\: %4.0lf milters/min\\l",
                    "COMMENT:[2009-05-21T01\\:31\\:00+09\\:00]\\r")

    @analyzer.output_all_graph
  end
end
