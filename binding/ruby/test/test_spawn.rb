class TestSpawn < Test::Unit::TestCase
  def test_new
    spawn = Milter::Manager::Spawn.new("child-milter")
    assert_equal("child-milter", spawn.name)
  end
end
