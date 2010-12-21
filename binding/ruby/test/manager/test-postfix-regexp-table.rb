# Copyright (C) 2010  Kouhei Sutou <kou@clear-code.com>
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

class TestPostfixRegexpTable < Test::Unit::TestCase

  include MilterParseTestUtils

  def setup
    @table = Milter::Manager::PostfixRegexpTable.new
  end

  def test_empty
    assert_nil(@table.find("sender@example.com"))
  end

  def test_positive_pattern
    @table.parse(create_input(<<-EOC))
/[%!@].*[%!@]/       550 Sender-specified routing rejected
/^postmaster@/       OK
EOC
    assert_equal("OK", @table.find("postmaster@example.com"))
  end

  def test_negative_pattern
    @table.parse(create_input(<<-EOC))
!/^owner-/          OK
/[%!@].*[%!@]/      550 Sender-specified routing rejected
EOC
    assert_equal("OK", @table.find("%xxx%"))
  end

  def test_expand
    @table.parse(create_input(<<-EOC))
/^(.*)-outgoing\\+(.*)@(.*)$/   550 Use ${1}+$(2)@$3 $$ instead
EOC
    assert_equal("550 Use user+ml@example.com $ instead",
                 @table.find("user-outgoing+ml@example.com"))
  end

  def test_if_positive_pattern
    @table.parse(create_input(<<-EOC))
if /^owner/
/@(.*)$/   OK
endif
EOC
    assert_equal("OK", @table.find("owner@example.com"))
    assert_nil(@table.find("user@example.com"))
  end

  def test_else_negative_pattern
    @table.parse(create_input(<<-EOC))
if !/^owner-/
/^(.*)-outgoing@(.*)$/   550 Use ${1}@${2} instead
endif
EOC
    assert_equal("550 Use user@example.com instead",
                 @table.find("user-outgoing@example.com"))
    assert_nil(@table.find("owner-outgoing@example.com"))
  end

  def test_invalid_pattern
    message = nil
    begin
      Regexp.new("left-(paren only", Regexp::IGNORECASE)
    rescue
      message = $!.message
    end
    error = invalid_value_error("left-(paren only",
                                message,
                                "/left-(paren only/ REJECT",
                                nil,
                                1)
    assert_raise(error) do
      @table.parse(create_input(<<-EOC))
/left-(paren only/ REJECT
/left-paren/ OK
EOC
    end
    assert_nil(@table.find("left-paren"))
  end

  def test_invalid_format
    error = invalid_format_error("left-paren OK",
                                 nil,
                                 1)
    assert_raise(error) do
      @table.parse(create_input(<<-EOC))
left-paren OK
EOC
    end
    assert_nil(@table.find("left-paren"))
  end

  private
  def invalid_value_error(value, detail, line, path, line_no)
    Milter::Manager::ConditionTable::InvalidValueError.new(value,
                                                           detail,
                                                           line,
                                                           path,
                                                           line_no)
  end

  def invalid_format_error(line, path, line_no)
    Milter::Manager::ConditionTable::InvalidFormatError.new(line, path, line_no)
  end
end
