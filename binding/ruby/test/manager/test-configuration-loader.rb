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

    @loader.define_applicable_condition("remote-network") do |condition|
      condition.description = "Check only remote network"

      condition.define_connect_checker do |child, host, address|
        host == "localhost"
      end

      condition.define_envelope_from_checker do |child, from|
        from =~ /@my-domain1.example.com\z/
      end

      condition.define_envelope_recipient_checker do |child, recipient|
        recipient =~ /@my-domain2.example.com\z/
      end

      condition.define_header_checker do |child, name, value|
        name == "X-Internal" and value =~ /\A(yes|true)\z/i
      end
    end

    @loader.define_milter("child-milter") do |milter|
      milter.connection_spec = "unix:/tmp/socket"
      milter.add_applicable_condition("remote-network")
    end

    assert_equal(<<-EOX, @configuration.to_xml)
<configuration>
  <milters>
    <milter>
      <name>child-milter</name>
      <connection-spec>unix:/tmp/socket</connection-spec>
      <applicable-conditions>
        <applicable-condition>remote-network</applicable-condition>
      </applicable-conditions>
    </milter>
  </milters>
  <applicable-conditions>
    <applicable-condition>
      <name>remote-network</name>
      <description>Check only remote network</description>
    </applicable-condition>
  </applicable-conditions>
</configuration>
EOX
  end


  def test_applicable_conditions
    @loader.define_applicable_condition("S25R") do |condition|
      condition.description = "Selective SMTP Rejection."
      condition.define_connect_checker do |child, host, address|
        case host
        when "unknown",
          /\A[^.]*\d[^\d.]+\d/,
          /\A[^.]*\d{5}/,
          /\A(?:[^.]+\.)?\d[^.]*\.[^.]+\..+\.[a-z]/i,
          /\A[^.]*\d\.[^.]*\d-\d/,
          /\A[^.]*\d\.[^.]*\d\.[^.]+\..+\./,
          /\A(?:dhcp|dialup|ppp|[achrsvx]?dsl)[^.]*\d/i
          false
        else
          true
        end
      end
    end

    assert_equal(["<configuration>",
                  "  <applicable-conditions>",
                  "    <applicable-condition>",
                  "      <name>S25R</name>",
                  "      <description>Selective SMTP Rejection.</description>",
                  "    </applicable-condition>",
                  "  </applicable-conditions>",
                  "</configuration>"].join("\n") + "\n",
                 @configuration.to_xml)
  end
end
