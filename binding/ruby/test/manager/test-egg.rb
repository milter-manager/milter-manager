class TestEgg < Test::Unit::TestCase
  def setup
    @name = "child-milter"
    @egg = Milter::Manager::Egg.new(@name)
  end

  def test_new
    name = "child-milter"
    egg = Milter::Manager::Egg.new(name)
    assert_equal(name, egg.name)
  end

  def test_name
    new_name = "#{@name}-new"
    assert_not_equal(new_name, @egg.name)
    @egg.name = new_name
    assert_equal(new_name, @egg.name)
  end

  def test_connection_timeout
    connection_timeout = 29
    assert_equal(300, @egg.connection_timeout)
    @egg.connection_timeout = connection_timeout
    assert_equal(connection_timeout, @egg.connection_timeout)
  end

  def test_writing_timeout
    writing_timeout = 29
    assert_equal(10, @egg.writing_timeout)
    @egg.writing_timeout = writing_timeout
    assert_equal(writing_timeout, @egg.writing_timeout)
  end

  def test_reading_timeout
    reading_timeout = 29
    assert_equal(10, @egg.reading_timeout)
    @egg.reading_timeout = reading_timeout
    assert_equal(reading_timeout, @egg.reading_timeout)
  end

  def test_end_of_message_timeout
    end_of_message_timeout = 29
    assert_equal(200, @egg.end_of_message_timeout)
    @egg.end_of_message_timeout = end_of_message_timeout
    assert_equal(end_of_message_timeout, @egg.end_of_message_timeout)
  end

  def test_user_name
    user_name = "milter-user"
    assert_nil(@egg.user_name)
    @egg.user_name = user_name
    assert_equal(user_name, @egg.user_name)
  end

  def test_command
    command = "/usr/bin/milter-test-client"
    assert_nil(@egg.command)
    @egg.command = command
    assert_equal(command, @egg.command)
  end

  priority :must
  def test_to_xml
    assert_equal(["<milter>",
                  "  <name>#{@name}</name>",
                  "</milter>"].join("\n") + "\n",
                 @egg.to_xml)

    @egg.signal_connect("to-xml") do |_, xml, indent|
      xml << " " * indent
      xml << "<additional-info>INFO</additional-info>\n"
    end
    assert_equal(["<milter>",
                  "  <name>#{@name}</name>",
                  "  <additional-info>INFO</additional-info>",
                  "</milter>"].join("\n") + "\n",
                 @egg.to_xml)
  end
end
