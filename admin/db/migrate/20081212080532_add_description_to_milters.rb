class AddDescriptionToMilters < ActiveRecord::Migration
  def self.up
    add_column :milters, :description, :text
  end

  def self.down
    remove_column :milters, :description
  end
end
