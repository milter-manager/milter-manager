class TestOption < Test::Unit::TestCase
  include MilterTestUtils

  def test_action_flags
    assert_const_defined(Milter, :ActionFlags)
    assert_const_defined(Milter, :ACTION_NONE)
  end

  def test_step_flags
    assert_const_defined(Milter, :StepFlags)
    assert_const_defined(Milter, :STEP_NONE)
  end
end
