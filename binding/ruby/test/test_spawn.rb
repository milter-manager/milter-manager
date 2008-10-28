class TestSpawn < Test::Unit::TestCase
  def setup
    @name = "child-milter"
    @spawn = Milter::Manager::Spawn.new(@name)
  end

  def test_new
    name = "child-milter"
    spawn = Milter::Manager::Spawn.new(name)
    assert_equal(name, spawn.name)
  end

  def test_name
    new_name = "#{@name}-new"
    assert_not_equal(new_name, @spawn.name)
    @spawn.name = new_name
    assert_equal(new_name, @spawn.name)
  end

  def test_connection_timeout
    connection_timeout = 29
    assert_equal(0, @spawn.connection_timeout)
    @spawn.connection_timeout = connection_timeout
    assert_equal(connection_timeout, @spawn.connection_timeout)
  end

  def test_writing_timeout
    writing_timeout = 29
    assert_equal(0, @spawn.writing_timeout)
    @spawn.writing_timeout = writing_timeout
    assert_equal(writing_timeout, @spawn.writing_timeout)
  end

  def test_reading_timeout
    reading_timeout = 29
    assert_equal(0, @spawn.reading_timeout)
    @spawn.reading_timeout = reading_timeout
    assert_equal(reading_timeout, @spawn.reading_timeout)
  end

  def test_end_of_message_timeout
    end_of_message_timeout = 29
    assert_equal(0, @spawn.end_of_message_timeout)
    @spawn.end_of_message_timeout = end_of_message_timeout
    assert_equal(end_of_message_timeout, @spawn.end_of_message_timeout)
  end

  def test_user_name
    user_name = "milter-user"
    assert_nil(@spawn.user_name)
    @spawn.user_name = user_name
    assert_equal(user_name, @spawn.user_name)
  end

  def test_command
    command = "/usr/bin/milter-test-client"
    assert_nil(@spawn.command)
    @spawn.command = command
    assert_equal(command, @spawn.command)
  end
end
