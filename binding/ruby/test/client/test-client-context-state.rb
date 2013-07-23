# Copyright (C) 2013  Kouhei Sutou <kou@clear-code.com>
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

class TestClientContextState < Test::Unit::TestCase
  include MilterTestUtils

  class TestPredicate < self
    def test_helo
      state = Milter::ClientContextState::HELO
      assert_true(state.helo?)
    end

    def test_negotiate
      state = Milter::ClientContextState::NEGOTIATE
      assert_true(state.negotiate?)
    end

    def test_connect
      state = Milter::ClientContextState::CONNECT
      assert_true(state.connect?)
    end

    def test_envelope_from
      state = Milter::ClientContextState::ENVELOPE_FROM
      assert_true(state.envelope_from?)
    end

    def test_envelope_recipient
      state = Milter::ClientContextState::ENVELOPE_RECIPIENT
      assert_true(state.envelope_recipient?)
    end

    def test_data
      state = Milter::ClientContextState::DATA
      assert_true(state.data?)
    end

    def test_unknown
      state = Milter::ClientContextState::UNKNOWN
      assert_true(state.unknown?)
    end

    def test_header
      state = Milter::ClientContextState::HEADER
      assert_true(state.header?)
    end

    def test_end_of_header
      state = Milter::ClientContextState::END_OF_HEADER
      assert_true(state.end_of_header?)
    end

    def test_body
      state = Milter::ClientContextState::BODY
      assert_true(state.body?)
    end

    def test_end_of_message
      state = Milter::ClientContextState::END_OF_MESSAGE
      assert_true(state.end_of_message?)
    end

    def test_quit
      state = Milter::ClientContextState::QUIT
      assert_true(state.quit?)
    end

    def test_abort
      state = Milter::ClientContextState::ABORT
      assert_true(state.abort?)
    end
  end
end
