class TestCustomXMLLoader < Test::Unit::TestCase
  include MilterTestUtils

  def setup
    @configuration = Milter::Manager::Configuration.new
    @loader = Milter::Manager::ConfigurationLoader.new(@configuration)
  end

  def test_security
    assert_false(@configuration.privilege_mode?)
    load(<<-EOC)
<configuration>
  <security>
    <privilege-mode>true</privilege-mode>
  </security>
</configuration>
EOC
    assert_true(@configuration.privilege_mode?)
  end

  def test_milter
    @loader.define_applicable_condition("S25R") do |condition|
      condition.description = "Selective SMTP Rejection"
    end

    load(<<-EOC)
<configuration>
  <milters>
    <milter>
      <name>milter@10026</name>
      <connection-spec>inet:10026@localhost</connection-spec>
      <applicable-conditions>
        <applicable-condition>S25R</applicable-condition>
      </applicable-conditions>
    </milter>
  </milters>
</configuration>
EOC

    assert_equal(<<-EOX, @configuration.to_xml)
<configuration>
  <milters>
    <milter>
      <name>milter@10026</name>
      <connection-spec>inet:10026@localhost</connection-spec>
      <applicable-conditions>
        <applicable-condition>S25R</applicable-condition>
      </applicable-conditions>
    </milter>
  </milters>
  <applicable-conditions>
    <applicable-condition>
      <name>S25R</name>
      <description>Selective SMTP Rejection</description>
    </applicable-condition>
  </applicable-conditions>
</configuration>
EOX
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
