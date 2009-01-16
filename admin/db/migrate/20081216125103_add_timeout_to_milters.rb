class AddTimeoutToMilters < ActiveRecord::Migration
  def self.up
    add_column :milters, :connection_timeout, :double
    add_column :milters, :writing_timeout, :double
    add_column :milters, :reading_timeout, :double
    add_column :milters, :end_of_message_timeout, :double
  end

  def self.down
    remove_column :milters, :connection_timeout
    remove_column :milters, :writing_timeout
    remove_column :milters, :reading_timeout
    remove_column :milters, :end_of_message_timeout
  end
end
