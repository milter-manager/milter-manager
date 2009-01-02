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
    @rc_conf.mkpath
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
    assert_equal("milter_manager", detector.name)
  end

  def test_variables
    (@rc_d + "milter-manager").open("w") do |file|
      file << milter_manager_rc_script
    end
    detector = bsd_rc_detector("milter-manager")
    assert_equal({
                   "enable" => "NO",
                   "pid_file" => "/var/run/milter-manager/milter-manager.pid",
                   "user_name" => "mailnull",
                   "group_name" => "mail",
                   "connection_spec" => "",
                 },
                 detector.variables)
  end

  private
  def bsd_rc_detector(name)
    Milter::Manager::ConfigurationLoader::BSDRCDetector.new(name) do |detector|
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
  end

  def milter_manager_rc_script
    top = Pathname(File.dirname(__FILE__)) + ".." + ".." + ".." + ".."
    (top + "data" + "rc.d" + "milter-manager").read
  end
end
