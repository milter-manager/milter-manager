# Copyright (C) 2008-2021  Sutou Kouhei <kou@clear-code.com>
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

class TestFreeBSDRCDetector < Test::Unit::TestCase
  include MilterDetectorTestUtils

  def setup
    @configuration = Milter::Manager::Configuration.new
    @loader = Milter::Manager::ConfigurationLoader.new(@configuration)
    @tmp_dir = Pathname(Dir.mktmpdir)
    @rc_d = @tmp_dir + "rc.d"
    @rc_conf = @tmp_dir + "rc.conf"
    @rc_conf_local = @tmp_dir + "rc.conf.local"
    @rc_conf_d = @tmp_dir + "rc.conf.d"
    @rc_d.mkpath
    @rc_conf_d.mkpath
  end

  def teardown
    FileUtils.rm_rf(@tmp_dir.to_s)
  end

  def test_rc_script_readable?
    omit("root user can read all files") if Etc.getpwuid.uid.zero?
    detector = freebsd_rc_detector("milter-manager")
    assert_false(detector.rc_script_readable?)
    (@rc_d + "milter-manager").open("w") {}
    assert_true(detector.rc_script_readable?)
    (@rc_d + "milter-manager").chmod(0200)
    assert_false(detector.rc_script_readable?)
  end

  def test_name
    (@rc_d + "milter-manager").open("w") do |file|
      file << milter_manager_rc_script
    end
    detector = freebsd_rc_detector("milter-manager")
    detector.detect
    assert_equal("milter_manager", detector.name)
  end

  def test_variables
    (@rc_d + "milter-manager").open("w") do |file|
      file << milter_manager_rc_script
    end
    detector = freebsd_rc_detector("milter-manager")
    detector.detect
    assert_equal({
                   "enable" => "NO",
                   "debug" => "NO",
                   "pid_file" => "/var/run/milter-manager/milter-manager.pid",
                   "user_name" => "mailnull",
                   "group_name" => "mail",
                   "socket_group_name" => "mail",
                   "connection_spec" => "",
                 },
                 detector.variables)
  end

  def test_parse_rc_conf
    (@rc_d + "milter-manager").open("w") do |file|
      file << milter_manager_rc_script
    end
    @rc_conf.open("w") do |file|
      file << <<-EOC
milter_manager_enable="YES"
# milter_manager_pid_file="/tmp/milter-manager.pid"
milter_manager_connection_spec=inet:10025@localhost
EOC
    end

    detector = freebsd_rc_detector("milter-manager")
    detector.detect
    assert_equal({
                   "enable" => "YES",
                   "debug" => "NO",
                   "pid_file" => "/var/run/milter-manager/milter-manager.pid",
                   "user_name" => "mailnull",
                   "group_name" => "mail",
                   "socket_group_name" => "mail",
                   "connection_spec" => "inet:10025@localhost",
                 },
                 detector.variables)
    assert_true(detector.enabled?)
  end

  def test_apply
    (@rc_d + "milter-manager").open("w") do |file|
      file << milter_manager_rc_script
    end
    @rc_conf.open("w") do |file|
      file << <<-EOC
milter_manager_enable="YES"
milter_manager_connection_spec=inet:10025@localhost
EOC
    end

    detector = freebsd_rc_detector("milter-manager")
    detector.detect
    detector.apply(@loader)
    assert_eggs([["milter-manager",
                  nil,
                  true,
                  (@rc_d + "milter-manager").to_s,
                  "start",
                  "inet:10025@localhost"]])
  end

  def test_apply_amavis_milter_style
    (@rc_d + "amavis-milter").open("w") do |file|
      file << <<-EOM
name=amavis_milter
amavis_milter_enable=${amavis_milter_enable:-"NO"}
amavis_milter_flags=${amavis_milter_flags:-"-D -p /var/amavis/amavis-milter.sock"}
EOM
    end
    @rc_conf.open("w") do |file|
      file << "amavis_milter_enable=YES"
    end

    detector = freebsd_rc_detector("amavis-milter")
    detector.detect
    detector.apply(@loader)
    assert_equal("amavis_milter", detector.name)
    assert_eggs([["amavis-milter",
                  nil,
                  true,
                  (@rc_d + "amavis-milter").to_s,
                  "start",
                  "unix:/var/amavis/amavis-milter.sock"]])
  end

  def test_apply_clamav_milter_before_0_95_style
    (@rc_d + "clamav-milter").open("w") do |file|
      file << <<-EOM
name=clamav_milter
: ${clamav_milter_enable="NO"}
: ${clamav_milter_socket="/var/run/clamav/clmilter.sock"}
EOM
    end

    detector = freebsd_rc_detector("clamav-milter")
    detector.detect
    detector.apply(@loader)
    assert_equal("clamav_milter", detector.name)
    assert_eggs([["clamav-milter",
                  nil,
                  false,
                  (@rc_d + "clamav-milter").to_s,
                  "start",
                  "unix:/var/run/clamav/clmilter.sock"]])
  end

  def test_apply_clamav_milter_since_0_95_style
    clamav_milter_conf = @tmp_dir + "clamav-milter.conf"
    (@rc_d + "clamav-milter").open("w") do |file|
      file << <<-EOM
name=clamav_milter
rcvar=`set_rcvar`

conf_file=#{clamav_milter_conf}
command=/usr/local/sbin/clamav-milter
required_dirs=/var/db/clamav
required_files=${conf_file}

# ...

# read settings, set default values
load_rc_config $name
: ${clamav_milter_enable="NO"}
: ${clamav_milter_socket="/var/run/clamav/clamav-milter.sock"}
: ${clamav_milter_flags="-c ${conf_file}"}
: ${clamav_milter_socktimeout="60"}
: ${clamav_milter_socket_mode="755"}
: ${clamav_milter_socket_user="clamav"}
: ${clamav_milter_socket_group="clamav"}
EOM
    end

    clamav_milter_conf.open("w") do |conf|
      conf << <<-EOC
# Default: no default
MilterSocket /var/run/clamav/clmilter.sock
#MilterSocket inet:7357
EOC
    end

    detector = freebsd_rc_detector("clamav-milter")
    detector.detect
    detector.apply(@loader)
    assert_equal("clamav_milter", detector.name)
    assert_eggs([["clamav-milter",
                  nil,
                  false,
                  (@rc_d + "clamav-milter").to_s,
                  "start",
                  "unix:/var/run/clamav/clmilter.sock"]])
  end

  def test_apply_milter_dkim_style
    (@rc_d + "milter-dkim").open("w") do |file|
      file << <<-EOM
milterdkim_enable=${milterdkim_enable:-"NO"}
milterdkim_socket=${milterdkim_socket:-}
name=milterdkim
EOM
    end
    @rc_conf.open("w") do |file|
      file << <<-EOC
milterdkim_enable=YES
milterdkim_socket="/var/run/milter/milter-dkim.sock"
EOC
    end

    detector = freebsd_rc_detector("milter-dkim")
    detector.detect
    detector.apply(@loader)
    assert_equal("milterdkim", detector.name)
    assert_eggs([["milter-dkim",
                  nil,
                  true,
                  (@rc_d + "milter-dkim").to_s,
                  "start",
                  "unix:/var/run/milter/milter-dkim.sock"]])
  end

  def test_apply_opendkim_style
    opendkim_conf = @tmp_dir + "opendkim.conf"
    (@rc_d + "milter-opendkim").open("w") do |file|
      file << <<-EOM
name=milteropendkim
: ${milteropendkim_enable="NO"}
: ${milteropendkim_uid="mailnull"}
: ${milteropendkim_profiles=""}
: ${milteropendkim_cfgfile="#{opendkim_conf}"}

: ${milteropendkim_socket=""}
: ${milteropendkim_domain=""}
: ${milteropendkim_key=""}
: ${milteropendkim_selector=""}
: ${milteropendkim_alg=""}
EOM
    end
    @rc_conf.open("w") do |file|
      file << <<-EOC
milteropendkim_enable=YES
milteropendkim_socket="/var/run/milter/milter-opendkim.sock"
EOC
    end

    detector = freebsd_rc_detector("milter-opendkim")
    detector.detect
    detector.apply(@loader)
    assert_equal("milteropendkim", detector.name)
    assert_eggs([["milter-opendkim",
                  nil,
                  true,
                  (@rc_d + "milter-opendkim").to_s,
                  "start",
                  "unix:/var/run/milter/milter-opendkim.sock"]])
  end

  def test_apply_opendkim_style_profile
    opendkim_conf = @tmp_dir + "opendkim.conf"
    opendkim_test_conf = @tmp_dir + "opendkim-test.conf"
    (@rc_d + "milter-opendkim").open("w") do |file|
      file << <<-EOM
name=milteropendkim
: ${milteropendkim_enable="NO"}
: ${milteropendkim_uid="mailnull"}
: ${milteropendkim_profiles=""}
: ${milteropendkim_cfgfile="#{opendkim_conf}"}

: ${milteropendkim_socket=""}
: ${milteropendkim_domain=""}
: ${milteropendkim_key=""}
: ${milteropendkim_selector=""}
: ${milteropendkim_alg=""}
EOM
    end
    @rc_conf.open("w") do |file|
      file << <<-EOC
milteropendkim_enable=YES
milteropendkim_socket="/var/run/milter/milter-opendkim.sock"

milteropendkim_profiles="inner outer test"

milteropendkim_inner_enable=NO
milteropendkim_outer_socket="/var/run/milter/milter-opendkim-outer.sock"
milteropendkim_test_cfgfile="#{opendkim_test_conf}"
EOC
    end
    opendkim_test_conf.open("w") do |file|
      file << <<-EOC
Socket inet:12345
EOC
    end

    detector = freebsd_rc_detector("milter-opendkim")
    detector.detect
    detector.apply(@loader)
    assert_equal("milteropendkim", detector.name)
    assert_eggs([["milter-opendkim-inner",
                  nil,
                  false,
                  (@rc_d + "milter-opendkim").to_s,
                  "start inner",
                  "unix:/var/run/milter/milter-opendkim.sock"],
                 ["milter-opendkim-outer",
                  nil,
                  true,
                  (@rc_d + "milter-opendkim").to_s,
                  "start outer",
                  "unix:/var/run/milter/milter-opendkim.sock"],
                 ["milter-opendkim-test",
                  nil,
                  true,
                  (@rc_d + "milter-opendkim").to_s,
                  "start test",
                  "inet:12345"]])
  end

  def test_apply_milter_greylist_style
    milter_greylist_conf = @tmp_dir + "greylist.conf"
    (@rc_d + "milter-greylist").open("w") do |file|
      file << <<-EOM
name="miltergreylist"

miltergreylist_enable=${miltergreylist_enable-"NO"}
miltergreylist_sockfile=${miltergreylist_sockfile-"/var/milter-greylist/milter-greylist.sock"}
miltergreylist_cfgfile=${miltergreylist_cfgfile-"#{milter_greylist_conf}"}
EOM
    end

    detector = freebsd_rc_detector("milter-greylist")
    detector.detect
    detector.apply(@loader)
    assert_equal("miltergreylist", detector.name)
    assert_eggs([["milter-greylist",
                  nil,
                  false,
                  (@rc_d + "milter-greylist").to_s,
                  "start",
                  "unix:/var/milter-greylist/milter-greylist.sock"]])

    milter_greylist_egg = @configuration.eggs[0]
    assert_equal([7.0, 7.0],
                 [milter_greylist_egg.writing_timeout,
                  milter_greylist_egg.reading_timeout])
  end

  def test_apply_milter_greylist_tarpit_style
    milter_greylist_conf = @tmp_dir + "greylist.conf"
    (@rc_d + "milter-greylist").open("w") do |file|
      file << <<-EOM
name="miltergreylist"

miltergreylist_enable=${miltergreylist_enable-"NO"}
miltergreylist_sockfile=${miltergreylist_sockfile-"/var/milter-greylist/milter-greylist.sock"}
miltergreylist_cfgfile=${miltergreylist_cfgfile-"#{milter_greylist_conf}"}
EOM
    end

    milter_greylist_conf.open("w") do |conf|
      conf << <<-EOC
pidfile "/var/run/milter-greylist.pid"
socket "/var/milter-greylist/milter-greylist.sock"
dumpfile "/var/milter-greylist/greylist.db"

racl whitelist tarpit 125s
#racl whitelist default
racl greylist default
EOC
    end

    detector = freebsd_rc_detector("milter-greylist")
    detector.detect
    detector.apply(@loader)
    assert_equal("miltergreylist", detector.name)
    assert_eggs([["milter-greylist",
                  nil,
                  false,
                  (@rc_d + "milter-greylist").to_s,
                  "start",
                  "unix:/var/milter-greylist/milter-greylist.sock"]])

    milter_greylist_egg = @configuration.eggs[0]
    assert_equal([132.0, 132.0],
                 [milter_greylist_egg.writing_timeout,
                  milter_greylist_egg.reading_timeout])
  end

  def test_apply_enma_style
    enma_conf = @tmp_dir + "enma.conf"
    (@rc_d + "milter-enma").open("w") do |file|
      file << <<-EOM
milterenma_enable=${milterenma_enable:-"NO"}
milterenma_cfgfile=${milterenma_cfgfile:-"#{enma_conf}"}
milterenma_pid=${milterenma_pid:-"/var/run/milterenma/enma.pid"}
milterenma_uid=${milterenma_uid:-"mailnull"}

. /etc/rc.subr

name="milterenma"
rcvar=`set_rcvar`

load_rc_config $name

if [ -f "${milterenma_cfgfile}" ];then
    milterenma_cfgfile="-c ${milterenma_cfgfile}"
else
    echo "milterenma_cfgfile is not correctly set"
    exit 1
fi
pidfile=${milterenma_pid}
command="/usr/local/bin/enma"
command_args="${milterenma_cfgfile}"
start_precmd="enma_precmd"
stop_postcmd="enma_postcmd"
_piddir=$(dirname ${pidfile})
EOM
    end

    enma_conf.open("w") do |conf|
      conf << <<-EOC
## Milter ##
milter.socket:  inet:10025@127.0.0.1
milter.user:    mailnull
milter.pidfile: /var/run/milterenma/enma.pid
milter.chdir:   /var/tmp
milter.timeout: 7210
milter.loglevel:   0
milter.postfix: false
EOC
    end

    detector = freebsd_rc_detector("milter-enma")
    detector.detect
    detector.apply(@loader)
    assert_equal("milterenma", detector.name)
    assert_eggs([["milter-enma",
                  nil,
                  false,
                  (@rc_d + "milter-enma").to_s,
                  "start",
                  "inet:10025@127.0.0.1"]])
  end

  def test_apply_rmilter_style
    rmilter_conf = @tmp_dir + "rmilter.conf"
    (@rc_d + "rmilter").open("w") do |rc|
      rc << <<-RC
name="rmilter"
rcvar=rmilter_enable
command="/usr/local/sbin/rmilter"

load_rc_config $name

: ${rmilter_enable="NO"}
: ${rmilter_pidfile="/var/run/rmilter/rmilter.pid"}
: ${rmilter_socket="/var/run/rmilter/rmilter.sock"}
: ${rmilter_user="_rmilter"}
: ${rmilter_group="mail"}

command_args="$procname -c /usr/local/etc/rmilter.conf"

run_rc_command "$1"
      RC
    end

    rmilter_conf.open("w") do |conf|
      conf << <<-CONFIG
# Sample config file for rmilter
# $Id$
#

# .include - directive to include other config file
#.include ./rmilter-grey.conf

# pidfile - path to pid file
#   Default: pidfile = /var/run/rmilter.pid
pidfile = /var/run/rmilter/rmilter.pid;

# bind_socket - socket credits for local bind:
# unix:/path/to/file - bind to local socket
# inet:port@host - bind to inet socket
#   Default: bind_socket = unix:/var/tmp/rmilter.sock;
bind_socket = unix:/var/run/rmilter/rmilter.sock;
      CONFIG
    end

    detector = freebsd_rc_detector("rmilter")
    detector.detect
    detector.apply(@loader)
    assert_equal("rmilter", detector.name)
    assert_eggs([["rmilter",
                  nil,
                  false,
                  (@rc_d + "rmilter").to_s,
                  "start",
                  "unix:/var/run/rmilter/rmilter.sock"]])
  end

  private
  def freebsd_rc_detector(name, options={})
    detector = Milter::Manager::FreeBSDRCDetector.new(@configuration, name)

    _rc_d = @rc_d
    _rc_conf = @rc_conf
    _rc_conf_local = @rc_conf_local
    _rc_conf_d = @rc_conf_d
    _candidate_service_commands = options[:candidate_service_commands] || []
    singleton_object = class << detector; self; end
    singleton_object.send(:define_method, :rc_d) do
      _rc_d.to_s
    end

    singleton_object.send(:define_method, :rc_conf) do
      _rc_conf.to_s
    end

    singleton_object.send(:define_method, :rc_conf_local) do
      _rc_conf_local.to_s
    end

    singleton_object.send(:define_method, :rc_conf_d) do
      _rc_conf_d.to_s
    end

    singleton_object.send(:define_method, :candidate_service_commands) do
      _candidate_service_commands
    end

    detector
  end

  def milter_manager_rc_script
    top = Pathname(File.dirname(__FILE__)) + ".." + ".." + ".." + ".."
    (top + "data" + "rc.d" + "milter-manager").read
  end
end
