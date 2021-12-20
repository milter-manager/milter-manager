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

class TestRedHatDetector < Test::Unit::TestCase
  include MilterDetectorTestUtils

  def setup
    @configuration = Milter::Manager::Configuration.new
    @loader = Milter::Manager::ConfigurationLoader.new(@configuration)
    @tmp_dir = Pathname(Dir.mktmpdir)
    @init_base_dir = @tmp_dir
    @init_d = @init_base_dir + "init.d"
    @sysconfig_dir = @tmp_dir + "sysconfig"
    @event_d = @tmp_dir + "event.d"
    @init_d.mkpath
    @sysconfig_dir.mkpath
    @event_d.mkpath
  end

  def teardown
    FileUtils.rm_rf(@tmp_dir.to_s)
  end

  def test_apply_clamav_milter_style
    (@init_d + "clamav-milter").open("w") do |file|
      file << clamav_milter_init_header
    end
    create_rc_files("clamav-milter")

    clamav_milter_conf = @tmp_dir + "clamav-milter.conf"
    (@sysconfig_dir + "clamav-milter").open("w") do |file|
      file << clamav_milter_sysconfig(clamav_milter_conf)
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

    detector = redhat_detector("clamav-milter")
    detector.detect
    detector.apply(@loader)
    assert_eggs([["clamav-milter",
                  "clamav-milter is a daemon which hooks into sendmail " +
                  "and routes email messages to clamav.",
                  true,
                  (@init_d + "clamav-milter").to_s,
                  "start",
                  "unix:/var/clamav/clmilter.socket"]])
  end

  def test_apply_clamav_milter_enabled_upstart_style
    (@init_d + "clamav-milter").open("w") do |file|
      file << clamav_milter_init_header
    end
    create_rc_files("clamav-milter")

    clamav_milter_conf = @tmp_dir + "clamav-milter.conf"
    (@event_d + "clamav-milter").open("w") do |file|
      file << clamav_milter_upstart(clamav_milter_conf, true)
    end

    (@sysconfig_dir + "clamav-milter").open("w") do |file|
      file << clamav_milter_sysconfig(clamav_milter_conf)
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

    detector = redhat_detector("clamav-milter")
    detector.detect
    detector.apply(@loader)
    assert_eggs([["clamav-milter",
                  nil,
                  true,
                  "/sbin/start",
                  "clamav-milter",
                  "unix:/var/clamav/clmilter.socket"]])
  end

  def test_apply_clamav_milter_disabled_upstart_style
    (@init_d + "clamav-milter").open("w") do |file|
      file << clamav_milter_init_header
    end
    create_rc_files("clamav-milter")

    clamav_milter_conf = @tmp_dir + "clamav-milter.conf"
    (@event_d + "clamav-milter").open("w") do |file|
      file << clamav_milter_upstart(clamav_milter_conf, false)
    end

    (@sysconfig_dir + "clamav-milter").open("w") do |file|
      file << <<-EOC
### Simple config file for clamav-milter, you should
### read the documentation and tweak it as you wish.

CLAMAV_FLAGS="--config-file=#{clamav_milter_conf}"
EOC
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

    detector = redhat_detector("clamav-milter")
    detector.detect
    detector.apply(@loader)
    assert_eggs([["clamav-milter",
                  "clamav-milter is a daemon which hooks into sendmail " +
                  "and routes email messages to clamav.",
                  true,
                  (@init_d + "clamav-milter").to_s,
                  "start",
                  "unix:/var/clamav/clmilter.socket"]])
  end

  private
  def redhat_detector(name, options={})
    detector = Milter::Manager::RedHatDetector.new(@configuration, name)

    _init_base_dir = @init_base_dir
    _sysconfig_dir = @sysconfig_dir
    _event_d = @event_d
    _candidate_service_commands = options[:candidate_service_commands] || []

    init_detector = detector.instance_variable_get("@init_detector")
    singleton_init_detector = class << init_detector; self; end
    singleton_init_detector.send(:define_method, :init_base_dir) do
      _init_base_dir.to_s
    end

    singleton_init_detector.send(:define_method, :sysconfig_dir) do
      _sysconfig_dir.to_s
    end

    singleton_init_detector.send(:define_method,
                                 :candidate_service_commands) do
      _candidate_service_commands
    end

    upstart_detector = detector.instance_variable_get("@upstart_detector")
    singleton_upstart_detector = class << upstart_detector; self; end
    singleton_upstart_detector.send(:define_method, :event_d) do
      _event_d.to_s
    end

    singleton_upstart_detector.send(:define_method,
                                    :candidate_service_commands) do
      _candidate_service_commands
    end

    detector
  end

  def create_rc_files(name)
    start_priority = 80
    end_priority = 20
    0.upto(6) do |i|
      case i
      when 2, 3, 4, 5
        mark = "S"
        priority = start_priority
      when 0, 1, 6
        mark = "K"
        priority = end_priority
      end
      rc_d = @init_base_dir + "rc#{i}.d"
      rc_d.mkpath
      base_name = "%s%02d%s" % [mark, priority, name]
      rc_file = rc_d + base_name
      rc_file.open("w") {}
    end
  end

  def clamav_milter_init_header
    <<-'EOS'
#!/bin/sh
#
# Startup script for the Clamav Milter Daemon
#
# chkconfig: 2345 77 23
# description: clamav-milter is a daemon which hooks into sendmail \
#              and routes email messages to clamav.
# processname: clamav-milter
# pidfile: /var/run/clamav/clamav-milter.pid
# config: /etc/sysconfig/clamav-milter

# Source function library.
. /etc/rc.d/init.d/functions

# Source networking configuration.
. /etc/sysconfig/network

[ -x /usr/sbin/clamav-milter ] || exit 0

# Local clamav-milter config
CLAMAV_FLAGS=
test -f /etc/sysconfig/clamav-milter && . /etc/sysconfig/clamav-milter
EOS
  end

  def clamav_milter_upstart(config, enabled)
    <<-EOS
### Uncomment these lines when you want clamav-milter to be a milter
### for a locally running MTA
#start on starting\ sendmail
#start on starting sendmail
#{enabled ? '' : '#'}start on starting\ postfix
#{enabled ? '' : '#'}start on starting postfix

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

  def clamav_milter_sysconfig(config)
    <<-EOC
### Simple config file for clamav-milter, you should
### read the documentation and tweak it as you wish.

CLAMAV_FLAGS="--config-file=#{config}"
EOC
  end
end
