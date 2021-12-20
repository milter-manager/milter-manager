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

class TestPkgsrcRCDetector < Test::Unit::TestCase
  include MilterDetectorTestUtils

  def setup
    @configuration = Milter::Manager::Configuration.new
    @loader = Milter::Manager::ConfigurationLoader.new(@configuration)
    @tmp_dir = Pathname(Dir.mktmpdir)
    @rc_d = @tmp_dir + "rc.d"
    @rc_conf = @tmp_dir + "rc.conf"
    @rc_conf_d = @tmp_dir + "rc.conf.d"
    @rc_d.mkpath
    @rc_conf_d.mkpath
  end

  def teardown
    FileUtils.rm_rf(@tmp_dir.to_s)
  end

  def test_rc_script_exist?
    detector = pkgsrc_rc_detector("milter-manager")
    assert_false(detector.rc_script_exist?)
    (@rc_d + "milter-manager").open("w") {}
    assert_true(detector.rc_script_exist?)
  end

  def test_name
    (@rc_d + "milter-manager").open("w") do |file|
      file << milter_manager_rc_script
    end
    detector = pkgsrc_rc_detector("milter-manager")
    detector.detect
    assert_equal("milter_manager", detector.name)
  end

  def test_variables
    (@rc_d + "milter-manager").open("w") do |file|
      file << milter_manager_rc_script
    end
    detector = pkgsrc_rc_detector("milter-manager")
    detector.detect
    assert_equal({
                   "debug" => "NO",
                   "pid_file" => "/var/run/milter-manager/milter-manager.pid",
                   "user_name" => "smmsp",
                   "group_name" => "mail",
                   "connection_spec" => "",
                 },
                 detector.variables)
    assert_equal("NO", detector.rcvar_value)
  end

  def test_parse_rc_conf
    (@rc_d + "milter-manager").open("w") do |file|
      file << milter_manager_rc_script
    end
    @rc_conf.open("w") do |file|
      file << <<-EOC
milter_manager="YES"
# milter_manager_pid_file="/tmp/milter-manager.pid"
milter_manager_connection_spec=inet:10025@localhost
EOC
    end

    detector = pkgsrc_rc_detector("milter-manager")
    detector.detect
    assert_equal({
                   "debug" => "NO",
                   "pid_file" => "/var/run/milter-manager/milter-manager.pid",
                   "user_name" => "smmsp",
                   "group_name" => "mail",
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
milter_manager="YES"
milter_manager_connection_spec=inet:10025@localhost
EOC
    end

    detector = pkgsrc_rc_detector("milter-manager")
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
    (@rc_d + "amavismilter").open("w") do |file|
      file << <<-EOM
name=amavismilter
rcvar=$name
command="/usr/pkg/sbin/amavis-milter"
required_vars="amavisd"
: ${amavismilter_user="amavis"}
: ${amavismilter_flags="-p local:/var/amavis/amavis-milter.sock"}
EOM
    end
    @rc_conf.open("w") do |file|
      file << "amavismilter=YES"
    end

    detector = pkgsrc_rc_detector("amavismilter")
    detector.detect
    detector.apply(@loader)
    assert_equal("amavismilter", detector.name)
    assert_eggs([["amavismilter",
                  nil,
                  true,
                  (@rc_d + "amavismilter").to_s,
                  "start",
                  "local:/var/amavis/amavis-milter.sock"]])
  end

  def test_apply_dkim_filter_style
    (@rc_d + "dkim-filter").open("w") do |file|
      file << <<-EOM
name="dkimfilter"
rcvar=$name
command="/usr/pkg/libexec/dkim-filter"
pidfile="/var/run/dkim-filter/${name}.pid"
command_args="-p local:/var/run/dkim-filter/${name}.sock -P ${pidfile} -l -x /usr/pkg/etc/dkim-filter.conf -u dkim:dkim"
required_files="/usr/pkg/etc/dkim-filter.conf"
start_precmd="dkimfilter_precmd"
EOM
    end
    @rc_conf.open("w") do |file|
      file << <<-EOC
dkimfilter=YES
EOC
    end

    detector = pkgsrc_rc_detector("dkim-filter")
    detector.detect
    detector.apply(@loader)
    assert_equal("dkimfilter", detector.name)
    assert_eggs([["dkim-filter",
                  nil,
                  true,
                  (@rc_d + "dkim-filter").to_s,
                  "start",
                  "local:/var/run/dkim-filter/dkimfilter.sock"]])
  end

  def test_apply_milter_greylist_style
    (@rc_d + "milter-greylist").open("w") do |file|
      file << <<-EOM
name="miltergreylist"
rcvar="miltergreylist"
command="/usr/pkg/bin/milter-greylist"
command_args="-p /var/milter-greylist/milter-greylist.sock -u smmsp"
EOM
    end

    detector = pkgsrc_rc_detector("milter-greylist")
    detector.detect
    detector.apply(@loader)
    assert_equal("miltergreylist", detector.name)
    assert_eggs([["milter-greylist",
                  nil,
                  false,
                  (@rc_d + "milter-greylist").to_s,
                  "start",
                  "unix:/var/milter-greylist/milter-greylist.sock"]])
  end

  def test_apply_spamass_milter_style
    (@rc_d + "spamass-milter").open("w") do |file|
      file << <<-EOM
name="spamass_milter"
rcvar=${name}
command="/usr/pkg/sbin/spamass-milter"
command_args="-u nobody -p /var/run/spamass.sock -f"

if [ -f /etc/rc.subr ]; then
	load_rc_config $name

	# hack: put ${spamass_milter_flags} last in args,
	# so that "-- ..." spamc flags can be added to the end
	#
	command_args="${command_args} ${spamass_milter_flags}"
	spamass_milter_flags=
else
	echo -n " ${name}"
	${command} ${command_args} ${spamass_milter_flags}
fi
EOM
    end

    detector = pkgsrc_rc_detector("spamass-milter")
    detector.detect
    detector.apply(@loader)
    assert_equal("spamass_milter", detector.name)
    assert_eggs([["spamass-milter",
                  nil,
                  false,
                  (@rc_d + "spamass-milter").to_s,
                  "start",
                  "unix:/var/run/spamass.sock"]])
  end

  def test_apply_dk_milter_style
    (@rc_d + "dk-milter").open("w") do |file|
      file << <<-EOM
dkmilter_flags="-h -l -p /var/run/dkmilter.sock"

if [ -f /etc/rc.subr ]; then
	. /etc/rc.subr
fi

name="dkmilter"
rcvar=$name
command="/usr/pkg/sbin/dk-milter"
pidfile="/var/run/${name}.pid"
command_args="-P ${pidfile}"
EOM
    end

    detector = pkgsrc_rc_detector("dk-milter")
    detector.detect
    detector.apply(@loader)
    assert_equal("dkmilter", detector.name)
    assert_eggs([["dk-milter",
                  nil,
                  false,
                  (@rc_d + "dk-milter").to_s,
                  "start",
                  "unix:/var/run/dkmilter.sock"]])
  end

  def test_apply_mimedefang_style
    (@rc_d + "mimedefang").open("w") do |file|
      file << <<-EOM
defangdir="/var/spool/mimedefang"

name="mimedefang"
rcvar=$name
command="/usr/pkg/bin/mimedefang"
pidfile="${defangdir}/${name}.pid"
mimedefang_user=${mimedefang_user-"mimedefang"}
command_args="-P ${pidfile}"

# default values, may be overridden on NetBSD by setting them in /etc/rc.conf
mimedefang_flags=${mimedefang_flags-"-p ${defangdir}/mimedefang.sock \
			-m ${defangdir}/mimedefang-multiplexor.sock"}

mimedefang=${mimedefang:-NO}
mimedefang_fdlimit=${mimedefang_fdlimit-"128"}
EOM
    end

    detector = pkgsrc_rc_detector("mimedefang")
    detector.detect
    detector.apply(@loader)
    assert_equal("mimedefang", detector.name)
    assert_eggs([["mimedefang",
                  nil,
                  false,
                  (@rc_d + "mimedefang").to_s,
                  "start",
                  "unix:/var/spool/mimedefang/mimedefang.sock"]])
  end

  def test_apply_milter_regex_style
    (@rc_d + "milter-regex").open("w") do |file|
      file << <<-EOM
name="milterregex"
rcvar="milterregex"
command="/usr/pkg/sbin/milter-regex"
command_args="-u smmsp"
EOM
    end

    detector = pkgsrc_rc_detector("milter-regex") do |_detector, guessed_spec|
      guessed_spec || "unix:/var/spool/milter-regex/sock"
    end
    detector.detect
    detector.apply(@loader)
    assert_equal("milterregex", detector.name)
    assert_eggs([["milter-regex",
                  nil,
                  false,
                  (@rc_d + "milter-regex").to_s,
                  "start",
                  "unix:/var/spool/milter-regex/sock"]])
  end

  def test_apply_enma_style
    enma_conf = @tmp_dir + "enma.conf"
    (@rc_d + "enma").open("w") do |file|
      file << <<-EOM
name="enma"
rcvar=${name}
command="@PREFIX@/bin/enma"
pidfile="/var/run/enma/${name}.pid"
required_files="#{enma_conf}"
command_args="-c #{enma_conf}"
EOM
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
milter.postfix: false
EOC
    end

    detector = pkgsrc_rc_detector("enma")
    detector.detect
    detector.apply(@loader)
    assert_equal("enma", detector.name)
    assert_eggs([["enma",
                  nil,
                  false,
                  (@rc_d + "enma").to_s,
                  "start",
                  "inet:10025@127.0.0.1"]])
  end

  private
  def pkgsrc_rc_detector(name, options={}, &spec_detector)
    detector = Milter::Manager::PkgsrcRCDetector.new(@configuration, name,
                                                     &spec_detector)

    _rc_d = @rc_d
    _rc_conf = @rc_conf
    _rc_conf_d = @rc_conf_d
    _candidate_service_commands = options[:candidate_service_commands] || []
    singleton_object = class << detector; self; end
    singleton_object.send(:define_method, :rc_d) do
      _rc_d.to_s
    end

    singleton_object.send(:define_method, :rc_conf) do
      _rc_conf.to_s
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
    <<EOS
. /etc/rc.subr

name="milter_manager"
rcvar=${name}
extra_commands="reload"

load_rc_config ${name}

: ${milter_manager_pid_file="/var/run/milter-manager/milter-manager.pid"}
if getent passwd milter-manager > /dev/null; then
    : ${milter_manager_user_name="milter-manager"}
else
    : ${milter_manager_user_name="smmsp"}
fi
: ${milter_manager_group_name="mail"}
: ${milter_manager_connection_spec=""}
: ${milter_manager_debug="NO"}

command="/usr/pkg/sbin/milter-manager"
pidfile="${milter_manager_pid_file}"
command_args="--pid-file ${milter_manager_pid_file}"
if test -n "${milter_manager_user_name}"; then
    command_args="${command_args} --user-name ${milter_manager_user_name}"
fi
if test -n "${milter_manager_group_name}"; then
    command_args="${command_args} --group-name ${milter_manager_group_name}"
fi
if test -n "$milter_manager_connection_connection_spec"; then
    command_args="${command_args} --spec ${milter_manager_connection_spec}"
fi
if test "$milter_manager_debug" = "YES"; then
    command_args="${command_args} --verbose"
else
    command_args="${command_args} --daemon"
fi

run_rc_command "$1"
EOS
  end
end
