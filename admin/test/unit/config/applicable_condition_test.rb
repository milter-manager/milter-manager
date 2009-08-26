require 'test_helper'

class Config::ApplicableConditionTest < ActiveSupport::TestCase
  def test_apply
    s25r = applicable_conditions(:s25r)
    remote_network = applicable_conditions(:remote_network)
    conditions = [s25r, remote_network]
    assert_equal(conditions.sort_by(&:id),
                 Config::ApplicableCondition.find(:all).sort_by(&:id))

    new_condition_config = {
      "name" => "my condition",
      "description" => "my original condition",
    }
    change_condition_config = {
      "name" => s25r.name,
      "description" => s25r.description + ".",
    }
    same_condition_config = {
      "name" => remote_network.name,
      "description" => remote_network.description,
    }
    Config::ApplicableCondition.apply([new_condition_config,
                                       change_condition_config,
                                       same_condition_config])
    changed_s25r = Config::ApplicableCondition.new(change_condition_config)
    my_condition = Config::ApplicableCondition.new(new_condition_config)
    assert_equal_conditions([changed_s25r, remote_network, my_condition],
                            Config::ApplicableCondition.find(:all))
  end

  def test_relation
    s25r = applicable_conditions(:s25r)

    virus_scanner = milters(:virus_scanner)
    greylist = milters(:greylist)
    assert_equal([virus_scanner, greylist].sort_by(&:id),
                 s25r.milters.sort_by(&:id))

    anti_spam = milters(:anti_spam)
    s25r.milters.delete(greylist)
    s25r = Config::ApplicableCondition.find(s25r)
    assert_equal([virus_scanner], s25r.milters)

    assert_difference("Config::Restriction.count", -1) do
      s25r.destroy
    end
  end

  private
  def assert_equal_conditions(expected, actual)
    assert_equal(normalize_conditions(expected),
                 normalize_conditions(actual))
  end

  def normalize_conditions(conditions)
    conditions.collect do |condition|
      {
        :name => condition.name,
        :description => condition.description
      }
    end.sort_by do |condition|
      condition[:name]
    end
  end
end
