class TestClientContext < Test::Unit::TestCase
  include MilterTestUtils

  def setup
    @context = Milter::ClientContext.new
  end

  def test_feed
    received_option = nil
    @context.signal_connect("negotiate") do |_, option|
      received_option = option
      Milter::STATUS_CONTINUE
    end
    encoder = Milter::CommandEncoder.new
    option = Milter::Option.new
    @context.feed(encoder.encode_negotiate(option))
    assert_equal(option, received_option)
  end
end
