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

  def test_applicable_condition
    assert_equal([], @configuration.applicable_conditions)

    name = "S25R"
    condition = Milter::Manager::ApplicableCondition.new(name)
    @configuration.add_applicable_condition(condition)
    assert_equal([name],
                 @configuration.applicable_conditions.collect {|cond| cond.name})

    @configuration.clear_applicable_conditions
    assert_equal([], @configuration.applicable_conditions)
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

  def test_load_paths
    @configuration.clear_load_paths
    assert_equal([], @configuration.load_paths)

    @configuration.append_load_path("XXX")
    assert_equal(["XXX"], @configuration.load_paths)

    @configuration.prepend_load_path("prepend")
    assert_equal(["prepend", "XXX"], @configuration.load_paths)

    @configuration.append_load_path("append")
    assert_equal(["prepend", "XXX", "append"], @configuration.load_paths)

    @configuration.clear_load_paths
    assert_equal([], @configuration.load_paths)
  end
end
