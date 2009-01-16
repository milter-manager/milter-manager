class AddConnectionSpecToMilters < ActiveRecord::Migration
  def self.up
    add_column :milters, :connection_spec, :string
  end

  def self.down
    remove_column :milters, :connection_spec
  end
end
