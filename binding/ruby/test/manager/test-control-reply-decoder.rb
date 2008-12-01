class TestControlReplyDecoder < Test::Unit::TestCase
  include MilterTestUtils

  def setup
    @encoder = Milter::Manager::ControlReplyEncoder.new
    @decoder = Milter::Manager::ControlReplyDecoder.new
  end

  def test_success
    success_emitted = false
    @decoder.signal_connect("success") do
      success_emitted = true
    end

    @decoder.decode(@encoder.encode_success)
    @decoder.end_decode
    assert_true(success_emitted)
  end

  def test_failure
    decoded_failure_message = nil
    @decoder.signal_connect("failure") do |_, message|
      decoded_failure_message = message
    end

    failure_message =  "Failure!"
    @decoder.decode(@encoder.encode_failure(failure_message))
    @decoder.end_decode
    assert_equal(failure_message, decoded_failure_message)
  end

  def test_error
    decoded_error_message = nil
    @decoder.signal_connect("error") do |_, message|
      decoded_error_message = message
    end

    error_message =  "Error!"
    @decoder.decode(@encoder.encode_error(error_message))
    @decoder.end_decode
    assert_equal(error_message, decoded_error_message)
  end

  def test_configuration
    decoded_configuration = nil
    @decoder.signal_connect("configuration") do |_, configuration|
      decoded_configuration = configuration
    end

    configuration = ["security.privilege_mode = true", "# comment"].join("\n")
    @decoder.decode(@encoder.encode_configuration(configuration))
    @decoder.end_decode
    assert_equal(configuration, decoded_configuration)
  end
end
