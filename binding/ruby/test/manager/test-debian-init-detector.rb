# -*- coding: utf-8 -*-
#
# Copyright (C) 2008-2009  Kouhei Sutou <kou@cozmixng.org>
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

class TestDebianInitDetector < Test::Unit::TestCase
  include MilterTestUtils

  def setup
    @configuration = Milter::Manager::Configuration.new
    @loader = Milter::Manager::ConfigurationLoader.new(@configuration)
    @tmp_dir = Pathname(File.dirname(__FILE__)) + ".." + "tmp"
    @init_base_dir = @tmp_dir
    @init_d = @init_base_dir + "init.d"
    @default_dir = @tmp_dir + "default"
    @init_d.mkpath
    @default_dir.mkpath
  end

  def teardown
    FileUtils.rm_rf(@tmp_dir.to_s)
  end

  def test_init_script_exist?
    detector = debain_init_detector("milter-manager")
    assert_false(detector.init_script_exist?)
    (@init_d + "milter-manager").open("w") {}
    assert_true(detector.init_script_exist?)
  end

  def test_name
    (@init_d + "milter-manager").open("w") do |file|
      file << milter_manager_init_script
    end
    detector = debain_init_detector("milter-manager")
    detector.detect
    assert_equal("milter-manager", detector.name)
  end

  def test_variables
    (@init_d + "milter-manager").open("w") do |file|
      file << milter_manager_init_script
    end
    detector = debain_init_detector("milter-manager")
    detector.detect
    assert_equal({
                   "PATH" => "/sbin:/usr/sbin:/bin:/usr/bin",
                   "DESC" => "flexible milter management daemon",
                   "NAME" => "milter-manager",
                   "DAEMON" => "/usr/sbin/milter-manager",
                   "USER" => "milter-manager",
                   "GROUP" => "mail",
                   "SOCKET_GROUP" => "mail",
                   "PIDFILE" => "/var/run/milter-manager/milter-manager.pid",
                   "CONNECTION_SPEC" => "unix:/var/run/milter-manager/milter-manager.sock",
                   "SCRIPTNAME" => "/etc/init.d/milter-manager",
                   "DAEMON_ARGS" => ["--daemon",
                                     "--pid-file",
                                     "/var/run/milter-manager/milter-manager.pid",
                                     "--connection-spec",
                                     "unix:/var/run/milter-manager/milter-manager.sock",
                                     "--user-name",
                                     "milter-manager",
                                     "--group-name", "mail",
                                     "--socket-group-name", "mail",
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
    detector = debain_init_detector("milter-manager")
    detector.detect
    assert_equal({
                   "Provides" => "milter-manager",
                   "Required-Start" => "$local_fs $remote_fs $network $syslog",
                   "Required-Stop" => "$local_fs $remote_fs $network $syslog",
                   "Default-Start" => "2 3 4 5",
                   "Default-Stop" => "0 1 6",
                   "Short-Description" => "milter-manager's init script",
                   "Description" =>
                     "The milter that manages milters to combine them flexibly.",
                 },
                 detector.info)
  end

  def test_parse_init_conf
    (@init_d + "milter-manager").open("w") do |file|
      file << milter_manager_init_script
    end
    (@default_dir + "milter-manager").open("w") do |file|
      file << <<-EOC
ENABLED="0"
CONNECTION_SPEC=inet:10025@localhost
# CONNECTION_SPEC=inet6:2929
EOC
    end

    detector = debain_init_detector("milter-manager")
    detector.detect
    assert_equal({
                   "ENABLED" => "0",
                   "PATH" => "/sbin:/usr/sbin:/bin:/usr/bin",
                   "DESC" => "flexible milter management daemon",
                   "NAME" => "milter-manager",
                   "DAEMON" => "/usr/sbin/milter-manager",
                   "USER" => "milter-manager",
                   "GROUP" => "mail",
                   "SOCKET_GROUP" => "mail",
                   "PIDFILE" => "/var/run/milter-manager/milter-manager.pid",
                   "CONNECTION_SPEC" => "inet:10025@localhost",
                   "SCRIPTNAME" => "/etc/init.d/milter-manager",
                   "DAEMON_ARGS" => ["--daemon",
                                     "--pid-file",
                                     "/var/run/milter-manager/milter-manager.pid",
                                     "--connection-spec",
                                     "unix:/var/run/milter-manager/milter-manager.sock",
                                     "--user-name",
                                     "milter-manager",
                                     "--group-name", "mail",
                                     "--socket-group-name", "mail",
                                    ].join(" ") + " ",
                   "OPTION_ARGS" => "",
                   "RETVAL" => "$?",
                 },
                 detector.variables)
    assert_false(detector.enabled?)
  end

  def test_apply
    (@init_d + "milter-manager").open("w") do |file|
      file << milter_manager_init_script
    end
    (@default_dir + "milter-manager").open("w") do |file|
      file << <<-EOC
CONNECTION_SPEC=inet:10025@localhost
EOC
    end

    detector = debain_init_detector("milter-manager")
    detector.detect
    detector.apply(@loader)
    assert_eggs([["milter-manager",
                  "The milter that manages milters to combine them flexibly.",
                  true,
                  (@init_d + "milter-manager").to_s,
                  "start",
                  "inet:10025@localhost"]])
  end

  def test_apply_amavisd_new_milter_style
    (@init_d + "amavisd-new-milter").open("w") do |file|
      file << <<-EOM
#! /bin/sh
#
# amavisd-new-milter    /etc/init.d/ initscript for amavisd-new milter
# 		$Id: amavisd-new-milter.init 411 2004-05-15 04:45:59Z hmh $
#
#		Copyright (c) 2003 by Brian May <bam@debian.org>
#			and Henrique M. Holschuh <hmh@debian.org>
#		Distributed under the GPL version 2
#
### BEGIN INIT INFO
# Provides:          amavisd-new-milter
# Required-Start:    $syslog 
# Required-Stop:     $syslog
# Should-Start:      $local_fs
# Should-Stop:       $local_fs
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6
# Short-Description: Starts the milter for amavisd-new 
# Description:       milter for sendmail/postfix and amavisd-new
### END INIT INFO

PATH=/sbin:/bin:/usr/sbin:/usr/bin
DAEMON=/usr/sbin/amavis-milter
PARENTDAEMON=/usr/sbin/amavisd-new
NAME=amavis-milter
DESC="AMaViS Daemons (milter)"
PARAMS="-D -p /var/lib/amavis/amavisd-new-milter.sock"

test -f ${PARENTDAEMON} || exit 0
test -f ${DAEMON} || exit 0

set -e

START="--start --quiet --pidfile /var/run/amavis/amavisd-new-milter.pid --chuid amavis --startas ${DAEMON} --name ${NAME} -- ${PARAMS}"
EOM
    end

    detector = debain_init_detector("amavisd-new-milter")
    detector.detect
    detector.apply(@loader)
    assert_equal("amavis-milter", detector.name)
    assert_eggs([["amavisd-new-milter",
                  "milter for sendmail/postfix and amavisd-new",
                  true,
                  (@init_d + "amavisd-new-milter").to_s,
                  "start",
                  "unix:/var/lib/amavis/amavisd-new-milter.sock"]])
  end

  def test_apply_clamav_milter_style
    (@init_d + "clamav-milter").open("w") do |file|
      file << <<-EOM
#!/bin/sh
### BEGIN INIT INFO
# Provides:          clamav-milter
# Required-Start:    $syslog
# Should-Start:      clamav-daemon
# Required-Stop:
# Should-Stop:       
# Default-Start:     2 3 4 5
# Default-Stop:      0 6
# Short-Description: ClamAV virus milter
# Description:       Clam AntiVirus milter interface
### END INIT INFO

# edit /etc/default/clamav-milter for options

PATH=/sbin:/bin:/usr/sbin:/usr/bin
DAEMON=/usr/sbin/clamav-milter
DESC="Sendmail milter plugin for ClamAV"
BASENAME=clamav-milter
CLAMAVCONF=/etc/clamav/clamd.conf
DEFAULT=/etc/default/clamav-milter
OPTIONS="-dq"
SUPERVISOR=/usr/bin/daemon
SUPERVISORPIDFILE="/var/run/clamav/daemon-clamav-milter.pid"
SUPERVISORARGS="-F $SUPERVISORPIDFILE --name=$BASENAME --respawn"
CLAMAVDAEMONUPGRADE="/var/run/clamav-daemon-being-upgraded"

[ -x "$DAEMON" ] || exit 0
[ -r "$CLAMAVCONF" ] || exit 0
[ -r "$DEFAULT" ] && . $DEFAULT
[ -z "$PIDFILE" ] && PIDFILE=/var/run/clamav/clamav-milter.pid
[ -z "$SOCKET" ] && SOCKET=local:/var/run/clamav/clamav-milter.ctl

case "$SOCKET" in
  /*)
  SOCKET_PATH="$SOCKET"
  SOCKET="local:$SOCKET"
  ;;
  *)
  SOCKET_PATH=`echo $SOCKET | sed -e s/local\://`
  # If the socket is type inet: we don't care - we can't rm -f that later :)
  ;;
esac
EOM
    end

    detector = debain_init_detector("clamav-milter")
    detector.detect
    detector.apply(@loader)
    assert_eggs([["clamav-milter",
                  "Clam AntiVirus milter interface",
                  true,
                  (@init_d + "clamav-milter").to_s,
                  "start",
                  "local:/var/run/clamav/clamav-milter.ctl"]])
  end

  def test_apply_dkim_filter_style
    (@init_d + "dkim-filter").open("w") do |file|
      file << <<-EOM
#! /bin/sh
#
### BEGIN INIT INFO
# Provides:		dkim-filter
# Required-Start:	$syslog
# Required-Stop:	$syslog
# Should-Start:		$local_fs $network
# Should-Stop:		$local_fs $network
# Default-Start:	2 3 4 5
# Default-Stop:		0 1 6
# Short-Description:	Start the DKIM Milter service
### END INIT INFO

PATH=/sbin:/bin:/usr/sbin:/usr/bin
DAEMON=/usr/sbin/dkim-filter
NAME=dkim-filter
DESC="DKIM Filter"
RUNDIR=/var/run/$NAME
USER=dkim-filter
GROUP=dkim-filter
SOCKET=local:$RUNDIR/$NAME.sock
PIDFILE=$RUNDIR/$NAME.pid
EOM
    end
    (@default_dir + "dkim-filter").open("w") do |file|
      file << <<-EOC
SOCKET="inet:12345@192.0.2.1" # listen on 192.0.2.1 on port 12345
EOC
    end

    detector = debain_init_detector("dkim-filter")
    detector.detect
    detector.apply(@loader)
    assert_eggs([["dkim-filter",
                  "DKIM Filter",
                  true,
                  (@init_d + "dkim-filter").to_s,
                  "start",
                  "inet:12345@192.0.2.1"]])
  end

  def test_apply_milter_greylist_style
    (@init_d + "milter-greylist").open("w") do |file|
      file << <<-EOM
#! /bin/sh

# Greylist init script
# July 2004
# BERTRAND Joël
#
### BEGIN INIT INFO
# Provides:                 milter-greylist
# Required-Start:    $local_fs $named $remote_fs $syslog
# Required-Stop:     mountall
# Should-Start:             sendmail
# Should-Stop:
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6
# Short-Description: Script to start/stop the greylist-milter
# Description: another spam-defense service
### END INIT INFO



# Based on skeleton by Miquel van Smoorenburg and Ian Murdock

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin
DAEMON=/usr/sbin/milter-greylist
NAME=greylist
SNAME=greylist
DESC="Greylist Mail Filter Daemon"
PIDFILE="/var/run/$NAME.pid"
PNAME="milter-greylist"
USER="greylist"
SOCKET=/var/run/milter-greylist/milter-greylist.sock


[ -x $DAEMON ] || DAEMON=/usr/bin/milter-greylist
[ -x $DAEMON ] || exit 0


export TMPDIR=/tmp
# Apparently people have trouble if this isn't explicitly set...

ENABLED=0
OPTIONS=""
NICE=

test -f /etc/default/milter-greylist && . /etc/default/milter-greylist

DOPTIONS="-P $PIDFILE -u $USER -p $SOCKET"
EOM
    end
    (@default_dir + "milter-greylist").open("w") do |file|
      file << <<-EOC
ENABLED=1
EOC
    end

    detector = debain_init_detector("milter-greylist")
    detector.detect
    detector.apply(@loader)
    assert_equal("greylist", detector.name)
    assert_eggs([["milter-greylist",
                  "another spam-defense service",
                  true,
                  (@init_d + "milter-greylist").to_s,
                  "start",
                  "unix:/var/run/milter-greylist/milter-greylist.sock"]])
  end

  def test_apply_spamass_milter_style
    (@init_d + "spamass-milter").open("w") do |file|
      file << <<-EOM
### BEGIN INIT INFO
# Provides:          spamass-milter
# Required-Start:    $syslog $local_fs
# Required-Stop:     $syslog $local_fs
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6
# Short-Description: milter for spamassassin
# Description:       Calls spamassassin to allow filtering out
#                    spam from ham in libmilter compatible MTAs.
### END INIT INFO


PATH=/sbin:/bin:/usr/sbin:/usr/bin
NAME=spamass-milter
DAEMON=/usr/sbin/spamass-milter
SOCKET=/var/run/spamass/spamass.sock
PIDFILE=/var/run/spamass/spamass.pid
DESC="Sendmail milter plugin for SpamAssassin"

DEFAULT=/etc/default/spamass-milter
OPTIONS=""
RUNAS="spamass-milter"
CHUID=""
SOCKETMODE="0600"
SOCKETOWNER="root:root"

test -x $DAEMON || exit 0

if [ -e /etc/mail/sendmail.cf ] && egrep -q 'X.+S=local:/var/run/sendmail/spamass\.sock' /etc/mail/sendmail.cf; then
    SOCKET=/var/run/sendmail/spamass.sock
    SOCKETMODE=""
    SOCKETOWNER=""
    RUNAS=""
    echo "WARNING: You are using the old location of spamass.sock. Change your input filter to use";
    echo "/var/run/spamass/spamass.sock so spamass-milter can run as spamass-milter";
fi;

# If /usr/sbin/postfix exists, set up the defaults for a postfix install
# These can be overridden in /etc/default/spamass-milter
if [ -x /usr/sbin/postfix ]; then
    SOCKET="/var/spool/postfix/spamass/spamass.sock"
    SOCKETOWNER="postfix:postfix"
    SOCKETMODE="0660"
fi;
EOM
    end
    (@default_dir + "spamass-milter").open("w") do |file|
      file << <<-EOC
SOCKET="/tmp/spamass.sock"
EOC
    end

    detector = debain_init_detector("spamass-milter")
    detector.detect
    detector.apply(@loader)
    assert_eggs([["spamass-milter",
                  "Calls spamassassin to allow filtering out spam " +
                  "from ham in libmilter compatible MTAs.",
                  true,
                  (@init_d + "spamass-milter").to_s,
                  "start",
                  "unix:/tmp/spamass.sock"]])
  end

  private
  def debain_init_detector(name)
    detector = Milter::Manager::DebianInitDetector.new(@configuration, name)

    _init_base_dir = @init_base_dir
    _default_dir = @default_dir
    singleton_object = class << detector; self; end
    singleton_object.send(:define_method, :init_base_dir) do
      _init_base_dir.to_s
    end

    singleton_object.send(:define_method, :default_dir) do
      _default_dir.to_s
    end

    detector
  end

  def milter_manager_init_script
    top = Pathname(File.dirname(__FILE__)) + ".." + ".." + ".." + ".."
    (top + "data" + "init.d" + "debian" + "milter-manager").read
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
