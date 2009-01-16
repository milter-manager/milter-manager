class AddCommandOptionsToMilters < ActiveRecord::Migration
  def self.up
    add_column :milters, :command_options, :string
  end

  def self.down
    remove_column :milters, :command_options
  end
end
