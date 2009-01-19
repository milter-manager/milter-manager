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

class TestRedHatInitDetector < Test::Unit::TestCase
  include MilterTestUtils

  def setup
    @configuration = Milter::Manager::Configuration.new
    @loader = Milter::Manager::ConfigurationLoader.new(@configuration)
    @tmp_dir = Pathname(File.dirname(__FILE__)) + ".." + "tmp"
    @init_d = @tmp_dir + "init.d"
    @sysconfig_dir = @tmp_dir + "sysconfig"
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
    assert_equal({
                   "name" => "milter manager",
                   "prog" => "milter-manager",
                   "milter_manager" => "/usr/sbin/milter-manager",
                   "USER" => "milter-manager",
                   "GROUP" => "smmsp",
                   "PIDFILE" => "/var/run/milter-manager/milter-manager.pid",
                   "CONNECTION_SPEC" => "unix:/var/run/milter-manager/milter-manager.sock",
                   "DAEMON_ARGS" => ["--daemon",
                                     "--pid-file",
                                     "/var/run/milter-manager/milter-manager.pid",
                                     "--connection-spec",
                                     "unix:/var/run/milter-manager/milter-manager.sock",
                                     "--user-name",
                                     "milter-manager",
                                     "--group-name",
                                     "smmsp"].join(" ") + " ",
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
    (@sysconfig_dir + "milter-manager").open("w") do |file|
      file << <<-EOC
CONNECTION_SPEC=inet:10025@localhost
# CONNECTION_SPEC=inet6:2929
EOC
    end

    detector = redhat_init_detector("milter-manager")
    detector.detect
    assert_equal({
                   "name" => "milter manager",
                   "prog" => "milter-manager",
                   "milter_manager" => "/usr/sbin/milter-manager",
                   "USER" => "milter-manager",
                   "GROUP" => "smmsp",
                   "PIDFILE" => "/var/run/milter-manager/milter-manager.pid",
                   "CONNECTION_SPEC" => "inet:10025@localhost",
                   "DAEMON_ARGS" => ["--daemon",
                                     "--pid-file",
                                     "/var/run/milter-manager/milter-manager.pid",
                                     "--connection-spec",
                                     "unix:/var/run/milter-manager/milter-manager.sock",
                                     "--user-name",
                                     "milter-manager",
                                     "--group-name",
                                     "smmsp"].join(" ") + " ",
                   "OPTION_ARGS" => "",
                   "RETVAL" => "$?",
                 },
                 detector.variables)
    assert_true(detector.enabled?)
  end

  def test_apply
    (@init_d + "milter-manager").open("w") do |file|
      file << milter_manager_init_script
    end
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

  def test_apply_clamav_milter_style
    (@init_d + "clamav-milter").open("w") do |file|
      file << clamav_milter_init_header
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


  def test_apply_milter_greylist_style
    (@init_d + "milter-greylist").open("w") do |file|
      file << milter_greylist_init_header
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

  def test_apply_milter_greylist_style
    (@init_d + "milter-greylist").open("w") do |file|
      file << milter_greylist_init_header
    end
    (@sysconfig_dir + "milter-greylist").open("w") do |file|
      file << <<-'EOC'
OPTIONS="-P $pidfile -p inet:10025@localhost"
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

  def test_apply_spamass_milter_style
    (@init_d + "spamass-milter").open("w") do |file|
      file << spamass_milter_init_header
    end

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

  private
  def redhat_init_detector(name)
    detector = Milter::Manager::ConfigurationLoader::RedHatInitDetector.new(name)

    _init_d = @init_d
    _sysconfig_dir = @sysconfig_dir
    singleton_object = class << detector; self; end
    singleton_object.send(:define_method, :init_d) do
      _init_d.to_s
    end

    singleton_object.send(:define_method, :sysconfig_dir) do
      _sysconfig_dir.to_s
    end

    detector
  end

  def milter_manager_init_script
    top = Pathname(File.dirname(__FILE__)) + ".." + ".." + ".." + ".."
    (top + "data" + "init.d" + "redhat" + "milter-manager").read
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

  def milter_greylist_init_header
    <<-'EOS'
#!/bin/sh
# $Id: rc-redhat.sh.in,v 1.7 2006/08/20 05:20:51 manu Exp $
#  init file for milter-greylist
#
# chkconfig: - 79 21
# description: Milter Greylist Daemon
#
# processname: /usr/bin/milter-greylist
# config: /etc/mail/greylist.conf
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

  def assert_eggs(expected_eggs)
    egg_info = @configuration.eggs.collect do |egg|
      [egg.name,
       egg.description,
       egg.enabled?,
       egg.command,
       egg.command_options,
       egg.connection_spec]
    end
    assert_equal(expected_eggs, egg_info)
  end
end
