class TestControlCommandDecoder < Test::Unit::TestCase
  include MilterTestUtils

  def setup
    @encoder = Milter::Manager::ControlCommandEncoder.new
    @decoder = Milter::Manager::ControlCommandDecoder.new
  end

  def test_set_configuration
    decoded_configuration = nil
    @decoder.signal_connect("set-configuration") do |_, configuration|
      decoded_configuration = configuration
    end

    configuration = ["security.privilege_mode = true", "# comment"].join("\n")
    @decoder.decode(@encoder.encode_set_configuration(configuration))
    @decoder.end_decode
    assert_equal(configuration, decoded_configuration)
  end
end
