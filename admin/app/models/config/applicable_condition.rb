class Config::ApplicableCondition < ActiveRecord::Base
  has_many :restrictions,
           :class_name => "Config::Restriction",
           :dependent => :destroy
  has_many :milters, :through => :restrictions

  class << self
    def apply(conditions_config)
      return if conditions_config.blank?

      transaction do
        names = []
        conditions_config.each do |condition_config|
          name = condition_config["name"]
          names << name
          description = condition_config["description"]
          condition = find_by_name(name)
          if condition
            condition.update_attributes!(condition_config)
          else
            create!(condition_config)
          end
        end

        destroy_all(["name NOT IN (?)", names])
      end
    end
  end
end
