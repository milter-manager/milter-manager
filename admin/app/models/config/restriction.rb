class Config::Restriction < ActiveRecord::Base
  belongs_to :milter, :class_name => "Config::Milter"
  belongs_to :applicable_condition, :class_name => "Config::ApplicableCondition"
end
