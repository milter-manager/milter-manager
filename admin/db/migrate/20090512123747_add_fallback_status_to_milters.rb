class AddFallbackStatusToMilters < ActiveRecord::Migration
  def self.up
    add_column(:milters, :fallback_status, :string)
  end

  def self.down
    remove_column(:milters, :fallback_status)
  end
end
