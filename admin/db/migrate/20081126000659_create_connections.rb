class CreateConnections < ActiveRecord::Migration
  def self.up
    create_table :connections do |t|
      t.string :name
      t.string :spec

      t.timestamps
    end
  end

  def self.down
    drop_table :connections
  end
end
