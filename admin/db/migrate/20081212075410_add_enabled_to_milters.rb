class AddEnabledToMilters < ActiveRecord::Migration
  def self.up
    add_column :milters, :enabled, :boolean
  end

  def self.down
    remove_column :milters, :enabled
  end
end
