class TestMailTransactionContext < Test::Unit::TestCase
  include MilterTestUtils

  def setup
    @context = Milter::ClientContext.new
    @context.event_loop = Milter::GLibEventLoop.new
    @shelf = Milter::Client::MailTransactionShelf.new(@context)
  end

  def test_set_value
    assert_nil(@shelf["key1"])
    @shelf["key1"] = "value1"
    assert_equal("value1", @shelf["key1"])
  end

  def test_each
    @shelf["key1"] = "value1"
    @shelf["key2"] = "value2"
    @shelf["key3"] = "value3"
    keys = []
    values = []
    @shelf.each do |k, v|
      keys << k
      values << v
    end
    assert_equal(["key1", "key2", "key3"], keys.sort)
    assert_equal(["value1", "value2", "value3"], values.sort)
  end
end
