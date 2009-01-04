# Copyright (C) 2008  Kouhei Sutou <kou@cozmixng.org>
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

class TestApplicableCondition < Test::Unit::TestCase
  def setup
    @name = "S25R"
    @condition = Milter::Manager::ApplicableCondition.new(@name)
  end

  def test_new
    name = "remote-network"
    condition = Milter::Manager::ApplicableCondition.new(name)
    assert_equal(name, condition.name)
    assert_nil(condition.description)
  end

  def test_name
    new_name = "#{@name}-new"
    assert_not_equal(new_name, @condition.name)
    @condition.name = new_name
    assert_equal(new_name, @condition.name)
  end

  def test_description
    description = "Selective SMTP Rejection"
    @condition.description = description
    assert_equal(description, @condition.description)
  end

  def test_merge
    merged_condition = Milter::Manager::ApplicableCondition.new("merged")
    @condition.description = "Selective SMTP Rejection"

    assert_nil(merged_condition.description)
    merged_condition.merge(@condition)
    assert_equal("Selective SMTP Rejection", merged_condition.description)
  end
end
