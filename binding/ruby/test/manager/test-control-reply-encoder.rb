class TestControlReplyEncoder < Test::Unit::TestCase
  include MilterManagerEncoderTestUtils

  def setup
    @encoder = Milter::Manager::ControlReplyEncoder.new
  end

  def test_success
    assert_equal(packet("success"), @encoder.encode_success)
  end

  def test_failure
    message = "Failure!"
    assert_equal(packet("failure", message),
                 @encoder.encode_failure(message))
  end

  def test_error
    message = "Error!"
    assert_equal(packet("error", message),
                 @encoder.encode_error(message))
  end

  def test_configuration
    configuration = ["security.privilege_mode = true", "# comment"].join("\n")
    assert_equal(packet("configuration", configuration),
                 @encoder.encode_configuration(configuration))
  end
end
