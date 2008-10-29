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
end
