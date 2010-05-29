# Copyright (C) 2008-2009  Kouhei Sutou <kou@clear-code.com>
#
# This library is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this library.  If not, see <http://www.gnu.org/licenses/>.

class TestOption < Test::Unit::TestCase
  include MilterTestUtils

  def test_action_flags
    assert_const_defined(Milter, :ActionFlags)
    assert_const_defined(Milter, :ACTION_NONE)
  end

  def test_add_action
    option = Milter::Option.new
    assert_equal(Milter::ACTION_NONE, option.action)
    
    option.add_action(Milter::ACTION_ADD_HEADERS)
    assert_equal(Milter::ACTION_ADD_HEADERS, option.action)
  end

  def test_step_flags
    assert_const_defined(Milter, :StepFlags)
    assert_const_defined(Milter, :STEP_NONE)
  end

  def test_step_flags_merge
    flags1 =
      Milter::StepFlags::SKIP |
      Milter::StepFlags::NO_CONNECT |
      Milter::StepFlags::NO_HELO |
      Milter::StepFlags::NO_DATA |
      Milter::StepFlags::NO_REPLY_DATA |
      Milter::StepFlags::NO_REPLY_END_OF_HEADER
    flags2 =
      Milter::StepFlags::HEADER_VALUE_WITH_LEADING_SPACE |
      Milter::StepFlags::NO_CONNECT |
      Milter::StepFlags::NO_HELO |
      Milter::StepFlags::NO_ENVELOPE_FROM |
      Milter::StepFlags::NO_ENVELOPE_RECIPIENT |
      Milter::StepFlags::NO_REPLY_DATA |
      Milter::StepFlags::NO_REPLY_UNKNOWN
    assert_equal(Milter::StepFlags::SKIP |
                 Milter::StepFlags::HEADER_VALUE_WITH_LEADING_SPACE |
                 Milter::StepFlags::NO_CONNECT |
                 Milter::StepFlags::NO_HELO |
                 Milter::StepFlags::NO_REPLY_DATA,
                 flags1.merge(flags2))
  end

  def test_step_no_event_mask
    assert_equal(Milter::StepFlags::NO_CONNECT |
                 Milter::StepFlags::NO_HELO |
                 Milter::StepFlags::NO_ENVELOPE_FROM |
                 Milter::StepFlags::NO_ENVELOPE_RECIPIENT |
                 Milter::StepFlags::NO_BODY |
                 Milter::StepFlags::NO_HEADERS |
                 Milter::StepFlags::NO_END_OF_HEADER |
                 Milter::StepFlags::NO_UNKNOWN |
                 Milter::StepFlags::NO_DATA,
                 Milter::StepFlags::NO_EVENT_MASK)
  end

  def test_step_no_reply_mask
    assert_equal(Milter::StepFlags::NO_REPLY_CONNECT |
                 Milter::StepFlags::NO_REPLY_HELO |
                 Milter::StepFlags::NO_REPLY_ENVELOPE_FROM |
                 Milter::StepFlags::NO_REPLY_ENVELOPE_RECIPIENT |
                 Milter::StepFlags::NO_REPLY_DATA |
                 Milter::StepFlags::NO_REPLY_UNKNOWN |
                 Milter::StepFlags::NO_REPLY_HEADER |
                 Milter::StepFlags::NO_REPLY_END_OF_HEADER |
                 Milter::StepFlags::NO_REPLY_BODY,
                 Milter::StepFlags::NO_REPLY_MASK)
  end

  def test_step_no_mask
    assert_equal(Milter::StepFlags::NO_EVENT_MASK |
                 Milter::StepFlags::NO_REPLY_MASK,
                 Milter::StepFlags::NO_MASK)
  end

  def test_step_yes_mask
    assert_equal(Milter::StepFlags::SKIP |
                 Milter::StepFlags::ENVELOPE_RECIPIENT_REJECTED |
                 Milter::StepFlags::HEADER_VALUE_WITH_LEADING_SPACE,
                 Milter::StepFlags::YES_MASK)
  end

  def test_remove_step
    option = Milter::Option.new
    option.step = Milter::StepFlags::NO_EVENT_MASK
    option.remove_step(Milter::StepFlags::NO_HELO |
                       Milter::StepFlags::NO_ENVELOPE_FROM |
                       Milter::StepFlags::NO_ENVELOPE_RECIPIENT)
    assert_equal(Milter::StepFlags::NO_CONNECT |
                 Milter::StepFlags::NO_BODY |
                 Milter::StepFlags::NO_HEADERS |
                 Milter::StepFlags::NO_END_OF_HEADER |
                 Milter::StepFlags::NO_UNKNOWN |
                 Milter::StepFlags::NO_DATA,
                 option.step)
  end
end
