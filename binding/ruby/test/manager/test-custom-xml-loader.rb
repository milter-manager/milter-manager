class TestCustomXMLLoader < Test::Unit::TestCase
  include MilterTestUtils

  def setup
    @configuration = Object.new
    mock(@configuration).signal_connect("to-xml")
    @loader = Milter::Manager::ConfigurationLoader.new(@configuration)
  end

  def test_security
    mock(@configuration).privilege_mode=(true)
    load(<<-EOC)
<configuration>
  <security>
    <privilege-mode>true</privilege-mode>
  </security>
</configuration>
EOC
  end

  private
  def load(content)
    file = Tempfile.new("xml-loader")
    file.open
    file.puts(content)
    file.close
    @loader.load_custom_configuration(file.path)
  end
end
