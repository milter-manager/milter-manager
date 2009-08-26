class AddEvaluationModeToMilters < ActiveRecord::Migration
  def self.up
    add_column :milters, :evaluation_mode, :boolean
  end

  def self.down
    remove_column :milters, :evaluation_mode
  end
end
