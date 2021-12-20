# Copyright (C) 2009-2021  Sutou Kouhei <kou@clear-code.com>
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
    @tmp_dir = Pathname(Dir.mktmpdir)
    @etc_dir = @tmp_dir + "etc"
    @event_d = @etc_dir + "event.d"
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

  def test_apply_clamav_milter_style_example
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

Example
EOC
    end

    detector = redhat_upstart_detector("clamav-milter")
    detector.detect
    detector.apply(@loader)
    assert_eggs([["clamav-milter",
                  nil,
                  false,
                  "/sbin/start",
                  "clamav-milter",
                  "unix:/var/clamav/clmilter.socket"]])
  end

  def test_apply_milter_greylist_style
    (@event_d + "milter-greylist").open("w") do |file|
      file << milter_greylist_upstart
    end

    etc_mail_dir = @etc_dir + "mail"
    etc_mail_dir.mkpath
    greylist_conf = etc_mail_dir + "greylist.conf"
    greylist_conf.open("w") do |file|
      file << <<-'EOC'
#
# Simple greylisting config file using the new features
# See greylist2.conf for a more detailed list of available options
#
# $Id: greylist.conf,v 1.45.2.1 2009/02/12 22:39:01 manu Exp $
#

#pidfile "/var/run/milter-greylist.pid"
socket "/var/run/milter-greylist/milter-greylist.sock"
dumpfile "/var/lib/milter-greylist/db/greylist.db" 600
geoipdb "/usr/share/GeoIP/GeoIP.dat"
dumpfreq 1
user "grmilter"
EOC
    end

    detector = redhat_upstart_detector("milter-greylist")
    detector.detect
    detector.apply(@loader)
    assert_eggs([["milter-greylist",
                  nil,
                  true,
                  "/sbin/start",
                  "milter-greylist",
                  "unix:/var/run/milter-greylist/milter-greylist.sock"]])
  end

  private
  def redhat_upstart_detector(name, options={})
    detector = Milter::Manager::RedHatUpstartDetector.new(@configuration, name)

    _etc_dir = @etc_dir
    _candidate_service_commands = options[:candidate_service_commands] || []
    singleton_object = class << detector; self; end
    singleton_object.send(:define_method, :etc_dir) do
      _etc_dir.to_s
    end

    singleton_object.send(:define_method, :candidate_service_commands) do
      _candidate_service_commands
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

  def milter_greylist_upstart
    <<-EOS
### Uncomment this line when you run want milter-greylist to be a
### milter for postfix on the current machine

start on starting postfix


stop  on runlevel 0
stop  on runlevel 1
stop  on runlevel 6

respawn
exec /usr/sbin/milter-greylist -D
EOS
  end
end
