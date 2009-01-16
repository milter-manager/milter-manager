class RemoveTargetFromMilters < ActiveRecord::Migration
  def self.up
    remove_column :milters, :target_hosts
    remove_column :milters, :target_addresses
    remove_column :milters, :target_senders
    remove_column :milters, :target_recipients
    remove_column :milters, :target_headers
  end

  def self.down
    add_column :milters, :target_hosts, :text
    add_column :milters, :target_addresses, :text
    add_column :milters, :target_senders, :text
    add_column :milters, :target_recipients, :text
    add_column :milters, :target_headers, :text
  end
end
