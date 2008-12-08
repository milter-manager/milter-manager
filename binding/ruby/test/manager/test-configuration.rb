class TestConfiguration < Test::Unit::TestCase
  def setup
    @configuration = Milter::Manager::Configuration.new
  end

  def test_add_egg
    assert_equal(0, @configuration.create_children.length)

    name = "child-milter"
    egg = Milter::Manager::Egg.new(name)
    egg.connection_spec = "unix:/tmp/socket"
    @configuration.add_egg(egg)
    assert_equal(1, @configuration.create_children.length)
  end

  def test_to_xml
    assert_equal(["<configuration>",
                  "</configuration>"].join("\n") + "\n",
                 @configuration.to_xml)

    name = "child-milter"
    spec = "unix:/tmp/socket"
    egg = Milter::Manager::Egg.new(name)
    egg.connection_spec = spec
    @configuration.add_egg(egg)
    assert_equal(["<configuration>",
                  "  <milters>",
                  "    <milter>",
                  "      <name>#{name}</name>",
                  "      <connection-spec>#{spec}</connection-spec>",
                  "    </milter>",
                  "  </milters>",
                  "</configuration>"].join("\n") + "\n",
                 @configuration.to_xml)

    @configuration.signal_connect("to-xml") do |_, xml, indent|
      xml << " " * indent
      xml << "<additional-info>INFO</additional-info>\n"
    end
    assert_equal(["<configuration>",
                  "  <milters>",
                  "    <milter>",
                  "      <name>#{name}</name>",
                  "      <connection-spec>#{spec}</connection-spec>",
                  "    </milter>",
                  "  </milters>",
                  "  <additional-info>INFO</additional-info>",
                  "</configuration>"].join("\n") + "\n",
                 @configuration.to_xml)
  end
end
