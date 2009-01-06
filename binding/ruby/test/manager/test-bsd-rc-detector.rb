# Copyright (C) 2008  Kouhei Sutou <kou@cozmixng.org>
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

class TestBSDRCDetector < Test::Unit::TestCase
  include MilterTestUtils

  def setup
    @configuration = Milter::Manager::Configuration.new
    @loader = Milter::Manager::ConfigurationLoader.new(@configuration)
    @tmp_dir = Pathname(File.dirname(__FILE__)) + ".." + "tmp"
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
    detector = bsd_rc_detector("milter-manager")
    assert_false(detector.rc_script_exist?)
    (@rc_d + "milter-manager").open("w") {}
    assert_true(detector.rc_script_exist?)
  end

  def test_name
    (@rc_d + "milter-manager").open("w") do |file|
      file << milter_manager_rc_script
    end
    detector = bsd_rc_detector("milter-manager")
    detector.detect
    assert_equal("milter_manager", detector.name)
  end

  def test_variables
    (@rc_d + "milter-manager").open("w") do |file|
      file << milter_manager_rc_script
    end
    detector = bsd_rc_detector("milter-manager")
    detector.detect
    assert_equal({
                   "enable" => "NO",
                   "pid_file" => "/var/run/milter-manager/milter-manager.pid",
                   "user_name" => "mailnull",
                   "group_name" => "mail",
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

    detector = bsd_rc_detector("milter-manager")
    detector.detect
    assert_equal({
                   "enable" => "YES",
                   "pid_file" => "/var/run/milter-manager/milter-manager.pid",
                   "user_name" => "mailnull",
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
milter_manager_enable="YES"
milter_manager_connection_spec=inet:10025@localhost
EOC
    end

    detector = bsd_rc_detector("milter-manager")
    detector.detect
    detector.apply(@loader)
    assert_eggs([["milter_manager",
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

    detector = bsd_rc_detector("amavis-milter")
    detector.detect
    detector.apply(@loader)
    assert_equal("amavis_milter", detector.name)
    assert_eggs([["amavis-milter",
                  true,
                  (@rc_d + "amavis-milter").to_s,
                  "start",
                  "unix:/var/amavis/amavis-milter.sock"]])
  end

  def test_apply_clamav_milter_style
    (@rc_d + "clamav-milter").open("w") do |file|
      file << <<-EOM
name=clamav_milter
: ${clamav_milter_enable="NO"}
: ${clamav_milter_socket="/var/run/clamav/clmilter.sock"}
EOM
    end

    detector = bsd_rc_detector("clamav-milter")
    detector.detect
    detector.apply(@loader)
    assert_equal("clamav_milter", detector.name)
    assert_eggs([["clamav-milter",
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

    detector = bsd_rc_detector("milter-dkim")
    detector.detect
    detector.apply(@loader)
    assert_equal("milterdkim", detector.name)
    assert_eggs([["milter-dkim",
                  true,
                  (@rc_d + "milter-dkim").to_s,
                  "start",
                  "unix:/var/run/milter/milter-dkim.sock"]])
  end

  def test_apply_milter_greylist_style
    (@rc_d + "milter-greylist").open("w") do |file|
      file << <<-EOM
name="miltergreylist"

miltergreylist_enable=${miltergreylist_enable-"NO"}
miltergreylist_sockfile=${miltergreylist_sockfile-"/var/milter-greylist/milter-greylist.sock"}
EOM
    end

    detector = bsd_rc_detector("milter-greylist")
    detector.detect
    detector.apply(@loader)
    assert_equal("miltergreylist", detector.name)
    assert_eggs([["milter-greylist",
                  false,
                  (@rc_d + "milter-greylist").to_s,
                  "start",
                  "unix:/var/milter-greylist/milter-greylist.sock"]])
  end

  private
  def bsd_rc_detector(name)
    detector = Milter::Manager::ConfigurationLoader::BSDRCDetector.new(name)

    _rc_d = @rc_d
    _rc_conf = @rc_conf
    _rc_conf_d = @rc_conf_d
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

    detector
  end

  def milter_manager_rc_script
    top = Pathname(File.dirname(__FILE__)) + ".." + ".." + ".." + ".."
    (top + "data" + "rc.d" + "milter-manager").read
  end

  def assert_eggs(expected_eggs)
    egg_info = @configuration.eggs.collect do |egg|
      [egg.name,
       egg.enabled?,
       egg.command,
       egg.command_options,
       egg.connection_spec]
    end
    assert_equal(expected_eggs, egg_info)
  end
end
