# -*- coding: utf-8 -*-
#
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

class TestRedHatUpstartDetector < Test::Unit::TestCase
  include MilterDetectorTestUtils

  def setup
    @configuration = Milter::Manager::Configuration.new
    @loader = Milter::Manager::ConfigurationLoader.new(@configuration)
    @tmp_dir = Pathname(File.dirname(__FILE__)) + ".." + "tmp"
    @event_d = @tmp_dir + "event.d"
    @event_d.mkpath
  end

  def teardown
    FileUtils.rm_rf(@tmp_dir.to_s)
  end

  def test_apply_clamav_milter_style
    clamav_milter_conf = @tmp_dir + "clamav-milter.conf"
    (@event_d + "clamav-milter").open("w") do |file|
      file << clamav_milter_upstart(clamav_milter_conf)
    end

    clamav_milter_conf.open("w") do |file|
      file << <<-'EOC'
# Define the interface through which we communicate with sendmail
# This option is mandatory! Possible formats are:
# [[unix|local]:]/path/to/file - to specify a unix domain socket
# inet:port@[hostname|ip-address] - to specify an ipv4 socket
# inet6:port@[hostname|ip-address] - to specify an ipv6 socket
#
# Default: no default
MilterSocket unix:/var/clamav/clmilter.socket
#MilterSocket inet:7357
EOC
    end

    detector = redhat_upstart_detector("clamav-milter")
    detector.detect
    detector.apply(@loader)
    assert_eggs([["clamav-milter",
                  nil,
                  true,
                  "/sbin/start",
                  "clamav-milter",
                  "unix:/var/clamav/clmilter.socket"]])
  end

  private
  def redhat_upstart_detector(name)
    detector = Milter::Manager::RedHatUpstartDetector.new(@configuration, name)

    _event_d = @event_d
    singleton_object = class << detector; self; end
    singleton_object.send(:define_method, :event_d) do
      _event_d.to_s
    end

    detector
  end

  def clamav_milter_upstart(config)
    <<-EOS
### Uncomment these lines when you want clamav-milter to be a milter
### for a locally running MTA
#start on starting\ sendmail
#start on starting sendmail
start on starting\ postfix
start on starting postfix

### Uncomment these lines when you want clamav-milter to be a milter
### for a remotely running MTA
#start on starting\ local
#start on starting local

stop  on runlevel 0
stop  on runlevel 1
stop  on runlevel 6

respawn
exec /usr/sbin/clamav-milter -c #{config} --nofork=yes
EOS
  end
end
