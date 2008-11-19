class TestControlCommandEncoder < Test::Unit::TestCase
  include MilterManagerEncoderTestUtils

  def setup
    @encoder = Milter::Manager::ControlCommandEncoder.new
  end

  def test_set_configuration
    configuration = "XXX"
    assert_equal(packet("S", configuration),
                 @encoder.encode_set_configuration(configuration))
  end
end
