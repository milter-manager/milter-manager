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

    @configuration.clear_eggs
    assert_equal([], @configuration.eggs)
    assert_equal(0, @configuration.create_children.length)
  end

  def test_find_egg
    egg1 = Milter::Manager::Egg.new("milter1")
    egg2 = Milter::Manager::Egg.new("milter2")
    @configuration.add_egg(egg1)
    @configuration.add_egg(egg2)

    assert_equal(egg1, @configuration.find_egg("milter1"))
    assert_nil(@configuration.find_egg("nonexistent"))
  end

  def test_remove_egg
    egg1 = Milter::Manager::Egg.new("milter1")
    egg2 = Milter::Manager::Egg.new("milter2")
    @configuration.add_egg(egg1)
    @configuration.add_egg(egg2)

    assert_equal([egg1, egg2], @configuration.eggs)
    @configuration.remove_egg("milter1")
    assert_equal([egg2], @configuration.eggs)
    @configuration.remove_egg(egg2)
    assert_equal([], @configuration.eggs)
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

  def test_find_applicable_condition
    s25r = Milter::Manager::ApplicableCondition.new("S25R")
    remote_network = Milter::Manager::ApplicableCondition.new("Remote Network")
    @configuration.add_applicable_condition(s25r)
    @configuration.add_applicable_condition(remote_network)

    assert_equal(s25r, @configuration.find_applicable_condition("S25R"))
    assert_nil(@configuration.find_applicable_condition("nonexistent"))
  end

  def test_remove_applicable_condition
    s25r = Milter::Manager::ApplicableCondition.new("S25R")
    remote_network = Milter::Manager::ApplicableCondition.new("Remote Network")
    @configuration.add_applicable_condition(s25r)
    @configuration.add_applicable_condition(remote_network)

    assert_equal([s25r, remote_network], @configuration.applicable_conditions)
    @configuration.remove_applicable_condition("S25R")
    assert_equal([remote_network], @configuration.applicable_conditions)
    @configuration.remove_applicable_condition(remote_network)
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
                  "      <enabled>true</enabled>",
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
                  "      <enabled>true</enabled>",
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
