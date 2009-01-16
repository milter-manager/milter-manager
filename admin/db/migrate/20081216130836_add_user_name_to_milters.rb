class AddUserNameToMilters < ActiveRecord::Migration
  def self.up
    add_column :milters, :user_name, :string
  end

  def self.down
    remove_column :milters, :user_name
  end
end
