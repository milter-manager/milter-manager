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

class TestRedHatInitDetector < Test::Unit::TestCase
  include MilterDetectorTestUtils

  def setup
    @configuration = Milter::Manager::Configuration.new
    @loader = Milter::Manager::ConfigurationLoader.new(@configuration)
    @tmp_dir = Pathname(Dir.mktmpdir)
    @init_base_dir = @tmp_dir
    @init_d = @init_base_dir + "init.d"
    @etc_dir = @tmp_dir + "etc"
    @sysconfig_dir = @etc_dir + "sysconfig"
    @init_d.mkpath
    @sysconfig_dir.mkpath
  end

  def teardown
    FileUtils.rm_rf(@tmp_dir.to_s)
  end

  def test_init_script_exist?
    detector = redhat_init_detector("milter-manager")
    assert_false(detector.init_script_exist?)
    (@init_d + "milter-manager").open("w") {}
    assert_true(detector.init_script_exist?)
  end

  def test_name
    (@init_d + "milter-manager").open("w") do |file|
      file << milter_manager_init_script
    end
    detector = redhat_init_detector("milter-manager")
    detector.detect
    assert_equal("milter-manager", detector.name)
  end

  def test_variables
    (@init_d + "milter-manager").open("w") do |file|
      file << milter_manager_init_script
    end
    detector = redhat_init_detector("milter-manager")
    detector.detect
    default_pidfile = "/var/run/milter-manager/milter-manager.pid"
    assert_equal({
                   "name" => "milter manager",
                   "prog" => "milter-manager",
                   "milter_manager" => "/usr/sbin/milter-manager",
                   "default_pidfile" => default_pidfile,
                   "killproc_options" => "-p #{default_pidfile}",
                   "USER" => "",
                   "GROUP" => "",
                   "SOCKET_GROUP" => "",
                   "PIDFILE" => "",
                   "CONNECTION_SPEC" => "",
                   "DAEMON_ARGS" => ["--daemon",
                                     "--pid-file", "",
                                     "--connection-spec", "",
                                     "--user-name", "",
                                     "--group-name", "",
                                     "--unix-socket-group", "",
                                    ].join(" ") + " ",
                   "OPTION_ARGS" => "",
                   "RETVAL" => "$?",
                 },
                 detector.variables)
  end

  def test_info
    (@init_d + "milter-manager").open("w") do |file|
      file << milter_manager_init_script
    end
    detector = redhat_init_detector("milter-manager")
    detector.detect
    assert_equal({
                   "chkconfig" => "2345 80 20",
                   "description" =>
                     "The milter that manages milters to combine them flexibly.",
                   "processname" => "milter-manager",
                   "config" => "/etc/sysconfig/milter-manager",
                   "pidfile" => "/var/run/milter-manager/milter-manager.pid",

                   "Provides" => "milter-manager",
                   "Default-Start" => "2 3 4 5",
                   "Default-Stop" => "0 1 6",
                   "Short-Description" => "milter manager's init script",
                   "Description" =>
                     "The milter that manages milters to combine them flexibly.",
                 },
                 detector.info)
  end

  def test_parse_init_conf
    (@init_d + "milter-manager").open("w") do |file|
      file << milter_manager_init_script
    end
    create_rc_files("milter-manager")
    (@sysconfig_dir + "milter-manager").open("w") do |file|
      file << <<-EOC
CONNECTION_SPEC=inet:10025@localhost
# CONNECTION_SPEC=inet6:2929
EOC
    end

    detector = redhat_init_detector("milter-manager")
    detector.detect
    default_pidfile = "/var/run/milter-manager/milter-manager.pid"
    assert_equal({
                   "name" => "milter manager",
                   "prog" => "milter-manager",
                   "milter_manager" => "/usr/sbin/milter-manager",
                   "default_pidfile" => default_pidfile,
                   "killproc_options" => "-p #{default_pidfile}",
                   "USER" => "",
                   "GROUP" => "",
                   "SOCKET_GROUP" => "",
                   "PIDFILE" => "",
                   "CONNECTION_SPEC" => "inet:10025@localhost",
                   "DAEMON_ARGS" => ["--daemon",
                                     "--pid-file", "",
                                     "--connection-spec", "",
                                     "--user-name", "",
                                     "--group-name", "",
                                     "--unix-socket-group", "",
                                    ].join(" ") + " ",
                   "OPTION_ARGS" => "",
                   "RETVAL" => "$?",
                 },
                 detector.variables)
    assert_true(detector.enabled?)
  end

  def test_enabled
    (@init_d + "milter-manager").open("w") do |file|
      file << milter_manager_init_script
    end

    detector = redhat_init_detector("milter-manager")
    detector.detect
    assert_false(detector.enabled?)

    create_rc_files("milter-manager")
    detector = redhat_init_detector("milter-manager")
    detector.detect
    assert_false(detector.enabled?)

    (@sysconfig_dir + "milter-manager").open("w") do |file|
      file << <<-EOC
CONNECTION_SPEC=inet:10025@localhost
EOC
    end
    detector = redhat_init_detector("milter-manager")
    detector.detect
    assert_true(detector.enabled?)
  end

  def test_apply
    (@init_d + "milter-manager").open("w") do |file|
      file << milter_manager_init_script
    end
    create_rc_files("milter-manager")
    (@sysconfig_dir + "milter-manager").open("w") do |file|
      file << <<-EOC
CONNECTION_SPEC=inet:10025@localhost
EOC
    end

    detector = redhat_init_detector("milter-manager")
    detector.detect
    detector.apply(@loader)
    assert_eggs([["milter-manager",
                  "The milter that manages milters to combine them flexibly.",
                  true,
                  (@init_d + "milter-manager").to_s,
                  "start",
                  "inet:10025@localhost"]])
  end

  def test_apply_amavisd_style
    (@init_d + "amavisd").open("w") do |file|
      file << amavisd_init_header
    end

    detector = redhat_init_detector("amavisd")
    detector.detect
    detector.apply(@loader)
    assert_equal("amavisd", detector.name)
    assert_eggs([["amavisd",
                  "AMaViS virus scanner.",
                  false,
                  (@init_d + "amavisd").to_s,
                  "start",
                  nil]])
  end

  def test_apply_amavisd_style_with_config
    (@init_d + "amavisd").open("w") do |file|
      file << amavisd_init_header
    end
    create_rc_files("amavisd")
    (@sysconfig_dir + "amavisd").open("w") do |file|
      file << <<-EOC
MILTER_SOCKET=inet:10025@localhost
EOC
    end

    detector = redhat_init_detector("amavisd")
    detector.detect
    detector.apply(@loader)
    assert_equal("amavisd", detector.name)
    assert_eggs([["amavisd",
                  "AMaViS virus scanner.",
                  true,
                  (@init_d + "amavisd").to_s,
                  "start",
                  "inet:10025@localhost"]])
  end

  def test_apply_clamav_milter_before_0_95_style
    (@init_d + "clamav-milter").open("w") do |file|
      file << clamav_milter_before_0_95_init_header
    end

    detector = redhat_init_detector("clamav-milter")
    detector.detect
    detector.apply(@loader)
    assert_eggs([["clamav-milter",
                  "clamav-milter is a daemon which hooks into sendmail " +
                  "and routes email messages to clamav.",
                  false,
                  (@init_d + "clamav-milter").to_s,
                  "start",
                  nil]])
  end

  def test_apply_clamav_milter_before_0_95_style_with_config
    (@init_d + "clamav-milter").open("w") do |file|
      file << clamav_milter_before_0_95_init_header
    end
    create_rc_files("clamav-milter")
    (@sysconfig_dir + "clamav-milter").open("w") do |file|
      file << <<-'EOC'
#SOCKET_ADDRESS="local:/var/clamav/clmilter.socket"
SOCKET_ADDRESS="inet:11121@[127.0.0.1]"
EOC
    end

    detector = redhat_init_detector("clamav-milter")
    detector.detect
    detector.apply(@loader)
    assert_eggs([["clamav-milter",
                  "clamav-milter is a daemon which hooks into sendmail " +
                  "and routes email messages to clamav.",
                  true,
                  (@init_d + "clamav-milter").to_s,
                  "start",
                  "inet:11121@[127.0.0.1]"]])
  end

  def test_apply_clamav_milter_style
    (@init_d + "clamav-milter").open("w") do |file|
      file << clamav_milter_init_header
    end
    create_rc_files("clamav-milter")

    clamav_milter_conf = @tmp_dir + "clamav-milter.conf"
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

    detector = redhat_init_detector("clamav-milter")
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

  def test_apply_clamav_milter_epel_style
    (@init_d + "clamav-milter").open("w") do |file|
      file << clamav_milter_epel_init_header
    end
    create_rc_files("clamav-milter")

    clamav_milter_conf = @tmp_dir + "clamav-milter.conf"
    (@sysconfig_dir + "clamav-milter").open("w") do |file|
      file << <<-EOC
CLAMAV_FLAGS='-lo -c #{clamav_milter_conf} local:/var/run/clamav-milter/clamav.sock'
EOC
    end

    clamav_milter_conf.open("w") do |file|
      file << <<-'EOC'
# Comment or remove the line below.
# Example

MilterSocket inet:7357
EOC
    end

    detector = redhat_init_detector("clamav-milter")
    detector.detect
    detector.apply(@loader)
    assert_eggs([["clamav-milter",
                  "clamav-milter is a daemon which hooks into sendmail " +
                  "and routes email messages for virus scanning with ClamAV",
                  true,
                  (@init_d + "clamav-milter").to_s,
                  "start",
                  "inet:7357"]])
  end

  def test_apply_clamav_milter_epel_style_example
    (@init_d + "clamav-milter").open("w") do |file|
      file << clamav_milter_epel_init_header
    end
    create_rc_files("clamav-milter")

    clamav_milter_conf = @tmp_dir + "clamav-milter.conf"
    (@sysconfig_dir + "clamav-milter").open("w") do |file|
      file << <<-EOC
CLAMAV_FLAGS='-lo -c #{clamav_milter_conf} local:/var/run/clamav-milter/clamav.sock'
EOC
    end

    clamav_milter_conf.open("w") do |file|
      file << <<-'EOC'
# Comment or remove the line below.
Example

MilterSocket inet:7357@[127.0.0.1]
EOC
    end

    detector = redhat_init_detector("clamav-milter")
    detector.detect
    detector.apply(@loader)
    assert_eggs([["clamav-milter",
                  "clamav-milter is a daemon which hooks into sendmail " +
                  "and routes email messages for virus scanning with ClamAV",
                  false,
                  (@init_d + "clamav-milter").to_s,
                  "start",
                  "inet:7357@[127.0.0.1]"]])
  end

  def test_apply_milter_greylist_style
    (@init_d + "milter-greylist").open("w") do |file|
      file << milter_greylist_init_header
    end
    create_rc_files("milter-greylist")

    detector = redhat_init_detector("milter-greylist")
    detector.detect
    detector.apply(@loader)
    assert_equal("milter-greylist", detector.name)
    assert_eggs([["milter-greylist",
                  "Milter Greylist Daemon",
                  true,
                  (@init_d + "milter-greylist").to_s,
                  "start",
                  "unix:/var/milter-greylist/milter-greylist.sock"]])
  end

  def test_apply_milter_greylist_style_with_sysconfig
    (@init_d + "milter-greylist").open("w") do |file|
      file << milter_greylist_init_header
    end
    create_rc_files("milter-greylist")
    (@sysconfig_dir + "milter-greylist").open("w") do |file|
      file << <<-'EOC'
OPTIONS="$OPTIONS -p inet:10025@localhost"
EOC
    end

    detector = redhat_init_detector("milter-greylist")
    detector.detect
    detector.apply(@loader)
    assert_equal("milter-greylist", detector.name)
    assert_eggs([["milter-greylist",
                  "Milter Greylist Daemon",
                  true,
                  (@init_d + "milter-greylist").to_s,
                  "start",
                  "inet:10025@localhost"]])
  end

  def test_apply_milter_greylist_style_with_config
    etc_mail_dir = @etc_dir + "mail"
    etc_mail_dir.mkpath
    greylist_conf = etc_mail_dir + "greylist.conf"
    (@init_d + "milter-greylist").open("w") do |file|
      file << milter_greylist_init_header(greylist_conf)
    end
    create_rc_files("milter-greylist")

    greylist_conf.open("w") do |file|
      file << <<-'EOC'
#
# Simple greylisting config file using the new features
# See greylist2.conf for a more detailed list of available options
#
# $Id: greylist.conf,v 1.45.2.1 2009/02/12 22:39:01 manu Exp $
#

#pidfile "/var/run/milter-greylist.pid"
socket "inet:10025@[127.0.0.1]"
dumpfile "/var/lib/milter-greylist/db/greylist.db" 600
geoipdb "/usr/share/GeoIP/GeoIP.dat"
dumpfreq 1
user "grmilter"
EOC
    end

    detector = redhat_init_detector("milter-greylist")
    detector.detect
    detector.apply(@loader)
    assert_equal("milter-greylist", detector.name)
    assert_eggs([["milter-greylist",
                  "Milter Greylist Daemon",
                  true,
                  (@init_d + "milter-greylist").to_s,
                  "start",
                  "unix:/var/milter-greylist/milter-greylist.sock"]])
  end

  def test_apply_milter_greylist_fedora_style
    etc_mail_dir = @etc_dir + "mail"
    etc_mail_dir.mkpath
    greylist_conf = etc_mail_dir + "greylist.conf"
    (@init_d + "milter-greylist").open("w") do |file|
      file << milter_greylist_init_header_fedora(greylist_conf)
    end
    create_rc_files("milter-greylist")

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

    detector = redhat_init_detector("milter-greylist")
    detector.detect
    detector.apply(@loader)
    assert_equal("milter-greylist", detector.name)
    assert_eggs([["milter-greylist",
                  "Milter Greylist Daemon",
                  true,
                  (@init_d + "milter-greylist").to_s,
                  "start",
                  "unix:/var/run/milter-greylist/milter-greylist.sock"]])
  end

  def test_apply_milter_greylist_epel_style
    etc_mail_dir = @etc_dir + "mail"
    etc_mail_dir.mkpath
    greylist_conf = etc_mail_dir + "greylist.conf"
    (@init_d + "milter-greylist").open("w") do |file|
      file << milter_greylist_init_header_fedora(greylist_conf)
    end
    create_rc_files("milter-greylist")

    greylist_conf.open("w") do |file|
      file << <<-'EOC'
#
# Simple greylisting config file using the new features
# See greylist2.conf for a more detailed list of available options
#
# $Id: greylist.conf,v 1.45.2.1 2009/02/12 22:39:01 manu Exp $
#

pidfile "/var/run/milter-greylist.pid"
socket "/var/milter-greylist/milter-greylist.sock" 660
dumpfile "/var/milter-greylist/greylist.db" 600
dumpfreq 1
geoipdb "/usr/share/GeoIP/GeoIP.dat"
user "grmilter"
EOC
    end

    detector = redhat_init_detector("milter-greylist")
    detector.detect
    detector.apply(@loader)
    assert_equal("milter-greylist", detector.name)
    assert_eggs([["milter-greylist",
                  "Milter Greylist Daemon",
                  true,
                  (@init_d + "milter-greylist").to_s,
                  "start",
                  "unix:/var/milter-greylist/milter-greylist.sock"]])
  end

  def test_apply_spamass_milter_style
    (@init_d + "spamass-milter").open("w") do |file|
      file << spamass_milter_init_header
    end
    create_rc_files("spamass-milter")

    detector = redhat_init_detector("spamass-milter")
    detector.detect
    detector.apply(@loader)
    assert_eggs([["spamass-milter",
                  "spamass-milter is a daemon which hooks into sendmail and " +
                  "routes email messages to spamassassin",
                  true,
                  (@init_d + "spamass-milter").to_s,
                  "start",
                  "unix:/var/run/spamass.sock"]])
  end

  def test_apply_enma_style
    enma_conf = @tmp_dir + "enma.conf"
    (@init_d + "enma").open("w") do |file|
      file << enma_init_header(enma_conf.to_s)
    end

    enma_conf.open("w") do |conf|
      conf << <<-EOC
## Milter ##
milter.socket:  inet:10025@127.0.0.1
milter.user:    daemon
milter.pidfile: /var/run/enma/enma.pid
milter.chdir:   /var/tmp
milter.timeout: 7210
milter.loglevel:   0
milter.sendmail813: false
milter.postfix: false
EOC
    end

    detector = redhat_init_detector("enma")
    detector.detect
    detector.apply(@loader)
    assert_equal("enma", detector.name)
    assert_eggs([["enma",
                  "A milter program for domain authentication technologies",
                  false,
                  (@init_d + "enma").to_s,
                  "start",
                  "inet:10025@127.0.0.1"]])
  end

  def test_apply_opendkim_style
    opendkim_conf = @tmp_dir + "opendkim.conf"
    (@init_d + "opendkim").open("w") do |file|
      file << opendkim_init_header(opendkim_conf.to_s)
    end

    opendkim_conf.open("w") do |conf|
      conf << <<-EOC
# Basic OpenDKIM config file
# See opendkim.conf(5) or /usr/share/doc/opendkim/opendkim.conf.sample for more
Mode   v
Syslog  yes
Socket local:/var/run/opendkim/opendkim.socket
EOC
    end

    detector = redhat_init_detector("opendkim")
    detector.detect
    detector.apply(@loader)
    assert_equal("opendkim", detector.name)
    assert_eggs([["opendkim",
                  "OpenDKIM milter for signing and verifying email",
                  false,
                  (@init_d + "opendkim").to_s,
                  "start",
                  "local:/var/run/opendkim/opendkim.socket"]])
  end

  def test_apply_rmilter_style
    rmilter_conf_sysvinit = @tmp_dir + "rmilter.conf.sysvinit"
    rmilter_conf_common = @tmp_dir + "rmilter.conf.common"
    (@init_d + "rmilter").open("w") do |file|
      file << rmilter_init_header(rmilter_conf_sysvinit.to_s)
    end

    rmilter_conf_sysvinit.open("w") do |conf|
      conf << <<-CONF
# sysvinit-specific settings for rmilter

.include #{rmilter_conf_common}

bind_socket = unix:/var/run/rmilter/rmilter.sock;

# pidfile - path to pid file
# Default: pidfile = /var/run/rmilter.pid

pidfile = /var/run/rmilter/rmilter.pid;

# include user's configuration
.try_include #{@tmp_dir}/rmilter.conf.local
.try_include #{@tmp_dir}/rmilter.conf.d/*.conf
.try_include #{@tmp_dir}/rmilter/rmilter.conf.local
.try_include #{@tmp_dir}/rmilter/conf.d/*.conf
      CONF
    end

    rmilter_conf_common.open("w") do |conf|
      conf << <<-CONF
# bind_socket - socket credits for local bind:
# unix:/path/to/file - bind to local socket
# inet:port@host - bind to inet socket
# Default: bind_socket = unix:/var/tmp/rmilter.sock;

bind_socket = unix:/run/rmilter/rmilter.sock;
      CONF
    end

    detector = redhat_init_detector("rmilter")
    detector.detect
    detector.apply(@loader)
    assert_equal("rmilter", detector.name)
    assert_eggs([["rmilter",
                  "rmilter is a spam filtering system",
                  false,
                  (@init_d + "rmilter").to_s,
                  "start",
                  "unix:/var/run/rmilter/rmilter.sock"]])
  end

  private
  def redhat_init_detector(name, options={})
    detector = Milter::Manager::RedHatInitDetector.new(@configuration, name)

    _init_base_dir = @init_base_dir
    _etc_dir = @etc_dir
    _candidate_service_commands = options[:candidate_service_commands] || []

    singleton_object = class << detector; self; end
    singleton_object.send(:define_method, :init_base_dir) do
      _init_base_dir.to_s
    end

    singleton_object.send(:define_method, :etc_dir) do
      _etc_dir.to_s
    end

    singleton_object.send(:define_method, :candidate_service_commands) do
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

  def milter_manager_init_script
    fixture_path("milter-manager.init").read
  end

  def amavisd_init_header
    <<-'EOS'
#!/bin/bash
#
# Init script for AMaViS email virus scanner.
#
# Written by Dag Wieers <dag@wieers.com>.
#
# chkconfig: 2345 79 31
# description: AMaViS virus scanner.
#
# processname: amavisd
# config: /etc/amavisd.conf
# pidfile: /var/run/amavisd.pid

source /etc/rc.d/init.d/functions

[ -x /usr/sbin/amavisd ] || exit 1
[ -r /etc/amavisd.conf ] || exit 1

### Default variables
AMAVIS_USER="amavis"
CONFIG_FILE="/etc/amavisd.conf"
MILTER_SOCKET=""
MILTER_FLAGS=""
SYSCONFIG="/etc/sysconfig/amavisd"
EOS
  end

  def clamav_milter_before_0_95_init_header
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

  def clamav_milter_epel_init_header
    <<-'EOS'
#!/bin/sh
#
# clamav-milter This script starts and stops the clamav-milter daemon
#
# chkconfig: - 79 40
#
# description: clamav-milter is a daemon which hooks into sendmail and routes \
#              email messages for virus scanning with ClamAV
# processname: clamav-milter
# pidfile: /var/lock/subsys/clamav-milter

# Source function library.
. /etc/rc.d/init.d/functions

# Source networking configuration.
. /etc/sysconfig/network

# Local clamav-milter config
CLAMAV_FLAGS=
test -f /etc/sysconfig/clamav-milter && . /etc/sysconfig/clamav-milter
EOS
  end

  def milter_greylist_init_header(conf_file=nil)
    <<-EOS
#!/bin/sh
# $Id: rc-redhat.sh.in,v 1.7 2006/08/20 05:20:51 manu Exp $
#  init file for milter-greylist
#
# chkconfig: - 79 21
# description: Milter Greylist Daemon
#
# processname: /usr/bin/milter-greylist
# config: #{conf_file || '/etc/mail/greylist.conf'}
# pidfile: /var/milter-greylist/milter-greylist.pid

# source function library
. /etc/init.d/functions

pidfile="/var/milter-greylist/milter-greylist.pid"
socket="/var/milter-greylist/milter-greylist.sock"
user="root"
OPTIONS="-P $pidfile -p $socket"
if [ -f /etc/sysconfig/milter-greylist ]
then
    . /etc/sysconfig/milter-greylist
fi
RETVAL=0
prog="Milter-Greylist"
EOS
  end

  def milter_greylist_init_header_fedora(conf_file)
    <<-EOS
#!/bin/sh
# $Id: rc-redhat.sh.in,v 1.7 2006/08/20 05:20:51 manu Exp $
#  init file for milter-greylist
#
# chkconfig: - 79 21
# description: Milter Greylist Daemon
#
# processname: /usr/sbin/milter-greylist
# config: #{conf_file}
# pidfile: /var/run/milter-greylist.pid

# source function library
. /etc/init.d/functions

pidfile="/var/run/milter-greylist.pid"
OPTIONS="-P $pidfile"
if [ -f /etc/sysconfig/milter-greylist ]
then
    . /etc/sysconfig/milter-greylist
fi
RETVAL=0
prog="Milter-Greylist"
EOS
  end

  def spamass_milter_init_header
    <<-'EOS'
#!/bin/bash
#
# Init file for Spamassassin sendmail milter.
#
# chkconfig: - 80 20
# description: spamass-milter is a daemon which hooks into sendmail and routes \
#              email messages to spamassassin
#
# processname: spamass-milter
# config: /etc/sysconfig/spamass-milter
# pidfile: /var/run/spamass-milter

source /etc/rc.d/init.d/functions
source /etc/sysconfig/network

# Check that networking is up.
[ ${NETWORKING} = "no" ] && exit 0

[ -x /usr/sbin/spamass-milter ] || exit 1

### Default variables
SOCKET="/var/run/spamass.sock"
EXTRA_FLAGS="-m -r 15"
SYSCONFIG="/etc/sysconfig/spamass-milter"

### Read configuration
[ -r "$SYSCONFIG" ] && source "$SYSCONFIG"

RETVAL=0
prog="spamass-milter"
desc="Spamassassin sendmail milter"
EOS
  end

  def enma_init_header(enma_conf)
    <<-EOS
#!/bin/bash
#
# Copyright (c) 2008-2009 Internet Initiative Japan Inc. All rights reserved.
#
# The terms and conditions of the accompanying program
# shall be provided separately by Internet Initiative Japan Inc.
# Any use, reproduction or distribution of the program are permitted
# provided that you agree to be bound to such terms and conditions.
#
# $Id: rc.enma-centos 882 2009-04-02 01:02:28Z takahiko $
# 
# chkconfig: 345 79 31
# description: A milter program for domain authentication technologies

# source function library
. /etc/init.d/functions

RETVAL=0
prog=enma

ENMA=/usr/libexec/enma
CONF_FILE=#{enma_conf}
LOCK_FILE=/var/lock/subsys/enma
EOS
  end

  def opendkim_init_header(opendkim_conf)
    <<-EOS
#!/bin/sh
#
# $Id: opendkim.init,v 1.1 2009/08/13 08:54:09 mmarkley Exp $
#
#### BEGIN INIT INFO
# Provides:          opendkim
# Required-Start:    $syslog $time $local_fs $remote_fs $named $network
# Required-Stop:     $syslog $time $local_fs $remote_fs $named
# Default-Start:     2 3 4 5
# Default-Stop:      S 0 1 6
# Short-Description: OpenDKIM Milter
# Description:       The OpenDKIM milter for signing and verifying email
#                    messages using the DomainKeys Identified Mail protocol
### END INIT INFO
#
# chkconfig: 345 20 80
# description: OpenDKIM milter for signing and verifying email
# processname: opendkim
#
# This script should run successfully on any LSB-compliant system. It will
# attempt to fall back to RedHatisms if LSB isn't available, or to
# commonly-available utilities if it's not even RedHat.

NAME=opendkim
PATH=/bin:/usr/bin:/sbin:/usr/sbin
DAEMON=/usr/sbin/$NAME
PIDFILE=/var/run/$NAME/$NAME.pid
CONFIG=#{opendkim_conf}
USER=opendkim
EOS
  end

  def rmilter_init_header(rmilter_conf)
    <<-HEADER
#!/bin/sh
#
# rmilter - this script starts and stops the rmilter daemon
#
# chkconfig:   - 85 15 
# description:  rmilter is a spam filtering system
# processname: rmilter
# config:      #{rmilter_conf}
# config:      /etc/sysconfig/rmilter
# pidfile:     /var/run/rmilter/rmilter.pid

# Source function library.
. /etc/rc.d/init.d/functions

# Source networking configuration.
. /etc/sysconfig/network

# Check that networking is up.
[ "$NETWORKING" = "no" ] && exit 0

rmilter="/usr/sbin/rmilter"
prog=$(basename $rmilter)

rmilter_CONF_FILE="#{rmilter_conf}"
rmilter_USER="_rmilter"
rmilter_GROUP="adm"
    HEADER
  end
end
