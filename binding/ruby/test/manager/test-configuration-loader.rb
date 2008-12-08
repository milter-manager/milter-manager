class TestConfigurationLoader < Test::Unit::TestCase
  def setup
    @configuration = Milter::Manager::Configuration.new
    @loader = Milter::Manager::ConfigurationLoader.new(@configuration)
  end

  def test_to_xml
    assert_equal(<<-EOX, @configuration.to_xml)
<configuration>
</configuration>
EOX

    @loader.define_milter("child-milter") do |milter|
      milter.connection_spec = "unix:/tmp/socket"
      milter.target_hosts << "external.com"
      milter.target_senders << /@external1.example.com\z/
      milter.target_recipients << /@external2.example.com\z/
      milter.add_target_header("X-External", /\A(yes|true)\z/i)
    end
    assert_equal(<<-EOX, @configuration.to_xml)
<configuration>
  <milters>
    <milter>
      <name>child-milter</name>
      <connection-spec>unix:/tmp/socket</connection-spec>
      <target_host>--- external.com
</target_host>
      <target_sender>--- !ruby/regexp /@external1.example.com\\z/
</target_sender>
      <target_recipient>--- !ruby/regexp /@external2.example.com\\z/
</target_recipient>
      <target_header>
        <name>--- X-External
</name>
        <value>--- !ruby/regexp /\\A(yes|true)\\z/i
</value>
      </target_header>
    </milter>
  </milters>
</configuration>
EOX
  end
end
